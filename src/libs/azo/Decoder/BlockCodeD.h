/*
 * BlockCodeD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_BLOCKCODE_H
#define AZO_DECODER_BLOCKCODE_H

#include "BoolStateD.h"
#include "AlphaCodeD.h"
#include "MatchCodeD.h"

namespace AZO {
namespace Decoder {

class BlockCode
{
public: 
    BlockCode(byte* buf, u_int size);
    ~BlockCode();
    
    int Code(EntropyCode& entropy);

private:
    bool CopyBlock(u_int pos, u_int dist, u_int len);
    u_int GetCode(EntropyCode& entropy, u_int pos);

private:
    byte* buf_;
    u_int bufSize_;

    BoolState<> matchState_;
    AlphaCode   alphaProb_;
    MatchCode   matchCode_;
};

} //namespaces Decoder
} //namespaces AZO

#include "BlockCodeD.cpp"

#endif /*AZO_DECODER_BLOCKCODE_H*/
