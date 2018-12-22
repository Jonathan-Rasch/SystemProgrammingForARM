#include "os.h"
#include "../utils/heap.h"
#include <stdio.h>
#include "utils/serial.h"
#include "patientPreemptivePriorityScheduler.h"
#include <stdlib.h>
#include "sleep.h"
#include "mutex.h"
static OS_mutex_t testMutex;

void task1(void const *const args) {
	while (1) {
		//OS_mutex_acquire(&testMutex);
		printf("1\t\t\t\t\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
		//OS_mutex_release(&testMutex);
	}
}
void task2(void const *const args) {
	while (1) {
		//OS_mutex_acquire(&testMutex);
		printf("\t\t2\t\t\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
		//OS_mutex_release(&testMutex);
	}
}
void task3(void const *const args) {
	while (1) {
		//OS_mutex_acquire(&testMutex);
		printf("\t\t\t\t3\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
		//OS_mutex_release(&testMutex);
	}
}
void task4(void const *const args) {
	while (1) {
		//OS_mutex_acquire(&testMutex);
		printf("\t\t\t\t\t\t4\t\tLock held by: %p\r\n",testMutex.tcbPointer);
		//OS_mutex_release(&testMutex);
	}
}

/* MAIN FUNCTION */

int main(void) {
	/* Initialise the serial port so printf() works */
	serial_init();

	printf("\r\nDocetOS Sleep and Mutex\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[64], stack2[64], stack3[64], stack4[64];
	static minHeapNode heapNodeArray[64];
	static OS_TCB_t TCB1, TCB2, TCB3, TCB4;
	
	OS_init_mutex(&testMutex);
	
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(&TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(&TCB2, stack2+64, task2, 0);
	OS_initialiseTCB(&TCB3, stack3+64, task3, 0);
	OS_initialiseTCB(&TCB4, stack4+64, task4, 0);

	/* Initialise and start the OS */
	patientPreemptivePriorityScheduler.initialize(heapNodeArray,64);
	OS_init(&patientPreemptivePriorityScheduler);
	OS_addTask(&TCB1,11);
	OS_addTask(&TCB2,5);
	OS_addTask(&TCB3,7);
	OS_addTask(&TCB4,2);
	OS_start();
}
