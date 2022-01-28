/* $Id: md5.c 216 2010-06-08 09:46:57Z tp $ */
/*
 * MD5 implementation.
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

#include "sph_md5.h"

#define F(B, C, D)     ((((C) ^ (D)) & (B)) ^ (D))
#define G(B, C, D)     ((((C) ^ (B)) & (D)) ^ (C))
#define H(B, C, D)     ((B) ^ (C) ^ (D))
#define I(B, C, D)     ((C) ^ ((B) | ~(D)))

#define ROTL(x, n)    SPH_ROTL32(SPH_T32(x), n)

static const sph_u32 IV[4] = {
	SPH_C32(0x67452301), SPH_C32(0xEFCDAB89),
	SPH_C32(0x98BADCFE), SPH_C32(0x10325476)
};

#define MD5_ROUND_BODY(in, r)   do { \
		sph_u32 A, B, C, D; \
 \
		A = (r)[0]; \
		B = (r)[1]; \
		C = (r)[2]; \
		D = (r)[3]; \
 \
  A = SPH_T32(B + ROTL(A + F(B, C, D) + in( 0) + SPH_C32(0xD76AA478), 7)); \
  D = SPH_T32(A + ROTL(D + F(A, B, C) + in( 1) + SPH_C32(0xE8C7B756), 12)); \
  C = SPH_T32(D + ROTL(C + F(D, A, B) + in( 2) + SPH_C32(0x242070DB), 17)); \
  B = SPH_T32(C + ROTL(B + F(C, D, A) + in( 3) + SPH_C32(0xC1BDCEEE), 22)); \
  A = SPH_T32(B + ROTL(A + F(B, C, D) + in( 4) + SPH_C32(0xF57C0FAF), 7)); \
  D = SPH_T32(A + ROTL(D + F(A, B, C) + in( 5) + SPH_C32(0x4787C62A), 12)); \
  C = SPH_T32(D + ROTL(C + F(D, A, B) + in( 6) + SPH_C32(0xA8304613), 17)); \
  B = SPH_T32(C + ROTL(B + F(C, D, A) + in( 7) + SPH_C32(0xFD469501), 22)); \
  A = SPH_T32(B + ROTL(A + F(B, C, D) + in( 8) + SPH_C32(0x698098D8), 7)); \
  D = SPH_T32(A + ROTL(D + F(A, B, C) + in( 9) + SPH_C32(0x8B44F7AF), 12)); \
  C = SPH_T32(D + ROTL(C + F(D, A, B) + in(10) + SPH_C32(0xFFFF5BB1), 17)); \
  B = SPH_T32(C + ROTL(B + F(C, D, A) + in(11) + SPH_C32(0x895CD7BE), 22)); \
  A = SPH_T32(B + ROTL(A + F(B, C, D) + in(12) + SPH_C32(0x6B901122), 7)); \
  D = SPH_T32(A + ROTL(D + F(A, B, C) + in(13) + SPH_C32(0xFD987193), 12)); \
  C = SPH_T32(D + ROTL(C + F(D, A, B) + in(14) + SPH_C32(0xA679438E), 17)); \
  B = SPH_T32(C + ROTL(B + F(C, D, A) + in(15) + SPH_C32(0x49B40821), 22)); \
 \
  A = SPH_T32(B + ROTL(A + G(B, C, D) + in( 1) + SPH_C32(0xF61E2562), 5)); \
  D = SPH_T32(A + ROTL(D + G(A, B, C) + in( 6) + SPH_C32(0xC040B340), 9)); \
  C = SPH_T32(D + ROTL(C + G(D, A, B) + in(11) + SPH_C32(0x265E5A51), 14)); \
  B = SPH_T32(C + ROTL(B + G(C, D, A) + in( 0) + SPH_C32(0xE9B6C7AA), 20)); \
  A = SPH_T32(B + ROTL(A + G(B, C, D) + in( 5) + SPH_C32(0xD62F105D), 5)); \
  D = SPH_T32(A + ROTL(D + G(A, B, C) + in(10) + SPH_C32(0x02441453), 9)); \
  C = SPH_T32(D + ROTL(C + G(D, A, B) + in(15) + SPH_C32(0xD8A1E681), 14)); \
  B = SPH_T32(C + ROTL(B + G(C, D, A) + in( 4) + SPH_C32(0xE7D3FBC8), 20)); \
  A = SPH_T32(B + ROTL(A + G(B, C, D) + in( 9) + SPH_C32(0x21E1CDE6), 5)); \
  D = SPH_T32(A + ROTL(D + G(A, B, C) + in(14) + SPH_C32(0xC33707D6), 9)); \
  C = SPH_T32(D + ROTL(C + G(D, A, B) + in( 3) + SPH_C32(0xF4D50D87), 14)); \
  B = SPH_T32(C + ROTL(B + G(C, D, A) + in( 8) + SPH_C32(0x455A14ED), 20)); \
  A = SPH_T32(B + ROTL(A + G(B, C, D) + in(13) + SPH_C32(0xA9E3E905), 5)); \
  D = SPH_T32(A + ROTL(D + G(A, B, C) + in( 2) + SPH_C32(0xFCEFA3F8), 9)); \
  C = SPH_T32(D + ROTL(C + G(D, A, B) + in( 7) + SPH_C32(0x676F02D9), 14)); \
  B = SPH_T32(C + ROTL(B + G(C, D, A) + in(12) + SPH_C32(0x8D2A4C8A), 20)); \
 \
  A = SPH_T32(B + ROTL(A + H(B, C, D) + in( 5) + SPH_C32(0xFFFA3942), 4)); \
  D = SPH_T32(A + ROTL(D + H(A, B, C) + in( 8) + SPH_C32(0x8771F681), 11)); \
  C = SPH_T32(D + ROTL(C + H(D, A, B) + in(11) + SPH_C32(0x6D9D6122), 16)); \
  B = SPH_T32(C + ROTL(B + H(C, D, A) + in(14) + SPH_C32(0xFDE5380C), 23)); \
  A = SPH_T32(B + ROTL(A + H(B, C, D) + in( 1) + SPH_C32(0xA4BEEA44), 4)); \
  D = SPH_T32(A + ROTL(D + H(A, B, C) + in( 4) + SPH_C32(0x4BDECFA9), 11)); \
  C = SPH_T32(D + ROTL(C + H(D, A, B) + in( 7) + SPH_C32(0xF6BB4B60), 16)); \
  B = SPH_T32(C + ROTL(B + H(C, D, A) + in(10) + SPH_C32(0xBEBFBC70), 23)); \
  A = SPH_T32(B + ROTL(A + H(B, C, D) + in(13) + SPH_C32(0x289B7EC6), 4)); \
  D = SPH_T32(A + ROTL(D + H(A, B, C) + in( 0) + SPH_C32(0xEAA127FA), 11)); \
  C = SPH_T32(D + ROTL(C + H(D, A, B) + in( 3) + SPH_C32(0xD4EF3085), 16)); \
  B = SPH_T32(C + ROTL(B + H(C, D, A) + in( 6) + SPH_C32(0x04881D05), 23)); \
  A = SPH_T32(B + ROTL(A + H(B, C, D) + in( 9) + SPH_C32(0xD9D4D039), 4)); \
  D = SPH_T32(A + ROTL(D + H(A, B, C) + in(12) + SPH_C32(0xE6DB99E5), 11)); \
  C = SPH_T32(D + ROTL(C + H(D, A, B) + in(15) + SPH_C32(0x1FA27CF8), 16)); \
  B = SPH_T32(C + ROTL(B + H(C, D, A) + in( 2) + SPH_C32(0xC4AC5665), 23)); \
 \
  A = SPH_T32(B + ROTL(A + I(B, C, D) + in( 0) + SPH_C32(0xF4292244), 6)); \
  D = SPH_T32(A + ROTL(D + I(A, B, C) + in( 7) + SPH_C32(0x432AFF97), 10)); \
  C = SPH_T32(D + ROTL(C + I(D, A, B) + in(14) + SPH_C32(0xAB9423A7), 15)); \
  B = SPH_T32(C + ROTL(B + I(C, D, A) + in( 5) + SPH_C32(0xFC93A039), 21)); \
  A = SPH_T32(B + ROTL(A + I(B, C, D) + in(12) + SPH_C32(0x655B59C3), 6)); \
  D = SPH_T32(A + ROTL(D + I(A, B, C) + in( 3) + SPH_C32(0x8F0CCC92), 10)); \
  C = SPH_T32(D + ROTL(C + I(D, A, B) + in(10) + SPH_C32(0xFFEFF47D), 15)); \
  B = SPH_T32(C + ROTL(B + I(C, D, A) + in( 1) + SPH_C32(0x85845DD1), 21)); \
  A = SPH_T32(B + ROTL(A + I(B, C, D) + in( 8) + SPH_C32(0x6FA87E4F), 6)); \
  D = SPH_T32(A + ROTL(D + I(A, B, C) + in(15) + SPH_C32(0xFE2CE6E0), 10)); \
  C = SPH_T32(D + ROTL(C + I(D, A, B) + in( 6) + SPH_C32(0xA3014314), 15)); \
  B = SPH_T32(C + ROTL(B + I(C, D, A) + in(13) + SPH_C32(0x4E0811A1), 21)); \
  A = SPH_T32(B + ROTL(A + I(B, C, D) + in( 4) + SPH_C32(0xF7537E82), 6)); \
  D = SPH_T32(A + ROTL(D + I(A, B, C) + in(11) + SPH_C32(0xBD3AF235), 10)); \
  C = SPH_T32(D + ROTL(C + I(D, A, B) + in( 2) + SPH_C32(0x2AD7D2BB), 15)); \
  B = SPH_T32(C + ROTL(B + I(C, D, A) + in( 9) + SPH_C32(0xEB86D391), 21)); \
 \
  r[0] = SPH_T32((r)[0] + A); \
  r[1] = SPH_T32((r)[1] + B); \
  r[2] = SPH_T32((r)[2] + C); \
  r[3] = SPH_T32((r)[3] + D); \
	} while (0)

/*
 * One round of MD5. The data must be aligned for 32-bit access.
 */
static void
md5_round(const unsigned char *data, sph_u32 r[4])
{
	/*
	 * On machines with fast little-endian decoding, we simply
	 * reread from the input buffer.
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

	MD5_ROUND_BODY(X, r);

#undef X
}

/* see sph_md5.h */
void
sph_md5_init(void *cc)
{
	sph_md5_context *sc;

	sc = cc;
	memcpy(sc->val, IV, sizeof IV);
#if SPH_64
	sc->count = 0;
#else
	sc->count_high = sc->count_low = 0;
#endif
}

#define RFUN   md5_round
#define HASH   md5
#define LE32   1
#include "md_helper.ch"

/* see sph_md5.h */
void
sph_md5_close(void *cc, void *dst)
{
	md5_close(cc, dst, 4);
	sph_md5_init(cc);
}

/* see sph_md5.h */
void
sph_md5_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	md5_addbits_and_close(cc, ub, n, dst, 4);
	sph_md5_init(cc);
}

/* see sph_md5.h */
void
sph_md5_comp(const sph_u32 msg[16], sph_u32 val[4])
{
#define X(i)   msg[i]
	MD5_ROUND_BODY(X, val);
#undef X
}
