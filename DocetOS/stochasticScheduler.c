#include "stochasticScheduler.h"
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
// vars
//=============================================================================

//FLAGS
static uint32_t systick_rollover_detected_FLAG = 0;

//TASK HEAP RELATED
static minHeap * taskHeap;
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

//SLEEP HEAP RELATED
static minHeap * sleepHeap;
/*Keeps track of tasks that requested sleep. tasks that requested sleep but have not been encountered by the scheduler (trying to run the task that has requested sleep)
might remain in the scheduler heap (might never leave it if their sleep time expires before scheduler encounters them, in which case scheduler will unset sleep state and 
remove the task from this hashtable).*/
static OS_hashtable * sleepingTasksHashTable;

//=============================================================================
// prototypes
//=============================================================================

//svc accessible
static OS_TCB_t const * stochasticScheduler_scheduler(void);
static void stochasticScheduler_addTask(OS_TCB_t * const tcb,uint32_t task_priority);
static void stochasticScheduler_taskExit(OS_TCB_t * const tcb);
static void stochasticScheduler_waitCallback(void * const _reason, uint32_t checkCode);
static void stochasticScheduler_notifyCallback(void * const reason);
static void stochasticScheduler_sleepCallback(OS_TCB_t * const tcb,uint32_t min_sleep_duration);

//internal
static int __getRandForTaskChoice(void);
static uint32_t __removeIfWaiting(uint32_t _index);
static uint32_t __removeIfSleeping(uint32_t _index);
static uint32_t __updateSleepState(OS_TCB_t * task);

//debug
static void DEBUG_hashTableState();

//=============================================================================
// Scheduler struct definition and init function
//=============================================================================

/*scheduler struct definition*/
OS_Scheduler_t const stochasticScheduler = {
        .preemptive = 1,
        .scheduler_callback = stochasticScheduler_scheduler,
        .addtask_callback = stochasticScheduler_addTask,
        .taskexit_callback = stochasticScheduler_taskExit,
				.wait_callback = stochasticScheduler_waitCallback,
				.notify_callback = stochasticScheduler_notifyCallback,
				.sleep_callback = stochasticScheduler_sleepCallback
};

void initialize_scheduler(OS_memcluster * _memcluster,uint32_t _size_of_heap_node_array){
	/* initializing heaps needed to keep track of tasks in various states. Carefull consideration needs to be taken
	when deciding on the capacity of these hashtables since if they are too small it might prevent wait() and sleep() from working
	(task cant be removed from heap if there is no space in waitingTasksHashTable since the pointer to the TCB would be lost)
	
	choosing the bucket size is not as important, but fewer buckets lead to increased item access time*/
	waitingTasksHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	activeTasksHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	tasksInHeapHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	sleepingTasksHashTable = new_hashtable(_memcluster,WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	//other init stuff
	taskHeap = initHeap(_memcluster,_size_of_heap_node_array);
	sleepHeap = initHeap(_memcluster,_size_of_heap_node_array);
	srand(OS_elapsedTicks());//pseudo random num, ok since this is not security related so dont really care
}

//=============================================================================
// SCHEDULER FUNCTION
//=============================================================================
/* Determines which of the AWAKE and NOT WAITING tasks should be executed. The choice of task is RANDOM,
but the probability of each task being selected corresponds to its position in the heap. So tasks with a low priority 
(low down in the heap) will have a small but non-zero chance of getting cpu time.
*/
static OS_TCB_t const * stochasticScheduler_scheduler(void){
    static int ticksSinceLastTaskSwitch = 0;// used to force task to yield after MAX_TASK_TIME_IN_SYSTICKS
		OS_TCB_t * currentTaskTCB = OS_currentTCB();
	
    /*check if task has yielded, is waiting or sleeping. If not then force it to yield if it has exceeded max allowed task time*/
		if( currentTaskTCB != OS_idleTCB_p ){
			uint32_t hasCurrentTaskYielded = currentTaskTCB->state & TASK_STATE_YIELD;
			uint32_t isCurrentTaskWaiting = currentTaskTCB->state & TASK_STATE_WAIT;
			uint32_t isCurrentTaskSleeping = currentTaskTCB->state & TASK_STATE_SLEEP;
			ticksSinceLastTaskSwitch += 1;
			if(!hasCurrentTaskYielded && !isCurrentTaskWaiting && !isCurrentTaskSleeping){
					// now check if task has run for more than max allowed time
					if(ticksSinceLastTaskSwitch <= MAX_TASK_TIME_IN_SYSTICKS){
							//task is allowed to continue running
							return currentTaskTCB;
					}
			}
		}
		
    /*Task has either yielded (or sleeping/waiting) or it exceeded its maximum allowed time, switch task*/
		//reset task state and task switch counter
    currentTaskTCB->state &= ~TASK_STATE_YIELD;// reset so task has chance of running after next task switch
		/*not resetting TASK_STATE_SLEEP or TASK_STATE_WAIT, these are reset elswhere upon certain conditions (i.e a lock getting released)
		occuring*/
    ticksSinceLastTaskSwitch = 0; //set to 0 so that next task can run for MAX_TASK_TIME_IN_SYSTICKS
		
		/*sleep heap update check must remain HERE, before checking if there are any tasks in the taskHeap, some tasks in sleepHeap might need to wake
		and move over to the taskHeap*/
		if(systick_rollover_detected_FLAG){
			/*cycle through all sleeping tasks and adjust their timestamp and remaining time*/
			for(int bucketInd=0;bucketInd<sleepingTasksHashTable->number_of_buckets;bucketInd++){
				const hashtable_value * element = hashtable_getFirstElementOfNthBucket(sleepingTasksHashTable,bucketInd);
				while(element){
					//TODO wont work, need to update priority
					OS_TCB_t * sleepingTask = (OS_TCB_t *)element->underlying_data;
					__updateSleepState(sleepingTask);
					element = (hashtable_value *)element->next_hashtable_value;
				}
			}
			/*reset flag*/
			systick_rollover_detected_FLAG = 0;
		}	
		void * sleepingTask;
		while(1){
			if(sleepHeap->currentNumNodes == 0){
				break;//no more nodes to check/update
			}
			removeNode(sleepHeap,&sleepingTask); //sleepingTask now points to task from sleepHeap with lowest remaining sleep time
			if(!__updateSleepState((OS_TCB_t*)sleepingTask)){
				/*task woke up*/
				if(hashtable_remove(sleepingTasksHashTable,(uint32_t)sleepingTask)){
					hashtable_put(activeTasksHashTable,(uint32_t)sleepingTask,(uint32_t*)sleepingTask,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
					hashtable_put(tasksInHeapHashTable,(uint32_t)sleepingTask,(uint32_t*)sleepingTask,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
					addNode(taskHeap,sleepingTask,((OS_TCB_t*)sleepingTask)->priority);
				}else{
					printf("\u001b[31m\r\nSCHEDULER: ERROR waking task %p failed, task is not in the sleepingTasksHashTable!\r\n",sleepingTask);
					DEBUG_hashTableState();
				}
			}else{
				//printf("SCHEDULER: task %p still asleep, remaining time %d\r\n",sleepingTask,((OS_TCB_t*)sleepingTask)->data2);
				/*task still asleep add it back to the sleep heap with updated priority. since the priority in the sleep heap is given
				by the tasks remaining sleep time we can stop here. If this task (which was at the top of the heap)
				is still sleeping then all others will also be still asleep*/
				addNode(sleepHeap,sleepingTask,((OS_TCB_t*)sleepingTask)->data2);
				break;
			}
		}	
		
		
		/*Is there any active task to run in the heap (THIS MUST RUN AFTER UPDATING SLEEP STATE! DONT MOVE THIS!)?*/
		if(taskHeap->currentNumNodes == 0){
			return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
		}
		/*Select random task to give cpu time to (probability of each task being selected based on its position in heap)*/
		OS_TCB_t * selectedTCB = NULL;
		{
			int randNodeSelection;
			int nodeIndex = 0; //starting with highest priority node
			int maxValidHeapIdx = taskHeap->currentNumNodes - 1;
			int lastActiveNodeIndex = 0;
			while(1){
				/*remove the current node if the task is waiting or sleeping*/
				if(__removeIfWaiting(nodeIndex) || __removeIfSleeping(nodeIndex) ){
					//waiting/sleeping node removed
					maxValidHeapIdx = taskHeap->currentNumNodes - 1;
					/*after removal of node there might be none left in which case we need to stop and return idle task*/
					if(taskHeap->currentNumNodes == 0){
						return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
					}
					/*after removing a waiting/sleeping node at this index it turns out that this index no longer
						lies within the valid heap, return the last not sleeping task encountered in the heap*/
					if(nodeIndex > maxValidHeapIdx){
						return taskHeap->ptrToUnderlyingArray[lastActiveNodeIndex].ptrToNodeContent;
					}
					/*sleeping or waiting node has been removed and the heap has been restored, this means that we need to
					rerun for the same index to check if the new node at the current index is sleeping or waiting*/
					continue;
				}else{
					/*we know the node at this index is neither sleeping or waiting, we can use it as a fallback for selection in case
					all of its children are sleeping or waiting*/
					lastActiveNodeIndex = nodeIndex;
				}
				/*Making the choice if the current node should be selected or if we should try and select one if its children (if the child
				is valid and not waiting or sleeping)*/
				randNodeSelection = __getRandForTaskChoice();
				if(!randNodeSelection){
					/*Current node selected (randNodeSelection==0). We previously checked that node at nodeIndex is not sleeping or waiting
					so we can be sure that current node is valid and stop the search here*/
					break;
				}
				/*current node not selected (randNodeSelection != 0), one of its children has been selected. check if node has valid child node*/
				int nextNodeIdx;
				if(randNodeSelection == 1){//left child
					nextNodeIdx = getFirstChildIndex(nodeIndex);
				}else{//right child
					nextNodeIdx = getSecondChildIndex(nodeIndex);
				}
				//check if selected child is valid node
				if (nextNodeIdx <= maxValidHeapIdx){
					nodeIndex = nextNodeIdx;
					continue;
				}else{
					break;//nodeIndex not updated, so current node is selected for cpu time
				}
			}
			selectedTCB = taskHeap->ptrToUnderlyingArray[nodeIndex].ptrToNodeContent;
		}
		if(selectedTCB == NULL){
			return OS_idleTCB_p;
		}else{
			return selectedTCB;
		}
}

//=============================================================================
// Task related function definitions
//=============================================================================

/* Adds task control block to the heap at a position determined by the specified priority.
Depending on the given priority the task has a higher or lower probability of being given cpu time.
NOTE: Schould there be no space on the heap the task is not added.
*/
static void stochasticScheduler_addTask(OS_TCB_t * const tcb,uint32_t task_priority){
	#ifdef stochasticScheduler_DEBUG
	printf("\r\nACTIVE TASK HASH TABLE:\r\n");
	DEBUG_printHashtable(activeTasksHashTable);
	#endif /*stochasticScheduler_DEBUG*/
	tcb->priority = task_priority;
	hashtable_put(activeTasksHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	hashtable_put(tasksInHeapHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	if ( addNode(taskHeap,tcb,task_priority)) {
		/*HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY prevent the same task (TCB=key) being added multiple times to the scheduler */
		#ifdef stochasticScheduler_DEBUG
			printf("TASK ADDED: %p %d\r\n",tcb,task_priority);
			printf("SCHEDULER: added task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d)\r\n",tcb,task_priority,taskHeap->currentNumNodes);
		#endif /*stochasticScheduler_DEBUG*/
	}else{
		#ifdef stochasticScheduler_DEBUG
				printf("SCHEDULER: unable to add task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d)\r\n",tcb,task_priority,taskHeap->currentNumNodes);
		#endif /*stochasticScheduler_DEBUG*/
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
static void stochasticScheduler_taskExit(OS_TCB_t * const tcb){
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
static void stochasticScheduler_waitCallback(void * const _reason, uint32_t checkCode){
	if (checkCode != OS_checkCode()){
		return;//checkcode mismatch, notify called during function that uses wait
	}
	//printf("\r\nINFO: task %p starting to wait for %p ...\r\n",OS_currentTCB(),_reason);
	/*add current task to the waiting hash_table*/
	OS_TCB_t * currentTCB = OS_currentTCB();
	if(hashtable_remove(activeTasksHashTable,(uint32_t)currentTCB) == NULL){
		printf("\u001b[31m\r\nSCHEDULER: ERROR, task %p requested wait for %p , but it is not an active task!\r\n",currentTCB,_reason);
		DEBUG_hashTableState();
	}
	if(!hashtable_put(waitingTasksHashTable,(uint32_t)_reason,(uint32_t *)currentTCB,HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY)){
		printf("\u001b[31m\r\nSCHEDULER: ERROR, task %p requested wait for %p , but it could not be added to the waitingTasksHashTable!\r\n",currentTCB,_reason);
		DEBUG_hashTableState();
	}
	/*HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY because mutex is used as key, and the same task is allowed to wait on more than one mutex in theory,
	but the same task cannot wait multiple times on the same mutex*/
	//TODO: if hashtable is full (returnCode = 0) make task sleep instead, for now assume waitinTasksHashTable can hold max number of tasks
	currentTCB->state |= TASK_STATE_WAIT;
}

static void stochasticScheduler_notifyCallback(void * const reason){
	/*add all the tasks that are waiting for the given reason back to the scheduler heap*/
	//printf("\r\n>>>>>>NOTIFY: %p <<<<<<\r\n",reason);
	if(waitingTasksHashTable == NULL){
		return;//during initialisation locks are released calling notify, but hash table is not created yet
	}
	while(1){
		OS_TCB_t * task = (OS_TCB_t*)hashtable_remove(waitingTasksHashTable,(uint32_t)reason);
		if(task == NULL){
			break; /*no task was waiting for _reason, this is normal behaviour*/
		}
		if(hashtable_put(activeTasksHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
			task->state &= ~TASK_STATE_WAIT;
		}else{
			printf("\u001b[31m\r\nSCHEDULER: ERROR, unable to add task %p which was previously waiting back to activeTasksHashTable!\r\n",task);
			DEBUG_hashTableState();
		}
		
		/*if the hashtable_put(tasksInHeapHashTable...) below fails (returns 0) that is expected behaviour. It simply means that a task
		requested wait but that it was never removed from the taskHeap because the scheduler did not come accross that node in the heap
		when searching for the next task to select (and therefore did not have a chance to remove it from the heap). */
		if(hashtable_put(tasksInHeapHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
			/*tcb was added to "tasksInHeapHashTable" meaning that it currently is not in the heap (if it had never been removed from the heap
			the hashtable_put operation would have returned 0 when attempting to add it again), hence add it*/
			addNode(taskHeap,task,task->priority);
		}
	}
}

static void stochasticScheduler_sleepCallback(OS_TCB_t * const tcb,uint32_t min_sleep_duration){
	/*First check if the sleep time is reasonable*/
	if(min_sleep_duration == 0){
		return;
	}
	if(hashtable_remove(activeTasksHashTable,(uint32_t)tcb)){
		/*set task state so that the scheduler can identify tasks that requested sleep and check if this
		condition still applies*/
		tcb->state |= TASK_STATE_SLEEP;
		tcb->data = OS_elapsedTicks(); /*what time did the task request the sleep ? needed to check if
		the task should wake up.*/
		tcb->data2 = min_sleep_duration;/*used to keep track of the remaining sleep duration*/
		hashtable_put(sleepingTasksHashTable,(uint32_t) tcb,(uint32_t*) tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
		/*task is not removed from the heap, this is done inside the scheduler callback schould the scheduler try to run 
		a task that is in the sleep state.*/
	}else{
		/*hashtable_remove returned NULL meaning that no element with the given key was present*/
		printf("\u001b[31m\r\nSCHEDULER: ERROR task %p called sleep, but it is not in the activeTasks HashTable!\r\n",tcb);
		DEBUG_hashTableState();
	}
	return;
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

If the task is sleeping it must be added to sleepingTasksHashTable prior to this func
being called or else the pointer to its tcb will be lost.

returns: 1 if the task at_index was waiting and has been removed, 0 otherwise.
*/
static void * removedWaitingNode;
static uint32_t __removeIfWaiting(uint32_t _index){
	OS_TCB_t * task = taskHeap->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(task->state & TASK_STATE_WAIT){
		removeNodeAt(taskHeap,_index,&removedWaitingNode);
		hashtable_remove(tasksInHeapHashTable,(uint32_t)task);
		return 1;
	}else{
		return 0;
	}
}

/* updates the state of a waiting task. sets the systick_rollover_detected_FLAG should it detect
a rollover of the systick counter. 

RETURNS:
1 = task is still sleeping
0 = task is not/no longer sleeping
*/
static uint32_t __updateSleepState(OS_TCB_t * task){
	if(!(task->state & TASK_STATE_SLEEP)){
		return 0;
	}
	/*update the remaining sleep duration*/
	uint32_t currentTime = OS_elapsedTicks();
	uint32_t lastSleepStateUpdate = task->data;
	uint32_t remainingTime = task->data2;
	/*determine elapsed time*/
	uint32_t deltaTime = 0;
	if(lastSleepStateUpdate > currentTime){
		/*systick timer has roled over, need to set flag to force scheduler to itterate through
		all elements in the sleepHeap and adjust the remainingTime and lastSleepStateUpdate timestamps to avoid
		those tasks from sleeping much MUCH (like up to 2^32 systicks) longer than desired.*/
		systick_rollover_detected_FLAG = 1;//TODO implement method to do the above mentioned
		uint32_t delta1 = (UINT32_MAX - lastSleepStateUpdate); /*time between last sleep update to task state and
			the moment the systick timer rolled over*/
		deltaTime = currentTime + delta1; // total elapsed time
	}else{
		deltaTime = currentTime - lastSleepStateUpdate;
	}
	task->data = currentTime;//updating update timestamp
	/*now determine if the task should wake up*/
	if(remainingTime <= deltaTime){
		/*yes update task state*/
		task->state &= ~TASK_STATE_SLEEP;
		task->data2 = 0;
		return 0;
	}else{
		/*nope, update state*/
		task->data2 -= deltaTime;
		return 1;
	}
}

/* checks if a task at _index in the heap is sleeping and remove it from the heap if
this is the case (heap property restored in the process). The task is then added to the
sleepHeap.

If the task is sleeping it must be added to sleepingTasksHashTable prior to this func
being called or else the pointer to its tcb will be lost.

RETURNS:
1 = task was sleeping and has been removed from heap and tasksInHeapHashTable
0 = task is awake, no change made
*/
static void * removedSleepingNode;
static uint32_t __removeIfSleeping(uint32_t _index){
	OS_TCB_t * task = taskHeap->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(__updateSleepState(task)){
		removeNodeAt(taskHeap,_index,&removedSleepingNode);
		hashtable_remove(tasksInHeapHashTable,(uint32_t)task);
		addNode(sleepHeap,task,task->data2);/*data2 keeps track of remaining sleep duration which
			the sleepHeap uses to order its elements. This means that tasks that are about to wake up are
			right at the top of the heap.*/
		return 1;
	}else{
		return 0;
	}
}

//=============================================================================
// DEBUG functions
//=============================================================================

static void DEBUG_hashTableState(){
	printf("\r\n\r\n######################################################################\r\n");
	printf("DEBUG: DUMPING CONTENT OF SCHEDULER HASH TABLES AND HALTING EXECUTION!\r\n");
	printf("\r\nACTIVE TASK HASH TABLE:");
	DEBUG_printHashtable(activeTasksHashTable);
	printf("\r\nTASKS IN SCHEDULER HEAP HASH TABLE:");
	DEBUG_printHashtable(tasksInHeapHashTable);
	printf("\r\nWAITING TASK HASH TABLE:\r\nNOTE: the key is the reason the task is waiting, multiple entries with same key are allowed provided their 'data' fields differ.\r\n");
	DEBUG_printHashtable(waitingTasksHashTable);
	printf("\r\nSLEEPING TASK HASH TABLE:");
	DEBUG_printHashtable(sleepingTasksHashTable);
	printf("\u001b[0m");
	ASSERT(0);
}
