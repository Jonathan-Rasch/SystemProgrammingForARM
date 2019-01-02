#include "patientPreemptivePriorityScheduler.h"
#include "../utils/heap.h"
#include "mutex.h"
#include <stdlib.h>
#include "hashtable.h"
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
static uint32_t __removeIfWaiting(uint32_t _index);

//things needed for creating the heap used in the scheduler
static minHeap * heapStruct;
/*waitingTasksHashTable
holds the TCB of all tasks that have entered the waiting state.
NOTE: a task being in this hashtable does not mean it is not present in the heap !! the scheduler only removes waiting tasks from the 
heap if it attemps to select them during task switch. This is to save cycles that would otherwise be wasted removing every single task from the 
heap that is waiting and restoring the heap property. As long as task is not selected it is ok for it to be in the heap, it might wake up before the
the heap gets around to removing it.*/
static OS_hashtable * waitingTasksHashTable;
/*activeTasksHashTable
holds the TCB of all tasks that are currently active and can be selected by the scheduler*/
static OS_hashtable * activeTasksHashTable;
/*tasksInHeapHashTable
keeps track of all tasks that are in the heap, active or otherwise. This is needed to prevent a task that exits the waiting state getting
added to the heap even though it was never removed in the first place.*/
static OS_hashtable * tasksInHeapHashTable;

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
void initialize_scheduler(OS_memcluster * _memcluster,uint32_t _size_of_heap_node_array){
		waitingTasksHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
		activeTasksHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
		tasksInHeapHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
    heapStruct = initHeap(_memcluster,_size_of_heap_node_array); //TODO check if correct size is passed down here
		srand(OS_elapsedTicks());//pseudo random num, ok since this is not security related so dont really care
}

//=============================================================================
// SCHEDULER FUNCTION
//=============================================================================
/* Determines which of the AWAKE and NOT WAITING tasks should be executed. The choice of task is RANDOM,
but the probability of each task being selected corresponds to its position in the heap. So tasks with a low priority 
(low down in the heap) will have a small but non-zero chance of getting cpu time.
*/
static OS_TCB_t const * patientPreemptivePriority_scheduler(void){
    static int ticksSinceLastTaskSwitch = 0;// used to force task to yield after MAX_TASK_TIME_IN_SYSTICKS
		OS_TCB_t * currentTaskTCB = OS_currentTCB();
	
    /*check if task has yielded or force it to yield if it has exceeded max allowed task time*/
		if( currentTaskTCB != OS_idleTCB_p ){
			uint32_t hasCurrentTaskYielded = currentTaskTCB->state & TASK_STATE_YIELD;
			uint32_t isCurrentTaskWaiting = currentTaskTCB->state & TASK_STATE_WAIT;
			ticksSinceLastTaskSwitch += 1;
			if(!hasCurrentTaskYielded && !isCurrentTaskWaiting){
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
		if(heapStruct->currentNumNodes == 0){
			return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
		}
		OS_TCB_t * selectedTCB = currentTaskTCB;
		{
			int randNodeSelection = __getRandForTaskChoice(); // please see function definition for info on return values
			int nodeIndex = 0; //starting with highest priority node
			int maxValidHeapIdx = heapStruct->currentNumNodes - 1;
			/*check if highest node is waiting, remove if it is, repeat until top node is not waiting*/
			while(__removeIfWaiting(0)){
				if(heapStruct->currentNumNodes == 0){
					/*after removing the top node from the heap it turns out that the heap is empty (all tasks waiting or asleep)
					hence return idle task*/
					return OS_idleTCB_p;
				}
			}
			int lastNotWaitingNodeIndex = nodeIndex;
			while(randNodeSelection){
				/*current node not selected, one of its children has been selected. check if node has valid child node*/
				int nextNodeIdx;
				if(randNodeSelection == 1){//left child
					nextNodeIdx = getFirstChildIndex(nodeIndex);
				}else{//right child
					nextNodeIdx = getSecondChildIndex(nodeIndex);
				}
				/*remove the current node if the task is waiting*/
				while(__removeIfWaiting(nodeIndex)){
					//waiting node removed
					maxValidHeapIdx = heapStruct->currentNumNodes - 1;
					if(nodeIndex > maxValidHeapIdx){
						/*after removing a waiting node at this index it turns out that this index no longer
						lies within the valid heap, return the last not sleeping task encountered in the heap*/
						return heapStruct->ptrToUnderlyingArray[lastNotWaitingNodeIndex].ptrToNodeContent;
					}
				}
				lastNotWaitingNodeIndex = nodeIndex;
				//check if selected child is valid node, if not, select parent
				if (nextNodeIdx <= maxValidHeapIdx){
					nodeIndex = nextNodeIdx;
					randNodeSelection = __getRandForTaskChoice();
					continue;
				}else{
					break;//nodeIndex not updated, so current node is selected for cpu time
				}
			}
			selectedTCB = heapStruct->ptrToUnderlyingArray[nodeIndex].ptrToNodeContent;
		}
//		printf("\r\n\r\n#####################################################################\r\n");
//		printf("\r\nACTIVE TASK HASH TABLE:\r\n");
//		DEBUG_printHashtable(activeTasksHashTable);
//		printf("\r\nWAITING TASK HASH TABLE:\r\n");
//		DEBUG_printHashtable(waitingTasksHashTable);
//		printf("\r\nTASKS IN HEAP HASH TABLE:\r\n");
//		DEBUG_printHashtable(tasksInHeapHashTable);
//		printHeap(heapStruct);
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
	#ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
	printf("\r\nACTIVE TASK HASH TABLE:\r\n");
	DEBUG_printHashtable(activeTasksHashTable);
	#endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
	tcb->priority = task_priority;
	hashtable_put(activeTasksHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	hashtable_put(tasksInHeapHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	if ( addNode(heapStruct,tcb,task_priority)) {
		/*HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY prevent the same task (TCB=key) being added multiple times to the scheduler */
		#ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
			printf("TASK ADDED: %p %d\r\n",tcb,task_priority);
			printf("SCHEDULER: added task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d)\r\n",tcb,task_priority,heapStruct->currentNumNodes);
		#endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
	}else{
		#ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
				printf("SCHEDULER: unable to add task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d)\r\n",tcb,task_priority,heapStruct->currentNumNodes);
		#endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
	}
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
		printf("test\r\n");
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

/*Marks the current task as waitin and adds it to the hashtable that keeps track of all waiting tasks. The Task is not
actually removed from the heap, this is done by the scheduler.*/
static void patientPreemptivePriority_waitCallback(void * const _reason, uint32_t checkCode){
	if (checkCode != OS_checkCode()){
		return;//checkcode mismatch, notify called during function that uses wait
	}
	//printf("\r\nINFO: task %p starting to wait for %p ...\r\n",OS_currentTCB(),_reason);
	/*add current task to the waiting hash_table*/
	OS_TCB_t * currentTCB = OS_currentTCB();
	hashtable_remove(activeTasksHashTable,(uint32_t)currentTCB);
	hashtable_put(waitingTasksHashTable,(uint32_t)_reason,(uint32_t *)currentTCB,HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY);
	/*HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY because mutex is used as key, and the same task is allowed to wait on more than one mutex in theory,
	but the same task cannot wait multiple times on the same mutex*/
	#ifdef PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG
	printf("\r\nACTIVE TASK HASH TABLE:\r\n");
	DEBUG_printHashtable(activeTasksHashTable);
	printf("\r\nWAITING TASK HASH TABLE:\r\n");
	DEBUG_printHashtable(waitingTasksHashTable);
	#endif /*PATIENTPREEMPTIVEPRIORITYSCHEDULER_DEBUG*/
	//TODO: if hashtable is full (returnCode = 0) make task sleep instead, for now assume waitinTasksHashTable can hold max number of tasks
	currentTCB->state |= TASK_STATE_WAIT;
}

static void patientPreemptivePriority_notifyCallback(void * const reason){
	/*add all the tasks that are waiting for the given reason back to the scheduler heap*/
	//printf("\r\n>>>>>>NOTIFY: %p <<<<<<\r\n",reason);
	if(waitingTasksHashTable == NULL){
		return;//during initialisation locks are released calling notify, but hash table is not created yet
	}
	while(1){
		OS_TCB_t * task = (OS_TCB_t*)hashtable_remove(waitingTasksHashTable,(uint32_t)reason);
		if(task == NULL){
			break;
		}
		hashtable_put(activeTasksHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
		task->state &= ~TASK_STATE_WAIT;
		if(hashtable_put(tasksInHeapHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
			/*tcb was added to "tasksInHeapHashTable" meaning that it currently is not in the heap (if it had never been removed from the heap
			the hashtable_put operation would have returned 0 when attempting to add it again), hence add it*/
			addNode(heapStruct,task,task->priority);
		}
	}
}

//=============================================================================
// Internal utility functions
//=============================================================================
/* Used for picking which task to give cpu time. returns 3 possible values:
0: pick the task at the current index in heap
1: go to left child of current node in the heap
2: go to right child of current node
*/
static int __getRandForTaskChoice(void){
	return rand() % 3;
}

/* Checks if a given task is currently waiting. If task is waiting it is removed from the 
heap used by the scheduler (heap property restored in the process).

returns: 1 if the task at_index was waiting and has been removed, 0 otherwise.
*/
static void * waitingNode;
static uint32_t __removeIfWaiting(uint32_t _index){
	OS_TCB_t * task = heapStruct->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(task->state & TASK_STATE_WAIT){
		removeNodeAt(heapStruct,_index,&waitingNode);
		hashtable_remove(tasksInHeapHashTable,(uint32_t)task);
		return 1;
	}else{
		return 0;
	}
}
