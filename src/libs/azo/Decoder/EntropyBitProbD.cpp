#ifndef AZO_DECODER_ENTROPYBITPROB_CPP
#define AZO_DECODER_ENTROPYBITPROB_CPP

#include "EntropyBitProbD.h"

namespace AZO {
namespace Decoder {

template <u_int N>
inline u_int EntropyBitProb<N>::Code(EntropyCode& entropy)
{
    u_int value = 0;

    u_int pre(1);
    for(int i=BIT_N-1; i>=0; --i)
    {
        const BOOL v = entropy.Code(GetProb(pre), TOTAL_BIT);
        if(v) value |= (static_cast<u_int>(1)<<i);

        pre = (pre << 1) | v;
    }

    Update(value);

    return value;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_ENTROPYBITPROB_CPP*/
