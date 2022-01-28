/* abc.c */

/*
 * ABC v.3 reference implementation
 *
 * Core routines for key and IV setup, block-, byte- and packetwise keystream
 * generation, encryption and decryption.
 */

#include "abc.h"

#include <string.h>


/* Helper macros */

/* ABC_CORE
 *
 * ABC core keystream generation routine.
 * NOTE: Due to optimization issues the LFSR state is not rotated. In cases
 *  when the rotation is neccessary, it is done with the ABC_LFSR_SHIFT macro.
 *
 * Arguments:
 *  u32 zz0, zz1, zz2, zz3      - A primitive (LFSR) state
 *  u32 xx                      - B primitive (single cycle function) state
 *  u32 dd0, dd1, dd2           - B primitive (single cycle function)
 *                                coeficients
 *  u32 rr                      - buffer filled with keystream
 *  u32* tt                     - pointer to optimization table (or to the
 *                                array of C coefficients in case optimization
 *                                window size is 1, that is, no optimization is
 *                                used)
 */
#if defined (ABC_WINDOW_1)
#define ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt) \
    zz0 = zz2 ^ (zz1 << 31) ^ (zz0 >> 1);                       \
    xx = zz0 + (((xx ^ dd0) + dd1) ^ dd2);                      \
    rr =                                                        \
        tt[32] +                                                \
        ((xx & U32C(0x00000001)) ? tt[0] : 0) +                 \
        ((xx & U32C(0x00000002)) ? tt[1] : 0) +                 \
        ((xx & U32C(0x00000004)) ? tt[2] : 0) +                 \
        ((xx & U32C(0x00000008)) ? tt[3] : 0) +                 \
        ((xx & U32C(0x00000010)) ? tt[4] : 0) +                 \
        ((xx & U32C(0x00000020)) ? tt[5] : 0) +                 \
        ((xx & U32C(0x00000040)) ? tt[6] : 0) +                 \
        ((xx & U32C(0x00000080)) ? tt[7] : 0) +                 \
        ((xx & U32C(0x00000100)) ? tt[8] : 0) +                 \
        ((xx & U32C(0x00000200)) ? tt[9] : 0) +                 \
        ((xx & U32C(0x00000400)) ? tt[10] : 0) +                \
        ((xx & U32C(0x00000800)) ? tt[11] : 0) +                \
        ((xx & U32C(0x00001000)) ? tt[12] : 0) +                \
        ((xx & U32C(0x00002000)) ? tt[13] : 0) +                \
        ((xx & U32C(0x00004000)) ? tt[14] : 0) +                \
        ((xx & U32C(0x00008000)) ? tt[15] : 0) +                \
        ((xx & U32C(0x00010000)) ? tt[16] : 0) +                \
        ((xx & U32C(0x00020000)) ? tt[17] : 0) +                \
        ((xx & U32C(0x00040000)) ? tt[18] : 0) +                \
        ((xx & U32C(0x00080000)) ? tt[19] : 0) +                \
        ((xx & U32C(0x00100000)) ? tt[20] : 0) +                \
        ((xx & U32C(0x00200000)) ? tt[21] : 0) +                \
        ((xx & U32C(0x00400000)) ? tt[22] : 0) +                \
        ((xx & U32C(0x00800000)) ? tt[23] : 0) +                \
        ((xx & U32C(0x01000000)) ? tt[24] : 0) +                \
        ((xx & U32C(0x02000000)) ? tt[25] : 0) +                \
        ((xx & U32C(0x04000000)) ? tt[26] : 0) +                \
        ((xx & U32C(0x08000000)) ? tt[27] : 0) +                \
        ((xx & U32C(0x10000000)) ? tt[28] : 0) +                \
        ((xx & U32C(0x20000000)) ? tt[29] : 0) +                \
        ((xx & U32C(0x40000000)) ? tt[30] : 0) +                \
        ((xx & U32C(0x80000000)) ? tt[31] : 0);                 \
    rr = zz1 + ROTL32(rr, 16)
#elif defined(ABC_WINDOW_2)
#define ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt) \
    zz0= zz2 ^ (zz1 << 31) ^ (zz0 >> 1);                        \
    xx = zz0 + (((xx ^ dd0) + dd1) ^ dd2);                      \
    rr =                                                        \
        tt[xx & U32C(0x03)] +                                   \
        tt[4 + ((xx >> 2) & U32C(0x03))] +                      \
        tt[8 + ((xx >> 4) & U32C(0x03))] +                      \
        tt[12 + ((xx >> 6) & U32C(0x03))] +                     \
        tt[16 + ((xx >> 8) & U32C(0x03))] +                     \
        tt[20 + ((xx >> 10) & U32C(0x03))] +                    \
        tt[24 + ((xx >> 12) & U32C(0x03))] +                    \
        tt[28 + ((xx >> 14) & U32C(0x03))] +                    \
        tt[32 + ((xx >> 16) & U32C(0x03))] +                    \
        tt[36 + ((xx >> 18) & U32C(0x03))] +                    \
        tt[40 + ((xx >> 20) & U32C(0x03))] +                    \
        tt[44 + ((xx >> 22) & U32C(0x03))] +                    \
        tt[48 + ((xx >> 24) & U32C(0x03))] +                    \
        tt[52 + ((xx >> 26) & U32C(0x03))] +                    \
        tt[56 + ((xx >> 28) & U32C(0x03))] +                    \
        tt[60 + ((xx >> 30) & U32C(0x03))];                     \
    rr = zz1 + ROTL32(rr, 16)
#elif defined(ABC_WINDOW_4)
#define ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt) \
    zz0 = zz2 ^ (zz1 << 31) ^ (zz0 >> 1);                       \
    xx = zz0 + (((xx ^ dd0) + dd1) ^ dd2);                      \
    rr =                                                        \
        tt[xx & U32C(0x0f)] +                                   \
        tt[16 + ((xx >> 4) & U32C(0x0f))] +                     \
        tt[32 + ((xx >> 8) & U32C(0x0f))] +                     \
        tt[48 + ((xx >> 12) & U32C(0x0f))] +                    \
        tt[64 + ((xx >> 16) & U32C(0x0f))] +                    \
        tt[80 + ((xx >> 20) & U32C(0x0f))] +                    \
        tt[96 + ((xx >> 24) & U32C(0x0f))] +                    \
        tt[112 + ((xx >> 28) & U32C(0x0f))];                    \
    rr = zz1 + ROTL32(rr, 16)
#elif defined(ABC_WINDOW_8)
#define ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt) \
    zz0 = zz2 ^ (zz1 << 31) ^ (zz0 >> 1);                       \
    xx = zz0 + (((xx ^ dd0) + dd1) ^ dd2);                      \
    rr =                                                        \
        tt[xx & U32C(0xff)] +                                   \
        tt[256 + ((xx >> 8) & U32C(0xff))] +                    \
        tt[512 + ((xx >> 16) & U32C(0xff))] +                   \
        tt[768 + ((xx >> 24) & U32C(0xff))];                    \
    rr = zz1 + ROTL32(rr, 16)
#elif defined(ABC_WINDOW_12)
#define ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt) \
    zz0 = zz2 ^ (zz1 << 31) ^ (zz0 >> 1);                       \
    xx = zz0 + (((xx ^ dd0) + dd1) ^ dd2);                      \
    rr =                                                        \
        tt[xx & U32C(0x0fff)] +                                 \
        tt[4096 + ((xx >> 12) & 0x0fff)] +                      \
        tt[8192 + ((xx >> 24) & U32C(0xff))];                   \
    rr = zz1 + ROTL32(rr, 16)
#endif

/* ------------------------------------------------------------------------- */

/* ABC_LFSR_SHIFT
 *
 * Macro shifting the LFSR A, as the shift is required in some cases.
 *
 * Arguments:
 *  u32 zz0, zz1, zz2, zz3 - A primitive (LFSR) state along with helper
 *                            buffer
 *  u32 rr                 - helper buffer
 */
#define ABC_LFSR_SHIFT(zz0, zz1, zz2, zz3, rr) \
    rr = zz0;                                  \
    zz0 = zz1;                                 \
    zz1 = zz2;                                 \
    zz2 = zz3;                                 \
    zz3 = rr

/* ------------------------------------------------------------------------- */

/* ABC_PROCESS_WORD
 *
 * Macro producing 4 bytes of ciphertext for use on ECRYPT_process_blocks().
 * Takes offset of first produced byte from beginning of input/output buffers
 * as a parameter.
 *
 * Arguments:
 *  u32 zz0, zz1, zz2, zz3      - A primitive (LFSR) state
 *  u32 xx                      - B primitive (single cycle function) state
 *  u32 dd0, dd1, dd2           - B primitive (single cycle function)
 *                                coeficients
 *  u32 rr                      - helper buffer
 *  u32* tt                     - pointer to optimization table (or to the
 *                                array of C coefficients in case optimization
 *                                window size is 1)
 *  u8* in                      - pointer to input data buffer
 *  u8* out                     - pointer to output data buffer
 *  u32 off                     - offset in input/output data buffers
 */
#define ABC_PROCESS_WORD(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt, in, out, off) \
    ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt);                          \
    rr ^= U8TO32_LITTLE(in + off);                                                    \
    U32TO8_LITTLE(out + off, rr)

/* ------------------------------------------------------------------------- */

/* ABC_KEYSTREAM_WORD
 *
 * Macro producing 4 bytes of keystream for use on ECRYPT_keystream_blocks
 * rotine. Takes offset of first produced byte from beginning of keystream
 * buffer as a parameter.
 *
 * Arguments:
 *  u32 zz0, zz1, zz2, zz3      - A primitive (LFSR) state
 *  u32 xx                      - B primitive (single cycle function) state
 *  u32 dd0, dd1, dd2           - B primitive (single cycle function)
 *                                coeficients
 *  u32 rr                      - buffer filled with keystream
 *  u32* tt                     - pointer to optimization table (or to the
 *                                array of C coefficients in case optimization
 *                                window size is 1)
 *  u8* buf                     - pointer to keystream buffer
 *  u32 off                     - offset in keystream buffer
 */
#define ABC_KEYSTREAM_WORD(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt, buf, off) \
    ABC_CORE(zz0, zz1, zz2, zz3, xx, dd0, dd1, dd2, rr, tt);                        \
    U32TO8_LITTLE(buf + off, rr)


/* ------------------------------------------------------------------------- */

/* Core routines */

/*
 * Key and message independent initialization.
 */
void ECRYPT_init()
{
}

/* ------------------------------------------------------------------------- */

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx,
  const u8* key,
  u32 keysize,                /* Key size in bits. */
  u32 ivsize)                 /* IV size in bits. */
{
    /* fixed precomputed table or coefficients for use in key expansion */
#include "abc-tables.h"

#ifdef ABC_WINDOW_1
    u32 *e = ctx->t;
#else /* ABC_WINDOW_1 */
    u32 e[ABC_C_COEF_NUM];
    u32 *t = ctx->t;
#endif /* ABC_WINDOW_1 */
    /* key-dependent state */
    u32 d0 = U8TO32_LITTLE(key + 12) & ABC_B_COEF_AND_MASK;
    u32 d1 = (U8TO32_LITTLE(key + 8) & ABC_B_COEF_AND_MASK) | ABC_B_COEF_OR_MASK;
    u32 d2 = U8TO32_LITTLE(key + 4) & ABC_B_COEF_AND_MASK;
    u32 x =  U8TO32_LITTLE(key);
    u32 z0 = ROTL32(U8TO32_LITTLE(key), 16) | ABC_A_MASK;
    u32 z1 = ROTL32(U8TO32_LITTLE(key + 4), 16);
    u32 z2 = ROTL32(U8TO32_LITTLE(key + 8), 16);
    u32 z3 = ROTL32(U8TO32_LITTLE(key + 12), 16);
    /* helper variables */
    u32 i, r, s;

    /* warm up */
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, table);
    x ^= r;
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, r, table);
    d0 = (d0 ^ r) & ABC_B_COEF_AND_MASK;
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, table);
    d1 = ((d1 ^ r) & ABC_B_COEF_AND_MASK) | ABC_B_COEF_OR_MASK;
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, table);
    d2 = (d2 ^ r) & ABC_B_COEF_AND_MASK;
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, table);
    z3 ^= r;
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, r, table);
    z0 ^= r;
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, table);
    z1 ^= r;
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, table);
    z2 ^= r;
    z0 |= ABC_A_MASK;

    /* fill ABC state */
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, ctx->d0, table);
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, ctx->d1, table);
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, ctx->d2, table);
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, ctx->x, table);
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, ctx->z0, table);
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, ctx->z1, table);
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, ctx->z2, table);
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, ctx->z3, table);
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, e[ABC_C_COEF_NUM - 1], table);
    for (i = 0; i < ABC_C_COEF_NUM - 1; ++i)
    {
        ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, s, table);
        ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, table);
        s &= r;
        ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, table);
        s &= r;
        ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, table);
        e[i] = s & r;
    }
    for (i = 0; i < 32; ++i)  
    {
        s = 1 << i;
        ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, r, table);
        e[(r >> 0) & U32C(0x1f)] |= s;
        e[(r >> 5) & U32C(0x1f)] |= s;
        e[(r >> 10) & U32C(0x1f)] |= s;
        e[(r >> 15) & U32C(0x1f)] |= s;
        e[(r >> 20) & U32C(0x1f)] |= s;
        e[(r >> 25) & U32C(0x1f)] |= s;
        ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, table);
        e[(r >> 0) & U32C(0x1f)] |= s;
        e[(r >> 5) & U32C(0x1f)] |= s;
        e[(r >> 10) & U32C(0x1f)] |= s;
        e[(r >> 15) & U32C(0x1f)] |= s;
        e[(r >> 20) & U32C(0x1f)] |= s;
        e[(r >> 25) & U32C(0x1f)] |= s;
        ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, table);
        e[(r >> 0) & U32C(0x1f)] |= s;
        e[(r >> 5) & U32C(0x1f)] |= s;
        e[(r >> 10) & U32C(0x1f)] |= s;
        e[(r >> 15) & U32C(0x1f)] |= s;
        e[(r >> 20) & U32C(0x1f)] |= s;
        e[(r >> 25) & U32C(0x1f)] |= s;
        ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, table);
        e[(r >> 0) & U32C(0x1f)] |= s;
        e[(r >> 5) & U32C(0x1f)] |= s;
    }
    /* the step against zero fillings in columns 0-15 */
    e[30] |= e[31] & ~ABC_C_AND_MASK;

    /* apply restrictions and save changeable part of state */
    ctx->z0i = ctx->z0 = ctx->z0 | ABC_A_MASK;
    ctx->z1i = ctx->z1;
    ctx->z2i = ctx->z2;
    ctx->z3i = ctx->z3;
    ctx->xi = ctx->x;
    ctx->d0i = ctx->d0 = ctx->d0 & ABC_B_COEF_AND_MASK;
    ctx->d1i = ctx->d1 = (ctx->d1  & ABC_B_COEF_AND_MASK) | ABC_B_COEF_OR_MASK;
    ctx->d2i = ctx->d2 = ctx->d2 & ABC_B_COEF_AND_MASK;
    e[31] = (e[31] & ABC_C_AND_MASK) | ABC_C_OR_MASK;

    /* precompute optimization table for keystream generation */
#if defined(ABC_WINDOW_2)
    t[0] = e[32];
    t[1] = e[32] + e[2 * 0 + 0];
    t[2] = e[32] + e[2 * 0 + 1];
    t[3] = e[32] + e[2 * 0 + 0] + e[2 * 0 + 1];

    t[4 * 1 + 0] = 0;
    t[4 * 1 + 1] = e[2 * 1 + 0];
    t[4 * 1 + 2] = e[2 * 1 + 1];
    t[4 * 1 + 3] = e[2 * 1 + 0] + e[2 * 1 + 1];

    t[4 * 2 + 0] = 0;
    t[4 * 2 + 1] = e[2 * 2 + 0];
    t[4 * 2 + 2] = e[2 * 2 + 1];
    t[4 * 2 + 3] = e[2 * 2 + 0] + e[2 * 2 + 1];

    t[4 * 3 + 0] = 0;
    t[4 * 3 + 1] = e[2 * 3 + 0];
    t[4 * 3 + 2] = e[2 * 3 + 1];
    t[4 * 3 + 3] = e[2 * 3 + 0] + e[2 * 3 + 1];

    t[4 * 4 + 0] = 0;
    t[4 * 4 + 1] = e[2 * 4 + 0];
    t[4 * 4 + 2] = e[2 * 4 + 1];
    t[4 * 4 + 3] = e[2 * 4 + 0] + e[2 * 4 + 1];

    t[4 * 5 + 0] = 0;
    t[4 * 5 + 1] = e[2 * 5 + 0];
    t[4 * 5 + 2] = e[2 * 5 + 1];
    t[4 * 5 + 3] = e[2 * 5 + 0] + e[2 * 5 + 1];

    t[4 * 6 + 0] = 0;
    t[4 * 6 + 1] = e[2 * 6 + 0];
    t[4 * 6 + 2] = e[2 * 6 + 1];
    t[4 * 6 + 3] = e[2 * 6 + 0] + e[2 * 6 + 1];

    t[4 * 7 + 0] = 0;
    t[4 * 7 + 1] = e[2 * 7 + 0];
    t[4 * 7 + 2] = e[2 * 7 + 1];
    t[4 * 7 + 3] = e[2 * 7 + 0] + e[2 * 7 + 1];

    t[4 * 8 + 0] = 0;
    t[4 * 8 + 1] = e[2 * 8 + 0];
    t[4 * 8 + 2] = e[2 * 8 + 1];
    t[4 * 8 + 3] = e[2 * 8 + 0] + e[2 * 8 + 1];

    t[4 * 9 + 0] = 0;
    t[4 * 9 + 1] = e[2 * 9 + 0];
    t[4 * 9 + 2] = e[2 * 9 + 1];
    t[4 * 9 + 3] = e[2 * 9 + 0] + e[2 * 9 + 1];

    t[4 * 10 + 0] = 0;
    t[4 * 10 + 1] = e[2 * 10 + 0];
    t[4 * 10 + 2] = e[2 * 10 + 1];
    t[4 * 10 + 3] = e[2 * 10 + 0] + e[2 * 10 + 1];

    t[4 * 11 + 0] = 0;
    t[4 * 11 + 1] = e[2 * 11 + 0];
    t[4 * 11 + 2] = e[2 * 11 + 1];
    t[4 * 11 + 3] = e[2 * 11 + 0] + e[2 * 11 + 1];

    t[4 * 12 + 0] = 0;
    t[4 * 12 + 1] = e[2 * 12 + 0];
    t[4 * 12 + 2] = e[2 * 12 + 1];
    t[4 * 12 + 3] = e[2 * 12 + 0] + e[2 * 12 + 1];

    t[4 * 13 + 0] = 0;
    t[4 * 13 + 1] = e[2 * 13 + 0];
    t[4 * 13 + 2] = e[2 * 13 + 1];
    t[4 * 13 + 3] = e[2 * 13 + 0] + e[2 * 13 + 1];

    t[4 * 14 + 0] = 0;
    t[4 * 14 + 1] = e[2 * 14 + 0];
    t[4 * 14 + 2] = e[2 * 14 + 1];
    t[4 * 14 + 3] = e[2 * 14 + 0] + e[2 * 14 + 1];

    t[4 * 15 + 0] = 0;
    t[4 * 15 + 1] = e[2 * 15 + 0];
    t[4 * 15 + 2] = e[2 * 15 + 1];
    t[4 * 15 + 3] = e[2 * 15 + 0] + e[2 * 15 + 1];
#elif defined(ABC_WINDOW_4)
    for(i = 0; i < 16; ++i)
    {
        t[i] = e[32];
        t[i] += (i & 0x1) ? e[4 * 0 + 0] : 0;
        t[i] += (i & 0x2) ? e[4 * 0 + 1] : 0;
        t[i] += (i & 0x4) ? e[4 * 0 + 2] : 0;
        t[i] += (i & 0x8) ? e[4 * 0 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[16 + i] = 0;
        t[16 + i] += (i & 0x1) ? e[4 * 1 + 0] : 0;
        t[16 + i] += (i & 0x2) ? e[4 * 1 + 1] : 0;
        t[16 + i] += (i & 0x4) ? e[4 * 1 + 2] : 0;
        t[16 + i] += (i & 0x8) ? e[4 * 1 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[32 + i] = 0;
        t[32 + i] += (i & 0x1) ? e[4 * 2 + 0] : 0;
        t[32 + i] += (i & 0x2) ? e[4 * 2 + 1] : 0;
        t[32 + i] += (i & 0x4) ? e[4 * 2 + 2] : 0;
        t[32 + i] += (i & 0x8) ? e[4 * 2 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[48 + i] = 0;
        t[48 + i] += (i & 0x1) ? e[4 * 3 + 0] : 0;
        t[48 + i] += (i & 0x2) ? e[4 * 3 + 1] : 0;
        t[48 + i] += (i & 0x4) ? e[4 * 3 + 2] : 0;
        t[48 + i] += (i & 0x8) ? e[4 * 3 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[64 + i] = 0;
        t[64 + i] += (i & 0x1) ? e[4 * 4 + 0] : 0;
        t[64 + i] += (i & 0x2) ? e[4 * 4 + 1] : 0;
        t[64 + i] += (i & 0x4) ? e[4 * 4 + 2] : 0;
        t[64 + i] += (i & 0x8) ? e[4 * 4 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[80 + i] = 0;
        t[80 + i] += (i & 0x1) ? e[4 * 5 + 0] : 0;
        t[80 + i] += (i & 0x2) ? e[4 * 5 + 1] : 0;
        t[80 + i] += (i & 0x4) ? e[4 * 5 + 2] : 0;
        t[80 + i] += (i & 0x8) ? e[4 * 5 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[96 + i] = 0;
        t[96 + i] += (i & 0x1) ? e[4 * 6 + 0] : 0;
        t[96 + i] += (i & 0x2) ? e[4 * 6 + 1] : 0;
        t[96 + i] += (i & 0x4) ? e[4 * 6 + 2] : 0;
        t[96 + i] += (i & 0x8) ? e[4 * 6 + 3] : 0;
    }
    for(i = 0; i < 16; ++i)
    {
        t[112 + i] = 0;
        t[112 + i] += (i & 0x1) ? e[4 * 7 + 0] : 0;
        t[112 + i] += (i & 0x2) ? e[4 * 7 + 1] : 0;
        t[112 + i] += (i & 0x4) ? e[4 * 7 + 2] : 0;
        t[112 + i] += (i & 0x8) ? e[4 * 7 + 3] : 0;
    }
#elif defined(ABC_WINDOW_8)
    for(i = 0; i < 256; ++i)
    {
        t[i] = e[32];
        t[i] += (i & 0x01) ? e[8 * 0 + 0] : 0;
        t[i] += (i & 0x02) ? e[8 * 0 + 1] : 0;
        t[i] += (i & 0x04) ? e[8 * 0 + 2] : 0;
        t[i] += (i & 0x08) ? e[8 * 0 + 3] : 0;
        t[i] += (i & 0x10) ? e[8 * 0 + 4] : 0;
        t[i] += (i & 0x20) ? e[8 * 0 + 5] : 0;
        t[i] += (i & 0x40) ? e[8 * 0 + 6] : 0;
        t[i] += (i & 0x80) ? e[8 * 0 + 7] : 0;
    }
    for(i = 0; i < 256; ++i)
    {
        t[256 + i] = 0;
        t[256 + i] += (i & 0x01) ? e[8 * 1 + 0] : 0;
        t[256 + i] += (i & 0x02) ? e[8 * 1 + 1] : 0;
        t[256 + i] += (i & 0x04) ? e[8 * 1 + 2] : 0;
        t[256 + i] += (i & 0x08) ? e[8 * 1 + 3] : 0;
        t[256 + i] += (i & 0x10) ? e[8 * 1 + 4] : 0;
        t[256 + i] += (i & 0x20) ? e[8 * 1 + 5] : 0;
        t[256 + i] += (i & 0x40) ? e[8 * 1 + 6] : 0;
        t[256 + i] += (i & 0x80) ? e[8 * 1 + 7] : 0;
    }
    for(i = 0; i < 256; ++i)
    {
        t[512 + i] = 0;
        t[512 + i] += (i & 0x01) ? e[8 * 2 + 0] : 0;
        t[512 + i] += (i & 0x02) ? e[8 * 2 + 1] : 0;
        t[512 + i] += (i & 0x04) ? e[8 * 2 + 2] : 0;
        t[512 + i] += (i & 0x08) ? e[8 * 2 + 3] : 0;
        t[512 + i] += (i & 0x10) ? e[8 * 2 + 4] : 0;
        t[512 + i] += (i & 0x20) ? e[8 * 2 + 5] : 0;
        t[512 + i] += (i & 0x40) ? e[8 * 2 + 6] : 0;
        t[512 + i] += (i & 0x80) ? e[8 * 2 + 7] : 0;
    }
    for(i = 0; i < 256; ++i)
    {
        t[768 + i] = 0;
        t[768 + i] += (i & 0x01) ? e[8 * 3 + 0] : 0;
        t[768 + i] += (i & 0x02) ? e[8 * 3 + 1] : 0;
        t[768 + i] += (i & 0x04) ? e[8 * 3 + 2] : 0;
        t[768 + i] += (i & 0x08) ? e[8 * 3 + 3] : 0;
        t[768 + i] += (i & 0x10) ? e[8 * 3 + 4] : 0;
        t[768 + i] += (i & 0x20) ? e[8 * 3 + 5] : 0;
        t[768 + i] += (i & 0x40) ? e[8 * 3 + 6] : 0;
        t[768 + i] += (i & 0x80) ? e[8 * 3 + 7] : 0;
    }
#elif defined(ABC_WINDOW_12)
    for(i = 0; i < 4096; ++i)
    {
        t[i] = e[32];
        t[i] += (i & 0x001) ? e[12 * 0 + 0] : 0;
        t[i] += (i & 0x002) ? e[12 * 0 + 1] : 0;
        t[i] += (i & 0x004) ? e[12 * 0 + 2] : 0;
        t[i] += (i & 0x008) ? e[12 * 0 + 3] : 0;
        t[i] += (i & 0x010) ? e[12 * 0 + 4] : 0;
        t[i] += (i & 0x020) ? e[12 * 0 + 5] : 0;
        t[i] += (i & 0x040) ? e[12 * 0 + 6] : 0;
        t[i] += (i & 0x080) ? e[12 * 0 + 7] : 0;
        t[i] += (i & 0x100) ? e[12 * 0 + 8] : 0;
        t[i] += (i & 0x200) ? e[12 * 0 + 9] : 0;
        t[i] += (i & 0x400) ? e[12 * 0 + 10] : 0;
        t[i] += (i & 0x800) ? e[12 * 0 + 11] : 0;
    }
    for(i = 0; i < 4096; ++i)
    {
        t[4096 + i] = 0;
        t[4096 + i] += (i & 0x001) ? e[12 * 1 + 0] : 0;
        t[4096 + i] += (i & 0x002) ? e[12 * 1 + 1] : 0;
        t[4096 + i] += (i & 0x004) ? e[12 * 1 + 2] : 0;
        t[4096 + i] += (i & 0x008) ? e[12 * 1 + 3] : 0;
        t[4096 + i] += (i & 0x010) ? e[12 * 1 + 4] : 0;
        t[4096 + i] += (i & 0x020) ? e[12 * 1 + 5] : 0;
        t[4096 + i] += (i & 0x040) ? e[12 * 1 + 6] : 0;
        t[4096 + i] += (i & 0x080) ? e[12 * 1 + 7] : 0;
        t[4096 + i] += (i & 0x100) ? e[12 * 1 + 8] : 0;
        t[4096 + i] += (i & 0x200) ? e[12 * 1 + 9] : 0;
        t[4096 + i] += (i & 0x400) ? e[12 * 1 + 10] : 0;
        t[4096 + i] += (i & 0x800) ? e[12 * 1 + 11] : 0;
    }
    for(i = 0; i < 256; ++i)
    {
        t[8192 + i] = 0;
        t[8192 + i] += (i & 0x01) ? e[12 * 2 + 0] : 0;
        t[8192 + i] += (i & 0x02) ? e[12 * 2 + 1] : 0;
        t[8192 + i] += (i & 0x04) ? e[12 * 2 + 2] : 0;
        t[8192 + i] += (i & 0x08) ? e[12 * 2 + 3] : 0;
        t[8192 + i] += (i & 0x10) ? e[12 * 2 + 4] : 0;
        t[8192 + i] += (i & 0x20) ? e[12 * 2 + 5] : 0;
        t[8192 + i] += (i & 0x40) ? e[12 * 2 + 6] : 0;
        t[8192 + i] += (i & 0x80) ? e[12 * 2 + 7] : 0;
    }
#endif
}

/* ------------------------------------------------------------------------- */

/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  const u8* iv)
{
    /* apply IV to restored state */
    u32 d0 = (ctx->d0i ^ U8TO32_LITTLE(iv + 12)) & ABC_B_COEF_AND_MASK;
    u32 d1 =
        ((ctx->d1i ^ U8TO32_LITTLE(iv + 8)) & ABC_B_COEF_AND_MASK) |
        ABC_B_COEF_OR_MASK;
    u32 d2 = (ctx->d2i ^ U8TO32_LITTLE(iv + 4)) & ABC_B_COEF_AND_MASK;
    u32 x = ctx->xi ^ U8TO32_LITTLE(iv);
    u32 z0 = (ctx->z0i ^ ROTL32(U8TO32_LITTLE(iv), 16)) | ABC_A_MASK;
    u32 z1 = ctx->z1i ^ ROTL32(U8TO32_LITTLE(iv + 4), 16);
    u32 z2 = ctx->z2i ^ ROTL32(U8TO32_LITTLE(iv + 8), 16);
    u32 z3 = ctx->z3i ^ ROTL32(U8TO32_LITTLE(iv + 12), 16);
    u32 *t = ctx->t;
    u32 r;

    /* warm up */
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, t);
    x ^= r;
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, r, t);
    d0 = (d0 ^ r) & ABC_B_COEF_AND_MASK;
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, t);
    d1 = ((d1 ^ r) & ABC_B_COEF_AND_MASK) | ABC_B_COEF_OR_MASK;
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, t);
    d2 = (d2 ^ r) & ABC_B_COEF_AND_MASK;
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, t);
    z3 ^= r;
    ABC_CORE(z1, z2, z3, z0, x, d0, d1, d2, r, t);
    z0 ^= r;
    ABC_CORE(z2, z3, z0, z1, x, d0, d1, d2, r, t);
    z1 ^= r;
    ABC_CORE(z3, z0, z1, z2, x, d0, d1, d2, r, t);
    z2 ^= r;
    z0 |= ABC_A_MASK;

    /* update the state */
    ctx->z0 = z0;
    ctx->z1 = z1;
    ctx->z2 = z2;
    ctx->z3 = z3;
    ctx->x = x;
    ctx->d0 = d0;
    ctx->d1 = d1;
    ctx->d2 = d2;
}

/* ------------------------------------------------------------------------- */

/*
 * Bytewise encryption/decryption.
 */
void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx,
  const u8* input,
  u8* output,
  u32 msglen)                 /* Message length in bytes. */
{
    /* local copy of the state */
    u32 z0 = ctx->z0;
    u32 z1 = ctx->z1;
    u32 z2 = ctx->z2;
    u32 z3 = ctx->z3;
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 d2 = ctx->d2;
    u32 *t = ctx->t;
    /* helper variables */
    u32 i, j, r;
    u8 buf[4];

#if defined(ABC_UNROLL_4)
    for (i = 0; i < (msglen & U32C(0xfffffff0)); i += 16)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < (msglen & U32C(0xffffffe0)); i += 32)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 16);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 20);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 24);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < (msglen & U32C(0xffffffc0)); i += 64)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 16);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 20);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 24);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 28);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 32);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 36);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 40);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 44);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 48);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 52);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 56);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 60);
    }
#endif /* ABC_UNROLL_4 */
    for (; i < (msglen & U32C(0xfffffffc)); i += 4)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_LFSR_SHIFT(z0, z1, z2, z3, r);
    }
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, t);
    U32TO8_LITTLE(buf, r);
    for (j = 0; j < (msglen & U32C(0x00000003)); ++j)
        (output + i)[j] = (input + i)[j] ^ buf[j];
}

/* ------------------------------------------------------------------------- */

/*
 * Bytewise keystream generation.
 */
void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)                 /* Length of keystream in bytes. */
{
    /* local copy of the state */
    u32 z0 = ctx->z0;
    u32 z1 = ctx->z1;
    u32 z2 = ctx->z2;
    u32 z3 = ctx->z3;
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 d2 = ctx->d2;
    u32 *t = ctx->t;
    /* helper variables */
    u32 i, r;

#if defined(ABC_UNROLL_4)
    for (i = 0; i < (length & U32C(0xfffffff0)); i += 16)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < (length & U32C(0xffffffe0)); i += 32)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 16);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 20);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 24);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < (length & U32C(0xffffffc0)); i += 64)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 16);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 20);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 24);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 28);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 32);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 36);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 40);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 44);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 48);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 52);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 56);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 60);
    }
#endif /* ABC_UNROLL_4 */
    for (; i < (length & U32C(0xfffffffc)); i += 4)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_LFSR_SHIFT(z0, z1, z2, z3, r);
    }
    ABC_CORE(z0, z1, z2, z3, x, d0, d1, d2, r, t);
    memcpy(keystream + i, &r, length & U32C(0x00000003));
}

/* ------------------------------------------------------------------------- */

/*
 * Blockwise encryption/decryption.
 */
void ECRYPT_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx,
  const u8* input,
  u8* output,
  u32 blocks)                 /* Message length in blocks. */
{
    /* local copy of the state */
    u32 z0 = ctx->z0;
    u32 z1 = ctx->z1;
    u32 z2 = ctx->z2;
    u32 z3 = ctx->z3;
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 d2 = ctx->d2;
    u32 *t = ctx->t;
    /* helper variables */
    u32 i, r;

#if defined(ABC_UNROLL_4)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 16)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 32)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 16);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 20);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 24);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 64)
    {
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 4);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 8);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 12);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 16);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 20);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 24);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 28);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 32);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 36);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 40);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 44);
        ABC_PROCESS_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, input, output, i + 48);
        ABC_PROCESS_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, input, output, i + 52);
        ABC_PROCESS_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, input, output, i + 56);
        ABC_PROCESS_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, input, output, i + 60);
    }
#endif /* ABC_UNROLL_4 */
    /* state update */
    ctx->z0 = z0;
    ctx->z1 = z1;
    ctx->z2 = z2;
    ctx->z3 = z3;
    ctx->x = x;
}

/* ------------------------------------------------------------------------- */

/*
 * Blockwise keystream generation.
 */
void ECRYPT_keystream_blocks(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 blocks)                 /* Keystream length in blocks. */
{
    /* local copy of the state */
    u32 z0 = ctx->z0;
    u32 z1 = ctx->z1;
    u32 z2 = ctx->z2;
    u32 z3 = ctx->z3;
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 d2 = ctx->d2;
    u32 *t = ctx->t;
    /* helper variables */
    u32 i, r;

#if defined(ABC_UNROLL_4)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 16)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 32)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 16);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 20);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 24);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 64)
    {
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 4);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 8);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 12);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 16);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 20);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 24);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 28);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 32);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 36);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 40);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 44);
        ABC_KEYSTREAM_WORD(z0, z1, z2, z3, x, d0, d1, d2, r, t, keystream, i + 48);
        ABC_KEYSTREAM_WORD(z1, z2, z3, z0, x, d0, d1, d2, r, t, keystream, i + 52);
        ABC_KEYSTREAM_WORD(z2, z3, z0, z1, x, d0, d1, d2, r, t, keystream, i + 56);
        ABC_KEYSTREAM_WORD(z3, z0, z1, z2, x, d0, d1, d2, r, t, keystream, i + 60);
    }
#endif /* ABC_UNROLL_4 */
    /* state update */
    ctx->z0 = z0;
    ctx->z1 = z1;
    ctx->z2 = z2;
    ctx->z3 = z3;
    ctx->x = x;
}
