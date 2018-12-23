#include "memcluster.h"


/* hash algo by D. J. Bernstein (https://cr.yp.to/djb.html), XOR variant.
I modified it slightly so that it operates on a single uint32_t only (i dont need to hash strings).
This hashing algo has a good distribution and colissions are rare, so perfect for my usecase*/
uint32_t djb2_hash(uint32_t thing_to_hash){
	uint32_t hash = 5381;
	uint32_t c;
	for(int i=0;i<4;i++){
		c = (thing_to_hash >> (i*8)) & 0x000000FF; // essentially pretending that uint32_t is a 4 char string
		hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
	}
	return hash;
}