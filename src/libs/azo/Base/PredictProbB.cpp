#ifndef AZO_BASE_PREDICTPROB_CPP
#define AZO_BASE_PREDICTPROB_CPP

#include <algorithm>
#include "PredictProbB.h"

namespace AZO {
namespace Base {

template <u_int KEY, u_int N, u_int SHIFT>
PredictProb<KEY, N, SHIFT>::PredictProb()
{    
    std::fill_n(lucky_, static_cast<size_t>(KEY), 0);
}

/*
template <u_int N, u_int N1, u_int N2>
void PredictProb<N, N1, N2>::IncreProb(EntropyBitProb<N>& prob1, EntropyBitProb<N>& prob2, u_int value)
{
    ASSERT(pre1 < N1);
    ASSERT(pre2 < N2);
    ASSERT(value < N);

    prob1.IncreProb(value);
    prob2.IncreProb(value);

    if(prob1.Prob(value) > prob2.Prob(value)) {
        ++lucky1_;
    } else if(prob1.Prob(value) < prob2.Prob(value)) {
        ++lucky2_;
    } else {
    }
}
*/

} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_PREDICTPROB_CPP*/
