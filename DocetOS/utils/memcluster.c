#include "memcluster.h"
#include "math.h"
#include <stdio.h>

//================================================================================
// Vars, Definitions, Prototypes And Init Function
//================================================================================

/*VARIABLES*/
static memory_pool pools[5];//array holding memory pools
static OS_mutex_t pool_locks[5]; //pools get locked when in use

/*PROTOTYPES*/
static void __addBlockToPool(memory_pool *,memBlock *);
static memBlock * __removeBlockFromPool(memory_pool *);

/* Cluster Init Function
*/
void memory_cluster_init(OS_memcluster * memory_cluster, uint32_t * memoryArray, uint32_t memory_Size_in_4byte_words){
	
	/*Initializing pools*/
	for(int i = SMALLEST_BLOCK_SIZE; i <= LARGEST_BLOCK_SIZE; i++){
		int array_idx = i-SMALLEST_BLOCK_SIZE;
		memory_pool * pool = &pools[array_idx];
		OS_mutex_t * pool_lock = &pool_locks[array_idx];
		OS_init_mutex(pool_lock);
		// pool setup
		pool->freeBlocks = 0;
		pool->blockSize = pow(2,i);
		pool->memory_pool_lock = pool_lock;
		pool->firstMemoryBlock = NULL;
	}

	/*allocating blocks to pools*/
	uint32_t counter = 0;
	int numberOfPools = (LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1;
	int numSkips = 0;// keeps track of largest sequence of block allocations taht where skipped due to insuficient remaining memory
	while(memory_Size_in_4byte_words){//exits if all memory allocated, or no further allocation possible.
		int pool_idx = counter % numberOfPools;
		memory_pool * pool = &pools[pool_idx];
		//check if block fits into remaining memory
		uint32_t requiredMemoryForBlock = (sizeof(memBlock)/4) + pool->blockSize;// in 4byte words
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
		blockPtr->blockSize = pool->blockSize;
		blockPtr->nextMemblock = NULL;
		uint32_t tmp_sizeOfMemblockStruct = (sizeof(memBlock)/4);//TODO: check that i got the size right here, mem should be addressed in 32bit words
		blockPtr->headPtr = memoryArray + tmp_sizeOfMemblockStruct; // 4bytes for memBlock struct fields
		//updating vars keeping track of remaining memory
		memoryArray = memoryArray+requiredMemoryForBlock;
		memory_Size_in_4byte_words -= requiredMemoryForBlock;
		/*inserting block into pool
		NOTE: the reason why I dont use the __addBlockToPool methode here is twofold:
		1) 	it uses mutex to lock pool, but during init there should be no tasks fighting for access, so that is just wasting cycles.
		2) 	the method uses "notify()" func, if I would use __addpool this would require the OS_init to be run before memorycluster init (bus fault otherwise since
				_scheduler will be a pointer to NULL), but scheduler needs to use memory cluster in its init.*/
		blockPtr->nextMemblock = (uint32_t *)pool->firstMemoryBlock;
		pool->firstMemoryBlock = blockPtr;
		pool->freeBlocks += 1;
		//updating counter so that on next itteration the pool index changes
		counter++;
	}
	
	
}

//================================================================================
// Internal Functions
//================================================================================

static void __addBlockToPool(memory_pool * _pool,memBlock * _block){
	OS_mutex_acquire(_pool->memory_pool_lock);
	_block->nextMemblock = (uint32_t *)_pool->firstMemoryBlock;
	_pool->firstMemoryBlock = _block;
	_pool->freeBlocks += 1;
	OS_notify(_pool);//notify any pools waiting on memory pool to have blocks available 
	OS_mutex_release(_pool->memory_pool_lock);
}

static memBlock * __removeBlockFromPool(memory_pool * _pool){
	OS_mutex_acquire(_pool->memory_pool_lock);
	/*TODO needs to be implemented:
	1) get memblock pointer from pool
	2) extract memory head pointer
	3) hash head pointer and use it as key to store memblock pointer in hashtable
	4) give memblock pointer to task*/
	OS_mutex_release(_pool->memory_pool_lock);
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