/*
EA COMP decompression 0.1
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ea_comp(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl = in + insz;
    unsigned char   c, *back_ptr;
    unsigned int    cycles;
    unsigned int    flags  = 1;
    int             i;
    unsigned char   *o = out;
    unsigned char   *outl = out + outsz;

    while((in < inl) && (o < outl)) {
        if(flags == 1) {
            flags = (*in | (in[1] << 8)) | 0x10000;
            in += 2;
        }
        for(cycles = ((inl - 32) < in) ? 1 : 16; cycles; cycles--) {
            if(flags & 1) {
                c = *in;
                back_ptr = o - (in[1] | ((unsigned char)(c & 0xF0) << 4));
                in += 2;
                for(i = (c & 0xF) + 3; i; i--) {
                    if((o + 1) > outl) break;
                    *o++ = *back_ptr++;
                }
            } else {
                if(o < outl) *o++ = *in++;
            }
            flags >>= 1;
        }
    }
    return o - out;
}

