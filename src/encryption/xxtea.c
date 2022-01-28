// modified by Luigi Auriemma
// original code from http://en.wikipedia.org/wiki/XXTEA
#include "xxtea.h"
#include <string.h>

#define DELTA 0x9e3779b9
#define MX (((z>>5)^(y<<2)) + ((y>>3)^(z<<4))) ^ ((sum^y) + (k[(p&3)^e] ^ z));

#ifndef GET_ULONG_BE
#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ] << 24 )        \
	    | ( (unsigned long) (b)[(i) + 1] << 16 )        \
	    | ( (unsigned long) (b)[(i) + 2] <<  8 )        \
	    | ( (unsigned long) (b)[(i) + 3]       );       \
}
#endif

#ifndef PUT_ULONG_BE
#define PUT_ULONG_BE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

#ifndef GET_ULONG_LE
#define GET_ULONG_LE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ]       )        \
	    | ( (unsigned long) (b)[(i) + 1] <<  8 )        \
	    | ( (unsigned long) (b)[(i) + 2] << 16 )        \
	    | ( (unsigned long) (b)[(i) + 3] << 24 );       \
}
#endif

#ifndef PUT_ULONG_LE
#define PUT_ULONG_LE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n)       );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
}
#endif



void xxtea_setup(xxtea_context *ctx, unsigned char key[16], int custom, uint32_t delta, uint32_t endian, uint32_t cycles, int inverted_sign) {
    int i;

    if(!ctx) return;
    memset(ctx, 0, sizeof(xxtea_context));

    ctx->delta  = DELTA;
    ctx->endian = 1;
    ctx->cycles = 6;
    if(custom) {
        ctx->delta  = delta;
        ctx->endian = endian;
        ctx->cycles = cycles;
        ctx->inverted_sign = inverted_sign;
    }

    for( i = 0; i < 4; i++ ) {
        if(ctx->endian) {
            GET_ULONG_BE( ctx->k[i], key, i << 2 );
        } else {
            GET_ULONG_LE( ctx->k[i], key, i << 2 );
        }
    }
}



void xxtea_setup_delta(xxtea_context *ctx, uint32_t delta) {
    if(!ctx) return;
    ctx->delta = delta;
}



void xxtea_crypt(xxtea_context *ctx, int mode, unsigned char *data, int len) {
    uint32_t *v, *k, y, z, sum;
    unsigned p, rounds, e;
    int     n;

    if(!ctx) return;
    if(len <= 0) return;
    v = (uint32_t *)data;
    n = len / 4;

    e = 1;
    if(*(char *)&e) {   // swap
        for(e = 0; e < n; e++) {
            p = v[e];
            if(ctx->endian) {
                PUT_ULONG_BE(p, data, e << 2);
            } else {
                PUT_ULONG_LE(p, data, e << 2);
            }
        }
    }

    if(ctx->inverted_sign) ctx->delta = -ctx->delta;

    k = ctx->k;
    if(mode == XXTEA_ENCRYPT) {
        rounds = ctx->cycles + 52/n;
        sum = 0;
        z = v[n-1];
        do {
            sum += ctx->delta;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++) {
                y = v[p+1], z = v[p] += MX;
            }
            y = v[0];
            z = v[n-1] += MX;
        } while (--rounds);
    } else {
        rounds = ctx->cycles + 52/n;
        sum = rounds*ctx->delta;
        y = v[0];
        do {
            e = (sum >> 2) & 3;
            for (p=n-1; p>0; p--) {
                z = v[p-1], y = v[p] -= MX;
            }
            z = v[n-1];
            y = v[0] -= MX;
            sum -= ctx->delta;
        //} while(sum); // never enable this
        } while (--rounds);
    }

    if(ctx->inverted_sign) ctx->delta = -ctx->delta;

    if(*(char *)&e) {   // swap
        for(e = 0; e < n; e++) {
            p = v[e];
            if(ctx->endian) {
                PUT_ULONG_BE(p, data, e << 2);
            } else {
                PUT_ULONG_LE(p, data, e << 2);
            }
        }
    }
}


