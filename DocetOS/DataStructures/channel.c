#include "channel.h"

//=============================================================================
// init, new and destroy
//=============================================================================

/*channel_init */
void channel_init(OS_channel_t * channel,uint32_t _channelID,uint32_t _capacity){
	channel->capacity = _capacity;
	channel->channelID = _channelID;
	/*(re)setting semaphores*/
	channel->readTokens->availableTokens = 0;
	channel->readTokens->maxTokens = _capacity;
	channel->writeTokens->availableTokens = _capacity;
	channel->writeTokens->maxTokens = _capacity;
	/*(re)setting queue*/
	channel->queue->readPointer = channel->queue->memoryStart;
	channel->queue->writePointer = channel->queue->memoryStart;
	channel->queue->maxCapacity = _capacity;
}

/* new_channel allocates and initialises a channel that can be used for inter-task communication
 * RETURNS: OS_channel_t pointer if successful, returns NULL otherwise.*/
OS_channel_t * new_channel(uint32_t _channelID, uint32_t _capacity){
    OS_channel_t * newChannel = OS_alloc(sizeof(OS_channel_t)/4);
    if(newChannel == NULL){
        printf("\r\nCHANNEL: ERROR, failed to allocate channel memory!\r\n");
        return NULL;
    }
    newChannel->channelID = _channelID;
    newChannel->capacity = _capacity;
    newChannel->readTokens = new_semaphore(0,_capacity);
    newChannel->writeTokens = new_semaphore(_capacity,_capacity);
    if(newChannel->readTokens == NULL || newChannel->writeTokens == NULL){
        printf("\r\nCHANNEL: ERROR, failed to allocate memory for channel semaphores!\r\n");
        return NULL;
    }
    newChannel->queueLock = new_mutex();
    if(newChannel->queueLock == NULL){
        printf("\r\nCHANNEL: ERROR, failed to allocate memory for channel mutex!\r\n");
        return NULL;
    }
    newChannel->queue = new_queue(_capacity);
    if(newChannel->queue == NULL){
        printf("\r\nCHANNEL: ERROR, failed to allocate memory for channel queue!\r\n");
        return NULL;
    }
    return newChannel;
}

/*destroy_channel frees all resources associated with the channel*/
uint32_t destroy_channel(OS_channel_t * _channel){
    destroy_semaphore(_channel->writeTokens);
    destroy_semaphore(_channel->readTokens);
    destroy_mutex(_channel->queueLock);
    destroy_queue(_channel->queue);
    OS_free((uint32_t*)_channel);
    return 1;
}

//=============================================================================
// functions
//=============================================================================

/*channel_write places _word into the channel. If the channel is full or if another task is currently using the
 * channel (read or write) the task will block on this method.*/
void channel_write(OS_channel_t * _channel, uint32_t _word){
    /*first need to get write token. if a token is available it means that there is space in the queue.*/
    OS_semaphore_acquire_token(_channel->writeTokens);
    /*got a token, lock the queue and write data*/
    OS_mutex_acquire(_channel->queueLock);
    OS_queue_write(_channel->queue,_word);
    /*there is new data available in the queue so we need to put a token into THE READ SEMAPHORE*/
    OS_semaphore_release_token(_channel->readTokens);
    /*finally release the lock so the next task can read/write*/
    OS_mutex_release(_channel->queueLock);
}

/*channel_read reads one 32-bit word from the channel. If the channel is empty or currently in use the task will block
 * on this method.*/
uint32_t channel_read(OS_channel_t * _channel){
    /*first need to get read token. if a token is available it means that there is data in the queue.*/
    OS_semaphore_acquire_token(_channel->readTokens);
    /*got a token, lock the queue and read data*/
    OS_mutex_acquire(_channel->queueLock);
    uint32_t readData;
    OS_queue_read(_channel->queue,&readData);
    /*reading from the queue frees up space, hence place a token into THE WRITE SEMAPHORE*/
    OS_semaphore_release_token(_channel->writeTokens);
    /*finally release the lock and return the data*/
    OS_mutex_release(_channel->queueLock);
    return readData;
}
