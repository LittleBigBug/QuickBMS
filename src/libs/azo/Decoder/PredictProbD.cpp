#ifndef AZO_DECODER_PREDICTPROB_CPP
#define AZO_DECODER_PREDICTPROB_CPP

#include "PredictProbD.h"

namespace AZO {
namespace Decoder {

template <u_int KEY, u_int N, u_int SHIFT>
inline u_int PredictProb<KEY, N, SHIFT>::Code(EntropyCode& entropy, u_int pre)
{
    ASSERT(pre < KEY);    
    
    u_int value(0);

    EntropyBitProb<N>& prob1 = prob1_[pre];
    EntropyBitProb<N>& prob2 = prob2_[pre>>SHIFT];

    if(lucky_[pre] >= 0) {
        value = prob1.Code(entropy);
        prob2.Update(value);
    } else {        
        value = prob2.Code(entropy);
        prob1.Update(value);
    }

    int r = EntropyBitProb<N>::Compare(prob1, prob2, value);
    if(r > 0) {
        ++lucky_[pre];
    } else if(r < 0) {
        --lucky_[pre];
    } else {
    }

    return value;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_PREDICTPROB_CPP*/
