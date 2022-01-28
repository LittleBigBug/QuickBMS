/*
 *  LZH-Light algorithm implementation v 1.0
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Permission to use, copy, modify, distribute and sell this  software 
 *  for any purpose is hereby  granted  without  fee,  subject  to  the 
 *  following restrictions:
 *  1. this notice may not be removed or altered;
 *  2. altered source versions must be plainly marked as such, and must 
 *     not be misrepresented as being the original software.
 */

#ifndef __lzhl_h
#define __lzhl_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { int _; }* LZHL_CHANDLE;
typedef struct { int _; }* LZHL_DHANDLE;
#define LZHL_CHANDLE_NULL ((LZHL_CHANDLE)0)
#define LZHL_DHANDLE_NULL ((LZHL_DHANDLE)0)

#ifdef __cplusplus
extern "C" {
#endif

LZHL_CHANDLE LZHLCreateCompressor( void );
size_t LZHLCompressorCalcMaxBuf( size_t );
size_t LZHLCompress( LZHL_CHANDLE, void* dst, const void* src, size_t srcSz );
void LZHLDestroyCompressor( LZHL_CHANDLE );

LZHL_DHANDLE LZHLCreateDecompressor( void );
int  LZHLDecompress( LZHL_DHANDLE, void* dst, size_t* dstSz, void* src, size_t* srcSz );
void LZHLDestroyDecompressor( LZHL_DHANDLE );

#ifdef __cplusplus
}
#endif

#endif
