#ifndef AZO_DECODER_MainCode_H
#define AZO_DECODER_MainCode_H

#include "../Base/MainCodeB.h"

namespace AZO {
namespace Decoder {

class MainCode : public Base::MainCode
{
public: 
    MainCode();
    ~MainCode();

    int Code(const byte*& in, u_int& iuAvailSize, 
             byte*& out, u_int& outAvailSize);

private:
	template <typename T>
	void ReadNumber(T& num, const byte* out, u_int size)
    {
	    num = 0;
        for(u_int i=0; i<size; ++i) {
		    num += (*out++ << (size-i-1)*8);
        }
    }

    int ReadBlock(const byte* in, u_int inSize, byte* out, u_int outSize);

private:
    u_int compressSize_;
    bool setSizeInfo_;
};

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_MainCode_H*/
