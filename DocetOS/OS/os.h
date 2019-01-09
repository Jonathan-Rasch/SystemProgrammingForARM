#ifndef _OS_H_
#define _OS_H_

#include "task.h"
#include "sleep.h"
#include "../structs.h"
/********************/
/* Type definitions */
/********************/

/* A set of numeric constants giving the appropriate SVC numbers for various callbacks. 
   If this list doesn't match the SVC dispatch table in os_asm.s, BIG TROUBLE will ensue. */
enum OS_SVC_e {
	OS_SVC_ENABLE_SYSTICK=0x00,
	OS_SVC_ADD_TASK,
	OS_SVC_EXIT,
	OS_SVC_YIELD,
	OS_SVC_SCHEDULE,
	OS_SVC_WAIT,
	OS_SVC_NOTIFY,
	OS_SVC_SLEEP,
	OS_CHANNEL_CONNECT,
	OS_CHANNEL_DISCONNECT,
	OS_CHANNEL_CHECK
};

/* A structure to hold callbacks for a scheduler, plus a 'preemptive' flag */
typedef struct {
	uint_fast8_t preemptive;
	OS_TCB_t const * (* scheduler_callback)(void);//ME:called by SysTick or when task yields
	void (* addtask_callback)(OS_TCB_t * const newTask, uint32_t taskPriority);//ME:called by user...to add task to scheduler
	void (* taskexit_callback)(OS_TCB_t * const task);//ME:called automatically on task func return. DO NOT CALL MANUALLY
	void (* wait_callback)(void * const reason, uint32_t check_Code);
	void (* notify_callback)(void * const reason);
	void (* sleep_callback)(OS_TCB_t * const task,uint32_t min_duration);
} OS_Scheduler_t;

/***************************/
/* OS management functions */
/***************************/

/* Initialises the OS.  Must be called before OS_start().  The argument is a pointer to an
   OS_Scheduler_t structure (see above). */
void OS_init(OS_Scheduler_t const * scheduler,uint32_t * memory,uint32_t memory_size);

/* Starts the OS kernel.  Never returns. */
void OS_start(void);

/* Returns a pointer to the TCB of the currently running task. */
OS_TCB_t * OS_currentTCB(void);

/* Returns the number of elapsed systicks since the last reboot (modulo 2^32). */
uint32_t OS_elapsedTicks(void);

/* Returns check code used in OS_wait() to determine if wait is still needed*/
uint32_t OS_checkCode(void);

/*functions to allow the user to allocate and deallocate memory through
the memory cluster of the OS*/
void * OS_alloc(uint32_t num_32bit_words);
void OS_free(uint32_t * head_ptr);
/******************************************/
/* Task creation and management functions */
/******************************************/

/* Initialises a task control block (TCB) and its associated stack.  The stack is prepared
   with a frame such that when this TCB is first used in a context switch, the given function will be
   executed.  If and when the function exits, a SVC call will be issued to kill the task, and a callback
   will be executed.
   The first argument is a pointer to a TCB structure to initialise.
   The second argument is a pointer to the TOP OF a region of memory to be used as a stack (stacks are full descending).
     Note that the stack MUST be 8-byte aligned.  This means if (for example) malloc() is used to create a stack,
     the result must be checked for alignment, and then the stack size must be added to the pointer for passing
     to this function.
   The third argument is a pointer to the function that the task should execute.
   The fourth argument is a void pointer to data that the task should receive. */
void OS_initialiseTCB(OS_TCB_t * TCB, uint32_t * const stack, void (* const func)(void const * const), void const * const data);

//=============================================================================
// scheduler svc
//=============================================================================

/* SVC delegate to add a task */
void __svc(OS_SVC_ADD_TASK) OS_addTask(OS_TCB_t const * const,uint32_t task_priority);

/* SVC delegate to allow task to wait for resource*/
void __svc(OS_SVC_WAIT) OS_wait(void * reason, uint32_t check_Code);

/* SVC delegate to allow task to notify that a resource has been released*/
void __svc(OS_SVC_NOTIFY) OS_notify(void * reason);

void __svc(OS_SVC_SLEEP) OS_sleep(uint32_t min_sleep_duration);

//=============================================================================
// channel manager svc
//=============================================================================

OS_channel_t * __svc(OS_CHANNEL_CONNECT) OS_channel_connect(uint32_t channelID,uint32_t capacity);
OS_channel_t * __svc(OS_CHANNEL_DISCONNECT) OS_channel_disconnect(uint32_t channelID);
OS_channel_t * __svc(OS_CHANNEL_CHECK) OS_channel_check(uint32_t channelID);

/************************/
/* Scheduling functions */
/************************/

/* SVC delegate to yield the current task */
void __svc(OS_SVC_YIELD) OS_yield(void);

/****************/
/* Declarations */
/****************/

/* Idle task TCB */
extern OS_TCB_t const * const OS_idleTCB_p;

#endif /* _OS_H_ */

