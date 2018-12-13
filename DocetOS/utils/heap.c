#include "heap.h"

//=============================================================================
// Internal function prototypes
//=============================================================================
static uint32_t _getFirstChildIndex(uint32_t node_index_zero_based);
static uint32_t _getSecondChildIndex(uint32_t node_index_zero_based);
static void * _getPointerToItemAtIndex(uint32_t node_index_zero_based);

void initHeap(minHeapNode * node_Array, minHeap * heap_struct, int max_number_of_heap_nodes){
	heap_struct->ptrToUnderlyingArray = node_Array;
	heap_struct->maxNumberOfNodes = max_number_of_heap_nodes; 
	heap_struct->nextEmptyElement = node_Array; //at first the first free heap node is the topmost node
	heap_struct->lastArrayElement = node_Array + (max_number_of_heap_nodes-1); // points to last node in heap
	for (uint32_t i = 0;i<max_number_of_heap_nodes;i++){
			heap_struct->ptrToUnderlyingArray[i].ptrToNodeContent = NULL;
			heap_struct->ptrToUnderlyingArray[i].nodeValue = UINT32_MAX;
	};
}

/* 	adding a Node (containing a pointer) to the heap. The heap property is then restored

PARAMETERS:
heap_to_operate_on: the heap to which the element should be added
element_to_Add: Pointer to the element that the user wishes to add to the heap
value_to_order_by: The heap needs to arrange its elements, for this it needs a value to use. Since every 
	element could be vastly different structs, int's or other types the user must provide this value

RETURNS:
status_code: 1 if element added succssfully, 0 if not (for example if the heap is full)*/
uint32_t addNode(minHeap * heap_to_operate_on, void * const element_to_add, const uint32_t value_to_order_by){
	/* Checking if the heap can take another node*/
	// is there space in the heap ?
	if (heap_to_operate_on->lastArrayElement < heap_to_operate_on->nextEmptyElement){
		
	}
	//
	//creating a new node containing the element pointer and the value used to sort
	minHeapNode newNode = {
		.ptrToNodeContent = element_to_add,
		.nodeValue = value_to_order_by
	};
	return 0;
}


uint32_t removeNode(minHeap * heap_to_operate_on, void * * elementToAdd){
	return 0;
}
