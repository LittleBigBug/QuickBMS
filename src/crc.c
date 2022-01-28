/*
    Copyright 2013-2021 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/



typedef struct {
    u64     table[256];
    u64     poly;
    int     bits;
    u64     init;
    u64     final;
    int     type;
    int     rever;
    int     bitmask_side;
} crc_context;



static u64 crc_bitmask(int bits, int mask) {
    u64     ret;

         if(bits < 0)   ret = (u64)0;
    else if(bits >= 64) ret = (u64)0;
    else                ret = (u64)1 << (u64)bits;
    if(mask) ret--;
    return ret;
}



static u64 crc_reflect(u64 v, int b) {
    u64     ret;
    int     i;

    ret = (u64)0;
    for(i = 0; i < b; i++) {
        ret = (u64)(ret << (u64)1) | (u64)(v & (u64)1);
        v >>= (u64)1;
    }
    return ret;
}



static u64 crc_safe_limit(u64 crc, int bits) {
    if(!bits) return crc;
    if(bits < 64) {
        crc &= crc_bitmask(bits, 1);
    }
    return(crc);
}



static u64 crc_cm_tab(int inbyte, u64 poly, int bits, int rever, int bitmask_side) {
    u64     r,
            topbit;
    int     i;

    if(bitmask_side > 0) topbit = 1;
    else                 topbit = crc_bitmask(bits - 1, 0);

    if(rever) inbyte = crc_reflect(inbyte, 8);

    if(bitmask_side > 0) r = (u64)inbyte;
    else                 r = (u64)inbyte << (u64)(bits - 8);

    for(i = 0; i < 8; i++) {
        r = crc_safe_limit(r, bits);
        if(r & topbit) {
            if(bitmask_side > 0) r = (r >> (u64)1) ^ poly;
            else                 r = (r << (u64)1) ^ poly;
        } else {
            if(bitmask_side > 0) r = (r >> (u64)1);
            else                 r = (r << (u64)1);
        }
    }

    if(rever) r = crc_reflect(r, bits);

    r &= crc_bitmask(bits, 1);
    return(r);
}



u8 *crc_make_table(u8 *op, int *oplen, u64 poly, int bits, int endian, int rever, int bitmask_side, void *(*add_func)()) {
    u64     num,
            *p64;
    int     i,
            len;

    if(oplen) len = *oplen;
    else      len = 0;

    if(!op) {
        len = 0;
        if(!add_func) {
            op = calloc(256, sizeof(num));
            if(!op) return NULL;
        }
    }
    p64 = (void *)op;

    for(i = 0; i < 256; i++) {
        num = crc_cm_tab(i, poly, bits, rever, bitmask_side);
        num = crc_safe_limit(num, bits);
        if(add_func) {
            op = add_func(op, &len, num, bits, endian);
        } else {
            p64[i] = num;
        }
    }

    if(oplen) *oplen = len;
    return(op);
}



u16 crc_in_cksum(u64 init, u8 *data, int len) {
    u64     sum;
    int     endian = 1; // big endian
    u16     crc,
            *p,
            *l;

    if(*(char *)&endian) endian = 0;
    sum = init;

    for(p = (u16 *)data, l = p + (len >> 1); p < l; p++) sum += *p;
    if(len & 1) sum += *p & (endian ? 0xff00 : 0xff);
    sum = (sum >> 16) + (sum & 0xffff);
    crc = sum + (sum >> 16);
    if(!endian) crc = (crc >> 8) | (crc << 8);
    return(crc);    // should be xored with 0xffff but this job is done later
}



u32 fnv32(unsigned char *data, int len, u32 hash) {
    int     i;
    for(i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x01000193;
    }
    return hash;
}



u32 UHash(unsigned char *data, int len, u32 hash, u32 poly, u32 TableSize) {
    u32     rand1 = 31415;
    u32     rand2 = 27183;
    if(poly) rand1 = poly;

    int     i;
    for(i = 0; i < len; i++) {
        hash *= rand1;
        hash = (hash + data[i]) % TableSize;
        rand1 = (rand1 * rand2) % (TableSize - 1);
    }
    return hash;
}



#include "libs/crc/murmurhash.c"
#include "libs/crc/more_crc.c"
#include "libs/spookyhash/spookyhash.h"
uint64 CityHash64(const char *buf, size_t len);
uint64 CityHash64WithSeed(const char *buf, size_t len, uint64 seed);
uint64 CityHash64WithSeeds(const char *buf, size_t len, uint64 seed0, uint64 seed1);
uint32 CityHash32(const char *buf, size_t len);
#include "libs/xxhash/xxhash.h"
/*
unsigned int       XXH32 (const void* input, size_t length, unsigned seed);
unsigned long long XXH64 (const void* input, size_t length, unsigned long long seed);
unsigned long long XXH3_64bits (const void* input, size_t length);
unsigned long long XXH3_64bits_withSeed (const void* input, size_t length, unsigned long long seed);
*/



QUICKBMS_u_int rol(QUICKBMS_u_int n1, QUICKBMS_u_int n2, QUICKBMS_u_int bits);
QUICKBMS_u_int ror(QUICKBMS_u_int n1, QUICKBMS_u_int n2, QUICKBMS_u_int bits);



u64 crc_calc(crc_context *ctx, u8 *data, int datalen, i32 *ret_error) {
    #define MYCRC   (crc_safe_limit(crc, ctx->bits))
    #define MYBYTE  data[i]
    #define CRC_CALC_CYCLE(X) { \
        for(i = 0; i < datalen; i++) { \
            crc = X; \
        } \
    }
    u64     crc;
    int     i;

    if(ret_error) *ret_error = 0;
    crc = ctx->init;    // Init
        // MYCRC is the current CRC/HASH
        // MYBYTE is the byte taken from the input data
         if(ctx->type == 0)  CRC_CALC_CYCLE(    ctx->table[(MYBYTE ^ MYCRC) & 0xff] ^ (MYCRC >> 8))
    else if(ctx->type == 1)  CRC_CALC_CYCLE(    ctx->table[(MYBYTE ^ (MYCRC >> (ctx->bits - 8))) & 0xff] ^ (MYCRC << 8))
    else if(ctx->type == 2)  CRC_CALC_CYCLE(    ((MYCRC << 8) | MYBYTE) ^ ctx->table[(MYCRC >> (ctx->bits - 8)) & 0xff])
    else if(ctx->type == 3)  CRC_CALC_CYCLE(    ((MYCRC >> 1) + ((MYCRC & 1) << (ctx->bits - 1))) + MYBYTE)
    else if(ctx->type == 4)  crc =              crc_in_cksum(MYCRC, data, datalen);
    else if(ctx->type == 5)  CRC_CALC_CYCLE(    MYCRC ^ MYBYTE)
    else if(ctx->type == 6)  CRC_CALC_CYCLE(    MYCRC + MYBYTE)    // lose lose
    else if(ctx->type == 7)  CRC_CALC_CYCLE(    ctx->table[(MYBYTE ^ MYCRC) & 0xff] ^ MYCRC)
    else if(ctx->type == 8)  CRC_CALC_CYCLE(    ctx->table[(MYBYTE ^ MYCRC) & 0xff] ^ (MYCRC >> (ctx->bits - 8)))
    else if(ctx->type == 9)  CRC_CALC_CYCLE(    (MYCRC << 1)  ^ MYBYTE)
    else if(ctx->type == 10) CRC_CALC_CYCLE(    (MYCRC << 1)  + MYBYTE)
    else if(ctx->type == 11) CRC_CALC_CYCLE(    rol(MYCRC, 1, 0) ^ MYBYTE)
    else if(ctx->type == 12) CRC_CALC_CYCLE(    rol(MYCRC, 1, 0) + MYBYTE)
    else if(ctx->type == 13) CRC_CALC_CYCLE(    ror(MYCRC, 1, 0) ^ MYBYTE)
    else if(ctx->type == 14) CRC_CALC_CYCLE(    ror(MYCRC, 1, 0) + MYBYTE)
    else if(ctx->type == 15) CRC_CALC_CYCLE(    (MYCRC << 5) + MYCRC + MYBYTE) // djb2 5381
    else if(ctx->type == 16) CRC_CALC_CYCLE(    (MYCRC * ctx->poly) + MYBYTE) // djb2 and sdbm
    else if(ctx->type == 17) CRC_CALC_CYCLE(    (MYCRC * ctx->poly) ^ MYBYTE) // djb2 and FNV-1
    else if(ctx->type == 18) CRC_CALC_CYCLE(    (MYCRC ^ MYBYTE) * ctx->poly) // FNV-1a
    else if(ctx->type == 19) CRC_CALC_CYCLE(    MYBYTE + (MYCRC << 6) + (MYCRC << 16) - MYCRC) // sdbm 65599
    else if(ctx->type == 20) CRC_CALC_CYCLE(    ctx->poly * (MYCRC + MYBYTE * (i + 1)))
    else if(ctx->type == 21) crc =              qhashmurmur3_32(data, datalen);
    else if(ctx->type == 22) crc =              qhashfnv1_32(data, datalen);
    else if(ctx->type == 23) crc =              qhashfnv1_64(data, datalen);
    else if(ctx->type == 24) crc =              XXH32(data, datalen, ctx->poly);
    else if(ctx->type == 25) crc =              XXH64(data, datalen, ctx->poly);
    else if(ctx->type == 26) crc =              jenkins_one_at_a_time_hash(data, datalen);
    else if(ctx->type == 27) crc =              xPear16(data, datalen);
    else if(ctx->type == 28) crc =              CityHash32(data, datalen);
    else if(ctx->type == 29) crc =              CityHash64(data, datalen);
    else if(ctx->type == 30) crc =              CityHash64WithSeed(data, datalen, ctx->poly);
    else if(ctx->type == 31) crc =              StormHash(data, datalen, MPQ_HASH_TABLE_INDEX);
    else if(ctx->type == 32) crc =              StormHash(data, datalen, MPQ_HASH_NAME_A);
    else if(ctx->type == 33) crc =              StormHash(data, datalen, MPQ_HASH_NAME_B);
    else if(ctx->type == 34) crc =              StormHash(data, datalen, MPQ_HASH_FILE_KEY);
    else if(ctx->type == 35) crc =              StormHash(data, datalen, MPQ_HASH_KEY2_MIX);
    else if(ctx->type == 36) crc =              jenkins_hashlittle(data, datalen, ctx->poly);
    else if(ctx->type == 37) crc =              adler32(0, data, datalen);
    else if(ctx->type == 38) crc =              fnv32(data, datalen, crc ? crc : 0x811c9dc5);
    else if(ctx->type == 39) crc =              UHash(data, datalen, crc, ctx->poly, 0x7fffffff);
    else if(ctx->type == 40) crc =              spookyhash_32(data, datalen, ctx->poly);
    else if(ctx->type == 41) crc =              spookyhash_64(data, datalen, ctx->poly);
    else if(ctx->type == 42) crc =              XXH3_64bits(data, datalen);
    else if(ctx->type == 43) crc =              XXH3_64bits_withSeed(data, datalen, ctx->poly);
    else {
        fprintf(stderr, "\nError: unsupported crc type %d\n", (int32_t)ctx->type);
        if(ret_error) *ret_error = 1;
        return -1;
    }
    crc ^= ctx->final;  // XorOut
    crc = MYCRC;
    return(crc);
    #undef MYCRC
    #undef CRC_CALC_CYCLE
}


