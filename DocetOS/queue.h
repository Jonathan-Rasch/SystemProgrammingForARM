
#ifndef OS_QUEUE_H
#define OS_QUEUE_H
	#include <stdint.h>
	#include "structs.h"
	
	uint32_t queue_write(OS_queue_t * queue, uint32_t data);
	uint32_t queue_read(OS_queue_t * queue, uint32_t * _return);
	uint32_t queue_isEmpty(OS_queue_t * queue);
	uint32_t queue_isFull(OS_queue_t * queue);
	uint32_t queue_peekAt(uint32_t _n, uint32_t * _return);//return code to indicate if _return is valid, _n relative to readPtr
	
#endif /*OS_QUEUE_H*/
