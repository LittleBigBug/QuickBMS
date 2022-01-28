#ifndef AZO_DECODER_LENGTHCODE_CPP
#define AZO_DECODER_LENGTHCODE_CPP

#include "LengthCodeD.h"
#include "../Common/MatchCodeTable.h"

namespace AZO {
namespace Decoder {

inline u_int LengthCode::Code(EntropyCode& entropy, uint8_t distCode)
{
    u_int lenCode = prob_.Code(entropy, distCode);

    u_int length = MATCH_LENGTH_CODE_TABLE[lenCode];
    if(MATCH_LENGTH_EXTRABIT_TABLE[lenCode]) {
        length += entropy.Code(MATCH_LENGTH_EXTRABIT_TABLE[lenCode]);
    }

    return length;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_LENGTHCODE_CPP*/
