#ifndef AZO_X86FILTER_CPP
#define AZO_X86FILTER_CPP

#include "x86Filter.h"

#include <iostream>

namespace AZO {

const byte CALL (0xE8);
const byte JMP  (0xE9);

inline void NumToByte4(const u_int n, byte* b)
{
    b[3] = static_cast<byte>(n >> 24);
    b[2] = static_cast<byte>(n >> 16);
    b[1] = static_cast<byte>(n >> 8);
    b[0] = static_cast<byte>(n);
}

inline u_int Byte4ToNum(const byte* b)
{
    return (static_cast<u_int>(b[3]) << 24) | 
           (static_cast<u_int>(b[2]) << 16) | 
           (static_cast<u_int>(b[1]) << 8) | 
           (static_cast<u_int>(b[0]));
}

void x86Filter(byte* buf, u_int size, bool encode)
{
    if(size < 5)
        return;

    for(u_int i=0; i<size-4; )
    {
        if((buf[i] == CALL) || (buf[i] == JMP))
        {
            if(buf[i+4] == 0x00 || buf[i+4] == 0xFF)
            {
                u_int addr = Byte4ToNum(buf+i+1);

                if(encode) {
                    addr += i;
                } else {
                    addr -= i;
                }

                if((addr >> 24) & 1)
                    addr |= 0xFF<<24;
                else
                    addr &= (1<<24)-1;

                NumToByte4(addr, buf+i+1);
            }
            
            i += 5;
        } else {
            ++i;
        }
    }
}

} //namespaces AZO

#endif /*AZO_X86FILTER_CPP*/
