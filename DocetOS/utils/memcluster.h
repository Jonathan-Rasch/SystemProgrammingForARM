#ifndef MEMCLUSTER_H
#define MEMCLUSTER_H

#include <stddef.h>
#include <stdint.h>
#include "debug.h" 
#include "mutex.h"

#define MEMPOOL_HASH_TABLE_BUCKET_NUM 3
#define SMALLEST_BLOCK_SIZE 4 //in ((2^N)*4)/1024 kb
#define LARGEST_BLOCK_SIZE 8 //in ((2^N)*4)/1024 kb
#define LARGER_BLOCK_ALLOCATION_STEP_LIMIT 0 //limits how many steps up (in terms of block size) the task should check for free blocks if no blocks of requested size are free. 0 for disabled
#define NUMBER_OF_POOLS ((LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1)

typedef struct{
	uint32_t volatile * nextMemblock; //pointer to next block, either in pool or in "allocated block linked list" of hashtable
	uint32_t 						blockSize; // memcluster needs to know to put block back into corresponding pool
	uint32_t * 					headPtr; // points to first address of memory that the requester can use.
} memBlock;

typedef struct{
		uint32_t 		volatile  freeBlocks;
		uint32_t 							blockSize; // does not include ID field and blockSize field, so actual size is blockSize+2 !
		OS_mutex_t 	* 				memory_pool_lock;
		memBlock 		* 				firstMemoryBlock;
} memory_pool;

typedef struct {
	uint32_t * (* allocate)	(uint32_t required_size_in_4byte_words);// guarantees returned memory is at least required_size_in_4byte_words large
	void 						(* deallocate)(void * memblock_head_ptr);
} OS_memcluster;

void memory_cluster_init(OS_memcluster * memory_cluster, uint32_t * memoryArray, uint32_t memory_Size);
uint32_t djb2_hash(uint32_t thing_to_hash);// hash algo by D. J. Bernstein (https://cr.yp.to/djb.html)

#endif /* MEMCLUSTER_H */

