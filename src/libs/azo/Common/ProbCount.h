#ifndef AZO_PROBCOUNT_H
#define AZO_PROBCOUNT_H

#include "../Common/AZOPrivate.h"

namespace AZO {

template <u_int N>
struct ProbCount
{
    ProbCount(int init = 0);

    uint32_t hit[N];
    uint32_t total;

private:
    template <typename T>
    void IncreProb(const T& value, int count = 1);
};

} //namespaces AZO

#include "ProbCount.cpp"

#endif /*AZO_PROBCOUNT_H*/
