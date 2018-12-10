#include <stdio.h>
#include "delay.h"
#include "serial.h"
#include "stm32f4xx.h"

int main(void) {
	
	int i;
	serial_init();
	
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= 0x55550000;
	
	printf("LED Test: Watch the LEDs counting.\r\n");
	while(1) {
		for (i = 0; i < 256; i++) {
			delay_ms(250);
			printf("Counter value: %d   \r", i);
			// Set a pattern into bits 8-15 of GPIO D
			GPIOD->BSRR = 0xFF000000UL | (i << 8);
		}
	}
	
}
