#ifndef AZO_BASE_PREDICTPROB_H
#define AZO_BASE_PREDICTPROB_H

#include "../Common/ProbCount.h"
#include "EntropyBitProbB.h"

namespace AZO {
namespace Base {

template <u_int KEY, u_int N, u_int SHIFT>
class PredictProb
{
public:
    PredictProb();

protected:
    //void IncreProb(EntropyBitProb<N>& prob1, EntropyBitProb<N>& prob2, u_int value);

protected:    
    int lucky_[KEY];    
};

} //namespaces Base
} //namespaces AZO

#include "PredictProbB.cpp"

#endif /*AZO_BASE_PREDICTPROB_H*/
