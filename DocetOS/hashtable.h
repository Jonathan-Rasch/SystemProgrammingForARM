#ifndef DOCETOS_HASHTABLE_H
#define DOCETOS_HASHTABLE_H

#include <stdint.h>
#include "..\structs.h"


//prototypes
OS_hashtable * new_hashtable(OS_memcluster * memcluster,uint32_t _capacity,uint32_t _number_of_buckets);
uint32_t djb2_hash(uint32_t thing_to_hash);
uint32_t hashtable_put(OS_hashtable *,uint32_t key,uint32_t * value);
uint32_t * hashtable_get(OS_hashtable *, uint32_t key);
uint32_t * hashtable_remove(OS_hashtable * _hashtable, uint32_t _key);
#endif

