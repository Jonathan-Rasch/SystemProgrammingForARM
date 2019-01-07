#include "../queue.h"
#include "os.h"
#include <stdio.h>
#include "utils/debug.h"

/* Allocates and initializes a queue of size _capacity*/
OS_queue_t * new_queue(uint32_t _capacity){
    if(_capacity == 0){
        printf("\r\nQUEUE: ERROR cannot create a queue with capacity 0\r\n");
        ASSERT(0);
        return NULL;
    }
    uint32_t * memory = OS_alloc((sizeof(OS_queue_t)/4)+(_capacity+1));
    OS_queue_t * queue = (OS_queue_t*)memory;
    memory += sizeof(OS_queue_t)/4;//move memory pointer along to start of memory for queue items
    queue->memoryStart = memory;
    queue->memoryEnd = memory + (_capacity);
    queue->maxCapacity = _capacity;
    queue->readPointer = queue->memoryStart;
    queue->writePointer = queue->memoryStart;
    return queue;
}

//=============================================================================
//  Data access functions
//=============================================================================

/* writes the _data value to the queue and advances the write pointer. Should this
 * operation succeed the function will return 1, returns 0 otherwise
 *
 * NOTE: queue is LIFO*/
uint32_t queue_write(OS_queue_t * queue, uint32_t _data){
    if(queue_isFull(queue) || queue == NULL){
        return 0;
    }
    /*getting here means there is space in the queue */
    *(queue->writePointer) = _data;
    /*moving the write pointer taking into account that it needs to loop back to start if its
     * current location is the end of the allowed memory (ringbuffer).*/
    if(queue->writePointer == queue->memoryEnd){
        /*no risk of accidentally pointing to same location as read pointer since we
         * previously checked that the que was not full.*/
        queue->writePointer = queue->memoryStart;
    }else{
        queue->writePointer++;
    }
    return 1;
}

/* places the value currently pointed to by the queue read pointer into the _return pointer
 * provided by the user, provided that the queue is not empty. Then moves the read pointer
 * to the next location in the ring buffer.
 *
 * NOTE: queue is FIFO
 *
 * return code of function is 1 if this succeeds and the read value has been placed into _return,
 * returns 0 otherwise
 * */
uint32_t queue_read(OS_queue_t * queue, uint32_t * _return){
    if(queue == NULL || queue_isEmpty(queue)){
        return 0;
    }
    /* reading value and placing it into the return value arg*/
    *_return = *(queue->readPointer);
    /*moving the read pointer taking into account that it needs to loop back to start if its
     * current location is the end of the allowed memory (ringbuffer).*/
    if(queue->readPointer == queue->memoryEnd){
        queue->readPointer = queue->memoryStart;
    }else{
        queue->readPointer++;
    }
    return 1;//indicating that value _return points to is the value read from queue
}

/* Obtains the value stored at position _n (relative to the read pointer) stored in
 * the queue without removing the value from the queue (read pointer does not change).
 * _n = 0 returns the value the current read pointer points to.
 *
 * If the given index is valid and the queue is not empty the value will be placed into
 * the _return pointer provided by the user and the return code of the function will be 1. else
 * the return code will be 0 and the *_return value will remain unchanged*/
uint32_t queue_peekAt(OS_queue_t * queue, uint32_t _n, uint32_t * _return){
    if(queue == NULL || queue_isEmpty(queue) || _n >= queue->maxCapacity){
        return 0;
    }
    uint32_t wordsTillArrayEnd = queue->memoryEnd - queue->readPointer;
    if(_n > wordsTillArrayEnd){
        _n = _n - wordsTillArrayEnd; //steps remaining after reaching array end
				uint32_t val = *(queue->memoryStart + (_n - 1));
        *_return = val;
    }else{
				uint32_t val = *(queue->readPointer + _n);
        *_return = val;
    }
    return 1;
}

//=============================================================================
//  Utility Functions
//=============================================================================

/*Returns 1 if the queue is empty, returns 0 otherwise*/
uint32_t queue_isEmpty(OS_queue_t * queue){
    if(queue->readPointer == queue->writePointer){
        return 1;
    }else{
        return 0;
    }
}

/* Returns 1 if the queue is full, returns 0 otherwise*/
uint32_t queue_isFull(OS_queue_t * queue){
    if (queue->readPointer == queue->memoryStart){
        if(queue->writePointer == queue->memoryEnd){
            return 1;
        }else{
            return 0;
        }
    }else{
        if(queue->writePointer == queue->readPointer - 1){
            return 1;
        }else{
            return 0;
        }
    }
}

