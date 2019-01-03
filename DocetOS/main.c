#include "os.h"
#include "utils/heap.h"
#include <stdio.h>
#include "utils/serial.h"
#include "stochasticScheduler.h"
#include <stdlib.h>
#include "sleep.h"
#include "mutex.h"
#include "../hashtable.h"
#include "utils/memcluster.h"
#include "structs.h"

static OS_mutex_t testMutex;
static OS_memcluster memcluster;
#define MEMCLUSTER_SIZE 8192 //8192 = 32 kb
__align(8)
static uint32_t mempool[MEMCLUSTER_SIZE]; 

static OS_mutex_t printLock;
static uint32_t taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8 = 0;

void task1(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter1++;
		printf("\t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		////////////////////////////
		OS_sleep(rand()%100);
	}
}

void task2(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter2++;
		printf("\t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

void task3(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter3++;
		printf("\t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

void task4(void const *const args) {
	while (1) {
		OS_mutex_acquire(&printLock);
		taskcounter4++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

void task5(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter5++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		////////////////////////////
		OS_sleep(rand()%100);
	}
}

void task6(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter6++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

void task7(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter7++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

void task8(void const *const args) {
	while (1) {
		OS_mutex_acquire(&printLock);
		taskcounter8++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
	}
}

/* MAIN FUNCTION */

int main(void) {
	//SCnSCB->ACTLR = SCnSCB_ACTLR_DISDEFWBUF_Msk; //DEBUG, converting IMPRECISERR into PRECISERR
	/* Initialise the serial port so printf() works */
	serial_init();
	printf("\r\n\t\t\t\t\t\t***************************");
	printf("\r\n\t\t\t\t\t\t* DocetOS Sleep and Mutex *\r\n");
	printf("\t\t\t\t\t\t***************************\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[64], stack2[64], stack3[64], stack4[64], stack5[64], stack6[64], stack7[64], stack8[64];
	static minHeapNode heapNodeArray[4];
	static OS_TCB_t TCB1, TCB2, TCB3, TCB4, TCB5, TCB6, TCB7, TCB8;
	
	memory_cluster_init(&memcluster,mempool,MEMCLUSTER_SIZE);
	OS_init_mutex(&testMutex);
	OS_init_mutex(&printLock);
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(&TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(&TCB2, stack2+64, task2, 0);
	OS_initialiseTCB(&TCB3, stack3+64, task3, 0);
	OS_initialiseTCB(&TCB4, stack4+64, task4, 0);
	OS_initialiseTCB(&TCB5, stack5+64, task5, 0);
	OS_initialiseTCB(&TCB6, stack6+64, task6, 0);
	OS_initialiseTCB(&TCB7, stack7+64, task7, 0);
	OS_initialiseTCB(&TCB8, stack8+64, task8, 0);
	
	/* Initialise and start the OS */
	OS_init(&stochasticScheduler);
	initialize_scheduler(&memcluster,8);
	
	/*NO CODE ABOVE THIS POINT*/
	

	OS_addTask(&TCB1,11);
	OS_addTask(&TCB2,5);
	OS_addTask(&TCB3,7);
	OS_addTask(&TCB4,2);
	OS_addTask(&TCB5,21);
	OS_addTask(&TCB6,1);
	OS_addTask(&TCB7,13);
	OS_addTask(&TCB8,3);
	
	
	OS_start();
}
