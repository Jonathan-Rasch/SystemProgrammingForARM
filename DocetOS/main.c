#include "os.h"
#include "../utils/heap.h"
#include <stdio.h>
#include "utils/serial.h"
#include "simpleRoundRobin.h"
#include <stdlib.h>
void task1(void const *const args) {
	while (0) {
		printf("Message from Task 1\r\n");
	}
}
void task2(void const *const args) {
	while (0) {
		printf("Message from Task 2\r\n");
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
	static uint32_t stack1[64], stack2[64];
	static OS_TCB_t TCB1, TCB2;
	
	/* HEAP TESTING*/
	minHeap heap;
	minHeapNode node_array[8];
	initHeap(node_array,&heap,8);
	srand(1231412);
	//testing add node
	for(int i=10;i>0;i--){
		stack1[i] = rand() % 20;
		int status_code = addNode(&heap,&stack1[i],stack1[i]);
		if(status_code){
			printf("ADDED: %d\r\n",stack1[i]);
		}else{
			printf("HEAP FULL, NOTHING ADDED\r\n");
		}
	}
	//testing remove node
	for(int i=10;i>0;i--){
		uint32_t * returned_val;
		int status_code = removeNode(&heap,(void *)&returned_val);
		if(status_code){
			printf("REMOVED: %d\r\n",*returned_val);
		}else{
			printf("HEAP EMPTY, NOTHING REMOVED\r\n");
		}
	}
	//testing add and remove node
	for(int i=3;i>0;i--){
		stack1[i] = rand() % 20;
		int status_code = addNode(&heap,&stack1[i],stack1[i]);
		if(status_code){
			printf("ADDED: %d\r\n",stack1[i]);
		}else{
			printf("HEAP FULL, NOTHING ADDED\r\n");
		}
	}
	for(int i=4;i>0;i--){
		uint32_t * returned_val;
		int status_code = removeNode(&heap,(void *)&returned_val);
		if(status_code){
			printf("REMOVED: %d\r\n",*returned_val);
		}else{
			printf("HEAP EMPTY, NOTHING REMOVED\r\n");
		}
	}
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(&TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(&TCB2, stack2+64, task2, 0);

	/* Initialise and start the OS */
	OS_init(&simpleRoundRobinScheduler);
	OS_addTask(&TCB1);
	OS_addTask(&TCB2);
	OS_start();
}
