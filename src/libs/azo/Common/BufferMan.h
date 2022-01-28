#ifndef AZO_BUFFERMAN_H
#define AZO_BUFFERMAN_H

#include "../Common/AZOPrivate.h"

namespace AZO {

class BufferMan
{
public:
    BufferMan();
    ~BufferMan();

    bool Capacity(u_int size);
    u_int Add(const byte* buf, u_int size);
    u_int Get(byte* buf, u_int size);
    u_int Ahead(u_int size);
    u_int Remove(u_int size);
    void Clear() { Remove(size_); }

    byte* GetBufPtr();
    u_int GetSize()      { return size_; }
    u_int GetUseSize() { return wrPtr_-rdPtr_; }
    u_int GetRemainSize() { return size_ - GetUseSize(); }
  
private:
    byte*       buf_;
    u_int       rdPtr_;
    u_int       wrPtr_;
    u_int       size_;
};

} //namespaces AZO

#include "BufferMan.cpp"

#endif /*AZO_BUFFERMAN_H*/
