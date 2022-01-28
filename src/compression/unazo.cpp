/*
Note:
It's necessary to compile quickbms with -D_GLIBCXX_IOSTREAM because
the "#include <iostream>" in x86Filter.cpp avoids to use quickbms
on Windows 98 due the _fstat64 linkage.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/azo/AZOWrapper.h"
#include "../libs/azo/Common/Allocator.h"



namespace AZO {

AZO_ALLOC_FUNCTON       Allocator::allocFunc_(NULL);
AZO_FREE_FUNCTON        Allocator::freeFunc_(NULL);    
void*                   Allocator::opaque_(NULL);

void* Allocator::Alloc(u_int n) {
    return calloc(1,n);
}

void Allocator::Free(void* p) {
    if(p) free(p);
}

} //namespaces AZO



extern "C"
int unazo(char *in, unsigned int insz, char *out, unsigned int outsz) {
    const char* next_in = in;
    unsigned int avail_in = insz;
    char* next_out = out;
    unsigned int avail_out = outsz;

    AZODecoder* decoder_ = new AZODecoder();
    int ret = decoder_->Decompress(next_in, avail_in, next_out, avail_out);
	delete decoder_;
    return ret;
}
