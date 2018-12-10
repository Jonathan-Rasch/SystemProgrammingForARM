#include <stdio.h>
#include <stdint.h>
#include "serial.h"
#include "gpio.h"

#define LEDs		(*(uint8_t *)(0x40020C15))
#define buttons	(*(uint8_t *)(0x40021011))

int main(void) {
	serial_init();
	gpio_init();

	printf("Button Test: Push the buttons to see the LEDs change.\r\n");

	while(1) {
		LEDs = buttons;
	}

}
