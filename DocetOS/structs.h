#ifndef STRUCTS_H
#define STRUCTS_H
#include <stdint.h>

//=============================================================================
// structs for queue.c
//=============================================================================

typedef struct{
	volatile uint32_t * writePointer;
	volatile uint32_t * readPointer;
	uint32_t maxCapacity;
	uint32_t * memoryStart;
	uint32_t * memoryEnd;
} OS_queue_t;

//=============================================================================
// structs for hashtable.c
//=============================================================================
typedef struct{
	volatile uint32_t * next_hashtable_value;
	volatile uint32_t * underlying_data;
	volatile uint32_t key;
}OS_hashtable_value_t;

typedef struct{
	uint32_t number_of_buckets;
	uint32_t maximum_capacity;
	volatile uint32_t remaining_capacity;
	volatile uint32_t validValueFlag; // set to 0 on entry to get or remove operation. set to 1 if valid
	volatile uint32_t * free_hashtable_value_struct_linked_list;
}OS_hashtable_t;

//=============================================================================
// structs for heap.c
//=============================================================================
typedef struct __s_node{
    void * volatile ptrToNodeContent; //ME: leave as volatile for now, ptr changes during swap
    /* Node value is used when restoring heap*/
    volatile uint32_t nodeValue;
}OS_minHeapNode_t;

typedef struct __s_heap{
	/* the heap has an underlying array of nodes. each node holds a value that is used during the
	 * restoration of the heap (node->nodeValue). */
	OS_minHeapNode_t *   ptrToUnderlyingArray;
	OS_hashtable_t * nodeContentIndexHashTable; // only created if requested by user, stores node index of a given node content for quick access.
	uint32_t        maxNumberOfNodes;
	volatile uint32_t        currentNumNodes;
	volatile OS_minHeapNode_t *   nextEmptyElement;
	OS_minHeapNode_t *   lastArrayElement;
} minHeap;

//=============================================================================
// structs for os.c
//=============================================================================
typedef struct {
	/* Task stack pointer.  It's important that this is the first entry in the structure,
	   so that a simple double-dereference of a TCB pointer yields a stack pointer. */
	void * volatile sp;
	/* This field is intended to describe the state of the thread - whether it's yielding,
	   runnable, or whatever.  Only one bit of this field is currently defined (see the #define
	   below), so you can use the remaining 31 bits for anything you like. */
	uint32_t volatile state;
	/* The remaining fields are provided for expandability.  None of them have a dedicated
	   purpose, but their names might imply a use.  Feel free to use these fields for anything
	   you like. */
	uint32_t volatile priority;
	uint32_t volatile data;
	uint32_t volatile data2;
	uint32_t volatile svc_return;//data returned from svc placed in here.
	uint32_t volatile inheritedPriority; // 0 if nothing inherited
    uint32_t volatile prevInheritedPriority; //used to check if inherited priority changed.
	void * volatile acquiredResourcesLinkedList; // 0 if nothing inherited
} OS_TCB_t;

//=============================================================================
// structs for mutex.c
//=============================================================================

typedef struct{
	uint32_t counter;
	OS_TCB_t * tcbPointer;//pointer to a pointer to a tcb
	void * nextAcquiredResource; // points to resource of next lower priority acquired by task;
} OS_mutex_t;

//=============================================================================
// structs for semaphore.c
//=============================================================================

typedef struct{
    volatile uint32_t availableTokens;
    uint32_t maxTokens;
} OS_semaphore_t;

//=============================================================================
// structs for memcluster.c
//=============================================================================

typedef struct{
	uint32_t volatile * nextMemblock; //pointer to next block, either in pool or in "allocated block linked list" of hashtable
	uint32_t 			blockSize; // memcluster needs to know to put block back into corresponding pool
	uint32_t * 			headPtr; // points to first address of memory that the requester can use.
} memBlock;

typedef struct{
	volatile      uint32_t 	freeBlocks;
	uint32_t 			    blockSize; // does not include ID field and blockSize field, so actual size is blockSize+2 !
	OS_mutex_t 	* 		    memory_pool_lock;
	memBlock    * volatile 	firstMemoryBlock;
} memory_pool;

typedef struct {
	uint32_t * (* allocate)	    (uint32_t required_size_in_4byte_words);// guarantees returned memory is at least required_size_in_4byte_words large
	void 	   (* deallocate)   (void * memblock_head_ptr);
} OS_memcluster_t;

//=============================================================================
// structs for channel.c
//=============================================================================

typedef struct{
	uint32_t channelID;
	uint32_t capacity;
	OS_semaphore_t * readTokens;
	OS_semaphore_t * writeTokens;
	OS_mutex_t * queueLock;
	OS_queue_t * queue;
} OS_channel_t;

//=============================================================================
// structs for channel.c
//=============================================================================

typedef struct{
	OS_channel_t * (* connect_callback)(uint32_t _channelID,uint32_t _capacity);
	uint32_t (* disconnect_callback)(uint32_t _channelID);
	uint32_t (* isAlive_callback)(uint32_t _channelID);
} OS_channelManager_t;

#endif /*STRUCTS_H*/
