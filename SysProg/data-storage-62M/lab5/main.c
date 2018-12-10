#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "serial.h"
#include "mempool.h"

/* Example data packet structure, just for demonstration purposes */
typedef struct {
	uint32_t id;
	char data[10];
} packet_t;

int main(void) {
	serial_init();
	
	/* Declare and intialise a memory pool */
	static pool_t pool;
	static packet_t poolElements[10];
	pool_init(&pool);
	for (int i = 0; i < 10; ++i) {
		pool_add(&pool, &poolElements[i]);
	}
	
	/* Example use of the pool */
	
	/* Allocate two blocks for data packets and fill them in */
	packet_t *p1 = pool_allocate(&pool);
	p1->id = 0;
	strncpy(p1->data, "first", 10);

	packet_t *p2 = pool_allocate(&pool);
	p2->id = 1;
	strncpy(p2->data, "second", 10);

	printf("First allocated packet (id %d, data '%s') at address %p\r\n", p1->id, p1->data, p1);
	printf("Second allocated packet (id %d, data '%s') at address %p\r\n", p2->id, p2->data, p2);

	/* Return the first packet to the pool */
	pool_deallocate(&pool, p1);
	
	/* Allocating a third should now re-use the block from the deallocated first */
	packet_t *p3 = pool_allocate(&pool);
	p3->id = 2;
	strncpy(p3->data, "third", 10);

	packet_t *p4 = pool_allocate(&pool);
	p4->id = 3;
	strncpy(p4->data, "fourth", 10);

	printf("Third allocated packet (id %d, data '%s') at address %p\r\n", p3->id, p3->data, p3);
	printf("Fourth allocated packet (id %d, data '%s') at address %p\r\n", p4->id, p4->data, p4);
}
