#ifndef AZO_ALLOCATOR_CPP
#define AZO_ALLOCATOR_CPP

#include <new>
#include "Allocator.h"

namespace AZO {

AZO_ALLOC_FUNCTON       Allocator::allocFunc_(NULL);
AZO_FREE_FUNCTON        Allocator::freeFunc_(NULL);    
void*                   Allocator::opaque_(NULL);

void* Allocator::Alloc(u_int n)
{
    void* p(0);
    if(allocFunc_) {
        p = allocFunc_(opaque_, n);
    } else {
        p = new (std::nothrow) char[static_cast<size_t>(n)];
    }

    //TRACE("alloc: %d (%p)", n, p);

    return p;
}

void Allocator::Free(void* p)
{
    if(p)
    {
        if(freeFunc_) {
            if(p)
                freeFunc_(opaque_, p);
        } else {
            delete [] static_cast<char*>(p);
        }

        //TRACE("free: %p", p);
    }
}

} //namespaces AZO

#endif /*AZO_ALLOCATOR_CPP*/
