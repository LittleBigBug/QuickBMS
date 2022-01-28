#ifndef AZO_DECODER_MATCHCODE_CPP
#define AZO_DECODER_MATCHCODE_CPP

#include "MatchCodeD.h"
#include "../Common/MatchCodeTable.h"

namespace AZO {
namespace Decoder {

inline void MatchCode::Code(EntropyCode& entropy, u_int pos, 
                            u_int& dist, u_int& length)
{
    u_int dictPos(0);
    if(dictTable_.Code(entropy, dictPos, length) == false)
    {
        dist = distProb_.Code(entropy);
        length = lenProb_.Code(entropy, GetMatchDistCode(dist));

        dictTable_.Add(pos, length);
    }
    else
    {
        dist = pos - dictPos;
    }

    //ASSERT(dist >= MATCH_MIN_DIST);
    //ASSERT(length >= MATCH_MIN_LENGTH);
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_MATCHCODE_CPP*/
