#include <stdio.h>
#include "serial.h"
#include "list.h"

int main(void) {
	serial_init();
	
	list_t list = {0};
	
	/* Add some elements to the start of the list */
	list_add(&list, 5, 0);
	list_add(&list, 4, 0);
	list_add(&list, 3, 0);
	list_add(&list, 2, 0);
	list_add(&list, 1, 0);
	
	/* Iterate through the list and print the values */
	element_t *element = list.first;
	while (element != 0) {
		printf("Element value: %d\r\n", element->contents);
		element = element->next;
	}
	printf("\r\n");
	
	/* Try iterating backwards through the list */
	element = list.last;
	while (element != 0) {
		printf("Element value: %d\r\n", element->contents);
		element = element->prev;
	}
	printf("\r\n");
	
	/* Delete the second element */
	list_delete(&list, list.first->next);
	/* Iterate through the list and print the remaining values */
	element = list.first;
	while (element != 0) {
		printf("Element value: %d\r\n", element->contents);
		element = element->next;
	}
	printf("\r\n");
	
	/* Delete the first and last elements */
	list_delete(&list, list.first);	
	list_delete(&list, list.last);
	/* Try iterating backwards through the list again */
	element = list.last;
	while (element != 0) {
		printf("Element value: %d\r\n", element->contents);
		element = element->prev;
	}
}
