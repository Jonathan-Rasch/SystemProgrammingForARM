#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "serial.h"

void secret(void) {
	printf("Access granted\r\n");
}

int main(void) {
	serial_init();

	char const *pwd_store = "password";
	char attempt[10];
	
	// Try to overwrite the link register here so that secret() is run
	// even if the password is incorrect
	strcpy(attempt, "attempt");
	if (0 == strcmp(attempt, pwd_store)) {
		secret();
	}
}
