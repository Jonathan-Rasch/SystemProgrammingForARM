#include "mutex.h"
#include <stdio.h>
#include "os_internal.h"
#include "os.h"

//================================================================================
// init, new and destroy
//================================================================================

void OS_init_mutex(OS_mutex_t * mutex){
	mutex->counter = 0;
	mutex->tcbPointer = NULL; // NULL is just 0 ofc, but i think this makes it clearer
	mutex->resourcePriorityValue = 0; //invalid priority indicating no task waiting on resource
};

/*Allocates and initialises mutex.
It is the users responsibility not to deallocate a mutex that is in use.*/
OS_mutex_t * new_mutex(){
	OS_mutex_t * mutex = OS_alloc(sizeof(OS_mutex_t)/4);
	OS_init_mutex(mutex);
	return mutex;
}

/*releases all resources associated with this mutex*/
uint32_t destroy_mutex(OS_mutex_t * _mutex){
	OS_free((uint32_t*)_mutex);
	return 1;
}

//================================================================================
// Exported Functions
//================================================================================


void OS_mutex_acquire(OS_mutex_t * _mutex){
	uint32_t mutexTcbPtr; 
	uint32_t lockNotObtained;
	OS_TCB_t * currentTCB = OS_currentTCB();
	while(1){
		uint32_t checkCode = OS_checkCode();
		mutexTcbPtr = (uint32_t)__LDREXW((uint32_t*) &(_mutex->tcbPointer));
		if(mutexTcbPtr == NULL || mutexTcbPtr == (uint32_t)OS_currentTCB()){
			//no tcb has locked the mutex, try and aquire lock
			//OS_TCB_t * currentTCB = OS_currentTCB();
			lockNotObtained = __STREXW((uint32_t)currentTCB,(uint32_t*) &(_mutex->tcbPointer)); // returns 0 on success !
			if(lockNotObtained){
				continue;// try again, go back to loading
			}else{
				OS_notify_resource_aquired((uint32_t *)_mutex,1);
				break; //got the lock
			}
		}else{
			/*Update the resourcePriorityValue of the mutex. This value reflects the priority of the highest
			priority task waiting on this resource. Any task that holds this mutex can assume the priority
			resourcePriorityValue until releasing this mutex.*/
			uint32_t updatingMutexPriorityValue = 1;
			while(updatingMutexPriorityValue){
				uint32_t value = (uint32_t)__LDREXW((uint32_t*) &(_mutex->resourcePriorityValue));
				if(value != 0 && value <= currentTCB->priority){
					updatingMutexPriorityValue = 0; // a task with higher or equal priority is already waiting on this resource
					continue;
				}else{
					value = currentTCB->priority;
					updatingMutexPriorityValue = __STREXW((uint32_t)value,(uint32_t*) &(_mutex->resourcePriorityValue));
				}
			}
			//somebody else holds the lock, wait for its release
			OS_wait(_mutex,checkCode);
			continue;//repeat the process of trying to obtain lock
		}
	}
	//lock has been obtained, increase lock counter
	_mutex->counter++;
}

//uint32_t OS_mutex_acquire_non_blocking(OS_mutex_t * _mutex){
//	uint32_t mutexTcbPtr; 
//	uint32_t lockNotObtained;
//	uint32_t checkCode = OS_checkCode();
//	//attempt to get lock
//	mutexTcbPtr = (uint32_t)__LDREXW((uint32_t*) &(_mutex->tcbPointer));
//	if(mutexTcbPtr == NULL){
//		//no tcb has locked the mutex, try and aquire lock
//		OS_TCB_t * currentTCB = OS_currentTCB();
//		lockNotObtained = __STREXW((uint32_t)currentTCB,(uint32_t*) &(_mutex->tcbPointer)); // returns 0 on success !
//		if(lockNotObtained){
//			return 0;//could not get lock
//		}else{
//			_mutex->counter++;
//			return 1;//got lock !
//		}
//	}
//	return 0;// other TCB owns lock, continue with execution (since this is nonblocking)
//}

void OS_mutex_release(OS_mutex_t * _mutex){
	OS_TCB_t * currentTCB = OS_currentTCB();
	if(_mutex->tcbPointer != currentTCB){
		printf("ERROR: task that did not own lock tried to release it!");
		ASSERT(0);
		return;
	}
	_mutex->counter--;
	if(_mutex->counter == 0){
		//releasing lock fully
		_mutex->tcbPointer = NULL;
		if(currentTCB == NULL){
			return;//not a task, dont try and sleep !!
		}
		OS_notify(_mutex);
		OS_yield();//give other tasks a chance to get lock
	}
}


