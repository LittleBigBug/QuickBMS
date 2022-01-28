#ifndef AZO_BASE_ENTROPYBITPROB_H
#define AZO_BASE_ENTROPYBITPROB_H

#include "../Common/ProbCount.h"

namespace AZO {
namespace Base {

template <u_int N>
class EntropyBitProb
{
public:
    EntropyBitProb();
    ~EntropyBitProb();

public:
    void Init();
    void Update(u_int value);    

protected:
    uint32_t GetProb(u_int pre);
    void IncreProb(u_int pre, BOOL value);

protected:
    static const u_int BIT_N = StaticLog2<N-1>::value + 1;
    static const u_int ARRAY_N = 1<<BIT_N;
    static const u_int TOTAL_BIT = 10;
    static const uint32_t TOTAL_COUNT = 1<<TOTAL_BIT;

private:
    uint32_t prob_[ARRAY_N];

public:    
    static int Compare(EntropyBitProb<N> e1, EntropyBitProb<N> e2, u_int value);
};

} //namespaces Base
} //namespaces AZO

#include "EntropyBitProbB.cpp"

#endif /*AZO_BASE_ENTROPYBITPROB_H*/
