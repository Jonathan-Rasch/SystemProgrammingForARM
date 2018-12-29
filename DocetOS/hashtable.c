#include "hashtable.h"
#include <stdint.h>
#include "utils/memcluster.h"
/*possible feature list
-> dont have a fixed size, rehash once X% is filled
-> protect hshtable via mutex*/

//=============================================================================
// things for the header
//=============================================================================


//=============================================================================
// Internal Function Prototypes and Variable Declarations
//=============================================================================




//structs
typedef struct{
	uint32_t * next_hastable_value;
	uint32_t * underlying_data;
}hastable_value;

typedef struct{
	uint32_t number_of_buckets;
	uint32_t maximum_capacity;
	uint32_t remaining_capacity;
}OS_hashtable;

//prototypes
uint32_t djb2_hash(uint32_t thing_to_hash);
uint32_t hashtable_put(OS_hashtable *,uint32_t key,uint32_t * value);
uint32_t * hashtable_get(OS_hashtable *, uint32_t key);

//=============================================================================
// INIT
//=============================================================================

OS_hashtable * init_hashtable(OS_memcluster * memcluster,uint32_t _capacity,uint32_t _number_of_buckets){
	uint32_t required_size_4ByteWords = sizeof(OS_hashtable)/4;
	required_size_4ByteWords += _capacity * (sizeof(hastable_value)/4);
	required_size_4ByteWords += _number_of_buckets;
	uint32_t * memory = memcluster->allocate(required_size_4ByteWords);
	if(memory == NULL){
		printf("Cannot create hashtable, could not obtain valid memory!\r\n");
		return NULL;
	}
	/* Structure of hashtable in memory:
		0) number_of_buckets		-
		1) maximum_capacity			|
		2) remaining_capacity		- OS_hashtable struct
		3) bucket 0							-	
	...												| bucket array (with N elements)
	N+2) bucket N							-
	N+3) next_hastable_value	-
	N+4) underlying_data			| M* hashtable_value structs
	N+5) next_hastable_value	| 
	...	 											|
N+2+2*M) underlying_data		-*/
	
	/*placing hashtable struct into allocated memory*/
	OS_hashtable * hashtable = (OS_hashtable *)memory;
	hashtable->number_of_buckets = _number_of_buckets;
	hashtable->maximum_capacity = _capacity;
	hashtable->remaining_capacity = _capacity;
	memory = memory+sizeof(OS_hashtable)/4;
	
	/*clearing the remaining memory (dont want future buckets or hashtable value structs to contain invalid data).
	my mempool already clears the memory, but i think it is better not to rely on this (other mempools might not do this)*/
	uint32_t words_to_be_cleared = _number_of_buckets + _capacity*2;
	for(int i=0;i<words_to_be_cleared;i++){
		memory[i] = NULL;
	}
	
	//TODO: need to add unused hastable value structs to unused list !
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
static uint32_t hashtable_put(OS_hashtable * _hashtable, uint32_t key,uint32_t * value){
	if(_hashtable->remaining_capacity == 0){
		return 0; //hashtable is full
	}
	/*obtain hashtable_value struct if one is available*/
	hastable_value * hashtable_val = (hastable_value *)(_hashtable + sizeof(OS_hashtable)/4 + _hashtable->number_of_buckets);
	hashtable_val += _hashtable->maximum_capacity - _hashtable->remaining_capacity; // now points to unused struct
	hashtable_val->underlying_data = value;
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(key) % _hashtable->number_of_buckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable)/4;
	/*insert hashtable value into bucket linked list*/
	hastable_value * tmp_hastable_value = (hastable_value *)bucket_array[bucket_number];
	bucket_array[bucket_number] = (uint32_t )hashtable_val;
	hashtable_val->next_hastable_value = (uint32_t *)tmp_hastable_value;
	/*update capacity*/
	_hashtable->remaining_capacity -= 1;
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