#include "heap.h"

//================================================================================
// Internal Function Prototypes
//================================================================================
void printHeap(minHeap * heap);
static uint32_t __getParentIndex(uint32_t node_index_zero_based);
static minHeapNode * __getPointerToItemAtIndex(minHeap * heap, uint32_t node_index_zero_based);

//================================================================================
// Exported Functions
//================================================================================

/* Function that initialises a minHeap:
-> sets all node element content pointers to NULL
-> sets the priority/value of each node to UINT32_MAX. Note that this is the lowest priority since
the heap os a MIN_HEAP, meaning tha t the element with the lowest value is on top. Setting the value to
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
	heap_struct->currentNumNodes = 0;
	for (uint32_t i = 0;i<max_number_of_heap_nodes;i++){
		/*this is not really needed since the value of a node beyond write_ptr-1 is irrelevant,
		 * but this makes it easier to see what is going on when printing out heap content*/
			heap_struct->ptrToUnderlyingArray[i].ptrToNodeContent = NULL;
			heap_struct->ptrToUnderlyingArray[i].nodeValue = UINT32_MAX;
	};
}

/* 	adding a Node (containing a pointer) to the heap. The heap property is then restored automatically.

PARAMETERS:
-> _heap: the heap to which the element should be added
-> _element_to_add: Pointer to the element that the user wishes to add to the heap
-> _value_to_order_by: The heap needs to arrange its elements, for this it needs a value to use. Since every 
				element could be vastly different structs, int's or other types the user must provide this value

RETURNS:
-> status_code: 1 if element added succssfully, 0 if not (for example if the heap is full)*/
uint32_t addNode(minHeap * _heap, void * const _element_to_add, const uint32_t _value_to_order_by){
	#ifdef HEAP_DEBUG
	printf("\r\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DEBUG ADD NODE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
	printf("*** INITIAL STATE ***\r\n");
	printHeap(_heap);
	#endif /*HEAP_DEBUG*/
	// is there space in the heap ?
	if (_heap->lastArrayElement < _heap->nextEmptyElement){
		#ifdef HEAP_DEBUG
		printf("INFO: [utils/heap.c addNode(...)]\r\nAttempt to add node to minHeap that is full, returning 0.\r\n");
		printHeap(_heap); //DEBUG
		#endif /*HEAP_DEBUG*/
		return 0; // cant add node, no space
	}
	
	/* restoring heap property */
	volatile minHeapNode * node = _heap->nextEmptyElement;
	node->ptrToNodeContent = _element_to_add;
	node->nodeValue = _value_to_order_by;
	uint32_t currentNodeIndex = _heap->nextEmptyElement - _heap->ptrToUnderlyingArray;
	while(currentNodeIndex){
		#ifdef HEAP_DEBUG
		printf("RESTORING HEAP PROPERTY...\r\n");
		printHeap(_heap);
		#endif /*HEAP_DEBUG*/
		uint32_t parentNodeIndex = __getParentIndex(currentNodeIndex);
		minHeapNode * parentNode = __getPointerToItemAtIndex(_heap,parentNodeIndex);
		// compare and swap
		if (parentNode->nodeValue > node->nodeValue){
			minHeapNode * tempNodeArray = _heap->ptrToUnderlyingArray;
			minHeapNode tempNode = *node;
			tempNodeArray[currentNodeIndex] = tempNodeArray[parentNodeIndex];
			tempNodeArray[parentNodeIndex] = tempNode;
			currentNodeIndex = parentNodeIndex;//current node index is now index of previous parent (they swapped)
			node = &tempNodeArray[currentNodeIndex];// updating ref to current node
			continue; // child node is now at position of previous parent, repeat this procedure
		}else{
			break; // heap property restored
		}
	}
	/* updating empty element pointer */
	_heap->nextEmptyElement++;
	_heap->currentNumNodes = _heap->nextEmptyElement - _heap->ptrToUnderlyingArray;
	#ifdef HEAP_DEBUG
	printf("\r\n*** FINAL STATE ***\r\n");
	printHeap(_heap);
	#endif /*HEAP_DEBUG*/
	return 1;
}

/* Removes the node with the lowest node value (topmost node) from the heap and
replaces the pointer _return_content points to with the pointer contained in the node.
the heap property is restored automatically during this operation.

PARAMETERS:
-> _heap: heap from which to extract the pointer contained in node at index 0
-> _return_content:  pointer to pointer pointing to some data structure, pointer to data structure
										that the topmost node has is place in here. (WILL ONLY BE MODIFIED IF SUCCESSFULL,
										RETAINS VALUE OTHERWISE)

RETURNS:
->  status_code: 1 if element removed succssfully, 0 if not (for example if the heap is empty)*/
uint32_t removeNode(minHeap * _heap, void * * _return_content){
	#ifdef HEAP_DEBUG
	printf("\r\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DEBUG REMOVE NODE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
	printf("*** INITIAL STATE ***\r\n");
	printHeap(_heap); //DEBUG
	#endif /*HEAP_DEBUG*/
	
	/* checks */
	if(_heap->ptrToUnderlyingArray == _heap->nextEmptyElement){
		#ifdef HEAP_DEBUG
		printf("INFO: [utils/heap.c removeNode(...)]\r\nHeap is empty, cannot remove node.\r\n");
		printHeap(_heap); //DEBUG
		#endif /*HEAP_DEBUG*/
		return 0; // cant remove node, heap empty
	}
	
	/* return content pointer of element 0 (via _return_content) and restore heap property*/
	const minHeapNode nodeToReturn = _heap->ptrToUnderlyingArray[0];
	//swapping inserting lowest node at index 0
	uint32_t elemIdx = (_heap->nextEmptyElement-1) - _heap->ptrToUnderlyingArray;
	_heap->ptrToUnderlyingArray[0] = _heap->ptrToUnderlyingArray[elemIdx];
	_heap->nextEmptyElement--;//node removed, so need to move empty element pointer
	// restore heap property (heap down)
	elemIdx = 0;
	const int maxValidIdx = (_heap->nextEmptyElement-1) - _heap->ptrToUnderlyingArray;
	while(maxValidIdx != -1 ){// maxValidIdx is -1 when nextEmptyElement == ptrToUnderlyingArray
		#ifdef HEAP_DEBUG
		printf("RESTORING HEAP PROPERTY...\r\n");
		printHeap(_heap);
		#endif /*HEAP_DEBUG*/
		uint32_t firstChildIdx = getFirstChildIndex(elemIdx);
		uint32_t secondChildIdx = getSecondChildIndex(elemIdx);
		minHeapNode firstChild = _heap->ptrToUnderlyingArray[firstChildIdx];
		minHeapNode secondChild = _heap->ptrToUnderlyingArray[secondChildIdx];
		// are both returned indicies valid nodes (not UNUSED) ?
		uint32_t smallerChildIdx;
		if(secondChildIdx <= maxValidIdx){
			//no need to check first childIdx when second childIdx is valid
			smallerChildIdx = (firstChild.nodeValue <= secondChild.nodeValue)?firstChildIdx:secondChildIdx;
		}else if(firstChildIdx <= maxValidIdx){
			smallerChildIdx = firstChildIdx;
		}else{
			// node has no children, nothing to swap, heap property restored
			break;
		}
		// swap (only if node value is larger than that of child)
		minHeapNode tmpNode = _heap->ptrToUnderlyingArray[elemIdx];
		if (tmpNode.nodeValue > _heap->ptrToUnderlyingArray[smallerChildIdx].nodeValue){
			_heap->ptrToUnderlyingArray[elemIdx] = _heap->ptrToUnderlyingArray[smallerChildIdx];
			_heap->ptrToUnderlyingArray[smallerChildIdx] = tmpNode;
			elemIdx = smallerChildIdx;
		}else{
			//node value is smaller than its children, heap property restored
			break;
		}
	}
	
	/* returning values*/
	#ifdef HEAP_DEBUG
	printf("*** FINAL STATE ***\r\n");
	printHeap(_heap);
    #endif /*HEAP_DEBUG*/
	_heap->currentNumNodes = _heap->nextEmptyElement - _heap->ptrToUnderlyingArray;
	*_return_content = nodeToReturn.ptrToNodeContent;
	return 1;
}

// TODO : determine if this is still needed
//uint32_t getIndexOfNodeWithThisContent(minHeap * _heap,  void * const _content_ptr, uint32_t * _return_index){
//	//TODO need hashmap implementation for improved efficiency
//}

/* Obtain the index of the first child of the given node
(Note: the index for nodes in this min heap is ZERO BASED!)*/
uint32_t getFirstChildIndex(uint32_t node_index_zero_based){
	return 2*node_index_zero_based + 1;
}

/* Obtain the index of the second child of the given node
(Note: the index for nodes in this min heap is ZERO BASED!)*/
uint32_t getSecondChildIndex(uint32_t node_index_zero_based){
	return 2*(node_index_zero_based+1);
}

//================================================================================
// Internal Functions
//================================================================================

/* Get the index of the parent of node C
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static uint32_t __getParentIndex(uint32_t node_index_zero_based){
	/* checks: */
	if (node_index_zero_based == 0){
		#ifdef HEAP_DEBUG
		printf("WARNING: [utils/heap.c __getParentIndex(...)]\r\nAttempt to get parent of minHeap node index 0 (topmost node)\r\n");
		#endif /*HEAP_DEBUG*/
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
		printf("ERROR: [utils/heap.c __getPointerToItemAtIndex(...)]\r\nProvided index %d is outside of heap size %d\r\n",node_index_zero_based,heap->maxNumberOfNodes);
		#ifdef HEAP_DEBUG
		ASSERT(0);
		#endif /*HEAP_DEBUG*/
		return NULL;
	}
	/* get pointer:*/
	return &(heap->ptrToUnderlyingArray[node_index_zero_based]);
}

//================================================================================
// DEBUG FUNCTIONS
//================================================================================

/*Debug function to print heap content and information. Note: REQUIRES MONOSPACED FONT!!!
Example of output:
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DEBUG <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
----------------------------------------------------------------------------------------------------------------
GENERAL INFO:
capacity:					5
ptr_to_underlying_array:	200009d4
ptr_next_empty_elem:		200009e4
ptr_last_elem:				200009f4

HEAP CONTENTS:
[STATUS]            [INDEX]             [RELATION]          [NODE_VALUE]        [NODE_PTR]          [PARENT_PTR]       
                    0                   N/A                 41                  200009d4            N/A                 
                    1                   0.0                 62                  200009dc            200009d4            
-WPtr->             2                   0.1                 -1                  200009e4            200009d4            
[UNUSED]            3                   1.0                 -1                  200009ec            200009dc            
[UNUSED]            4                   1.1                 -1                  200009f4            200009dc            
----------------------------------------------------------------------------------------------------------------
*/
void printHeap(minHeap * heap){
	printf("----------------------------------------------------------------------------------------------------------------\r\n");
	printf("GENERAL INFO:\r\n");
	printf("capacity:%-40d\r\n",heap->maxNumberOfNodes);
	printf("ptr_to_underlying_array:%-40p\r\n",heap->ptrToUnderlyingArray);
	printf("ptr_next_empty_elem:%-40p\r\n",heap->nextEmptyElement);
	printf("ptr_last_elem:%-40p\r\n",heap->lastArrayElement);
	printf("\r\nHEAP CONTENTS:\r\n");
	printf("%-20s%-20s%-20s%-20s%-20s%-20s\r\n","[STATUS]","[INDEX]","[RELATION]","[NODE_VALUE]","[NODE_PTR]","[PARENT_PTR]");
	//zero node
	minHeapNode * zero_node = heap->ptrToUnderlyingArray;
	if(heap->ptrToUnderlyingArray == heap->nextEmptyElement){
		printf("%-20s%-20d%-20s%-20d%-20p%-20s\r\n","-WPtr->",0,"N/A",zero_node->nodeValue,zero_node,"N/A");
	}else{
		printf("%-20s%-20d%-20s%-20d%-20p%-20s\r\n","",0,"N/A",zero_node->nodeValue,zero_node,"N/A");
	}
	//all other nodes
	for(int i=1;i<heap->maxNumberOfNodes;i++){
		int parentIdx = __getParentIndex(i);
		int childNum = (i % 2) ? 0:1;
		minHeapNode * node = __getPointerToItemAtIndex(heap,i);
		minHeapNode * parentNode = __getPointerToItemAtIndex(heap,parentIdx);
		char relation[64];
		sprintf(relation,"%d.%d",parentIdx,childNum);
		if(node < heap->nextEmptyElement){
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","",i,relation,node->nodeValue,node,parentNode);
		}else if (node == heap->nextEmptyElement){
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","-WPtr->",i,relation,node->nodeValue,node,parentNode);
		}else{
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","[UNUSED]",i,relation,node->nodeValue,node,parentNode);
		}
	}
	printf("----------------------------------------------------------------------------------------------------------------\n\r\n");
}
