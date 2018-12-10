#include "count.h"
#include <stdint.h>
#include <stdio.h>

void count(void) {
	printf("count(): Counter increased from %d", counter);
	counter++;
	printf(" to %d\r\n", counter);
}
