/*
by Luigi Auriemma
*/

#include <stdio.h>
#include <stdlib.h>

// int is 64bit in Oodle64
int   __stdcall (*OodleLZ_Compress)(int algo, void *in, int insz, void *out, int max, void *a, void *b, void *c, void *d, int e) = NULL;
int   __stdcall (*OodleLZ_Decompress)(void *in, int insz, void *out, int outsz, int a, int b, int c, void *d, void *e, void *f, void *g, void *h, void *i, int j) = NULL;   // Oodle 2.3.0

int OodleLZ_Compress_argc   = -1;
int OodleLZ_Decompress_argc = -1;

void  __stdcall (*Oodle_GetConfigValues)(void *info) = NULL;
void  __stdcall (*Oodle_SetConfigValues)(void *info) = NULL;
char* __stdcall (*OodleLZ_Compressor_GetName)(int a) = NULL;

int   __stdcall (*OodleNetwork1UDP_Decode)(void *state, void *shared, void *in, int insz, void *out, int outsz) = NULL;
int   __stdcall (*OodleNetwork1UDP_Encode)(void *state, void *shared, void *in, int insz, void *out) = NULL;
int   __stdcall (*OodleNetwork1UDP_State_Size)() = NULL;
void  __stdcall (*OodleNetwork1UDP_State_Uncompact)(void *out, void *in) = NULL;
void  __stdcall (*OodleNetwork1_Shared_SetWindow)(void *out, int out_bits, void *in, int insz) = NULL;
int   __stdcall (*OodleNetwork1_Shared_Size)(int a) = NULL;

//void* __stdcall (*OodlePlugins_SetAssertion)(void *func) = NULL;
//int __stdcall oodle_noassert(const char * a,const int b,const char * c,const char * d) { return 0; }



// Compress uses the enum value while Decompress uses the raw value stored in the header of the compressed file
// the algorithm number is NOT the same of the enum, that's why we need this
typedef struct {
    char    *name;
    int     algo_compress;
    int     algo_raw;       // 0x8c followed by this byte
} Oodle_algorithms_raw_t;

#define Oodle_algorithms_raw_max    100
Oodle_algorithms_raw_t    Oodle_algorithms_raw[Oodle_algorithms_raw_max] = { // pre-built and taken from getname
    { "LZH",            0,  7 },
    { "LZHLW",          1,  0 },
    { "LZNIB",          2,  1 },
    { "None",           3,  7 },    // 0x8c->0xcc
    { "LZB16",          4,  2 },
    { "LZBLW",          5,  3 },
    { "LZA",            6,  4 },
    { "LZNA",           7,  5 },
    { "Kraken",         8,  6 },
    { "Mermaid",        9, 10 },
    { "BitKnit",       10, 11 },
    { "Selkie",        11, 10 },
    { "Hydra",         12,  6 },
    { "Leviathan",     13, 12 },
    { NULL,            -1, -1 }
};



int myOodle_Init2(void) {
    int     info[32] = {0};
    if(Oodle_GetConfigValues && Oodle_SetConfigValues) {
        Oodle_GetConfigValues(&info);
        info[5] = 5;    // maintain compatibility with oodle 2.5.x used in many games
        Oodle_SetConfigValues(&info);
    }

    if(OodleLZ_Compressor_GetName) {
        int     i, x;
        char    *name;
        for(i = 0; i < Oodle_algorithms_raw_max; i++) {
            name = OodleLZ_Compressor_GetName(i);
            if(!name) break;    // not used
            if(!stricmp(name, "invalid")) break;
            for(x = 0; Oodle_algorithms_raw[x].name; x++) {
                if(!stricmp(Oodle_algorithms_raw[x].name, name)) break;
            }
            Oodle_algorithms_raw[x].algo_compress = i;
            if(!Oodle_algorithms_raw[x].name) {
                Oodle_algorithms_raw[x].name = name;
                Oodle_algorithms_raw[x].algo_raw = x;   // ???
                x++;
                Oodle_algorithms_raw[x].name = NULL;
                Oodle_algorithms_raw[x].algo_compress = -1;
                Oodle_algorithms_raw[x].algo_raw = -1;
            }
        }
    }
    return 0;
}



#ifdef WIN32

#include "oo2core_dll.h"
#include "oo2net_dll.h"



void *myOodle_GetProcAddress_scanner(HMODULE hlib, char *func_name, int argc_min, int *ret_argc) {
    char    buff[strlen(func_name) + 32];
    int     argc    = -1;
    void    *ret    = NULL;

    if(ret_argc) *ret_argc = -1;
    if(func_name) {
        if(argc_min < 0) argc_min = 0;
        for(argc = argc_min;; argc++) {
            sprintf(buff, "_%s@%d", func_name, (int)(argc * sizeof(void *)));
            ret = (void *)MemoryGetProcAddress(hlib, buff);
            if(ret) break;
            sprintf(buff,  "%s@%d", func_name, (int)(argc * sizeof(void *)));   // without "_", impossible but let's try anyway
            ret = (void *)MemoryGetProcAddress(hlib, buff);
            if(ret) break;
        }
        if(ret_argc) *ret_argc = argc;
    }
    return ret;
}



int myOodle_Init(void) {
    static HMODULE hlib     = NULL;
    static HMODULE hlib_net = NULL;
    if(!hlib) {

        #define Oodle_GetProcAddress(X,Y,Z) \
            if(!X) X = (void *)MemoryGetProcAddress(hlib##Z, #X); \
            if(!X) X = (void *)MemoryGetProcAddress(hlib##Z, "_"#X"@"#Y);

        hlib = (void *)MemoryLoadLibrary((void *)oo2core_dll, sizeof(oo2core_dll));
        if(hlib) {
            // it's boring that the prototypes of OodleLZ_* change so often,
            // let my calling_convention.h do the job for me :)

            Oodle_GetProcAddress(OodleLZ_Compress, 40,)
            if(!OodleLZ_Compress)   OodleLZ_Compress    = myOodle_GetProcAddress_scanner(hlib, "OodleLZ_Compress",    5, &OodleLZ_Compress_argc);
            Oodle_GetProcAddress(OodleLZ_Decompress, 56,)
            if(!OodleLZ_Decompress) OodleLZ_Decompress  = myOodle_GetProcAddress_scanner(hlib, "OodleLZ_Decompress",  4, &OodleLZ_Decompress_argc);

            Oodle_GetProcAddress(Oodle_GetConfigValues, 4,)
            Oodle_GetProcAddress(Oodle_SetConfigValues, 4,)
            Oodle_GetProcAddress(OodleLZ_Compressor_GetName, 4,)

            Oodle_GetProcAddress(OodleNetwork1UDP_Decode, 24,)
            Oodle_GetProcAddress(OodleNetwork1UDP_Encode, 20,)
            Oodle_GetProcAddress(OodleNetwork1UDP_State_Size, 0,)
            Oodle_GetProcAddress(OodleNetwork1UDP_State_Uncompact, 8,)
            Oodle_GetProcAddress(OodleNetwork1_Shared_SetWindow, 16,)
            Oodle_GetProcAddress(OodleNetwork1_Shared_Size, 4,)
        }
        if(!hlib) {
            fprintf(stderr, "\nError: unable to load the Oodle DLL\n");
            myexit(QUICKBMS_ERROR_DLL);
        }
        if(!OodleLZ_Compress || !OodleLZ_Decompress) {
            fprintf(stderr, "\nError: unable to find the OodleLZ_* functions\n");
            myexit(QUICKBMS_ERROR_DLL);
        }

        hlib_net = (void *)MemoryLoadLibrary((void *)oo2net_dll, sizeof(oo2net_dll));
        if(hlib_net) {
            Oodle_GetProcAddress(OodleNetwork1UDP_Decode, 24, _net)
            Oodle_GetProcAddress(OodleNetwork1UDP_Encode, 20, _net)
            Oodle_GetProcAddress(OodleNetwork1UDP_State_Size, 0, _net)
            Oodle_GetProcAddress(OodleNetwork1UDP_State_Uncompact, 8, _net)
            Oodle_GetProcAddress(OodleNetwork1_Shared_SetWindow, 16, _net)
            Oodle_GetProcAddress(OodleNetwork1_Shared_Size, 4, _net)
        }

        // better to leave the asserts enabled for debug information
        //if(!OodlePlugins_SetAssertion) OodlePlugins_SetAssertion = (void *)MemoryGetProcAddress(hlib, "OodlePlugins_SetAssertion");
        //if(!OodlePlugins_SetAssertion) OodlePlugins_SetAssertion = (void *)MemoryGetProcAddress(hlib, "_OodlePlugins_SetAssertion@4");
        //if(OodlePlugins_SetAssertion) OodlePlugins_SetAssertion(oodle_noassert);

        myOodle_Init2();
    }
    return 0;
}



#else

    #if defined(i386) || defined(IA64)

        // requires -msse2 to build

        extern int Kraken_Decompress(const byte *src, size_t src_len, byte *dst, size_t dst_len);
        int   __stdcall _OodleLZ_Decompress(void *in, int insz, void *out, int outsz, int a, int b, int c, void *d, void *e, void *f, void *g, void *h, void *i, int j) {
            return Kraken_Decompress(in, insz, out, outsz);
        }

        int myOodle_Init(void) {
            static int  init = 0;
            if(!init) {
                init = 1;
                OodleLZ_Decompress = _OodleLZ_Decompress;
                myOodle_Init2();
            }
            return 0;
        }

    #else

        int myOodle_Init(void) {
            return -1;
        }

    #endif

#endif



int oodle_get_algo(char *name, int raw) {
    int     i;
    if(name) {
        if(!stricmp(name, "LZQ1")) name = "Kraken";
        if(!stricmp(name, "LZNIB2")) name = "Mermaid";
        if(!stricmp(name, "Akkorokamui")) name = "Hydra";

        for(i = 0; Oodle_algorithms_raw[i].name; i++) {
            if(!stricmp(name, Oodle_algorithms_raw[i].name)) {
                if(raw) return Oodle_algorithms_raw[i].algo_raw;
                else    return Oodle_algorithms_raw[i].algo_compress;
            }
        }
    }
    return -1;
}



int myOodleLZ_Compress(unsigned char *in, int insz, unsigned char *out) {
    myOodle_Init();
    if(!OodleLZ_Compress) return -1;
    int     algo,
            ret = -1;
    // algo is not supported, intentionally! Only public information are used here
    algo = oodle_get_algo("BitKnit", 0);
    if(algo < 0) algo = 0;  // something valid
    if(OodleLZ_Compress_argc < 0) {
        ret = OodleLZ_Compress(
            algo, in, insz, out, 8,
            NULL, NULL, NULL, NULL, 0);
    } else {
        ret = stdcall_call(OodleLZ_Compress, OodleLZ_Compress_argc,
            algo, in, insz, out, 8,
            0,0,0,0,0
            #ifdef CALLCONV_INIT
                     ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  // put lot of fake arguments, who knows the future...
            #endif
            );
    }
    return ret;
}



int myOodleLZ_Decompress(unsigned char *in, int insz, unsigned char *out, int outsz, char *algo_name) {
    myOodle_Init();
    if(!OodleLZ_Decompress) return -1;
    unsigned char   *p = NULL;
    int     algo = oodle_get_algo(algo_name, 1);
    if(algo >= 0) { // custom algorithm for headerless data
        p = malloc(2 + insz);
        memcpy(p + 2, in, insz);
        p[0] = 0x8c;
        p[1] = algo;
        insz += 2;
    }
    if(OodleLZ_Decompress_argc < 0) {
        outsz = OodleLZ_Decompress(
            in, insz, out, outsz,
            0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 3 /*3 and 0 are the same*/);
    } else {
        outsz = stdcall_call(OodleLZ_Decompress, OodleLZ_Decompress_argc,
            in, insz, out, outsz,
            0,0,0,0,0,0,0,0,0,0
            #ifdef CALLCONV_INIT
                               ,0,0,0,0,0,0,0,0,0,0 // put lot of fake arguments, who knows the future...
            #endif
            );
    }
    FREE(p);    // automatically check "if(p)"
    if(!outsz) return -1;   // Oodle returns 0 on error
    return outsz;
}



int myOodleNetwork1UDP_State_Uncompact(unsigned char *in, int insz, unsigned char **ret_out, int *ret_outsz) {
    myOodle_Init();
    if(!OodleNetwork1UDP_State_Uncompact) return -1;
    if(!ret_out || !ret_outsz) return -1;
    int     outsz = *ret_outsz;
    if(OodleNetwork1UDP_State_Size) outsz = OodleNetwork1UDP_State_Size();
    myalloc(ret_out, outsz, ret_outsz);
    OodleNetwork1UDP_State_Uncompact(*ret_out, in);
    return outsz;
}



int myOodleNetwork1_Shared_SetWindow(unsigned char *in, int insz, unsigned char **ret_out, int *ret_outsz, int bits) {
    myOodle_Init();
    if(!OodleNetwork1_Shared_SetWindow) return -1;
    if(!ret_out || !ret_outsz) return -1;
    if(bits <= 0) bits = 20;    // usually 19 is used
    int     outsz = 0x18 + ((1 << bits) * 8);
    if(OodleNetwork1_Shared_Size) outsz = OodleNetwork1_Shared_Size(bits);
    myalloc(ret_out, outsz, ret_outsz);
    OodleNetwork1_Shared_SetWindow(*ret_out, bits, in, insz);
    return outsz;
}



int myOodleNetwork1UDP_Decode(void *state, void *shared, void *in, int insz, void *out, int outsz) {
    myOodle_Init();
    if(!OodleNetwork1UDP_Decode) return -1;
    if(!state) return -1;
    if(!shared) return -1;
    if(!OodleNetwork1UDP_Decode(state, shared, in, insz, out, outsz)) return -1;
    return outsz;
}



int myOodleNetwork1UDP_Encode(void *state, void *shared, void *in, int insz, void *out, int outsz) {
    myOodle_Init();
    if(!OodleNetwork1UDP_Encode) return -1;
    if(!state) return -1;
    if(!shared) return -1;
    outsz = OodleNetwork1UDP_Encode(state, shared, in, insz, out);
    if(!outsz) return -1;   // Oodle returns 0 on error
    return outsz;
}


