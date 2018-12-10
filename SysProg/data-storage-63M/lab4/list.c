#include "list.h"
#include <stdlib.h>

element_t *list_add(list_t *list, int contents, element_t *marker) {
	/* Allocate the element and set its contents and prev pointer */
	element_t *element = malloc(sizeof(element_t));
	element->contents = contents;
	element->prev = marker;
	
	if (marker == 0) {
		/* Adding to the start of the list */
		/* Set the element's next pointer */
		element->next = list->first;
		/* Update the first element pointer in the list */
		list->first = element;
	} else {
		/* Adding after an existing element */
		/* Set the element's next pointer */
		element->next = marker->next;
		/* Set the next pointer of the marker */
		marker->next = element;
	}

	/* Set the prev pointer of the element after the marker, if there is one */
	if (element->next != 0) {
		element->next->prev = element;
	}

	/* Were we adding after the last element? */
	/* If so, update the last element pointer in the list */
	/* This also catches the case when the list was is empty */
	if (list->last == marker) {
		list->last = element;
	}
	
	return element;
}

void list_delete(list_t *list, element_t *element) {
	/* Write this! */
}
