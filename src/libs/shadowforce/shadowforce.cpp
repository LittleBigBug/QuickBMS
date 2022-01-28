#include <stdio.h>
#include <stdlib.h>

#include "Compress.h"
#include "BHUT.h"
#include "ELSCoder.h"
#include "LZ77.h"
#include "RefPack.h"

#define shadowforce_compressor(X) \
    switch(algo) { \
        case CT_HughesTransform:    { HughesTransform ht;   ht.X(cin, &cout);   break; } \
        case CT_ELSCoder:           { ELSCoder        ec;   ec.X(cin, &cout);   break; } \
        case CT_LZ77:               { LZ77            lz;   lz.X(cin, &cout);   break; } \
        case CT_RefPack:            { RefPack         rp;   rp.X(cin, &cout);   break; } \
        default: break; \
    }



extern "C"
int shadowforce_decompress(int algo, unsigned char *in, int insz, unsigned char *out, int outsz) {
    DecompressorInput   cin;
    CompressorInput     cout;
    int                 ret;
    
    cin.buffer          = in;
    cin.lengthInBytes   = insz;
    shadowforce_compressor(decompress)
    ret = cout.lengthInBytes;
    if((ret > 0) && (ret <= outsz)) {
        memcpy(out, static_cast<u_int8 *>(cout.buffer), ret);
    } else {
        ret = -1;
    }
    delete []cout.buffer;
    return ret;
}



extern "C"
int shadowforce_compress(int algo, unsigned char *in, int insz, unsigned char *out, int outsz) {
    DecompressorInput   cout;
    CompressorInput     cin;
    int                 ret;
    
    cin.buffer          = in;
    cin.lengthInBytes   = insz;
    shadowforce_compressor(compress)
    ret = cout.lengthInBytes;
    if((ret > 0) && (ret <= outsz)) {
        memcpy(out, static_cast<u_int8 *>(cout.buffer), ret);
    } else {
        ret = -1;
    }
    delete []cout.buffer;
    return ret;
}

