/* $Id: md4.c 216 2010-06-08 09:46:57Z tp $ */
/*
 * MD4 implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#include <stddef.h>
#include <string.h>

#include "sph_md4.h"

#define F(B, C, D)     ((((C) ^ (D)) & (B)) ^ (D))
#define G(B, C, D)     (((D) & (C)) | (((D) | (C)) & (B)))
#define H(B, C, D)     ((B) ^ (C) ^ (D))

#define ROTL   SPH_ROTL32

static const sph_u32 IV[4] = {
	SPH_C32(0x67452301), SPH_C32(0xEFCDAB89),
	SPH_C32(0x98BADCFE), SPH_C32(0x10325476)
};

#define MD4_ROUND_BODY(in, r)   do { \
		sph_u32 A, B, C, D; \
 \
		A = (r)[0]; \
		B = (r)[1]; \
		C = (r)[2]; \
		D = (r)[3]; \
 \
  A = ROTL(SPH_T32(A + F(B, C, D) + in( 0)), 3); \
  D = ROTL(SPH_T32(D + F(A, B, C) + in( 1)), 7); \
  C = ROTL(SPH_T32(C + F(D, A, B) + in( 2)), 11); \
  B = ROTL(SPH_T32(B + F(C, D, A) + in( 3)), 19); \
  A = ROTL(SPH_T32(A + F(B, C, D) + in( 4)), 3); \
  D = ROTL(SPH_T32(D + F(A, B, C) + in( 5)), 7); \
  C = ROTL(SPH_T32(C + F(D, A, B) + in( 6)), 11); \
  B = ROTL(SPH_T32(B + F(C, D, A) + in( 7)), 19); \
  A = ROTL(SPH_T32(A + F(B, C, D) + in( 8)), 3); \
  D = ROTL(SPH_T32(D + F(A, B, C) + in( 9)), 7); \
  C = ROTL(SPH_T32(C + F(D, A, B) + in(10)), 11); \
  B = ROTL(SPH_T32(B + F(C, D, A) + in(11)), 19); \
  A = ROTL(SPH_T32(A + F(B, C, D) + in(12)), 3); \
  D = ROTL(SPH_T32(D + F(A, B, C) + in(13)), 7); \
  C = ROTL(SPH_T32(C + F(D, A, B) + in(14)), 11); \
  B = ROTL(SPH_T32(B + F(C, D, A) + in(15)), 19); \
 \
  A = ROTL(SPH_T32(A + G(B, C, D) + in( 0) + SPH_C32(0x5A827999)), 3); \
  D = ROTL(SPH_T32(D + G(A, B, C) + in( 4) + SPH_C32(0x5A827999)), 5); \
  C = ROTL(SPH_T32(C + G(D, A, B) + in( 8) + SPH_C32(0x5A827999)), 9); \
  B = ROTL(SPH_T32(B + G(C, D, A) + in(12) + SPH_C32(0x5A827999)), 13); \
  A = ROTL(SPH_T32(A + G(B, C, D) + in( 1) + SPH_C32(0x5A827999)), 3); \
  D = ROTL(SPH_T32(D + G(A, B, C) + in( 5) + SPH_C32(0x5A827999)), 5); \
  C = ROTL(SPH_T32(C + G(D, A, B) + in( 9) + SPH_C32(0x5A827999)), 9); \
  B = ROTL(SPH_T32(B + G(C, D, A) + in(13) + SPH_C32(0x5A827999)), 13); \
  A = ROTL(SPH_T32(A + G(B, C, D) + in( 2) + SPH_C32(0x5A827999)), 3); \
  D = ROTL(SPH_T32(D + G(A, B, C) + in( 6) + SPH_C32(0x5A827999)), 5); \
  C = ROTL(SPH_T32(C + G(D, A, B) + in(10) + SPH_C32(0x5A827999)), 9); \
  B = ROTL(SPH_T32(B + G(C, D, A) + in(14) + SPH_C32(0x5A827999)), 13); \
  A = ROTL(SPH_T32(A + G(B, C, D) + in( 3) + SPH_C32(0x5A827999)), 3); \
  D = ROTL(SPH_T32(D + G(A, B, C) + in( 7) + SPH_C32(0x5A827999)), 5); \
  C = ROTL(SPH_T32(C + G(D, A, B) + in(11) + SPH_C32(0x5A827999)), 9); \
  B = ROTL(SPH_T32(B + G(C, D, A) + in(15) + SPH_C32(0x5A827999)), 13); \
 \
  A = ROTL(SPH_T32(A + H(B, C, D) + in( 0) + SPH_C32(0x6ED9EBA1)), 3); \
  D = ROTL(SPH_T32(D + H(A, B, C) + in( 8) + SPH_C32(0x6ED9EBA1)), 9); \
  C = ROTL(SPH_T32(C + H(D, A, B) + in( 4) + SPH_C32(0x6ED9EBA1)), 11); \
  B = ROTL(SPH_T32(B + H(C, D, A) + in(12) + SPH_C32(0x6ED9EBA1)), 15); \
  A = ROTL(SPH_T32(A + H(B, C, D) + in( 2) + SPH_C32(0x6ED9EBA1)), 3); \
  D = ROTL(SPH_T32(D + H(A, B, C) + in(10) + SPH_C32(0x6ED9EBA1)), 9); \
  C = ROTL(SPH_T32(C + H(D, A, B) + in( 6) + SPH_C32(0x6ED9EBA1)), 11); \
  B = ROTL(SPH_T32(B + H(C, D, A) + in(14) + SPH_C32(0x6ED9EBA1)), 15); \
  A = ROTL(SPH_T32(A + H(B, C, D) + in( 1) + SPH_C32(0x6ED9EBA1)), 3); \
  D = ROTL(SPH_T32(D + H(A, B, C) + in( 9) + SPH_C32(0x6ED9EBA1)), 9); \
  C = ROTL(SPH_T32(C + H(D, A, B) + in( 5) + SPH_C32(0x6ED9EBA1)), 11); \
  B = ROTL(SPH_T32(B + H(C, D, A) + in(13) + SPH_C32(0x6ED9EBA1)), 15); \
  A = ROTL(SPH_T32(A + H(B, C, D) + in( 3) + SPH_C32(0x6ED9EBA1)), 3); \
  D = ROTL(SPH_T32(D + H(A, B, C) + in(11) + SPH_C32(0x6ED9EBA1)), 9); \
  C = ROTL(SPH_T32(C + H(D, A, B) + in( 7) + SPH_C32(0x6ED9EBA1)), 11); \
  B = ROTL(SPH_T32(B + H(C, D, A) + in(15) + SPH_C32(0x6ED9EBA1)), 15); \
 \
		(r)[0] = SPH_T32(r[0] + A); \
		(r)[1] = SPH_T32(r[1] + B); \
		(r)[2] = SPH_T32(r[2] + C); \
		(r)[3] = SPH_T32(r[3] + D); \
	} while (0)

/*
 * One round of MD4. The data must be aligned for 32-bit access.
 */
static void
md4_round(const unsigned char *data, sph_u32 r[4])
{
	/*
	 * On machines with native little-endian representation, we do
	 * not use local variables for the message blocks: we simply
	 * reread from the input buffer. Speedup is about 3% on Athlon XP.
	 */
#if SPH_LITTLE_FAST
#define X(idx)    sph_dec32le_aligned(data + 4 * (idx))
#else
	sph_u32 X_var[16];
	int i;

	for (i = 0; i < 16; i ++)
		X_var[i] = sph_dec32le_aligned(data + 4 * i);
#define X(idx)    X_var[idx]
#endif

	MD4_ROUND_BODY(X, r);

#undef X
}

/* see sph_md4.h */
void
sph_md4_init(void *cc)
{
	sph_md4_context *sc;

	sc = cc;
	memcpy(sc->val, IV, sizeof IV);
#if SPH_64
	sc->count = 0;
#else
	sc->count_high = sc->count_low = 0;
#endif
}

#define RFUN   md4_round
#define HASH   md4
#define LE32   1
#include "md_helper.ch"

/* see sph_md4.h */
void
sph_md4_close(void *cc, void *dst)
{
	md4_close(cc, dst, 4);
	sph_md4_init(cc);
}

/* see sph_md4.h */
void
sph_md4_comp(const sph_u32 msg[16], sph_u32 val[4])
{
#define X(i)   msg[i]
	MD4_ROUND_BODY(X, val);
#undef X
}
