#include "hashtable.h"
#include <stdint.h>
#include <stdio.h>
#include "utils/memcluster.h"
#include "../utils/debug.h"
/*possible feature list
-> dont have a fixed size, rehash once X% is filled
-> protect hshtable via mutex*/

//=============================================================================
// INIT
//=============================================================================

OS_hashtable * new_hashtable(OS_memcluster * memcluster,uint32_t _capacity,uint32_t _number_of_buckets){
	uint32_t required_size_4ByteWords = sizeof(OS_hashtable)/4;
	required_size_4ByteWords += _capacity * (sizeof(hashtable_value)/4);
	required_size_4ByteWords += _number_of_buckets;
	uint32_t * memory = memcluster->allocate(required_size_4ByteWords);
	if(memory == NULL){
		printf("Cannot create hashtable, could not obtain valid memory!\r\n");
		return NULL;
	}
	/* Structure of hashtable in memory:
		0) number_of_buckets		-
		1) maximum_capacity			|
		2) remaining_capacity		| OS_hashtable struct
		3) hashval linked list	-
		4) bucket 0							-	
	...												| bucket array (with N elements)
	N+3) bucket N							-
	N+4) next_hashtable_value	-
	N+5) underlying_data			| M* hashtable_value structs
	N+6) key									| 
	...	 											|
N+2+3*M) underlying_data		-*/
	
	/*placing hashtable struct into allocated memory*/
	OS_hashtable * hashtable = (OS_hashtable *)memory;
	hashtable->number_of_buckets = _number_of_buckets;
	hashtable->maximum_capacity = _capacity;
	hashtable->remaining_capacity = _capacity;
	hashtable->free_hashtable_value_struct_linked_list = NULL; // will be set up later in this func
	memory = memory+sizeof(OS_hashtable)/4;
	
	/*clearing the remaining memory (dont want future buckets or hashtable value structs to contain invalid data).
	my mempool already clears the memory, but i think it is better not to rely on this (other mempools might not do this)*/
	/*clearing bucket array*/
	uint32_t * bucket_array = memory;
	for (int i = 0; i< hashtable->number_of_buckets;i++){
		bucket_array[i] = NULL;
	}
	memory = memory + hashtable->number_of_buckets;//now points to start of memory section for hashtable_value structs
	
	/*setting up hashtable_value structs and adding them to the free hashtable_value linked list*/
	hashtable_value * hash_val = (hashtable_value *)memory;
	hash_val->key = NULL;
	hash_val->underlying_data = NULL;
	hashtable->free_hashtable_value_struct_linked_list = (uint32_t *)hash_val; // link first element in struct
	for(int i=1;i<_capacity;i++){
		/*get next element and clear it*/
		hashtable_value * next_element = hash_val + 1;
		next_element->key = NULL;
		next_element->underlying_data = NULL;
		/*link, then next element becommes current element, repeat process*/
		hash_val->next_hashtable_value = (uint32_t *)next_element;
		hash_val = next_element;
	}
	
	/*all done*/
	return hashtable;
}

//=============================================================================
// HASHTABLE OPERATIONS
//=============================================================================
/* places "value" into bucket (which is part of "hashtable") corresponding to "key".

_checkForDuplicates can be used to control if the hashtable accepts dublicate keys or values:
0= accept both duplicate keys and values (hasht[11] = 5 ok, hasht[11] ok, can now call remove on key 11 twice and get 5)
1= reject duplicate keys (hasht[11] = 5 ok, hasht[11] = 5 ERROR key already in hash table)
2= reject duplicate values (hasht[11] = 5 ok, hasht[11] = 6 ok, hasht[11]=5 ERROR value 5 is already stored at key 11)

returns: 
1 = success
0 = failure (e.g table is at full capacity)*/
uint32_t hashtable_put(OS_hashtable * _hashtable, uint32_t _key,uint32_t * _value,uint32_t  _checkForDuplicates){
	if(_hashtable->remaining_capacity == 0){
		return 0; //hashtable is full
	}
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(_key) % _hashtable->number_of_buckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable)/4;
	/*ensure that element with provided _key is not already in bucket*/
	if(_checkForDuplicates != HASHTABLE_DISABLE_DUPLICATE_CHECKS){
		hashtable_value * tmp_hashtable_val = (hashtable_value *)bucket_array[bucket_number];
		while(tmp_hashtable_val){
			/*check if key already exists in bucket (might want to allow this, e.g multiple tasks waiting for same mutex (mutex used as key))*/
			if(_checkForDuplicates == HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY && tmp_hashtable_val->key == _key){
				printf("\r\nERROR: key '%d' is already in hashtable!\r\n",_key);
				return 0;
			}
			/*prevent the same data being added twice. this is usefull if multiple tasks should be allowed to wait on the same mutex,
			but the same task should not be allowed to wait on one mutex multiple times.*/
			if(_checkForDuplicates == HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY && tmp_hashtable_val->underlying_data == _value){
				printf("\r\nERROR: value '%p' is already in hashtable at key %d !\r\n",_value,_key);
				return 0;
			}
			tmp_hashtable_val = (hashtable_value *)tmp_hashtable_val->next_hashtable_value;
		}
	}
	/*obtain hashtable_value struct from linked list of free structs*/
	hashtable_value * hashtable_val = (hashtable_value*)_hashtable->free_hashtable_value_struct_linked_list;
	_hashtable->free_hashtable_value_struct_linked_list = hashtable_val->next_hashtable_value;
	hashtable_val->underlying_data = _value;
	hashtable_val->key = _key;
	/*insert hashtable value into bucket linked list*/
	hashtable_value * tmp_hashtable_value = (hashtable_value *)bucket_array[bucket_number];
	bucket_array[bucket_number] = (uint32_t )hashtable_val;
	hashtable_val->next_hashtable_value = (uint32_t *)tmp_hashtable_value;
	/*update capacity*/
	_hashtable->remaining_capacity -= 1;
	#ifdef HASHTABLE_DEBUG
	printf("HASHTABLE: added %p at key: %d\r\n",_value,_key);
	#endif /*HASHTABLE_DEBUG*/
	return 1;
}

/* retrives the value at given key, if no value present in table NULL is returned*/
uint32_t * hashtable_get(OS_hashtable * _hashtable, uint32_t _key){
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(_key) % _hashtable->number_of_buckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable)/4;
	/*search linked list*/
	uint32_t * value = NULL;
	hashtable_value * hash_val = (hashtable_value *)bucket_array[bucket_number];
	while(hash_val){
		if(hash_val->key == _key){
			value = hash_val->underlying_data;
			break;
		}else{
			hash_val = (hashtable_value *)hash_val->next_hashtable_value;
		}
	}
	#ifdef HASHTABLE_DEBUG
	printf("HASHTABLE: removed %p key: %d\r\n",hash_val->underlying_data,_key);
	#endif /*HASHTABLE_DEBUG*/
	return value;
}

/* retrieves the value associated with "key" then REMOVES IT FROM THE HASHTABLE. If no value is associated
with "key" NULL is returned*/
uint32_t * hashtable_remove(OS_hashtable * _hashtable, uint32_t _key){
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(_key) % _hashtable->number_of_buckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable)/4;
	/*search linked list*/
	uint32_t * value = NULL;
	hashtable_value * prev_hash_val = NULL;
	hashtable_value * hash_val = (hashtable_value *)bucket_array[bucket_number];
	while(hash_val){
		if(hash_val->key == _key){
			value = hash_val->underlying_data;
			/*remove from bucket, clear, and return to linked list of unused elements*/
			if(prev_hash_val){
				prev_hash_val->next_hashtable_value = hash_val->next_hashtable_value;
			}else{//no previous hash_val
				bucket_array[bucket_number] = (uint32_t)hash_val->next_hashtable_value;
			}
			hash_val->key = NULL;
			hash_val->next_hashtable_value = NULL;
			hash_val->underlying_data = NULL; //TODO am i deleting the pointer or the value the pointer points to ?
			//return to linked list
			hashtable_value * tmp_hashtable_val = (hashtable_value *)_hashtable->free_hashtable_value_struct_linked_list;
			_hashtable->free_hashtable_value_struct_linked_list = (uint32_t *)hash_val;
			hash_val->next_hashtable_value = (uint32_t *)tmp_hashtable_val;
			//increase remaining capacity since an element just freed up
			_hashtable->remaining_capacity++;
			break;
		}else{
			prev_hash_val = hash_val;
			hash_val = (hashtable_value *)hash_val->next_hashtable_value;
		}
	}
	return value;
}

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

void DEBUG_printHashtable(OS_hashtable * _hashtable){
	printf("\r\n--------------------------------------------------------------------------\r\n");
	for(int i =0; i<_hashtable->number_of_buckets;i++){
		hashtable_value * value = (hashtable_value *)(((uint32_t*)_hashtable)+(sizeof(OS_hashtable)/4))[i];
		printf("BUCKET %d: \r\n",i);
		while(value){
			printf("\t\t--- Block %p ---\r\n",value);
			printf("\t\tkey: %d\r\n",value->key);
			printf("\t\tnext val: %p\r\n",value->next_hashtable_value);
			printf("\t\tdata: %p\r\n",value->underlying_data);
			value = (hashtable_value *)value->next_hashtable_value;
		}
	}
}

/* modified hash algo by D. J. Bernstein (https://cr.yp.to/djb.html), XOR variant.
I modified it slightly so that it operates on a single uint32_t only (i dont need to hash strings).
This hashing algo has a good distribution and colissions are rare, so perfect for my usecase*/
uint32_t djb2_hash(uint32_t thing_to_hash){
	uint32_t hash = 5381;
	uint32_t c=0;
	for(int i=0;i<8;i++){
		c += (thing_to_hash >> (i*4)) ;
		hash = ((hash << 5) + hash) ^ c; /* (hash * 33) + c */
	}
	return hash;
}
