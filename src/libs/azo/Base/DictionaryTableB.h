#ifndef AZO_BASE_DICTIONARYTABLE_H
#define AZO_BASE_DICTIONARYTABLE_H

#include "../Common/AZOPrivate.h"

namespace AZO {
namespace Base {


class DictionaryTable
{
public:
    DictionaryTable(const byte* buf, u_int size);

    void Update(u_int pos, u_int len);
    void Update(u_int pos, u_int len, u_int delIdx);

protected:
    const byte* buf_;
    u_int bufSize_;

protected:
    static const u_int N = DICTIONARY_SIZE;

    struct Data {
        u_int pos;
        u_int len;

        Data() : pos(0), len(0) {}
    };

    Data data_[N];
};

} //namespaces Base
} //namespaces AZO

#include "DictionaryTableB.cpp"

#endif /*AZO_BASE_DICTIONARYTABLE_H*/
