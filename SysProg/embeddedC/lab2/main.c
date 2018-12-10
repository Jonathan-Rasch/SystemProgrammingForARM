#include <stdio.h>
#include <stdint.h>
#include "serial.h"

void copyString(char *source, char *dest) {
	// Loop until a termination character is found
	while (*source != 0) {
		// Copy a character
		*dest = *source;
		// Increment the source and destination pointers
		source++;
		dest++;
	}
	// Finally add the termination character to the destination string
	*dest = 0;
}

int main(void) {
	serial_init();
	
	char buffer[16];
	char message[] = "Hello, world!";
	
	copyString(message, buffer);
	
	// Print the contents of the 'buffer' array as a string
	printf("%s\r\n", buffer);
}
