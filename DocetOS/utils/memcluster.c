#include "memcluster.h"
#include "math.h"
#include <stdio.h>

//================================================================================
// Vars, Definitions and Init Function
//================================================================================

static memory_pool pools[5];//array holding memory pools
static OS_mutex_t pool_locks[5]; //pools get locked when in use

void memory_cluster_init(OS_memcluster * memory_cluster, uint32_t * memoryArray, uint32_t memory_Size_in_4byte_words){
	
	/*Initializing pools*/
	for(int i = SMALLEST_BLOCK_SIZE; i <= LARGEST_BLOCK_SIZE; i++){
		int array_idx = i-SMALLEST_BLOCK_SIZE;
		memory_pool pool = pools[array_idx];
		OS_mutex_t pool_lock = pool_locks[array_idx];
		OS_init_mutex(&pool_lock);
		// pool setup
		pool.freeBlocks = 0;
		pool.blockSize = pow(2,i);
		pool.memory_pool_lock = &pool_lock;
		pool.firstMemoryBlock = NULL;
	}

	/*allocating blocks to pools*/
	uint32_t counter = 0;
	int numberOfPools = (LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1;
	int numSkips = 0;// keeps track of largest sequence of block allocations taht where skipped due to insuficient remaining memory
	while(memory_Size_in_4byte_words){//exits if all memory allocated, or no further allocation possible.
		int pool_idx = counter % numberOfPools;
		memory_pool pool = pools[pool_idx];
		//check if block fits into remaining memory
		uint32_t requiredMemoryForBlock = (sizeof(memBlock)/4) + pool.blockSize;// in 4byte words
		if(requiredMemoryForBlock > memory_Size_in_4byte_words){
			numSkips++;
			if(numSkips < numberOfPools){
				continue;// try allocating a different size block
			}else{
				break;//no block fits
			}
		}
		//placing memblock struct into memory, struct is 12 bytes
		memBlock * blockPtr = (memBlock *)memoryArray;
		blockPtr->blockSize = pool.blockSize;
		blockPtr->nextMemblock = NULL;
		blockPtr->headPtr = memoryArray + (sizeof(memBlock)/4); // 4bytes for memBlock struct fields
		//updating vars keeping track of remaining memory
		memoryArray = memoryArray+requiredMemoryForBlock;
		memory_Size_in_4byte_words -= requiredMemoryForBlock;
		//inserting block into pool
		
	}
	
	
}




//================================================================================
// Utility Functions
//================================================================================

/* hash algo by D. J. Bernstein (https://cr.yp.to/djb.html), XOR variant.
I modified it slightly so that it operates on a single uint32_t only (i dont need to hash strings).
This hashing algo has a good distribution and colissions are rare, so perfect for my usecase*/
uint32_t djb2_hash(uint32_t thing_to_hash){
	uint32_t hash = 5381;
	uint32_t c;
	for(int i=0;i<4;i++){
		c = (thing_to_hash >> (i*8)) & 0x000000FF; // essentially pretending that uint32_t is a 4 char string
		hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
	}
	return hash;
}