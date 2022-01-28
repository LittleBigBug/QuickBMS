

#ifndef AZO_BASE_BoolState_H
#define AZO_BASE_BoolState_H

#include "../Common/AZOPrivate.h"

namespace AZO {
namespace Base {

template <u_int N>
class BoolState
{
public:
    BoolState();
    ~BoolState();

protected:
    void Update(BOOL b);

    uint32_t GetProb() {
        ASSERT(state_ < ARRAY_N);
        return prob_[state_];
    }

    uint32_t TotalBit() {
        return TOTAL_BIT;
    }

private:
    u_int state_;
    
    static const u_int ARRAY_N = 1<<N;
    static const u_int TOTAL_BIT = 12;
    static const u_int TOTAL_COUNT = 1<<TOTAL_BIT;

    uint32_t prob_[ARRAY_N];
};

} //namespaces Base
} //namespaces AZO

#ifndef TEMPLATES_NO_REQUIRE_SOURCE
#include "BoolStateB.cpp"
#endif /*TEMPLATES_NO_REQUIRE_SOURCE*/

#endif /*AZO_BASE_BoolState_H*/
