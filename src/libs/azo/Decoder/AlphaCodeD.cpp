/*
 * AlphaCodeD.cpp
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_ALPHACODE_CPP
#define AZO_DECODER_ALPHACODE_CPP

#include "AlphaCodeD.h"

namespace AZO {
namespace Decoder {

inline byte AlphaCode::Code(EntropyCode& entropy, u_int pos, byte pre)
{
    UNUSED_ARG(pos);

    return static_cast<byte>(alphaProb_.Code(entropy, pre));
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_ALPHACODE_CPP*/
