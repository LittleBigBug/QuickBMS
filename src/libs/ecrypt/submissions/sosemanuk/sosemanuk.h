/*
 * SOSEMANUK reference API.
 *
 * This file documents the reference implementation API. If the
 * macro SOSEMANUK_ECRYPT is defined, the API follows the ECRYPT
 * conventions (types, function names...) and uses the ECRYPT files;
 * otherwise, a simpler API is used.
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

#ifndef SOSEMANUK_H__
#define SOSEMANUK_H__

/*
 * This macro enables the ECRYPT API, and disables the local API.
 * It is defined by default, for ECRYPT processing.
 */
#define SOSEMANUK_ECRYPT

#ifdef SOSEMANUK_ECRYPT

#include "ecrypt-sync.h"

#else

#include <limits.h>

/*
 * Input/Output is defined in terms of octets, but C provides only
 * the C notion of "byte". We require that C bytes are actually octets.
 */
#if CHAR_BIT != 8
#error We need 8-bit bytes
#endif

/*
 * We want an unsigned integer type with at least (and possibly exactly)
 * 32 bits. Such a type implements arithmetics modulo 2^n for a value
 * n greater than or equal to 32. The type is named "unum32".
 *
 * Note: we try to use C99 features such as <stdint.h>. This may prove
 * problematic on architectures which claim C99 conformance, but fail
 * to actually conform. If necessary, define the macro BROKEN_C99 to
 * fall back to C90, whatever the environment claims:
#define BROKEN_C99  1
 */

#if !defined BROKEN_C99 && defined __STDC__ && __STDC_VERSION__ >= 199901L

/*
 * C99 implementation. We use "uint_least32_t" which has the required
 * semantics.
 */
#include <stdint.h>
typedef uint_least32_t unum32;

#else

/*
 * Pre-C99 implementation. "unsigned long" is guaranteed to be wide
 * enough, but we want to use "unsigned int" if possible (especially
 * for 64-bit architectures).
 */
#if UINT_MAX >= 0xFFFFFFFF
typedef unsigned int unum32;
#else
typedef unsigned long unum32;
#endif

#endif

/*
 * We want (and sometimes need) to perform explicit truncations to 32 bits.
 */
#define ONE32    ((unum32)0xFFFFFFFF)
#define T32(x)   ((x) & ONE32)

/*
 * Some of our functions will be tagged as "inline" to help the compiler
 * optimize things. We use "inline" only if the compiler is advanced
 * enough to understand it; C99 compilers, and pre-C99 versions of gcc,
 * understand enough "inline" for our purposes.
 */
#if (!defined BROKEN_C99 && defined __STDC__ && __STDC_VERSION__ >= 199901L) \
	|| defined __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

/*
 * API description:
 *
 * The SOSEMANUK algorithm works with a secret key and an initial value (IV).
 * Two context structures are used:
 *
 * -- "sosemanuk_key_context" holds the processed secret key. The contents
 * of this structure depends only on the key, not the IV.
 *
 * -- "sosemanuk_run_context" holds the current cipher internal state. This
 * structure is initialized using the "sosemanuk_key_context" structure, and
 * the IV; it is updated each time some output is produced.
 *
 * Both structures may be allocated as local variables. There is no
 * other external allocation (using malloc() or any similar function).
 * There is no global state; hence, this code is thread-safe and
 * reentrant.
 */

typedef struct {
	/*
	 * Sub-keys for Serpent24.
	 */
	unum32 sk[100];
} sosemanuk_key_context;

typedef struct {
	/*
	 * Internal cipher state.
	 */
	unum32 s00, s01, s02, s03, s04, s05, s06, s07, s08, s09;
	unum32 r1, r2;

	/*
	 * Buffering: the stream cipher produces output data by
	 * blocks of 640 bits. buf[] contains such a block, and
	 * "ptr" is the index of the next output byte.
	 */
	unsigned char buf[80];
	unsigned ptr;
} sosemanuk_run_context;

/*
 * Key schedule: initialize the key context structure with the provided
 * secret key. The secret key is an array of 1 to 32 bytes.
 */
void sosemanuk_schedule(sosemanuk_key_context *kc,
	unsigned char *key, size_t key_len);

/*
 * Cipher initialization: the cipher internal state is initialized, using
 * the provided key context and IV. The IV length is up to 16 bytes. If
 * "iv_len" is 0 (no IV), then the "iv" parameter can be NULL.
 */
void sosemanuk_init(sosemanuk_run_context *rc,
	sosemanuk_key_context *kc, unsigned char *iv, size_t iv_len);

/*
 * Cipher operation, as a PRNG: the provided output buffer is filled with
 * pseudo-random bytes as output from the stream cipher.
 */
void sosemanuk_prng(sosemanuk_run_context *rc,
	unsigned char *out, size_t out_len);

/*
 * Cipher operation, as a stream cipher: data is read from the "in"
 * buffer, combined by XOR with the stream, and the result is written
 * in the "out" buffer. "in" and "out" must be either equal, or
 * reference distinct buffers (no partial overlap is allowed).
 */
void sosemanuk_encrypt(sosemanuk_run_context *rc,
	unsigned char *in, unsigned char *out, size_t data_len);

#endif

#endif
