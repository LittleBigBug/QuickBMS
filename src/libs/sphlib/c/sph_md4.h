/* $Id: sph_md4.h 216 2010-06-08 09:46:57Z tp $ */
/**
 * MD4 interface.
 *
 * MD4 is described in RFC 1320.
 *
 * @warning   The MD4 hash function is considered as severely broken,
 * cryptographically speaking: collisions have been published and new
 * collisions can be built very efficiently. Do not use MD4 for actual
 * security purposes.
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
 * @file     sph_md4.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_MD4_H__
#define SPH_MD4_H__

#include <stddef.h>
#include "sph_types.h"

/**
 * Output size (in bits) for MD4.
 */
#define SPH_SIZE_md4   128

/**
 * This structure is a context for MD4 computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * a MD4 computation has been performed, the context can be reused for
 * another computation.
 *
 * The contents of this structure are private. A running MD4 computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[64];    /* first field, for alignment */
	sph_u32 val[4];
#if SPH_64
	sph_u64 count;
#else
	sph_u32 count_high, count_low;
#endif
#endif
} sph_md4_context;

/**
 * Initialize a MD4 context. This process performs no memory allocation.
 *
 * @param cc   the MD4 context (pointer to a <code>sph_md4_context</code>)
 */
void sph_md4_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the MD4 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_md4(void *cc, const void *data, size_t len);

/**
 * Terminate the current MD4 computation and output the result into the
 * provided buffer. The destination buffer must be wide enough to
 * accomodate the result (16 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the MD4 context
 * @param dst   the destination buffer
 */
void sph_md4_close(void *cc, void *dst);

/**
 * Apply the MD4 compression function on the provided data. The
 * <code>msg</code> parameter contains the 16 32-bit input blocks,
 * as numerical values (hence after the little-endian decoding). The
 * <code>val</code> parameter contains the 4 32-bit input blocks for
 * the compression function; the output is written in place in this
 * array.
 *
 * @param msg   the message block (16 values)
 * @param val   the function 128-bit input and output
 */
void sph_md4_comp(const sph_u32 msg[16], sph_u32 val[4]);

#endif
