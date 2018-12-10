#include <stdio.h>
#include <stdint.h>
#include "serial.h"

void count(void) {
	uint32_t counter = 0;
	counter++;
	printf("count() called, value = %d\r\n", counter);
}

int main(void) {
	serial_init();
	
	while(1) {
		count();
	}
}
