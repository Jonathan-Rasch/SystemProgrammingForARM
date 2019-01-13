#include "heap.h"
#include "os.h"
#include "../hashtable.h"
//================================================================================
// Internal Function Prototypes
//================================================================================
void printHeap(minHeap * heap);
static uint32_t __getParentIndex(uint32_t node_index_zero_based);
static OS_minHeapNode_t * __getPointerToItemAtIndex(minHeap * heap, uint32_t node_index_zero_based);
static void __heapDown(minHeap * _heap,uint32_t _index_of_node_to_remove);
//================================================================================
// Exported Functions
//================================================================================

/* Function that initialises a minHeap:
-> sets all node element content pointers to NULL
-> sets the priority/value of each node to UINT32_MAX. Note that this is the lowest priority since
the heap os a MIN_HEAP, meaning tha t the element with the lowest value is on top. Setting the value to
the minimum ensures that empty nodes are never the parent of nodes that actyally point to something
usefull.
-> if _enableQuickNodeContentIndexLookup is > 1 a hashtable will be created that maps the "content"
	 to the node index, this is useful for quickly retrieving the location of the node that stores "content"
*/
minHeap * new_heap(uint32_t max_number_of_heap_nodes, uint32_t _enableQuickNodeContentIndexLookup){
	
	//allocating memory
	minHeap * heap_struct = (minHeap *)OS_alloc(sizeof(minHeap)/4);
	OS_minHeapNode_t * node_Array = (OS_minHeapNode_t *)OS_alloc(max_number_of_heap_nodes*sizeof(OS_minHeapNode_t)/4);

	/*This adds a hashtable which stores the index of the node with a given content. This allows the user to quickly obtain the index of any node
	given that the user knows the content pointer they are trying to locate. this is useful in combination with the removeNodeAt function*/
	if(_enableQuickNodeContentIndexLookup){
        heap_struct->nodeContentIndexHashTable = new_hashtable(max_number_of_heap_nodes,CONTENT_INDEX_LOOKUP_HASHTABLE_BUCKETS_NUM);
	}else{
		heap_struct->nodeContentIndexHashTable = NULL;
	}
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
	
	return heap_struct;
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
	// is there space in the heap ?
	if (_heap->lastArrayElement < _heap->nextEmptyElement){
		return 0; // cant add node, no space
	}
	
	/* restoring heap property */
	volatile OS_minHeapNode_t * node = _heap->nextEmptyElement;
	node->ptrToNodeContent = _element_to_add;
	node->nodeValue = _value_to_order_by;
	uint32_t currentNodeIndex = _heap->nextEmptyElement - _heap->ptrToUnderlyingArray;
	/*add new node to content index hash table if applicable*/
	if(_heap->nodeContentIndexHashTable){
    hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)node->ptrToNodeContent);
		hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)node->ptrToNodeContent,(uint32_t*)currentNodeIndex, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	}
	while(currentNodeIndex){
		uint32_t parentNodeIndex = __getParentIndex(currentNodeIndex);
		OS_minHeapNode_t * parentNode = __getPointerToItemAtIndex(_heap,parentNodeIndex);
		// compare and swap
		if (parentNode->nodeValue > node->nodeValue){
			/*if applicable update the indexes in the hashtable used to quickly retrieve the index a certain node with "content" can be found at*/
			if(_heap->nodeContentIndexHashTable){
				hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)node->ptrToNodeContent);
				if(!_heap->nodeContentIndexHashTable->validValueFlag){
					printf("\r\nHEAP: ERROR, invalid nodeContentIndexHashTable state!\r\n");
					DEBUG_printHashtable(_heap->nodeContentIndexHashTable);
					ASSERT(0);
				}
				hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)parentNode->ptrToNodeContent);
				if(!_heap->nodeContentIndexHashTable->validValueFlag){
					printf("\r\nHEAP: ERROR, invalid nodeContentIndexHashTable state!\r\n");
					DEBUG_printHashtable(_heap->nodeContentIndexHashTable);
					ASSERT(0);
				}
				hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)node->ptrToNodeContent,(uint32_t*)parentNodeIndex, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
        hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)parentNode->ptrToNodeContent,(uint32_t*)currentNodeIndex, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
				
			}
			/*do the swap*/
			OS_minHeapNode_t * tempNodeArray = _heap->ptrToUnderlyingArray;
			OS_minHeapNode_t tempNode = *node;
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
	/* checks */
	if(_heap->ptrToUnderlyingArray == _heap->nextEmptyElement){
		return 0; // cant remove node, heap empty
	}
	/* return content pointer of element 0 (via _return_content) and restore heap property*/
	const OS_minHeapNode_t nodeToReturn = _heap->ptrToUnderlyingArray[0];
	__heapDown(_heap,0);
	/* returning values*/
	*_return_content = nodeToReturn.ptrToNodeContent;
	return 1;
}

/*return content of node at _index in the heap without removing it, or NULL if _index is not valid*/
uint32_t * peek(minHeap * _heap,uint32_t _index){
	if(_index >= _heap->currentNumNodes){
		return NULL;
	}
	const OS_minHeapNode_t nodeToReturn = _heap->ptrToUnderlyingArray[_index];
	return nodeToReturn.ptrToNodeContent;
}

/*if this was enabled during init this function can retrieve the index at which a node containing "content" is located*/
uint32_t indexOfContent(minHeap * _heap,uint32_t _content){
	if(!_heap->nodeContentIndexHashTable){
		printf("\r\nHEAP: ERROR, quick index lockup was not enabled during init of this heap!\r\n");
		ASSERT(0);
		return 0;
	}
	uint32_t index = (uint32_t)hashtable_get(_heap->nodeContentIndexHashTable,_content);
	if(!_heap->nodeContentIndexHashTable->validValueFlag){
		printf("\r\nHEAP: ERROR, index of content could not be found!\r\n");
		return UINT32_MAX;
	}else{
		return index;
	}
}

/* Removes the node at the given index from the heap and places its content into _return_content

PARAMETERS:
-> _heap: heap from which to extract the pointer contained in node at index _index
-> _return_content:  pointer to pointer pointing to some data structure, pointer to data structure
										that the node at _index has is place in here. (WILL ONLY BE MODIFIED IF SUCCESSFULL,
										RETAINS VALUE OTHERWISE)

RETURNS:
->  status_code: 1 if element removed succssfully, 0 if not*/
uint32_t removeNodeAt(minHeap * _heap,uint32_t _index, void * * _return_content){
	/*is the provided index within the range of valid nodes?*/
	if(_index > _heap->currentNumNodes-1){
		printf("ERROR: the provided index %d is outside of the heap (max index %d)",_index,_heap->currentNumNodes-1);
		return 0;
	}
	const OS_minHeapNode_t nodeToReturn = _heap->ptrToUnderlyingArray[_index];
	__heapDown(_heap,_index);
	*_return_content = nodeToReturn.ptrToNodeContent;
	return 1;
}

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

static void __heapDown(minHeap * _heap,uint32_t _index_of_node_to_remove){
	uint32_t elemIdx = (_heap->nextEmptyElement-1) - _heap->ptrToUnderlyingArray; // index of lowest heap node
	/*if applicable update the indexes in the hashtable used to quickly retrieve the index a certain node with "content" can be found at*/
	if(_heap->nodeContentIndexHashTable){
		OS_minHeapNode_t lowestIdxNode = _heap->ptrToUnderlyingArray[elemIdx];
		OS_minHeapNode_t nodeToRemove = _heap->ptrToUnderlyingArray[_index_of_node_to_remove];
		uint32_t * ret1 = hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)lowestIdxNode.ptrToNodeContent);
		uint32_t * ret2 = hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)nodeToRemove.ptrToNodeContent);
		uint32_t ret3 = hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)lowestIdxNode.ptrToNodeContent,(uint32_t*)_index_of_node_to_remove, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	}
	//swapping, inserting lowest node at index _index_of_node_to_remove
	_heap->ptrToUnderlyingArray[_index_of_node_to_remove] = _heap->ptrToUnderlyingArray[elemIdx];
	//erasing data from node (this is not strictly necessary)
	_heap->ptrToUnderlyingArray[elemIdx].nodeValue = UINT32_MAX;
	_heap->ptrToUnderlyingArray[elemIdx].ptrToNodeContent = NULL;
	//updating number of nodes counter and empty node pointer (one less node now)
	_heap->nextEmptyElement--;//node removed, so need to move empty element pointer
	_heap->currentNumNodes = _heap->nextEmptyElement - _heap->ptrToUnderlyingArray;
	//if the removed node was the lowest node in heap then the heap property is intact.
	if(elemIdx == _index_of_node_to_remove){
		return;
	}
	// restore heap property (heap down)
	elemIdx = _index_of_node_to_remove; //index where node was removed
	const int maxValidIdx = (_heap->nextEmptyElement-1) - _heap->ptrToUnderlyingArray;
	while(maxValidIdx != -1 ){// maxValidIdx is -1 when nextEmptyElement == ptrToUnderlyingArray
		uint32_t firstChildIdx = getFirstChildIndex(elemIdx);
		uint32_t secondChildIdx = getSecondChildIndex(elemIdx);
		OS_minHeapNode_t firstChild = _heap->ptrToUnderlyingArray[firstChildIdx];
		OS_minHeapNode_t secondChild = _heap->ptrToUnderlyingArray[secondChildIdx];
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
		OS_minHeapNode_t currentNode = _heap->ptrToUnderlyingArray[elemIdx];
		if (currentNode.nodeValue > _heap->ptrToUnderlyingArray[smallerChildIdx].nodeValue){
			/*if applicable update the indexes in the hashtable used to quickly retrieve the index at which a certain node with "content" can be found at*/
			if(_heap->nodeContentIndexHashTable){
				OS_minHeapNode_t childNode = _heap->ptrToUnderlyingArray[smallerChildIdx];
				hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)currentNode.ptrToNodeContent);
				hashtable_remove(_heap->nodeContentIndexHashTable,(uint32_t)childNode.ptrToNodeContent);
				hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)currentNode.ptrToNodeContent,(uint32_t*)smallerChildIdx, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
				hashtable_put(_heap->nodeContentIndexHashTable,(uint32_t)childNode.ptrToNodeContent,(uint32_t*)elemIdx, HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
			}
			/*swap*/
			_heap->ptrToUnderlyingArray[elemIdx] = _heap->ptrToUnderlyingArray[smallerChildIdx];
			_heap->ptrToUnderlyingArray[smallerChildIdx] = currentNode;
			elemIdx = smallerChildIdx;
		}else{
			//node value is smaller than its children, heap property restored
			break;
		}
	}
}

/* Get the index of the parent of node C
(Note: the index for nodes in this min heap is ZERO BASED!)*/
static uint32_t __getParentIndex(uint32_t node_index_zero_based){
	/* checks: */
	if (node_index_zero_based == 0){
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
static OS_minHeapNode_t * __getPointerToItemAtIndex(minHeap * heap, uint32_t node_index_zero_based){
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
[STATUS]            [INDEX]             [RELATION]          [NODE_VALUE]        [NODE_PTR]          [CONTENT_PTR]       
                    0                   N/A                 41                  200009d4            N/A                 
                    1                   0.0                 62                  200009dc            2000055a            
-WPtr->             2                   0.1                 -1                  200009e4            200109d4            
[UNUSED]            3                   1.0                 -1                  200009ec            20220111            
[UNUSED]            4                   1.1                 -1                  200009f4            20123123            
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
	printf("%-20s%-20s%-20s%-20s%-20s%-20s\r\n","[STATUS]","[INDEX]","[RELATION]","[NODE_VALUE]","[NODE_PTR]","[CONTENT_PTR]");
	//zero node
	OS_minHeapNode_t * zero_node = heap->ptrToUnderlyingArray;
	if(heap->ptrToUnderlyingArray == heap->nextEmptyElement){
		printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","-WPtr->",0,"N/A",zero_node->nodeValue,zero_node,zero_node->ptrToNodeContent);
	}else{
		printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","",0,"N/A",zero_node->nodeValue,zero_node,zero_node->ptrToNodeContent);
	}
	//all other nodes
	for(int i=1;i<heap->maxNumberOfNodes;i++){
		int parentIdx = __getParentIndex(i);
		int childNum = (i % 2) ? 0:1;
		OS_minHeapNode_t * node = __getPointerToItemAtIndex(heap,i);
		OS_minHeapNode_t * parentNode = __getPointerToItemAtIndex(heap,parentIdx);
		char relation[64];
		sprintf(relation,"%d.%d",parentIdx,childNum);
		if(node < heap->nextEmptyElement){
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","",i,relation,node->nodeValue,node,node->ptrToNodeContent);
		}else if (node == heap->nextEmptyElement){
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","-WPtr->",i,relation,node->nodeValue,node,node->ptrToNodeContent);
		}else{
			printf("%-20s%-20d%-20s%-20d%-20p%-20p\r\n","[UNUSED]",i,relation,node->nodeValue,node,node->ptrToNodeContent);
		}
	}
	printf("----------------------------------------------------------------------------------------------------------------\n\r\n");
}
