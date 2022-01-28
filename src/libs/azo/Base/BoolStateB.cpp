#ifndef AZO_BASE_BoolState_CPP
#define AZO_BASE_BoolState_CPP

#include "BoolStateB.h"

namespace AZO {
namespace Base {

template <u_int N>
BoolState<N>::BoolState() : state_(0)
{
    for(u_int i=0; i<ARRAY_N; ++i) {
        prob_[i] = TOTAL_COUNT/2;
    }
}

template <u_int N>
BoolState<N>::~BoolState()
{
}

template <u_int N>
inline void BoolState<N>::Update(BOOL b)
{
    ASSERT(state_ < ARRAY_N);
    
    const u_int SHIFT_BIT = TOTAL_BIT-6;
    uint32_t& p = prob_[state_];

    if(b == 0) {
        p += (TOTAL_COUNT-p)>>SHIFT_BIT;
    } else {
        p -= p>>SHIFT_BIT;
    }

    state_ = ((state_ << 1) & (ARRAY_N-1)) | b;
}

} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_BoolState_CPP*/
