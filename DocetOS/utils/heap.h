#ifndef DOCETOS_HEAP_H
#define DOCETOS_HEAP_H

#include <stdio.h>
#include <stdint.h>
#include "debug.h"
#include "..\structs.h"

#define MAX_HEAP_SIZE 63 // 2^N - 1 , where n is the number of desired levels in binary tree



//=============================================================================
// Exported Functions
//=============================================================================
minHeap * initHeap(OS_memcluster * memcluster, int max_number_of_heap_nodes);
/*NOTE:
 * the return value (uint32_t) in these functions ALWAYS indicates a status code (success/failure) of
 * the operation. Other returns (e.g index of a node) are obtained */
uint32_t addNode(minHeap * heap_to_operate_on, void * const element_to_add, const uint32_t value_to_order_by);
uint32_t removeNode(minHeap * _heap, void * * _return_content);
uint32_t removeNodeAtIndex(minHeap * _heap, uint32_t _index, void * * _return_content); //TODO implement
uint32_t getIndexOfNodeWithThisContent(minHeap * _heap,  void * const _content_ptr, uint32_t * _return_index);
uint32_t getFirstChildIndex(uint32_t node_index_zero_based);
uint32_t getSecondChildIndex(uint32_t node_index_zero_based);
void printHeap(minHeap * heap);

#endif //DOCETOS_HEAP_H

