#include <stdio.h>
#include <stdint.h>
#include "serial.h"

void test(void) {
	printf("Hello!\r\n");
}

int main(void) {
	serial_init();

	uint32_t array[10];
	
  // First try overflowing the stack with garbage and see what happens
	// Then try to overwrite the link register here so that test() is run
}
