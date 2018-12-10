#include "os.h"
#include <stdio.h>
#include "utils/serial.h"
#include "simpleRoundRobin.h"

void task1(void const *const args) {
	for (int i = 0; i < 100; ++i) {
		printf("A");
	}
}
#include "stm32f4xx.h"
void task2(void const *const args) {
	for (int i = 0; i < 200; ++i) {
		printf("B");
	}
}

/* MAIN FUNCTION */

int main(void) {
	/* Initialise the serial port so printf() works */
	serial_init();

	printf("\r\nDocetOS Demo\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[64], stack2[64];
	static OS_TCB_t TCB1, TCB2;

	/* Initialise the TCBs using the two functions above */


	/* Initialise and start the OS */
	
}
