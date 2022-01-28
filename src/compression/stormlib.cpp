#include <stdio.h>
#include <stdlib.h>
#include "stormlib_huff.h"

extern "C" int StormLib_Compress_huff(void * out, int outsz, void * in, int insz)
{
    THuffmannTree ht(true);
    TOutputStream os(out, outsz);
    return ht.Compress(&os, in, insz, 8);
}

extern "C" int StormLib_Decompress_huff(void * out, int outsz, void * in, int insz)
{
    THuffmannTree ht(false);
    TInputStream is(in, insz);
    outsz = ht.Decompress(out, outsz, &is);
    if(outsz <= 0) return -1;
    return outsz;
}

