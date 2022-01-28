#ifndef AZO_BASE_DICTIONARYTABLE_CPP
#define AZO_BASE_DICTIONARYTABLE_CPP

#include <algorithm>
#include "DictionaryTableB.h"

namespace AZO {
namespace Base {

inline DictionaryTable::DictionaryTable(const byte* buf, u_int size) :
    buf_(buf), bufSize_(size)
{
    for(u_int i=0; i<N; ++i)
    {
        data_[i].len = MATCH_MIN_LENGTH + i;        
    }
}

inline void DictionaryTable::Update(u_int pos, u_int len)
{
    std::copy_backward(data_, data_+N-1, data_+N);
    data_[0].len = len;
    data_[0].pos = pos;
}

inline void DictionaryTable::Update(u_int pos, u_int len, u_int delIdx)
{
    ASSERT(delIdx < N);

    std::copy_backward(data_, data_+delIdx, data_+delIdx+1);
    data_[0].len = len;
    data_[0].pos = pos;
}

} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_DICTIONARYTABLE_CPP*/
