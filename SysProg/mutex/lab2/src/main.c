#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "utils.h"

xTaskHandle hTask1;
xTaskHandle hTask2;

portTASK_FUNCTION_PROTO( vTask1, pvParameters );
portTASK_FUNCTION_PROTO( vTask2, pvParameters );

static uint32_t volatile count;

// ============================================================================
int main( void ) {

	// Initialise all of the STM32F4DISCOVERY hardware (including the serial port)
	HwInit();

	// Welcome message
	printf( "\r\nFreeRTOS Test\r\n" );

	// Tasks get started here.
	// Arguments to xTaskCreate are:
	// 1- The function to execute as a task (make sure it never exits!)
	// 2- The task name (keep it short)
	// 3- The stack size for the new task (configMINIMAL_STACK_SIZE == 130)
	// 4- Parameter for the task (we won't be using this, so set it to NULL)
	// 5- The task priority; higher numbers are higher priority, and the idle task has priority tskIDLE_PRIORITY
	// 6- A pointer to an xTaskHandle variable where the TCB will be stored
	xTaskCreate( vTask1, "TASK1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &hTask1 );
	xTaskCreate( vTask2, "TASK2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &hTask2 );

	vTaskStartScheduler(); // This should never return.

	// Will only get here if there was insufficient memory to create
	// the idle task.
	while(1);  
}

// ---------------------------------------------------------------------------- 
portTASK_FUNCTION( vTask1, pvParameters ) {
	while (1) {
		count ++;
	}
}

// ---------------------------------------------------------------------------- 
portTASK_FUNCTION( vTask2, pvParameters ) {
	portTickType xLastWakeTime = xTaskGetTickCount();
	while (1) {
		printf("Counter value: %d\r\n", count);
		count = 0;
		vTaskDelayUntil( &xLastWakeTime, ( 500 / portTICK_RATE_MS ) );
	}
}
