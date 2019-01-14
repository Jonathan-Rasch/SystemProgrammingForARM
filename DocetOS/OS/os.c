#include "os.h"
#include "os_internal.h"
#include "stm32f4xx.h"
#include "channelManger.h"
#include "memcluster.h"
#include "stochasticScheduler.h"
#include <stdlib.h>
#include <string.h>
__align(8)
/* Idle task stack frame area and TCB.  The TCB is not declared const, to ensure that it is placed in writable
   memory by the compiler.  The pointer to the TCB _is_ declared const, as it is visible externally - but it will
   still be writable by the assembly-language context switch. */
static OS_StackFrame_t const volatile _idleTaskSF;
static OS_TCB_t OS_idleTCB = { (void *)(&_idleTaskSF + 1), 0, 0, 0 };
OS_TCB_t const * const OS_idleTCB_p = &OS_idleTCB;

/* Total elapsed ticks */
static volatile uint32_t _ticks = 0;

/* Pointer to the 'scheduler' struct containing callback pointers */
static OS_Scheduler_t const * _scheduler = 0;

/* Pointer to the memcluster. used for allocating memory */
static OS_memcluster_t _memcluster;

/* pointer to channel manager struct. Manages channels used for inter task communication*/
static OS_channelManager_t const *_channelManager = 0;

/* GLOBAL: check code used by wait() function to determine if wait is still needed or if notify has been called in the interim*/
static volatile uint32_t  _checkCode = 0;
uint32_t OS_checkCode(void){
	return _checkCode;
}

/* GLOBAL: Holds pointer to current TCB.  DO NOT MODIFY, EVER. */
OS_TCB_t * volatile _currentTCB = 0;
/* Getter for the current TCB pointer.  Safer to use because it can't be used
   to change the pointer itself. */
OS_TCB_t * OS_currentTCB() {
	return _currentTCB;
}

/* Getter for the current time. */
uint32_t OS_elapsedTicks() {
	return _ticks;
}

/* IRQ handler for the system tick.  Schedules PendSV */
void SysTick_Handler(void) {
	_ticks = _ticks + 1;
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler for OS_schedule().  Simply schedules PendSV */
void _svc_OS_schedule(void) {
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* Sets up the OS by storing a pointer to the structure containing all the callbacks.
   Also establishes the system tick timer and interrupt if preemption is enabled. */
void OS_init(OS_Scheduler_t const * scheduler,uint32_t * memory,uint32_t memory_size) {
	_scheduler = scheduler;
	_channelManager = &channelManager;
  *((uint32_t volatile *)0xE000ED14) |= (1 << 9); // Set STKALIGN
	ASSERT(_scheduler->scheduler_callback);
	ASSERT(_scheduler->addtask_callback);
	ASSERT(_scheduler->taskexit_callback);
	ASSERT(_scheduler->wait_callback);
	ASSERT(_scheduler->notify_callback);
	memory_cluster_init(&_memcluster,memory,memory_size); //TODO why &_memcluster ?
	initialize_scheduler(16);
	initialize_channelManager(16);
}

/* OS_alloc allocates num_32bit_words words of memory if possible.
 *
 * RETURNS: pointer to the allocated memory if successful, else it returns NULL*/
void * OS_alloc(uint32_t num_32bit_words){
	return _memcluster.allocate(num_32bit_words);
}

void OS_free(uint32_t* head_ptr){
	_memcluster.deallocate((uint32_t*)head_ptr);
}

uint32_t OS_isMemclusterInUse(){
	return _memcluster.clusterInUseFLAG;
}

/* Starts the OS and never returns. */
void OS_start() {
	ASSERT(_scheduler);
	// This call never returns (and enables interrupts and resets the stack)
	_task_init_switch(OS_idleTCB_p);
}

/* Initialises a task control block (TCB) and its associated stack.  See os.h for details. */
void OS_initialiseTCB(OS_TCB_t * TCB, uint32_t * const stack, void (* const func)(void const * const), void const * const data) {
	TCB->originalSpMemoryPointer = stack - 64; //needed for dealloc of stack later on
	TCB->sp = stack - (sizeof(OS_StackFrame_t) / sizeof(uint32_t));
	TCB->priority = TCB->inheritedPriority = TCB->prevInheritedPriority = TCB->state = TCB->data = 0;
	OS_StackFrame_t *sf = (OS_StackFrame_t *)(TCB->sp);
	memset(sf, 0, sizeof(OS_StackFrame_t));
	/* By placing the address of the task function in pc, and the address of _OS_task_end() in lr, the task
	   function will be executed on the first context switch, and if it ever exits, _OS_task_end() will be
	   called automatically */
	sf->lr = (uint32_t)_OS_task_end;
	sf->pc = (uint32_t)(func);
	sf->r0 = (uint32_t)(data);
	sf->psr = 0x01000000;  /* Sets the thumb bit to avoid a big steaming fault */
}

/* Function that's called by a task when it ends (the address of this function is
   inserted into the link register of the initial stack frame for a task).  Invokes a SVC
   call (see os_internal.h); the handler is _svc_OS_task_exit() (see below). */
void _OS_task_end(void) {
	_OS_task_exit();
	/* DO NOT STEP OUT OF THIS FUNCTION when debugging!  PendSV must be allowed to run
	   and switch tasks.  A hard fault awaits if you ignore this.
		 If you want to see what happens next, or debug something, set a breakpoint at the
	   start of PendSV_Handler (see os_asm.s) and hit 'run'. */
}

/* SVC handler to enable systick from unprivileged code */
void _svc_OS_enable_systick(void) {
	if (_scheduler->preemptive) {
		SystemCoreClockUpdate();
		SysTick_Config(SystemCoreClock / 1000);
		NVIC_SetPriority(SysTick_IRQn, 0x10);
	}
}

/*ME: quick overview of how software interrupt works (as reminder for myself)

1) task calls svc delegate defined in os.h
		e.g "void __svc(OS_SVC_WAIT) OS_wait(void * reason)", where OS_SVC_WAIT is a constant used to identify the appropriate handler later on

2) if CMSIS compiler sees __svc(CODE) it:
		2.1) pushes to corresponding stack (Main stack pointer (MSP) for handler / process stack pointer (PSP) for thread mode code)
		-> it pushes arguments of the function into r0-r3 (in this case r0 = reason)
		-> pushes r12 (Intra Procedure call scratch Register) running ocde might be using it as temporary storage !
		-> pushes link-register (LR) (task might be executing some function, when the interrupt handler returns we want to know where that function was meant to return to !)
		-> pushes programm-counter (PC) (after returning from interrupt we want to know what instruction to continue at)
		-> pushes programm-status-register (PSR) (contains information such as priviledge level, by poping it back later we restor it)
NOTE: this is all esentially because the current task "called" the interrupt and needs to follow the calling conventions, ie preserving r0-r3,r12. the interrupt handler must then ensure
			to preserve r4-r11,LR and SP

		2.2) a special code is loaded into the LR which indicates what stack the code was using (MSP/PSP) before the interrupt occured, it also causes the registers that were
			pushed to the stacked due to the interrupt to be poped again once the interrupt handler is done and branches back.
		2.3) MSP is selected, CPU enters priviledged mode and executes SVC_Handler (in os_asm)

3) SVC_Handler begins:
		-> determines what stack was in use by using bit at index 2 in the special code now stored in LR
		-> loads that stack pointer into r0 
		-> goes back through stack to obtain code that SVC was called with
		-> looks in SVC_table for the given code, if code exists it branches to the corresponding handler function in os.c (if not it branches back to line after svc was called,
				unstacking registers to restore state in the process)
		
4) handler function is now running in priviledged mode and can call the relevant callback. at this point r0 still contains the stack pointer (points at ), so if the handler has an argument
	we are able to access this value. from this we can then access the rest of the stack too (r0 points at first stacked element (Full descending stack)).

*/

//=============================================================================
// scheduler svc
//=============================================================================

/* SVC handler for OS_wait()*/
void _svc_OS_wait(_OS_SVC_StackFrame_t const * const stack){
	void * reason = (void *)stack->r0;
	uint32_t checkCode = (uint32_t)stack->r1;
	uint32_t isReasonMutex = (uint32_t)stack->r2;
	_scheduler->wait_callback(reason, checkCode,isReasonMutex);
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler for OS_notify()*/
void _svc_OS_notify(_OS_SVC_StackFrame_t const * const stack){
	void * reason = (void *)stack->r0;
	uint32_t notStored = 1;
	/* following do-while makes sure that every notify results in a change to _checkCode.
	(even though this is not a problem at the moment since no higher priority interrupt does anything with notify() )*/
	do{
		uint32_t temp_checkCode = (uint32_t)__LDREXW((uint32_t *)&_checkCode);
		temp_checkCode++; // change check code to inform any wait() calls that notify has been called in interim
		notStored = __STREXW(temp_checkCode,(uint32_t *)&_checkCode);//returns 0 when success
	}while(notStored);
	_scheduler->notify_callback(reason);
}

/* SVC handler to add a task.  Invokes a callback to do the work. */
void _svc_OS_addTask(_OS_SVC_StackFrame_t const * const stack) {
	/* The TCB pointer is on the stack in the r0 position, having been passed as an
	   argument to the SVC pseudo-function.  SVC handlers are called with the stack
	   pointer in r0 (see os_asm.s) so the stack can be interrogated to find the TCB
	   pointer. */
	uint32_t task_priority = stack->r1;
	_scheduler->addtask_callback((OS_TCB_t *)stack->r0,task_priority);
}

/*notifies the scheduler that the task is requesting sleep for AT LEAST min_requested_sleep_duration but the
actual time the task sleeps might be longer. Sleep just guarantees that the task wont be executed for at least
min_requested_sleep_duration ticks*/
void _svc_OS_sleep(_OS_SVC_StackFrame_t const * const stack){
	OS_TCB_t * task = _currentTCB;
	uint32_t min_requested_sleep_duration = stack->r0;
	_scheduler->sleep_callback(task,min_requested_sleep_duration);
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // dont want to continue execution of task after sleep call
}

/* SVC handler to invoke the scheduler (via a callback) from PendSV */
OS_TCB_t const * _OS_scheduler() {
	return _scheduler->scheduler_callback();
}

/* SVC handler that's called by _OS_task_end when a task finishes.  Invokes the
   task end callback and then queues PendSV to call the scheduler. */
void _svc_OS_task_exit(void) {
	_scheduler->taskexit_callback(_currentTCB);
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler for OS_yield().  Sets the TASK_STATE_YIELD flag and schedules PendSV */
void _svc_OS_yield(void) {
	_currentTCB->state |= TASK_STATE_YIELD;
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

void _svc_OS_resource_acquired(_OS_SVC_StackFrame_t const * const stack) {
	OS_mutex_t * resource = (OS_mutex_t *)stack->r0;
	_scheduler->resourceAcquired_callback(resource);
}

//=============================================================================
// channel manager svc
//=============================================================================

//wrappers around svc delegate to make usage easier
OS_channel_t * OS_channel_connect(uint32_t _channelID,uint32_t _capacity){
	__OS_channel_connect(_channelID,_capacity);
	OS_channel_t * channel = (OS_channel_t *)OS_currentTCB()->svc_return;
	return channel;
}

uint32_t OS_channel_disconnect(uint32_t _channelID){
	__OS_channel_disconnect(_channelID);
	uint32_t returnVal = OS_currentTCB()->svc_return;
	return returnVal;
}

uint32_t OS_channel_check(uint32_t _channelID){
	__OS_channel_check(_channelID);
	uint32_t returnVal = OS_currentTCB()->svc_return;
	return returnVal;
}

//functions called in handler mode

void _svc_OS_channelManager_connect(_OS_SVC_StackFrame_t const * const stack){
	OS_TCB_t * tcb = OS_currentTCB();
	uint32_t channelID = stack->r0;
	uint32_t capacity = stack->r1;
	OS_channel_t * channel = _channelManager->connect_callback(channelID,capacity);
	tcb->svc_return = (uint32_t)channel;
}

void _svc_OS_channelManager_disconnect(_OS_SVC_StackFrame_t const * const stack){
	OS_TCB_t * tcb = OS_currentTCB();
	uint32_t channelID = stack->r0;
	uint32_t return_val = _channelManager->disconnect_callback(channelID);
	tcb->svc_return = return_val;
}

void _svc_OS_channelManager_checkAlive(_OS_SVC_StackFrame_t const * const stack){
	OS_TCB_t * tcb = OS_currentTCB();
	uint32_t channelID = stack->r0;
	uint32_t return_val = _channelManager->isAlive_callback(channelID);
	tcb->svc_return = return_val;
}
