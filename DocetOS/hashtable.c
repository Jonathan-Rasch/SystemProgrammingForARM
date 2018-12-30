#include "hashtable.h"
#include <stdint.h>
#include "utils/memcluster.h"

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
/* places "value" into bucket (which is part of "hashtable") corresponding to "key"
returns: 
1 = success
0 = failure (e.g table is at full capacity)*/
uint32_t hashtable_put(OS_hashtable * _hashtable, uint32_t key,uint32_t * _value){
	if(_hashtable->remaining_capacity == 0){
		return 0; //hashtable is full
	}
	/*obtain hashtable_value struct from linked list of free structs*/
	hashtable_value * hashtable_val = (hashtable_value*)_hashtable->free_hashtable_value_struct_linked_list;
	_hashtable->free_hashtable_value_struct_linked_list = hashtable_val->next_hashtable_value;
	hashtable_val->underlying_data = _value;
	hashtable_val->key = key;
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(key) % _hashtable->number_of_buckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable)/4;
	/*insert hashtable value into bucket linked list*/
	hashtable_value * tmp_hashtable_value = (hashtable_value *)bucket_array[bucket_number];
	bucket_array[bucket_number] = (uint32_t )hashtable_val;
	hashtable_val->next_hashtable_value = (uint32_t *)tmp_hashtable_value;
	/*update capacity*/
	_hashtable->remaining_capacity -= 1;
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

/* hash algo by D. J. Bernstein (https://cr.yp.to/djb.html), XOR variant.
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