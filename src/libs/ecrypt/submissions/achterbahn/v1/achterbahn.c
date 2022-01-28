/* ------------------------------------------------------------------------- *
 *
 *   Program:   A C H T E R B A H N  (Implementation)
 *
 *              A new binary additive synchroneous stream cipher submitted 
 *              to the ECRYPT call for stream cipher primitives, profile 2.
 *              (80 bit key length, efficient for hardware applications)
 *
 *   Authors:   Berndt M. Gammel, Rainer Goettfert, Oliver Kniffler
 *
 *     Email:   berndt.gammel@infineon.com
 *              rainer.goettfert@infineon.com
 *              oliver.kniffler@infineon.com
 *
 *   Address:   Infineon Technologies AG
 *              St.-Martin-Str. 76
 *              D-81541 Munich
 *              Germany
 *
 *   Version:   1.0  (last change: Apr 18, 2005)
 *                
 *  Language:   ANSI C99
 *
 *   Sources:   achterbahn.c
 *                
 *  Includes:   ecrypt-sync.h, 
 *              ecrypt-portable.h, 
 *              ecrypt-config.h, 
 *              ecrypt-machine.h
 *                
 *  Makefile:   Makefile
 *
 * Platforms:   This program has been verified on the following platforms:
 *              gcc 3.4.3 on S.u.S.E. Linux 9.1
 *              gcc 3.3.2 on SunOS 5.7 / Solaris 2.7S
 *              gcc 3.3.2 on Redhat Linux 9.2
 *              gcc 3.3.1 on Cygwin (Windows 2000)
 *
 * Copyright:   (C) 2005 by Berndt M. Gammel, Rainer Goettfert, Oliver Kniffler,
 *              Infineon Technologies AG, St.-Martin-Str. 76, 
 *              D-81541 Munich, Germany.
 *              
 *              This software is free for commercial and non-commercial 
 *              use subject to the following conditions:
 *
 *              1.  Copyright remains vested in Infineon Technologies AG, 
 *              and Copyright notices in the code are not to be removed.  
 *              If this software is used in a product, Infineon Technologies AG
 *              should be given attribution as the author of the Achterbahn
 *              encryption algorithm. This can be in the form of a textual
 *              message at program startup or in documentation (online or
 *              textual) provided with the software.
 *
 *              2.  Redistribution and use in source and binary forms,
 *              with or without modification, are permitted provided that 
 *              the following conditions are met:
 *
 *              a. Redistributions of source code must retain the copyright notice,
 *                 this list of conditions and the following disclaimer.
 *
 *              b. Redistributions in binary form must reproduce the above
 *                 copyright notice, this list of conditions and the following 
 *                 disclaimer in the documentation and/or other materials provided 
 *                 with the distribution.
 *
 *              c. All advertising materials mentioning features or use of this
 *                 software must display the following acknowledgement:  This product
 *                 includes software developed by Infineon Technologies AG.
 *
 *              3.  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *              WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *              MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
 *              INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 *              CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *              EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *              PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *              PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *              LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *              NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *              SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *              4.  The license and distribution terms for any publically available
 *              version or derivative of this code cannot be changed, that is, 
 *              this code cannot simply be copied and put under another distribution 
 *              license including the GNU Public License.
 *             
 *              5.  A free and irrevocable license is hereby granted for the 
 *              use of the Achterbahn encryption algorithm for any purpose. 
 *
 * ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- *
 * if DONT_COMPILE_TEST_CODE is defined the implementation test code (main) 
 * in this file won't be compiled. 
 * Preferably this flag is defined by the Makefile, not in the code.
 * ------------------------------------------------------------------------- */

#ifdef ECRYPT_API
#define DONT_COMPILE_TEST_CODE
#endif

/* ------------------------------------------------------------------------- *
 * if DONT_USE_CONFIGURATION is defined the linear filtering of the
 * output of the driving NLFSRs and the corresponding configuration
 * setup phase is removed.
 * Preferably this flag is defined by the Makefile, not in the code.
 * ------------------------------------------------------------------------- */

/* #define DONT_USE_CONFIGURATION */

/* ------------------------------------------------------------------------- *
 * general include files
 * ------------------------------------------------------------------------- */

#include <assert.h>

#ifndef DONT_COMPILE_TEST_CODE
#include <stdio.h>
#include <string.h>
#endif

/* ------------------------------------------------------------------------- *
 * ECRYPT include file
 * ------------------------------------------------------------------------- */

#include "ecrypt-sync.h"

/* ------------------------------------------------------------------------- *
 * Achterbahn local definitions
 * ------------------------------------------------------------------------- */

enum {
  false  = 0U,                 /* truth value */
  true   = 1U,                 /* truth value */
                            
  ZERO   = 0U,                 /* unsigned constant */
  ONE    = 1U,                 /* unsigned constant */
                               
  maxkey = ECRYPT_MAXKEYSIZE,  /* key size */
  maxini = 112U,               /* initialization cycles for driving NLFSRs */
  cfgini = 48U,                /* initialization cycles for output configuration */

  A_size = 22U,                /* NLFSR A: n = 22 */
  B_size = 23U,                /* NLFSR B: n = 23 */
  C_size = 25U,                /* NLFSR C: n = 25 */
  D_size = 26U,                /* NLFSR D: n = 26 */
  E_size = 27U,                /* NLFSR E: n = 27 */
  F_size = 28U,                /* NLFSR F: n = 28 */
  G_size = 29U,                /* NLFSR G: n = 29 */
  H_size = 31U,                /* NLFSR H: n = 31 */
  V_size = 64U,                /* NLFSR V: n = 64 */
                               
  A_mask = 0x003FFFFFU,        /* NLFSR A: n = 22 */
  B_mask = 0x007FFFFFU,        /* NLFSR B: n = 23 */
  C_mask = 0x01FFFFFFU,        /* NLFSR C: n = 25 */
  D_mask = 0x03FFFFFFU,        /* NLFSR D: n = 26 */
  E_mask = 0x07FFFFFFU,        /* NLFSR E: n = 27 */
  F_mask = 0x0FFFFFFFU,        /* NLFSR F: n = 28 */
  G_mask = 0x1FFFFFFFU,        /* NLFSR G: n = 29 */
  H_mask = 0x7FFFFFFFU,        /* NLFSR H: n = 31 */

  A_period = (ONE<<A_size)-1,  /* NLFSR A: period */
  B_period = (ONE<<B_size)-1,  /* NLFSR B: period */
  C_period = (ONE<<C_size)-1,  /* NLFSR C: period */
  D_period = (ONE<<D_size)-1,  /* NLFSR D: period */
  E_period = (ONE<<E_size)-1,  /* NLFSR E: period */
  F_period = (ONE<<F_size)-1,  /* NLFSR F: period */
  G_period = (ONE<<G_size)-1,  /* NLFSR G: period */
  H_period = (ONE<<H_size)-1   /* NLFSR H: period */
};

/* ------------------------------------------------------------------------- *
 *
 * Functions to step each NLFSR by one clock cycle implemented as macros.
 *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * NLFSR A  (length 22)
 * 
 * A(x) = x0 + x1x4x11 + x1x4x13x14 + x2x7 + x4x14 + x5 + x6 
 *           + x7 + x8x9 + x10 + x10x11 + x11 + x12 + x13 + x17 + x20
 * ------------------------------------------------------------------------- */

#define A_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^((x>>1)&(x>>4)&(x>>11)) \
	       ^((x>>1)&(x>>4)&(x>>13)&(x>>14))^((x>>2)&(x>>7)) \
	       ^((x>>4)&(x>>14))^(x>>5)^(x>>6)^(x>>7) \
	       ^((x>>8)&(x>>9))^(x>>10)^((x>>10)&(x>>11))^(x>>11) \
	       ^(x>>12)^(x>>13)^(x>>17)^(x>>20) \
         )) << 21)

/* ------------------------------------------------------------------------- *
 * NLFSR B  (length 23)
 * 
 * B(x) = x0 + x1x3x5x10 + x1x4 + x2x4x8 + x2x7 + x4x11x12x13 + x5x9 + x6 
 *           + x6x10 + x7 + x9 + x11 + x12 + x14 + x15 + x17 + x19 + x21 
 * ------------------------------------------------------------------------- */

#define B_cycle(x,feedin) \
    x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^(((x)>>1)&((x)>>3)&((x)>>5)&((x)>>10))^(((x)>>1)&((x)>>4)) \
               ^(((x)>>2)&((x)>>4)&((x)>>8))^(((x)>>2)&((x)>>7)) \
               ^(((x)>>4)&((x)>>11)&((x)>>12)&((x)>>13))^(((x)>>5)&((x)>>9)) \
               ^((x)>>6)^(((x)>>6)&((x)>>10))^((x)>>7)^((x)>>9)^((x)>>11) \
               ^((x)>>12)^((x)>>14)^((x)>>15)^((x)>>17)^((x)>>19)^((x)>>21) \
        )) << 22)

/* ------------------------------------------------------------------------- *
 * NLFSR C  (length 25)
 * 
 * C(x) = x0 + x1 + x1x4x11x15 + x1x6 + x2x5x8x10 + x3 + x4x13 + x5 
 *           + x5x11x14 + x6 + x7 + x8x16 + x9 + x12 + x12x15 
 *           + x14 + x15 + x17 + x18 + x22
 * ------------------------------------------------------------------------- */

#define C_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^((x)>>1)^(((x)>>1)&((x)>>4)&((x)>>11)&((x)>>15)) \
               ^(((x)>>1)&((x)>>6))^(((x)>>2)&((x)>>5)&((x)>>8)&((x)>>10)) \
               ^((x)>>3)^(((x)>>4)&((x)>>13))^((x)>>5) \
               ^(((x)>>5)&((x)>>11)&((x)>>14))^((x)>>6)^((x)>>7) \
               ^(((x)>>8)&((x)>>16))^((x)>>9)^((x)>>12)^(((x)>>12)&((x)>>15)) \
               ^((x)>>14)^((x)>>15)^((x)>>17)^((x)>>18)^((x)>>22) \
        )) << 24)

/* ------------------------------------------------------------------------- *
 * NLFSR D  (length 26)
 * 
 * D(x) = x0 + x1 + x1x3x14x16 + x1x6 + x4 + x4x7 + x4x15x17 + x5 
 *           + x7 + x7x9x10 + x8 + x8x11x12x17 + x9 + x12x16 
 *           + x13 + x14 + x15x17 + x16 + x20 + x24
 * ------------------------------------------------------------------------- */

#define D_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^(x>>1)^((x>>1)&(x>>3)&(x>>14)&(x>>16)) \
	       ^((x>>1)&(x>>6))^(x>>4)^((x>>4)&(x>>7)) \
	       ^((x>>4)&(x>>15)&(x>>17))^(x>>5)^(x>>7) \
	       ^((x>>7)&(x>>9)&(x>>10))^(x>>8) \
	       ^((x>>8)&(x>>11)&(x>>12)&(x>>17))^(x>>9) \
	       ^((x>>12)&(x>>16))^(x>>13)^(x>>14) \
	       ^((x>>15)&(x>>17))^(x>>16)^(x>>20)^(x>>24) \
        )) << 25)

/* ------------------------------------------------------------------------- *
 * NLFSR E  (length 27)
 * 
 * E(x) = x0 + x1 + x1x8 + x2 + x3x5x16x17 + x3x12 + x5x6x15 + x6 
 *           + x7x12x14x15 + x8 + x9 + x10 + x11x17 + x13 + x14 
 *           + x15x18 + x16 + x19 + x21 + x23
 * ------------------------------------------------------------------------- */

#define E_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^(x>>1)^((x>>1)&(x>>8))^(x>>2) \
               ^((x>>3)&(x>>5)&(x>>16)&(x>>17))^((x>>3)&(x>>12)) \
               ^((x>>5)&(x>>6)&(x>>15))^(x>>6) \
               ^((x>>7)&(x>>12)&(x>>14)&(x>>15))^(x>>8)^(x>>9)^(x>>10) \
               ^((x>>11)&(x>>17))^(x>>13)^(x>>14)^((x>>15)&(x>>18)) \
               ^(x>>16)^(x>>19)^(x>>21)^(x>>23) \
        )) << 26)

/* ------------------------------------------------------------------------- *
 * NLFSR F  (length 28) 
 * 
 * F(x) = x0 + x1 + x2 + x5x14x19 + x6x9x17x18 + x6x10x12 + x7 
 *           + x9x17 + x10x12x19x20 + x10x18 + x11x14 + x12x13 
 *           + x15 + x17 + x19 + x20 + x22 + x27
 * ------------------------------------------------------------------------- */

#define F_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^(x>>1)^(x>>2)^((x>>5)&(x>>14)&(x>>19)) \
                   ^((x>>6)&(x>>9)&(x>>17)&(x>>18))^((x>>6)&(x>>10)&(x>>12)) \
                   ^((x>>7))^((x>>9)&(x>>17))^((x>>10)&(x>>12)&(x>>19)&(x>>20)) \
                   ^((x>>10)&(x>>18))^((x>>11)&(x>>14))^((x>>12)&(x>>13)) \
                   ^(x>>15)^(x>>17)^(x>>19)^(x>>20)^(x>>22)^(x>>27) \
         )) << 27)

/* ------------------------------------------------------------------------- *
 * NLFSR G  (length 29) 
 * 
 * G(x) = x0 + x1x5x15x21 + x2 + x2x7x17x20 + x3 + x5 + x5x7 + x6 
 *           + x6x20 + x8x19x21 + x9 + x10x14 + x11x16x18 + x13x18 
 *           + x14 + x15 + x16 + x18 + x21 + x27
 * ------------------------------------------------------------------------- */

#define G_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^((x>>1)&(x>>5)&(x>>15)&(x>>21))^(x>>2) \
                   ^((x>>2)&(x>>7)&(x>>17)&(x>>20))^(x>>3)^(x>>5) \
                   ^((x>>5)&(x>>7))^(x>>6)^((x>>6)&(x>>20)) \
                   ^((x>>8)&(x>>19)&(x>>21))^(x>>9)^((x>>10)&(x>>14)) \
                   ^((x>>11)&(x>>16)&(x>>18))^((x>>13)&(x>>18))^(x>>14)^(x>>15) \
                   ^(x>>16)^(x>>18)^(x>>21)^(x>>27) \
        )) << 28)

/* ------------------------------------------------------------------------- *
 * NLFSR H  (length 31)
 * 
 * H(x) = x0 + x1x2x19 + x1x12x14x17 + x2x5x13x20 + x3 + x5 + x5x15 
 *           + x7 + x10 + x11x18 + x16 + x16x22 + x17 + x17x21 
 *           + x18 + x19 + x20 + x21 + x24 + x30
 * ------------------------------------------------------------------------- */

#define H_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^(x)^(((x)>>1)&((x)>>2)&((x)>>19))^(((x)>>1)&((x)>>12)&((x)>>14)&((x)>>17)) \
               ^(((x)>>2)&((x)>>5)&((x)>>13)&((x)>>20))^((x)>>3)^((x)>>5) \
               ^(((x)>>5)&((x)>>15))^((x)>>7)^((x)>>10)^(((x)>>11)&((x)>>18))^((x)>>16) \
               ^(((x)>>16)&((x)>>22))^((x)>>17)^(((x)>>17)&((x)>>21))^((x)>>18) \
               ^((x)>>19)^((x)>>20)^((x)>>21)^(((x)>>24))^((x)>>30) \
        )) << 30)

/* ------------------------------------------------------------------------- *
 * NLFSR V (length 64)
 * 
 * V(x) = 1 + x0 + x3 + x7 + x10 + x12 + x27 + x28 + x38 + x46 + x47
 *               + x8x20 + x17x23 +x24x25 + x29x31 + x33x34x37  
 *               + x1x3x9x10 + x39x41x51x52
 * ------------------------------------------------------------------------- */

#define V_cycle(x,feedin) \
   x = ((x) >> ONE) \
      | ((ONE & ((feedin) \
               ^1^(x)^(x>>3)^(x>>7)^(x>>10)^(x>>12)^(x>>27)^(x>>28)^(x>>38) \
                 ^(x>>46)^(x>>47)^((x>>8)&(x>>20))^((x>>17)&(x>>23)) \
                 ^((x>>24)&(x>>25))^((x>>29)&(x>>31)) \
                 ^((x>>33)&(x>>34)&(x>>37))^((x>>39)&(x>>41)&(x>>51)&(x>>52)) \
                 ^((x>>1)&(x>>3)&(x>>9)&(x>>10)) \
         )) << 63)


/* ------------------------------------------------------------------------- *
 *
 * Setup of Achterbahn stream cipher. 
 *
 * Called by ECRYPT_keysetup() and ECRYPT_ivsetup().
 * 
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_setup (
  ECRYPT_ctx *ctx
  ) 
{ 
  u32 i;

  /* ----------------------------------------------------------------------- *
   * phase 1: 
   * load all 8 driving NLFSRs with the first key bits in parallel.
   * ----------------------------------------------------------------------- */
  ctx->A = ctx->key0 & A_mask;
  ctx->B = ctx->key0 & B_mask;
  ctx->C = ctx->key0 & C_mask;
  ctx->D = ctx->key0 & D_mask;
  ctx->E = ctx->key0 & E_mask;
  ctx->F = ctx->key0 & F_mask;
  ctx->G = ctx->key0 & G_mask;
  ctx->H = ctx->key0 & H_mask;

  /* ----------------------------------------------------------------------- *
   * phase 2: 
   * for each driving NLFSR, feed-in the key bits, not already loaded into the
   * register in phase 1, into the NLFSR.
   * ----------------------------------------------------------------------- */
  for (i = 0; i < maxkey-A_size; ++i) { A_cycle(ctx->A,ctx->ky[A_size+i]); }
  for (i = 0; i < maxkey-B_size; ++i) { B_cycle(ctx->B,ctx->ky[B_size+i]); }
  for (i = 0; i < maxkey-C_size; ++i) { C_cycle(ctx->C,ctx->ky[C_size+i]); }
  for (i = 0; i < maxkey-D_size; ++i) { D_cycle(ctx->D,ctx->ky[D_size+i]); }
  for (i = 0; i < maxkey-E_size; ++i) { E_cycle(ctx->E,ctx->ky[E_size+i]); }
  for (i = 0; i < maxkey-F_size; ++i) { F_cycle(ctx->F,ctx->ky[F_size+i]); }
  for (i = 0; i < maxkey-G_size; ++i) { G_cycle(ctx->G,ctx->ky[G_size+i]); }
  for (i = 0; i < maxkey-H_size; ++i) { H_cycle(ctx->H,ctx->ky[H_size+i]); }

  /* ----------------------------------------------------------------------- *
   * phase 3: 
   * for each driving NLFSR, feed-in all IV bits, into the NLFSR.
   * ----------------------------------------------------------------------- */
  for (i = 0; i < ctx->ivsize; ++i) { 
    A_cycle(ctx->A,ctx->iv[i]);
    B_cycle(ctx->B,ctx->iv[i]); 
    C_cycle(ctx->C,ctx->iv[i]); 
    D_cycle(ctx->D,ctx->iv[i]); 
    E_cycle(ctx->E,ctx->iv[i]); 
    F_cycle(ctx->F,ctx->iv[i]); 
    G_cycle(ctx->G,ctx->iv[i]); 
    H_cycle(ctx->H,ctx->iv[i]); 
  }

  /* ----------------------------------------------------------------------- *
   * phase 4: 
   * set the least significant bits (LSB) of NLFSRs A to H to 1.
   * ----------------------------------------------------------------------- */
  ctx->A |= ONE;
  ctx->B |= ONE;
  ctx->C |= ONE;
  ctx->D |= ONE;
  ctx->E |= ONE;
  ctx->F |= ONE;
  ctx->G |= ONE;
  ctx->H |= ONE;

  /* ----------------------------------------------------------------------- *
   * phase 5: 
   * warming up.
   * ----------------------------------------------------------------------- */
  for (i = 0; i < maxini-maxkey+A_size; ++i) { A_cycle(ctx->A,ZERO); }
  for (i = 0; i < maxini-maxkey+B_size; ++i) { B_cycle(ctx->B,ZERO); }
  for (i = 0; i < maxini-maxkey+C_size; ++i) { C_cycle(ctx->C,ZERO); }
  for (i = 0; i < maxini-maxkey+D_size; ++i) { D_cycle(ctx->D,ZERO); }
  for (i = 0; i < maxini-maxkey+E_size; ++i) { E_cycle(ctx->E,ZERO); }
  for (i = 0; i < maxini-maxkey+F_size; ++i) { F_cycle(ctx->F,ZERO); }
  for (i = 0; i < maxini-maxkey+G_size; ++i) { G_cycle(ctx->G,ZERO); }
  for (i = 0; i < maxini-maxkey+H_size; ++i) { H_cycle(ctx->H,ZERO); }


#ifndef DONT_USE_CONFIGURATION

  /* ----------------------------------------------------------------------- *
   * phase 6: 
   * Setup the configuration of the output logics of the 8 driving NLFSRs.
   * ----------------------------------------------------------------------- */

  /* load V with the first 64 key bits in parallel. */
  ctx->V = ctx->key0;

  /* feed-in the key bits, not already loaded, into the NLFSR. */
  for (i = 0; i < maxkey-V_size; ++i) 
    V_cycle(ctx->V,(u64)ctx->ky[V_size+i]);

  /* feed-in the IV bits into the NLFSR. */
  for (i = 0; i < ctx->ivsize; ++i) 
    V_cycle(ctx->V,(u64)ctx->iv[i]);
  
  /* warming up. */
  for (i = 0; i < cfgini; ++i) 
    V_cycle(ctx->V,(u64)ZERO);

  /* copy back configuration bits to array iv[] for faster lookup. */
  for (i = 0; i < 64; ++i) 
    ctx->iv[i] = 0xFFU * (ONE&((ctx->V) >> i));

#endif
}


/* ------------------------------------------------------------------------- *
 *
 * Key and message independent initialization. 
 *
 * This function will be called once when the program starts.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_init()
{
  /* nothing to do */ 
}


/* ------------------------------------------------------------------------- *
 *
 * Key setup. 
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_keysetup(
  ECRYPT_ctx *ctx, 
  const u8   *key, 
  u32        keysize,              /* Key size in bits. */ 
  u32        ivsize                /* IV size in bits. */ 
  ) 
{             
  u32 i;

  /* always assert valid arguments: fixed key length = 80
     and IV size = 0, 8, 16, 24, 32, 40, 48, 56, or 64 */
  assert( ctx && 
          key &&
          (keysize == ECRYPT_MAXKEYSIZE) && 
          ((ivsize%8) == 0) && 
          (ivsize <= ECRYPT_MAXIVSIZE) );

  /* save key bits 0 to 63 in one context variable - for parallel loading */
  ctx->key0 = ((u64)key[7]<<56) 
            | ((u64)key[6]<<48) 
            | ((u64)key[5]<<40)  
            | ((u64)key[4]<<32) 
            | ((u64)key[3]<<24) 
            | ((u64)key[2]<<16) 
            | ((u64)key[1]<<8)  
            | ((u64)key[0]);

  /* save key all bits in context - 1 bit/word for faster lookup */
  for (i = 0; i < ECRYPT_MAXKEYSIZE; ++i)
    ctx->ky[i] = ONE & ((u32)key[i/8]>>(i%8));

  /* save size of IV in context */
  ctx->ivsize = ivsize;

  /* If the size of the IV is zero the key setup will be completed in this function,
     otherwise, a flag is set that the IV setup is not yet completed */
  if (ivsize)
    ctx->iv_setup_done = false;
  else {
    ACHTERBAHN_setup(ctx);
    ctx->iv_setup_done = true;
  }
}


/* ------------------------------------------------------------------------- *
 *
 * IV setup. 
 *
 * After having called ECRYPT_keysetup(), the user is allowed to
 * call ECRYPT_ivsetup() different times in order to encrypt/decrypt 
 * different messages with the same key but different IV's.
 * 
 * If the size of the IV is zero the keystream can be generated right after
 * the key setup. i.e. it is allowed to call 
 *
 *   ECRYPT_keysetup(ctx,key,ECRYPT_MAXKEYSIZE,0);
 *   ECRYPT_encrypt_bytes(ctx,plaintext,ciphertext,msglen);
 *
 * If there is an IV then ECRYPT_ivsetup() must be called before 
 * keystream generation, e.g.
 *
 *   ECRYPT_keysetup(ctx,key,ECRYPT_MAXKEYSIZE,48);
 *   ECRYPT_ivsetup(ctx,iv);
 *   ECRYPT_encrypt_bytes(ctx,plaintext,ciphertext,msglen);
 *
 *
 * Achterbahn allows for the IV lengths any value of 0, 8, 16, 24, 32, 40, 
 * 48, 56, 64. (An IV length equal to 0, of course, means that no 
 * synchronization is performed.) 
 * It is necessary to define the mapping of the bits of the IV to the
 * bit positions in the byte array of argument iv of the function.
 * The following order for storing the bits of the initialization vector 
 * IV[0...n], n = 0...63, in the argument array of bytes iv[0...k], 
 * k = 0 (mod 8), is defined: 
 *
 *     IV[63], ...    , IV[16] ,IV[15],  ..., IV[8], IV[7], ..., IV[0]
 *     |                       |                   |                 |
 *     | byte iv[7], ...       |    byte iv[1]     |    byte iv[0]   |
 *
 * The least significant bit of byte iv[0] is bit IV[0], the most significant
 * bit of byte iv[0] is bit IV[7], and so on.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_ivsetup(
  ECRYPT_ctx *ctx, 
  const u8   *iv
  )
{
  u32 i;

  /* always assert valid arguments */
  assert( ctx && iv );

  /* save IV in context - 1 bit/word for faster lookup */
  for (i = 0; i < ctx->ivsize; ++i)
    ctx->iv[i] = ONE & ((u32)iv[i/8]>>(i%8));
  
  /* call setup function and set the initialization flag*/
  ACHTERBAHN_setup(ctx);
  ctx->iv_setup_done = true;
}


/* ------------------------------------------------------------------------- *
 *
 * Encryption of arbitrary length messages.
 *
 * The ECRYPT_encrypt_bytes() function encrypts byte strings 
 * of arbitrary length.
 *
 * For a description of the bit order of the generated keystream read 
 * the comments to function ECRYPT_keystream_bytes() below.
 * 
 * ------------------------------------------------------------------------- */

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx *ctx, 
  const u8   *plaintext, 
  u8         *ciphertext, 
  u32        msglen                 /* Message length in bytes. */ 
  ) 
{
  u32 i;

  /* always assert valid arguments 
     and ensure that setup is done before encryption is started */
  assert( ctx && ctx->iv_setup_done && plaintext && ciphertext );

  /* return on zero message length */
  if ( !msglen ) return;

  /* loop over byte length of message */
  for (i = 0; i < msglen; ++i) {

    u32 k;
    
    /* Evaluate configuration */

#ifdef DONT_USE_CONFIGURATION
    
    /* evaluate Boolean combining function */
    ciphertext[i] = plaintext[i] 
                   ^ (u8)((ctx->A) ^ (ctx->C) ^ (ctx->D) ^ (ctx->E) 
			  ^ (((ctx->B)&(ctx->H)) | ((ctx->G)&(ctx->H)) | ((ctx->F)&(ctx->G))));

#else
 
#define filter(IVBIT, NLFSR, BITPOS)  ( (ctx->iv)[IVBIT] & ((ctx->NLFSR) >> (BITPOS)) )

    u32 a,b,c,d,e,f,g,h;

    a = (ctx->A)^filter( 0,A,1)^filter( 1,A,2)^filter(2,A,3)
                ^filter( 3,A,4)^filter( 4,A,5)^filter(5,A,6);

    b = (ctx->B)^filter( 6,B,1)^filter( 7,B,2)^filter( 8,B,3)^filter( 9,B,4)
                ^filter(10,B,5)^filter(11,B,6)^filter(12,B,7);

    c = (ctx->C)^filter(13,C,1)^filter(14,C,2)^filter(15,C,3)^filter(16,C,4)
                ^filter(17,C,5)^filter(18,C,6)^filter(19,C,7);

    d = (ctx->D)^filter(20,D,1)^filter(21,D,2)^filter(22,D,3)^filter(23,D,4)
                ^filter(24,D,5)^filter(25,D,6)^filter(26,D,7)^filter(27,D,8);

    e = (ctx->E)^filter(28,E,1)^filter(29,E,2)^filter(30,E,3)^filter(31,E,4)
                ^filter(32,E,5)^filter(33,E,6)^filter(34,E,7)^filter(35,E,8);

    f = (ctx->F)^filter(36,F,1)^filter(37,F,2)^filter(38,F,3)^filter(39,F,4)^filter(40,F,5)
                ^filter(41,F,6)^filter(42,F,7)^filter(43,F,8)^filter(44,F,9);

    g = (ctx->G)^filter(45,G,1)^filter(46,G,2)^filter(47,G,3)^filter(48,G,4)^filter(49,G,5)
                ^filter(50,G,6)^filter(51,G,7)^filter(52,G,8)^filter(53,G,9);

    h = (ctx->H)^filter(54,H,1)^filter(55,H,2)^filter(56,H,3)^filter(57,H,4)^filter(58,H,5)
                ^filter(59,H,6)^filter(60,H,7)^filter(61,H,8)^filter(62,H,9)^filter(63,H,10);

#undef filter

    /* evaluate Boolean combining function and add key stream byte to plaintext */
    ciphertext[i] = plaintext[i] ^ (u8)(a ^ c ^ d ^ e ^ ((b&h) | (g&h) | (f&g)));

#endif

    /* cycle eight times to prepare a new byte */
    for (k = 0; k < 8; ++k) {
      A_cycle(ctx->A,ZERO);
      B_cycle(ctx->B,ZERO);
      C_cycle(ctx->C,ZERO);
      D_cycle(ctx->D,ZERO);
      E_cycle(ctx->E,ZERO);
      F_cycle(ctx->F,ZERO);
      G_cycle(ctx->G,ZERO);
      H_cycle(ctx->H,ZERO);
    }

  } /* for (i) */
}


/* ------------------------------------------------------------------------- *
 *
 * Decryption of arbitrary length messages.
 *
 * The ECRYPT_decrypt_bytes() function decrypts byte strings 
 * of arbitrary length.
 * 
 * ------------------------------------------------------------------------- */

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx *ctx, 
  const u8   *ciphertext, 
  u8         *plaintext, 
  u32        msglen                 /* Message length in bytes. */ 
)
{
  ECRYPT_encrypt_bytes(ctx,ciphertext,plaintext,msglen);
}


/* ------------------------------------------------------------------------- * 
 *
 * Generates keystream without having to provide it with a zero plaintext. 
 *
 * The following order for storing the bits of the keystream 
 * z[0], z[1], ..., z[n] in the output byte array keystream[0...k],
 * is defined: 
 *
 *     z[n], ...          , z[16] , z[15],  ...,  z[8], z[7], ...,   z[0]
 *     |                          |                   |                 |
 *     | keystream iv[k], ...     |   keystream[1]    |  keystream[0]   |
 *
 * The least significant bit of byte keystream[0] is bit z[0], 
 * the most significant bit of byte keystream[0] is bit z[7], and so on.
 * 
 * ------------------------------------------------------------------------- */

void ECRYPT_keystream_bytes(
  ECRYPT_ctx  *ctx,
  u8          *keystream,
  u32         length           /* Length of keystream in bytes. */
  )
{
  u32 i;

  /* assert valid arguments */
  assert( ctx && keystream );

  /* return on zero message length */
  if ( !length ) return;

  /* loop over byte length of key stream */
  for (i = 0; i < length; ++i) {

    u32 k;

#ifdef DONT_USE_CONFIGURATION
    
    /* evaluate Boolean combining function */
    keystream[i] = (u8)((ctx->A) ^ (ctx->C) ^ (ctx->D) ^ (ctx->E) 
                     ^ (((ctx->B)&(ctx->H)) | ((ctx->G)&(ctx->H)) | ((ctx->F)&(ctx->G))));

#else
 

#define filter(IVBIT, NLFSR, BITPOS) ((ctx->iv)[IVBIT]&((ctx->NLFSR)>>(BITPOS)))
    u32 a,b,c,d,e,f,g,h;

    /* evaluate configuration */
    a = (ctx->A)^filter( 0,A,1)^filter( 1,A,2)^filter(2,A,3)
                ^filter( 3,A,4)^filter( 4,A,5)^filter(5,A,6);

    b = (ctx->B)^filter( 6,B,1)^filter( 7,B,2)^filter( 8,B,3)^filter( 9,B,4)
                ^filter(10,B,5)^filter(11,B,6)^filter(12,B,7);

    c = (ctx->C)^filter(13,C,1)^filter(14,C,2)^filter(15,C,3)^filter(16,C,4)
                ^filter(17,C,5)^filter(18,C,6)^filter(19,C,7);

    d = (ctx->D)^filter(20,D,1)^filter(21,D,2)^filter(22,D,3)^filter(23,D,4)
                ^filter(24,D,5)^filter(25,D,6)^filter(26,D,7)^filter(27,D,8);

    e = (ctx->E)^filter(28,E,1)^filter(29,E,2)^filter(30,E,3)^filter(31,E,4)
                ^filter(32,E,5)^filter(33,E,6)^filter(34,E,7)^filter(35,E,8);

    f = (ctx->F)^filter(36,F,1)^filter(37,F,2)^filter(38,F,3)^filter(39,F,4)^filter(40,F,5)
                ^filter(41,F,6)^filter(42,F,7)^filter(43,F,8)^filter(44,F,9);

    g = (ctx->G)^filter(45,G,1)^filter(46,G,2)^filter(47,G,3)^filter(48,G,4)^filter(49,G,5)
                ^filter(50,G,6)^filter(51,G,7)^filter(52,G,8)^filter(53,G,9);

    h = (ctx->H)^filter(54,H,1)^filter(55,H,2)^filter(56,H,3)^filter(57,H,4)^filter(58,H,5)
                ^filter(59,H,6)^filter(60,H,7)^filter(61,H,8)^filter(62,H,9)^filter(63,H,10);

    /* evaluate Boolean combining function */
    keystream[i] = (u8)(a ^ c ^ d ^ e ^ ((b&h) | (g&h) | (f&g)));

#undef filter
 
#endif

    /* cycle eight times to prepare a new byte */
    for (k = 0; k < 8; ++k) {
      A_cycle(ctx->A,ZERO);
      B_cycle(ctx->B,ZERO);
      C_cycle(ctx->C,ZERO);
      D_cycle(ctx->D,ZERO);
      E_cycle(ctx->E,ZERO);
      F_cycle(ctx->F,ZERO);
      G_cycle(ctx->G,ZERO);
      H_cycle(ctx->H,ZERO);
    }
    
  } /* for (i) */
}

/* ------------------------------------------------------------------------- *
 * end of implementation
 * ------------------------------------------------------------------------- */



#ifndef DONT_COMPILE_TEST_CODE

/* ------------------------------------------------------------------------- *
 *
 *   A C H T E R B A H N  Test Suite
 *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * Printing utility functions.
 * ------------------------------------------------------------------------- */

static void print_bitword (const char msg[], u32 x)
{
  int k;
  printf("%s",msg);
  for (k = 0; k < 32; ++k) 
    printf("%d",(x>>(31-k))&1);
  printf("\n");
  fflush(0);
}

static void print_hexword (const char msg[], u32 x)
{
  int k;
  printf("%s",msg);
  for (k = 24; k >= 0; k -= 8) 
    printf("%02X",(u8)(x>>k));
  printf("\n");
  fflush(0);
}

static void print_hexdword (const char msg[], u64 x)
{
  int k;
  printf("%s",msg);
  for (k = 56; k >= 0; k -= 8) 
    printf("%02X",(u8)(x>>k));
  printf("\n");
  fflush(0);
}

static void print_string (const char msg[], u32 len, const u8 *s)
{
  int k;
  printf("%s",msg);
  for (k = 0; k < len; ++k) 
    printf("%c",s[k]);
  printf("\n");
  fflush(0);
}

static void print_hexstr (const char msg[], u32 len, const u8 *s)
{
  int k;
  printf("%s",msg);
  for (k = 0; k < len; ++k) {
    printf("%02X",(u32)s[k]);
    if (k % 32 == 31) printf("\n");
  }
  printf("\n");
  fflush(0);
}


/* ------------------------------------------------------------------------- * 
 *
 * Print an introduction screen.
 * 
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_splashscreen()
{
  printf("\n* ================================================================ *\n");
  printf("\n                        A C H T E R B A H N\n");
  printf("\n         A Hardware Oriented Synchroneous Stream Cipher\n");
  printf("\n            %s\n",ECRYPT_AUTHORS);
  printf("                              Email:\n");
  printf("   %s\n",ECRYPT_EMAIL);
  printf("\n                    %s\n",ECRYPT_AFFILIATION); 
  printf("           %s\n",ECRYPT_ADDRESS); 
  printf("\n                    Reference Implementation\n");
  printf("                           Version %s\n",ECRYPT_VERSION);
  printf("\n* ================================================================ *\n");
}


/* ------------------------------------------------------------------------- * 
 *
 * Functional verification of the implementation.
 * 
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- * 
 * Period tests of all NLFSR components
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_NLFSRs()
{
  printf("* ================================================================ *\n");
  printf("* Period tests of all NLFSR components (be patient)\n");
  printf("* ================================================================ *\n");
  fflush(0);

  u32 i, state, seed = 0xc1a0be1a;

  printf("  A..."); fflush(0);
  state = seed & A_mask;
  for(i = 0; i < A_period; ++i) A_cycle(state,ZERO);
  assert(state == (seed&A_mask));
  printf("passed **\n");
  
  printf("  B..."); fflush(0);
  state = seed & B_mask;
  for(i = 0; i < B_period; ++i) B_cycle(state,ZERO);
  assert(state == (seed&B_mask));
  printf("passed **\n");
  
  printf("  C..."); fflush(0);
  state = seed & C_mask;
  for(i = 0; i < C_period; ++i) C_cycle(state,ZERO);
  assert(state == (seed&C_mask));
  printf("passed **\n");
  
  printf("  D..."); fflush(0);
  state = seed & D_mask;
  for(i = 0; i < D_period; ++i) D_cycle(state,ZERO);
  assert(state == (seed&D_mask));
  printf("passed **\n");

  printf("  E..."); fflush(0);
  state = seed & E_mask;
  for(i = 0; i < E_period; ++i) E_cycle(state,ZERO);
  assert(state == (seed&E_mask));
  printf("passed **\n");
  
  printf("  F..."); fflush(0);
  state = seed & F_mask;
  for(i = 0; i < F_period; ++i) F_cycle(state,ZERO);
  assert(state == (seed&F_mask));
  printf("passed **\n");
  
  printf("  G..."); fflush(0);
  state = seed & G_mask;
  for(i = 0; i < G_period; ++i) G_cycle(state,ZERO);
  assert(state == (seed&G_mask));
  printf("passed **\n");
  
  printf("  H..."); fflush(0);
  state = seed & H_mask;
  for(i = 0; i < H_period; ++i) H_cycle(state,ZERO);
  assert(state == (seed&H_mask));
  printf("passed **\n\n");
}

/* ------------------------------------------------------------------------- * 
 * Verify consistency of ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()
 * ------------------------------------------------------------------------- */

static void  ACHTERBAHN_verify_encrypt()
{
  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  typedef u8 key_t[10];     /* type for 80 bit key */
  typedef u8  iv_t[8];      /* type for max 64 bit IV */

  enum  { N = 512 };        /* buffer size */
  u8    plaintext[N],
        ciphertext[N],
        keystream[N];

  ECRYPT_ctx ctx;
  u32   length, i, ivlen, k;

  key_t key = "DonaldDuck";
  iv_t  iv  = "Monalisa";

  /* create a message */
  for (k = 0; k < N; ++k) plaintext[k] = (u8)k;
  
  /* loop over all allowed sizes of the IV */
  for (i = 0; ECRYPT_IVSIZE(i) <= ECRYPT_MAXIVSIZE; ++i) {
    ivlen = ECRYPT_IVSIZE(i);
    
    /* setup key */
    ECRYPT_keysetup(&ctx,key,80,ivlen);
    printf("  ivlen=%d\n",ivlen);
    fflush(0);
   
    /* loop over various message lengths */
    for (length = 0; length <= N; ++length) {
      
      /* generate key stream for given IV */
      ECRYPT_ivsetup(&ctx,iv);
      ECRYPT_keystream_bytes(&ctx,keystream,length);
      
      /* encrypt directly for given IV */
      ECRYPT_ivsetup(&ctx,iv);
      ECRYPT_encrypt_bytes(&ctx,plaintext,ciphertext,length);
           
      /* verify result */
      for (k = 0; k < length; ++k)
        assert( ciphertext[k] == (keystream[k] ^ plaintext[k]) );
    }    
  }
  
  printf("\n** passed ** \n\n");
}

/* ------------------------------------------------------------------------- * 
 * Some encryption/decryption tests
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_encr_decr()
{
  printf("* ================================================================ *\n");
  printf("* Encryption/decryption test\n");
  printf("* ================================================================ *\n\n");
  fflush(0);

  typedef u8 key_t[10];     /* type for 80 bit key */
  typedef u8  iv_t[8];      /* type for max 64 bit IV */

  enum  { N  = 1024 };      /* buffer size */
  u8    plaintext[N],
        ciphertext[N];

  ECRYPT_ctx ctx;
  u32   i, msglen, ivlen;

  key_t key = "DonaldDuck";
  iv_t  iv  = "Monalisa";

  /* 166 byte message */
  const u8 *original = "The wireless telegraph is not difficult to understand.\n"
                       "The ordinary telegraph is like a very long cat.\n"
                       "You pull the tail in New York, and it meows in Los Angeles.\n"
                       "The wireless is the same, only without the cat. (A. Einstein)";

  msglen = strlen(original);

  /* loop over all possible IV sizes */
  for (i = 0; ECRYPT_IVSIZE(i) <= ECRYPT_MAXIVSIZE; ++i) {
    ivlen = ECRYPT_IVSIZE(i);

    ECRYPT_keysetup(&ctx,key,ECRYPT_MAXKEYSIZE,ivlen);
    ECRYPT_ivsetup(&ctx,iv);
    ECRYPT_encrypt_bytes(&ctx,original,ciphertext,msglen);

    ECRYPT_keysetup(&ctx,key,ECRYPT_MAXKEYSIZE,ivlen);
    ECRYPT_ivsetup(&ctx,iv);
    ECRYPT_decrypt_bytes(&ctx,ciphertext,plaintext,msglen);

    if (ivlen == ECRYPT_MAXIVSIZE) {
      print_string("key: ",10,key);
      print_string("IV:  ",8,iv);
      print_string("------------------------ original  -------------------------\n",msglen,original);
      print_hexstr("------------------------ encrypted -------------------------\n",msglen,ciphertext);
      print_string("------------------------ decrypted -------------------------\n",msglen,plaintext);
    }

    assert( !strncmp(original,plaintext,msglen) );
  }

  printf("\n** passed **\n\n");
}

/* ------------------------------------------------------------------------- * 
 * Verify ECRYPT_keystream_bytes()
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_keystream()
{
  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  u32 k;
  typedef u8 key_t[10];     /* type for 80 bit key */
  typedef u8  iv_t[8];      /* type for max 64 bit IV */

  enum  { N  = 512 };       /* buffer size */
  u8 keystream[N];
  
#ifdef DONT_USE_CONFIGURATION
  u8 reference[N] = {
    0x7A,0x1B,0x56,0x2D,0x30,0x06,0xB7,0x4E,0x0A,0x34,0xCF,0xA2,0x2E,0x00,0x31,0x64,
    0xA4,0x0F,0x9C,0x31,0xAF,0xF7,0x68,0xC7,0x31,0xC0,0xD1,0xD6,0x59,0x12,0x2E,0x12,
    0x92,0x2D,0x48,0x47,0x6E,0x92,0x7A,0x34,0x04,0xCC,0x4E,0xB8,0x0E,0x0F,0xDC,0x22,
    0xBE,0xA1,0xF7,0x5C,0x94,0x0A,0xFD,0x36,0x82,0x08,0x8C,0x91,0xF5,0x52,0xBB,0x90,
    0xB4,0x69,0x0B,0xD1,0x36,0x92,0xAD,0x71,0x17,0xBC,0x5A,0x30,0xEA,0x12,0x08,0x80,
    0xA0,0x95,0x77,0xDA,0x55,0x0B,0xA7,0x9B,0x53,0x2B,0x1D,0x98,0x51,0x5D,0x37,0x4C,
    0x17,0x31,0x6F,0x47,0x7F,0x9C,0x2F,0x9C,0xB9,0x24,0x55,0x59,0x77,0xB9,0xD4,0x3B,
    0xD5,0xB1,0x13,0xB2,0x99,0x0B,0xB2,0x27,0x6A,0xBD,0x72,0x3C,0x88,0xF7,0x28,0xC1,
    0xE5,0xB0,0x6A,0xC5,0x50,0xA7,0x42,0x44,0xAA,0x14,0xCE,0x8D,0x16,0x7E,0x85,0x43,
    0x03,0x1F,0xDF,0xD3,0x44,0x32,0x4E,0xF8,0x07,0xAA,0x35,0x9C,0x95,0x7C,0x12,0xED,
    0x05,0x98,0xD7,0x20,0xF1,0x0E,0x0A,0x02,0xC7,0x71,0xF0,0xBD,0x33,0x06,0xD5,0x8D,
    0x69,0x29,0x50,0x72,0xF4,0x6A,0x35,0x96,0xF9,0x98,0x51,0xE8,0x62,0x71,0x97,0xE0,
    0xA4,0x23,0x46,0x20,0x4A,0xA9,0x8E,0xEF,0x64,0xB0,0x38,0x9A,0xBC,0x7A,0xA2,0x23,
    0x3A,0x54,0xA5,0x86,0x8C,0x12,0x65,0x1E,0xE3,0x29,0x0F,0xF9,0x52,0x86,0x2E,0x12,
    0x9F,0x4C,0xD3,0x96,0x76,0xCC,0x53,0x6E,0xA3,0x4D,0x96,0xAD,0x6A,0x95,0x27,0x5B,
    0x4A,0xC0,0x2A,0xFA,0x12,0xF0,0x12,0xCA,0xF6,0x49,0xCD,0xD1,0xEA,0x5F,0xB8,0x32,
    0x18,0xCD,0x0E,0xF8,0xF6,0xFB,0x6B,0xB7,0x44,0xF6,0xD4,0xA3,0x5E,0x5B,0xB0,0x89,
    0xCB,0x93,0x4A,0x4F,0xBC,0x97,0xB7,0x7C,0xB2,0xF3,0x84,0xCE,0xD0,0x02,0x77,0x9B,
    0x1E,0xFC,0x32,0xB6,0xAD,0xA4,0x3C,0x9A,0x92,0x80,0x24,0x91,0xAE,0x5C,0x72,0x41,
    0x8E,0x13,0x2D,0xEF,0x32,0x3B,0x5D,0x40,0xBC,0x3E,0x28,0xA2,0x21,0x3F,0x54,0x56,
    0x4E,0xC4,0xF6,0xC2,0xCD,0x94,0x35,0xB6,0xCD,0x36,0xA3,0x24,0xCF,0xD5,0xD1,0x5A,
    0x6F,0xA0,0x32,0xB7,0xAB,0x21,0xB0,0x45,0xB6,0x0A,0x6D,0x9D,0x55,0x4B,0x0E,0xD0,
    0xD3,0x4B,0xB9,0xE7,0x8E,0x3D,0x6D,0xA3,0x8D,0x30,0x76,0xA8,0xA7,0x55,0x42,0xD6,
    0x21,0xB7,0x93,0x8F,0x09,0xB2,0xB9,0x78,0x4A,0x50,0x9F,0x2F,0xB5,0x55,0x45,0xEF,
    0xA2,0x41,0x2A,0xE7,0x61,0x05,0x4B,0x00,0x58,0x2F,0xCE,0x67,0x18,0x94,0x29,0x09,
    0x81,0xA1,0x41,0xC5,0xFD,0x12,0x19,0x41,0xBF,0x98,0x4E,0x2D,0xF2,0x37,0xD7,0x27,
    0x2F,0xDD,0x7A,0xC5,0xD7,0x8A,0x04,0xD4,0xFF,0x57,0x88,0xA5,0x0B,0x45,0x48,0x81,
    0x1F,0x85,0x20,0xA6,0xB5,0x51,0x6E,0x87,0x21,0x04,0x60,0x6F,0x27,0x76,0x31,0x3F,
    0x77,0x6E,0xEA,0x37,0x58,0xCB,0x40,0x35,0x56,0x15,0x94,0xE5,0x36,0x76,0x29,0xAB,
    0x3D,0xBD,0xD2,0x8C,0x6C,0xA8,0xED,0xC2,0x7B,0x7B,0xC8,0x90,0x3F,0x8E,0xD4,0x3B,
    0x62,0xFC,0xD7,0x48,0x52,0x38,0xD8,0xD1,0x82,0x5E,0x92,0x1E,0x1F,0x21,0x4F,0xFF,
    0x34,0x8E,0x38,0xB6,0x51,0x4E,0x9A,0xFF,0xED,0x95,0xD3,0x64,0x28,0x5A,0x3D,0x80
  };
#else
  u8 reference[N] = {
    0x71,0x57,0x03,0x54,0x3B,0x07,0x03,0x79,0x2A,0x51,0xC4,0xB8,0x61,0x1E,0x59,0x78,
    0xE1,0x98,0x16,0x11,0xE8,0x2F,0xE8,0x20,0xB2,0xEC,0xB0,0x84,0x6E,0xF4,0xC3,0xF8,
    0xDF,0xCC,0x09,0x49,0xD6,0xB6,0x67,0xE9,0x8F,0xB0,0x68,0x4E,0xB8,0x0B,0x76,0x78,
    0x87,0x06,0xDC,0xF3,0x9C,0xCF,0x79,0x91,0x5D,0x58,0xAB,0xFC,0x86,0x32,0x02,0xE5,
    0x6B,0xB6,0x62,0x39,0x01,0xDB,0x33,0x20,0xD8,0xEC,0xC0,0xC1,0x44,0x6D,0x65,0x4A,
    0x41,0x28,0x55,0x1A,0xF0,0xAE,0x90,0xCD,0x34,0xA8,0x25,0x54,0x30,0xFD,0x85,0x65,
    0x9B,0x7A,0x04,0xB7,0x9A,0xF1,0xFE,0xC5,0x67,0x92,0x93,0x1D,0xA2,0x45,0xA3,0x26,
    0xF8,0x22,0xF2,0x7A,0x7A,0x1A,0x3E,0x49,0x63,0x1C,0x13,0x39,0x73,0x96,0x11,0xDD,
    0x6E,0xD0,0x77,0x83,0xBB,0x7B,0x37,0x36,0x22,0x13,0x6D,0x8D,0x0E,0xE4,0x41,0x33,
    0x2A,0xBC,0x89,0xFB,0xDA,0x77,0x9B,0xF9,0x3E,0x14,0xD3,0x14,0x05,0x0B,0x76,0x2F,
    0xFA,0xA8,0xC3,0x2C,0xD4,0x12,0x3F,0x65,0x60,0xC6,0x69,0x5A,0xBC,0x21,0x5B,0xAF,
    0xB7,0x10,0x9F,0x26,0x50,0xDA,0xC2,0xAB,0x47,0xF1,0x78,0x8D,0x38,0xCD,0xB9,0x8A,
    0x64,0x8E,0xC4,0x2D,0x09,0xAA,0x2E,0x7D,0x1D,0x98,0xF7,0xAA,0x95,0x24,0x37,0x5A,
    0x9F,0x6B,0x6B,0x7C,0xF2,0xDE,0xE5,0xF1,0x81,0xA6,0xA5,0x47,0xC9,0xF8,0xB2,0x52,
    0xA9,0xE8,0x9A,0xEB,0x39,0x00,0x96,0x2A,0xE8,0xD1,0xE4,0x6B,0x40,0x90,0x73,0x4D,
    0x2E,0x39,0xF4,0x58,0x9D,0x31,0xF1,0x94,0xBB,0xB4,0xF0,0x9E,0x4A,0xDE,0xA5,0xE6,
    0x01,0x1C,0x67,0xD7,0x5D,0xB6,0xA9,0xCE,0xCB,0xCF,0x33,0x8A,0xBF,0x8F,0x80,0x9D,
    0xD8,0x22,0x13,0x6A,0xD2,0x05,0x1A,0x39,0x9D,0x40,0x9B,0xF9,0xE2,0x44,0x3E,0x62,
    0x80,0xEC,0x80,0xC2,0xE9,0xB3,0x50,0x15,0x56,0xB4,0x12,0xF9,0x1D,0x75,0xF2,0x36,
    0x24,0x9F,0x92,0x7D,0x04,0x98,0x95,0x9A,0x63,0xBC,0x4D,0x4B,0xDB,0x95,0x2C,0x93,
    0xE1,0x25,0x65,0x76,0xC3,0x82,0xB6,0xD9,0x52,0xFD,0x3A,0x13,0xB5,0x76,0x16,0x79,
    0x5A,0x34,0x53,0xD6,0xA4,0x54,0x2C,0xAC,0xD2,0x75,0x74,0x1F,0x1B,0x31,0x2C,0x70,
    0x5B,0x54,0xCC,0x3B,0x57,0x94,0x8D,0xE2,0xB5,0xD7,0x58,0xDC,0x09,0xEF,0x0B,0x91,
    0xEA,0x63,0x43,0x8C,0x8E,0xC8,0x32,0xBA,0x33,0x85,0xBC,0xAC,0x4E,0x5E,0xE5,0xBD,
    0x71,0x75,0x42,0x01,0x6E,0xED,0x85,0x45,0xA4,0x78,0x6D,0x8C,0xF1,0xD9,0x65,0x10,
    0x66,0xB9,0x45,0x1D,0x85,0xF2,0x28,0x8C,0xA6,0xB0,0x06,0xF2,0x36,0xF6,0x83,0xF8,
    0xB8,0xD9,0x8E,0x10,0xA6,0x31,0x89,0x5B,0x36,0x6D,0xB9,0xA0,0xFB,0x24,0x22,0x97,
    0x9E,0x09,0x03,0xD1,0xBF,0x5E,0x48,0xC3,0xB8,0x68,0xE8,0xF1,0x44,0x90,0x6E,0x11,
    0x0D,0x36,0xD3,0xE7,0xB0,0xE0,0x99,0xA7,0x56,0xFD,0xCD,0x06,0x2B,0xD3,0xBE,0xCD,
    0x20,0x69,0xC4,0xE7,0xDE,0x29,0x70,0xB2,0x0A,0xB7,0x47,0xD0,0x4F,0xB9,0x9C,0xF4,
    0xC0,0x1E,0x2B,0x5D,0x13,0xBE,0x22,0x26,0x0D,0xA3,0x43,0xE3,0x87,0xA6,0xDF,0x23,
    0xEE,0x7F,0x24,0xA7,0x82,0x5C,0xC5,0x56,0x00,0x4C,0x9D,0x5D,0x52,0xBD,0xFE,0xAD
  };
#endif
 
  ECRYPT_ctx ctx;
  key_t key = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55 };
  iv_t  iv  = { 0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA };

  print_hexstr("key = ",10,key);
  print_hexstr("iv  = ",8,iv);

  ECRYPT_keysetup(&ctx,key,80,64);
  ECRYPT_ivsetup(&ctx,iv);
  ECRYPT_keystream_bytes(&ctx,keystream,N);

  print_hexstr ("keystream =\n", N, keystream);

  for (k = 0; k < N; ++k) 
    assert(keystream[k] == reference[k]);

  printf("** passed **\n\n");
}


/* ------------------------------------------------------------------------- * 
 * 
 * Main program.
 * Perform functional verification tests.
 * 
 * ------------------------------------------------------------------------- */

int main (void)
{
  /* ----------------------------------------------------------------------- *
   * Introduction screen
   * ----------------------------------------------------------------------- */

  ACHTERBAHN_splashscreen();

  /* ----------------------------------------------------------------------- *
   * Verify basic requirements.
   * ----------------------------------------------------------------------- */
  
  /* check required types */
  assert(sizeof(u32) >= 4);
  assert(sizeof(u64) >= 8);
  
  /* check constants in header file */
  assert( !strcmp(ECRYPT_NAME,"ACHTERBAHN") );
  assert(ECRYPT_MAXIVSIZE == 64);
  assert(ECRYPT_MAXKEYSIZE == 80);
  
  /* ----------------------------------------------------------------------- *
   * Functional verification of the implementation.
   * ----------------------------------------------------------------------- */

#ifdef DONT_USE_CONFIGURATION
  printf("* Configuration feature is disabled\n");
#else
  printf("* Configuration feature is enabled\n");
#endif

  ACHTERBAHN_verify_keystream();
  ACHTERBAHN_verify_encrypt();  
  ACHTERBAHN_verify_encr_decr();
  ACHTERBAHN_verify_NLFSRs();

  printf("* ================================================================ *\n");
  printf("* all tests passed\n");
  printf("* ================================================================ *\n");
  printf("*\n");
  printf("* You should run the test with configuration feature\n");
  printf("* enabled and disabled. See \"README\" or call \"make help\"\n");
  printf("*\n");

  return 0;
}

/* ------------------------------------------------------------------------- */

#endif
