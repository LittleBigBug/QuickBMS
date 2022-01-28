/* CryptMT Stream Cipher, Relying SFMT */
/* By Matsumoto-Saito-Nishimura-Hagita */
/* SFMT included */
/* 2006/6/14 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ecrypt-config.h"
#include "ecrypt-machine.h"
#include "ecrypt-portable.h"

#include "ecrypt-sync.h"
/* #define SLOW_CODE 1 */
#if defined(__GNUC__)
#define INLINE __inline__
#else
#define INLINE
#endif

/* Period parameters */
#define N 156
#include "params.h"
struct SFMT_T {
    u32 sfmt[N][4];
};
typedef struct SFMT_T sfmt_t;


/* prototypes */
static int is_simd_cpu(void);
static INLINE void do_recursion(u32 a[4], const u32 b[4], const u32 c[4]);
static void genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
				u32 len);
static void fast_genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[],
				     const u8 plain[], u32 len);
static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[], 
			  u32 len);
static INLINE void fast_genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[],
				      const u8 plain[], u32 len);
static void genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
				const u8 plain[]);
static void fast_genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
					    const u8 plain[]);
static INLINE void genrand_block(ECRYPT_ctx * ctx, u8 cipher[],
				 const u8 plain[]);
static INLINE void fast_genrand_block(ECRYPT_ctx * ctx, u8 cipher[],
				      const u8 plain[]);
static INLINE void boot_up(ECRYPT_ctx *ctx, s32 length);
static INLINE void fast_boot_up(ECRYPT_ctx *ctx, s32 length);
static void filter_16bytes(u32 sfmt[], u32 accum[4], u8 cipher[],
			   const u8 plain[], s32 count);
static INLINE void filter_bytes(u32 sfmt[], u32 accum[4], u8 cipher[],
				const u8 plain[], s32 len);
static INLINE void booter_am(u32 acc[4], u32 pos1[][4], u32 pos2[][4],
			     s32 count);

#if (!defined(SLOW_CODE)) && defined(__ppc__) && defined(__ALTIVEC__)
static int fast_code = 1;
#include "altivec.c"
#elif (!defined(SLOW_CODE)) && defined(__INTEL_COMPILER)
static int fast_code = 1;
#include "sse2.c"
#elif (!defined(SLOW_CODE)) && defined(__GNUC__) && defined(__SSE__)
static int fast_code = 1;
#include "sse2.c"
#else  /* other normal C */
static int fast_code = 0;
static int is_simd_cpu(void) {
    return 0;
}

static void fast_genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[],
					    const u8 plain[], u32 len) {
    fprintf(stderr, "ERROR:fast_genrand_bytes_first not implemented\n");
    exit(1);
}

static INLINE void fast_genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[],
				      const u8 plain[], u32 len) {
    fprintf(stderr, "ERROR:fast_genrand_bytes not implemented\n");
    exit(1);
}

static void fast_genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
					    const u8 plain[]) {
    fprintf(stderr, "ERROR:fast_genrand_block_first not implemented\n");
    exit(1);
}

static INLINE void fast_genrand_block(ECRYPT_ctx * ctx, u8 cipher[],
				      const u8 plain[]) {
    fprintf(stderr, "ERROR:fast_genrand_block not implemented\n");
    exit(1);
}

static INLINE void fast_boot_up(ECRYPT_ctx *ctx, s32 length) {
    fprintf(stderr, "ERROR:fast_boot_up not implemented\n");
    exit(1);
}

#endif

/**********************
 * IMPLEMRNT FUNCTIONS
 **********************/
static void genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
				u32 len)
{
    s32 i, p, count;
    sfmt_t *sp;
 
    count = (len + 7) / 8;
    sp = (sfmt_t *)ctx->psfmt;
    p = ctx->length - 2;
    booter_am(ctx->lung, &sp->sfmt[0], &sp->sfmt[p], count);
    filter_16bytes(sp->sfmt[0], ctx->accum, cipher, plain, len / 16);
    i = (len / 16) * 2;
    len = len % 16;
    if (len != 0) {
	filter_bytes(sp->sfmt[i], ctx->accum, &cipher[i * 8],
		     &plain[i * 8], len);
    }
}

static INLINE void do_recursion(u32 a[4], const u32 b[4], const u32 c[4]) {
    u64 t;
    u32 bb[4];
    u32 tmp;

    t = ((u64)b[1] << 32) | ((u64)b[0]);
    t = t >> SHIFT64;
    bb[0] = (u32)t;
    bb[1] = (u32)(t >> 32);
    t = ((u64)b[3] << 32) | ((u64)b[2]);
    t = t >> SHIFT64;
    bb[2] = (u32)t;
    bb[3] = (u32)(t >> 32);
    tmp = a[0];
    a[0] = a[1] ^ b[1] ^ bb[0] ^ (c[0] & MSK1);
    a[1] = a[2] ^ b[3] ^ bb[1] ^ (c[1] & MSK2);
    a[2] = a[3] ^ b[0] ^ bb[2] ^ (c[2] & MSK3);
    a[3] = tmp  ^ b[2] ^ bb[3] ^ (c[3] & MSK4);
}

static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len)
{
    u32 *accum;
    sfmt_t *ps;
    int i;
    s32 count;
    accum = ctx->accum;
    ps = (sfmt_t *)ctx->psfmt;
    count = (len + 7) / 8;
 
    do_recursion(ps->sfmt[0], ps->sfmt[POS1], ps->sfmt[N - 1]);
    count--;
    for (i = 1; (count > 0) && (i < N - POS1); i++, count--) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1], ps->sfmt[i - 1]);
    }
    for (; (count > 0) && (i < N); i++, count--) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1 - N], ps->sfmt[i - 1]);
    }

    filter_16bytes(ps->sfmt[0], ctx->accum, cipher, plain, len / 16);
    i = (len / 16) * 2;
    len = len % 16;
    if (len != 0) {
	filter_bytes(ps->sfmt[i], ctx->accum, &cipher[i * 8],
		     &plain[i * 8], len);
    }
}

static void genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[])
{
    s32 p;
    sfmt_t *ps;
    int i;

    ps = (sfmt_t *)ctx->psfmt;
    p = ctx->length - 2;
    booter_am(ctx->lung, &ps->sfmt[0], &ps->sfmt[p], N);
    filter_16bytes(ps->sfmt[0], ctx->accum, cipher, plain, N / 2);
    ps->sfmt[0][3] = INIL;
    for (i = 0; i < 4; i++) {
	ps->sfmt[N][i] = ps->sfmt[0][i];
    }
    do_recursion(ps->sfmt[N], ps->sfmt[POS1], ps->sfmt[N - 1]);
    ctx->psfmt = ps->sfmt[1];
}

static void filter_16bytes(u32 sfmt[], u32 accum[4], u8 cipher[],
			   const u8 plain[], s32 count) {
    u32 t1, t2, t3, t4;
    u32 ac1, ac2, ac3, ac4;
    int i;

    ac1 = accum[0];
    ac2 = accum[1];
    ac3 = accum[2];
    ac4 = accum[3];

    for (i = 0; i < count; i++) {
	t1 = ac1;
	ac1 = ac1 ^ (ac2 >> 1);
	ac2 = ac2 ^ (ac3 >> 1);
	ac3 = ac3 ^ (ac4 >> 1);
	ac4 = ac4 ^ (t1 >> 1);
	ac1 = (2 * ac1 + 1) * sfmt[0] + ac1;
	ac2 = (2 * ac2 + 1) * sfmt[1] + ac2;
	ac3 = (2 * ac3 + 1) * sfmt[2] + ac3;
	ac4 = (2 * ac4 + 1) * sfmt[3] + ac4;
	t1 = (ac1 >> 16) ^ ac1;
	t2 = (ac2 >> 16) ^ ac2;
	t3 = (ac3 >> 16) ^ ac3;
	t4 = (ac4 >> 16) ^ ac4;
	cipher[0] = (u8)(plain[0] ^ (u8)(t1));
	cipher[1] = (u8)(plain[1] ^ (u8)(t1 >> 8));
	cipher[4] = (u8)(plain[4] ^ (u8)(t2));
	cipher[5] = (u8)(plain[5] ^ (u8)(t2 >> 8));
	cipher[8] = (u8)(plain[8] ^ (u8)(t3));
	cipher[9] = (u8)(plain[9] ^ (u8)(t3 >> 8));
	cipher[12] = (u8)(plain[12] ^ (u8)(t4));
	cipher[13] = (u8)(plain[13] ^ (u8)(t4 >> 8));

	t1 = ac1;
	ac1 = ac1 ^ (ac2 >> 1);
	ac2 = ac2 ^ (ac3 >> 1);
	ac3 = ac3 ^ (ac4 >> 1);
	ac4 = ac4 ^ (t1 >> 1);
	ac1 = (2 * ac1 + 1) * sfmt[4] + ac1;
	ac2 = (2 * ac2 + 1) * sfmt[5] + ac2;
	ac3 = (2 * ac3 + 1) * sfmt[6] + ac3;
	ac4 = (2 * ac4 + 1) * sfmt[7] + ac4;
	t1 = (ac1 >> 16) ^ ac1;
	t2 = (ac2 >> 16) ^ ac2;
	t3 = (ac3 >> 16) ^ ac3;
	t4 = (ac4 >> 16) ^ ac4;
	cipher[2] = (u8)(plain[2] ^ (u8)(t1));
	cipher[3] = (u8)(plain[3] ^ (u8)(t1 >> 8));
	cipher[6] = (u8)(plain[6] ^ (u8)(t2));
	cipher[7] = (u8)(plain[7] ^ (u8)(t2 >> 8));
	cipher[10] = (u8)(plain[10] ^ (u8)(t3));
	cipher[11] = (u8)(plain[11] ^ (u8)(t3 >> 8));
	cipher[14] = (u8)(plain[14] ^ (u8)(t4));
	cipher[15] = (u8)(plain[15] ^ (u8)(t4 >> 8));
	sfmt += 8;
	cipher += 16;
	plain += 16;
    }
    accum[0] = ac1;
    accum[1] = ac2;
    accum[2] = ac3;
    accum[3] = ac4;
}

static INLINE void filter_bytes(u32 sfmt[], u32 accum[4], u8 cipher[],
				const u8 plain[], int len) {
    u32 t1, t2, t3, t4, t5, t6, t7, t8;

    t1 = accum[0];
    accum[0] = accum[0] ^ (accum[1] >> 1);
    accum[1] = accum[1] ^ (accum[2] >> 1);
    accum[2] = accum[2] ^ (accum[3] >> 1);
    accum[3] = accum[3] ^ (t1 >> 1);
    accum[0] = (2 * accum[0] + 1) * sfmt[0] + accum[0];
    accum[1] = (2 * accum[1] + 1) * sfmt[1] + accum[1];
    accum[2] = (2 * accum[2] + 1) * sfmt[2] + accum[2];
    accum[3] = (2 * accum[3] + 1) * sfmt[3] + accum[3];
    t1 = (accum[0] >> 16) ^ accum[0];
    t2 = (accum[1] >> 16) ^ accum[1];
    t3 = (accum[2] >> 16) ^ accum[2];
    t4 = (accum[3] >> 16) ^ accum[3];

    t5 = accum[0];
    accum[0] = accum[0] ^ (accum[1] >> 1);
    accum[1] = accum[1] ^ (accum[2] >> 1);
    accum[2] = accum[2] ^ (accum[3] >> 1);
    accum[3] = accum[3] ^ (t5 >> 1);
    accum[0] = (2 * accum[0] + 1) * sfmt[4] + accum[0];
    accum[1] = (2 * accum[1] + 1) * sfmt[5] + accum[1];
    accum[2] = (2 * accum[2] + 1) * sfmt[6] + accum[2];
    accum[3] = (2 * accum[3] + 1) * sfmt[7] + accum[3];
    t5 = (accum[0] >> 16) ^ accum[0];
    t6 = (accum[1] >> 16) ^ accum[1];
    t7 = (accum[2] >> 16) ^ accum[2];
    t8 = (accum[3] >> 16) ^ accum[3];

    cipher[0] = (u8)(plain[0] ^ (u8)(t1));
    if (--len == 0) return;
    cipher[1] = (u8)(plain[1] ^ (u8)(t1 >> 8));
    if (--len == 0) return;
    cipher[2] = (u8)(plain[2] ^ (u8)(t5));
    if (--len == 0) return;
    cipher[3] = (u8)(plain[3] ^ (u8)(t5 >> 8));
    if (--len == 0) return;
    cipher[4] = (u8)(plain[4] ^ (u8)(t2));
    if (--len == 0) return;
    cipher[5] = (u8)(plain[5] ^ (u8)(t2 >> 8));
    if (--len == 0) return;
    cipher[6] = (u8)(plain[6] ^ (u8)(t6));
    if (--len == 0) return;
    cipher[7] = (u8)(plain[7] ^ (u8)(t6 >> 8));
    if (--len == 0) return;
    cipher[8] = (u8)(plain[8] ^ (u8)(t3));
    if (--len == 0) return;
    cipher[9] = (u8)(plain[9] ^ (u8)(t3 >> 8));
    if (--len == 0) return;
    cipher[10] = (u8)(plain[10] ^ (u8)(t7));
    if (--len == 0) return;
    cipher[11] = (u8)(plain[11] ^ (u8)(t7 >> 8));
    if (--len == 0) return;
    cipher[12] = (u8)(plain[12] ^ (u8)(t4));
    if (--len == 0) return;
    cipher[13] = (u8)(plain[13] ^ (u8)(t4 >> 8));
    if (--len == 0) return;
    cipher[14] = (u8)(plain[14] ^ (u8)(t8));
    if (--len == 0) return;
    cipher[15] = (u8)(plain[15] ^ (u8)(t8 >> 8));
}


static INLINE void genrand_block(ECRYPT_ctx * ctx, u8 cipher[],
				 const u8 plain[])
{
    int i;
    sfmt_t *ps;

    ps = (sfmt_t *)ctx->psfmt;
    do_recursion(ps->sfmt[0], ps->sfmt[POS1], ps->sfmt[N - 1]);
    for (i = 1; i < N - POS1; i++) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1], ps->sfmt[i - 1]);
    }
    for (; i < N; i++) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1 - N], ps->sfmt[i - 1]);
    }
    filter_16bytes(ps->sfmt[0], ctx->accum, cipher, plain, N / 2);
}

/********* END of SFMT **********/

static INLINE void booter_am(u32 acc[4], u32 pos1[][4], u32 pos2[][4],
			     s32 count)
{
    u32 a[4], b[4];
    u32 tmp;
    int i, j;

    for (i = 0; i < count; i++) {
	for (j = 0; j < 4; j++) {
	    pos1[i][j] = a[j] = pos1[i][j] + pos2[i][j];
	}
	tmp = a[0];
	a[0] = a[3] ^ (a[0] >> 13);
	a[3] = a[2] ^ (a[3] >> 13);
	a[2] = a[1] ^ (a[2] >> 13);
	a[1] = tmp  ^ (a[1] >> 13);
	b[0] = pos2[i + 1][3] ^ (pos2[i + 1][0] >> 11);
	b[1] = pos2[i + 1][2] ^ (pos2[i + 1][1] >> 11);
	b[2] = pos2[i + 1][0] ^ (pos2[i + 1][2] >> 11);
	b[3] = pos2[i + 1][1] ^ (pos2[i + 1][3] >> 11);
	for (j = 0; j < 4; j++) {
	    acc[j] = (2 * b[j] + 1) * acc[j] + b[j];
	    pos2[i + 2][j] = a[j] - acc[j];
	}
    }
}

static void boot_up(ECRYPT_ctx *ctx, s32 length)
{
    s32 i, p;

    ctx->psfmt = ctx->sfmt[length + 2];
    p = ctx->ivsize / 4;
    for (i = 0; i < 4; i++) {
	ctx->lung[i] = ctx->sfmt[p * 4][i] | 1;
    }
    p = length - 2;
    booter_am(ctx->lung, &ctx->sfmt[0], &ctx->sfmt[p], length + 2);
    for (i = 0; i < 4; i++) {
	ctx->accum[i] = ctx->sfmt[2 * length + 1][i];
    }
}

/************************
 * ECRYPT API FUNCTIONS
 ************************/
void ECRYPT_init(void)
{
    if (fast_code) {
	fast_code = is_simd_cpu();
    }
}

/* Key size in bits. */
/* IV size in bits. */
void ECRYPT_keysetup(ECRYPT_ctx * ctx, const u8 * key, u32 keysize,
		     u32 ivsize)
{
    s32 i;

    memset(ctx, 0, sizeof(ctx));
    ctx->keysize = keysize / 128;
    ctx->ivsize = ivsize / 128;
    for (i = 0; i < keysize / 32; i++) {
	ctx->key[i] = U8TO32_LITTLE(key);
	key += 4;
    }
}

void ECRYPT_ivsetup(ECRYPT_ctx * ctx, const u8 * iv)
{
    s32 p, ivsize, keysize, i, j;
    u32 block_size;
 
    ivsize = ctx->ivsize;
    keysize = ctx->keysize;
    block_size = ivsize + keysize;
    for (i = 0; i < ivsize; i++) {
	for (j = 0; j < 4; j++) {
	    ctx->sfmt[i][j] = U8TO32_LITTLE(iv);
	    iv += 4;
	}
    }
    memcpy(ctx->sfmt[ivsize], ctx->key, keysize * 16);
    memcpy(ctx->sfmt[block_size], ctx->sfmt, block_size * 16);
    p = 2 * block_size -1;
    ctx->sfmt[p][0] += 314159UL;
    ctx->sfmt[p][1] += 265358UL;
    ctx->sfmt[p][2] += 979323UL;
    ctx->sfmt[p][3] += 846264UL;
    ctx->length = block_size * 2;
    if (fast_code) {
	fast_boot_up(ctx, ctx->length);
    } else {
	boot_up(ctx, ctx->length);
    }
    ctx->first = 1;
}

/* Message length in bytes. */
void ECRYPT_encrypt_bytes(ECRYPT_ctx * ctx,
			  const u8 * plaintext,
			  u8 * ciphertext, u32 msglen)
{
    if (fast_code) {
	if (ctx->first && (msglen > 0)) {
	    if (msglen >= ECRYPT_BLOCKLENGTH) {
	    	fast_genrand_block_first(ctx, ciphertext, plaintext);
		ciphertext += ECRYPT_BLOCKLENGTH;
		plaintext += ECRYPT_BLOCKLENGTH;
		msglen -= ECRYPT_BLOCKLENGTH;
		ctx->first = 0;
	    } else {
		fast_genrand_bytes_first(ctx, ciphertext, plaintext, msglen);
		return;
	    }
	}
	while (msglen >= ECRYPT_BLOCKLENGTH) {
	    fast_genrand_block(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	    msglen -= ECRYPT_BLOCKLENGTH;
	}
	if (msglen != 0) {
	    fast_genrand_bytes(ctx, ciphertext, plaintext, msglen);
	}
    } else {
	if (ctx->first && (msglen > 0)) {
	    if (msglen >= ECRYPT_BLOCKLENGTH) {
	    	genrand_block_first(ctx, ciphertext, plaintext);
		ciphertext += ECRYPT_BLOCKLENGTH;
		plaintext += ECRYPT_BLOCKLENGTH;
		msglen -= ECRYPT_BLOCKLENGTH;
		ctx->first = 0;
	    } else {
		genrand_bytes_first(ctx, ciphertext, plaintext, msglen);
		return;
	    }
	}
	while (msglen >= ECRYPT_BLOCKLENGTH) {
	    genrand_block(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	    msglen -= ECRYPT_BLOCKLENGTH;
	}
	if (msglen != 0) {
	    genrand_bytes(ctx, ciphertext, plaintext, msglen);
	}
    }
}

/* Message length in bytes. */
void ECRYPT_decrypt_bytes(ECRYPT_ctx * ctx,
			  const u8 * ciphertext,
			  u8 * plaintext, u32 msglen)
{
    ECRYPT_encrypt_bytes(ctx, ciphertext, plaintext, msglen);
}

/* Message length in blocks. */
void ECRYPT_encrypt_blocks(ECRYPT_ctx * ctx,
			   const u8 * plaintext,
			   u8 * ciphertext, u32 blocks)
{
    s32 i;
    
    if (fast_code) {
	if (ctx->first && (blocks > 0)) {
	    fast_genrand_block_first(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	    blocks--;
	    ctx->first = 0;
	}
	for (i = 0; i < blocks; i++) {
	    fast_genrand_block(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	}
    } else {
	if (ctx->first && (blocks > 0)) {
	    genrand_block_first(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	    blocks--;
	    ctx->first = 0;
	}
	for (i = 0; i < blocks; i++) {
	    genrand_block(ctx, ciphertext, plaintext);
	    ciphertext += ECRYPT_BLOCKLENGTH;
	    plaintext += ECRYPT_BLOCKLENGTH;
	}
    }
}

/* Message length in blocks. */
void ECRYPT_decrypt_blocks(ECRYPT_ctx * ctx,
			   const u8 * ciphertext,
			   u8 * plaintext, u32 blocks)
{
    ECRYPT_encrypt_blocks(ctx, ciphertext, plaintext, blocks);
}
