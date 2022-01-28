/*
EA JDLZ decompression 0.1
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ea_jdlz(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl = in + insz;
    unsigned char   *o = out;
    unsigned char   *outl = out + outsz;
    unsigned short  flags1 = 1, flags2 = 1;
    int     i, t, length;

    while((in < inl) && (o < outl)) {
        if(flags1 == 1) flags1 = *in++ | 0x100;
        if(flags2 == 1) flags2 = *in++ | 0x100;
        if(flags1 & 1) {
            if(flags2 & 1) {
                length = (in[1] | ((*in & 0xF0) << 4)) + 3;
                t = (*in & 0xF) + 1;
            } else {
                t = (in[1] | ((*in & 0xE0) << 3)) + 17;
                length = (*in & 0x1F) + 3;
            }
            in += 2;
            if((o - t) < out) return -1;
            for(i = 0; i < length; i++) {
                if((o + i + 1) > outl) break;
                o[i] = o[i - t];
            }
            o += i;
            flags2 >>= 1;
        } else {
            if(o < outl) *o++ = *in++;
        }
        flags1 >>= 1;
    }
    return o - out;
}

