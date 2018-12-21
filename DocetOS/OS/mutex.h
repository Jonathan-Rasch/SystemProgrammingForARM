#ifndef MUTEX_H
#define MUTEX_H

#include "os.h"
#include "stm32f4xx.h"

typedef struct{
	uint32_t counter;
	OS_TCB_t * tcbPointer;//pointer to a pointer to a tcb
}OS_mutex_t;


//================================================================================
// Exported Functions
//================================================================================

void OS_mutex_acquire(OS_mutex_t * mutex);
void OS_mutex_release(OS_mutex_t * mutex);
void OS_init_mutex(OS_mutex_t * mutex);

#endif /*MUTEX_H*/