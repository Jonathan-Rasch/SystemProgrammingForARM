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
#include "queue.h"
#include "channel.h"

#define MEMPOOL_SIZE 16384 //
__align(8)
static uint32_t memory[MEMPOOL_SIZE]; 

static OS_mutex_t printLock;
static uint32_t taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8 = 0;

void task1(void const *const args) {
	OS_channel_t * channel = OS_channel_connect(1234,16);//todo no return from svc delegate. place in tcb->data ?
	while(1){
		taskcounter1++;
		channel_write(channel,taskcounter1);
	}
}

void task2(void const *const args) {
	OS_channel_t * channel = OS_channel_connect(1234,16);
	while(1){
		OS_mutex_acquire(&printLock);
		uint32_t value = channel_read(channel);
		printf("READ: %d",value);
		OS_mutex_release(&printLock);
	}
}

void task3(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter3++;
		printf("\t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		//OS_sleep(rand()%100);
	}
}

void task4(void const *const args) {
	while (1) {
		OS_mutex_acquire(&printLock);
		taskcounter4++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		//OS_sleep(rand()%100);
	}
}

void task5(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter5++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		////////////////////////////
		//OS_sleep(rand()%100);
	}
}

void task6(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter6++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		//OS_sleep(rand()%100);
	}
}

void task7(void const *const args) {
	while(1){
		OS_mutex_acquire(&printLock);
		taskcounter7++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		//OS_sleep(rand()%100);
	}
}

void task8(void const *const args) {
	while (1) {
		OS_mutex_acquire(&printLock);
		taskcounter8++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t %04d \t\t\u001b[31m[%04d]\u001b[0m\r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4,taskcounter5,taskcounter6,taskcounter7,taskcounter8);
		OS_mutex_release(&printLock);
		//OS_sleep(rand()%100);
	}
}

/* MAIN FUNCTION */
int main(void) {
	//SCnSCB->ACTLR = SCnSCB_ACTLR_DISDEFWBUF_Msk; //DEBUG, converting IMPRECISERR into PRECISERR
	/* Initialise the serial port so printf() works */
	serial_init();
	printf("\r\n***********");
	printf("\r\n* DocetOS *");
	printf("\r\n***********\r\n");

	/* Initialise the OS */
	
	OS_init(&stochasticScheduler,memory,MEMPOOL_SIZE);
	
	OS_TCB_t * TCB1 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB2 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB3 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB4 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB5 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB6 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB7 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	OS_TCB_t * TCB8 = (OS_TCB_t*)OS_alloc(sizeof(OS_TCB_t));
	uint32_t * stack1 = OS_alloc(64);
	uint32_t * stack2 = OS_alloc(64);
	uint32_t * stack3 = OS_alloc(64);
	uint32_t * stack4 = OS_alloc(64);
	uint32_t * stack5 = OS_alloc(64);
	uint32_t * stack6 = OS_alloc(64);
	uint32_t * stack7 = OS_alloc(64);
	uint32_t * stack8 = OS_alloc(64);
	OS_init_mutex(&printLock);
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(TCB2, stack2+64, task2, 0);
	OS_initialiseTCB(TCB3, stack3+64, task3, 0);
	OS_initialiseTCB(TCB4, stack4+64, task4, 0);
	OS_initialiseTCB(TCB5, stack5+64, task5, 0);
	OS_initialiseTCB(TCB6, stack6+64, task6, 0);
	OS_initialiseTCB(TCB7, stack7+64, task7, 0);
	OS_initialiseTCB(TCB8, stack8+64, task8, 0);
	
	/*TESTING QUEUE*/
	OS_queue_t * queue = new_queue(8);
	ASSERT(queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(queue_write(queue,42));
	ASSERT(!queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(queue_write(queue,43));
	ASSERT(queue_write(queue,44));
	ASSERT(queue_write(queue,45));
	ASSERT(queue_write(queue,46));
	ASSERT(queue_write(queue,47));
	ASSERT(queue_write(queue,48));
	ASSERT(!queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(queue_write(queue,49));
	ASSERT(!queue_isEmpty(queue));
	ASSERT(queue_isFull(queue));
	ASSERT(!queue_write(queue,50));
	
	
	static uint32_t _return;
	_return = 40;
	ASSERT(queue_peekAt(queue,0,&_return) && _return == 42);
	ASSERT(queue_peekAt(queue,1,&_return) && _return == 43);
	ASSERT(queue_peekAt(queue,2,&_return) && _return == 44);
	ASSERT(queue_peekAt(queue,3,&_return) && _return == 45);
	ASSERT(queue_peekAt(queue,4,&_return) && _return == 46);
	ASSERT(queue_peekAt(queue,5,&_return) && _return == 47);
	ASSERT(queue_peekAt(queue,6,&_return) && _return == 48);
	ASSERT(queue_peekAt(queue,7,&_return) && _return == 49);
	ASSERT(!queue_peekAt(queue,8,&_return) && _return == 49);

	ASSERT(queue_read(queue,&_return) && _return == 42);
	ASSERT(!queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(queue_peekAt(queue,0,&_return) && _return == 43);
	ASSERT(queue_read(queue,&_return) && _return == 43);
	ASSERT(queue_peekAt(queue,1,&_return) && _return == 45);
	ASSERT(queue_read(queue,&_return) && _return == 44);
	ASSERT(queue_read(queue,&_return) && _return == 45);
	ASSERT(queue_read(queue,&_return) && _return == 46);
	ASSERT(queue_read(queue,&_return) && _return == 47);
	ASSERT(queue_read(queue,&_return) && _return == 48);
	ASSERT(!queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(queue_read(queue,&_return) && _return == 49);
	ASSERT(queue_isEmpty(queue));
	ASSERT(!queue_isFull(queue));
	ASSERT(!queue_read(queue,&_return) && _return == 49);
	
	OS_addTask(TCB1,1);
	OS_addTask(TCB2,2);
	OS_addTask(TCB3,3);
	OS_addTask(TCB4,4);
	OS_addTask(TCB5,5);
	OS_addTask(TCB6,6);
	OS_addTask(TCB7,7);
	OS_addTask(TCB8,8);
	
	
	OS_start();
}
