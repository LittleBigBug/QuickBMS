#ifndef AZO_Base_MainCode_CPP
#define AZO_Base_MainCode_CPP

#include "MainCodeB.h"

namespace AZO {
namespace Base {

inline MainCode::MainCode(): 
    blockSize_(0), coderObj_(NULL), 
    init_(false), finish_(false), useFilter_(false)
{
}

inline MainCode::~MainCode()
{    
}


inline void MainCode::AllocInBuffer(u_int n)
{ 
    inBuf_.Capacity(n);
}

inline void MainCode::AllocOutBuffer(u_int n)
{
    outBuf_.Capacity(n);
}

inline const byte* MainCode::GetInBuffer(const byte*& in, u_int& inAvailSize, u_int needSize)
{
    if(inBuf_.GetUseSize() >= needSize) {
        return inBuf_.GetBufPtr();
    }

    if(inBuf_.GetUseSize() == 0 && inAvailSize >= needSize) {
        return in;
    }

    inBuf_.Capacity(needSize);

    u_int size = inBuf_.Add(in, inAvailSize);

    inAvailSize -= size;
    in += size;

    if(inBuf_.GetUseSize() >= needSize) {
        return inBuf_.GetBufPtr();
    } else {
        return NULL;
    }
}

inline byte* MainCode::GetInBuffer2(const byte*& in, u_int& inAvailSize, u_int needSize)
{
    inBuf_.Capacity(needSize);

    u_int size = inBuf_.Add(in, inAvailSize);

    inAvailSize -= size;
    in += size;

    if(inBuf_.GetUseSize() >= needSize) {
        return inBuf_.GetBufPtr();
    } else {
        return NULL;
    }
}

inline void MainCode::RemoveInBuffer(const byte*& in, u_int& inAvailSize, u_int useSize)
{
    if(inBuf_.GetUseSize() < useSize)
    {
        in += useSize;
        inAvailSize -= useSize;
    }
    else
    {
        inBuf_.Remove(useSize);
    }
}

inline byte* MainCode::GetOutBuffer(byte*& out, u_int& outAvailSize, u_int needSize)
{
    if(outBuf_.GetUseSize())
    {        
        u_int size = outBuf_.Get(out, outAvailSize);
       
        out += size;
        outAvailSize -= size;
    }

    if(outBuf_.GetUseSize()) {
        return NULL;
    }

    if(outAvailSize >= outBuf_.GetSize())
    {
        ASSERT(outBuf_.GetUseSize() == 0);

        if(outAvailSize >= needSize) {
            return out;
        }
    }

    outBuf_.Capacity(needSize);
    ASSERT(outBuf_.GetRemainSize() >= needSize);

    return outBuf_.GetBufPtr();
}

inline void MainCode::RemoveOutBuffer(byte*& out, u_int& outAvailSize, u_int useSize)
{
    if(outAvailSize >= outBuf_.GetSize()) {
        ASSERT(outBuf_.GetUseSize() == 0);

        out += useSize;
        outAvailSize -= useSize;
    } else {
        outBuf_.Ahead(useSize);

        u_int size = outBuf_.Get(out, outAvailSize);
       
        outAvailSize -= size;
        out += size;
    }
}

} //namespaces Base
} //namespaces AZO

#endif /*AZO_Base_MainCode_CPP*/
