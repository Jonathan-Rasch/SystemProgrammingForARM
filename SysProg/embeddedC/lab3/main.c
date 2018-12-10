#include <stdio.h>
#include <stdint.h>
#include "serial.h"
#include "count.h"

int main(void) {
	serial_init();
	
	counter = 0;
	printf("Counter value: %d\r\n", counter);
	count();
	printf("Counter value: %d\r\n", counter);
	count();
	printf("Counter value: %d\r\n", counter);
}
