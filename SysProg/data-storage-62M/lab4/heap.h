#ifndef HEAP_H
#define HEAP_H

typedef struct {
	int *store;
	unsigned int length;
} heap_t;

void heap_init(heap_t *heap, int *store);
void heap_insert(heap_t *heap, int value);
int heap_extract(heap_t *heap);
int heap_isEmpty(heap_t *heap);

#endif /* HEAP_H */
