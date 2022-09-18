// modified by Luigi Auriemma, replaced alignas with pragma pack, since the caller also uses NIBRANS_NO_SSE2
/*
nibrans.h - Simple, single-file, nibble-based, adaptive rANS library with SSE2-accelerated modeling

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring
rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/*
nibrans supports the following three configurations:
#define NIBRANS_EXTERN
    Default, should be used when using nibrans in multiple compilation units within the same project.
#define NIBRANS_IMPLEMENTATION
    Must be defined in exactly one source file within a project for nibrans to be found by the linker.
#define NIBRANS_STATIC
    Defines all nibrans functions as static, useful if nibrans is only used in a single compilation unit.

nibrans supports the following additional options:
#define NIBRANS_NO_SSE2
    Disables all SSE2 optimizations, which will likely make nibrans perform badly and is not recommended.
#define NIBRANS_RATE_BITS
    Number of bits for adaption rate, higher meaning slower. Must be less than 14. Default value is 6.
#define NIBRANS_CTZ(V)
    Overrides the count trailing zeroes function used by nibrans with your own, unnecessary for gcc.

nibrans performance:
    The modeling (which is typically the most expensive aspect of adaptive entropy coding) makes use
    of SSE2 extensively. With SSE2 and "gcc -O2" it can get to around 75-95MB/s en/decoding throughput.
    In some use cases you may want to give nibrans its own thread so it doesn't hinder the main thread.
*/

//header section
#ifndef NIBRANS_H
#define NIBRANS_H

//process configuration
#ifdef NIBRANS_STATIC
    #define NIBRANS_IMPLEMENTATION
    #define NBRADEF static
#else //NIBRANS_EXTERN
    #define NBRADEF extern
#endif

//includes
#include <stdint.h> //integer types
#include <stddef.h> //size_t

//structs
struct nibrans {
    uint16_t cdf1[17]; //cdf for most nibble
    uint16_t cdf2[17]; //cdf for least nibble
};

//function declarations
NBRADEF void nibransInit(struct nibrans*);
    //initializes the given nibrans context, must be done before use with other functions
    //this context contains model data, which changes adaptively as it is used to en/decode
    //calls to en/decode functions that fail (returning 0) will leave the model unmodified
NBRADEF size_t nibransEncode(struct nibrans*, unsigned char*, size_t, const unsigned char*, size_t);
    //uses the given nibrans context (must have been initialized) to encode binary data from
    //given input buffer (last 2 arguments) to given output buffer (2nd and 3rd argument)
    //to encode only a single chunk, simply pass in an input size of at most 4096 bytes.
    //returns number of bytes written to output, or 0 on failure (e.g. buffer too small)
NBRADEF size_t nibransDecode(struct nibrans*, unsigned char*, size_t, const unsigned char*, size_t);
    //uses the given nibrans context (must have been initialized) to decode binary data from
    //given input buffer (last 2 arguments) to given output buffer (2nd and 3rd argument)
    //to decode only a single chunk, simply pass in an output size of at most 4096 bytes.
    //returns number of bytes read from input, or 0 on failure (e.g. buffer too small)

#endif //NIBRANS_H

//implementation section
#ifdef NIBRANS_IMPLEMENTATION
#undef NIBRANS_IMPLEMENTATION

//macros
#ifndef NIBRANS_CTZ
    #ifdef __GNUC__
        #define NIBRANS_CTZ(V) __builtin_ctz(V)
    #else
        #define NIBRANS_CTZ(V) nbraCtz(V)
        static int nbraCtz (int v) {
            //returns number of trailing zeroes
            static const int DeBruijn[32] =
            {0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9};
            return DeBruijn[(uint32_t)((v & -v) * 0x77CB531U) >> 27];
        }
    #endif
#endif
#ifdef __cplusplus
    #define NBRA_ALIGN16(A) A /*alignas(16)*/
#else
    #define NBRA_ALIGN16(A) /*alignas(16)*/ A
#endif
#define NBRA_MIXIN(I, J) (J+1) + ((I < (J+1)) ? NBRA_PROB_SIZE - 16 : 0)
#define NBRA_MIXIN_I(I) \
    {NBRA_MIXIN(I, 0), NBRA_MIXIN(I, 1), NBRA_MIXIN(I, 2), NBRA_MIXIN(I, 3), \
    NBRA_MIXIN(I, 4), NBRA_MIXIN(I, 5), NBRA_MIXIN(I, 6), NBRA_MIXIN(I, 7), \
    NBRA_MIXIN(I, 8), NBRA_MIXIN(I, 9), NBRA_MIXIN(I, 10), NBRA_MIXIN(I, 11), \
    NBRA_MIXIN(I, 12), NBRA_MIXIN(I, 13), NBRA_MIXIN(I, 14), NBRA_MIXIN(I, 15)}
#define NBRA_MIXIN_INIT \
    {NBRA_MIXIN_I(0), NBRA_MIXIN_I(1), NBRA_MIXIN_I(2), NBRA_MIXIN_I(3), \
    NBRA_MIXIN_I(4), NBRA_MIXIN_I(5), NBRA_MIXIN_I(6), NBRA_MIXIN_I(7), \
    NBRA_MIXIN_I(8), NBRA_MIXIN_I(9), NBRA_MIXIN_I(10), NBRA_MIXIN_I(11), \
    NBRA_MIXIN_I(12), NBRA_MIXIN_I(13), NBRA_MIXIN_I(14), NBRA_MIXIN_I(15)}

//constants
#ifndef NIBRANS_RATE_BITS
    #define NIBRANS_RATE_BITS 6 //number of rate bits for adaption shift
#endif
#define NBRA_CODE_BITS 24 //number of bits for coding
#define NBRA_PROB_BITS 14 //number of bits for probability
#define NBRA_CODE_NORM (1 << NBRA_CODE_BITS) //lower bound for normalization
#define NBRA_PROB_SIZE (1 << NBRA_PROB_BITS) //total for probability factors
#define NBRA_CHNK_SIZE 4096 //number of bytes per chunk

//includes
#ifndef NIBRANS_NO_SSE2
    #include <emmintrin.h> //SSE2 intrinsics
#endif
//#include <stdalign.h> //alignas
#include <string.h> //memmove

//structs
struct nbra_range {
    uint16_t start;
    uint16_t width;
};

//function declarations
static size_t nbraEncodeChunk(uint16_t[17], uint16_t[17], unsigned char*, size_t, const unsigned char*, size_t);
static size_t nbraDecodeChunk(uint16_t[17], uint16_t[17], unsigned char*, size_t, const unsigned char*, size_t);
static int nbraEncPut(uint32_t*, unsigned char**, struct nbra_range, const unsigned char*);
static int nbraEncFlush(const uint32_t*, unsigned char**, const unsigned char*);
static int nbraDecInit(uint32_t*, unsigned char**, const unsigned char*);
static int nbraDecPut(uint32_t*, unsigned char**, struct nbra_range, const unsigned char*);
static uint16_t nbraDecGet(const uint32_t*);
static struct nbra_range nbraModRange(const uint16_t[17], unsigned char);
static unsigned char nbraModSymbol(const uint16_t[17], uint16_t);
static void nbraModUpdate(uint16_t[17], unsigned char);

//public functions
NBRADEF void nibransInit (struct nibrans* nbra) {
    int i;
    //this just initializes both the models to a uniform distribution
    for (i = 0; i < 17; i++) nbra->cdf1[i] = i << (NBRA_PROB_BITS - 4);
    for (i = 0; i < 17; i++) nbra->cdf2[i] = i << (NBRA_PROB_BITS - 4);
}
NBRADEF size_t nibransEncode (struct nibrans* nbra, unsigned char* out, size_t out_size, const unsigned char* in, size_t in_size) {
    //prepare variables and aligned cdf copies
    unsigned char* out_cur = out; const unsigned char* in_cur = in;
    size_t out_rem = out_size, in_rem = in_size, ret;
    #pragma pack(2)
    uint16_t NBRA_ALIGN16(cdf1_align[24]);
    uint16_t NBRA_ALIGN16(cdf2_align[24]);
    #pragma pack()
    //7 in so that cdf[1] is 16 aligned
    uint16_t* cdf1 = &cdf1_align[7];
    uint16_t* cdf2 = &cdf2_align[7];
    //copy cdf values
    memcpy(cdf1, nbra->cdf1, sizeof(nbra->cdf1));
    memcpy(cdf2, nbra->cdf2, sizeof(nbra->cdf2));
    //encode in chunks of NBRA_CHNK_SIZE
    while (in_rem > NBRA_CHNK_SIZE) {
        //return 0 if any one chunk returns 0, indicating failure
        if (!(ret = nbraEncodeChunk(cdf1, cdf2, out_cur, out_rem, in_cur, NBRA_CHNK_SIZE))) return 0;
        //advance out cursor
        out_cur = &out_cur[ret]; out_rem -= ret;
        //advance in cursor
        in_cur = &in_cur[NBRA_CHNK_SIZE]; in_rem -= NBRA_CHNK_SIZE;
    }
    //encode last chunk
    if (!(ret = nbraEncodeChunk(cdf1, cdf2, out_cur, out_rem, in_cur, in_rem))) return 0;
    //advance out counter
    out_rem -= ret;
    //copy back cdf values
    memcpy(nbra->cdf1, cdf1, sizeof(nbra->cdf1));
    memcpy(nbra->cdf2, cdf2, sizeof(nbra->cdf2));
    //return
    return out_size - out_rem;
}
NBRADEF size_t nibransDecode (struct nibrans* nbra, unsigned char* out, size_t out_size, const unsigned char* in, size_t in_size) {
    //prepare variables and aligned cdf copies
    unsigned char* out_cur = out; const unsigned char* in_cur = in;
    size_t out_rem = out_size, in_rem = in_size, ret;
    #pragma pack(2)
    uint16_t NBRA_ALIGN16(cdf1_align[24]);
    uint16_t NBRA_ALIGN16(cdf2_align[24]);
    #pragma pack()
    //7 in so that cdf[1] is 16 aligned
    uint16_t* cdf1 = &cdf1_align[7];
    uint16_t* cdf2 = &cdf2_align[7];
    //copy cdf values
    memcpy(cdf1, nbra->cdf1, sizeof(nbra->cdf1));
    memcpy(cdf2, nbra->cdf2, sizeof(nbra->cdf2));
    //decode in chunks of NBRA_CHNK_SIZE
    while (out_rem > NBRA_CHNK_SIZE) {
        //return 0 if any one chunk returns 0, indicating failure
        if (!(ret = nbraDecodeChunk(cdf1, cdf2, out_cur, NBRA_CHNK_SIZE, in_cur, in_rem))) return 0;
        //advance in cursor
        in_cur = &in_cur[ret]; in_rem -= ret;
        //advance out cursor
        out_cur = &out_cur[NBRA_CHNK_SIZE]; out_rem -= NBRA_CHNK_SIZE;
    }
    //decode last chunk
    if (!(ret = nbraDecodeChunk(cdf1, cdf2, out_cur, out_rem, in_cur, in_rem))) return 0;
    //advance in counter
    in_rem -= ret;
    //copy back cdf values
    memcpy(nbra->cdf1, cdf1, sizeof(nbra->cdf1));
    memcpy(nbra->cdf2, cdf2, sizeof(nbra->cdf2));
    //return
    return in_size - in_rem;
}

//internal functions
static size_t nbraEncodeChunk (uint16_t cdf1[17], uint16_t cdf2[17], unsigned char* out, size_t out_size, const unsigned char* in, size_t in_size) {
    //two nibbles in a byte, so we need two ranges per input byte
    struct nbra_range rng_buff[NBRA_CHNK_SIZE*2];
    //model data forwards (in_size must be <= NBRA_CHNK_SIZE)
    size_t i;
    for (i = 0; i < in_size; i++) {
        //grab nibbles for this byte
        unsigned char n1 = in[i] >> 4;
        unsigned char n2 = in[i] & 0x0F;
        //insert ranges of nibbles into buffer
        rng_buff[i*2] = nbraModRange(cdf1, n1);
        rng_buff[i*2+1] = nbraModRange(cdf2, n2);
        nbraModUpdate(cdf1, n1);
        nbraModUpdate(cdf2, n2);
    }
    //pop buffered ranges backwards for encoding
    unsigned char* ptr = &out[out_size];
    uint32_t cod2 = NBRA_CODE_NORM, cod1 = NBRA_CODE_NORM;
    //process buffered ranges in reverse
    for (i = in_size*2; i > 0; i -= 2) {
        //put two symbols (interleaved for ILP)
        if (nbraEncPut(&cod2, &ptr, rng_buff[i-1], out)) return 0;
        if (nbraEncPut(&cod1, &ptr, rng_buff[i-2], out)) return 0;
    }
    //flush encoder state
    if (nbraEncFlush(&cod2, &ptr, out)) return 0;
    if (nbraEncFlush(&cod1, &ptr, out)) return 0;
    //move result to start of output buffer
    size_t size = &out[out_size] - ptr;
    memmove(out, ptr, size);
    //return encoded size
    return size;
}
static size_t nbraDecodeChunk (uint16_t cdf1[17], uint16_t cdf2[17], unsigned char* out, size_t out_size, const unsigned char* in, size_t in_size) {
    //decode data forwards (out_size must be <= NBRA_CHNK_SIZE)
    unsigned char* ptr = (unsigned char*)in;
    uint32_t cod1, cod2;
    if (nbraDecInit(&cod1, &ptr, &in[in_size])) return 0;
    if (nbraDecInit(&cod2, &ptr, &in[in_size])) return 0;
    size_t i;
    for (i = 0; i < out_size; i++) {
        //decode nibbles for this byte
        unsigned char n1 = nbraModSymbol(cdf1, nbraDecGet(&cod1));
        unsigned char n2 = nbraModSymbol(cdf2, nbraDecGet(&cod2));
        struct nbra_range rng1 = nbraModRange(cdf1, n1);
        struct nbra_range rng2 = nbraModRange(cdf2, n2);
        nbraModUpdate(cdf1, n1);
        nbraModUpdate(cdf2, n2);
        //advance decoder state (step + renorm)
        if (nbraDecPut(&cod1, &ptr, rng1, &in[in_size])) return 0;
        if (nbraDecPut(&cod2, &ptr, rng2, &in[in_size])) return 0;
        //write nibbles to output
        out[i] = (n1 << 4) | n2;
    }
    //return 0 if final coder state mismatched
    if ((cod1 != NBRA_CODE_NORM)||(cod2 != NBRA_CODE_NORM)) return 0;
    //return read size
    return ptr - in;
}
static int nbraEncPut (uint32_t* c, unsigned char** pptr, struct nbra_range rng, const unsigned char* lim) {
    //puts a symbol with given range into given coder, writing bytes to pptr, stops at limit
    uint32_t x = *c;
    //renormalize state
    uint32_t x_max = rng.width << (NBRA_CODE_BITS - NBRA_PROB_BITS + 8);
    if (x >= x_max) {
        unsigned char* ptr = *pptr;
        do {
            //return if already at limit
            if (ptr == lim) return 1;
            //advance pointer and write
            *--ptr = x; x >>= 8;
        } while (x >= x_max);
        *pptr = ptr;
    }
    //x = C(s,x)
    *c = x + (NBRA_PROB_SIZE - rng.width) * (x / rng.width) + rng.start;
    //return
    return 0;
}
static int nbraEncFlush (const uint32_t* c, unsigned char** pptr, const unsigned char* lim) {
    //flushes the given coder for encoding, returns 1 if too close to limit
    if (*pptr < &lim[4]) return 1;
    //do flushing
    *--*pptr = *c;
    *--*pptr = *c >> 8;
    *--*pptr = *c >> 16;
    *--*pptr = *c >> 24;
    //return
    return 0;
}
static int nbraDecInit (uint32_t* c, unsigned char** pptr, const unsigned char* lim) {
    //initializes the given coder for decoding, returns 1 if too close to limit
    if (*pptr > &lim[-4]) return 1;
    //read bytes from pptr
    *c  = *(*pptr)++ << 24;
    *c |= *(*pptr)++ << 16;
    *c |= *(*pptr)++ << 8;
    *c |= *(*pptr)++;
    //return
    return 0;
}
static int nbraDecPut (uint32_t* c, unsigned char** pptr, struct nbra_range rng, const unsigned char* lim) {
    //advances and normalizes given coder for decoding, reading bytes from pptr, stops at limit
    uint32_t x = *c;
    // s, x = D(x)
    x = rng.width * (x >> NBRA_PROB_BITS) + (x & (NBRA_PROB_SIZE - 1)) - rng.start;
    //renormalize state
    if (x < NBRA_CODE_NORM) {
        unsigned char* ptr = *pptr;
        do {
            //return if already at limit
            if (ptr == lim) return 1;
            //read and advance pointer
            x = (x << 8) | *ptr++;
        } while (x < NBRA_CODE_NORM);
        *pptr = ptr;
    }
    *c = x;
    //return
    return 0;
}
static uint16_t nbraDecGet (const uint32_t* c) {
    //returns the current prb of given coder for decoding
    return *c & (NBRA_PROB_SIZE - 1);
}
static struct nbra_range nbraModRange (const uint16_t cdf[17], unsigned char c) {
    //returns the range for given symbol in given model
    return (struct nbra_range){cdf[c], (uint16_t)(cdf[c+1] - cdf[c])};
}
static unsigned char nbraModSymbol (const uint16_t cdf[17], uint16_t prb) {
    //returns the symbol the range of which matches the given prb in given model
    #ifndef NIBRANS_NO_SSE2
        //prepare comparison vector
        __m128i comp = _mm_set1_epi16(prb);
        //results of (comp < cdf1) and (comp < cdf9) using SSE2
        __m128i cmp1 = _mm_cmplt_epi16(comp, *(__m128i*)&cdf[1]);
        __m128i cmp9 = _mm_cmplt_epi16(comp, *(__m128i*)&cdf[9]);
        //return symbol based on trailing zeroes of packed results
        return NIBRANS_CTZ(_mm_movemask_epi8(_mm_packs_epi16(cmp1, cmp9)));
    #else
        //no SSE2 variant (simple linear search)
        int i;
        for (i = 0; i < 16; i++)
            if (prb < cdf[i+1])
                return i;
    #endif
    return 0; //???
}
static void nbraModUpdate (uint16_t cdf[17], unsigned char c) {
    //updates the given model, registering an occurence of given symbol
    #pragma pack(2)
    static const uint16_t NBRA_ALIGN16(mixin[16][16]) = NBRA_MIXIN_INIT;
    #pragma pack()
    #ifndef NIBRANS_NO_SSE2
        //load already aligned cdf values into SSE2 values
        __m128i cdf1 = _mm_load_si128((__m128i*)&cdf[1]);
        __m128i cdf9 = _mm_load_si128((__m128i*)&cdf[9]);
        //do calculations and store in model: cdf += (mix - cdf) >> NIBRANS_RATE_BITS
        *(__m128i*)&cdf[1] = _mm_add_epi16(cdf1, _mm_srai_epi16(_mm_sub_epi16(*(__m128i*)&mixin[c][0], cdf1), NIBRANS_RATE_BITS));
        *(__m128i*)&cdf[9] = _mm_add_epi16(cdf9, _mm_srai_epi16(_mm_sub_epi16(*(__m128i*)&mixin[c][8], cdf9), NIBRANS_RATE_BITS));
    #else
        //no SSE variant (shifted up to avoid subtraction underflow)
        int i;
        for (i = 1; i < 16; i++)
            cdf[i] = (((uint32_t)cdf[i] << NIBRANS_RATE_BITS) + mixin[c][i-1] - cdf[i]) >> NIBRANS_RATE_BITS;
    #endif
}

#endif //NIBRANS_IMPLEMENTATION