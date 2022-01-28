#ifndef AZO_DECODER_SYMBOLCODE_CPP
#define AZO_DECODER_SYMBOLCODE_CPP

#include "SymbolCodeD.h"

namespace AZO {
namespace Decoder {

template <typename T, u_int N, u_int HISTORY_N>
inline T SymbolCode<T, N, HISTORY_N>::Code(EntropyCode& entropy)
{
    T ret(0);
    if(history_.Code(entropy, ret) == false) {
        ret = prob_.Code(entropy);
        history_.Add(ret);
    }

    return ret;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_SYMBOLCODE_CPP*/
