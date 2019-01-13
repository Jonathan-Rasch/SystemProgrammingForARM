#include "hashtable.h"
#include <stdint.h>
#include <stdio.h>
#include "memcluster.h"
#include "os.h"
/*possible feature list
-> dont have a fixed size, rehash once X% is filled
-> protect hshtable via mutex*/

//=============================================================================
// INIT
//=============================================================================

OS_hashtable_t * new_hashtable(uint32_t _capacity,uint32_t _numberOfBuckets){
	uint32_t required_size_4ByteWords = sizeof(OS_hashtable_t)/4;
	required_size_4ByteWords += _capacity * (sizeof(OS_hashtable_value_t)/4);
	required_size_4ByteWords += _numberOfBuckets;
	uint32_t * memory = OS_alloc(required_size_4ByteWords);
	if(memory == NULL){
		printf("Cannot create hashtable, could not obtain valid memory!\r\n");
		return NULL;
	}
	/* Structure of hashtable in memory:
		0) numberOfBuckets		-
		1) maximumCapacity			|
		2) remainingCapacity		| OS_hashtable_t struct
		3) hashval linked list	-
		4) bucket 0							-	
	...												| bucket array (with N elements)
	N+3) bucket N							-
	N+4) nextHashtableValue	-
	N+5) underlyingData			| M* hashtable_value structs
	N+6) key									| 
	...	 											|
N+2+3*M) underlyingData		-*/
	
	/*placing hashtable struct into allocated memory*/
	OS_hashtable_t * hashtable = (OS_hashtable_t *)memory;
	hashtable->numberOfBuckets = _numberOfBuckets;
	hashtable->maximumCapacity = _capacity;
	hashtable->remainingCapacity = _capacity;
	hashtable->freeHashtableValueStructLinkedList = NULL; // will be set up later in this func
	memory = memory+sizeof(OS_hashtable_t)/4;
	
	/*clearing the remaining memory (dont want future buckets or hashtable value structs to contain invalid data).
	my mempool already clears the memory, but i think it is better not to rely on this (other mempools might not do this)*/
	/*clearing bucket array*/
	uint32_t * bucket_array = memory;
	for (int i = 0; i< hashtable->numberOfBuckets;i++){
		bucket_array[i] = NULL;
	}
	memory = memory + hashtable->numberOfBuckets;//now points to start of memory section for hashtable_value structs
	
	/*setting up hashtable_value structs and adding them to the free hashtable_value linked list*/
	OS_hashtable_value_t * hash_val = (OS_hashtable_value_t *)memory;
	hash_val->key = NULL;
	hash_val->underlyingData = NULL;
	hashtable->freeHashtableValueStructLinkedList = (uint32_t *)hash_val; // link first element in struct
	for(int i=1;i<_capacity;i++){
		/*get next element and clear it*/
		OS_hashtable_value_t * next_element = hash_val + 1;
		next_element->key = NULL;
		next_element->underlyingData = NULL;
		/*link, then next element becommes current element, repeat process*/
		hash_val->nextHashtableValue = (uint32_t *)next_element;
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
uint32_t hashtable_put(OS_hashtable_t * _hashtable, uint32_t _key,uint32_t * _value,uint32_t  _checkForDuplicates){
	if(_hashtable->remainingCapacity == 0){
		return 0; //hashtable is full
	}
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(_key) % _hashtable->numberOfBuckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable_t)/4;
	/*ensure that element with provided _key is not already in bucket*/
	if(_checkForDuplicates != HASHTABLE_DISABLE_DUPLICATE_CHECKS){
		OS_hashtable_value_t * tmp_hashtable_val = (OS_hashtable_value_t *)bucket_array[bucket_number];
		while(tmp_hashtable_val){
			/*check if key already exists in bucket (might want to allow this, e.g multiple tasks waiting for same mutex (mutex used as key))*/
			if(_checkForDuplicates == HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY && tmp_hashtable_val->key == _key){
				return 0;
			}
			/*prevent the same data being added twice. this is usefull if multiple tasks should be allowed to wait on the same mutex,
			but the same task should not be allowed to wait on one mutex multiple times.*/
			if(_checkForDuplicates == HASHTABLE_REJECT_MULTIPLE_IDENTICAL_VALUES_PER_KEY && tmp_hashtable_val->underlyingData == _value){
				return 0;
			}
			tmp_hashtable_val = (OS_hashtable_value_t *)tmp_hashtable_val->nextHashtableValue;
		}
	}
	/*obtain hashtable_value struct from linked list of free structs*/
	OS_hashtable_value_t * hashtable_val = (OS_hashtable_value_t*)_hashtable->freeHashtableValueStructLinkedList;
	_hashtable->freeHashtableValueStructLinkedList = hashtable_val->nextHashtableValue;
	hashtable_val->underlyingData = _value;
	hashtable_val->key = _key;
	/*insert hashtable value into bucket linked list*/
	OS_hashtable_value_t * tmp_hashtable_value = (OS_hashtable_value_t *)bucket_array[bucket_number];
	bucket_array[bucket_number] = (uint32_t )hashtable_val;
	hashtable_val->nextHashtableValue = (uint32_t *)tmp_hashtable_value;
	/*update capacity*/
	_hashtable->remainingCapacity -= 1;
	return 1;
}

/* retrives the value at given key, if no value present in table NULL is returned and validValueFlag is set to 0*/
uint32_t * hashtable_get(OS_hashtable_t * _hashtable, uint32_t _key){
    _hashtable->validValueFlag = 0;
    /*determine bucket*/
    uint32_t bucket_number = djb2_hash(_key) % _hashtable->numberOfBuckets;
    uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable_t)/4;
    /*search linked list*/
    uint32_t * value = NULL;
    OS_hashtable_value_t * hash_val = (OS_hashtable_value_t *)bucket_array[bucket_number];
    while(hash_val){
        if(hash_val->key == _key){
            value = hash_val->underlyingData;
            _hashtable->validValueFlag = 1;
            return value;
        }else{
            hash_val = (OS_hashtable_value_t *)hash_val->nextHashtableValue;
        }
    }
    return NULL;
}

/* This function allows the retrieval of multiple values stored under the same key by setting the value _n.
 * when _n is 0 this function returns the first instance with _key, when _n is 1 it returns the second etc.
 * when no _nth instance with _key exists NULL is returned AND validValueFlag REMAINS 0*/
uint32_t * hashtable_getNthValueAtKey(OS_hashtable_t * _hashtable, uint32_t _key, uint32_t _n){
    _hashtable->validValueFlag = 0;
    /*determine bucket*/
    uint32_t bucket_number = djb2_hash(_key) % _hashtable->numberOfBuckets;
    uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable_t)/4;
    /*search linked list*/
    uint32_t * value = NULL;
    OS_hashtable_value_t * hash_val = (OS_hashtable_value_t *)bucket_array[bucket_number];
    while(hash_val){
        if(hash_val->key == _key){
            if(_n != 0){
                _n--;
								hash_val = (OS_hashtable_value_t *)hash_val->nextHashtableValue;
                continue;
            }
            value = hash_val->underlyingData;
            _hashtable->validValueFlag = 1;
            return value;
        }else{
            hash_val = (OS_hashtable_value_t *)hash_val->nextHashtableValue;
        }
    }
    return NULL;
}

/* retrieves the value associated with "key" then REMOVES IT FROM THE HASHTABLE. If no value is associated
with "key" NULL is returned*/
uint32_t * hashtable_remove(OS_hashtable_t * _hashtable, uint32_t _key){
	_hashtable->validValueFlag = 0;
	/*determine bucket*/
	uint32_t bucket_number = djb2_hash(_key) % _hashtable->numberOfBuckets;
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable_t)/4;
	/*search linked list*/
	uint32_t * value = NULL;
	OS_hashtable_value_t * prev_hash_val = NULL;
	OS_hashtable_value_t * hash_val = (OS_hashtable_value_t *)bucket_array[bucket_number];
	while(hash_val){
		if(hash_val->key == _key){
			_hashtable->validValueFlag = 1;
			value = hash_val->underlyingData;
			/*remove from bucket, clear, and return to linked list of unused elements*/
			if(prev_hash_val){
				prev_hash_val->nextHashtableValue = hash_val->nextHashtableValue;
			}else{//no previous hash_val
				bucket_array[bucket_number] = (uint32_t)hash_val->nextHashtableValue;
			}
			hash_val->key = NULL;
			hash_val->nextHashtableValue = NULL;
			hash_val->underlyingData = NULL;
			//return to linked list
			OS_hashtable_value_t * tmp_hashtable_val = (OS_hashtable_value_t *)_hashtable->freeHashtableValueStructLinkedList;
			_hashtable->freeHashtableValueStructLinkedList = (uint32_t *)hash_val;
			hash_val->nextHashtableValue = (uint32_t *)tmp_hashtable_val;
			//increase remaining capacity since an element just freed up
			_hashtable->remainingCapacity++;
			break;
		}else{
			prev_hash_val = hash_val;
			hash_val = (OS_hashtable_value_t *)hash_val->nextHashtableValue;
		}
	}
	return value;
}

/*Returns the content of the first element in the Nth bucket, might be NULL if bucket is empty or
	if n is larger than the largest bucket index of the hashtable. this function helps 
	to access all elements stored in the hashtable*/
const OS_hashtable_value_t * hashtable_getFirstElementOfNthBucket(OS_hashtable_t * _hashtable, uint32_t _n){
	_hashtable->validValueFlag = 0;
	if(_n > _hashtable->numberOfBuckets-1){
		return NULL;
	}
	uint32_t * bucket_array = ((uint32_t *)_hashtable) + sizeof(OS_hashtable_t)/4;
	OS_hashtable_value_t * val = (OS_hashtable_value_t *)bucket_array[_n];
	if(val){
		_hashtable->validValueFlag = 1;
		return val;
	}else{
		return NULL;
	}
}

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

void DEBUG_printHashtable(OS_hashtable_t * _hashtable){
	printf("\r\n--------------------------------------------------------------------------\r\n");
	for(int i =0; i<_hashtable->numberOfBuckets;i++){
		OS_hashtable_value_t * value = (OS_hashtable_value_t *)(((uint32_t*)_hashtable)+(sizeof(OS_hashtable_t)/4))[i];
		printf("BUCKET %d: \r\n",i);
		while(value){
			printf("\t\t--- Block %p ---\r\n",value);
			printf("\t\tkey: %d\r\n",value->key);
			printf("\t\tnext val: %p\r\n",value->nextHashtableValue);
			printf("\t\tdata: %p\r\n",value->underlyingData);
			value = (OS_hashtable_value_t *)value->nextHashtableValue;
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
