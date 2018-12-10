#include <stdio.h>
#include <stdint.h>
#include "serial.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
	int32_t x;
	int32_t y;
	void *ptr;
	void (*callback) (void *);
} structure_t;

#pragma pack(pop)

int main(void) {
	serial_init();

	// Your code goes here...
	
	while(1);
}
