#include "memcluster.h"
#include "math.h"
#include <stdio.h>
#include "stm32f4xx.h"

//================================================================================
// Vars, Definitions, Prototypes And Init Function
//================================================================================

/*VARIABLES*/
static memory_pool pools[5];//array holding memory pools
static OS_mutex_t pool_locks[5]; //pools get locked when in use
static OS_mutex_t hashtable_lock;
static OS_mutex_t DEBUG_printHashtableMutex;
static memBlock * allocatedBlocksBuckets[MEMPOOL_HASH_TABLE_BUCKET_NUM];

/*PROTOTYPES*/
//for memory pools
static void __addBlockToPool(memory_pool *,memBlock *);
static memBlock * __removeBlockFromPool(memory_pool *);
//for hashtable
static void __placeBlockIntoBucket(memBlock *);
static memBlock * __recoverBlockFromBucket(uint32_t * memory_pointer);
static void __printHashtable(void);
//for memcluster struct
static uint32_t * 	allocate		(uint32_t required_size_in_4byte_words);
static void 				deallocate	(void * memblock_head_ptr);

/* Cluster Init Function
*/
void memory_cluster_init(OS_memcluster * memory_cluster, uint32_t * memoryArray, uint32_t memory_Size_in_4byte_words){
	
	for(int i=0;i<memory_Size_in_4byte_words;i++){
		memoryArray[i] = NULL;
	}
	
	/*Initializing pools*/
	for(int i = SMALLEST_BLOCK_SIZE; i <= LARGEST_BLOCK_SIZE; i++){
		int array_idx = i-SMALLEST_BLOCK_SIZE;
		memory_pool * pool = &pools[array_idx];
		OS_mutex_t * pool_lock = &pool_locks[array_idx];
		OS_init_mutex(pool_lock);
		OS_init_mutex(&DEBUG_printHashtableMutex);
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
		//place in pool
		__addBlockToPool(pool,blockPtr);
		//updating counter so that on next itteration the pool index changes
		counter++;
	}
	/*SETUP FOR HASHTABLE*/
	for(int i=0;i<MEMPOOL_HASH_TABLE_BUCKET_NUM;i++){
		allocatedBlocksBuckets[i] = NULL;//not guaranteed that memory is initialized to 0x00, hence enforce.
	}
	
	/*adding function pointers to memcluster struct*/
	//TODO
	memory_cluster->allocate = allocate;
	memory_cluster->deallocate = deallocate;
}
//================================================================================
// MemCluster struct functions
//================================================================================

/* "allocate" retrieves a memory region that the task is free to write to.
-> it guarantees that the returned memory region has a size of AT LEAST required_size_in_4byte_words (BUT COULD BE LARGER!)
-> call will block if no memory blocks of specified size (or larger) are available, and will resume as soon as this changes
-> the user is responsible for not writing more than the size they requested (even though the block returned COULD be larger).
-> user is responsible for passing the SAME pointer (not a pointer somewhere in the given memory) back to the deallocate function when no longer needed.*/
static uint32_t * allocate(uint32_t required_size_in_4byte_words){
	//input checking
	if(required_size_in_4byte_words == 0){
		printf("ERROR: cannot allocate memory of size 0 words\r\n");
		return NULL;
	}
	//determine the minimum size block required
	memory_pool * selectedPool = NULL;
	int selectedPoolIdx;
	int numberOfPools = (LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1;
	for(int i=0;i<numberOfPools;i++){
		if(required_size_in_4byte_words > pools[i].blockSize){
			continue;
		}
		selectedPoolIdx = i;
		selectedPool = &pools[i];
		break;
	}
	if(selectedPool == NULL){
		printf("ERROR: cannot allocate memory of size %d 4byte words, no block large enough.\r\n",required_size_in_4byte_words);
	}
	// pool of correct size found, no try to grab a lock on a pool with free blocks
	uint32_t initialSelectedIdx = selectedPoolIdx;
	OS_mutex_t * poolLock;
	uint32_t freeBlocks = 0;
	for(int i=initialSelectedIdx;i<=numberOfPools;i++){
		selectedPoolIdx = i;
		selectedPool = &pools[i];
		poolLock = selectedPool->memory_pool_lock;
		if(i < (initialSelectedIdx+LARGER_BLOCK_ALLOCATION_STEP_LIMIT) && i < numberOfPools){
				/*try and get a hold of the lock of a pool of adequate size, then check if it has free blocks*/
				if(OS_mutex_acquire_non_blocking(poolLock)){
					freeBlocks = selectedPool->freeBlocks;
					if(freeBlocks){
						break;
					}else{
						/*no free blocks, try next pool*/
						continue;
					}
				}else{
					/*could not get lock, try next pool*/
					continue;
				}
		}else{
				/*trying to aquire lock of largest allowed pool, if that fails then wait for the original selected pool to get released*/
				if(!OS_mutex_acquire_non_blocking(poolLock)){
					/*could not aquire lock of largest allowed pool, now go back to the pool of the smallest size that fits the requested
					memory and wait for its lock to be released*/
					OS_mutex_acquire(&pool_locks[initialSelectedIdx]);
				}
				/*got the lock, but there is no guarantee that there are free blocks, hence check. If still
				no blocks are free restart the process of trying to aquire a larger block*/
				freeBlocks = selectedPool->freeBlocks;
				if(!freeBlocks){//still nothing free
					i = initialSelectedIdx; //reset counter, restart process
					continue;
				}else{
					break;
				}
		}
	}
	/*got pool lock on pool with free block(s). store block in hashmap and return pointer to usable memory*/
	memBlock * block = __removeBlockFromPool(selectedPool);
	OS_mutex_release(poolLock);
	return block->headPtr;
}

/* "deallocate" checks if the given pointer belongs to an allocated memory block and returns it to its corresponding memory pool should 
this be the case. Nothing happens (except error message) when a user tries to return a block to the cluster that does not belong to the cluster*/
static void deallocate(void * memblock_head_ptr){
	memBlock * block = __recoverBlockFromBucket(memblock_head_ptr);
	if(block == NULL){ // NULL pointer returned, provided memory ptr is not valid
		return;
	}
	/*determine in what pool the block belongs:
	2^(SMALLEST_BLOCK_SIZE + X ) = B
		B: block_size
		X: number such that (X + SMALLEST_BLOCK_SIZE <= LARGEST_BLOCK_SIZE)
	hence:
	X = (log(B)/log(2)) - SMALLEST_BLOCK_SIZE
	
	TODO: implement when FPU is enabled, for now just cycle through pools*/
	int numberOfPools = (LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1;
	memory_pool * pool = NULL;
	int poolIdx = 0;
	for(int i = 0;i<numberOfPools;i++){
		if(pools[i].blockSize != block->blockSize){
			continue;
		}
		pool = &pools[i];
		poolIdx = i;
		break;
	}
	OS_mutex_acquire(&pool_locks[poolIdx]);
	__addBlockToPool(pool,block);
	OS_mutex_release(&pool_locks[poolIdx]);
}

//================================================================================
// Internal Functions
//================================================================================

/*MEMORY POOL RELATED
these functions assume that a lock for _pool has been obtained PRIOR to them being called!*/

static void __addBlockToPool(memory_pool * _pool,memBlock * _block){
	_block->nextMemblock = (uint32_t *)_pool->firstMemoryBlock;
	_pool->firstMemoryBlock = _block;
	_pool->freeBlocks += 1; 
}


static memBlock * __removeBlockFromPool(memory_pool * _pool){
	//retrieve block from pool
	memBlock * block = _pool->firstMemoryBlock;
	_pool->firstMemoryBlock = (memBlock *) block->nextMemblock;
	block->nextMemblock = NULL;
	_pool->freeBlocks -= 1;
	//place into hashtable that keeps track of allocated blocks
	__placeBlockIntoBucket(block);
	return block;
}

/* HASH TABLE RELATED
lock for the hashtable is aquired when function is called (blocking)*/

static void __placeBlockIntoBucket(memBlock * _block){
	OS_mutex_acquire(&hashtable_lock);
	//determine into what bucket to place the block
	uint32_t hash = djb2_hash((uint32_t)_block->headPtr); // hashing the pointer to memory NOT THE BLOCK PTR! (since that is what task passes to deallocate)
	uint32_t bucketNumber = hash % MEMPOOL_HASH_TABLE_BUCKET_NUM;
	//now place into linked list in bucket bucketNumber
	memBlock * tmp_block = allocatedBlocksBuckets[bucketNumber];
	allocatedBlocksBuckets[bucketNumber] = _block;
	_block->nextMemblock = (uint32_t *)tmp_block;
	__printHashtable();
	OS_mutex_release(&hashtable_lock);
}

static memBlock * __recoverBlockFromBucket(uint32_t * memory_pointer){
	OS_mutex_acquire(&hashtable_lock);
	//determine what bucket the block corresponding to the memory pointer should be in
	uint32_t hash = djb2_hash((uint32_t)memory_pointer);
	uint32_t bucketNumber = hash % MEMPOOL_HASH_TABLE_BUCKET_NUM;
	//now search for a memblock with the given pointer inside that bucket
	memBlock * blockPtr = allocatedBlocksBuckets[bucketNumber]; // get ptr to first block
	while(blockPtr){//stop when NULL ptr (means no more items in linked list)
		if(blockPtr->headPtr != memory_pointer){
			//nope, not this block, try next
			blockPtr = (memBlock *)blockPtr->nextMemblock;
			continue;
		}
		// match ! now remove the block from the linked list and return it so that it can be readded to one of the pools
		allocatedBlocksBuckets[bucketNumber] = (memBlock *)blockPtr->nextMemblock;
		OS_mutex_release(&hashtable_lock);
		__printHashtable();
		return blockPtr;
	}
	// if it gets here then then no block matched (user probably messed up and tried to dealloc a memblock not belonging to the cluster)
	printf("ERROR: attempt to deallocate memory block that is not part of this memory cluster !\n\r");
	//ASSERT(0);
	OS_mutex_release(&hashtable_lock);
	//__printHashtable();
	return NULL;
}

static OS_mutex_t DEBUG_printHashtableMutex;
static void __printHashtable(void){
	OS_mutex_acquire(&DEBUG_printHashtableMutex);
	printf("\r\n");
	for(int i =0; i<MEMPOOL_HASH_TABLE_BUCKET_NUM;i++){
		memBlock * block = allocatedBlocksBuckets[i];
		printf("BUCKET %d: ",i);
		while(block){
			printf("%p[%d] -> ",block,block->blockSize);
			block = (memBlock *)block->nextMemblock;
		}
		printf("NULL\r\n");
	}
	OS_mutex_release(&DEBUG_printHashtableMutex);
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
