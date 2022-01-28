#ifndef AZO_BASE_HISTORYLIST_H
#define AZO_BASE_HISTORYLIST_H

#include "../Common/AZOPrivate.h"

namespace AZO {
namespace Base {

template <typename T, u_int N>
class HistoryList
{
public:
    HistoryList(T init);

    u_int Size() { return N; }

    void Add(const T& value);
    void Add(const T& value, u_int delIdx);

protected:
    T rep_[N];
};

} //namespaces Base
} //namespaces AZO

#include "HistoryListB.cpp"

#endif /*AZO_BASE_HISTORYLIST_H*/
