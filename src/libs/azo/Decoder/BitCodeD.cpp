/*
 * BitCodeD.cpp
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_BitCode_CPP
#define AZO_DECODER_BitCode_CPP

#include "BitCodeD.h"

namespace AZO {
namespace Decoder {

inline BitCode::BitCode(const TYPE* buf, u_int size) : inBuf_(buf), bufSize_(size*8)
{
    remainBit_ = TYPE_BIT_SIZE;
    readSize_ = 0;
}


template <typename T>
inline bool BitCode::Code(T& value, u_int bitSize)
{
//    ASSERT(bufSize_ >= readSize_ + bitSize);
    if(bufSize_ < readSize_ + bitSize) {
        return false;
    }

    readSize_ += bitSize;
    value = 0;

    if(remainBit_ <= bitSize)
    {   
        bitSize -= remainBit_;
        value = static_cast<T>((*inBuf_++ & ((1<<remainBit_)-1)) << bitSize);
        remainBit_ = TYPE_BIT_SIZE;
    
        while(bitSize >= TYPE_BIT_SIZE)
        {
            bitSize -= TYPE_BIT_SIZE;
            value |= (*inBuf_++ & TYPE_BIT_MASK) << bitSize;
        }
    }

    if(bitSize)
    {
        remainBit_ -= bitSize;
        value |= (*inBuf_ >> remainBit_) & ((1<<bitSize)-1);
    }

    return true;
}



template <typename T>
inline bool BitCode::Code(T* values, u_int bitSize)
{
    for(u_int i=0; i<bitSize/8; ++i) {
        if(Code(values[i], sizeof(T)*8) == false) {
            return false;
        }
    }

    int r = bitSize - bitSize/8*8;
    if(r)
    {
        if(Put(values[bitSize/8+1], r) == false) {
            return false;
        }
    }

    return true;
}


inline bool BitCode::Code(bool& v)
{
    //return Code(v, 1);

	ASSERT(readSize_ <= bufSize_);
    if(bufSize_ == readSize_) {
        return false;
    }

    ++readSize_;
	--remainBit_;
	
	v = *inBuf_ & (1u << remainBit_) ? true:false;

	if(remainBit_ == 0) {
		++inBuf_;
        remainBit_ = TYPE_BIT_SIZE;
	}

    return true;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_BitCode_CPP*/
