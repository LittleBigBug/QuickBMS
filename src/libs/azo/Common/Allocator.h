#ifndef AZO_ALLOCATOR_H
#define AZO_ALLOCATOR_H

#include "../Common/AZOPrivate.h"

namespace AZO {

class Allocator
{
public:
    static void Set(AZO_ALLOC_FUNCTON allocFunc, AZO_FREE_FUNCTON freeFunc, void* opaque)
    {
        allocFunc_ = allocFunc;
        freeFunc_ = freeFunc;
        opaque_ = opaque;
    }

    static void* Alloc(u_int n);
    static void Free(void *);
  
private:
    static AZO_ALLOC_FUNCTON   allocFunc_;
    static AZO_FREE_FUNCTON    freeFunc_;    
    static void*               opaque_;
};

} //namespaces AZO

#endif /*AZO_ALLOCATOR_H*/
