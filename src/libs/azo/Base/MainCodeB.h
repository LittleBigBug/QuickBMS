#ifndef AZO_BASE_MAINCODE_H
#define AZO_BASE_MAINCODE_H

#include "../Common/AZOPrivate.h"
#include "../Common/BufferMan.h"

namespace AZO {
namespace Base {

class MainCode
{
protected:
    static const u_int MAIN_HEAD_SIZE = 2;
    static const u_int BLOCK_SIZE_SIZE = 4;
    static const u_int BLOCK_HEAD_SIZE = BLOCK_SIZE_SIZE*3;

public:
    MainCode();
    ~MainCode();

protected:
    void AllocInBuffer(u_int n);
    void AllocOutBuffer(u_int n);

    const byte* GetInBuffer(const byte*& in, u_int& inAvailSize, u_int needSize);
    byte* GetInBuffer2(const byte*& in, u_int& inAvailSize, u_int needSize);
    void RemoveInBuffer(const byte*& in, u_int& inAvailSize, u_int needSize);    

    byte* GetOutBuffer(byte*& out, u_int& outAvailSize, u_int needSize);
    void RemoveOutBuffer(byte*& out, u_int& outAvailSize, u_int needSize);

    u_int InBufferUseSize() { return inBuf_.GetUseSize(); }
    u_int OutBufferUseSize() { return outBuf_.GetUseSize(); }

private:
    BufferMan inBuf_;
    BufferMan outBuf_;

protected:
    u_int blockSize_;

    void* coderObj_;
    bool init_;
    bool finish_;
    bool useFilter_;
};

} //namespaces Base
} //namespaces AZO


#include "MainCodeB.cpp"

#endif /*AZO_BASE_MAINCODE_H*/
