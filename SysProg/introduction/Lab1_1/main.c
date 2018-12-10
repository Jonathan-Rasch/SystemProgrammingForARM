#include <stdio.h>
#include "delay.h"
#include "serial.h"
#include "lcd.h"
#include "stm32f4xx.h"

int main(void) {
	
	serial_init();
	lcd_init();
	
	printf("System Test\r\n");
	printf("0123456789ABCDEF\r\n");
	
	lcd_move(3,0);
	lcd_print("System Test");
	lcd_move(0,1);
	lcd_print("0123456789ABCDEF");
	
	/* Infinite loop: embedded software must never end */
	while(1);
	
}
