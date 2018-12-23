#ifndef MEMCLUSTER_H
#define MEMCLUSTER_H

#include <stddef.h>
#include <stdint.h>
#include "debug.h"

//values must add to 100 !
typedef struct{
	uint32_t Percentage16byteBlocks;
	uint32_t Percentage32byteBlocks;
	uint32_t Percentage64byteBlocks;
	uint32_t Percentage128byteBlocks;
	uint32_t Percentage256byteBlocks;
} memClusterInitStruct;

typedef struct{
	uint32_t const memblockID; //used to prevent freeing of non memblock memory regions by checking if ID is valid. ID = djb2_hash(blockSize ^ headPtr)
	uint32_t const blockSize; // memcluster needs to know to put block back into corresponding pool
	uint32_t * const headPtr; //size of returned block can be determined by doing ptr--
} memBlock;

typedef struct {
	void * const 	(* allocate)	(uint32_t required_size_in_4byte_words);// guarantees returned memory is at least required_size_in_4byte_words large
	void 					(* deallocate)(void * memblock_head_ptr);
} OS_memcluster;

void memory_cluster_init(memClusterInitStruct initStruct, void * memoryArray);
uint32_t djb2_hash(uint32_t thing_to_hash);// hash algo by D. J. Bernstein (https://cr.yp.to/djb.html)

#endif /* MEMCLUSTER_H */