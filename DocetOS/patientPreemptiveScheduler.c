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
static OS_TCB_t const * patientPreemptivePriorityScheduler(void);
