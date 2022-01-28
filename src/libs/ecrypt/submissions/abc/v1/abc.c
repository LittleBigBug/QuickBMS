/* abc.c */

/*
 * ABC cipher core, containing routines for key and IV setup, block-, byte- and
 * packetwise keystream generation, encryption and decryption.
 *
 * ABC reference imlementation
 * (c) Vladimir Anashin, Andrey Bogdanov, Ilya Kizhvatov 2005
 *
 * Faculty for Information security,
 * Institute for Information Sciences and Security Technologies,
 * Russian State University for the Humanities.
 */

#include "abc.h"

#include <string.h>


/* Helper macros */

/* ABC_UPPER_PART
 * Upper part of ABC core transform. Takes pointer to optimization table or
 * array of C coefficients as a parameter. Lacks addition of an entry from the
 * last table.
 *
 * ABC_OUTPUT_WORD
 * Returns the output word, performing an addition of an entry from the last
 * table.
 *
 * Assumed variables:
 *  u32 z0, z1 - A primitive (LFSR) state
 *  u32 x      - B primitive (top function) state
 *  u32 d0, d1 - B primitive (top function) coeficients
 *  u32 r, s   - helper variables
 */
#if defined (ABC_WINDOW_1)
#define ABC_UPPER_PART(tbl)                                   \
    s = z1 ^ (z0 >> 1) ^ (z1 << 31);                          \
    z0 = z1;                                                  \
    z1 = s;                                                   \
    s = x ^ d1;                                               \
    x = z1 + d0 + s + (s << 2);                               \
    r = tbl[32] + ((x & U32C(0x00000001)) ? tbl[0] : 0);      \
    r += (x & U32C(0x00000002)) ? tbl[1] : 0;                 \
    r += (x & U32C(0x00000004)) ? tbl[2] : 0;                 \
    r += (x & U32C(0x00000008)) ? tbl[3] : 0;                 \
    r += (x & U32C(0x00000010)) ? tbl[4] : 0;                 \
    r += (x & U32C(0x00000020)) ? tbl[5] : 0;                 \
    r += (x & U32C(0x00000040)) ? tbl[6] : 0;                 \
    r += (x & U32C(0x00000080)) ? tbl[7] : 0;                 \
    r += (x & U32C(0x00000100)) ? tbl[8] : 0;                 \
    r += (x & U32C(0x00000200)) ? tbl[9] : 0;                 \
    r += (x & U32C(0x00000400)) ? tbl[10] : 0;                \
    r += (x & U32C(0x00000800)) ? tbl[11] : 0;                \
    r += (x & U32C(0x00001000)) ? tbl[12] : 0;                \
    r += (x & U32C(0x00002000)) ? tbl[13] : 0;                \
    r += (x & U32C(0x00004000)) ? tbl[14] : 0;                \
    r += (x & U32C(0x00008000)) ? tbl[15] : 0;                \
    r += (x & U32C(0x00010000)) ? tbl[16] : 0;                \
    r += (x & U32C(0x00020000)) ? tbl[17] : 0;                \
    r += (x & U32C(0x00040000)) ? tbl[18] : 0;                \
    r += (x & U32C(0x00080000)) ? tbl[19] : 0;                \
    r += (x & U32C(0x00100000)) ? tbl[20] : 0;                \
    r += (x & U32C(0x00200000)) ? tbl[21] : 0;                \
    r += (x & U32C(0x00400000)) ? tbl[22] : 0;                \
    r += (x & U32C(0x00800000)) ? tbl[23] : 0;                \
    r += (x & U32C(0x01000000)) ? tbl[24] : 0;                \
    r += (x & U32C(0x02000000)) ? tbl[25] : 0;                \
    r += (x & U32C(0x04000000)) ? tbl[26] : 0;                \
    r += (x & U32C(0x08000000)) ? tbl[27] : 0;                \
    r += (x & U32C(0x10000000)) ? tbl[28] : 0;                \
    r += (x & U32C(0x20000000)) ? tbl[29] : 0;                \
    r += (x & U32C(0x40000000)) ? tbl[30] : 0
#define ABC_OUTPUT_WORD(tbl) (r + ((x & U32C(0x80000000)) ? tbl[31] : 0))
#elif defined(ABC_WINDOW_2)
#define ABC_UPPER_PART(tbl)          \
    s = z1 ^ (z0 >> 1) ^ (z1 << 31); \
    z0 = z1;                         \
    z1 = s;                          \
    s = x ^ d1;                      \
    x = z1 + d0 + s + (s << 2);      \
    s = x;                           \
    r = tbl[s & U32C(0x03)];         \
    s >>= 2;                         \
    r += tbl[4 + (s & U32C(0x03))];  \
    s >>= 2;                         \
    r += tbl[8 + (s & U32C(0x03))];  \
    s >>= 2;                         \
    r += tbl[12 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[16 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[20 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[24 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[28 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[32 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[36 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[40 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[44 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[48 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[52 + (s & U32C(0x03))]; \
    s >>= 2;                         \
    r += tbl[56 + (s & U32C(0x03))]; \
    s >>= 2
#define ABC_OUTPUT_WORD(tbl) (r + tbl[60 + (s & U32C(0x03))])
#elif defined(ABC_WINDOW_4)
#define ABC_UPPER_PART(tbl)          \
    s = z1 ^ (z0 >> 1) ^ (z1 << 31); \
    z0 = z1;                         \
    z1 = s;                          \
    s = x ^ d1;                      \
    x = z1 + d0 + s + (s << 2);      \
    s = x;                           \
    r = tbl[s & U32C(0x0f)];         \
    s >>= 4;                         \
    r += tbl[16 + (s & U32C(0x0f))]; \
    s >>= 4;                         \
    r += tbl[32 + (s & U32C(0x0f))]; \
    s >>= 4;                         \
    r += tbl[48 + (s & U32C(0x0f))]; \
    s >>= 4;                         \
    r += tbl[64 + (s & U32C(0x0f))]; \
    s >>= 4;                         \
    r += tbl[80 + (s & U32C(0x0f))]; \
    s >>= 4;                         \
    r += tbl[96 + (s & U32C(0x0f))]; \
    s >>= 4
#define ABC_OUTPUT_WORD(tbl) (r + tbl[112 + (s & U32C(0x0f))])
#elif defined(ABC_WINDOW_8)
#define ABC_UPPER_PART(tbl)           \
    s = z1 ^ (z0 >> 1) ^ (z1 << 31);  \
    z0 = z1;                          \
    z1 = s;                           \
    s = x ^ d1;                       \
    x = z1 + d0 + s + (s << 2);       \
    s = x;                            \
    r = tbl[s & U32C(0xff)];          \
    s >>= 8;                          \
    r += tbl[256 + (s & U32C(0xff))]; \
    s >>= 8;                          \
    r += tbl[512 + (s & U32C(0xff))]; \
    s >>= 8
#define ABC_OUTPUT_WORD(tbl) (r + tbl[768 + (s & U32C(0xff))])
#elif defined(ABC_WINDOW_12)
#define ABC_UPPER_PART(tbl)          \
    s = z1 ^ (z0 >> 1) ^ (z1 << 31); \
    z0 = z1;                         \
    z1 = s;                          \
    s = x ^ d1;                      \
    x = z1 + d0 + s + (s << 2);      \
    s = x;                           \
    r = tbl[s & U32C(0x0fff)];       \
    s >>= 12;                        \
    r += tbl[4096 + (s & 0x0fff)];   \
    s >>= 12
#define ABC_OUTPUT_WORD(tbl) (r + tbl[8192 + (s & U32C(0xff))])
#endif

/* ABC_POSTILTER
 * Applies 16-bit rotation and adds part of LFSR state. Should be used after
 * ABC_OUTPUT_WORD macro.
 */
#define ABC_POSTFILTER(var) (z0 + ((var >> 16) | (var << 16)))

/* ------------------------------------------------------------------------- */

/*
 * Macro producing exlusive OR of given 4-byte variable var and 4-byte word of
 * keystream generated using optimization table pointed by tbl (or array of
 * coefficients of C primitive if window size is 1). Used in ECRYPT_keysetup()
 * and ECRYPT_ivsetup() routines.
 */
#define ABC_WARMUP_WORD(var)      \
    ABC_UPPER_PART(table);        \
    r = ABC_OUTPUT_WORD(table);   \
    var ^= ABC_POSTFILTER(r)

/* ------------------------------------------------------------------------- */

/*
 * Macro placing 4 bytes of generated keystream to given variable var. Used
 * in ECRYPT_keysetup().
 */
#define ABC_KEYSETUP_WORD(var)   \
    ABC_UPPER_PART(table);       \
    r = ABC_OUTPUT_WORD(table);  \
    var = ABC_POSTFILTER(r)

/* ------------------------------------------------------------------------- */

/*
 * Macro producing 4 bytes of ciphertext for use on ECRYPT_process_blocks().
 * Takes offset of first produced byte from beginning of input/output buffers
 * as a parameter.
 *
 * Assumed variables:
 *  const u8 *input - pointer to input data buffer
 *  u8 *output      - pointer to output data buffer
 */
#define ABC_PROCESS_WORD(offset)                                   \
    ABC_UPPER_PART(table);                                         \
    r = ABC_OUTPUT_WORD(table);                                    \
    U32TO8_LITTLE                                                  \
        (                                                          \
            output + offset,                                       \
            U8TO32_LITTLE(input + offset) ^ ABC_POSTFILTER(r)      \
        )

/* ------------------------------------------------------------------------- */

/*
 * Macro producing 4 bytes of keystream for use on ECRYPT_keystream_blocks
 * rotine. Takes offset of first produced byte from beginning of keystream
 * buffer as a parameter.
 *
 * Assumed variables:
 *  u8 *keystream - pointer to keystream buffer
 */
#define ABC_KEYSTREAM_WORD(offset)                            \
    ABC_UPPER_PART(table);                                    \
    r = ABC_OUTPUT_WORD(table);                               \
    U32TO8_LITTLE(keystream + offset, ABC_POSTFILTER(r))


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
#ifndef ABC_WINDOW_2
    u32 i;
#endif /* ABC_WINDOW_2 */
#endif /* ABC_WINDOW_1 */
    /* key-dependent state */
    u32 d0 =
        ABC_B_KEYSETUP_D0 ^
        ((U8TO32_LITTLE(key + 12) & U32C(0x0000ffff)) << 2) ^
        (U8TO32_LITTLE(key + 4) & ABC_A_MASK);
    u32 d1 =
        ABC_B_KEYSETUP_D1 ^
        ((U8TO32_LITTLE(key + 12) & U32C(0xffff0000)) >> 14);
    u32 x = U8TO32_LITTLE(key + 8);
    u32 z0 = U8TO32_LITTLE(key + 4) | ABC_A_MASK;
    u32 z1 = U8TO32_LITTLE(key);
    /* helper variables */
    u32 r, s;

    /* warm up */
    ABC_WARMUP_WORD(x);
    ABC_WARMUP_WORD(d0);
    d0 |= ABC_B_COEF_OR_MASK;
    ABC_WARMUP_WORD(d1);
    d1 &= ABC_B_COEF_AND_MASK;
    ABC_WARMUP_WORD(z0);
    z0 |= ABC_A_MASK;
    ABC_WARMUP_WORD(z1);
    /* fill ABC state */
    ABC_KEYSETUP_WORD(e[32]);
    ABC_KEYSETUP_WORD(e[0]);
    ABC_KEYSETUP_WORD(e[1]);
    ABC_KEYSETUP_WORD(e[2]);
    ABC_KEYSETUP_WORD(e[3]);
    ABC_KEYSETUP_WORD(e[4]);
    ABC_KEYSETUP_WORD(e[5]);
    ABC_KEYSETUP_WORD(e[6]);
    ABC_KEYSETUP_WORD(e[7]);
    ABC_KEYSETUP_WORD(e[8]);
    ABC_KEYSETUP_WORD(e[9]);
    ABC_KEYSETUP_WORD(e[10]);
    ABC_KEYSETUP_WORD(e[11]);
    ABC_KEYSETUP_WORD(e[12]);
    ABC_KEYSETUP_WORD(e[13]);
    ABC_KEYSETUP_WORD(e[14]);
    ABC_KEYSETUP_WORD(e[15]);
    ABC_KEYSETUP_WORD(e[16]);
    ABC_KEYSETUP_WORD(e[17]);
    ABC_KEYSETUP_WORD(e[18]);
    ABC_KEYSETUP_WORD(e[19]);
    ABC_KEYSETUP_WORD(e[20]);
    ABC_KEYSETUP_WORD(e[21]);
    ABC_KEYSETUP_WORD(e[22]);
    ABC_KEYSETUP_WORD(e[23]);
    ABC_KEYSETUP_WORD(e[24]);
    ABC_KEYSETUP_WORD(e[25]);
    ABC_KEYSETUP_WORD(e[26]);
    ABC_KEYSETUP_WORD(e[27]);
    ABC_KEYSETUP_WORD(e[28]);
    ABC_KEYSETUP_WORD(e[29]);
    ABC_KEYSETUP_WORD(e[30]);
    ABC_KEYSETUP_WORD(e[31]);
    ABC_KEYSETUP_WORD(ctx->d0);
    ABC_KEYSETUP_WORD(ctx->d1);
    ABC_KEYSETUP_WORD(ctx->x);
    ABC_KEYSETUP_WORD(ctx->z0);
    ABC_KEYSETUP_WORD(ctx->z1);

    /* apply restrictions and save changeable part of state */
    ctx->z0i = ctx->z0 = ctx->z0 | ABC_A_MASK;
    ctx->z1i = ctx->z1;
    ctx->xi = ctx->x;
    ctx->d0i = ctx->d0 = ctx->d0 | ABC_B_COEF_OR_MASK;
    ctx->d1i = ctx->d1 = ctx->d1 & ABC_B_COEF_AND_MASK;
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
    u32 d0 =
        ctx->d0i ^
        ((U8TO32_LITTLE(iv + 12) & U32C(0x0000ffff)) << 2) ^
        (U8TO32_LITTLE(iv + 4) & ABC_A_MASK);
    u32 d1 =
        ctx->d1i ^ ((U8TO32_LITTLE(iv + 12) & U32C(0xffff0000)) >> 14);
    u32 x = ctx->xi ^ U8TO32_LITTLE(iv + 8);
    u32 z0 = (ctx->z0i ^ U8TO32_LITTLE(iv + 4)) | ABC_A_MASK;
    u32 z1 = ctx->z1i ^ U8TO32_LITTLE(iv);

    u32 *table = ctx->t;
    u32 r, s;
    /* warm up */
    ABC_WARMUP_WORD(x);
    ABC_WARMUP_WORD(d0);
    d0 |= ABC_B_COEF_OR_MASK;
    ABC_WARMUP_WORD(d1);
    d1 &= ABC_B_COEF_AND_MASK;
    ABC_WARMUP_WORD(z0);
    z0 |= ABC_A_MASK;
    ABC_WARMUP_WORD(z1);
    /* update the state */
    ctx->z0 = z0;
    ctx->z1 = z1;
    ctx->x = x;
    ctx->d0 = d0;
    ctx->d1 = d1;
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
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 *table = ctx->t;
    /* helper variables */
    u32 i, j, r, s = 0;
    u8 buf[4];
#if defined(ABC_UNROLL_1)
    for (i = 0; i < (msglen & U32C(0xfffffffc)); i += 4)
    {
        ABC_PROCESS_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    U32TO8_LITTLE(buf, r);
    for (j = 0; j < (msglen & U32C(0x00000003)); ++j)
        (output + i)[j] = (input + i)[j] ^ buf[j];
#elif defined(ABC_UNROLL_4)
    for (i = 0; i < (msglen & U32C(0xfffffff0)); i += 16)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
    }
    for (; i < (msglen & U32C(0xfffffffc)); i += 4)
    {
        ABC_PROCESS_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    U32TO8_LITTLE(buf, r);
    for (j = 0; j < (msglen & U32C(0x00000003)); ++j)
        (output + i)[j] = (input + i)[j] ^ buf[j];
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < (msglen & U32C(0xffffffe0)); i += 32)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
        ABC_PROCESS_WORD(i + 16);
        ABC_PROCESS_WORD(i + 20);
        ABC_PROCESS_WORD(i + 24);
        ABC_PROCESS_WORD(i + 28);
    }
    for (; i < (msglen & U32C(0xfffffffc)); i += 4)
    {
        ABC_PROCESS_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    U32TO8_LITTLE(buf, r);
    for (j = 0; j < (msglen & U32C(0x00000003)); ++j)
        (output + i)[j] = (input + i)[j] ^ buf[j];
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < (msglen & U32C(0xffffffc0)); i += 64)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
        ABC_PROCESS_WORD(i + 16);
        ABC_PROCESS_WORD(i + 20);
        ABC_PROCESS_WORD(i + 24);
        ABC_PROCESS_WORD(i + 28);
        ABC_PROCESS_WORD(i + 32);
        ABC_PROCESS_WORD(i + 36);
        ABC_PROCESS_WORD(i + 40);
        ABC_PROCESS_WORD(i + 44);
        ABC_PROCESS_WORD(i + 48);
        ABC_PROCESS_WORD(i + 52);
        ABC_PROCESS_WORD(i + 56);
        ABC_PROCESS_WORD(i + 60);
    }
    for (; i < (msglen & U32C(0xfffffffc)); i += 4)
    {
        ABC_PROCESS_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    U32TO8_LITTLE(buf, r);
    for (j = 0; j < (msglen & U32C(0x00000003)); ++j)
        (output + i)[j] = (input + i)[j] ^ buf[j];
#endif /* ABC_UNROLL_1 */
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
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 *table = ctx->t;
    /* helper variables */
    u32 i, r, s = 0;
#if defined(ABC_UNROLL_1)
    for (i = 0; i < (length & U32C(0xfffffffc)); i += 4)
    {
        ABC_KEYSTREAM_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    memcpy(keystream + i, &r, length & U32C(0x00000003));
#elif defined(ABC_UNROLL_4)
    for (i = 0; i < (length & U32C(0xfffffff0)); i += 16)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
    }
    for (; i < (length & U32C(0xfffffffc)); i += 4)
    {
        ABC_KEYSTREAM_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    memcpy(keystream + i, &r, length & U32C(0x00000003));
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < (length & U32C(0xffffffe0)); i += 32)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
        ABC_KEYSTREAM_WORD(i + 16);
        ABC_KEYSTREAM_WORD(i + 20);
        ABC_KEYSTREAM_WORD(i + 24);
        ABC_KEYSTREAM_WORD(i + 28);
    }
    for (; i < (length & U32C(0xfffffffc)); i += 4)
    {
        ABC_KEYSTREAM_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    memcpy(keystream + i, &r, length & U32C(0x00000003));
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < (length & U32C(0xffffffc0)); i += 64)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
        ABC_KEYSTREAM_WORD(i + 16);
        ABC_KEYSTREAM_WORD(i + 20);
        ABC_KEYSTREAM_WORD(i + 24);
        ABC_KEYSTREAM_WORD(i + 28);
        ABC_KEYSTREAM_WORD(i + 32);
        ABC_KEYSTREAM_WORD(i + 36);
        ABC_KEYSTREAM_WORD(i + 40);
        ABC_KEYSTREAM_WORD(i + 44);
        ABC_KEYSTREAM_WORD(i + 48);
        ABC_KEYSTREAM_WORD(i + 52);
        ABC_KEYSTREAM_WORD(i + 56);
        ABC_KEYSTREAM_WORD(i + 60);
    }
    for (; i < (length & U32C(0xfffffffc)); i += 4)
    {
        ABC_KEYSTREAM_WORD(i);
    }
    ABC_UPPER_PART(table);
    r = ABC_OUTPUT_WORD(table);
    r = ABC_POSTFILTER(r);
    memcpy(keystream + i, &r, length & U32C(0x00000003));
#endif /* ABC_UNROLL_1 */
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
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 *table = ctx->t;
    /* helper variables */
    u32 i, r, s = 0;
#if defined(ABC_UNROLL_1)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 4)
    {
        ABC_PROCESS_WORD(i);
    }
#elif defined(ABC_UNROLL_4)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 16)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 32)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
        ABC_PROCESS_WORD(i + 16);
        ABC_PROCESS_WORD(i + 20);
        ABC_PROCESS_WORD(i + 24);
        ABC_PROCESS_WORD(i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 64)
    {
        ABC_PROCESS_WORD(i);
        ABC_PROCESS_WORD(i + 4);
        ABC_PROCESS_WORD(i + 8);
        ABC_PROCESS_WORD(i + 12);
        ABC_PROCESS_WORD(i + 16);
        ABC_PROCESS_WORD(i + 20);
        ABC_PROCESS_WORD(i + 24);
        ABC_PROCESS_WORD(i + 28);
        ABC_PROCESS_WORD(i + 32);
        ABC_PROCESS_WORD(i + 36);
        ABC_PROCESS_WORD(i + 40);
        ABC_PROCESS_WORD(i + 44);
        ABC_PROCESS_WORD(i + 48);
        ABC_PROCESS_WORD(i + 52);
        ABC_PROCESS_WORD(i + 56);
        ABC_PROCESS_WORD(i + 60);
    }
#endif /* ABC_UNROLL_1 */
    /* state update */
    ctx->z0 = z0;
    ctx->z1 = z1;
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
    u32 x = ctx->x;
    u32 d0 = ctx->d0;
    u32 d1 = ctx->d1;
    u32 *table = ctx->t;
    /* helper variables */
    u32 i, r, s = 0;
#if defined(ABC_UNROLL_1)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 4)
    {
        ABC_KEYSTREAM_WORD(i);
    }
#elif defined(ABC_UNROLL_4)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 16)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
    }
#elif defined(ABC_UNROLL_8)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 32)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
        ABC_KEYSTREAM_WORD(i + 16);
        ABC_KEYSTREAM_WORD(i + 20);
        ABC_KEYSTREAM_WORD(i + 24);
        ABC_KEYSTREAM_WORD(i + 28);
    }
#elif defined(ABC_UNROLL_16)
    for (i = 0; i < blocks * ECRYPT_BLOCKLENGTH; i += 64)
    {
        ABC_KEYSTREAM_WORD(i);
        ABC_KEYSTREAM_WORD(i + 4);
        ABC_KEYSTREAM_WORD(i + 8);
        ABC_KEYSTREAM_WORD(i + 12);
        ABC_KEYSTREAM_WORD(i + 16);
        ABC_KEYSTREAM_WORD(i + 20);
        ABC_KEYSTREAM_WORD(i + 24);
        ABC_KEYSTREAM_WORD(i + 28);
        ABC_KEYSTREAM_WORD(i + 32);
        ABC_KEYSTREAM_WORD(i + 36);
        ABC_KEYSTREAM_WORD(i + 40);
        ABC_KEYSTREAM_WORD(i + 44);
        ABC_KEYSTREAM_WORD(i + 48);
        ABC_KEYSTREAM_WORD(i + 52);
        ABC_KEYSTREAM_WORD(i + 56);
        ABC_KEYSTREAM_WORD(i + 60);
    }
#endif /* ABC_UNROLL_1 */
    /* state update */
    ctx->z0 = z0;
    ctx->z1 = z1;
    ctx->x = x;
}
