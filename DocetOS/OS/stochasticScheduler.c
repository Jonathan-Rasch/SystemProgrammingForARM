#include "stochasticScheduler.h"
#include "../DataStructures/heap.h"
#include "../DataStructures/mutex.h"
#include <stdlib.h>
#include "../DataStructures/hashtable.h"
#include "memcluster.h"


//=============================================================================
// vars
//=============================================================================

//FLAGS
static uint32_t systick_rollover_detected_FLAG = 0;

//TASK HEAP RELATED
static OS_minHeap_t * schedulerHeap;
/*waitingTasksHashTable_reasonAsKey
holds the TCB of all tasks that have entered the waiting state.

NOTE: a task being in this hashtable does not mean it is not present in the heap !! the scheduler only removes waiting tasks from the 
heap if it attempts to select them during task switch. This is to save cycles that would otherwise be wasted removing every single task from the
heap that is waiting and restoring the heap property. As long as task is not selected it is ok for it to be in the heap, it might wake up before the
the heap gets around to removing it.

 NOTE: keys in this hashtable are the reason of why the task is waiting. This allows quick access to all tasks that are waiting for a specific resource*/
static OS_hashtable_t * waitingTasksHashTable_reasonAsKey;
static OS_hashtable_t * waitingTasksHashTable_tcbAsKey;/* allows quick access to specific task that is waiting which is not possible via
 * waitingTasksHashTable_reasonAsKey since the reason for waiting is used as the key. both waitingTasksHashTable_tcbAsKey and waitingTasksHashTable_reasonAsKey
 * are identical in terms of the tcb pointers they contain. waitingTasksHashTable_tcbAsKey has the _reason as value corresponding to the key*/
/*activeTasksHashTable
holds the TCB of all tasks that are currently active and can be selected by the scheduler*/
static OS_hashtable_t * activeTasksHashTable;
/*tasksInSchedulerHeapHashTable
keeps track of all tasks that are in the heap, active or otherwise. This is needed to prevent a task that exits the waiting state getting
added to the heap even though it was never removed in the first place.*/
static OS_hashtable_t * tasksInSchedulerHeapHashTable;

//SLEEP HEAP RELATED
static OS_minHeap_t * sleepHeap;
/*Keeps track of tasks that requested sleep. tasks that requested sleep but have not been encountered by the scheduler (trying to run the task that has requested sleep)
might remain in the scheduler heap (might never leave it if their sleep time expires before scheduler encounters them, in which case scheduler will unset sleep state and 
remove the task from this hashtable).*/
static OS_hashtable_t * sleepingTasksHashTable;

static OS_TCB_t * comletedTasksLinkedList = NULL; /*stores tasks that are ready for deallocation*/

//=============================================================================
// prototypes
//=============================================================================

//svc accessible
static OS_TCB_t const * stochasticScheduler_scheduler(void);
static void stochasticScheduler_addTask(OS_TCB_t * const tcb,uint32_t task_priority);
static void stochasticScheduler_taskExit(OS_TCB_t * const tcb);
static void stochasticScheduler_waitCallback(void * const _reason, uint32_t checkCode,uint32_t _isReasonMutex);
static void stochasticScheduler_notifyCallback(void * const reason);
static void stochasticScheduler_sleepCallback(OS_TCB_t * const tcb,uint32_t min_sleep_duration);
static void resourceAcquired_callback( OS_mutex_t * _resource);

//internal
static int __getRandForTaskChoice(void);
static uint32_t __removeIfWaiting(uint32_t _index);
static uint32_t __removeIfSleeping(uint32_t _index);
static uint32_t __removeIfExit(uint32_t _index);
static uint32_t __updateSleepState(OS_TCB_t * task);
static void __updatePriorityInheritance(OS_TCB_t * task);

//debug
static void DEBUG_hashTableState();
static void DEBUG_heapState();

//=============================================================================
// init
//=============================================================================

/*scheduler struct init*/
OS_Scheduler_t const stochasticScheduler = {
		.preemptive = 1,
		.scheduler_callback = stochasticScheduler_scheduler,
		.addtask_callback = stochasticScheduler_addTask,
		.taskexit_callback = stochasticScheduler_taskExit,
		.wait_callback = stochasticScheduler_waitCallback,
		.notify_callback = stochasticScheduler_notifyCallback,
		.sleep_callback = stochasticScheduler_sleepCallback,
        .resourceAcquired_callback =resourceAcquired_callback
};

void initialize_scheduler(uint32_t _sizeOfHeapNodeArray){
	/* initializing heaps needed to keep track of tasks in various states. Carefull consideration needs to be taken
	when deciding on the capacity of these hashtables since if they are too small it might prevent wait() and sleep() from working
	(task cant be removed from heap if there is no space in waitingTasksHashTable_reasonAsKey since the pointer to the TCB would be lost)
	
	choosing the bucket size is not as important, but fewer buckets lead to increased item access time*/
	waitingTasksHashTable_reasonAsKey = new_hashtable(WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	waitingTasksHashTable_tcbAsKey = new_hashtable(WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	activeTasksHashTable = new_hashtable(WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	tasksInSchedulerHeapHashTable = new_hashtable(WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	sleepingTasksHashTable = new_hashtable(WAIT_HASHTABLE_CAPACITY,NUM_BUCKETS_FOR_WAIT_HASHTABLE);
	//other init stuff
	schedulerHeap = new_heap(_sizeOfHeapNodeArray,1);
	sleepHeap = new_heap(_sizeOfHeapNodeArray,0);
	srand(OS_elapsedTicks());//pseudo random num, ok since this is not security related so don't really care
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
	
	/*check if task has yielded, is waiting, sleeping or has exited. If not then force it to yield if it has exceeded max allowed task time.
	Otherwise allow it to continue running.*/
	if( currentTaskTCB != OS_idleTCB_p ){
		uint32_t isCurrentTaskDone = currentTaskTCB->state & TASK_STATE_EXIT;
		uint32_t hasTaskStateChanged = currentTaskTCB->state & (TASK_STATE_YIELD | TASK_STATE_WAIT | TASK_STATE_SLEEP);
		uint32_t hasRemainingExecutionTime = ticksSinceLastTaskSwitch < MAX_TASK_TIME_IN_SYSTICKS;
		ticksSinceLastTaskSwitch += 1;
		if(!isCurrentTaskDone && !hasTaskStateChanged && hasRemainingExecutionTime){
			//task is allowed to continue running
			return currentTaskTCB;
		}
	}
	
	/*free resources associated with tasks that have run to completion. These reources might not have yet been freed. This could be caused
	by the memcluster being in use at the time.*/
	if(comletedTasksLinkedList){
		__disable_irq();
		if(!OS_isMemclusterInUse()){
				while(comletedTasksLinkedList){
				OS_TCB_t * taskToDealloc = comletedTasksLinkedList;
				comletedTasksLinkedList = (OS_TCB_t *)taskToDealloc->data;
				/* CRITICAL SECTION START
				-> preventing internal mutexes of the memory cluster using svc callbacks
				-> disable interrupts whilst svc callbacks of locks are disabled*/
				memory_cluster_setInternalLockState(0);
				OS_free((uint32_t*)taskToDealloc->originalSpMemoryPointer);
				OS_free((uint32_t*)taskToDealloc);
				memory_cluster_setInternalLockState(1);
				/* CRITICAL SECTION END*/
			}
		}
		__enable_irq();
	}
	
	/*Task has either yielded, exited or is sleeping/waiting or it exceeded its maximum allowed time, switch task*/
	//reset YIELD state and task switch counter
	currentTaskTCB->state &= ~TASK_STATE_YIELD;// reset so task has chance of running after next task switch
	/*not resetting TASK_STATE_SLEEP or TASK_STATE_WAIT, these are reset elswhere upon certain conditions (i.e a lock getting released)
	occurring*/
	ticksSinceLastTaskSwitch = 0; //set to 0 so that next task can run for MAX_TASK_TIME_IN_SYSTICKS
	
	/*A rollover of the systick counter might have happened. If this is the case then the timestamp of when a task started to
	sleep needs to be adjusted becaus otherwise it might happen that a task has gone to sleep at 513 ticks but the current tick is 11, which means
	that the elapsed time since the task went to sleep is -502 ticks.
	
	The following block UPDATES THE SLEEP STATE OF ALL SLEEPING TASKS, regardless of their location (sleepHeap or schedulerHeap)*/
	if(systick_rollover_detected_FLAG){
		/*cycle through all sleeping tasks and adjust their timestamp and remaining time*/
		for(int bucketInd=0;bucketInd<sleepingTasksHashTable->numberOfBuckets;bucketInd++){
			const OS_hashtable_value_t * element = OS_hashtable_getFirstElementOfNthBucket(sleepingTasksHashTable,bucketInd);
			while(element){
				OS_TCB_t * sleepingTask = (OS_TCB_t *)element->underlyingData;
				__updateSleepState(sleepingTask);
				element = (OS_hashtable_value_t *)element->nextHashtableValue;
			}
		}
		/*reset flag*/
		systick_rollover_detected_FLAG = 0;
	}
	/*The following block removes the first node of the sleepHeap and updates its sleep state (remaining time etc), if this node
	is found to have woken it is removed from the sleepingTasksHashTable and added back to the activeTasksHashTable, tasksInSchedulerHeapHashTable
	and the schedulerHeap. If it has not woken it is simply added back to the sleepHeap and the block exits (if the top node is not awake then
	no other node will be awake either).
	
	It is important to note that this block ONLY UPDATES SLEEPING TASKS IN THE SLEEP HEAP, nodes that are asleep but have not been
	removed from the schedulerHeap (nodes present in sleepingTasksHashTable, tasksInSchedulerHeapHashTable and schedulerHeap but not in sleepHeap) are dealt with
	later in the scheduler function.*/
	void * sleepingTask;
	while(1){
		if(!OS_heap_removeNode(sleepHeap,&sleepingTask)){
			break;/*No nodes on the heap to wake*/
		}
		//sleepingTask now points to task from sleepHeap with lowest remaining sleep time
		if(__updateSleepState((OS_TCB_t*)sleepingTask)){
			/*task still asleep add it back to the sleep heap with updated priority. since the priority in the sleep heap is given
			by the tasks remaining sleep time we can stop here. If this task (which was at the top of the heap)
			is still sleeping then all others will also be still asleep*/
			OS_heap_addNode(sleepHeap,sleepingTask,((OS_TCB_t*)sleepingTask)->data2);
			break;
		}
	}	
	
	
	/*Is there any active task to run in the heap (THIS MUST RUN AFTER UPDATING SLEEP STATE! DONT MOVE THIS!)?*/
	if(schedulerHeap->currentNumNodes == 0){
		return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
	}
	/*Select random task to give cpu time to (probability of each task being selected based on its position in heap)*/
	OS_TCB_t * selectedTCB = NULL;
	{
		int randNodeSelection;
		int nodeIndex = 0; //starting with highest priority node
		int maxValidHeapIdx = schedulerHeap->currentNumNodes - 1;
		int lastActiveNodeIndex = 0;
		while(1){
			/*remove the current node if the task is waiting or sleeping*/
			if(__removeIfExit(nodeIndex) || __removeIfWaiting(nodeIndex) || __removeIfSleeping(nodeIndex) ){
				//waiting/sleeping node removed
				maxValidHeapIdx = schedulerHeap->currentNumNodes - 1;
				/*after removal of node there might be none left in which case we need to stop and return idle task*/
				if(schedulerHeap->currentNumNodes == 0){
					return OS_idleTCB_p;// no active tasks currently (maybe all sleeping).
				}
				/*after removing a waiting/sleeping node at this index it turns out that this index no longer
					lies within the valid heap, return the last not sleeping task encountered in the heap*/
				if(nodeIndex > maxValidHeapIdx){
					return schedulerHeap->ptrToUnderlyingArray[lastActiveNodeIndex].ptrToNodeContent;
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
				nextNodeIdx = OS_heap_getFirstChildIndex(nodeIndex);
			}else{//right child
				nextNodeIdx = OS_heap_getSecondChildIndex(nodeIndex);
			}
			//check if selected child is valid node
			if (nextNodeIdx <= maxValidHeapIdx){
				nodeIndex = nextNodeIdx;
				continue;
			}else{
				break;//nodeIndex not updated, so current node is selected for cpu time
			}
		}
		selectedTCB = schedulerHeap->ptrToUnderlyingArray[nodeIndex].ptrToNodeContent;
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
	if(tcb == NULL){
		printf("\r\nSCHEDULER: ERROR, attempt to add null pointer tcb to scheduler!\r\n");
		ASSERT(0);
		return;
	}else if(tcb->state != 0){
		printf("\r\nSCHEDULER: ERROR, cannot add tasks that have a state other than 0x00 to scheduler! The task you tried to add has state 0x%08x\r\n",tcb->state);
		ASSERT(0);
		return;
	}else if(task_priority == 0){
		printf("\r\nSCHEDULER: ERROR, tried to add task with priority 0, lowest allowed priority is 1!\r\n");
		ASSERT(0);
		return;
	}
	tcb->priority = task_priority;
	OS_hashtable_put(activeTasksHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	OS_hashtable_put(tasksInSchedulerHeapHashTable,(uint32_t)tcb,(uint32_t*)tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	if ( !OS_heap_addNode(schedulerHeap,tcb,task_priority)) {
		/*HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY prevent the same task (TCB=key) being added multiple times to the scheduler */
		printf("\r\nSCHEDULER: unable to add task (TCB:%p) with priority %d to scheduler. (scheduler task num= %d)\r\n",tcb,task_priority,schedulerHeap->currentNumNodes);
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
    tcb->state |= TASK_STATE_EXIT;
		tcb->data = NULL; // used in completed tasks linked list. Needs to be NULL if not pointing to other completed task
    /*It would be easy to release all mutexes held by the task here...not sure if advisable since the user might make a mistake
			whilst setting up a task which causes it to exit before releasing the locks. If I release the locks automatically it would
			hide the actual problem of the task exiting too soon, which in turn could make it a lot harder to debug.*/
}

//=============================================================================
// wait, notify and sleep
//=============================================================================

/*Marks the current task as waitin and adds it to the hashtable that keeps track of all waiting tasks. The Task is not
actually removed from the heap, this is done by the scheduler.*/
static void stochasticScheduler_waitCallback(void * const _reason, uint32_t checkCode,uint32_t _isReasonMutex){
	if (checkCode != OS_checkCode()){
		return;//checkcode mismatch, notify called during function that uses wait
	}
	//printf("\r\nINFO: task %p starting to wait for %p ...\r\n",OS_currentTCB(),_reason);
	/*add current task to the waiting hash_table*/
	OS_TCB_t * currentTCB = OS_currentTCB();
	if(OS_hashtable_remove(activeTasksHashTable,(uint32_t)currentTCB) == NULL){
		printf("\u001b[31m\r\nSCHEDULER: ERROR, task %p requested wait for %p , but it is not an active task!\r\n",currentTCB,_reason);
		DEBUG_hashTableState();
		DEBUG_heapState();
		printf("\u001b[0m");
		ASSERT(0);
	}
	if(!OS_hashtable_put(waitingTasksHashTable_reasonAsKey,(uint32_t)_reason,(uint32_t *)currentTCB,HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY)){
		printf("\u001b[31m\r\nSCHEDULER: ERROR, task %p requested wait for %p , but it could not be added to the waitingTasksHashTable_reasonAsKey!\r\n",currentTCB,_reason);
		DEBUG_hashTableState();
		DEBUG_heapState();
		printf("\u001b[0m");
		ASSERT(0);
	}else{
		/*mirroring waitingTaskHashTable_reasonAsKey*/
		OS_hashtable_put(waitingTasksHashTable_tcbAsKey,(uint32_t )currentTCB,(uint32_t *)_reason,HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY);
	}
	/*HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY because mutex is used as key, and the same task is allowed to wait on more than one mutex in theory,
	but the same task cannot wait multiple times on the same mutex*/
	currentTCB->state |= TASK_STATE_WAIT;

    /*start the priority inherritance */
    if(_isReasonMutex ){
        OS_mutex_t * mutex = (OS_mutex_t * ) _reason;
        OS_TCB_t * mutexOwner = mutex->tcbPointer;
        __updatePriorityInheritance(mutexOwner);
    }
}

static void stochasticScheduler_notifyCallback(void * const reason){
	/*add all the tasks that are waiting for the given reason back to the scheduler heap*/
	if(waitingTasksHashTable_reasonAsKey == NULL){
		return;//during initialisation locks are released calling notify, but hash table is not created yet
	}
	while(1){
		OS_TCB_t * task = (OS_TCB_t*)OS_hashtable_remove(waitingTasksHashTable_reasonAsKey,(uint32_t)reason);
		/*mirroring waitingTasksHashTable_reasonAsKey content*/
		OS_hashtable_remove(waitingTasksHashTable_tcbAsKey,(uint32_t)task);
		if(task == NULL){
			break; /*no task was waiting for _reason, this is normal behaviour*/
		}
		if(OS_hashtable_put(activeTasksHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
			task->state &= ~TASK_STATE_WAIT;
		}else{
			printf("\u001b[31m\r\nSCHEDULER: ERROR, unable to add task %p which was previously waiting back to activeTasksHashTable!\r\n",task);
			DEBUG_hashTableState();
			DEBUG_heapState();
			printf("\u001b[0m");
			ASSERT(0);
		}
		
		/*if the OS_hashtable_put(tasksInSchedulerHeapHashTable...) below fails (returns 0) that is expected behaviour. It simply means that a task
		requested wait but that it was never removed from the schedulerHeap because the scheduler did not come accross that node in the heap
		when searching for the next task to select (and therefore did not have a chance to remove it from the heap). */
		if(OS_hashtable_put(tasksInSchedulerHeapHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
			/*tcb was added to "tasksInSchedulerHeapHashTable" meaning that it currently is not in the heap (if it had never been removed from the heap
			the OS_hashtable_put operation would have returned 0 when attempting to add it again), hence add it*/
			if(task->inheritedPriority)
                OS_heap_addNode(schedulerHeap,task,task->inheritedPriority);
            else{
                OS_heap_addNode(schedulerHeap,task,task->priority);
            }
		}
	}
	/*This task has just released a resource. If it is currently running under inherited priority this priority needs
	 * to be updated now to reflect this change.*/
	OS_TCB_t * currentTCB = OS_currentTCB();
	/*remove the released resource from the tasks linked list of acquired mutexes, (if the resource cannot be found
	 * in that list then do nothing, priority inheritance currently only works for mutex)*/
	OS_mutex_t * prevAcquiredMutex = NULL;
	OS_mutex_t * acquiredMutex = currentTCB->acquiredResourcesLinkedList;
	while (acquiredMutex){
			if(acquiredMutex == reason){
					/*found resource, remove it from the list since the task no longer owns it*/
					if(prevAcquiredMutex){
							prevAcquiredMutex->nextAcquiredResource = acquiredMutex->nextAcquiredResource;
					}else{
							currentTCB->acquiredResourcesLinkedList = acquiredMutex->nextAcquiredResource;
					}
					acquiredMutex->nextAcquiredResource = NULL;//reset to avoid infinite loop
					__updatePriorityInheritance(currentTCB);
					break;
			}
			prevAcquiredMutex = acquiredMutex;
			acquiredMutex = acquiredMutex->nextAcquiredResource;
	}
}

static void stochasticScheduler_sleepCallback(OS_TCB_t * const tcb,uint32_t min_sleep_duration){
	/*First check if the sleep time is reasonable*/
	if(min_sleep_duration == 0){
		return;
	}
	if(OS_hashtable_remove(activeTasksHashTable,(uint32_t)tcb)){
		/*set task state so that the scheduler can identify tasks that requested sleep and check if this
		condition still applies*/
		tcb->state |= TASK_STATE_SLEEP;
		tcb->data = OS_elapsedTicks(); /*what time did the task request the sleep ? needed to check if
		the task should wake up.*/
		tcb->data2 = min_sleep_duration;/*used to keep track of the remaining sleep duration*/
		OS_hashtable_put(sleepingTasksHashTable,(uint32_t) tcb,(uint32_t*) tcb,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
		/*task is not removed from the heap, this is done inside the scheduler callback should the scheduler try to run
		a task that is in the sleep state.*/
	}else{
		/*OS_hashtable_remove returned NULL meaning that no element with the given key was present*/
		printf("\u001b[31m\r\nSCHEDULER: ERROR task %p called sleep, but it is not in the activeTasks HashTable!\r\n",tcb);
		DEBUG_hashTableState();
		DEBUG_heapState();
		printf("\u001b[0m");
		ASSERT(0);
	}
	return;
}

/*changes the tasks priority depending on the priority of other tasks that are waiting on resources acquired by this
 * task. The task will use the highest priority (lowest value, minHeap) as its own priority as long as it holds the
 * resource*/
static void __updatePriorityInheritance(OS_TCB_t * task){
    OS_mutex_t * acquiredMutex = task->acquiredResourcesLinkedList;
    uint32_t highestPriority = task->priority;
    /*loop through all mutexes owned by the given task and determine the highest priority that the task should iinherit*/
    while (acquiredMutex){
        uint32_t instanceCounter = 0;
        OS_TCB_t * taskWaitingOnMutex = (OS_TCB_t *)OS_hashtable_getNthValueAtKey(waitingTasksHashTable_reasonAsKey,(uint32_t)acquiredMutex,instanceCounter);
        /*loop through all tasks waiting on this mutex and determine what the highest priority of the waiting tasks is*/
        while(waitingTasksHashTable_reasonAsKey->validValueFlag){
            //REMEMBER, minheap so larger value means less priority
            uint32_t waitingTaskMaxPriority = (taskWaitingOnMutex->inheritedPriority)? taskWaitingOnMutex->inheritedPriority:taskWaitingOnMutex->priority;
            if(waitingTaskMaxPriority < highestPriority){
                highestPriority = waitingTaskMaxPriority;
            }
            instanceCounter++;
            taskWaitingOnMutex = (OS_TCB_t *)OS_hashtable_getNthValueAtKey(waitingTasksHashTable_reasonAsKey,(uint32_t)acquiredMutex,instanceCounter);
        }
        acquiredMutex = acquiredMutex->nextAcquiredResource;
    }
    /*having determined the priority to inherit set it and move the task to the correct index in the heap (if it is part
     * of the scheduler heap at this point in time)*/
		uint32_t isActiveAndInHeap = OS_hashtable_get(tasksInSchedulerHeapHashTable,(uint32_t)task) && tasksInSchedulerHeapHashTable->validValueFlag;
    if(highestPriority < task->priority ){
        task->prevInheritedPriority = task->inheritedPriority;
        task->inheritedPriority = highestPriority;
    }else{
				if(task->inheritedPriority && isActiveAndInHeap){//if true it means that the task was running under inherited priority previously
					/*remove and re-add task to reset priority*/
					uint32_t taskHeapIndex = OS_heap_indexOfContent(schedulerHeap,(uint32_t)task);
					void * removedTask;
					OS_heap_removeNodeAt(schedulerHeap,taskHeapIndex,&removedTask);
					if(removedTask != task){
            printf("SCHEDULER: ERROR during priority inheritance reset, taskHeapIndex is incorrect!");
            ASSERT(0);
					}
					OS_heap_addNode(schedulerHeap,task,task->priority);
				}
        task->inheritedPriority = 0;//nothing inherited
    }
		/*remove and re-add the task to the heap with the inherited priority*/
    if(isActiveAndInHeap && task->inheritedPriority && task->prevInheritedPriority != task->inheritedPriority){
        uint32_t taskHeapIndex = OS_heap_indexOfContent(schedulerHeap,(uint32_t)task);
        void * removedTask;
				OS_heap_removeNodeAt(schedulerHeap,taskHeapIndex,&removedTask);
        if(removedTask != task){
            printf("SCHEDULER: ERROR during priority inheritance, taskHeapIndex is incorrect!");
            ASSERT(0);
        }
        OS_heap_addNode(schedulerHeap,task,task->inheritedPriority);
    }
}

//=============================================================================
// Priority inheritance related
//=============================================================================

static void resourceAcquired_callback( OS_mutex_t * _acquiredMutex){
    OS_TCB_t * currentTcb = OS_currentTCB();
    /*Note: for now priority inheritance only works for mutex but can be extended in the future to work for
     * semaphores too. */
    if(_acquiredMutex->counter != 0 || currentTcb == NULL){
        /*This resource already seems to have been acquired before by this task and therefore should
         * not be added again to the acquired resources linked list*/
        return;
    }
    /*add the resource to the linked list of acquired resources for later use during priority inheritance*/
	if(_acquiredMutex == currentTcb->acquiredResourcesLinkedList){
		return;
	}
    _acquiredMutex->nextAcquiredResource = NULL;
    OS_mutex_t * tmpMutex = currentTcb->acquiredResourcesLinkedList;
    currentTcb->acquiredResourcesLinkedList = _acquiredMutex;
    _acquiredMutex->nextAcquiredResource = tmpMutex;
}

//=============================================================================
// Internal utility functions
//=============================================================================
/* Used for picking which task to give cpu time. returns 3 possible values:
0: pick the task at the current index in heap (40% chance)
1: go to left child of current node in the heap (30% chance)
2: go to right child of current node (30% chance)

Biased towards picking the current node to favour tasks with higher priority. consider the following
example illustrating the problem that arises without this bias

																					3
																				/   \
																			5       7
																		 / \     /
																		13 21   11

->Node with priority 3 has a 1/3 chance of being picked because it has two valid children.
->Node with priority 7 has a 1/3*2/3 chance of being picked because it has only one valid child, if
	the scheduler tries to select its right hand child it will go back to the parent node and select
	it instead. its left hand child has a probability of 1/3*1/3, so higher priority results in better
	chances of being picked.
->This breaks down with the Node of priority 5. it has a probability of being picked of 1/3*1/3 because
	it has 2 valid children and these children have no children of their own, so their chances are also 1/3*1/3.
	This means that even though the parent has a higher priority it has the same chances of being picked as its
	children. Hence the need to bias.
*/
static int __getRandForTaskChoice(void){
	uint32_t value = rand() % 10;
	if(value <= 3){
		return 0;//choose parent
	}else if(value <= 6){
		return 1;//choose left child
	}else{
		return 2;//choose right child
	}
}

/* Checks if a given task is currently waiting. If task is waiting it is removed from the 
heap used by the scheduler (heap property restored in the process).

If the task is sleeping it must be added to sleepingTasksHashTable prior to this func
being called or else the pointer to its tcb will be lost.

returns: 1 if the task at_index was waiting and has been removed, 0 otherwise.
*/
static void * removedWaitingNode;
static uint32_t __removeIfWaiting(uint32_t _index){
	OS_TCB_t * task = schedulerHeap->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(task->state & TASK_STATE_WAIT){
		OS_heap_removeNodeAt(schedulerHeap,_index,&removedWaitingNode);
		OS_hashtable_remove(tasksInSchedulerHeapHashTable,(uint32_t)task);
		return 1;
	}else{
		return 0;
	}
}

/*
Checks if a task is still running, if remove it from all scheduler hashtables and heaps that keep track of tasks

returns: 1 if the task at_index exited and has therefore been removed, 0 otherwise.
*/
static void * removedNode;
static uint32_t __removeIfExit(uint32_t _index){
	OS_TCB_t * task = schedulerHeap->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(task->state & (TASK_STATE_EXIT) && task->state & (TASK_STATE_SLEEP | TASK_STATE_WAIT)){
		/*cannot remove task since if SLEEP or WAITING states are set it might be in a hashtables other than tasksInSchedulerHeapHashTable
		and activeTasksHashTable. A waiting task should never be able to terminate before it has been woken (same goes for waiting)
		so something must be really wrong.*/
		printf("\u001b[31m\r\nSCHEDULER: ERROR task %p has state TASK_STATE_EXIT set whilst being asleep or waiting!\r\n",task);
		DEBUG_hashTableState();
		DEBUG_heapState();
		printf("\u001b[0m");
		ASSERT(0);
		return 0;
	}else if(task->state & (TASK_STATE_EXIT)){
		/*task tcb pointer will only be present in schedulerHeap, tasksInSchedulerHeapHashTable, activeTasksHashTable. Removing
		it there will cause the task to vanish*/
		OS_heap_removeNodeAt(schedulerHeap,_index,&removedNode);
		OS_hashtable_remove(tasksInSchedulerHeapHashTable,(uint32_t)task);
		OS_hashtable_remove(activeTasksHashTable,(uint32_t)task);
		/*free resources associated with task if nothing else is currently using the memcluster. If memcluster is in use
		then free resources at a later point in time. Cannot go ahead unless no other task was using the memcluster when the scheduler
		interrupt was called, since this operation could interfere severly.*/
		__disable_irq();
		if(!OS_isMemclusterInUse() && 0){
			/* CRITICAL SECTION START
			-> preventing internal mutexes of the memory cluster using svc callbacks
			-> disable interrupts whilst svc callbacks of locks are disabled*/
			memory_cluster_setInternalLockState(0);
			OS_free((uint32_t*)task->originalSpMemoryPointer);
			OS_free((uint32_t*)task);
			memory_cluster_setInternalLockState(1);
			
			/* CRITICAL SECTION END*/
		}else if(comletedTasksLinkedList){
			OS_TCB_t * tmpTask = comletedTasksLinkedList;
			comletedTasksLinkedList = task;
			task->data = (uint32_t)tmpTask;
		}else{
			comletedTasksLinkedList = task;
		}
		__enable_irq();
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
		/*systick timer has rolled over, need to set flag to force scheduler to iterate through
		all elements in the sleepHeap and adjust the remainingTime and lastSleepStateUpdate timestamps to avoid
		those tasks from sleeping much MUCH (like up to 2^32 systicks) longer than desired.*/
		systick_rollover_detected_FLAG = 1;
		uint32_t delta1 = (UINT32_MAX - lastSleepStateUpdate); /*time between last sleep update to task state and
		the moment the systick timer rolled over*/
		deltaTime = currentTime + delta1; // total elapsed time
	}else{
		deltaTime = currentTime - lastSleepStateUpdate;
	}
	task->data = currentTime;//updating update timestamp
	/*now determine if the task should wake up*/
	if(remainingTime <= deltaTime){
		/*yes update hash tables to reflect its new state*/
		OS_hashtable_remove(sleepingTasksHashTable,(uint32_t)task);
		OS_hashtable_put(activeTasksHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
		/*add the task back to the schedulerHeap should it not already be in there*/
		if(OS_hashtable_put(tasksInSchedulerHeapHashTable,(uint32_t)task,(uint32_t*)task,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY)){
            if(task->inheritedPriority)
                OS_heap_addNode(schedulerHeap,task,task->inheritedPriority);
            else{
                OS_heap_addNode(schedulerHeap,task,task->priority);
            }
		}
		/*finally update the TCB*/
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
1 = task was sleeping and has been removed from heap and tasksInSchedulerHeapHashTable
0 = task is awake, no change made
*/
static void * removedSleepingNode;
static uint32_t __removeIfSleeping(uint32_t _index){
	OS_TCB_t * task = schedulerHeap->ptrToUnderlyingArray[_index].ptrToNodeContent;
	if(__updateSleepState(task)){
		OS_heap_removeNodeAt(schedulerHeap,_index,&removedSleepingNode);
		OS_hashtable_remove(tasksInSchedulerHeapHashTable,(uint32_t)task);
		OS_heap_addNode(sleepHeap,task,task->data2);/*data2 keeps track of remaining sleep duration which
			the sleepHeap uses to order its elements. This means that tasks that are about to wake up are
			right at the top of the heap.*/
		return 1;
	}else{
		return 0;
	}
}

//=============================================================================
// Externally accessible utility functions
//=============================================================================
/* Following functions can be used to check various states of a specific task by checking the internal hashtables of the scheduler.
 *
 * RETURNS: 1 if true, 0 otherwise*/

uint32_t OS_scheduler_isTaskActive(OS_TCB_t * _task){
	if(OS_hashtable_get(activeTasksHashTable,(uint32_t)_task)){
		return 1;
	}else{
		return 0;
	}
}

uint32_t OS_scheduler_isTaskSleeping(OS_TCB_t * _task){
	if(OS_hashtable_get(sleepingTasksHashTable,(uint32_t)_task)){
		return 1;
	}else{
		return 0;
	}
}

uint32_t OS_scheduler_isTaskWaiting(OS_TCB_t * _task){
	if(OS_hashtable_get(waitingTasksHashTable_tcbAsKey,(uint32_t)_task)){
		return 1;
	}else{
		return 0;
	}
}

uint32_t OS_scheduler_doesTaskExist(OS_TCB_t * _task){
	if(OS_scheduler_isTaskActive(_task) || OS_scheduler_isTaskSleeping(_task) || OS_scheduler_isTaskWaiting(_task)){
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
	printf("DEBUG: DUMPING CONTENT OF SCHEDULER HASH TABLES!\r\n");
	printf("\r\nACTIVE TASK HASH TABLE:");
	DEBUG_printHashtable(activeTasksHashTable);
	printf("\r\nTASKS IN SCHEDULER HEAP HASH TABLE:");
	DEBUG_printHashtable(tasksInSchedulerHeapHashTable);
	printf("\r\nWAITING TASK HASH TABLE:\r\nNOTE: the key is the reason the task is waiting, multiple entries with same key are allowed provided their 'data' fields differ.\r\n");
	DEBUG_printHashtable(waitingTasksHashTable_reasonAsKey);
	printf("\r\nSLEEPING TASK HASH TABLE:");
	DEBUG_printHashtable(sleepingTasksHashTable);
    printf("\r\nNODE CONTENT INDEX:");
    DEBUG_printHashtable(schedulerHeap->nodeContentIndexHashTable);
}

static void DEBUG_heapState(){
	printf("\r\n\r\n######################################################################\r\n");
	printf("DEBUG: DUMPING CONTENT OF HEAPS!\r\n");
	printf("\r\nTASK HEAP:\r\n");
	printHeap(schedulerHeap);
	printf("\r\nSLEEP HEAP:\r\n");
	printHeap(sleepHeap);
}
