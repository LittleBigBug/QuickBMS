// modified by Luigi Auriemma
// layout from xtea.c and code from my halo_pck_algo.h

#include "tea.h"
#include <string.h>

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



void tea_setup(tea_context *ctx, unsigned char key[16], int custom, uint32_t delta, uint32_t sum, uint32_t endian, uint32_t cycles, int inverted_sign, int encrypt_mode) {
    int i;

    if(!ctx) return;
    memset(ctx, 0, sizeof(tea_context));

    ctx->delta  = 0x9e3779b9;
    ctx->sum    = encrypt_mode ? 0 : 0xc6ef3720;
    ctx->endian = 1;
    ctx->cycles = 32;
    if(custom) {
        ctx->delta  = delta;
        ctx->sum    = sum;
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



void tea_setup_delta(tea_context *ctx, uint32_t delta, uint32_t sum) {
    if(!ctx) return;
    ctx->delta = delta;
    ctx->sum   = sum;
}



void tea_crypt(tea_context *ctx, int mode, unsigned char input[8], unsigned char output[8]) {
    uint32_t *k, y, z, i, sum;

    if(!ctx) return;
    k = ctx->k;

    if(ctx->endian) {
        GET_ULONG_BE(y, input, 0);
        GET_ULONG_BE(z, input, 4);
    } else {
        GET_ULONG_LE(y, input, 0);
        GET_ULONG_LE(z, input, 4);
    }

    if(ctx->inverted_sign) ctx->delta = -ctx->delta;

    if(mode == TEA_ENCRYPT) {
        sum = 0;
        for(i = 0; i < ctx->cycles; i++) {
            sum += ctx->delta;
            y += ((z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
            z += ((y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
        }
    } else {
        sum = ctx->sum;
        for(i = 0; i < ctx->cycles; i++) {
            z -= ((y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
            y -= ((z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
            sum -= ctx->delta;
        }
    }

    if(ctx->inverted_sign) ctx->delta = -ctx->delta;

    if(ctx->endian) {
        PUT_ULONG_BE(y, output, 0);
        PUT_ULONG_BE(z, output, 4);
    } else {
        PUT_ULONG_LE(y, output, 0);
        PUT_ULONG_LE(z, output, 4);
    }
}


