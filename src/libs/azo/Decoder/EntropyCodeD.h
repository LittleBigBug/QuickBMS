#ifndef AZO_DECODER_ENTROPYCODE_H
#define AZO_DECODER_ENTROPYCODE_H

#include "../Base/EntropyCodeB.h"
#include "BitCodeD.h"

namespace AZO {
namespace Decoder {

class EntropyCode : public Base::EntropyCode
{
    typedef Base::EntropyCode super;

public:
    EntropyCode(const byte* buf, u_int size) :
        bit_(buf, size), tag_(0), init_(false) {}

    ~EntropyCode() {}

    void Initialize();
    void Finalize();
    
    u_int Code(u_int totalBit);
    BOOL Code(u_int cumCount, u_int totalBit);

	u_int GetSize() { return (bit_.GetReadSize()+7)/8; }

private:
    void Rescale();

private:
	using super::MSB;
    using super::sMSB;
	using super::low_;
    using super::up_;

private:
    BitCode bit_;
    RANGE_TYPE tag_;
    bool init_;
};

} //namespaces Decoder
} //namespaces AZO

#include "EntropyCodeD.cpp"

#endif /*AZO_DECODER_ENTROPYCODE_H*/
