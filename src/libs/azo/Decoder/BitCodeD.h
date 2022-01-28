/*
 * BitCodeD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_BitCode_H
#define AZO_DECODER_BitCode_H

#include "../Common/AZOPrivate.h"

namespace AZO {
namespace Decoder {

class BitCode
{
    typedef byte    TYPE;

public:
    BitCode(const TYPE* buf, u_int size);

public:
    template <typename T>
    bool Code(T& value, u_int bitSize);

    template <typename T>
    bool Code(T* values, u_int bitSize);

    bool Code(bool& v);

    u_int GetReadSize() { return readSize_; }

private:
    static const u_int TYPE_BIT_SIZE = sizeof(TYPE)*8;
    static const TYPE TYPE_BIT_MASK = (1<<TYPE_BIT_SIZE)-1;

    const TYPE* inBuf_;
    u_int bufSize_;
    u_int remainBit_;
    u_int readSize_;
};

} //namespaces Decoder
} //namespaces AZO

#ifndef TEMPLATES_NO_REQUIRE_SOURCE
#include "BitCodeD.cpp"
#endif /*TEMPLATES_NO_REQUIRE_SOURCE*/

#endif /*AZO_DECODER_BitCode_H*/
