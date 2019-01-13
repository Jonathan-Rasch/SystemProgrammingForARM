#ifndef DOCETOS_HEAP_H
#define DOCETOS_HEAP_H

#include <stdio.h>
#include <stdint.h>
#include "debug.h"
#include "structs.h"
#include "os.h"
#include "hashtable.h"
#define MAX_HEAP_SIZE 63 // 2^N - 1 , where n is the number of desired levels in binary tree
#define CONTENT_INDEX_LOOKUP_HASHTABLE_BUCKETS_NUM 8


//=============================================================================
// Exported Functions
//=============================================================================
OS_minHeap_t * new_heap(uint32_t max_number_of_heap_nodes,uint32_t _enableQuickNodeContentIndexLookup);
/*NOTE:
 * the return value (uint32_t) in these functions ALWAYS indicates a status code (success/failure) of
 * the operation. Other returns (e.g index of a node) are obtained */
uint32_t addNode(OS_minHeap_t * heap_to_operate_on, void * const element_to_add, const uint32_t value_to_order_by);
uint32_t removeNode(OS_minHeap_t * _heap, void * * _return_content);
uint32_t removeNodeAt(OS_minHeap_t * _heap,uint32_t _index, void * * _return_content);
uint32_t * peek(OS_minHeap_t * _heap,uint32_t _index);
uint32_t indexOfContent(OS_minHeap_t * _heap,uint32_t _content);
uint32_t getFirstChildIndex(uint32_t node_index_zero_based);
uint32_t getSecondChildIndex(uint32_t node_index_zero_based);
void printHeap(OS_minHeap_t * heap);

#endif //DOCETOS_HEAP_H

