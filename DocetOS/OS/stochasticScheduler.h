#ifndef DOCETOS_stochasticScheduler_H
#define DOCETOS_stochasticScheduler_H

#include "os.h"
#include "stm32f4xx.h"
#include "structs.h"

/*  The maximum number of tasks the scheduler can deal with*/
#define MAX_TASKS MAX_HEAP_SIZE

/*measured in SysTicks. If any given task has no*/
#define MAX_TASK_TIME_IN_SYSTICKS 100

#define NUM_BUCKETS_FOR_WAIT_HASHTABLE 8
#define WAIT_HASHTABLE_CAPACITY 32

void initialize_scheduler(uint32_t _size_of_heap_node_array);
extern OS_Scheduler_t const stochasticScheduler;

/*externally accessible utility functions. This is useful for checking the state of an arbitrary task*/
uint32_t isTaskActive(OS_TCB_t * _task);
uint32_t isTaskSleeping(OS_TCB_t * _task);
uint32_t isTaskWaiting(OS_TCB_t * _task);
uint32_t doesTaskExist(OS_TCB_t * _task);

#endif //DOCETOS_stochasticScheduler_H
