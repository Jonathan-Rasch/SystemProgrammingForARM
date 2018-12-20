#include "patientPreemptivePriorityScheduler.h"

/*
 * The scheduler has the following features:
 * -> Task priority:
 *      ->  tasks that are not currently running and are NOT WAITING are placed in a max heap.
 *          when the current task (TaskA) yields, exits or the scheduler forces a context switch the task
 *          with the next highest priority (TaskB) becomes the current task. the previously running task is
 *          placed back into the max heap
 *
 */

//static functions, only visible to same translation unit
static void initialize_scheduler(void);
static OS_TCB_t const * patientPreemptivePriority_scheduler(void);
static void patientPreemptivePriority_addTask(OS_TCB_t * const tcb,uint32_t task_priority);
static void patientPreemptivePriority_taskExit(OS_TCB_t * const tcb);


//things needed for creating the heap used in the scheduler
static minHeapNode * nodeArray[MAX_HEAP_SIZE];
static minHeap heapStruct;
volatile uint32_t task_has_exited_flag = 0;

/*scheduler struct definition*/
OS_Scheduler_t const patientPreemptiveScheduler = {
        .preemptive = 1,
        .initialize = initialize_scheduler,
        .scheduler_callback = patientPreemptivePriority_scheduler,
        .addtask_callback = patientPreemptivePriority_addTask,
        .taskexit_callback = patientPreemptivePriority_taskExit
};

static void initialize_scheduler(void){
    //initHeap(nodeArray,&heapStruct, sizeof(nodeArray)); //TODO check if correct size is passed down here
}

static OS_TCB_t const * patientPreemptivePriority_scheduler(void){
    static int ticksSinceLastTaskSwitch = 0;// used to force task to yield after MAX_TASK_TIME_IN_SYSTICKS
    /*check if any tasks have exited and remove them from the heap*/

    /*check if task has yielded or force it to yield if it has exceeded max allowed task time*/
    OS_TCB_t * currentTaskTCB = OS_currentTCB();
    uint32_t hasCurrentTaskYielded = currentTaskTCB->state & TASK_STATE_YIELD;
    ticksSinceLastTaskSwitch += 1;
    if(!hasCurrentTaskYielded){
        // now check if task has run for more than max allowed time
        if(ticksSinceLastTaskSwitch <= MAX_TASK_TIME_IN_SYSTICKS){
            //task is allowed to continue running
            return currentTaskTCB;
        }
    }
    /*Task has either yielded or it exceeded its maximum allowed time, switch task*/
    currentTaskTCB->state &= ~TASK_STATE_YIELD;// reset so task has chance of running after next task switch
    ticksSinceLastTaskSwitch = 0; //set to 0 so that next task can run for MAX_TASK_TIME_IN_SYSTICKS
    /*TODO determine probabilities for choosing each layer in the heap
     * (based on number of nodes in heap). Then get RANDOM NUMBER to determine what layer, and what node
     * from that layer should be choosen. This gives high priority tasks a bigger likelyhood of being
     * selected but at the same time avoids starvation of lower priority tasks*/
}

static void patientPreemptivePriority_addTask(OS_TCB_t * const tcb,uint32_t task_priority){
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

    task_has_exited_flag = 1; //solution for now
    
}

