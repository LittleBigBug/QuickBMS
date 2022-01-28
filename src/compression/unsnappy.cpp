#include "../libs/snappy/snappy.h"

extern "C" int unsnappy(void *in, int insz, void *out, int outsz) {
    size_t  size;

    if(!snappy::GetUncompressedLength((const char *)in, insz, &size)) return(-1);
    if(size > outsz) return(-1);
    if(!snappy::RawUncompress((const char *)in, insz, (char *)out)) return(-1);
    return(size);
}

