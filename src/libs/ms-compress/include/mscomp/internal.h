// ms-compress: implements Microsoft compression algorithms
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


// General include file which includes necessary files, defines, and typedefs for internal use.

// This has been made for use with MSVC, GCC, and Clang with fallbacks for other compilers.
// To make other compilers work at their full potential, you should add stuff below, particularly
// to the INLINE/FORCE_INLINE and Builtins/Intrinsics sections.

#ifndef MSCOMP_INTERNAL_H
#define MSCOMP_INTERNAL_H

// For MSVC
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

// For GCC
#define __STDC_LIMIT_MACROS

#include "general.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <new>

// Check that it is 8 bits to the byte
#if CHAR_BIT != 8
	#error Unsupported char size
#endif

///// Determine the endianness of the compilation, however this isn't very accurate /////
// It would be much better to define MSCOMP_LITTLE_ENDIAN, MSCOMP_MSCOMP_BIG_ENDIAN, or PDP_ENDIAN yourself
// MSCOMP_LITTLE_ENDIAN is what the program is developed for and tested with
// MSCOMP_BIG_ENDIAN and PDP_ENDIAN are untested
#if !defined(MSCOMP_LITTLE_ENDIAN) && !defined(MSCOMP_BIG_ENDIAN) && !defined(MSCOMP_PDP_ENDIAN)
	#if defined(_MSC_VER) || defined(_WIN32)
		#define MSCOMP_LITTLE_ENDIAN
	#elif defined(__BYTE_ORDER__)
		#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			#define MSCOMP_LITTLE_ENDIAN
		#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			#define MSCOMP_BIG_ENDIAN
		#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
			#define MSCOMP_PDP_ENDIAN
		#else
			#error unknown endian, define one of LITTLE_ENDIAN, BIG_ENDIAN, or PDP_ENDIAN
		#endif
	#elif defined(__LITTLE_ENDIAN__)
		#define MSCOMP_LITTLE_ENDIAN
	#elif defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
		#define MSCOMP_BIG_ENDIAN
	#else
		#include <endian.h> // may also be in machine/endian.h (Mac OS and Open BSD) or sys/endian.h (non-open BSD)
		#if defined(__BYTE_ORDER)
			#if   __BYTE_ORDER == __LITTLE_ENDIAN
				#define MSCOMP_LITTLE_ENDIAN
			#elif __BYTE_ORDER == __BIG_ENDIAN
				#define MSCOMP_BIG_ENDIAN
			#elif __BYTE_ORDER == __PDP_ENDIAN
				#define MSCOMP_PDP_ENDIAN
			#endif
		#elif defined(_BYTE_ORDER)
			#if   _BYTE_ORDER == _LITTLE_ENDIAN
				#define MSCOMP_LITTLE_ENDIAN
			#elif _BYTE_ORDER == _BIG_ENDIAN
				#define MSCOMP_BIG_ENDIAN
			#elif _BYTE_ORDER == _PDP_ENDIAN
				#define MSCOMP_PDP_ENDIAN
			#endif
		#endif
	#endif
#endif

///// Get ints from a byte stream /////
// These assume that the byte stream is little-endian (except RAW which don't care)
#if defined(MSCOMP_WITH_UNALIGNED_ACCESS)
#define GET_UINT16_RAW(x)		(*(const uint16_t*)(x))
#define GET_UINT32_RAW(x)		(*(const uint32_t*)(x))
#define SET_UINT16_RAW(x,val)	(*(uint16_t*)(x) = (uint16_t)(val))
#define SET_UINT32_RAW(x,val)	(*(uint32_t*)(x) = (uint32_t)(val))
#if defined(MSCOMP_LITTLE_ENDIAN)
	#define GET_UINT16(x)		(*(const uint16_t*)(x))
	#define GET_UINT32(x)		(*(const uint32_t*)(x))
	#define SET_UINT16(x,val)	(*(uint16_t*)(x) = (uint16_t)(val))
	#define SET_UINT32(x,val)	(*(uint32_t*)(x) = (uint32_t)(val))
#elif defined(MSCOMP_BIG_ENDIAN)
	// These could also use the without-unaligned-access versions always
	#define GET_UINT16(x)		byte_swap(*(const uint16_t*)(x))
	#define GET_UINT32(x)		byte_swap(*(const uint32_t*)(x))
	#define SET_UINT16(x,val)	(*(uint16_t*)(x) = byte_swap((uint16_t)(val)))
	#define SET_UINT32(x,val)	(*(uint32_t*)(x) = byte_swap((uint32_t)(val)))
#elif defined(MSCOMP_PDP_ENDIAN) // for 16-bit ints its the same as little-endian
	#define GET_UINT16(x)		(*(const uint16_t*)(x))
	#define GET_UINT32(x)		((*(const uint16_t*)(x)<<16)|*(const uint16_t*)((x)+2))
	#define SET_UINT16(x,val)	(*(uint16_t*)(x) = (uint16_t)(val))
	#define SET_UINT32(x,val)	(*(uint16_t*)(x) = (uint16_t)((val) >> 16), *(((uint16_t*)(x))+1) = (uint16_t)(val))
#else
	#error unknown endian, define one of MSCOMP_LITTLE_ENDIAN, MSCOMP_BIG_ENDIAN, or MSCOMP_PDP_ENDIAN
#endif
#else
#define GET_UINT16_RAW(x)		(((byte*)(x))[0]|(((byte*)(x))[1]<<8))
#define GET_UINT32_RAW(x)		(((byte*)(x))[0]|(((byte*)(x))[1]<<8)|(((byte*)(x))[2]<<16)|(((byte*)(x))[3]<<24))
#define SET_UINT16_RAW(x,val)	(((byte*)(x))[0]=(byte)(val), ((byte*)(x))[1]=(byte)((val)>>8))
#define SET_UINT32_RAW(x,val)	(((byte*)(x))[0]=(byte)(val), ((byte*)(x))[1]=(byte)((val)>>8), ((byte*)(x))[2]=(byte)((val)>>16), ((byte*)(x))[3]=(byte)((val)>>24))
#if defined(MSCOMP_LITTLE_ENDIAN)
	#define GET_UINT16(x)		(((byte*)(x))[0]|(((byte*)(x))[1]<<8))
	#define GET_UINT32(x)		(((byte*)(x))[0]|(((byte*)(x))[1]<<8)|(((byte*)(x))[2]<<16)|(((byte*)(x))[3]<<24))
	#define SET_UINT16(x,val)	(((byte*)(x))[0]=(byte)(val), ((byte*)(x))[1]=(byte)((val)>>8))
	#define SET_UINT32(x,val)	(((byte*)(x))[0]=(byte)(val), ((byte*)(x))[1]=(byte)((val)>>8), ((byte*)(x))[2]=(byte)((val)>>16), ((byte*)(x))[3]=(byte)((val)>>24))
#elif defined(MSCOMP_BIG_ENDIAN)
	#define GET_UINT16(x)		((((byte*)(x))[0]<<8)|((byte*)(x))[1])
	#define GET_UINT32(x)		((((byte*)(x))[0]<<24)|(((byte*)(x))[1]<<16)|(((byte*)(x))[2]<<8)|((byte*)(x))[3])
	#define SET_UINT16(x,val)	(((byte*)(x))[0]=(byte)((val)>>8), ((byte*)(x))[1]=(byte)((val)>>0))
	#define SET_UINT32(x,val)	(((byte*)(x))[0]=(byte)((val)>>24), ((byte*)(x))[1]=(byte)((val)>>16), ((byte*)(x))[2]=(byte)((val)>>8), ((byte*)(x))[3]=(byte)(val))
#elif defined(MSCOMP_PDP_ENDIAN) // for 16-bit ints its the same as little-endian
	#define GET_UINT16(x)		(((byte*)(x))[0]|(((byte*)(x))[1]<<8))
	#define GET_UINT32(x)		((((byte*)(x))[0]<<16)|(((byte*)(x))[1]<<24)|((byte*)(x))[2]|(((byte*)(x))[3]<<8))
	#define SET_UINT16(x,val)	(((byte*)(x))[0]=(byte)(val), ((byte*)(x))[1]=(byte)((val)>>8))
	#define SET_UINT32(x,val)	(((byte*)(x))[0]=(byte)((val)>>16), ((byte*)(x))[1]=(byte)((val)>>24), ((byte*)(x))[2]=(byte)(val), ((byte*)(x))[3]=(byte)((val)>>8))
#else
	#error unknown endian, define one of MSCOMP_LITTLE_ENDIAN, MSCOMP_BIG_ENDIAN, or MSCOMP_PDP_ENDIAN
#endif
#endif

///// Determine the number of bits used by pointers /////
#ifndef PNTR_BITS
	#if SIZE_MAX == UINT64_MAX
		#define PNTR_BITS 64
	#elif SIZE_MAX == UINT32_MAX
		#define PNTR_BITS 32
	#elif SIZE_MAX == UINT16_MAX
		#define PNTR_BITS 16
	#else
		#error You must define PNTR_BITS to be the number of bits used for pointers
	#endif
#endif

///// Determine if we have processor extensions /////
// Note: most compilers define these for us, just MSVC doesn't define __SSE__/__SSE2__
// However it does define __AVX__
#if defined(_MSC_VER)
#if defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP == 2)
#define __SSE2__
#define __SSE__
#elif defined(_M_IX86_FP) && _M_IX86_FP == 1
#define __SSE__
#endif
#endif
#ifdef __SSE__
#include <xmmintrin.h>
#endif

///// Get NOINLINE, INLINE and FORCE_INLINE /////
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#define INLINE __inline
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#define INLINE inline
#define FORCE_INLINE inline __attribute__((always_inline))
#elif (__STDC_VERSION__ >= 199901L)
#define NOINLINE
#define INLINE inline
#define FORCE_INLINE INLINE
#else
#define NOINLINE
#define INLINE
#define FORCE_INLINE INLINE
#endif

///// Get RESTRICT /////
#if defined(_MSC_VER)
#define RESTRICT __restrict
#elif defined(__GNUC__)
#define RESTRICT __restrict__
#elif (__STDC_VERSION__ >= 199901L)
#define RESTRICT restrict
#else
#define RESTRICT
#endif
// typedef the most common restricted pointers
typedef byte* RESTRICT rest_bytes;
typedef const_byte* RESTRICT const_rest_bytes;


///// Intrinsic and Built-in functions /////
// The available compiler hints are:
// 	 LIKELY(x) / UNLIKELY(x)	- used in conditionals to tell the compiler the outcome is (un)likely
//   ALWAYS(x) / NEVER(x)    - used to tell the compiler about known restrictions of variables
//   ASSERT_ALWAYS(x)        - assert in DEBUG mode, ALWAYS in non-DEBUG
//	 UNREACHABLE()           - tells the compiler that execution can never reach here
// If compiled with DEBUG_ALWAYS_NEVER defined, ALWAYS/NEVER/UNREACHABLE print errors if they fail instead
//
// The available functions that may map to intrinsics are:
// (all have uint8_t, uint16_t, and uint64_t argument overloads)
//   uint32_t rotl(uint32_t x, int bits)      - rotate left with carry
//   int count_bits_set(uint32_t x)           - count number of 1 bits
//   int count_leading_zeros(uint32_t x)      - count number of most-significant zeros, undefined for 0
//   int log2(uint32_t x)                     - get log-base-2 of an integer, undefined for 0
//   uint32_t byte_swap(uint32_t x)           - swap the order of the bytes, not available for uint8_t
//
// To make log2 give 0 for 0 instead of be undefined, use it like log2(x|1)

#if defined(_MSC_VER)
	// see https://msdn.microsoft.com/en-us/library/hh977022.aspx
	#include <intrin.h>
	#define ALWAYS(x)     __assume(x)
	#define LIKELY(x)     (x)
	#define UNLIKELY(x)   (x)
	#define NEVER(x)      __assume(!(x))
	#define UNREACHABLE() __assume(0)
	#ifdef __SSE__
	#define PREFETCH(p)   _mm_prefetch((char*)(p), _MM_HINT_NTA)
	#else
	#define PREFETCH(p)   
	#endif
	#define ASSUME_ALIGNED(p, n) ((void*)(p))
	#pragma intrinsic(_rotl, memset, memcpy)
	uint8_t  FORCE_INLINE rotl(uint8_t x,  int bits) { return _rotl8 (x, (unsigned char)bits); }
	uint16_t FORCE_INLINE rotl(uint16_t x, int bits) { return _rotl16(x, (unsigned char)bits); }
	uint32_t FORCE_INLINE rotl(uint32_t x, int bits) { return _rotl  (x, (unsigned char)bits); }
	uint64_t FORCE_INLINE rotl(uint64_t x, int bits) { return _rotl64(x, (unsigned char)bits); }
	#if defined(_M_ARM)
		int FORCE_INLINE count_bits_set(uint8_t x)  { return _CountOneBits(x); }
		int FORCE_INLINE count_bits_set(uint16_t x) { return _CountOneBits(x); }
		int FORCE_INLINE count_bits_set(uint32_t x) { return _CountOneBits(x); }
		int FORCE_INLINE count_bits_set(uint64_t x) { return _CountOneBits64(x); }
		int FORCE_INLINE count_leading_zeros(uint8_t x)  { return _CountLeadingZeros(x) - 24; }
		int FORCE_INLINE count_leading_zeros(uint16_t x) { return _CountLeadingZeros(x) - 16; }
		int FORCE_INLINE count_leading_zeros(uint32_t x) { return _CountLeadingZeros(x); }
		int FORCE_INLINE count_leading_zeros(uint64_t x) { return _CountLeadingZeros64(x); }
		int FORCE_INLINE log2(uint8_t x)  { return 31 - _CountLeadingZeros(x);   }
		int FORCE_INLINE log2(uint16_t x) { return 31 - _CountLeadingZeros(x);   }
		int FORCE_INLINE log2(uint32_t x) { return 31 - _CountLeadingZeros(x);   }
		int FORCE_INLINE log2(uint64_t x) { return 63 - _CountLeadingZeros64(x); }
	#elif defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64)
		// TODO: lzcnt16/lzcnt/lzcnt64 are only available if bit 5 of CPUInfo[2] (ECX) is set after __cpuid(int cpuInfo[4], 0x80000001) [Haswell - 2013]
		// TODO: popcnt16/popcnt/popcnt64 are only available if bit 23 of CPUInfo[2] (ECX) is set after __cpuid(int cpuInfo[4], 0x00000001) [Nephalem - 2008]
		int FORCE_INLINE count_bits_set(uint8_t x)  { return __popcnt16(x); }
		int FORCE_INLINE count_bits_set(uint16_t x) { return __popcnt16(x); }
		int FORCE_INLINE count_bits_set(uint32_t x) { return __popcnt(x); }
		int FORCE_INLINE count_leading_zeros(uint8_t x)  { unsigned long r; _BitScanReverse(&r, x); return (31-r); }
		int FORCE_INLINE count_leading_zeros(uint16_t x) { unsigned long r; _BitScanReverse(&r, x); return (31-r); }
		int FORCE_INLINE count_leading_zeros(uint32_t x) { unsigned long r; _BitScanReverse(&r, x); return (31-r); }
		//int FORCE_INLINE count_leading_zeros(uint8_t x)  { return __lzcnt16(x); }
		//int FORCE_INLINE count_leading_zeros(uint16_t x) { return __lzcnt16(x); }
		//int FORCE_INLINE count_leading_zeros(uint32_t x) { return __lzcnt(x);   }
		int FORCE_INLINE log2(uint8_t x)  { unsigned long r; _BitScanReverse(&r, x); return r; }
		int FORCE_INLINE log2(uint16_t x) { unsigned long r; _BitScanReverse(&r, x); return r; }
		int FORCE_INLINE log2(uint32_t x) { unsigned long r; _BitScanReverse(&r, x); return r; }
		//int FORCE_INLINE log2(uint8_t x)  { return 15 - __lzcnt16(x); }
		//int FORCE_INLINE log2(uint16_t x) { return 15 - __lzcnt16(x); }
		//int FORCE_INLINE log2(uint32_t x) { return 31 - __lzcnt(x);   }
		#if defined(_M_AMD64) || defined(_M_X64)
			int FORCE_INLINE count_bits_set(uint64_t x) { return (int)__popcnt64(x); }
			int FORCE_INLINE count_leading_zeros(uint64_t x) { unsigned long r; _BitScanReverse64(&r, x); return (63-r); }
			//int FORCE_INLINE count_leading_zeros(uint64_t x) { return __lzcnt64(x); }
			int FORCE_INLINE log2(uint64_t x) { unsigned long r; _BitScanReverse64(&r, x); return r; }
			//int FORCE_INLINE log2(uint64_t x) { return 63 - __lzcnt64(x); }
		#else
			int FORCE_INLINE count_bits_set(uint64_t x) { return __popcnt((uint32_t)x) + __popcnt((uint32_t)(x >> 32)); }
			int FORCE_INLINE count_leading_zeros(uint64_t x) { unsigned long r; uint32_t y = (uint32_t)(x>>32); if (y) { _BitScanReverse(&r, y); return (31-r); } else { _BitScanReverse(&r, (uint32_t)x); return (63-r); } }
			//int FORCE_INLINE count_leading_zeros(uint64_t x) { uint32_t y = (uint32_t)(x>>32); return y ? _lzcnt(y)+32 : __lzcnt((uint32_t)x); }
			int FORCE_INLINE log2(uint64_t x) { unsigned long r; uint32_t y = (uint32_t)(x>>32); if (y) { _BitScanReverse(&r, y); return r+32; } else { _BitScanReverse(&r, (uint32_t)x); return r; } }
			//int FORCE_INLINE log2(uint64_t x) { uint32_t y = (uint32_t)(x>>32); return y ? (63-_lzcnt(y)) : (31-__lzcnt((uint32_t)x)); }
		#endif
	#endif
	uint16_t FORCE_INLINE byte_swap(uint16_t x) { return _byteswap_ushort(x); }
	uint32_t FORCE_INLINE byte_swap(uint32_t x) { return _byteswap_ulong(x);  }
	uint64_t FORCE_INLINE byte_swap(uint64_t x) { return _byteswap_uint64(x); }
#elif defined(__GNUC__) // GCC and Clang
	// see https://gcc.gnu.org/onlinedocs/gcc-4.5.0/gcc/Other-Builtins.html
	#define ALWAYS(x)     if (!(x)) { __builtin_unreachable(); }
	#define LIKELY(x)     __builtin_expect((x), 1)
	#define UNLIKELY(x)   __builtin_expect((x), 0)
	#define NEVER(x)      if (x) { __builtin_unreachable(); }
	#define UNREACHABLE() __builtin_unreachable()
	#define PREFETCH(p)   __builtin_prefetch(p, 0, 0)
	#define ASSUME_ALIGNED(p, n) __builtin_assume_aligned(p, n)
	uint8_t  FORCE_INLINE rotl(uint8_t x,  int bits) { return ((x << bits) | (x >> (8  - bits))); } // the compiler detects these and optimizes, no need for a special builtin
	uint16_t FORCE_INLINE rotl(uint16_t x, int bits) { return ((x << bits) | (x >> (16 - bits))); }
	uint32_t FORCE_INLINE rotl(uint32_t x, int bits) { return ((x << bits) | (x >> (32 - bits))); }
	uint64_t FORCE_INLINE rotl(uint64_t x, int bits) { return ((x << bits) | (x >> (64 - bits))); }
	int FORCE_INLINE count_bits_set(uint8_t x)  { return __builtin_popcount(x); }
	int FORCE_INLINE count_bits_set(uint16_t x) { return __builtin_popcount(x); }
	int FORCE_INLINE count_bits_set(uint32_t x) { return __builtin_popcount(x); }
	int FORCE_INLINE count_bits_set(uint64_t x) { return __builtin_popcountll(x); }
	int FORCE_INLINE count_leading_zeros(uint8_t  x) { return __builtin_clz(x) - 24; }
	int FORCE_INLINE count_leading_zeros(uint16_t x) { return __builtin_clz(x) - 16; }
	int FORCE_INLINE count_leading_zeros(uint32_t x) { return __builtin_clz(x); }
	int FORCE_INLINE count_leading_zeros(uint64_t x) { return __builtin_clzll(x); }
	int FORCE_INLINE log2(uint8_t x)  { return 31 - __builtin_clz(x);   }
	int FORCE_INLINE log2(uint16_t x) { return 31 - __builtin_clz(x);   }
	int FORCE_INLINE log2(uint32_t x) { return 31 - __builtin_clz(x);   }
	int FORCE_INLINE log2(uint64_t x) { return 63 - __builtin_clzll(x); }
	uint16_t FORCE_INLINE byte_swap(uint16_t x) { return (x<<8)|(x>>8); }
	uint32_t FORCE_INLINE byte_swap(uint32_t x) { return (uint32_t)__builtin_bswap32((int32_t)x); }
	uint64_t FORCE_INLINE byte_swap(uint64_t x) { return (uint64_t)__builtin_bswap64((int64_t)x); }
#else
	#define ALWAYS(x)     
	#define LIKELY(x)     (x)
	#define UNLIKELY(x)   (x)
	#define NEVER(x)      
	#define UNREACHABLE()
	#define PREFETCH(p)
	#define ASSUME_ALIGNED(p, n) ((void*)(p))
	uint8_t  FORCE_INLINE rotl(uint8_t x,  int bits) { return ((x << bits) | (x >> (8  - bits))); }
	uint16_t FORCE_INLINE rotl(uint16_t x, int bits) { return ((x << bits) | (x >> (16 - bits))); }
	uint32_t FORCE_INLINE rotl(uint32_t x, int bits) { return ((x << bits) | (x >> (32 - bits))); }
	uint64_t FORCE_INLINE rotl(uint64_t x, int bits) { return ((x << bits) | (x >> (64 - bits))); }
	int FORCE_INLINE count_bits_set(uint8_t x)  { x -= (x>>1)&0x55; x = (((x>>2)&0x33) + (x&0x33)); x = (((x>>4)+x)&0x0f); return x&0x0f; }
	int FORCE_INLINE count_bits_set(uint16_t x) { x -= (x>>1)&0x5555; x = (((x>>2)&0x3333) + (x&0x3333)); x = (((x>>4)+x)&0x0f0f); x += (x>>8); return x&0x1f; }
	int FORCE_INLINE count_bits_set(uint32_t x) { x -= (x>>1)&0x55555555; x = (((x>>2)&0x33333333) + (x&0x33333333)); x = (((x>>4)+x)&0x0f0f0f0f); x += (x>>8); x += (x>>16); return x&0x3f; }
	int FORCE_INLINE count_bits_set(uint64_t x) { x -= (x>>1)&0x5555555555555555ull; x = ((x>>2)&0x3333333333333333ull) + (x&0x3333333333333333ull); return (int)(((((x>>4)+x)&0xf0f0f0f0f0f0f0full)*0x101010101010101ull)>>56); }
	int FORCE_INLINE count_leading_zeros(uint8_t x)  { x |= (x>>1); x |= (x>>2); x |= (x>>4); return 8 - count_bits_set(x); }
	int FORCE_INLINE count_leading_zeros(uint16_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); return 16 - count_bits_set(x); }
	int FORCE_INLINE count_leading_zeros(uint32_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); x |= (x>>16); return 32 - count_bits_set(x); }
	int FORCE_INLINE count_leading_zeros(uint64_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); x |= (x>>16); x |= (x>>32); return 64 - count_bits_set(x); }
	int FORCE_INLINE log2(uint8_t x)  { x |= (x>>1); x |= (x>>2); x |= (x>>4); return count_bits_set(x) - 1; } // returns 0x0 - 0x7
	int FORCE_INLINE log2(uint16_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); return count_bits_set(x) - 1; } // returns 0x0 - 0xF
	int FORCE_INLINE log2(uint32_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); x |= (x>>16); return count_bits_set(x) - 1; } // returns 0x00 - 0x1F
	int FORCE_INLINE log2(uint64_t x) { x |= (x>>1); x |= (x>>2); x |= (x>>4); x |= (x>>8); x |= (x>>16); x |= (x>>32); return count_bits_set(x) - 1; } // returns 0x0 - 0x3F
	uint16_t FORCE_INLINE byte_swap(uint16_t x) { return (x<<8)|(x>>8); }
	uint32_t FORCE_INLINE byte_swap(uint32_t x) { return (x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24); }
	uint64_t FORCE_INLINE byte_swap(uint64_t x) { return (x<<56)|((x<<40)&0x00FF000000000000ull)|((x<<24)&0x0000FF0000000000ull)|((x<<8)&0x000000FF00000000ull)|((x>>8)&0x00000000FF000000ull)|((x>>24)&0x0000000000FF0000)|((x>>40)&0x000000000000FF00)|(x>>56); }
#endif
#ifdef DEBUG_ALWAYS_NEVER
	#include <stdio.h>
	#undef ALWAYS
	#undef NEVER
	#undef UNREACHABLE
	#define ALWAYS(x)     if (!(x)) { fprintf(stderr, "Not always: '%s' (%s:%d)\n", #x, __FILE__, __LINE__); }
	#define NEVER(x)      if (x) { fprintf(stderr, "Not never: '%s' (%s:%d)\n", #x, __FILE__, __LINE__); }
	#define UNREACHABLE() fprintf(stderr, "Should have been unreachable (%s:%d)\n", __FILE__, __LINE__);
#endif
#define ASSERT_ALWAYS(x)  assert(x); ALWAYS(x)
// TODO: some other intrinsics to look into:
//  __builtin_ffs       one plus the index of the least significant 1-bit of x, or if x is zero, returns zero (similar to ctz)
//  __builtin_ctz       the number of trailing 0-bits in x, starting at the least significant bit position; if x is 0, the result is undefined (similar to ffs)
//  __builtin_parity    the parity of x, i.e. the number of 1-bits in x modulo 2
// (all operate on unsigned int and return int, add an l for unsigned long and ll for unsigned long-long arguments)

//  __builtin_choose_expr  like the ? operator except the condition needs to be constant and has some other benefits

///// Get ARRAYSIZE /////
#ifndef ARRAYSIZE
	#define ARRAYSIZE(x) sizeof(x)/sizeof(x[0])
#endif

///// Get the minimum/maximum of 2 /////
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

///// Get SIZE_T format specifier /////
#if defined(_WIN32) && (!defined(__USE_MINGW_ANSI_STDIO) || __USE_MINGW_ANSI_STDIO != 1)
#define SSIZE_T_FMT "I"
#else
#define SSIZE_T_FMT "z"
#endif

///// Compile it right /////
#if defined(__cplusplus_cli)
#pragma unmanaged
#endif
#if defined(_MSC_VER) && defined(NDEBUG)
#pragma optimize("t", on)
#endif
#if defined(__GNUC__) && defined(__MINGW32__)
// GCC assumes a 16-byte aligned stack, Windows only guarantees 4-byte alignment, we need to tell
// GCC to fix all entry points to have a 16-byte alignment (but once aligned, we are good to go).
// ENTRY_POINT only needs to be used on functions that may use SSE instructions in their call stack
// and that can be called directly by outside code (export, callback, or main function). 
#define ENTRY_POINT __attribute__((force_align_arg_pointer))
#else
#define ENTRY_POINT
#endif

///// Warning disable support /////
#if defined(_MSC_VER)
#define WARNINGS_PUSH() __pragma(warning(push))
#define WARNINGS_POP()  __pragma(warning(pop))
#define WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT()         __pragma(warning(disable:4127))
#define WARNINGS_IGNORE_ASSIGNMENT_WITHIN_COND_EXPR()       __pragma(warning(disable:4706))
#define WARNINGS_IGNORE_TRUNCATED_OVERFLOW()                __pragma(warning(disable:4309))
#define WARNINGS_IGNORE_ASSIGNMENT_OPERATOR_NOT_GENERATED() __pragma(warning(disable:4512))
#define WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()   __pragma(warning(disable:4701 4703))
#define WARNINGS_IGNORE_DIV_BY_0()				            __pragma(warning(disable:4723 4724))
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define WARNINGS_PUSH() _Pragma("GCC diagnostic push")
#define WARNINGS_POP()  _Pragma("GCC diagnostic pop")
#define WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT()         
#define WARNINGS_IGNORE_ASSIGNMENT_WITHIN_COND_EXPR()       
#define WARNINGS_IGNORE_TRUNCATED_OVERFLOW()                _Pragma("GCC diagnostic ignored \"-Woverflow\"")
#define WARNINGS_IGNORE_ASSIGNMENT_OPERATOR_NOT_GENERATED() 
#define WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()   _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define WARNINGS_IGNORE_DIV_BY_0()                          _Pragma("GCC diagnostic ignored \"-Wdiv-by-zero\"")
#else
#define WARNINGS_PUSH() 
#define WARNINGS_POP()  
#define WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT()         
#define WARNINGS_IGNORE_ASSIGNMENT_WITHIN_COND_EXPR()       
#define WARNINGS_IGNORE_TRUNCATED_OVERFLOW()                
#define WARNINGS_IGNORE_ASSIGNMENT_OPERATOR_NOT_GENERATED() 
#define WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()   
#define WARNINGS_IGNORE_DIV_BY_0()                          
#endif

///// Compile-time assert /////
#define CASSERT(expr)		char _UNIQUE_NAME[expr]
#define _UNIQUE_NAME		_MAKE_NAME(__LINE__)
#define _MAKE_NAME(line)	_MAKE_NAME2(line)
#define _MAKE_NAME2(line)	cassert_##line

///// Error and Warning Messages /////
#if defined(MSCOMP_WITH_ERROR_MESSAGES) || defined(MSCOMP_WITH_WARNING_MESSAGES)
#include <stdio.h>
#if _MSC_VER
#define snprintf _snprintf
#endif
#endif

#ifdef MSCOMP_WITH_ERROR_MESSAGES
#define SET_ERROR(s, ...)	snprintf(s->error, ARRAYSIZE(s->error), __VA_ARGS__)
#define INIT_STREAM_ERROR_MESSAGE(s) s->error[0] = 0
#else
#define SET_ERROR(s, ...)
#define INIT_STREAM_ERROR_MESSAGE(s)
#endif

#ifdef MSCOMP_WITH_WARNING_MESSAGES
#define SET_WARNING(s, ...)	snprintf(s->warning, ARRAYSIZE(s->warning), __VA_ARGS__)
#define INIT_STREAM_WARNING_MESSAGE(s) s->warning[0] = 0
#else
#define SET_WARNING(s, ...)
#define INIT_STREAM_WARNING_MESSAGE(s)
#endif

///// Stream initialization and checking /////
#define INIT_STREAM(s, c, f) \
	if (UNLIKELY(s == NULL)) { SET_ERROR(s, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; } \
	s->format = f; s->compressing = c; \
	s->in = NULL; s->out = NULL; \
	s->in_avail = 0; s->out_avail = 0; \
	s->in_total = 0; s->out_total = 0; \
	INIT_STREAM_ERROR_MESSAGE(s); INIT_STREAM_WARNING_MESSAGE(s); \
	s->state = NULL
#define CHECK_STREAM(s, c, f) \
	if (UNLIKELY(s == NULL || s->format != f || s->compressing != c || (s->in == NULL && s->in_avail != 0) || (s->out == NULL && s->out_avail != 0))) { SET_ERROR(s, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }
#define CHECK_STREAM_PLUS(s, c, f, x) \
	if (UNLIKELY(s == NULL || s->format != f || s->compressing != c || (s->in == NULL && s->in_avail != 0) || (s->out == NULL && s->out_avail != 0) || (x))) { SET_ERROR(s, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }

#define ADVANCE_IN(s, x)      s->in  += (x);          s->in_total  += (x);          s->in_avail -= (x)
#define ADVANCE_IN_TO_END(s)  s->in  += s->in_avail;  s->in_total  += s->in_avail;  s->in_avail  = 0
#define ADVANCE_OUT(s, x)     s->out += (x);          s->out_total += (x);          s->out_avail -= (x)
#define ADVANCE_OUT_TO_END(s) s->out += s->out_avail; s->out_total += s->out_avail; s->out_avail  = 0

///// Dump data from internal output buffer to stream /////
// Used by all inflate/deflate functions (except copy)
#define DUMP_OUT(state, stream) \
	if (state->out_avail) \
	{ \
		const size_t out_size = MIN(state->out_avail, stream->out_avail); \
		memcpy(stream->out, state->out + state->out_pos, out_size); \
		stream->out       += out_size; \
		stream->out_total += out_size; \
		if (state->out_avail == out_size) \
		{ \
			stream->out_avail -= out_size; \
			/*state->out_pos     = 0;*/ \
			state->out_avail   = 0; \
		} \
		else \
		{ \
			stream->out_avail  = 0; \
			state->out_pos    += out_size; \
			state->out_avail  -= out_size; \
			return MSCOMP_OK; \
		} \
	}

///// Append data to internal input buffer /////
// Used by all inflate/deflate functions (except copy)
#define APPEND_IN(state, stream, OP) \
	if (state->in_avail) \
	{ \
		for (;;) \
		{ \
			const size_t copy = MIN(state->in_needed, stream->in_avail); \
			memcpy(state->in + state->in_avail, stream->in, copy); \
			state->in_avail  += copy; \
			state->in_needed -= copy; \
			ADVANCE_IN(stream, copy); \
			OP \
			break; \
		} \
		state->in_avail = 0; \
	}

// COPY_4x - Copy the next 4 indices (0 through 3) of the pointer in to the pointer out
#define COPY_4x(out, in)    (out)[0] = (in)[0]; (out)[1] = (in)[1]; (out)[2] = (in)[2]; (out)[3] = (in)[3]

#if defined(MSCOMP_WITH_UNALIGNED_ACCESS)
// COPY_32 - Copy a 32-bit value from the pointer in to the pointer out
#define COPY_32(out, in)    *(uint32_t*)(out) = *(uint32_t*)(in)
// COPY_4x32 - Copy 4 32-bit values from the pointer in to the pointer out
#define COPY_4x32(out, in)  COPY_4x(((uint32_t*)(out)), ((uint32_t*)(in)))
#else
#define COPY_32(out, in)    COPY_4x((byte*)(out), (byte*)(in))
#define COPY_4x32(out, in)  COPY_32(((uint32_t*)(out)), ((uint32_t*)(in))); COPY_32(((uint32_t*)(out))+1, ((uint32_t*)(in))+1); COPY_32(((uint32_t*)(out))+2, ((uint32_t*)(in))+2); COPY_32(((uint32_t*)(out))+3, ((uint32_t*)(in))+3)
#endif

// COPY_128_FAST - Copy a 128-bit value from the pointer in to the pointer out
#if defined(__SSE__) && defined(MSCOMP_WITH_UNALIGNED_ACCESS)
#define COPY_128_FAST(out, in) _mm_storeu_ps((float*)(out), _mm_loadu_ps((float*)(in)))
#else
#define COPY_128_FAST(out, in) COPY_4x32(out, in)
#endif

#define FAST_COPY_ROOM 16

///// Copies data very fast from a buffer to itself /////
// This does limited checks for overruns. Before calling this there should be at least
// FAST_COPY_ROOM available in out. The "SHORT" version is designed for shorter runs on average.
// (at the moment they are the same because the SSE version ended up being slower once correct).
//  * out - the destination buffer
//  * in  - the source buffer
//  * off - the offset between the buffers (out-in)
//  * near_end - a pointer that is at least FAST_COPY_ROOM away from the end of the out buffer
//  * SLOW_COPY - code to be run when copying is not complete and we are near the end of the buffer
//                (typically a length check and a goto), it must jump (goto or return).
// out and len are updated as copy progress is made
#define FAST_COPY_SHORT(out, in, len, off, near_end, SLOW_COPY) \
{ \
	/* Write up to 3 bytes for close offsets so that we have >=4 bytes to read in all cases */ \
	switch (off) \
	{ \
	case 1: out[0] = out[1] = out[2] = in[0];       out += 3; len -= 3; break; \
	case 2: out[0] = in[0]; out[1] = in[1];         out += 2; len -= 2; break; \
	case 3: out[0]=in[0];out[1]=in[1];out[2]=in[2]; out += 3; len -= 3; break; \
	} \
	if (len) \
	{ \
		COPY_32(out+0, in+0); \
		COPY_32(out+4, in+4); \
		COPY_32(out+8, in+8); /* now have >=16 bytes that can be read in chunks of 4 bytes */ \
		if (len > 12) \
		{ \
			out += 12; in += 12; len -= 12; \
			if (UNLIKELY(out >= near_end)) { SLOW_COPY; } \
			/* Repeatedly write 16 bytes */ \
			while (len > 16) \
			{ \
				COPY_4x32(out, in); out += 16; in += 16; len -= 16; \
				if (UNLIKELY(out >= near_end)) { SLOW_COPY; } \
			} \
			/* Last 16 bytes */ \
			COPY_4x32(out, in); \
		} \
		out += len; \
	} \
}
#define FAST_COPY(out, in, len, off, near_end, SLOW_COPY) FAST_COPY_SHORT(out, in, len, off, near_end, SLOW_COPY)

#define ALL_AT_ONCE_WRAPPER_COMPRESS(name) \
	ENTRY_POINT MSCompStatus name##_compress(const_bytes in, size_t in_len, bytes out, size_t* _out_len) \
	{ \
		mscomp_stream strm; \
		MSCompStatus status = name##_deflate_init(&strm); \
		if (UNLIKELY(status != MSCOMP_OK)) { return status; } \
		strm.in = in; \
		strm.in_avail = in_len; \
		strm.out = out; \
		strm.out_avail = *_out_len; \
		status = name##_deflate(&strm, true); \
		*_out_len = strm.out_total; \
		name##_deflate_end(&strm); \
		return LIKELY(status == MSCOMP_STREAM_END) ? MSCOMP_OK : (status == MSCOMP_OK ? MSCOMP_BUF_ERROR : status); \
	}

#define ALL_AT_ONCE_WRAPPER_DECOMPRESS(name) \
	ENTRY_POINT MSCompStatus name##_decompress(const_bytes in, size_t in_len, bytes out, size_t* _out_len) \
	{ \
		mscomp_stream strm; \
		MSCompStatus status = name##_inflate_init(&strm); \
		if (UNLIKELY(status != MSCOMP_OK)) { return status; } \
		strm.in = in; \
		strm.in_avail = in_len; \
		strm.out = out; \
		strm.out_avail = *_out_len; \
		status = name##_inflate(&strm); \
		if (UNLIKELY(status <= MSCOMP_OK)) { name##_inflate_end(&strm); if (status == MSCOMP_OK) { status = MSCOMP_BUF_ERROR; } } \
		else if (LIKELY((status = name##_inflate_end(&strm)) == MSCOMP_OK)) { *_out_len = strm.out_total; } \
		return status; \
	}

#endif
