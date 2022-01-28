#ifndef AZO_DECOEDR_HISTORYLIST_H
#define AZO_DECOEDR_HISTORYLIST_H

#include "../Base/HistoryListB.h"
#include "EntropyCodeD.h"
#include "BoolStateD.h"
#include "EntropyBitProbD.h"

namespace AZO {
namespace Decoder {

template <typename T, u_int N>
class HistoryList : public Base::HistoryList<T, N>
{
    typedef Base::HistoryList<T, N> super;
public:
    HistoryList(T init = 0) : super(init) {}

    bool Code(EntropyCode& entropy, T& value);

private:
    using super::rep_;

    BoolState<>     state_;
    EntropyBitProb<N>  prob_;
};

} //namespaces Decoder
} //namespaces AZO

#include "HistoryListD.cpp"

#endif /*AZO_DECOEDR_HISTORYLIST_H*/
