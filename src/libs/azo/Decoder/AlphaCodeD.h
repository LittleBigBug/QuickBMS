/*
 * AlphaCodeD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_ALPHACODE_H
#define AZO_DECODER_ALPHACODE_H

#include "PredictProbD.h"

namespace AZO {
namespace Decoder {

class AlphaCode
{
public:
    AlphaCode() {}

    byte Code(EntropyCode& entropy, u_int pos, byte pre);

private:    
    PredictProb<ALPHA_SIZE, ALPHA_SIZE, ALPHACODE_PREDICT_SHIFT> alphaProb_;
};

} //namespaces Decoder
} //namespaces AZO

#include "AlphaCodeD.cpp"

#endif /*AZO_DECODER_ALPHACODE_H*/
