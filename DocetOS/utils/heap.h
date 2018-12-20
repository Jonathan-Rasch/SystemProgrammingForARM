#ifndef DOCETOS_HEAP_H
#define DOCETOS_HEAP_H

#include <stdio.h>
#include <stdint.h>
#include "debug.h"

#define MAX_HEAP_SIZE 63 // 2^N - 1 , where n is the number of desired levels in binary tree

typedef struct __s_node{
    /* Promising not to modify where the pointer points to or the underlying
     * value*/
    void * ptrToNodeContent;
    /* Node value is used when restoring heap*/
    uint32_t nodeValue;
}minHeapNode;

typedef struct __s_heap{
    /* the heap has an underlying array of nodes. each node holds a value that is used during the
     * restoration of the heap (node->nodeValue). */
    minHeapNode *   ptrToUnderlyingArray;
    uint32_t        maxNumberOfNodes;
    uint32_t        currentNumNodes;
    minHeapNode *   nextEmptyElement;
    minHeapNode *   lastArrayElement;

}minHeap;

//=============================================================================
// Exported Functions
//=============================================================================
void initHeap(minHeapNode * node_Array, minHeap * heap_struct, int max_number_of_heap_nodes);
/*NOTE:
 * the return value (uint32_t) in these functions ALWAYS indicates a status code (success/failure) of
 * the operation. Other returns (e.g index of a node) are obtained */
uint32_t addNode(minHeap * heap_to_operate_on, void * const element_to_add, const uint32_t value_to_order_by);
uint32_t removeNode(minHeap * _heap, void * * _return_content);
uint32_t removeNodeAtIndex(minHeap * _heap, void * * _return_content); //TODO implement
uint32_t getIndexOfNodeWithThisContent(minHeap * _heap,  void * const _content_ptr, uint32_t _return_index); //TODO implement

void printHeap(minHeap * heap);

#endif //DOCETOS_HEAP_H