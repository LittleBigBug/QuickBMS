/*
 * BoolStateD.cpp
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_Decoder_BoolState_CPP
#define AZO_Decoder_BoolState_CPP

#include "BoolStateD.h"

namespace AZO {
namespace Decoder {

template <u_int N>
inline BOOL BoolState<N>::Code(EntropyCode& entropy)
{
    BOOL b = entropy.Code(GetProb(), TotalBit());
    Update(b);

    return b;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_Decoder_BoolState_CPP*/
