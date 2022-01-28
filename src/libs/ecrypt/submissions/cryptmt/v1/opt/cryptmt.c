/* CryptMT Stream Cipher, Relying Mersenne Twister
 * By Hagita-Matsumoto-Nishimura-Saito
 * MT included
 * 2005/04/16
 * optimized version 2005/12/27
 * Copyright 2005, Matsumoto, Hagita, Nishimura, Saito
 * All rights reserved.
 */
/*
  LICENSE CONDITION

  1. A patent on this algorithm is pending as of 2005 May. The  
  intellectual property belongs to Hiroshima University and Ochanomizu  
  University.

  2. This software is free for non-commercial use.
  2-1 "Non-commercial use" explicitly includes research and educational  
  purposes, as well as use in software which is made available for free.
  2-2 "Non-commercial use" also includes any other kind of use, except  
  those stated in item 3.

  3. Contacting the inventors is required for commercial use, as  
  defined in item 3-1.
  3-1. In the case of selling software or hardware which utilizes this  
  software or this algorithm.

  4. Royalties may be required in cases of commercial use as defined in  
  item 3. However with the exception of a few cases as stated in  
  Background, below, it is our intention to allow general royalty-free  
  use.

  5. We disclaim any responsibility for any direct or indirect damages  
  caused by this software or algorithm.

  Background:
  A. The inventors of this algorithm are Makoto Matsumoto (Hiroshima  
  University), Mariko Hagita (Ochanomizu University), Takuji Nishimura  
  (Yamagata University), and Mutsuo Saito (Hiroshima University).  
  (Affiliations as of 2005 Dec.)
  
  B. By the regulations of Hiroshima University and Ochanomizu  
  University, all inventions by the worker(s) are examined with respect  
  to patent, and their intellectual property is owned by the university 
  (ies), if deemed necessary.

  C. The inventors' wish is to allow the algorithm/software to be as  
  freely and widely used as possible.

  D. The desire of the intellectual property centers (IPCs) of  
  Hiroshima University and Ochanomizu University is to obtain the  
  minimum amount of income necessary to cover the expense for the  
  patent application, maintaining the patent, and continuing this  
  research.

  E. Thus, we plan to make this algorithm/software free even for  
  commercial uses, if the IPCs can obtain this amount of income from  
  large companies (such as Microsoft or Apple) or governmental  
  organizations, in the form of royalties or contributions.

  F. The names of companies and organizations who pay royalties or make  
  contributions will be included in this program when it is revised,  
  under the consent of the inventors, and will be advertised as  
  sponsors on the home page of this algorithm.

  G. This algorithm is an applicant for the eSTREAM stream cipher  
  project http://www.ecrypt.eu.org/stream/. If eSTREAM selects this  
  algorithm as one of the recommendable stream ciphers in its final  
  report planned for 2008 January, then we will make this algorithm/ 
  software free even for commercial use, regardless of the condition in  
  E. However, preferably we hope to recieve some royalties or  
  contributions to satisfy the desire in item D.

  These statements are claimed by us, namely, the Inventors listed in  
  item A, the IPC of Hiroshima University, and the IPC of Ochanomizu  
  University.

  Corresponding address: 
  Makoto Matsumoto, Department of Mathematics, Hiroshima University.
  1-3-1 Kagamiyama, Higashi-Hiroshima 739-8526 Japan.
  http://www.math.sci.hiroshima-u.ac.jp/~m-mat/eindex.html
  email: m-mat "at-mark" math.sci.hiroshima-u.ac.jp
*/

#include <stdio.h>
#include <string.h>

#include "ecrypt-config.h"
#include "ecrypt-machine.h"
#include "ecrypt-portable.h"

#include "ecrypt-sync.h"


/**********Start of MT ************/
/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A U32C(0x9908B0DF)	/* constant vector a */
#define UPPER_MASK U32C(0x80000000)	/* most significant w-r bits */
#define LOWER_MASK U32C(0x7FFFFFFF)	/* least significant r bits */

/* prototypes */
static void init_genrand(ECRYPT_ctx * ctx, u32 s);
static void init_by_array(ECRYPT_ctx * ctx, const u32 init_key[],
			  int key_length);
static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len);
static void genrand_keystream_bytes(ECRYPT_ctx * ctx, u8 cipher[], u32 len);
static void genrand_accum(ECRYPT_ctx * ctx, u32 len);
static void genrand_block(ECRYPT_ctx * ctx, u8 cipher[N], const u8 plain[N]);
static void genrand_keystream_block(ECRYPT_ctx * ctx, u8 cipher[N]);

/* initializes mt[N] with a seed */
static void init_genrand(ECRYPT_ctx * ctx, u32 s)
{
    u32 *mt;
    u32 mti;

    mt = ctx->mt;
    mt[0] = s & U32C(0xFFFFFFFF);
    for (mti = 1; mti < N; mti++) {
	mt[mti] =
	    (U32C(1812433253) * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
	/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
	/* In the previous versions, MSBs of the seed affect   */
	/* only MSBs of the array mt[].                        */
	/* 2002/01/09 modified by Makoto Matsumoto             */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
static void init_by_array(ECRYPT_ctx * ctx, const u32 init_key[], 
			  int key_length)
{
    int i, j, k;
    u32 *mt;

    mt = ctx->mt;
    init_genrand(ctx, U32C(19650218));
    i = 1;
    j = 0;
    k = (N > key_length ? N : key_length);
    for (; k; k--) {
	mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * U32C(1664525)))
	    + init_key[j] + j;	/* non linear */
	i++;
	j++;
	if (i >= N) {
	    mt[0] = mt[N - 1];
	    i = 1;
	}
	if (j >= key_length) {
	    j = 0;
	}
    }
    for (k = N - 1; k; k--) {
	mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * U32C(1566083941)))
	    - i;		/* non linear */
	i++;
	if (i >= N) {
	    mt[0] = mt[N - 1];
	    i = 1;
	}
    }

    mt[0] = U32C(0x80000000);	/* MSB is 1; assuring non-zero initial array */
}


/* generates whole array of random numbers in [0,0xffffffff]-interval */
/* len must be < N-M */
static void genrand_accum(ECRYPT_ctx * ctx, u32 len)
{
    u32 y;
    u32 accum = 1;
    u32 *mt;
    static const u32 mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    int kk;
    mt = ctx->mt;

    for (kk = 0; kk < len; kk++) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
    }
    ctx->accum = accum;
}

/* generates whole array of random numbers in [0,0xffffffff]-interval */
static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len)
{
    u32 y;
    u32 accum;
    u32 *mt;
    static const u32 mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    int kk;

    mt = ctx->mt;
    accum = ctx->accum;
    for (kk = 0; (kk < N - M) && (len > 0); kk++, len--) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8) (accum >> 24);
    }
    if (len <= 0) {
	return;
    }
    for (; (kk < N - 1) && (len > 0); kk++, len--) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8) (accum >> 24);
    }
    if (len <= 0) {
	return;
    }
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];
    accum *= mt[N - 1] | 0x1;
    cipher[N - 1] = plain[N - 1] ^ (u8) (accum >> 24);
}

/* generates whole array of random numbers in [0,0xffffffff]-interval */
static void genrand_keystream_bytes(ECRYPT_ctx * ctx, u8 cipher[], u32 len)
{
    u32 y;
    u32 accum;
    u32 *mt;
    static const u32 mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    int kk;

    mt = ctx->mt;
    accum = ctx->accum;
    for (kk = 0; (kk < N - M) && (len > 0); kk++, len--) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = (u8) (accum >> 24);
    }
    if (len <= 0) {
	return;
    }
    for (; (kk < N - 1) && (len > 0); kk++, len--) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = (u8) (accum >> 24);
    }
    if (len <= 0) {
	return;
    }
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];
    accum *= mt[N - 1] | 0x1;
    cipher[N - 1] = (u8) (accum >> 24);
}

static void genrand_block(ECRYPT_ctx * ctx, u8 cipher[N],
			  const u8 plain[N])
{
    u32 y;
    u32 accum;
    u32 *mt;
    static const u32 mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    int kk;
    mt = ctx->mt;
    accum = ctx->accum;

    for (kk = 0; kk < N - M; kk++) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8) (accum >> 24);
    }
    for (; kk < N - 1; kk++) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8) (accum >> 24);
    }
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];
    accum *= mt[N - 1] | 0x1;
    cipher[N - 1] = plain[N - 1] ^ (u8) (accum >> 24);
    ctx->accum = accum;
}

static void genrand_keystream_block(ECRYPT_ctx * ctx, u8 cipher[N])
{
    u32 y;
    u32 accum;
    u32 *mt;
    static const u32 mag01[2] = { 0x0, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    int kk;
    mt = ctx->mt;
    accum = ctx->accum;

    for (kk = 0; kk < N - M; kk++) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = (u8) (accum >> 24);
    }
    for (; kk < N - 1; kk++) {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
	accum *= mt[kk] | 0x1;
	cipher[kk] = (u8) (accum >> 24);
    }
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];
    accum *= mt[N - 1] | 0x1;
    cipher[N - 1] = (u8) (accum >> 24);
    ctx->accum = accum;
}

/********* END of MT **********/

/* This is a stream cipher. The algorithm is as follows. */
/* Generate 32 bit nonsecure random numbers by MT. */
/* Multiply three consequtive words, and use only */
/* the most significant 8 bits. */

void ECRYPT_init(void)
{
    /* do nothing */
}

/* Key size in bits. */
/* IV size in bits. */
void ECRYPT_keysetup(ECRYPT_ctx * ctx, const u8 * key, u32 keysize, u32 ivsize)
{
    int i;

    ctx->keysize = keysize / 32;
    ctx->ivsize = ivsize / 32;
    for (i = 0; i < ctx->keysize; i++) {
	ctx->key[i] = U8TO32_LITTLE(key);
	key += 4;
    }
}

void ECRYPT_ivsetup(ECRYPT_ctx * ctx, const u8 * iv)
{
    int i, t, s;
    u32 tmp[64];

    t = ctx->keysize;
    s = ctx->ivsize;
    for (i = 0; i < s; i++) {
	ctx->key[t + i] = U8TO32_LITTLE(iv);
	iv += 4;
    }

    init_by_array(ctx, ctx->key, t + s);
    genrand_accum(ctx, 64);
    memcpy(tmp, ctx->mt, sizeof(u32) * 64);
    memmove(ctx->mt, &(ctx->mt[64]), sizeof(u32) * (N - 64));
    memcpy(&(ctx->mt[N - 64]), tmp, sizeof(u32) * 64);
}

/* Message length in bytes. */
void ECRYPT_encrypt_bytes(ECRYPT_ctx * ctx, const u8 * plaintext,
			  u8 * ciphertext, u32 msglen)
{
    while (msglen >= N) {
	genrand_block(ctx, ciphertext, plaintext);
	ciphertext += N;
	plaintext += N;
	msglen -= N;
    }
    if (msglen != 0) {
	genrand_bytes(ctx, ciphertext, plaintext, msglen);
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
    int i;

    for (i = 0; i < blocks; i++) {
	genrand_block(ctx, ciphertext, plaintext);
	ciphertext += N;
	plaintext += N;
    }
}

/* Message length in blocks. */
void ECRYPT_decrypt_blocks(ECRYPT_ctx * ctx,
			   const u8 * ciphertext,
			   u8 * plaintext, u32 blocks)
{				
    ECRYPT_encrypt_blocks(ctx, ciphertext, plaintext, blocks);
}

void ECRYPT_keystream_bytes(ECRYPT_ctx * ctx, u8 * keystream, u32 msglen)
{
    while (msglen >= N) {
	genrand_keystream_block(ctx, keystream);
	keystream += N;
	msglen -= N;
    }
    if (msglen != 0) {
	genrand_keystream_bytes(ctx, keystream, msglen);
    }
}

/* Keystream length in blocks. */
void ECRYPT_keystream_blocks(ECRYPT_ctx * ctx, u8 * keystream, u32 blocks)
{				
    int i;

    for (i = 0; i < blocks; i++) {
	genrand_keystream_block(ctx, keystream);
	keystream += N;
    }
}
