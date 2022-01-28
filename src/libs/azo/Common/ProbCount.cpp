#ifndef AZO_PROBCOUNT_CPP
#define AZO_PROBCOUNT_CPP

#include "ProbCount.h"
#include <algorithm>

namespace AZO {

template <u_int N>
inline ProbCount<N>::ProbCount(int init)
{
    std::fill_n(hit, N, init);
    total = init * N;
}

template <u_int N>
template <typename T>
inline void ProbCount<N>::IncreProb(const T& value, int count)
{
    ASSERT(value < N);

    hit[value] += count;
    total += count;
}

} //namespaces AZO

#endif /*AZO_PROBCOUNT_CPP*/
