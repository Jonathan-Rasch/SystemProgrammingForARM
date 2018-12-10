#include <stdio.h>
#include <stdint.h>
#include "serial.h"

void fib(uint32_t n);

void report(uint32_t value) {
	printf("%d\r\n", value);
}

int main(void) {
	serial_init();

	fib(20);

	while(1);
}
