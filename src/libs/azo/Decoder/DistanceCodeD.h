/*
 * DistanceCodeD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_DISTANCECODE_H
#define AZO_DECODER_DISTANCECODE_H

#include "HistoryListD.h"
#include "EntropyBitProbD.h"

namespace AZO {
namespace Decoder {

class DistanceCode
{
public:
    DistanceCode() : 
        history_(MATCH_MIN_DIST) {}
    
    u_int Code(EntropyCode& entropy);

private:
    HistoryList<u_int, DISTANCE_HISTORY_SIZE>    history_;     
    EntropyBitProb<MATCH_DIST_CODE_SIZE>            prob_;
};

} //namespaces Decoder
} //namespaces AZO

#include "DistanceCodeD.cpp"

#endif /*AZO_DECODER_DISTANCECODE_H*/
