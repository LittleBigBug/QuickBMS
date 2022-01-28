// just a copy of the functions available in file.c
// to be used with external code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mybits.h"



void mybits_init(mybits_ctx_t *ctx, uint8_t *data, int size, FILE *fd) {
    if(!ctx) return;
    memset(ctx, 0, sizeof(mybits_ctx_t));
    ctx->bitchr         = 0;
    ctx->bitpos         = 0;
    if(fd) {
        ctx->fd         = fd;
    } else {
        ctx->data       = data;
        ctx->data_start = data;
        ctx->data_limit = data + size;
    }
    ctx->eof            = 0;
    ctx->force_endian   = -1;
}



uint64_t mybits_read(mybits_ctx_t *ctx, uint32_t bits, int endian) {
    uint64_t    num = 0;
    int         i,
                t;
    uint8_t     bc  = 0,
                bp  = 0;

    if(!ctx) return 0;
    if(bits > (sizeof(num) * 8)) return 0;
    if(ctx->force_endian >= 0) endian = ctx->force_endian;
    bc = ctx->bitchr;
    bp = ctx->bitpos;
    (bp) &= 7; // just for security
    for(i = 0; i < bits; i++) {
        if(!bp) {
            if(ctx->fd) {
                t = fgetc(ctx->fd);
                if(t < 0) { ctx->eof = 1; break; }
            } else {
                if(ctx->data >= ctx->data_limit) { ctx->eof = 1; break; }
                t = *ctx->data++;
            }
            bc = (t < 0) ? 0 : t;
        }
        if(!endian) {
            num = (num >> (uint64_t)1) | (uint64_t)((((uint64_t)bc >> (uint64_t)bp) & (uint64_t)1) << (uint64_t)(bits - 1));
        } else {
            num = (num << (uint64_t)1) | (uint64_t)((((uint64_t)bc << (uint64_t)bp) >> (uint64_t)7) & (uint64_t)1);
        }
        (bp)++;
        (bp) &= 7; // leave it here
    }
    ctx->bitchr = bc;
    ctx->bitpos = bp;
    return num;
}



int mybits_write(mybits_ctx_t *ctx, uint64_t num, uint32_t bits, int endian) {
    int         i,
                t,
                bit,
                rem = 0;
    uint8_t     bc  = 0,
                bp  = 0;

    if(!ctx) return 0;
    if(bits > (sizeof(num) * 8)) return 0;
    if(ctx->force_endian >= 0) endian = ctx->force_endian;
    bc = ctx->bitchr;
    bp = ctx->bitpos;
    (bp) &= 7; // just for security
    for(i = 0; i < bits; i++) {
        if(!bp) {
            if(rem) {
                if(ctx->fd) {
                    if(fseek(ctx->fd, -1, SEEK_CUR) < 0) { ctx->eof = 1; break; }
                    if(fputc(bc, ctx->fd) < 0) { ctx->eof = 1; break; }
                } else {
                    if(ctx->data <= ctx->data_start) { ctx->eof = 1; break; }
                    ctx->data[-1] = bc;
                }
                rem = 0;
            }
            if(ctx->fd) {
                t = fgetc(ctx->fd);
                if(t < 0) { ctx->eof = 1; break; }
            } else {
                if(ctx->data >= ctx->data_limit) { ctx->eof = 1; break; }
                t = *ctx->data++;
            }
            if(t < 0) {
                bc = 0;
                if(ctx->fd) {
                    if(fputc(bc, ctx->fd) < 0) { ctx->eof = 1; break; }
                } else {
                    if(ctx->data >= ctx->data_limit) { ctx->eof = 1; break; }
                    *ctx->data++ = bc;
                }
            } else {
                bc = t;
            }
        }
        if(!endian) { // uhmmm I don't think it's very fast... but works
            t = (uint64_t)1 << (uint64_t)bp;
            bit = (num >> (uint64_t)i) & (uint64_t)1;
        } else {
            t = (uint64_t)1 << (uint64_t)(7 - bp);
            bit = (num >> (uint64_t)((bits - i) - 1)) & 1;
        }
        if(bit) {
            bc |= t;   // put 1
        } else {
            bc &= ~t;  // put 0
        }
        (bp)++;
        (bp) &= 7; // leave it here
        rem++;
    }
    if(rem) {
        if(ctx->fd) {
            if(fseek(ctx->fd, -1, SEEK_CUR) < 0) { ctx->eof = 1; }
            if(fputc(bc, ctx->fd) < 0) { ctx->eof = 1; }
        } else {
            if(ctx->data <= ctx->data_start) { ctx->eof = 1; }
            else ctx->data[-1] = bc;
        }
    }
    ctx->bitchr = bc;
    ctx->bitpos = bp;
    return i;
}



void mybits_flush(mybits_ctx_t *ctx) {
    if(!ctx) return;
    ctx->bitchr = 0;
    ctx->bitpos = 0;
}


