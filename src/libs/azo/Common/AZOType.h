#ifndef AZO_AZOTYPE_H
#define AZO_AZOTYPE_H

#include <sys/types.h>

namespace AZO {

#ifdef _WIN32
typedef __int8              int8_t;
typedef __int16             int16_t;
typedef __int32             int32_t;
typedef __int64             int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;

typedef unsigned int        u_int;

#else
typedef char                int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

//typedef unsigned int        u_int;
#endif /*_WIN32*/

typedef uint8_t             byte;
typedef /*bool*/ u_int      BOOL; //for performance


template <int N> struct UINT {
};

template <> struct UINT<8> {
    typedef uint64_t type;
};

template <> struct UINT<4> {
    typedef uint32_t type;
};

typedef UINT<sizeof(void*)>::type  uint_t;

} //namespaces AZO

#endif /*AZO_AZOTYPE_H*/
