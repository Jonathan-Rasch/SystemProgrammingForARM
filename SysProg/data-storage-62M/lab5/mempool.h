#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

typedef struct {
	void *head;
} pool_t;

void pool_init(pool_t *pool);
void *pool_allocate(pool_t *pool);
void pool_deallocate(pool_t *pool, void *item);

#define pool_add pool_deallocate

#endif /* MEMPOOL_H */
