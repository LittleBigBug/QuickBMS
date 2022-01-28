/*
 * BoolStateD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_BOOLSTATE_H
#define AZO_DECODER_BOOLSTATE_H

#include "../Base/BoolStateB.h"
#include "EntropyCodeD.h"

namespace AZO {
namespace Decoder {

template <u_int N = 8>
class BoolState : Base::BoolState<N>
{
    typedef Base::BoolState<N> super;
public:
    BoolState() {}
    ~BoolState() {}
    
    BOOL Code(EntropyCode& entropy);

private:
    using super::GetProb;
    using super::Update;
    using super::TotalBit;
};

} //namespaces Decoder
} //namespaces AZO

#include "BoolStateD.cpp"

#endif /*AZO_DECODER_BOOLSTATE_H*/
