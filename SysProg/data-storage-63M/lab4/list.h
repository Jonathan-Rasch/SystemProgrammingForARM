#ifndef LIST_H
#define LIST_H

typedef struct element {
	int contents;
	struct element *prev;
	struct element *next;
} element_t;

typedef struct {
	element_t *first;
	element_t *last;
} list_t;

element_t *list_add(list_t *list, int contents, element_t *marker);

void list_delete(list_t *list, element_t *element);

#endif /* LIST_H */
