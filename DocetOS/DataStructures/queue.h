
#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#include <stdint.h>
#include "structs.h"
#include <stdio.h>
#include "../OS/debug.h"
#include "os.h"

//alloc/init
OS_queue_t * new_queue(uint32_t capacity);
uint32_t destroy_queue(OS_queue_t * _queue);
//data access functions
uint32_t OS_queue_write(OS_queue_t * queue, uint32_t data);
uint32_t OS_queue_read(OS_queue_t * queue, uint32_t * _return);
uint32_t OS_queue_peekAt(OS_queue_t * queue, uint32_t _n, uint32_t * _return);//return code to indicate if _return is valid, _n relative to readPtr
//utility functions
uint32_t OS_queue_isEmpty(OS_queue_t * queue);
uint32_t OS_queue_isFull(OS_queue_t * queue);

#endif /*OS_QUEUE_H*/
