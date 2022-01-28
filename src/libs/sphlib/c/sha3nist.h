/* $Id: sha3nist.h 170 2010-05-07 13:27:24Z tp $ */
/**
 * NIST SHA-3 competition compatibility header.
 *
 * This file defines the API specified by NIST for the SHA-3 competition
 * candidates; it is applied over one of the compatible implementations
 * in this library. This header must first be modified to select a
 * function (inclusion of the relevant header files, and value of the
 * <code>SPH_NIST</code> macro).
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
 * @file     sha3nist.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_SHA3NIST_H__
#define SPH_SHA3NIST_H__

#ifndef DOXYGEN_IGNORE

/*
 * Here, define SPH_NIST to the base of the internal names for the
 * hash function. If SPH_NIST evaluates to the identifier "foo", then
 * the functions foo224, foo256, foo384 and foo512 will be used.
 */
#define SPH_NIST   sha

#include "sph_sha3.h"
#include <limits.h>

typedef unsigned char BitSequence;

#ifdef ULLONG_MAX
typedef unsigned long long DataLength;
#else
typedef unsigned long DataLength;
#endif

typedef enum {
	SUCCESS = 0,
	FAIL = 1,
	BAD_HASHBITLEN = 2
} HashReturn;

#define SPH_CAT(x, y)    SPH_CAT_(x, y)
#define SPH_CAT_(x, y)   x ## y

typedef struct {
	union {
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 224_context) ctx224;
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 256_context) ctx256;
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 384_context) ctx384;
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 512_context) ctx512;
	} u;
	int hashbitlen;
	BitSequence output[64];
	int output_computed;
} hashState;

HashReturn Init(hashState *state, int hashbitlen);

HashReturn Update(hashState *state,
	const BitSequence *data, DataLength databitlen);

HashReturn Final(hashState *state, BitSequence *hashval);

HashReturn Hash(int hashbitlen, const BitSequence *data,
	DataLength databitlen, BitSequence *hashval);

#endif

#endif
