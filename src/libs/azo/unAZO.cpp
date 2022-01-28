/*
 * unAZO.cpp
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_UNAZO_CPP
#define AZO_UNAZO_CPP

#include "AZO.h"
#include "Common/Allocator.h"
#include "Decoder/MainCodeD.h"

using namespace AZO;

int AZO_Version()
{
    return AZO_VERSION;
}

void AZO_Allocator(AZO_ALLOC_FUNCTON allocFunc, 
                   AZO_FREE_FUNCTON freeFunc, 
                   void* opaque)
{
    Allocator::Set(allocFunc, freeFunc, opaque);
}

//Compress
int AZO_DecompressInit(AZO_HCOMPRESS* pHandle /*OUT*/)
{
    if(pHandle == NULL) {
        return AZO_PARAM_ERROR;
    }

    void* buf = Allocator::Alloc(sizeof(Decoder::MainCode));
    if(buf == NULL) {
        return AZO_MEM_ERROR;
    }

    Decoder::MainCode* coder = new (buf) Decoder::MainCode();
    *pHandle = static_cast<AZO_HCOMPRESS>(coder);

    return AZO_OK;
}

int AZO_Decompress(AZO_HCOMPRESS handle, 
                 AZO_Stream* stream /*INOUT*/)
{
    Decoder::MainCode* coder = static_cast<Decoder::MainCode*>(handle);

    if(coder == NULL || stream == NULL) {
        return AZO_PARAM_ERROR;
    }

    const byte* in = reinterpret_cast<const  byte*>(stream->next_in);
    u_int iuAvailSize = stream->avail_in;
    byte* out = reinterpret_cast<byte*>(stream->next_out);
    u_int outAvailSize = stream->avail_out;

    int ret = coder->Code(in, iuAvailSize, out, outAvailSize);

    stream->next_in = reinterpret_cast<const char*>(in);
    stream->avail_in = iuAvailSize;
    stream->next_out = reinterpret_cast<char*>(out);
    stream->avail_out = outAvailSize;

    return ret;
}

int AZO_DecompressEnd(AZO_HCOMPRESS handle)
{
    Decoder::MainCode* coder = static_cast<Decoder::MainCode*>(handle);

    if(coder == NULL) {
        return AZO_PARAM_ERROR;
    }

    coder->~MainCode();
    Allocator::Free(coder);

    return AZO_OK;
}

int AZO_BufferDecompress(char* dest,
                         u_int* destLen,
                         const char* source,
                         u_int sourceLen)
{
    if(dest == NULL || destLen == NULL || source == NULL) {
        return AZO_PARAM_ERROR;
    }

    void* buf = Allocator::Alloc(sizeof(Decoder::MainCode));
    if(buf == NULL) {
        return AZO_MEM_ERROR;
    }

    Decoder::MainCode* coder = new (buf) Decoder::MainCode();

    const byte* in = reinterpret_cast<const  byte*>(source);
    u_int iuAvailSize = sourceLen;
    byte* out = reinterpret_cast<byte*>(dest);
    u_int outAvailSize = *destLen;

    int ret = coder->Code(in, iuAvailSize, out, outAvailSize);
    
    *destLen -= outAvailSize;

    coder->~MainCode();
    Allocator::Free(coder);

    return ret;
}


#endif /*AZO_UNAZO_CPP*/
