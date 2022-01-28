/*
 * SOSEMANUK reference implementation.
 *
 * This code is supposed to run on any conforming C implementation (C90
 * or later). When compiled with the SOSEMANUK_VECTOR macro defined, this
 * is a stand-alone program which outputs detailed test vectors. When
 * compiled with the SOSEMANUK_SPEED macro defined, this is a stand-alone
 * program which performs an implementation speed measure.
 *
 * (c) 2005 X-CRYPT project. This software is provided 'as-is', without
 * any express or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to no restriction.
 *
 * Technical remarks and questions can be addressed to
 * <thomas.pornin@cryptolog.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SOSEMANUK_SPEED
#include <time.h>
#endif

#include "sosemanuk.h"

/* ======================================================================== */

#ifdef SOSEMANUK_ECRYPT

/*
 * No local speed testing when using the ECRYPT mode.
 */
#undef SOSEMANUK_SPEED

/*
 * If we are using the ECRYPT API, then we rely on the ECRYPT portability
 * macros and types.
 */

#define unum32    u32
#define T32(x)    U32V(x)

#define decode32le(data)       U8TO32_LITTLE(data)
#define encode32le(dst, val)   do { \
		u8 *encode_dst = (dst); \
		u32 encode_val = (val); \
		U32TO8_LITTLE(encode_dst, encode_val); \
	} while (0)

#define ROTL(x, n)    ROTL32(x, n)
#define INLINE

#else

/*
 * 32-bit data decoding, little endian.
 */
static INLINE unum32
decode32le(unsigned char *data)
{
#ifdef __i386
	/*
	 * On i386, we prefer accessing data directly. Unaligned accesses
	 * imply only a one-cycle penalty; even with that penalty, this
	 * method is quite faster than the generic one. Note that i486
	 * and later may be set in a mode where unaligned access trigger
	 * exceptions; but such a mode is not compatible with usual ABI
	 * (which require only 4-byte alignment for "double" and "long
	 * double", hence operating systems do not set that "alignment
	 * check" flag.
	 *
	 * If this optimized access proves to be a problem, replace the
	 * test above by "#if 0".
	 */
	return *(unum32 *)data;
#else
	return (unum32)data[0]
		| ((unum32)data[1] << 8)
		| ((unum32)data[2] << 16)
		| ((unum32)data[3] << 24);
#endif
}

/*
 * 32-bit data encoding, little-endian.
 */
static INLINE void
encode32le(unsigned char *dst, unum32 val)
{
#ifdef __i386__
	/*
	 * Optimized version for i386. See comments in decode32le().
	 */
	*(unum32 *)dst = val;
#else
	dst[0] = val & 0xFF;
	dst[1] = (val >> 8) & 0xFF;
	dst[2] = (val >> 16) & 0xFF;
	dst[3] = (val >> 24) & 0xFF;
#endif
}

/*
 * Left-rotation by n bits (0 < n < 32).
 */
#define ROTL(x, n)    (T32(((x) << (n)) | T32((x) >> (32 - (n)))))

#endif

/* ======================================================================== */

/*
 * Serpent S-boxes, implemented in bitslice mode. These circuits have
 * been published by Dag Arne Osvik ("Speeding up Serpent", published in
 * the 3rd AES Candidate Conference) and work on five 32-bit registers:
 * the four inputs, and a fifth scratch register. There are meant to be
 * quite fast on Pentium-class processors. These are not the fastest
 * published, but they are "fast enough" and they are unencumbered as
 * far as intellectual property is concerned (note: these are rewritten
 * from the article itself, and hence are not covered by the GPL on
 * Dag's code, which was not used here).
 *
 * The output bits are permuted. Here is the correspondance:
 *   S0:  1420
 *   S1:  2031
 *   S2:  2314
 *   S3:  1234
 *   S4:  1403
 *   S5:  1302
 *   S6:  0142
 *   S7:  4310
 * (for instance, the output of S0 is in "r1, r4, r2, r0").
 */

#define S0(r0, r1, r2, r3, r4)   do { \
		r3 ^= r0;  r4  = r1; \
		r1 &= r3;  r4 ^= r2; \
		r1 ^= r0;  r0 |= r3; \
		r0 ^= r4;  r4 ^= r3; \
		r3 ^= r2;  r2 |= r1; \
		r2 ^= r4;  r4 = ~r4; \
		r4 |= r1;  r1 ^= r3; \
		r1 ^= r4;  r3 |= r0; \
		r1 ^= r3;  r4 ^= r3; \
	} while (0)

#define S1(r0, r1, r2, r3, r4)   do { \
		r0 = ~r0;  r2 = ~r2; \
		r4  = r0;  r0 &= r1; \
		r2 ^= r0;  r0 |= r3; \
		r3 ^= r2;  r1 ^= r0; \
		r0 ^= r4;  r4 |= r1; \
		r1 ^= r3;  r2 |= r0; \
		r2 &= r4;  r0 ^= r1; \
		r1 &= r2; \
		r1 ^= r0;  r0 &= r2; \
		r0 ^= r4; \
	} while (0)

#define S2(r0, r1, r2, r3, r4)   do { \
		r4  = r0;  r0 &= r2; \
		r0 ^= r3;  r2 ^= r1; \
		r2 ^= r0;  r3 |= r4; \
		r3 ^= r1;  r4 ^= r2; \
		r1  = r3;  r3 |= r4; \
		r3 ^= r0;  r0 &= r1; \
		r4 ^= r0;  r1 ^= r3; \
		r1 ^= r4;  r4 = ~r4; \
	} while (0)

#define S3(r0, r1, r2, r3, r4)   do { \
		r4  = r0;  r0 |= r3; \
		r3 ^= r1;  r1 &= r4; \
		r4 ^= r2;  r2 ^= r3; \
		r3 &= r0;  r4 |= r1; \
		r3 ^= r4;  r0 ^= r1; \
		r4 &= r0;  r1 ^= r3; \
		r4 ^= r2;  r1 |= r0; \
		r1 ^= r2;  r0 ^= r3; \
		r2  = r1;  r1 |= r3; \
		r1 ^= r0; \
	} while (0)

#define S4(r0, r1, r2, r3, r4)   do { \
		r1 ^= r3;  r3 = ~r3; \
		r2 ^= r3;  r3 ^= r0; \
		r4  = r1;  r1 &= r3; \
		r1 ^= r2;  r4 ^= r3; \
		r0 ^= r4;  r2 &= r4; \
		r2 ^= r0;  r0 &= r1; \
		r3 ^= r0;  r4 |= r1; \
		r4 ^= r0;  r0 |= r3; \
		r0 ^= r2;  r2 &= r3; \
		r0 = ~r0;  r4 ^= r2; \
	} while (0)

#define S5(r0, r1, r2, r3, r4)   do { \
		r0 ^= r1;  r1 ^= r3; \
		r3 = ~r3;  r4  = r1; \
		r1 &= r0;  r2 ^= r3; \
		r1 ^= r2;  r2 |= r4; \
		r4 ^= r3;  r3 &= r1; \
		r3 ^= r0;  r4 ^= r1; \
		r4 ^= r2;  r2 ^= r0; \
		r0 &= r3;  r2 = ~r2; \
		r0 ^= r4;  r4 |= r3; \
		r2 ^= r4; \
	} while (0)

#define S6(r0, r1, r2, r3, r4)   do { \
		r2 = ~r2;  r4  = r3; \
		r3 &= r0;  r0 ^= r4; \
		r3 ^= r2;  r2 |= r4; \
		r1 ^= r3;  r2 ^= r0; \
		r0 |= r1;  r2 ^= r1; \
		r4 ^= r0;  r0 |= r3; \
		r0 ^= r2;  r4 ^= r3; \
		r4 ^= r0;  r3 = ~r3; \
		r2 &= r4; \
		r2 ^= r3; \
	} while (0)

#define S7(r0, r1, r2, r3, r4)   do { \
		r4  = r1;  r1 |= r2; \
		r1 ^= r3;  r4 ^= r2; \
		r2 ^= r1;  r3 |= r4; \
		r3 &= r0;  r4 ^= r2; \
		r3 ^= r1;  r1 |= r4; \
		r1 ^= r0;  r0 |= r4; \
		r0 ^= r2;  r1 ^= r4; \
		r2 ^= r1;  r1 &= r0; \
		r1 ^= r4;  r2 = ~r2; \
		r2 |= r0; \
		r4 ^= r2; \
	} while (0)

/*
 * The Serpent linear transform.
 */
#define SERPENT_LT(x0, x1, x2, x3)  do { \
		x0 = ROTL(x0, 13); \
		x2 = ROTL(x2, 3); \
		x1 = x1 ^ x0 ^ x2; \
		x3 = x3 ^ x2 ^ T32(x0 << 3); \
		x1 = ROTL(x1, 1); \
		x3 = ROTL(x3, 7); \
		x0 = x0 ^ x1 ^ x3; \
		x2 = x2 ^ x3 ^ T32(x1 << 7); \
		x0 = ROTL(x0, 5); \
		x2 = ROTL(x2, 22); \
	} while (0)

/* ======================================================================== */

#ifdef SOSEMANUK_ECRYPT
void
ECRYPT_init(void)
{
	return;
}
#endif

#ifdef SOSEMANUK_ECRYPT
void
ECRYPT_keysetup(ECRYPT_ctx *kc, const u8 *key, u32 keysize, u32 ivsize)
#else
/* see sosemanuk.h */
void
sosemanuk_schedule(sosemanuk_key_context *kc,
	unsigned char *key, size_t key_len)
#endif
{
	/*
	 * This key schedule is actually a truncated Serpent key schedule.
	 * The key-derived words (w_i) are computed within the eight
	 * local variables w0 to w7, which are reused again and again.
	 */

#define SKS(S, o0, o1, o2, o3, d0, d1, d2, d3)   do { \
		unum32 r0, r1, r2, r3, r4; \
		r0 = w ## o0; \
		r1 = w ## o1; \
		r2 = w ## o2; \
		r3 = w ## o3; \
		S(r0, r1, r2, r3, r4); \
		kc->sk[i ++] = r ## d0; \
		kc->sk[i ++] = r ## d1; \
		kc->sk[i ++] = r ## d2; \
		kc->sk[i ++] = r ## d3; \
	} while (0)

#define SKS0    SKS(S0, 4, 5, 6, 7, 1, 4, 2, 0)
#define SKS1    SKS(S1, 0, 1, 2, 3, 2, 0, 3, 1)
#define SKS2    SKS(S2, 4, 5, 6, 7, 2, 3, 1, 4)
#define SKS3    SKS(S3, 0, 1, 2, 3, 1, 2, 3, 4)
#define SKS4    SKS(S4, 4, 5, 6, 7, 1, 4, 0, 3)
#define SKS5    SKS(S5, 0, 1, 2, 3, 1, 3, 0, 2)
#define SKS6    SKS(S6, 4, 5, 6, 7, 0, 1, 4, 2)
#define SKS7    SKS(S7, 0, 1, 2, 3, 4, 3, 1, 0)

#define WUP(wi, wi5, wi3, wi1, cc)   do { \
		unum32 tt = (wi) ^ (wi5) ^ (wi3) \
			^ (wi1) ^ (0x9E3779B9 ^ (unum32)(cc)); \
		(wi) = ROTL(tt, 11); \
	} while (0)

#define WUP0(cc)   do { \
		WUP(w0, w3, w5, w7, cc); \
		WUP(w1, w4, w6, w0, cc + 1); \
		WUP(w2, w5, w7, w1, cc + 2); \
		WUP(w3, w6, w0, w2, cc + 3); \
	} while (0)

#define WUP1(cc)   do { \
		WUP(w4, w7, w1, w3, cc); \
		WUP(w5, w0, w2, w4, cc + 1); \
		WUP(w6, w1, w3, w5, cc + 2); \
		WUP(w7, w2, w4, w6, cc + 3); \
	} while (0)

	unsigned char wbuf[32];
	register unum32 w0, w1, w2, w3, w4, w5, w6, w7;
	int i = 0;
#ifdef SOSEMANUK_ECRYPT
	size_t key_len = keysize / 8;

	kc->ivlen = ivsize / 8;
#endif

	/*
	 * The key is copied into the wbuf[] buffer and padded to 256 bits
	 * as described in the Serpent specification.
	 */
	if (key_len == 0 || key_len > 32) {
		fprintf(stderr, "invalid key size: %lu\n",
			(unsigned long)key_len);
		exit(EXIT_FAILURE);
	}
	memcpy(wbuf, key, key_len);
	if (key_len < 32) {
		wbuf[key_len] = 0x01;
		if (key_len < 31)
			memset(wbuf + key_len + 1, 0, 31 - key_len);
	}

#ifdef SOSEMANUK_VECTOR
	{
		size_t u;

		printf("key = ");
		for (u = 0; u < key_len; u ++)
			printf("%02X", key[u]);
		printf("\n");
	}
#endif

	w0 = decode32le(wbuf);
	w1 = decode32le(wbuf + 4);
	w2 = decode32le(wbuf + 8);
	w3 = decode32le(wbuf + 12);
	w4 = decode32le(wbuf + 16);
	w5 = decode32le(wbuf + 20);
	w6 = decode32le(wbuf + 24);
	w7 = decode32le(wbuf + 28);

#ifdef SOSEMANUK_VECTOR
	printf("  -> %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX\n",
		(unsigned long)w7, (unsigned long)w6,
		(unsigned long)w5, (unsigned long)w4,
		(unsigned long)w3, (unsigned long)w2,
		(unsigned long)w1, (unsigned long)w0);
#endif

	WUP0(0);   SKS3;
	WUP1(4);   SKS2;
	WUP0(8);   SKS1;
	WUP1(12);  SKS0;
	WUP0(16);  SKS7;
	WUP1(20);  SKS6;
	WUP0(24);  SKS5;
	WUP1(28);  SKS4;
	WUP0(32);  SKS3;
	WUP1(36);  SKS2;
	WUP0(40);  SKS1;
	WUP1(44);  SKS0;
	WUP0(48);  SKS7;
	WUP1(52);  SKS6;
	WUP0(56);  SKS5;
	WUP1(60);  SKS4;
	WUP0(64);  SKS3;
	WUP1(68);  SKS2;
	WUP0(72);  SKS1;
	WUP1(76);  SKS0;
	WUP0(80);  SKS7;
	WUP1(84);  SKS6;
	WUP0(88);  SKS5;
	WUP1(92);  SKS4;
	WUP0(96);  SKS3;

#ifdef SOSEMANUK_VECTOR
	{
		unsigned u;

		for (u = 0; u < 100; u += 4) {
			printf("Serpent24 subkey %2u:"
				" %08lX %08lX %08lX %08lX\n", u / 4,
				(unsigned long)kc->sk[u + 3],
				(unsigned long)kc->sk[u + 2],
				(unsigned long)kc->sk[u + 1],
				(unsigned long)kc->sk[u + 0]);
		}
	}
#endif

#undef SKS
#undef SKS0
#undef SKS1
#undef SKS2
#undef SKS3
#undef SKS4
#undef SKS5
#undef SKS6
#undef SKS7
#undef WUP
#undef WUP0
#undef WUP1
}

#ifdef SOSEMANUK_ECRYPT
void
ECRYPT_ivsetup(ECRYPT_ctx *ctx, const u8 *iv)
#else
/* see sosemanuk.h */
void
sosemanuk_init(sosemanuk_run_context *rc, sosemanuk_key_context *kc,
	unsigned char *iv, size_t iv_len)
#endif
{

#ifdef SOSEMANUK_ECRYPT
#define rc       ctx
#define kc       ctx
#define iv_len   (ctx->ivlen)
#endif

	/*
	 * The Serpent key addition step.
	 */
#define KA(zc, x0, x1, x2, x3)  do { \
		x0 ^= kc->sk[(zc)]; \
		x1 ^= kc->sk[(zc) + 1]; \
		x2 ^= kc->sk[(zc) + 2]; \
		x3 ^= kc->sk[(zc) + 3]; \
	} while (0)

	/*
	 * One Serpent round.
	 *   zc = current subkey counter
	 *   S = S-box macro for this round
	 *   i0 to i4 = input register numbers (the fifth is a scratch register)
	 *   o0 to o3 = output register numbers
	 */
#define FSS(zc, S, i0, i1, i2, i3, i4, o0, o1, o2, o3)  do { \
		KA(zc, r ## i0, r ## i1, r ## i2, r ## i3); \
		S(r ## i0, r ## i1, r ## i2, r ## i3, r ## i4); \
		SERPENT_LT(r ## o0, r ## o1, r ## o2, r ## o3); \
	} while (0)

	/*
	 * Last Serpent round. Contrary to the "true" Serpent, we keep
	 * the linear transformation for that last round.
	 */
#define FSF(zc, S, i0, i1, i2, i3, i4, o0, o1, o2, o3)  do { \
		KA(zc, r ## i0, r ## i1, r ## i2, r ## i3); \
		S(r ## i0, r ## i1, r ## i2, r ## i3, r ## i4); \
		SERPENT_LT(r ## o0, r ## o1, r ## o2, r ## o3); \
		KA(zc + 4, r ## o0, r ## o1, r ## o2, r ## o3); \
	} while (0)

	register unum32 r0, r1, r2, r3, r4;
	unsigned char ivtmp[16];

	if (iv_len >= sizeof ivtmp) {
		memcpy(ivtmp, iv, sizeof ivtmp);
	} else {
		if (iv_len > 0)
			memcpy(ivtmp, iv, iv_len);
		memset(ivtmp + iv_len, 0, (sizeof ivtmp) - iv_len);
	}

#ifdef SOSEMANUK_VECTOR
	{
		size_t u;

		printf("IV = ");
		for (u = 0; u < 16; u ++)
			printf("%02X", ivtmp[u]);
		printf("\n");
	}
#endif

	/*
	 * Decode IV into four 32-bit words (little-endian).
	 */
	r0 = decode32le(ivtmp);
	r1 = decode32le(ivtmp + 4);
	r2 = decode32le(ivtmp + 8);
	r3 = decode32le(ivtmp + 12);

#ifdef SOSEMANUK_VECTOR
	printf("  -> %08lX %08lX %08lX %08lX\n",
		(unsigned long)r3, (unsigned long)r2,
		(unsigned long)r1, (unsigned long)r0);
#endif

	/*
	 * Encrypt IV with Serpent24. Some values are extracted from the
	 * output of the twelfth, eighteenth and twenty-fourth rounds.
	 */
	FSS(0, S0, 0, 1, 2, 3, 4, 1, 4, 2, 0);
	FSS(4, S1, 1, 4, 2, 0, 3, 2, 1, 0, 4);
	FSS(8, S2, 2, 1, 0, 4, 3, 0, 4, 1, 3);
	FSS(12, S3, 0, 4, 1, 3, 2, 4, 1, 3, 2);
	FSS(16, S4, 4, 1, 3, 2, 0, 1, 0, 4, 2);
	FSS(20, S5, 1, 0, 4, 2, 3, 0, 2, 1, 4);
	FSS(24, S6, 0, 2, 1, 4, 3, 0, 2, 3, 1);
	FSS(28, S7, 0, 2, 3, 1, 4, 4, 1, 2, 0);
	FSS(32, S0, 4, 1, 2, 0, 3, 1, 3, 2, 4);
	FSS(36, S1, 1, 3, 2, 4, 0, 2, 1, 4, 3);
	FSS(40, S2, 2, 1, 4, 3, 0, 4, 3, 1, 0);
	FSS(44, S3, 4, 3, 1, 0, 2, 3, 1, 0, 2);
	rc->s09 = r3;
	rc->s08 = r1;
	rc->s07 = r0;
	rc->s06 = r2;

	FSS(48, S4, 3, 1, 0, 2, 4, 1, 4, 3, 2);
	FSS(52, S5, 1, 4, 3, 2, 0, 4, 2, 1, 3);
	FSS(56, S6, 4, 2, 1, 3, 0, 4, 2, 0, 1);
	FSS(60, S7, 4, 2, 0, 1, 3, 3, 1, 2, 4);
	FSS(64, S0, 3, 1, 2, 4, 0, 1, 0, 2, 3);
	FSS(68, S1, 1, 0, 2, 3, 4, 2, 1, 3, 0);
	rc->r1  = r2;
	rc->s04 = r1;
	rc->r2  = r3;
	rc->s05 = r0;

	FSS(72, S2, 2, 1, 3, 0, 4, 3, 0, 1, 4);
	FSS(76, S3, 3, 0, 1, 4, 2, 0, 1, 4, 2);
	FSS(80, S4, 0, 1, 4, 2, 3, 1, 3, 0, 2);
	FSS(84, S5, 1, 3, 0, 2, 4, 3, 2, 1, 0);
	FSS(88, S6, 3, 2, 1, 0, 4, 3, 2, 4, 1);
	FSF(92, S7, 3, 2, 4, 1, 0, 0, 1, 2, 3);
	rc->s03 = r0;
	rc->s02 = r1;
	rc->s01 = r2;
	rc->s00 = r3;

#ifdef SOSEMANUK_VECTOR
	printf("Initial LFSR state:\n");
	printf("      s1  = %08lX\n", (unsigned long)rc->s00);
	printf("      s2  = %08lX\n", (unsigned long)rc->s01);
	printf("      s3  = %08lX\n", (unsigned long)rc->s02);
	printf("      s4  = %08lX\n", (unsigned long)rc->s03);
	printf("      s5  = %08lX\n", (unsigned long)rc->s04);
	printf("      s6  = %08lX\n", (unsigned long)rc->s05);
	printf("      s7  = %08lX\n", (unsigned long)rc->s06);
	printf("      s8  = %08lX\n", (unsigned long)rc->s07);
	printf("      s9  = %08lX\n", (unsigned long)rc->s08);
	printf("      s10 = %08lX\n", (unsigned long)rc->s09);
	printf("Initial FSM state:  r1 = %08lX   r2 = %08lX\n",
		(unsigned long)rc->r1, (unsigned long)rc->r2);
#endif

#ifndef SOSEMANUK_ECRYPT
	rc->ptr = sizeof rc->buf;
#endif

#undef KA
#undef FSS
#undef FSF

#ifdef SOSEMANUK_ECRYPT
#undef rc
#undef kc
#undef iv_len
#endif
}

/*
 * Multiplication by alpha: alpha * x = T32(x << 8) ^ mul_a[x >> 24]
 */
static unum32 mul_a[] = {
	0x00000000, 0xE19FCF13, 0x6B973726, 0x8A08F835,
	0xD6876E4C, 0x3718A15F, 0xBD10596A, 0x5C8F9679,
	0x05A7DC98, 0xE438138B, 0x6E30EBBE, 0x8FAF24AD,
	0xD320B2D4, 0x32BF7DC7, 0xB8B785F2, 0x59284AE1,
	0x0AE71199, 0xEB78DE8A, 0x617026BF, 0x80EFE9AC,
	0xDC607FD5, 0x3DFFB0C6, 0xB7F748F3, 0x566887E0,
	0x0F40CD01, 0xEEDF0212, 0x64D7FA27, 0x85483534,
	0xD9C7A34D, 0x38586C5E, 0xB250946B, 0x53CF5B78,
	0x1467229B, 0xF5F8ED88, 0x7FF015BD, 0x9E6FDAAE,
	0xC2E04CD7, 0x237F83C4, 0xA9777BF1, 0x48E8B4E2,
	0x11C0FE03, 0xF05F3110, 0x7A57C925, 0x9BC80636,
	0xC747904F, 0x26D85F5C, 0xACD0A769, 0x4D4F687A,
	0x1E803302, 0xFF1FFC11, 0x75170424, 0x9488CB37,
	0xC8075D4E, 0x2998925D, 0xA3906A68, 0x420FA57B,
	0x1B27EF9A, 0xFAB82089, 0x70B0D8BC, 0x912F17AF,
	0xCDA081D6, 0x2C3F4EC5, 0xA637B6F0, 0x47A879E3,
	0x28CE449F, 0xC9518B8C, 0x435973B9, 0xA2C6BCAA,
	0xFE492AD3, 0x1FD6E5C0, 0x95DE1DF5, 0x7441D2E6,
	0x2D699807, 0xCCF65714, 0x46FEAF21, 0xA7616032,
	0xFBEEF64B, 0x1A713958, 0x9079C16D, 0x71E60E7E,
	0x22295506, 0xC3B69A15, 0x49BE6220, 0xA821AD33,
	0xF4AE3B4A, 0x1531F459, 0x9F390C6C, 0x7EA6C37F,
	0x278E899E, 0xC611468D, 0x4C19BEB8, 0xAD8671AB,
	0xF109E7D2, 0x109628C1, 0x9A9ED0F4, 0x7B011FE7,
	0x3CA96604, 0xDD36A917, 0x573E5122, 0xB6A19E31,
	0xEA2E0848, 0x0BB1C75B, 0x81B93F6E, 0x6026F07D,
	0x390EBA9C, 0xD891758F, 0x52998DBA, 0xB30642A9,
	0xEF89D4D0, 0x0E161BC3, 0x841EE3F6, 0x65812CE5,
	0x364E779D, 0xD7D1B88E, 0x5DD940BB, 0xBC468FA8,
	0xE0C919D1, 0x0156D6C2, 0x8B5E2EF7, 0x6AC1E1E4,
	0x33E9AB05, 0xD2766416, 0x587E9C23, 0xB9E15330,
	0xE56EC549, 0x04F10A5A, 0x8EF9F26F, 0x6F663D7C,
	0x50358897, 0xB1AA4784, 0x3BA2BFB1, 0xDA3D70A2,
	0x86B2E6DB, 0x672D29C8, 0xED25D1FD, 0x0CBA1EEE,
	0x5592540F, 0xB40D9B1C, 0x3E056329, 0xDF9AAC3A,
	0x83153A43, 0x628AF550, 0xE8820D65, 0x091DC276,
	0x5AD2990E, 0xBB4D561D, 0x3145AE28, 0xD0DA613B,
	0x8C55F742, 0x6DCA3851, 0xE7C2C064, 0x065D0F77,
	0x5F754596, 0xBEEA8A85, 0x34E272B0, 0xD57DBDA3,
	0x89F22BDA, 0x686DE4C9, 0xE2651CFC, 0x03FAD3EF,
	0x4452AA0C, 0xA5CD651F, 0x2FC59D2A, 0xCE5A5239,
	0x92D5C440, 0x734A0B53, 0xF942F366, 0x18DD3C75,
	0x41F57694, 0xA06AB987, 0x2A6241B2, 0xCBFD8EA1,
	0x977218D8, 0x76EDD7CB, 0xFCE52FFE, 0x1D7AE0ED,
	0x4EB5BB95, 0xAF2A7486, 0x25228CB3, 0xC4BD43A0,
	0x9832D5D9, 0x79AD1ACA, 0xF3A5E2FF, 0x123A2DEC,
	0x4B12670D, 0xAA8DA81E, 0x2085502B, 0xC11A9F38,
	0x9D950941, 0x7C0AC652, 0xF6023E67, 0x179DF174,
	0x78FBCC08, 0x9964031B, 0x136CFB2E, 0xF2F3343D,
	0xAE7CA244, 0x4FE36D57, 0xC5EB9562, 0x24745A71,
	0x7D5C1090, 0x9CC3DF83, 0x16CB27B6, 0xF754E8A5,
	0xABDB7EDC, 0x4A44B1CF, 0xC04C49FA, 0x21D386E9,
	0x721CDD91, 0x93831282, 0x198BEAB7, 0xF81425A4,
	0xA49BB3DD, 0x45047CCE, 0xCF0C84FB, 0x2E934BE8,
	0x77BB0109, 0x9624CE1A, 0x1C2C362F, 0xFDB3F93C,
	0xA13C6F45, 0x40A3A056, 0xCAAB5863, 0x2B349770,
	0x6C9CEE93, 0x8D032180, 0x070BD9B5, 0xE69416A6,
	0xBA1B80DF, 0x5B844FCC, 0xD18CB7F9, 0x301378EA,
	0x693B320B, 0x88A4FD18, 0x02AC052D, 0xE333CA3E,
	0xBFBC5C47, 0x5E239354, 0xD42B6B61, 0x35B4A472,
	0x667BFF0A, 0x87E43019, 0x0DECC82C, 0xEC73073F,
	0xB0FC9146, 0x51635E55, 0xDB6BA660, 0x3AF46973,
	0x63DC2392, 0x8243EC81, 0x084B14B4, 0xE9D4DBA7,
	0xB55B4DDE, 0x54C482CD, 0xDECC7AF8, 0x3F53B5EB
};

/*
 * Multiplication by 1/alpha: 1/alpha * x = (x >> 8) ^ mul_ia[x & 0xFF]
 */
static unum32 mul_ia[] = {
	0x00000000, 0x180F40CD, 0x301E8033, 0x2811C0FE,
	0x603CA966, 0x7833E9AB, 0x50222955, 0x482D6998,
	0xC078FBCC, 0xD877BB01, 0xF0667BFF, 0xE8693B32,
	0xA04452AA, 0xB84B1267, 0x905AD299, 0x88559254,
	0x29F05F31, 0x31FF1FFC, 0x19EEDF02, 0x01E19FCF,
	0x49CCF657, 0x51C3B69A, 0x79D27664, 0x61DD36A9,
	0xE988A4FD, 0xF187E430, 0xD99624CE, 0xC1996403,
	0x89B40D9B, 0x91BB4D56, 0xB9AA8DA8, 0xA1A5CD65,
	0x5249BE62, 0x4A46FEAF, 0x62573E51, 0x7A587E9C,
	0x32751704, 0x2A7A57C9, 0x026B9737, 0x1A64D7FA,
	0x923145AE, 0x8A3E0563, 0xA22FC59D, 0xBA208550,
	0xF20DECC8, 0xEA02AC05, 0xC2136CFB, 0xDA1C2C36,
	0x7BB9E153, 0x63B6A19E, 0x4BA76160, 0x53A821AD,
	0x1B854835, 0x038A08F8, 0x2B9BC806, 0x339488CB,
	0xBBC11A9F, 0xA3CE5A52, 0x8BDF9AAC, 0x93D0DA61,
	0xDBFDB3F9, 0xC3F2F334, 0xEBE333CA, 0xF3EC7307,
	0xA492D5C4, 0xBC9D9509, 0x948C55F7, 0x8C83153A,
	0xC4AE7CA2, 0xDCA13C6F, 0xF4B0FC91, 0xECBFBC5C,
	0x64EA2E08, 0x7CE56EC5, 0x54F4AE3B, 0x4CFBEEF6,
	0x04D6876E, 0x1CD9C7A3, 0x34C8075D, 0x2CC74790,
	0x8D628AF5, 0x956DCA38, 0xBD7C0AC6, 0xA5734A0B,
	0xED5E2393, 0xF551635E, 0xDD40A3A0, 0xC54FE36D,
	0x4D1A7139, 0x551531F4, 0x7D04F10A, 0x650BB1C7,
	0x2D26D85F, 0x35299892, 0x1D38586C, 0x053718A1,
	0xF6DB6BA6, 0xEED42B6B, 0xC6C5EB95, 0xDECAAB58,
	0x96E7C2C0, 0x8EE8820D, 0xA6F942F3, 0xBEF6023E,
	0x36A3906A, 0x2EACD0A7, 0x06BD1059, 0x1EB25094,
	0x569F390C, 0x4E9079C1, 0x6681B93F, 0x7E8EF9F2,
	0xDF2B3497, 0xC724745A, 0xEF35B4A4, 0xF73AF469,
	0xBF179DF1, 0xA718DD3C, 0x8F091DC2, 0x97065D0F,
	0x1F53CF5B, 0x075C8F96, 0x2F4D4F68, 0x37420FA5,
	0x7F6F663D, 0x676026F0, 0x4F71E60E, 0x577EA6C3,
	0xE18D0321, 0xF98243EC, 0xD1938312, 0xC99CC3DF,
	0x81B1AA47, 0x99BEEA8A, 0xB1AF2A74, 0xA9A06AB9,
	0x21F5F8ED, 0x39FAB820, 0x11EB78DE, 0x09E43813,
	0x41C9518B, 0x59C61146, 0x71D7D1B8, 0x69D89175,
	0xC87D5C10, 0xD0721CDD, 0xF863DC23, 0xE06C9CEE,
	0xA841F576, 0xB04EB5BB, 0x985F7545, 0x80503588,
	0x0805A7DC, 0x100AE711, 0x381B27EF, 0x20146722,
	0x68390EBA, 0x70364E77, 0x58278E89, 0x4028CE44,
	0xB3C4BD43, 0xABCBFD8E, 0x83DA3D70, 0x9BD57DBD,
	0xD3F81425, 0xCBF754E8, 0xE3E69416, 0xFBE9D4DB,
	0x73BC468F, 0x6BB30642, 0x43A2C6BC, 0x5BAD8671,
	0x1380EFE9, 0x0B8FAF24, 0x239E6FDA, 0x3B912F17,
	0x9A34E272, 0x823BA2BF, 0xAA2A6241, 0xB225228C,
	0xFA084B14, 0xE2070BD9, 0xCA16CB27, 0xD2198BEA,
	0x5A4C19BE, 0x42435973, 0x6A52998D, 0x725DD940,
	0x3A70B0D8, 0x227FF015, 0x0A6E30EB, 0x12617026,
	0x451FD6E5, 0x5D109628, 0x750156D6, 0x6D0E161B,
	0x25237F83, 0x3D2C3F4E, 0x153DFFB0, 0x0D32BF7D,
	0x85672D29, 0x9D686DE4, 0xB579AD1A, 0xAD76EDD7,
	0xE55B844F, 0xFD54C482, 0xD545047C, 0xCD4A44B1,
	0x6CEF89D4, 0x74E0C919, 0x5CF109E7, 0x44FE492A,
	0x0CD320B2, 0x14DC607F, 0x3CCDA081, 0x24C2E04C,
	0xAC977218, 0xB49832D5, 0x9C89F22B, 0x8486B2E6,
	0xCCABDB7E, 0xD4A49BB3, 0xFCB55B4D, 0xE4BA1B80,
	0x17566887, 0x0F59284A, 0x2748E8B4, 0x3F47A879,
	0x776AC1E1, 0x6F65812C, 0x477441D2, 0x5F7B011F,
	0xD72E934B, 0xCF21D386, 0xE7301378, 0xFF3F53B5,
	0xB7123A2D, 0xAF1D7AE0, 0x870CBA1E, 0x9F03FAD3,
	0x3EA637B6, 0x26A9777B, 0x0EB8B785, 0x16B7F748,
	0x5E9A9ED0, 0x4695DE1D, 0x6E841EE3, 0x768B5E2E,
	0xFEDECC7A, 0xE6D18CB7, 0xCEC04C49, 0xD6CF0C84,
	0x9EE2651C, 0x86ED25D1, 0xAEFCE52F, 0xB6F3A5E2
};

/*
 * Compute the next block of bits of output stream. This is equivalent
 * to one full rotation of the shift register.
 *
 * If SOSEMANUK_SPEED is defined, this function takes an extra parameter
 * "counter". The function then returns the sum of all produced
 * 32-bit words, in an "unum32". That sum prevents the compiler from
 * optimizing out part of the computation.
 *
 * If SOSEMANUK_ECRYPT is defined, the input and output buffers are
 * provided also, and this function performs the XOR. The input and
 * output buffers are assumed to be 32-bit aligned.
 */
#if defined SOSEMANUK_ECRYPT
static void
sosemanuk_internal(ECRYPT_ctx *rc, const u32 *src, u32 *dst)
#elif defined SOSEMANUK_SPEED
static unum32
sosemanuk_internal(sosemanuk_run_context *rc, unsigned long counter)
#else
static void
sosemanuk_internal(sosemanuk_run_context *rc)
#endif
{
	/*
	 * MUL_A(x) computes alpha * x (in F_{2^32}).
	 * MUL_G(x) computes 1/alpha * x (in F_{2^32}).
	 */
#define MUL_A(x)    (T32((x) << 8) ^ mul_a[(x) >> 24])
#define MUL_G(x)    (((x) >> 8) ^ mul_ia[(x) & 0xFF])

	/*
	 * This macro computes the special multiplexer, which chooses
	 * between "x" and "x xor y", depending on the least significant
	 * bit of the control word. We use the C "?:" selection operator
	 * (which most compilers know how to optimise) except for Alpha,
	 * where the manual sign extension seems to perform equally well
	 * with DEC/Compaq/HP compiler, and much better with gcc.
	 */
#ifdef __alpha
#define XMUX(c, x, y)   ((((signed int)((c) << 31) >> 31) & (y)) ^ (x))
#else
#define XMUX(c, x, y)   (((c) & 0x1) ? ((x) ^ (y)) : (x))
#endif

	/*
	 * FSM() updates the finite state machine.
	 */
#define FSM(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9)   do { \
		unum32 tt, or1; \
		tt = XMUX(r1, s ## x1, s ## x8); \
		or1 = r1; \
		r1 = T32(r2 + tt); \
		tt = T32(or1 * 0x54655307); \
		r2 = ROTL(tt, 7); \
		PFSM; \
	} while (0)

	/*
	 * LRU updates the shift register; the dropped value is stored
	 * in variable "dd".
	 */
#define LRU(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, dd)   do { \
		dd = s ## x0; \
		s ## x0 = MUL_A(s ## x0) ^ MUL_G(s ## x3) ^ s ## x9; \
		PLFSR(dd, s ## x1, s ## x2, s ## x3, s ## x4, s ## x5, \
			s ## x6, s ## x7, s ## x8, s ## x9, s ## x0); \
	} while (0)

	/*
	 * CC1 stores into variable "ee" the next intermediate word
	 * (combination of the new states of the LFSR and the FSM).
	 */
#define CC1(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, ee)   do { \
		ee = T32(s ## x9 + r1) ^ r2; \
		PCCVAL(ee); \
	} while (0)

	/*
	 * STEP computes one internal round. "dd" receives the "s_t"
	 * value (dropped from the LFSR) and "ee" gets the value computed
	 * from the LFSR and FSM.
	 */
#define STEP(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, dd, ee)   do { \
		FSM(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9); \
		LRU(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, dd); \
		CC1(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, ee); \
	} while (0)

	/*
	 * Apply one Serpent round (with the provided S-box macro), XOR
	 * the result with the "v" values, and encode the result into
	 * the destination buffer, at the provided offset. The "x*"
	 * arguments encode the output permutation of the "S" macro.
	 */
#ifdef SOSEMANUK_SPEED

#define SRD(S, x0, x1, x2, x3, ooff)   do { \
		S(u0, u1, u2, u3, u4); \
		speed_acc += u ## x0 ^ v0; \
		speed_acc += u ## x1 ^ v1; \
		speed_acc += u ## x2 ^ v2; \
		speed_acc += u ## x3 ^ v3; \
	} while (0)

#else

#ifdef SOSEMANUK_ECRYPT
#define SRD(S, x0, x1, x2, x3, ooff)   do { \
		PSPIN(u0, u1, u2, u3); \
		S(u0, u1, u2, u3, u4); \
		PSPOUT(u ## x0, u ## x1, u ## x2, u ## x3); \
		dst[(ooff / 4)] = src[(ooff / 4)] \
			^ U32TO32_LITTLE(u ## x0 ^ v0); \
		dst[(ooff / 4) + 1] = src[(ooff / 4) + 1] \
			^ U32TO32_LITTLE(u ## x1 ^ v1); \
		dst[(ooff / 4) + 2] = src[(ooff / 4) + 2] \
			^ U32TO32_LITTLE(u ## x2 ^ v2); \
		dst[(ooff / 4) + 3] = src[(ooff / 4) + 3] \
			^ U32TO32_LITTLE(u ## x3 ^ v3); \
		POUT((unsigned char *)dst + ooff); \
	} while (0)
#else
#define OUTWORD_BASE   (rc->buf)
#define SRD(S, x0, x1, x2, x3, ooff)   do { \
		PSPIN(u0, u1, u2, u3); \
		S(u0, u1, u2, u3, u4); \
		PSPOUT(u ## x0, u ## x1, u ## x2, u ## x3); \
		encode32le(OUTWORD_BASE + ooff, u ## x0 ^ v0); \
		encode32le(OUTWORD_BASE + ooff + 4, u ## x1 ^ v1); \
		encode32le(OUTWORD_BASE + ooff + 8, u ## x2 ^ v2); \
		encode32le(OUTWORD_BASE + ooff + 12, u ## x3 ^ v3); \
		POUT(OUTWORD_BASE + ooff); \
	} while (0)
#endif

#endif

	/*
	 * Audit code; used for detailed test vectors.
	 */
#ifdef SOSEMANUK_VECTOR

#define PFSM   do { \
		printf("New FSM state:  r1 = %08lX   r2 = %08lX\n", \
			(unsigned long)r1, (unsigned long)r2); \
	} while (0)

#define PLFSR(dd, x1, x2, x3, x4, x5, x6, x7, x8, x9, x0)   do { \
		printf("New LFSR state:\n"); \
		printf("   dropped (s_t): %08lX\n", (unsigned long)dd); \
		printf("         s_t+1  = %08lX\n", (unsigned long)x1); \
		printf("         s_t+2  = %08lX\n", (unsigned long)x2); \
		printf("         s_t+3  = %08lX\n", (unsigned long)x3); \
		printf("         s_t+4  = %08lX\n", (unsigned long)x4); \
		printf("         s_t+5  = %08lX\n", (unsigned long)x5); \
		printf("         s_t+6  = %08lX\n", (unsigned long)x6); \
		printf("         s_t+7  = %08lX\n", (unsigned long)x7); \
		printf("         s_t+8  = %08lX\n", (unsigned long)x8); \
		printf("         s_t+9  = %08lX\n", (unsigned long)x9); \
		printf("         s_t+10 = %08lX\n", (unsigned long)x0); \
	} while (0)

#define PCCVAL(ee)   do { \
		printf("Intermediate output: %08lX\n", (unsigned long)ee); \
	} while (0)

#define PSPIN(x0, x1, x2, x3)    do { \
		printf("Serpent1 input:  %08lX %08lX %08lX %08lX\n", \
			(unsigned long)x3, (unsigned long)x2, \
			(unsigned long)x1, (unsigned long)x0); \
	} while (0)

#define PSPOUT(x0, x1, x2, x3)    do { \
		printf("Serpent1 output: %08lX %08lX %08lX %08lX\n", \
			(unsigned long)x3, (unsigned long)x2, \
			(unsigned long)x1, (unsigned long)x0); \
	} while (0)

#define POUT(buf)   do { \
		size_t j; \
		printf("Stream output: "); \
		for (j = 0; j < 16; j ++) \
			printf("%02X", (buf)[j]); \
		printf("\n"); \
	} while (0)

#else

#define PFSM                      (void)0
#define PLFSR(dd, x1, x2, x3, x4, x5, x6, x7, x8, x9, x0)   (void)0
#define PCCVAL(ee)                (void)0
#define PSPIN(x0, x1, x2, x3)     (void)0
#define PSPOUT(x0, x1, x2, x3)    (void)0
#define POUT(buf)                 (void)0

#endif

	unum32 s00 = rc->s00;
	unum32 s01 = rc->s01;
	unum32 s02 = rc->s02;
	unum32 s03 = rc->s03;
	unum32 s04 = rc->s04;
	unum32 s05 = rc->s05;
	unum32 s06 = rc->s06;
	unum32 s07 = rc->s07;
	unum32 s08 = rc->s08;
	unum32 s09 = rc->s09;
	unum32 r1 = rc->r1;
	unum32 r2 = rc->r2;
	unum32 u0, u1, u2, u3, u4;
	unum32 v0, v1, v2, v3;
#ifdef SOSEMANUK_SPEED
	unum32 speed_acc = 0;
#endif

#ifdef SOSEMANUK_SPEED
	while (counter -- > 0) {
#endif

	STEP(00, 01, 02, 03, 04, 05, 06, 07, 08, 09, v0, u0);
	STEP(01, 02, 03, 04, 05, 06, 07, 08, 09, 00, v1, u1);
	STEP(02, 03, 04, 05, 06, 07, 08, 09, 00, 01, v2, u2);
	STEP(03, 04, 05, 06, 07, 08, 09, 00, 01, 02, v3, u3);
	SRD(S2, 2, 3, 1, 4, 0);
	STEP(04, 05, 06, 07, 08, 09, 00, 01, 02, 03, v0, u0);
	STEP(05, 06, 07, 08, 09, 00, 01, 02, 03, 04, v1, u1);
	STEP(06, 07, 08, 09, 00, 01, 02, 03, 04, 05, v2, u2);
	STEP(07, 08, 09, 00, 01, 02, 03, 04, 05, 06, v3, u3);
	SRD(S2, 2, 3, 1, 4, 16);
	STEP(08, 09, 00, 01, 02, 03, 04, 05, 06, 07, v0, u0);
	STEP(09, 00, 01, 02, 03, 04, 05, 06, 07, 08, v1, u1);
	STEP(00, 01, 02, 03, 04, 05, 06, 07, 08, 09, v2, u2);
	STEP(01, 02, 03, 04, 05, 06, 07, 08, 09, 00, v3, u3);
	SRD(S2, 2, 3, 1, 4, 32);
	STEP(02, 03, 04, 05, 06, 07, 08, 09, 00, 01, v0, u0);
	STEP(03, 04, 05, 06, 07, 08, 09, 00, 01, 02, v1, u1);
	STEP(04, 05, 06, 07, 08, 09, 00, 01, 02, 03, v2, u2);
	STEP(05, 06, 07, 08, 09, 00, 01, 02, 03, 04, v3, u3);
	SRD(S2, 2, 3, 1, 4, 48);
	STEP(06, 07, 08, 09, 00, 01, 02, 03, 04, 05, v0, u0);
	STEP(07, 08, 09, 00, 01, 02, 03, 04, 05, 06, v1, u1);
	STEP(08, 09, 00, 01, 02, 03, 04, 05, 06, 07, v2, u2);
	STEP(09, 00, 01, 02, 03, 04, 05, 06, 07, 08, v3, u3);
	SRD(S2, 2, 3, 1, 4, 64);

#ifdef SOSEMANUK_SPEED
	}
#endif

	rc->s00 = s00;
	rc->s01 = s01;
	rc->s02 = s02;
	rc->s03 = s03;
	rc->s04 = s04;
	rc->s05 = s05;
	rc->s06 = s06;
	rc->s07 = s07;
	rc->s08 = s08;
	rc->s09 = s09;
	rc->r1 = r1;
	rc->r2 = r2;

#ifdef SOSEMANUK_SPEED
	return T32(speed_acc);
#endif
}

#if !defined SOSEMANUK_ECRYPT && !defined SOSEMANUK_SPEED

/*
 * Combine buffers in1[] and in2[] by XOR, result in out[]. The length
 * is "data_len" (in bytes). Partial overlap of out[] with either in1[]
 * or in2[] is not allowed. Total overlap (out == in1 and/or out == in2)
 * is allowed.
 *
 * This function tries to be smart and to perform 32-bit accesses whenever
 * safe (aligned pointers). According to the Holy Standard From ISO, this
 * is heretic; but it works everywhere (hopefully).
 */
static INLINE void
xorbuf(const unsigned char *in1, const unsigned char *in2,
	unsigned char *out, size_t data_len)
{
	unum32 *s1, *s2, *d;
	size_t tl;

	if (!(((unsigned)in1 | (unsigned)in2 | (unsigned)out) & 0x03)) {
		s1 = (unum32 *)in1;
		s2 = (unum32 *)in2;
		d = (unum32 *)out;
		tl = data_len >> 2;
		while (tl -- > 0)
			*d ++ = *s1 ++ ^ *s2 ++;

		switch (data_len & (size_t)0x03) {
		case 0x03:
			out[data_len - 3] =
				in1[data_len - 3] ^ in2[data_len - 3];
			/* fall through */
		case 0x02:
			out[data_len - 2] =
				in1[data_len - 2] ^ in2[data_len - 2];
			/* fall through */
		case 0x01:
			out[data_len - 1] =
				in1[data_len - 1] ^ in2[data_len - 1];
		}
	} else {
		while (data_len -- > 0)
			*out ++ = *in1 ++ ^ *in2 ++;
	}
}

#endif

/* ======================================================================== */
/*
 * External API.
 */

#if defined SOSEMANUK_ECRYPT

/* see ecrypt-sync.h */
void
ECRYPT_process_bytes(int action, ECRYPT_ctx *ctx,
	const u8 *input, u8 *output, u32 msglen)
{
	(void)action;

	while (msglen > 0) {
		u32 ibuf[ECRYPT_BLOCKLENGTH / 4];
		u32 obuf[ECRYPT_BLOCKLENGTH / 4];
		size_t len;

		len = sizeof ibuf;
		if (len > msglen)
			len = msglen;
		memcpy(ibuf, input, len);
		sosemanuk_internal(ctx, ibuf, obuf);
		memcpy(output, obuf, len);
		input += len;
		output += len;
		msglen -= len;
	}
}

/* see ecrypt-sync.h */
void
ECRYPT_keystream_bytes(ECRYPT_ctx *ctx, u8 *keystream, u32 length)
{
	static const u32 zb[ECRYPT_BLOCKLENGTH];

	while (length > 0) {
		u32 tbuf[ECRYPT_BLOCKLENGTH / 4];

		sosemanuk_internal(ctx, zb, tbuf);
		if (length >= ECRYPT_BLOCKLENGTH) {
			memcpy(keystream, tbuf, ECRYPT_BLOCKLENGTH);
			keystream += ECRYPT_BLOCKLENGTH;
			length -= ECRYPT_BLOCKLENGTH;
		} else {
			memcpy(keystream, tbuf, length);
			return;
		}
	}
}

/* see ecrypt-sync.h */
void
ECRYPT_process_blocks(int action, ECRYPT_ctx *ctx,
	const u8 *input, u8 *output, u32 blocks)
{
	(void)action;

	while (blocks -- > 0) {
		sosemanuk_internal(ctx, (u32 *)input, (u32 *)output);
		input += ECRYPT_BLOCKLENGTH;
		output += ECRYPT_BLOCKLENGTH;
	}
}

/* see ecrypt-sync.h */
void
ECRYPT_keystream_blocks(ECRYPT_ctx *ctx, u8 *keystream, u32 blocks)
{
	static const u32 zb[ECRYPT_BLOCKLENGTH / 4];

	while (blocks -- > 0) {
		sosemanuk_internal(ctx, zb, (u32 *)keystream);
		keystream += ECRYPT_BLOCKLENGTH;
	}
}

#elif !defined SOSEMANUK_SPEED

/* see sosemanuk.h */
void
sosemanuk_prng(sosemanuk_run_context *rc, unsigned char *out, size_t out_len)
{
	if (rc->ptr < (sizeof rc->buf)) {
		size_t rlen = (sizeof rc->buf) - rc->ptr;

		if (rlen > out_len)
			rlen = out_len;
		memcpy(out, rc->buf + rc->ptr, rlen);
		out += rlen;
		out_len -= rlen;
		rc->ptr += rlen;
	}
	while (out_len > 0) {
		sosemanuk_internal(rc);
		if (out_len >= sizeof rc->buf) {
			memcpy(out, rc->buf, sizeof rc->buf);
			out += sizeof rc->buf;
			out_len -= sizeof rc->buf;
		} else {
			memcpy(out, rc->buf, out_len);
			rc->ptr = out_len;
			out_len = 0;
		}
	}
}

/* see sosemanuk.h */
void
sosemanuk_encrypt(sosemanuk_run_context *rc,
	unsigned char *in, unsigned char *out, size_t data_len)
{
	if (rc->ptr < (sizeof rc->buf)) {
		size_t rlen = (sizeof rc->buf) - rc->ptr;

		if (rlen > data_len)
			rlen = data_len;
		xorbuf(rc->buf + rc->ptr, in, out, rlen);
		in += rlen;
		out += rlen;
		data_len -= rlen;
		rc->ptr += rlen;
	}
	while (data_len > 0) {
		sosemanuk_internal(rc);
		if (data_len >= sizeof rc->buf) {
			xorbuf(rc->buf, in, out, sizeof rc->buf);
			in += sizeof rc->buf;
			out += sizeof rc->buf;
			data_len -= sizeof rc->buf;
		} else {
			xorbuf(rc->buf, in, out, data_len);
			rc->ptr = data_len;
			data_len = 0;
		}
	}
}

#endif

#if defined SOSEMANUK_VECTOR

/* ======================================================================== */
/*
 * Test code. This code is used to generate test vectors, with the
 * SOSEMANUK_VECTOR macro defined.
 */

/*
 * Generate 160 bytes of stream with the provided key and IV.
 */
static void
maketest(int tvn, unsigned char *key, size_t key_len,
	unsigned char *iv, size_t iv_len)
{
#ifdef SOSEMANUK_ECRYPT
	ECRYPT_ctx ctx;
#else
	sosemanuk_key_context kc;
	sosemanuk_run_context rc;
#endif
	unsigned char tmp[160];
	unsigned u;

	printf("=====================================================\n");
	printf("Detailed test vector %d:\n", tvn);

#ifdef SOSEMANUK_ECRYPT
	ECRYPT_init();
	ECRYPT_keysetup(&ctx, key, key_len * 8, iv_len * 8);
	ECRYPT_ivsetup(&ctx, iv);
#if defined SOSEMANUK_TEST_ENCRYPT_BYTES
	memset(tmp, 0, sizeof tmp);
	ECRYPT_encrypt_bytes(&ctx, tmp, tmp, sizeof tmp);
#elif defined SOSEMANUK_TEST_DECRYPT_BYTES
	memset(tmp, 0, sizeof tmp);
	ECRYPT_decrypt_bytes(&ctx, tmp, tmp, sizeof tmp);
#elif defined SOSEMANUK_TEST_ENCRYPT_BLOCKS
	memset(tmp, 0, sizeof tmp);
	ECRYPT_encrypt_blocks(&ctx, tmp, tmp, 2);
#elif defined SOSEMANUK_TEST_DECRYPT_BLOCKS
	memset(tmp, 0, sizeof tmp);
	ECRYPT_decrypt_blocks(&ctx, tmp, tmp, 2);
#elif defined SOSEMANUK_TEST_KEYSTREAM_BLOCKS
	ECRYPT_keystream_blocks(&ctx, tmp, 2);
#else
	ECRYPT_keystream_bytes(&ctx, tmp, sizeof tmp);
#endif
#else
	sosemanuk_schedule(&kc, key, key_len);
	sosemanuk_init(&rc, &kc, iv, iv_len);
	sosemanuk_prng(&rc, tmp, sizeof tmp);
#endif

	printf("\n");
	printf("Total output:");
	for (u = 0; u < sizeof tmp; u ++) {
		if ((u & 0x0F) == 0)
			printf("\n");
		printf(" %02X", (unsigned)tmp[u]);
	}
	printf("\n\n");
}

int
main(void)
{
	static unsigned char key1[] = { 0xA7, 0xC0, 0x83, 0xFE, 0xB7 };
	static unsigned char iv1[] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};

	static unsigned char key2[] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	static unsigned char iv2[] = {
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
	};

	maketest(1, key1, sizeof key1, iv1, sizeof iv1);
	maketest(2, key2, sizeof key2, iv2, sizeof iv2);
	return 0;
}

#elif defined SOSEMANUK_SPEED

/* ======================================================================== */
/*
 * Test code. This code is used to measure implementation speed. The
 * provided argument is the size of benched output stream, in megabytes.
 */

static void
usage(void)
{
	fprintf(stderr, "missing argument: output length (in megabytes)\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	static unsigned char key[] = { 0xA7, 0xC0, 0x83, 0xFE, 0xB7 };
	static unsigned char iv[] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	sosemanuk_key_context kc;
	sosemanuk_run_context rc;
	unsigned long speed_counter;
	clock_t orig, end;
	double nw, ts;
	unum32 sum;

	if (argc < 2)
		usage();
	speed_counter = strtoul(argv[1], 0, 0);
	speed_counter = (speed_counter * 65536UL) / 5;
	if (speed_counter == 0)
		usage();
	nw = (double)speed_counter * 20.0;
	printf("number of 32-bit words: %.0f\n", nw);
	sosemanuk_schedule(&kc, key, sizeof key);
	sosemanuk_init(&rc, &kc, iv, sizeof iv);
	sosemanuk_internal(&rc, 16);
	orig = clock();
	sum = sosemanuk_internal(&rc, speed_counter);
	end = clock();
	ts = (double)end / CLOCKS_PER_SEC - (double)orig / CLOCKS_PER_SEC;
	if (ts <= 1.0) {
		printf("too fast: no meaningful result\n");
	} else {
		printf("elapsed time: %.4f seconds\n", ts);
		printf("32-bit words per second: %.0f\n", nw / ts);
	}
	printf("sum = %08lX\n", (unsigned long)sum);
	return 0;
}

#endif
