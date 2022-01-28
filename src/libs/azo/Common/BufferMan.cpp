#ifndef AZO_BUFFERMAN_CPP
#define AZO_BUFFERMAN_CPP

#include <cstring>
#include <algorithm>
#include "BufferMan.h"
#include "Allocator.h"

namespace AZO {

inline BufferMan::BufferMan() : 
    buf_(NULL), rdPtr_(0), wrPtr_(0), size_(0)
{
}

inline BufferMan::~BufferMan()
{
    if(buf_) {
        Allocator::Free(buf_);        
    }
}

inline bool BufferMan::Capacity(u_int size)
{
    if(size_ < size)
    {
        byte* newBuf = static_cast<byte*>(Allocator::Alloc(size));
        if(newBuf == NULL) {
            return false;
        }

        if(GetUseSize()) {
            ASSERT(buf_);
            memcpy(newBuf, buf_, static_cast<size_t>(size_));
        }

        if(buf_) {
            Allocator::Free(buf_);            
        }

        buf_ = newBuf;
        size_ = size;
    }

    return true;
}

inline byte* BufferMan::GetBufPtr()
{
    ASSERT(wrPtr_ >= rdPtr_);

    if(rdPtr_ != 0) {
        std::memmove(buf_, buf_ + rdPtr_, static_cast<size_t>(GetUseSize()));
        wrPtr_ -= rdPtr_;
        rdPtr_ = 0;
    }

    return buf_;
}

inline u_int BufferMan::Add(const byte* buf, u_int size)
{
    u_int s = std::min(GetRemainSize(), size);

    if(size_ - wrPtr_ < size) {
        GetBufPtr();
    }

    std::memcpy(buf_+wrPtr_, buf, static_cast<size_t>(s));
    wrPtr_ += s;

    return s;
}

inline u_int BufferMan::Ahead(u_int size)
{
    ASSERT(size_ - wrPtr_ >= size);    
    wrPtr_ += size;

    return size;
}

inline u_int BufferMan::Get(byte* buf, u_int size)
{
    u_int s = std::min(GetUseSize(), size);

    std::memcpy(buf, buf_+rdPtr_, static_cast<size_t>(s));
    rdPtr_ += s;

    if(rdPtr_ == wrPtr_) {
        rdPtr_ = wrPtr_  = 0;
    }

    return s;
}

inline u_int BufferMan::Remove(u_int size)
{
    u_int s = std::min(GetUseSize(), size);
    
    rdPtr_ += s;

    if(rdPtr_ == wrPtr_) {
        rdPtr_ = wrPtr_  = 0;
    }

    return s;
}

} //namespaces AZO

#endif /*AZO_BUFFERMAN_CPP*/
