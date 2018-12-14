#ifndef DOCETOS_DEBUG_H
#define DOCETOS_DEBUG_H

#define ASSERT(x) do{if(!(x))__breakpoint(0);}while(0)
//#define HEAP_DEBUG

#endif //DOCETOS_DEBUG_H
