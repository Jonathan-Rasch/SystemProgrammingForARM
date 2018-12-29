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
#define MEMCLUSTER_SIZE 8192 //8192 = 32 kb
static uint32_t mempool[MEMCLUSTER_SIZE]; 

static OS_mutex_t printLock;

static void DEBUG_printARR(uint32_t * arr,uint32_t size){
	printf("\t\t\t Array %p : [",arr);
	for(int i=0;i<size-1;i++){
		printf("\r\n\t\t\t\t%#08x, @ %p",arr[i], arr+i);
	}
	printf("\r\n\t\t\t\t%#08x] @ %p\r\n",arr[size-1],arr+(size-1));
	ASSERT(0);
}

void task1(void const *const args) {
	while(1){
		
		int random =  rand() % 256 + 1;
		uint32_t * mem = memcluster.allocate(random);
		OS_sleep(rand()%100);
		////////////////////////////////
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 1:\r\n",OS_elapsedTicks());
		for(uint32_t i=0;i<random;i++){
			*(mem+i) = 0x1111;
			if(*(mem+i) == (0x1111)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		printf("\tstored %d words at %p\r\n",random,mem);
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x1111)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
				DEBUG_printARR(mem,random);
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 1:\r\n",OS_elapsedTicks());
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x1111)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		
		printf("\tdeallocateing.. %p\r\n",mem);
		OS_mutex_release(&printLock);
		////////////////////////////
		OS_sleep(rand()%100);
		memcluster.deallocate(mem);
		OS_sleep(rand()%100);
	}
}

void task2(void const *const args) {
	while(1){
		int random = rand() % 256 + 1;
		uint32_t * mem = memcluster.allocate(random);
		//////////////////////////////
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 2:\r\n",OS_elapsedTicks());
		for(uint32_t i=0;i<random;i++){
			mem[i] = 0x22222222;
		}
		printf("\tstored %d words at %p -> %p\r\n",random,mem,mem+random);
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK2 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 2:\r\n",OS_elapsedTicks());
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK2 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		
		printf("\tdeallocating... %p\r\n",mem);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		memcluster.deallocate(mem);
		OS_sleep(rand()%100);
	}
}

void task3(void const *const args) {
	while(1){
		int random = rand() % 256 + 1;
		uint32_t * mem = memcluster.allocate(random);
		//////////////////////////////
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 3:\r\n",OS_elapsedTicks());
		for(uint32_t i=0;i<random;i++){
			mem[i] = 0x22222222;
		}
		printf("\tstored %d words at %p -> %p\r\n",random,mem,mem+random);
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK3 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 3:\r\n",OS_elapsedTicks());
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK3 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		
		printf("\tdeallocating... %p\r\n",mem);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		memcluster.deallocate(mem);
		OS_sleep(rand()%100);
	}
}

void task4(void const *const args) {
	while (1) {
		int random = rand() % 256 + 1;
		uint32_t * mem = memcluster.allocate(random);
		//////////////////////////////
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 4:\r\n",OS_elapsedTicks());
		for(uint32_t i=0;i<random;i++){
			mem[i] = 0x22222222;
		}
		printf("\tstored %d words at %p -> %p\r\n",random,mem,mem+random);
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK4 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		printf("[%d]TASK 4:\r\n",OS_elapsedTicks());
		printf("\tchecking memory %p -> %p\r\n",mem,mem+random);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == (0x22222222)){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK4 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		
		printf("\tdeallocating... %p\r\n",mem);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		memcluster.deallocate(mem);
		OS_sleep(rand()%100);
	}
}

/* MAIN FUNCTION */

int main(void) {
	/* Initialise the serial port so printf() works */
	serial_init();
	srand(10);
	printf("\r\n\t\t\t\t\t\t***************************");
	printf("\r\n\t\t\t\t\t\t* DocetOS Sleep and Mutex *\r\n");
	printf("\t\t\t\t\t\t***************************\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[64], stack2[64], stack3[64], stack4[64];
	static OS_TCB_t TCB1, TCB2, TCB3, TCB4;
	
	memory_cluster_init(&memcluster,mempool,MEMCLUSTER_SIZE);
	OS_init_mutex(&testMutex);
	OS_init_mutex(&printLock);
	/* Initialise and start the OS */
	OS_init(&simpleRoundRobinScheduler);
	
	/*NO CODE ABOVE THIS POINT*/
	
	
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
