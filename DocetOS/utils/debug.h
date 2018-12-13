#ifndef DOCETOS_DEBUG_H
#define DOCETOS_DEBUG_H

#define ASSERT(x) do{if(!(x))__breakpoint(0);}while(0)
#define DEBUG_V
#define DEBUG_VV
#define DEBUG_VVV

#endif //DOCETOS_DEBUG_H
