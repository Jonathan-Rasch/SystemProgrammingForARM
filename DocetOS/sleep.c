#include "sleep.h"
#include "os_internal.h"

/* OS_sleep
Function sets state for the task so that the scheduler will ignore it for _time ticks when choosing a next task
*/
void OS_sleep(uint32_t _time){
	OS_TCB_t * currentTask = OS_currentTCB();
	currentTask->data = _time; // store how long the task should sleep for
	currentTask->data2 = OS_elapsedTicks(); // store when it started sleeping
	currentTask->state |= TASK_STATE_SLEEP;
	OS_yield();
}