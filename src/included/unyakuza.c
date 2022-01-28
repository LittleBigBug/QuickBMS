/*
Simple LZSS used in SEGA 0.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

Used in Yakuza 3 and Binary Domain.

    bytes   description
    4       SLLZ
    1       0 for little endian, 1 for big
    1       ???
    2       0x10
    4       uncompressed size
    4       compressed size
    x       compressed data
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define unyakuza_bswap32(n) \
        (((n & 0xff000000) >> 24) | \
         ((n & 0x00ff0000) >>  8) | \
         ((n & 0x0000ff00) <<  8) | \
         ((n & 0x000000ff) << 24))



int unyakuza(unsigned char *in, int insz, unsigned char *out, int outsz, int check_head) {
    typedef struct {
        unsigned char   sign[4];
        unsigned char   endian;
        unsigned char   dummy;
        unsigned short  zoff;   // type or data offset?
        unsigned int    size;
        unsigned int    zsize;
    } yakuza_t;
    yakuza_t        *yakuza;
    int             i,
                    d,
                    a,
                    b,
                    op;
    unsigned char   *p  = in,
                    *o  = out,
                    *il = in + insz,
                    *ol = out + outsz;

    if(!in || (insz < 0) || !out || (outsz < 0)) return(-1);

    if(check_head) {
        if(insz < sizeof(yakuza_t)) return(-2);
        yakuza = (yakuza_t *)p;
        if(memcmp(yakuza->sign, "SLLZ", 4)) return(-3);
        if(yakuza->endian) {
            yakuza->zoff  = (yakuza->zoff >> 8) | (yakuza->zoff << 8);
            yakuza->size  = unyakuza_bswap32(yakuza->size);
            yakuza->zsize = unyakuza_bswap32(yakuza->zsize);
        }
        if(yakuza->zoff != 0x10) return(-4);
        if(yakuza->size > outsz) return(-5);
        if(yakuza->zsize > insz) return(-6);
        p += 0x10;
    }

    b = *p++;
    a = 8;
    while(o < ol) {
        if(p >= il) return(-7);

        if(b & 0x80) op = 1;
        else         op = 0;

        b <<= 1;
        a--;
        if(!a) {
            b = *p++;
            a = 8;
        }

        if(op) {
            d = ((p[0] >> 4) | (p[1] << 4)) + 1;
            for(i = (p[0] & 15) + 3; i > 0; i--) {
                if(o >= ol) break;
                *o = *(o - d);
                o++;
            }
            p += 2;
        } else {
            if(o >= ol) break;
            *o++ = *p++;
        }
    }
    return(o - out);
}

