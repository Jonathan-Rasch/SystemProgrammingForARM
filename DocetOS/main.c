#include "os.h"
#include "utils/heap.h"
#include <stdio.h>
#include "utils/serial.h"
#include "patientPreemptivePriorityScheduler.h"
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
static uint32_t taskcounter1,taskcounter2,taskcounter3,taskcounter4 = 0;
void task1(void const *const args) {
	while(1){
		int random =  rand() % 256 + 1;
		uint32_t * mem = memcluster.allocate(random);
		OS_sleep(rand()%100);
		////////////////////////////////
		OS_mutex_acquire(&printLock);
		taskcounter1++;
		printf("\t\t[%04d]\t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			*(mem+i) = random;
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		taskcounter1++;
		printf("\t\t[%04d]\t\t %04d \t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK1 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
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
		taskcounter2++;
		printf("\t\t %04d \t\t[%04d]\t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			mem[i] = random;
		}
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK2 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		//DEBUG_printARR(mem,random);
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		taskcounter2++;
		printf("\t\t %04d \t\t[%04d]\t\t %04d \t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK2 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
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
		taskcounter3++;
		printf("\t\t %04d \t\t %04d \t\t[%04d]\t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			mem[i] = random;
		}
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK3 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		taskcounter3++;
		printf("\t\t %04d \t\t %04d \t\t[%04d]\t\t %04d \r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK3 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
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
		taskcounter4++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t[%04d]\r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			mem[i] = random;
		}
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK4 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		OS_mutex_acquire(&printLock);
		taskcounter4++;
		printf("\t\t %04d \t\t %04d \t\t %04d \t\t[%04d]\r\n",taskcounter1,taskcounter2,taskcounter3,taskcounter4);
		for(uint32_t i=0;i<random;i++){
			if(*(mem+i) == random){
				continue;
			}else{
				printf("\tMEMORY ADDR %p NOT INTACT IN TASK4 %#10x\r\n",(mem+i),*(mem+i));
			}
		}
		OS_mutex_release(&printLock);
		OS_sleep(rand()%100);
		memcluster.deallocate(mem);
		OS_sleep(rand()%100);
	}
}

/* MAIN FUNCTION */

int main(void) {
	SCnSCB->ACTLR = SCnSCB_ACTLR_DISDEFWBUF_Msk; //DEBUG, converting IMPRECISERR into PRECISERR
	/* Initialise the serial port so printf() works */
	serial_init();
	printf("\r\n\t\t\t\t\t\t***************************");
	printf("\r\n\t\t\t\t\t\t* DocetOS Sleep and Mutex *\r\n");
	printf("\t\t\t\t\t\t***************************\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[64], stack2[64], stack3[64], stack4[64];
	static minHeapNode heapNodeArray[4];
	static OS_TCB_t TCB1, TCB2, TCB3, TCB4;
	
	memory_cluster_init(&memcluster,mempool,MEMCLUSTER_SIZE);
	OS_init_mutex(&testMutex);
	OS_init_mutex(&printLock);
	
	/* Initialise the TCBs using the two functions above */
	OS_initialiseTCB(&TCB1, stack1+64, task1, 0);
	OS_initialiseTCB(&TCB2, stack2+64, task2, 0);
	OS_initialiseTCB(&TCB3, stack3+64, task3, 0);
	OS_initialiseTCB(&TCB4, stack4+64, task4, 0);

	/* Initialise and start the OS */
	OS_init(&patientPreemptivePriorityScheduler);
	initialize_scheduler(&memcluster,8);
	
	/*NO CODE ABOVE THIS POINT*/
	
	/*memcluster test*/
	OS_hashtable * hasht = new_hashtable(&memcluster,8,4);
	uint32_t someval[9] = {5,7,9,11,13,15,17,19,19};
	for(int i=0;i<8;i++){
		hashtable_put(hasht,i*5,&someval[i],HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	}
	for(int i=0;i<8;i++){
		printf("value %d\r\n",*hashtable_get(hasht,i*5));
	}
	for(int i=0;i<8;i++){
		printf("value %d\r\n",*hashtable_get(hasht,i*5));
	}
	for(int i=0;i<8;i++){
		printf("value %d\r\n",*hashtable_remove(hasht,i*5));
	}
	for(int i=0;i<8;i++){
		printf("value %d\r\n",*hashtable_get(hasht,i*5));
	}
	for(int i=0;i<9;i++){
		hashtable_put(hasht,i*5,&someval[i],HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
	}
	for(int i=0;i<8;i++){
		printf("value %d\r\n",*hashtable_get(hasht,i*5));
	}
	OS_addTask(&TCB1,11);
	OS_addTask(&TCB2,5);
	OS_addTask(&TCB3,7);
	OS_addTask(&TCB4,2);
	
	
	OS_start();
}
