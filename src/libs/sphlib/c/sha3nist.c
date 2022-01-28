/* $Id: sha3nist.c 154 2010-04-26 17:00:24Z tp $ */
/*
 * NIST SHA-3 competition compatibility code.
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

#include "sha3nist.h"

/*
 * Compute the output, using the provided extra bits (0 to 7 bits).
 * If the output is already computed, then this function returns 0.
 * On error (invalid saved output length), -1 is returned.
 */
static int
compute_output(hashState *state, unsigned ub, unsigned n)
{
	if (state->output_computed)
		return 0;
	switch (state->hashbitlen) {
	case 224:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 224_addbits_and_close)(
			&state->u.ctx224, ub, n, state->output);
		break;
	case 256:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 256_addbits_and_close)(
			&state->u.ctx256, ub, n, state->output);
		break;
	case 384:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 384_addbits_and_close)(
			&state->u.ctx384, ub, n, state->output);
		break;
	case 512:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 512_addbits_and_close)(
			&state->u.ctx512, ub, n, state->output);
		break;
	default:
		return -1;
	}
	state->output_computed = 1;
	return 0;
}

/* see sha3nist.h */
HashReturn
Init(hashState *state, int hashbitlen)
{
	switch (hashbitlen) {
	case 224:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 224_init)(&state->u.ctx224);
		break;
	case 256:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 256_init)(&state->u.ctx256);
		break;
	case 384:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 384_init)(&state->u.ctx384);
		break;
	case 512:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 512_init)(&state->u.ctx512);
		break;
	default:
		return BAD_HASHBITLEN;
	}
	state->hashbitlen = hashbitlen;
	state->output_computed = 0;
	return SUCCESS;
}

/* see sha3nist.h */
HashReturn
Update(hashState *state, const BitSequence *data, DataLength databitlen)
{
	unsigned extra;
	size_t len;

	if (state->output_computed)
		return FAIL;
	extra = (unsigned)databitlen & 0x7U;
	len = (size_t)(databitlen >> 3);
	switch (state->hashbitlen) {
	case 224:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 224)(
			&state->u.ctx224, data, len);
		break;
	case 256:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 256)(
			&state->u.ctx256, data, len);
		break;
	case 384:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 384)(
			&state->u.ctx384, data, len);
		break;
	case 512:
		SPH_CAT(SPH_CAT(sph_, SPH_NIST), 512)(
			&state->u.ctx512, data, len);
		break;
	default:
		return FAIL;
	}
	if (extra > 0)
		compute_output(state, data[len], extra);
	return SUCCESS;
}

/* see sha3nist.h */
HashReturn
Final(hashState *state, BitSequence *hashval)
{
	size_t len;

	if (compute_output(state, 0, 0) < 0)
		return FAIL;
	len = state->hashbitlen >> 3;
	if (len > 64)
		return FAIL;
	memcpy(hashval, state->output, len);
	return SUCCESS;
}

/* see sha3nist.h */
HashReturn
Hash(int hashbitlen, const BitSequence *data,
	DataLength databitlen, BitSequence *hashval)
{
	hashState st;
	HashReturn r;

	r = Init(&st, hashbitlen);
	if (r != SUCCESS)
		return r;
	r = Update(&st, data, databitlen);
	if (r != SUCCESS)
		return r;
	return Final(&st, hashval);
}
