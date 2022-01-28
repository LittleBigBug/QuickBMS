/* $Id: sph_sha3.h 208 2010-06-02 20:33:00Z tp $ */
/**
 * SHA-3 is the colloquial codename for the future standard hash function
 * from NIST. SHA-3 will be "API compatible" (same range of output sizes)
 * with the SHA-2 family (SHA-224, SHA-256, SHA-384 and SHA-512). This
 * library implements a number of SHA-3 candidates (functions submitted
 * to the NIST competition for selection of SHA-3). This header file
 * includes the header files for SHA-2 and all the implemented SHA-3
 * candidates.
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
 * @file     sph_sha3.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_SHA3_H__
#define SPH_SHA3_H__

#include "sph_sha2.h"

#include "sph_blake.h"
#include "sph_bmw.h"
#include "sph_cubehash.h"
#include "sph_echo.h"
#include "sph_fugue.h"
#include "sph_groestl.h"
#include "sph_hamsi.h"
#include "sph_jh.h"
#include "sph_keccak.h"
#include "sph_luffa.h"
#include "sph_shabal.h"
#include "sph_shavite.h"
#include "sph_simd.h"
#include "sph_skein.h"

#endif
