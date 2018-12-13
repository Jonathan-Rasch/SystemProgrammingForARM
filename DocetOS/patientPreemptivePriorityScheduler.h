#ifndef DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H
#define DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H

#include "os.h"
#include "heap.h"
/*  The amount of sys_tickts that the scheduler will wait for a task to yield
 *  before forcibly taking the reigns.*/
#define PATIENCE = 10

/*  The maximum number of tasks the scheduler can deal with*/
#define MAX_TASKS = MAX_HEAP_SIZE

extern OS_Scheduler_t const patientPreemptivePriorityScheduler;

#endif //DOCETOS_PATIENTPREEMPTIVEPRIORITYSCHEDULER_H
