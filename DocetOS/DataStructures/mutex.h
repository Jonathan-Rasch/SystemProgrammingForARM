#ifndef MUTEX_H
#define MUTEX_H

#include "os.h"
#include "stm32f4xx.h"
#include "structs.h"
#include <stdio.h>
#include "os_internal.h"

//================================================================================
// Exported Functions
//================================================================================

void OS_mutex_acquire(OS_mutex_t * mutex);
uint32_t OS_mutex_acquire_non_blocking(OS_mutex_t * _mutex);
void OS_mutex_release_noYield(OS_mutex_t * _mutex);
void OS_mutex_release(OS_mutex_t * mutex);
void OS_init_mutex(OS_mutex_t * mutex);
OS_mutex_t * new_mutex(void);
uint32_t destroy_mutex(OS_mutex_t * _mutex);
//uint32_t OS_mutex_acquire_non_blocking(OS_mutex_t * _mutex);

#endif /*MUTEX_H*/
