#ifndef DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H
#define DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H

#include "os.h"
#include "stm32f4xx.h"
#include "utils/heap.h"

/*  The maximum number of tasks the scheduler can deal with*/
#define MAX_TASKS MAX_HEAP_SIZE

/*measured in SysTicks. If any given task has no*/
#define MAX_TASK_TIME_IN_SYSTICKS 10

extern OS_Scheduler_t const patientPreemptivePriorityScheduler;

#endif //DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H
