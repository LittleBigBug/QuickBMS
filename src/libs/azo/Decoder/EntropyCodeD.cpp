#ifndef AZO_DECODER_ENTROPYCODE_CPP
#define AZO_DECODER_ENTROPYCODE_CPP

#include "EntropyCodeD.h"

namespace AZO {
namespace Decoder {


inline void EntropyCode::Initialize()
{
    ASSERT(init_ == false);

	//bit_.Code(tag_, std::numeric_limits<RANGE_TYPE>::digits);
	const size_t s = sizeof(RANGE_TYPE)/sizeof(byte);
	for(size_t i=0; i<s; ++i)
	{
		RANGE_TYPE b(0);
		bit_.Code(b, 8);
		tag_ |= b<<((s-1-i)*8);
	}

    init_ = true;
}


inline void EntropyCode::Rescale()
{
    while((low_ & MSB) == (up_ & MSB))
    {
        bool b(0);
        bit_.Code(b);
        tag_ <<= 1; tag_ |= static_cast<RANGE_TYPE>(b);

        low_ <<= 1;
        up_ <<= 1; up_ |= 1;
    }

    while((low_ & sMSB) && (up_ & sMSB) == 0)
	{
        bool b(0);
        bit_.Code(b);
        tag_ <<= 1; tag_ |= static_cast<RANGE_TYPE>(b);
        tag_ ^= MSB;

        low_ <<= 1;
        low_ &= (MSB-1);
        up_ <<= 1; up_ |= 1;
        up_ |= MSB;
    }
}

inline u_int EntropyCode::Code(u_int totalBit)
{
    ASSERT(init_);
    ASSERT(low_ < up_);

    RANGE_TYPE t(0);
    if(low_ == std::numeric_limits<RANGE_TYPE>::min() && 
        up_ == std::numeric_limits<RANGE_TYPE>::max()) { 
        t = ONE << (std::numeric_limits<RANGE_TYPE>::digits - totalBit);
    } else {
        t = (up_ - low_ + 1) >> totalBit;
    }

    const RANGE_TYPE value = (tag_ - low_) / t;    

    up_ = low_ + t * (value+1) - 1;
    low_ = low_ + t * value;

    Rescale();

    return static_cast<u_int>(value);
}

inline BOOL EntropyCode::Code(u_int cumCount, u_int totalBit)
{
    ASSERT(init_);

    RANGE_TYPE t(0);
    if(low_ == std::numeric_limits<RANGE_TYPE>::min() && 
        up_ == std::numeric_limits<RANGE_TYPE>::max()) { 
        t = ONE << (std::numeric_limits<RANGE_TYPE>::digits - totalBit);
    } else {
        t = (up_ - low_ + 1) >> totalBit;
    }

    const RANGE_TYPE v = (tag_ - low_) / t;

    if(v >= cumCount) {
        low_ += t * static_cast<RANGE_TYPE>(cumCount);
    } else {
        up_ = low_ + t * static_cast<RANGE_TYPE>(cumCount) - 1;     
    }
    ASSERT(low_ < up_);

    Rescale();

    return v >= cumCount ? 1:0;
}

inline void EntropyCode::Finalize()
{
	//TRACE("read byte: %d (%d)", (bit_.GetReadSize()+7)/8, bit_.GetReadSize());
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_ENTROPYCODE_CPP*/
