/*
 * DECIMv2 reference implementation.
 *
 * This code is supposed to run on any conforming C implementation (C90
 * or later). 
 *
 * Because DECIM is a hardware candidate, we deliberately made a very 
 * easy to understand and not at all optimised implementation. 
 * 
 * (c) 2005 X-CRYPT project. This software is provided 'as-is', without
 * any express or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * excluding commercial applications, and to alter it and redistribute it
 * freely except for commercial applications.
 *
 * France Telecom believes that the Decim cipher is covered in part by
 * the patent application under reference PCT/FR 04/2070 in co-ownership
 * between France Telecom and l'Universite de Caen Basse-Normandie. 
 * The X-CRYPT project members do not know of other licensed, patented
 * or otherwise legally restricted existing work that the Decim cipher
 * would be based on.
 *
 * Technical remarks and questions can be addressed to
 * <come.berbain@francetelecom.com>
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"ecrypt-sync.h"




/* update the LFSR (shift the bits, input new bit) */
void decim_lfsr_clock(ECRYPT_ctx *ctx);

/* this function computes the boolean function on the LFSR state */
void decim_lfsr_filter(ECRYPT_ctx *ctx);

void decim_absg(ECRYPT_ctx *ctx, u8 bit);

/* clock the LFSR for the IV injection
 * apply the  boolean function
 * compute the next bit of the LFSR
 * update the LFSR state*/ 
void decim_lfsr_init(ECRYPT_ctx *ctx);

void decim_step(ECRYPT_ctx *ctx);

/* when we got enough bits of key stream
 * pack it into a key stream byte
 * is_stream_byte: is stream_byte a valid key stream byte?
 * */
void decim_process_buffer(ECRYPT_ctx *ctx, int *is_stream_byte, u8 *stream_byte);




