#include "patientPreemptivePriorityScheduler.h"
#include <stdlib.h>

/*
 * The scheduler has the following features:
 * -> Task priority:
 *      ->  tasks that are not currently running and are NOT WAITING are placed in a max heap.
 *          when the current task (TaskA) yields, exits or the scheduler forces a context switch the task
 *          with the next highest priority (TaskB) becomes the current task. the previously running task is
 *          placed back into the max heap
 *
 */


//=============================================================================
// Prototypes and vars
//=============================================================================

//static functions, only visible to same translation unit
static OS_TCB_t const * patientPreemptivePriority_scheduler(void);
static void patientPreemptivePriority_addTask(OS_TCB_t * const tcb,uint32_t task_priority);
static void patientPreemptivePriority_taskExit(OS_TCB_t * const tcb);
static void patientPreemptivePriority_waitCallback(void * const _reason, uint32_t checkCode);
static void patientPreemptivePriority_notifyCallback(void * const reason);
static int __getRandForTaskChoice(void);

//things needed for creating the heap used in the scheduler
static minHeap heapStruct;
static OS_mutex_t heapLock;

//=============================================================================
// Scheduler struct definition and init function
//=============================================================================

/*scheduler struct definition*/
OS_Scheduler_t const patientPreemptivePriorityScheduler = {
        .preemptive = 1,
        .scheduler_callback = patientPreemptivePriority_scheduler,
        .addtask_callback = patientPreemptivePriority_addTask,
        .taskexit_callback = patientPreemptivePriority_taskExit,
				.wait_callback = patientPreemptivePriority_waitCallback,
				.notify_callback = patientPreemptivePriority_notifyCallback,
};

/*TODO init scheduler by just passing it pointer to mempool !*/
static void initialize_scheduler(OS_memcluster * _memcluster,uint32_t _size_of_heap_node_array){
    initHeap(_memcluster,&heapStruct, _size_of_heap_node_array); //TODO check if correct size is passed down here
		OS_init_mutex(&heapLock);//TODO: determine if a mutex is needed in the scheduler
		srand(OS_elapsedTicks());//pseudo random num, ok since this is not security related so dont really care
}

//=============================================================================
// SCHEDULER FUNCTION
//=============================================================================
/* Determines which of the AWAKE and NOT WAITING tasks should be executed. The choice of task is RANDOM,
but the probability of each task being selected corresponds to its position in the heap. So tasks with a low priority 
(low down in the heap) will have a small but non-zero chance of getting cpu time.

TODO: Need default task to run for when heap is empty.(round robin scheduler has one I think)
*/
static OS_TCB_t const * patientPreemptivePriority_scheduler(void){
    static int ticksSinceLastTaskSwitch = 0;// used to force task to yield after MAX_TASK_TIME_IN_SYSTICKS
		OS_TCB_t * currentTaskTCB = OS_currentTCB();
	
    /*check if task has yielded or force it to yield if it has exceeded max allowed task time*/
		if( currentTaskTCB != OS_idleTCB_p ){
			uint32_t hasCurrentTaskYielded = currentTaskTCB->state & TASK_STATE_YIELD;
			ticksSinceLastTaskSwitch += 1;
			if(!hasCurrentTaskYielded){
					// now check if task has run for more than max allowed time
					if(ticksSinceLastTaskSwitch <= MAX_TASK_TIME_IN_SYSTICKS){
							//task is allowed to continue running
							return currentTaskTCB;
					}
			}
		}
		
    /*Task has either yielded or it exceeded its maximum allowed time, switch task*/
		//reset task state and task switch counter
    currentTaskTCB->state &= ~TASK_STATE_YIELD;// reset so task has chance of running after next task switch
    ticksSinceLastTaskSwitch = 0; //set to 0 so that next task can run for MAX_TASK_TIME_IN_SYSTICKS
		
		/*Select random task to give cpu time to (probability of each task being selected based on its position in heap)*/
		if(heapStruct.currentNumNodes == 0){
			return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
		}
		OS_TCB_t * selectedTCB = currentTaskTCB;
		{
			int randNodeSelection = __getRandForTaskChoice(); // please see function definition for info on return values
			int nodeIndex = 0; //starting with highest priority node
			const int maxValidHeapIdx = heapStruct.currentNumNodes - 1;
			while(randNodeSelection){
				/*current node not selected, one of its children has been selected. check if node has valid child node*/
				int nextNodeIdx;
				if(randNodeSelection == 1){//left child
					nextNodeIdx = getFirstChildIndex(nodeIndex);
				}else{//right child
					nextNodeIdx = getSecondChildIndex(nodeIndex);
				}
				//check if selected child is valid node, if not, select parent
				if (nextNodeIdx <= maxValidHeapIdx){
					nodeIndex = nextNodeIdx;
					randNodeSelection = __getRandForTaskChoice();
					continue;
				}else{
					break;//nodeIndex not updated, so current node is selected for cpu time
				}
			}
			selectedTCB = heapStruct.ptrToUnderlyingArray[nodeIndex].ptrToNodeContent;
		}
		return selectedTCB;
}

//=============================================================================
// Task related function definitions
//=============================================================================

/* Adds task control block to the heap at a position determined by the specified priority.
Depending on the given priority the task has a higher or lower probability of being given cpu time.
NOTE: Schould there be no space on the heap the task is not added.
*/
static void patientPreemptivePriority_addTask(OS_TCB_t * const tcb,uint32_t task_priority){
	/* DISABELING INTERRUPTS FOR THE DURATION OF THIS FUNCTION. Reason:
	-> This scheduler is based on probability, so a low priority task (lets say TaskZ) still has a small probability of being run
	-> If TaskZ runs and calls "addTask" it will result in the heap property needing to be checked/restored.
	-> if that process gets interrupted by context switch to TaskB, which also calls "addTask" before heap is restored it could mess up the heap
	-> this means that the heap needs to be locked. This could be done via mutex.
	-> But if low priority TaskZ gets the lock, it might take a while for it to "randomly" get cpu time again (chance of being choosen based on task priority).
	-> all other tasks that need to work with the scheduler heap will be blocked in the meantime.
	-> hence it is more efficient to just prevent interrupts when heap is being worked on.*/
	__disable_irq();
    if (addNode(&heapStruct,tcb,task_priority)) {
        #ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
            printf("SCHEDULER: added task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d/%d)\r\n",tcb,task_priority,heapStruct.currentNumNodes,
                   sizeof(nodeArray));
        #endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
    }else{
        #ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
            printf("SCHEDULER: unable to add task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d/%d)\r\n",tcb,task_priority,heapStruct.currentNumNodes,
               sizeof(nodeArray));
        #endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
    }
	__enable_irq();
}


/* This function is automatically called when a task exits
 *
 * Task is started by loading the memory location of the corresponding function into the program counter so that on the
 * next cycle the execution begins. To ensure that this function runs at the end its address is loaded into the link
 * register for this task, so that upon termination of the function it branches back to the start of the task exit func.
 *
 * NOTE: this function should NEVER be called manually
 * */
static void patientPreemptivePriority_taskExit(OS_TCB_t * const tcb){

    /*TODO: I need an efficient way to search for a task inside the heap in order to remove it. I dont
     * just want to linear search through the node array and compare stack pointers. I could do this by
     * searching via the priority value, I could store node index inside heap array inside a hashmap
     *
     * key: Node priority
     * values: linked list of nodes with that priority value
     * node at each value: index of its current position in heap array (updated during swap)
     *
     * to remove a node:
     * 1) put node priority as key into hashmap, this returns a linked list of nodes
     * 2) traverse linked list comparing stack pointers until a match is found
     * 3) obtain the index of that node inside the heap (stored in linked list node)
     * 4) using the index remove the node directly from the heap
     *
     * this approach has a lower complexity than simple array traversal. It will result in larger time
     * needed if only a few tasks are present, but it has better complexity than O(n) of linear array
     * search
     * */
    //TODO INVESTIGATE: at what number of tasks does this method become more efficient than linear search ?
    /*^
     * cannot just use the number of nodes from the heap itself because this function is not called in PENDSV, and hence
     * could be interrupted, which could lead to the node size changing, resulting in the task not getting removed from
     * the heap!, hence the need to search through all elements
     *
     * TODO INVESTIGATE: this could still be a problem even when searching through entire array
     * -> should i disable interrupts during task exit?
     * -> set a flag in task TCB and let scheduler in handler mode sort this out?
     * -> or lock heap?*/   
}

//=============================================================================
// wait and notify hash table
//=============================================================================
#define NUM_BUCKETS_FOR_WAIT_HASHTABLE
static void patientPreemptivePriority_waitCallback(void * const _reason, uint32_t checkCode){
	
}

static void patientPreemptivePriority_notifyCallback(void * const reason){
	// TODO IMPLEMENT
}

//=============================================================================
// Internal utility functions
//=============================================================================
/* Used for picking which task to give cpu time. returns 3 possible values:
0: pick the task at the current index in heap
1: go to left child of current node in the heap
2: go to right child of current node
*/
int __getRandForTaskChoice(void){
	return rand() % 3;
}
