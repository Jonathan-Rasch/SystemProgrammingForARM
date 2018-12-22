#include "hashtable.h"
#include <stdint.h>

//=============================================================================
// Internal Function Prototypes and Variable Declarations
//=============================================================================

//prototypes
static uint32_t __hash(const void *);

//variables and structs
static uint32_t numBuckets;