#ifndef AZO_BASE_ENTROPYBITPROB_CPP
#define AZO_BASE_ENTROPYBITPROB_CPP

#include "EntropyBitProbB.h"

namespace AZO {
namespace Base {

template <u_int N>
EntropyBitProb<N>::EntropyBitProb()
{
    Init();
}

template <u_int N>
EntropyBitProb<N>::~EntropyBitProb()
{
}

template <u_int N>
void EntropyBitProb<N>::Init()
{
    std::fill_n(prob_, static_cast<size_t>(ARRAY_N), TOTAL_COUNT/2);    
}

template <u_int N>
inline uint32_t EntropyBitProb<N>::GetProb(u_int pre)
{
    return prob_[pre];
}

template <u_int N>
inline void EntropyBitProb<N>::IncreProb(u_int pre, BOOL value)
{
    ASSERT(pre < ARRAY_N);
    ASSERT(value < 2);

    const u_int SHIFT_BIT = TOTAL_BIT-6;

    uint32_t& p = prob_[pre];
    if(value == 0) {
        p += (TOTAL_COUNT-p)>>SHIFT_BIT;
    } else {
        p -= p>>SHIFT_BIT;
    }
}

template <u_int N>
inline void EntropyBitProb<N>::Update(u_int value)
{
    ASSERT(value < N);
    
    u_int pre(1);
    for(int i=BIT_N-1; i>=0; --i)
    {
        ASSERT(pre < ARRAY_N);

        const u_int v = ((value >> i) & 1);
        IncreProb(pre, v);        
        pre = (pre << 1) | v;
    }
}

template <u_int N>
int EntropyBitProb<N>::Compare(EntropyBitProb<N> e1, EntropyBitProb<N> e2, u_int value)
{
    uint32_t prob1(1);
    uint32_t prob2(1);

    u_int pre(1);
    for(int i=BIT_N-1; i>=0; --i)
    {
        const uint32_t p1 = e1.GetProb(pre);
        const uint32_t p2 = e2.GetProb(pre);
        const BOOL v = ((value >> i) & 1);

        if((prob1 | prob2) & (((1u<<TOTAL_BIT)-1) << (32-TOTAL_BIT)))
        {
            prob1 >>= TOTAL_BIT;
            prob2 >>= TOTAL_BIT;
            //ASSERT(prob1);
            //ASSERT(prob2);
        }

        prob1 *= (v ? (TOTAL_COUNT-p1) : p1);
        prob2 *= (v ? (TOTAL_COUNT-p2) : p2);
        
        pre = (pre << 1) | v;
    }

    if(prob1 > prob2) {
        return 1;
    } else if(prob1 < prob2) {
        return -1;
    } else {
        return 0;
    }
}


} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_ENTROPYBITPROB_CPP*/
