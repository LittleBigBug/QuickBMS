#ifndef AZO_DECODER_PREDICTPROB_H
#define AZO_DECODER_PREDICTPROB_H

#include "../Base/PredictProbB.h"
#include "EntropyBitProbD.h"

namespace AZO {
namespace Decoder {

template <u_int KEY, u_int N, u_int SHIFT>
class PredictProb : public Base::PredictProb<KEY, N, SHIFT>
{
    typedef Base::PredictProb<KEY, N, SHIFT> super;
public:
    PredictProb() {}
    
    u_int Code(EntropyCode& entropy, u_int pre);

private:
    EntropyBitProb<N> prob1_[KEY];
    EntropyBitProb<N> prob2_[KEY>>SHIFT];
    using super::lucky_;
};

} //namespaces Decoder
} //namespaces AZO

#include "PredictProbD.cpp"

#endif /*AZO_DECODER_PREDICTPROB_H*/
