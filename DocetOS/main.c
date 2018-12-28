#include "os.h"
#include "utils/heap.h"
#include <stdio.h>
#include "utils/serial.h"
#include "simpleRoundRobin.h"
#include <stdlib.h>
#include "sleep.h"
#include "mutex.h"
#include "utils/memcluster.h"

static OS_mutex_t testMutex;
static OS_memcluster memcluster;

void task1(void const *const args) {
	while (1) {
		int somenumber = rand() % 128;
		uint32_t * someArr = memcluster.allocate(somenumber);
		for(int i=0;i<somenumber;i++){
			someArr[i] = rand();
			OS_mutex_acquire(&testMutex);
			//printf("1\t\t\t\t\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
			OS_mutex_release(&testMutex);
			OS_sleep((rand()% 100) + 1);
		}
		memcluster.deallocate(someArr);
	}
}
void task2(void const *const args) {
	while (1) {
		int somenumber = rand() % 128;
		uint32_t * someArr = memcluster.allocate(somenumber);
		for(int i=0;i<somenumber;i++){
			someArr[i] = rand();
			OS_mutex_acquire(&testMutex);
			//printf("\t\t2\t\t\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
			OS_mutex_release(&testMutex);
			OS_sleep((rand()% 100) + 1);
		}
		memcluster.deallocate(someArr);
	}
}
void task3(void const *const args) {
	while (1) {
		int somenumber = rand() % 128;
		uint32_t * someArr = memcluster.allocate(somenumber);
		for(int i=0;i<somenumber;i++){
			someArr[i] = rand();
			OS_mutex_acquire(&testMutex);
			//printf("\t\t\t\t3\t\t\t\tLock held by: %p\r\n",testMutex.tcbPointer);
			OS_mutex_release(&testMutex);
			OS_sleep((rand()% 100) + 1);
		}
		memcluster.deallocate(someArr);
	}
}

void task4(void const *const args) {
	while (1) {
		int somenumber = rand() % 128;
		uint32_t * someArr = memcluster.allocate(somenumber);
		for(int i=0;i<somenumber;i++){
			someArr[i] = rand();
			OS_mutex_acquire(&testMutex);
			//printf("\t\t\t\t\t\t4\t\tLock held by: %p\r\n",testMutex.tcbPointer);
			OS_mutex_release(&testMutex);
			OS_sleep((rand()% 100) + 1);
		}
		memcluster.deallocate(someArr);
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
	static uint32_t mempool[8192]; // 32 kb
	memory_cluster_init(&memcluster,mempool,8192);
	OS_init_mutex(&testMutex);

	/* Initialise and start the OS */
	OS_init(&simpleRoundRobinScheduler);
	
	/*NO CODE ABOVE THIS POINT*/
	
	static uint32_t * stack1;
	static uint32_t * stack2;
	static uint32_t * stack3;
	static uint32_t * stack4;
	static OS_TCB_t TCB1, TCB2, TCB3, TCB4;
	stack1 = memcluster.allocate(64);
	stack2 = memcluster.allocate(64);
	stack3 = memcluster.allocate(64);
	stack4 = memcluster.allocate(64);
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(&TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(&TCB2, stack2+64, task2, 0);
	OS_initialiseTCB(&TCB3, stack3+64, task3, 0);
	OS_initialiseTCB(&TCB4, stack4+64, task4, 0);
	
	OS_addTask(&TCB1);
	OS_addTask(&TCB2);
	OS_addTask(&TCB3);
	OS_addTask(&TCB4);
	OS_start();
}
