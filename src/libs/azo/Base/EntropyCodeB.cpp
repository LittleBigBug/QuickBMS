#ifndef AZO_BASE_EntropyCode_CPP
#define AZO_BASE_EntropyCode_CPP

#include <limits>
#include "EntropyCodeB.h"

namespace AZO {
namespace Base {

inline EntropyCode::EntropyCode() : 
    low_(std::numeric_limits<RANGE_TYPE>::min()), 
    up_(std::numeric_limits<RANGE_TYPE>::max())
{
}

} //namespaces Base
} //namespaces AZO

#endif /*AZO_BASE_EntropyCode_CPP*/
