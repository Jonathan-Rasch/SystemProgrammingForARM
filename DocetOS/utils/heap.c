#include "heap.h"
#include "debug.h"
//================================================================================
// Internal Function Prototypes
//================================================================================
static uint32_t __getFirstChildIndex(uint32_t node_index_zero_based);
static uint32_t __getSecondChildIndex(uint32_t node_index_zero_based);
static uint32_t __getParentIndex(uint32_t node_index_zero_based);
static minHeapNode * __getPointerToItemAtIndex(minHeap * heap, uint32_t node_index_zero_based);

//================================================================================
// Exported Functions
//================================================================================

/* Function that initialises a minHeap:
-> sets all node element content pointers to NULL 
-> sets the priority/value of each node to UINT32_MAX. Note that this is the lowest priority since
the heap os a MIN_HEAP, meaning that the element with the lowest value is on top. Setting the value to
the minimum ensures that empty nodes are never the parent of nodes that actyally point to something
usefull.
*/
void initHeap(minHeapNode * node_Array, minHeap * heap_struct, int max_number_of_heap_nodes){
	/*assertions for debugging*/
	ASSERT(max_number_of_heap_nodes > 0);
	
	/* setting up heap with default node values*/
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
	// is there space in the heap ?
	if (heap_to_operate_on->lastArrayElement < heap_to_operate_on->nextEmptyElement){
		
		printf("INFO: [utils/heap.c addNode(...)]\nAttempt to add node to minHeap that is full, returning 0.\r\n");
		#ifdef DEBUG_V
		ASSERT(0);
		#endif /*DEBUG_V*/
		return 0; // cant add node, no space
	}
	// 
	minHeapNode * node = heap_to_operate_on->nextEmptyElement;
	node->ptrToNodeContent = element_to_add;
	node->nodeValue = value_to_order_by;
	/* restoring heap property */
	//gives index because of "C Standard (6.5.6 Additive operators)"
	uint32_t currentNodeIndex = heap_to_operate_on->nextEmptyElement - heap_to_operate_on->ptrToUnderlyingArray;
	while(currentNodeIndex){
		uint32_t parentNodeIndex = __getParentIndex(currentNodeIndex);
		minHeapNode * parentNode = __getPointerToItemAtIndex(heap_to_operate_on,parentNodeIndex);
		// compare and swap
		if (parentNode->nodeValue > node->nodeValue){
			minHeapNode * tempNodeArray = heap_to_operate_on->ptrToUnderlyingArray;
			minHeapNode tempNode = *node;
			tempNodeArray[currentNodeIndex] = tempNodeArray[parentNodeIndex];
			tempNodeArray[parentNodeIndex] = tempNode;
			continue; // child node is now at position of previous parent, repeat this procedure
		}else{
			break; // heap property restored
		}
	}
	/* updating empty element pointer */
	heap_to_operate_on->nextEmptyElement++;
	return 1;
}


uint32_t removeNode(minHeap * heap_to_operate_on, void * * elementToAdd){
	return 0;
}

//================================================================================
// Internal Functions
//================================================================================

/* Obtain the index of the first child of the given node
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static uint32_t __getFirstChildIndex(uint32_t node_index_zero_based){
	return 2*node_index_zero_based + 1;
}

/* Obtain the index of the second child of the given node
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static uint32_t __getSecondChildIndex(uint32_t node_index_zero_based){
	return 2*(node_index_zero_based+1);
}

/* Get the index of the parent of node C
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static uint32_t __getParentIndex(uint32_t node_index_zero_based){
	/* checks: */
	if (node_index_zero_based == 0){
		
		printf("WARNING: [utils/heap.c __getParentIndex(...)]\nAttempt to get parent of minHeap node index 0 (topmost node)\r\n");
		#ifdef DEBUG_V
		ASSERT(0);
		#endif /*DEBUG_V*/
		return 0;
	}
	/* get index: */
	if (node_index_zero_based % 2){
		return (node_index_zero_based-1)/2;
	} else{
		return (node_index_zero_based/2)-1;
	}
}

/* Get the pointer of node at specified index in heap
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static minHeapNode * __getPointerToItemAtIndex(minHeap * heap, uint32_t node_index_zero_based){
	/* checks:*/
	if(heap->maxNumberOfNodes <= node_index_zero_based){
		printf("ERROR: [utils/heap.c __getPointerToItemAtIndex(...)]\nProvided index %d is outside of heap size %d\r\n",node_index_zero_based,heap->maxNumberOfNodes);
		ASSERT(0);
		return NULL;
	}
	/* get pointer:*/
	return &(heap->ptrToUnderlyingArray[node_index_zero_based]);
}
