// stdafx.h : includefile for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include "targetver.h"

//#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
//#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include <intrin.h>
#include <assert.h>
//#include <Windows.h>

//#pragma warning (disable: 4244)

//#include <x86intrin.h>
#include <immintrin.h>
#include <stdint.h>
#include <math.h>
#define __forceinline

    static inline uint16_t _byteswap_ushort(uint16_t x) {
		return ((( x  >> 8 ) & 0xffu ) | (( x  & 0xffu ) << 8 ));
	}
    static inline uint32_t _byteswap_ulong(uint32_t x) {
        return ((( x & 0xff000000u ) >> 24 ) |
                (( x & 0x00ff0000u ) >> 8  ) |
                (( x & 0x0000ff00u ) << 8  ) |
                (( x & 0x000000ffu ) << 24 ));
    }
    static inline uint64_t _byteswap_uint64(uint64_t x) {
        return ((( x & 0xff00000000000000ull ) >> 56 ) |
                (( x & 0x00ff000000000000ull ) >> 40 ) |
                (( x & 0x0000ff0000000000ull ) >> 24 ) |
                (( x & 0x000000ff00000000ull ) >> 8  ) |
                (( x & 0x00000000ff000000ull ) << 8  ) |
                (( x & 0x0000000000ff0000ull ) << 24 ) |
                (( x & 0x000000000000ff00ull ) << 40 ) |
                (( x & 0x00000000000000ffull ) << 56 ));
    }

static inline unsigned char _BitScanForward(
   unsigned long * Index,
   unsigned long Mask
) {
	unsigned long ret = __builtin_ctz(Mask);
	if(Index) *Index = ret;
	return Mask ? 1 : 0;
}
static inline unsigned char _BitScanReverse(
   unsigned long * Index,
   unsigned long Mask
) {
	unsigned long ret = __builtin_clz(Mask);
	if(Index) *Index = ret;
	return Mask ? 1 : 0;
}

// TODO: reference additional headers your program requires here
typedef uint8_t byte;
typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef unsigned int uint;
