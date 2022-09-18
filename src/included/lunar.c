/*
by Luigi Auriemma
*/



enum {
    LC_LZ1 = 0,
    LC_LZ2,
    LC_LZ3,
    LC_LZ4,
    LC_LZ5,
    LC_LZ6,
    LC_LZ7,
    LC_LZ8,
    LC_LZ9,
    LC_LZ10,
    LC_LZ11,
    LC_LZ12,
    LC_LZ13,
    LC_LZ14,
    LC_LZ15,
    LC_LZ16,
    LC_LZ17,
    LC_LZ18,
    LC_LZ19,
    //LC_LZ20,  // in DLL 1.90 but there is no need to update the one in quickbms since never used

    LC_RLE1 = 100,
    LC_RLE2,
    LC_RLE3,
    LC_RLE4,
};



#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include "lunar_dll.h"



void *mymemmem(const void *b1, const void *b2, i32 len1, i32 len2);



static unsigned char* (__stdcall *LunarOpenRAMFile)(void* data, u32 FileMode, u32 size) = NULL;
static u32 (__stdcall *LunarDecompress)(void* Destination,u32 AddressToStart,u32 MaxDataSize, u32 Format, u32 Format2, u32* LastROMPosition) = NULL;
static u32 (__stdcall *LunarRecompress)(void* Source, void* Destination,u32 DataSize, u32 MaxDataSize, u32 Format, u32 Format2);
static u32 (__stdcall *LunarCloseFile)() = NULL;



int lunar_init(void) {
    static HMODULE hlib = NULL;
    unsigned char   *p;
    if(!hlib) {
        // "The DLL file is corrupt..."
        p = mymemmem(lunar_dll, "\x8B\x45\x0C\x83\xE8\x01\x72", sizeof(lunar_dll), 7);
        if(p) memcpy(p, "\x33\xc0\x90", 3);

        hlib = (void *)MemoryLoadLibrary((void *)lunar_dll, sizeof(lunar_dll));
        if(!hlib) return -1;
        LunarOpenRAMFile = (void *)MemoryGetProcAddress(hlib, "LunarOpenRAMFile");
        if(!LunarOpenRAMFile) return -2;
        LunarDecompress  = (void *)MemoryGetProcAddress(hlib, "LunarDecompress");
        if(!LunarDecompress) return -3;
        LunarRecompress  = (void *)MemoryGetProcAddress(hlib, "LunarRecompress");
        if(!LunarRecompress) return -4;
        LunarCloseFile   = (void *)MemoryGetProcAddress(hlib, "LunarCloseFile");
        if(!LunarCloseFile) return -5;
    }
    return 0;
}
#endif



int lunar_compress(unsigned char *in, int insz, unsigned char *out, int outsz, int format, int format2) {
#ifdef WIN32
    int     ret;
    ret = lunar_init();
    if(ret < 0) return ret;
    if(!LunarOpenRAMFile(in, 0 /*LC_READONLY*/, insz)) return(-5);
    ret = LunarRecompress(in, out, insz, outsz, format, format2);
    LunarCloseFile();
    return ret;
#else
    return -1;
#endif
}



int lunar_uncompress(unsigned char *in, int insz, unsigned char *out, int outsz, int format, int format2) {
#ifdef WIN32
    int     ret;
    ret = lunar_init();
    if(ret < 0) return ret;
    if(!LunarOpenRAMFile(in, 0 /*LC_READONLY*/, insz)) return(-5);
    ret = LunarDecompress(out, 0, outsz, format, format2, NULL);
    LunarCloseFile();
    return ret;
#else
    return -1;
#endif
}


