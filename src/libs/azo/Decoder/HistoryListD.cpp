#ifndef AZO_DECOEDR_HISTORYLIST_CPP
#define AZO_DECOEDR_HISTORYLIST_CPP

#include <algorithm>
#include "HistoryListD.h"

namespace AZO {
namespace Decoder {

template <typename T, u_int N>
inline bool HistoryList<T, N>::Code(EntropyCode& entropy, T& value)
{
    if(state_.Code(entropy))
    {
        u_int idx(0);
        idx = prob_.Code(entropy);
        ASSERT(idx < N);
        value = rep_[idx];

        this->Add(value, idx);

        return true;
    }
    else
    {
        return false;
    }
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECOEDR_HISTORYLIST_CPP*/
