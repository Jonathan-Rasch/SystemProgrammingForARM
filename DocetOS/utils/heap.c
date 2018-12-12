#include "heap.h"

//================================================================================
// Internal function prototypes
//================================================================================
static uint32_t _getFirstChildIndex(uint32_t node_index_zero_based);
static uint32_t _getSecondChildIndex(uint32_t node_index_zero_based);
static void * _getPointerToItemAtIndex(uint32_t node_index_zero_based);

//================================================================================
// Struct
//================================================================================
typedef struct s_node{
    /* Promising not to modify where the pointer points to or the underlying
     * value*/
    const void * const ptrToNodeContent;
    /* Node value is used when restoring heap*/
    const uint32_t nodeValue;
}node;

struct s_heap{
    /* the heap has an underlying array of nodes. each node holds a value that is used during the
     * restoration of the heap (node->nodeValue). */
    const node * const ptrToUnderlyingArray;
};

/* adding a Node (containing a pointer) to the heap*/
static uint32_t addNode(heap * heap_to_operate_on, const void * const element_To_Add, uint32_t value_to_order_by){

}


static uint32_t removeNode(heap * heap_to_operate_on, void * * elementToAdd){

}
