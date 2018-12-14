#ifndef DOCETOS_HEAP_H
#define DOCETOS_HEAP_H

#include <stdio.h>
#include <stdint.h>
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
    minHeapNode * ptrToUnderlyingArray;
		uint32_t maxNumberOfNodes;
		minHeapNode * nextEmptyElement;
		minHeapNode * lastArrayElement;
		
}minHeap;

//=============================================================================
// Exported Functions
//=============================================================================
void initHeap(minHeapNode * node_Array, minHeap * heap_struct, int max_number_of_heap_nodes);
uint32_t addNode(minHeap * heap_to_operate_on, void * const element_to_add, const uint32_t value_to_order_by);
uint32_t removeNode(minHeap * _heap, void * * _return_content);
void printHeap(minHeap * heap);

#endif //DOCETOS_HEAP_H