#ifndef DOCETOS_HASHTABLE_H
#define DOCETOS_HASHTABLE_H

#include <stdint.h>

//structs
typedef struct{
	uint32_t * next_hashtable_value;
	uint32_t * underlying_data;
	uint32_t key;
}hashtable_value;

typedef struct{
	uint32_t number_of_buckets;
	uint32_t maximum_capacity;
	uint32_t remaining_capacity;
	uint32_t * free_hashtable_value_struct_linked_list;
}OS_hashtable;

//prototypes
uint32_t djb2_hash(uint32_t thing_to_hash);
uint32_t hashtable_put(OS_hashtable *,uint32_t key,uint32_t * value);
uint32_t * hashtable_get(OS_hashtable *, uint32_t key);
uint32_t * hashtable_remove(OS_hashtable * _hashtable, uint32_t _key);
#endif