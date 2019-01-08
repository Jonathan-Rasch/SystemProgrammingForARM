#ifndef DOCETOS_HASHTABLE_H
#define DOCETOS_HASHTABLE_H

#include <stdint.h>
#include "..\structs.h"


//prototypes
OS_hashtable_t * new_hashtable(uint32_t _capacity,uint32_t _number_of_buckets);
uint32_t djb2_hash(uint32_t thing_to_hash);
uint32_t hashtable_put(OS_hashtable_t * _hashtable, uint32_t _key,uint32_t * _value,uint32_t  _checkForDuplicates);
uint32_t * hashtable_get(OS_hashtable_t *, uint32_t key);
uint32_t * hashtable_remove(OS_hashtable_t * _hashtable, uint32_t _key);
const hashtable_value * hashtable_getFirstElementOfNthBucket(OS_hashtable_t * _hashtable, uint32_t _n);
void DEBUG_printHashtable(OS_hashtable_t * _hashtable);

#define HASHTABLE_DISABLE_DUPLICATE_CHECKS 0
#define HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY 1 //<-- this is normal hashtable operation 1 key holds 1 value
#define HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY 2

#endif

