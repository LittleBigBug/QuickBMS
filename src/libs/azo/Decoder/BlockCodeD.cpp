/*
 * BlockCodeD.cpp
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_BLOCKCODE_CPP
#define AZO_DECODER_BLOCKCODE_CPP

#include "BlockCodeD.h"

namespace AZO {
namespace Decoder {

inline BlockCode::BlockCode(byte* buf, u_int size) : 
    buf_(buf), bufSize_(size),
    matchCode_(buf, size)
{    
}

inline BlockCode::~BlockCode()
{
}

inline bool BlockCode::CopyBlock(u_int pos, u_int dist, u_int len)
{
    if(pos >= dist && pos+len <= bufSize_) {
        for(u_int i=0; i<len; ++i) {
            buf_[pos+i] = buf_[pos-dist+i];
        }

        return true;
    } else {
        return false;
    }
}


inline u_int BlockCode::GetCode(EntropyCode& entropy, u_int pos)
{
    if(matchState_.Code(entropy) == false)
    {        
        ASSERT(pos > 0);

        buf_[pos] = alphaProb_.Code(entropy, pos, buf_[pos-1]);

        return 1;
    }
    else
    {
        u_int dist(0);
        u_int length(0);
        matchCode_.Code(entropy, pos, dist, length);

        if(CopyBlock(pos, dist, length)) {
            return length;
        } else {
            return 0;
        }
    }
}

inline int BlockCode::Code(EntropyCode& entropy)
{
    int ret(AZO_OK);

    entropy.Initialize();

    buf_[0] = alphaProb_.Code(entropy, 0, 0);

    for(u_int i=1; i<bufSize_; )
    {
        u_int retLen = GetCode(entropy, i);
        if(retLen == 0) {  //error
            ret = AZO_DATA_ERROR;
            break;
        }

        i += retLen;
    }
    entropy.Finalize();

    return ret;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_BLOCKCODE_CPP*/
