#ifndef AZO_DECODER_ENTROPYBITPROB_H
#define AZO_DECODER_ENTROPYBITPROB_H

#include "../Base/EntropyBitProbB.h"
#include "EntropyCodeD.h"

namespace AZO {
namespace Decoder {

template <u_int N>
class EntropyBitProb : public Base::EntropyBitProb<N>
{
    typedef Base::EntropyBitProb<N> super;
public:
    EntropyBitProb() {}
    ~EntropyBitProb() {}
    
    using super::Init;
    using super::Update;
    u_int Code(EntropyCode& entropy);

private:    
    using super::GetProb;
 
private:
    using super::BIT_N;
    using super::TOTAL_BIT;
};

} //namespaces Decoder
} //namespaces AZO

#include "EntropyBitProbD.cpp"

#endif /*AZO_DECODER_ENTROPYBITPROB_H*/
