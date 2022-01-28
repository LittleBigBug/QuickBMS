#ifndef AZO_BASE_HISTORYLIST_CPP
#define AZO_BASE_HISTORYLIST_CPP

#include "HistoryListB.h"
#include <algorithm>

namespace AZO {
namespace Base {

template <typename T, u_int N>
HistoryList<T, N>::HistoryList(T init)
{
    for(T i=0; i<N; ++i) {
        rep_[i] = init + i;
    }
}

template <typename T, u_int N>
inline void HistoryList<T, N>::Add(const T& value)
{
    std::copy_backward(rep_, rep_+N-1, rep_+N);
    rep_[0] = value;
}

template <typename T, u_int N>
inline void HistoryList<T, N>::Add(const T& value, u_int delIdx)
{
    std::copy_backward(rep_, rep_+delIdx, rep_+delIdx+1);
    rep_[0] = value;
}

} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_HISTORYLIST_CPP*/
