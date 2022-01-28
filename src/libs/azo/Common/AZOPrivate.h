#ifndef AZO_AZOPRIVATE_H
#define AZO_AZOPRIVATE_H

#include <cassert>
#include <cstdio>

#include "../AZO.h"
#include "AZOOption.h"
#include "AZOType.h"

namespace AZO {

#ifdef _DEBUG
#define ASSERT(a)   assert(a)
//#define ASSERT(a)	if(!(a)) { _asm { int 3 } }
#define TRACE(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#else
#define ASSERT(a)
#define TRACE(format, ...)
#endif /*_DEBUG*/

#define UNUSED_ARG(a) //do {} while(&a == 0)


template <uint64_t N>
struct StaticLog2 {
    static const size_t value = StaticLog2<N/2>::value + 1;
};

template <>
struct StaticLog2<1> {
    static const u_int value = 0;
};

template <>
struct StaticLog2<0> {
    static const u_int value = 0;
};

} //namespaces AZO

#endif /*AZO_AZOPRIVATE_H*/
