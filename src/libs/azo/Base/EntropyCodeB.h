#ifndef AZO_BASE_ENTROPYCODE_H
#define AZO_BASE_ENTROPYCODE_H

#include <limits>
#include "../Common/AZOPrivate.h"

namespace AZO {
namespace Base {

class EntropyCode
{
protected:
    typedef uint32_t    RANGE_TYPE;

    static const RANGE_TYPE ONE = static_cast<RANGE_TYPE>(1);
    static const RANGE_TYPE MSB = ONE << (std::numeric_limits<RANGE_TYPE>::digits-1);
    static const RANGE_TYPE sMSB = ONE << (std::numeric_limits<RANGE_TYPE>::digits-2);

    EntropyCode();
    
protected:    
    RANGE_TYPE low_;
    RANGE_TYPE up_;
};

} //namespaces Base
} //namespaces AZO

#include "EntropyCodeB.cpp"

#endif /*AZO_BASE_ENTROPYCODE_H*/
