#ifndef MEMCLUSTER_H
#define MEMCLUSTER_H

#include <stddef.h>
#include <stdint.h>
#include "../OS/debug.h" 
#include "../OS/structs.h"

#define MEMPOOL_HASH_TABLE_BUCKET_NUM 3
#define SMALLEST_BLOCK_SIZE 4 //in ((2^N)*4)/1024 kb
#define LARGEST_BLOCK_SIZE 8 //in ((2^N)*4)/1024 kb
#define NUM_MEMPOOLS 5
#define NUMBER_OF_POOLS ((LARGEST_BLOCK_SIZE - SMALLEST_BLOCK_SIZE) + 1)

void memory_cluster_init(OS_memcluster_t * memory_cluster, uint32_t * memoryArray, uint32_t memory_Size);
void memory_cluster_setInternalLockState(uint32_t _enable);
uint32_t djb2_hash(uint32_t thing_to_hash);// hash algo by D. J. Bernstein (https://cr.yp.to/djb.html)

#endif /* MEMCLUSTER_H */

