/*
 * AZO.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_AZO_H
#define AZO_AZO_H

const int AZO_VERSION       (0x100);

// return error
const int AZO_OK                    (0);
const int AZO_STREAM_END            (1);
const int AZO_PARAM_ERROR           (-1);
const int AZO_MEM_ERROR             (-2);
const int AZO_OUTBUFF_FULL          (-3);
const int AZO_DATA_ERROR            (-4);
const int AZO_DATA_ERROR_VERSON     (-5);
const int AZO_DATA_ERROR_BLOCKSIZE  (-6);


typedef void* (*AZO_ALLOC_FUNCTON)(void*, unsigned int);
typedef void (*AZO_FREE_FUNCTON)(void*, void*);

struct AZO_Stream 
{
    const char* next_in;
    unsigned int avail_in;

    char* next_out;
    unsigned int avail_out;
};

// handle
typedef void*   AZO_HCOMPRESS;
typedef void*   AZO_HDECOMPRESS;


// init
int AZO_Version();

void AZO_Allocator(AZO_ALLOC_FUNCTON allocFunc, 
                   AZO_FREE_FUNCTON freeFunc, 
                   void* opaque);

//Decompress
int AZO_DecompressInit(AZO_HDECOMPRESS* pHandle /*OUT*/);

int AZO_Decompress(AZO_HDECOMPRESS handle,
                   AZO_Stream* stream /*INOUT*/);

int AZO_DecompressEnd(AZO_HDECOMPRESS handle);


int AZO_BufferDecompress(char* dest,
                        unsigned int* destLen,
                        const char* source,
                        unsigned int sourceLen);

#endif /*AZO_AZO_H*/
