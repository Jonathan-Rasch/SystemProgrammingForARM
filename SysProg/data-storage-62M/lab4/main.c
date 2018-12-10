#include <stdio.h>
#include "serial.h"
#include "heap.h"

int main(void) {
	serial_init();	
	
	static heap_t heap;
	static int heapStore[20];
	heap_init(&heap, heapStore);
	
	// Remember not to insert so many things that the heap overflows!
	heap_insert(&heap, 2);
	heap_insert(&heap, 4);
	heap_insert(&heap, 8);
	heap_insert(&heap, 9);
	heap_insert(&heap, 7);
	heap_insert(&heap, 1);
	heap_insert(&heap, 4);
	heap_insert(&heap, 1);
	heap_insert(&heap, 3);
	heap_insert(&heap, 2);
	heap_insert(&heap, 8);
	heap_insert(&heap, 3);
	heap_insert(&heap, 2);
	heap_insert(&heap, 1);
	heap_insert(&heap, 2);
	heap_insert(&heap, 3);
	heap_insert(&heap, 7);
	heap_insert(&heap, 6);
	heap_insert(&heap, 6);
	heap_insert(&heap, 1);
	
	while (!heap_isEmpty(&heap)) {
		printf("Extracted %d\r\n", heap_extract(&heap));
	}
}
