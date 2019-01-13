#ifndef DOCETOS_DEBUG_H
#define DOCETOS_DEBUG_H

#define ASSERT(x) do{if(!(x))__breakpoint(0);}while(0)

/*UNCOMMENT TO ENABLE DEBUG MODE IN CORRESPONDING FILES*/
//#define HEAP_DEBUG
//#define stochasticScheduler_DEBUG
//#define MEMCLUSTER_DEBUG
//#define HASHTABLE_DEBUG

#endif //DOCETOS_DEBUG_H