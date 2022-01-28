/* ------------------------------------------------------------------------- *
 *
 *   Program:   A C H T E R B A H N  - 128 / 80
 *              Version 1.2 of the C reference implementation (optimized)
 *
 *   Authors:   Berndt M. Gammel, Email: gammel@matpack.de
 *              Rainer Goettfert, Email: rainer.goettfert@gmx.de
 *              Oliver Kniffler,  Email: oliver.kniffler@arcor.de
 *
 *  Language:   ANSI C99
 *
 *   Sources:   achterbahn.c
 *              achterbahn.h
 *              ecrypt-sync.h
 *
 *  Includes:   ecrypt-portable.h,
 *              ecrypt-config.h,
 *              ecrypt-machine.h
 *
 *  Makefile:   Makefile
 *
 * Platforms:   This program has been tested on the following platforms:
 *              gcc 3.4.4, Cygwin, Windows 2000
 *              gcc 4.1.0, S.u.S.E. Linux 10.1
 *
 * Copyright:   (C) 2005-2006 by Berndt M. Gammel, Rainer Goettfert, 
 *                               and Oliver Kniffler
 *
 * History:     1.0, 30.Jun.2006, original submission
 *              1.1, 10.Nov.2006, performance optimizations
 *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * This is the implementation file "achterbahn.c"
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * if COMPILE_TEST_CODE is defined the implementation test code (main)
 * in this file will be compiled.
 * Preferably this flag is defined by the Makefile, not in the code.
 * Call "make -f Makefile.achterbahn"
 * ------------------------------------------------------------------------- */

/* #define COMPILE_TEST_CODE */
#ifdef COMPILE_TEST_CODE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#endif

/* ------------------------------------------------------------------------- *
 * ECRYPT API include file
 * ------------------------------------------------------------------------- */

#include "ecrypt-sync.h"

/* ------------------------------------------------------------------------- *
 * implementation include file
 * ------------------------------------------------------------------------- */

#include "achterbahn.h"

/* ------------------------------------------------------------------------- *
 * ECRYPT API functions
 * ------------------------------------------------------------------------- */

void ECRYPT_init(void) 
{
}

/* ------------------------------------------------------------------------- *
 * Macro to define local variables for the state (longkey, A0, A1,..., A12)
 * and intermediary variables for the macros implementing the NLFSRs.
 * ------------------------------------------------------------------------- */

#define DEFINE_LOCAL_STATE()\
  u32 longkey,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11;\
  u64 A12;\
  u32 F, M;\
  u64 FL, ML;

/* ------------------------------------------------------------------------- *
 * Macro to load the state saved after key setup to the 
 * local variables longkey, A0, A1,..., A12.
 * ------------------------------------------------------------------------- */

#define LOAD_KEY_STATE()\
  longkey = ctx->longkey,\
  A0 = ctx->sA0,\
  A1 = ctx->sA1,\
  A2 = ctx->sA2,\
  A3 = ctx->sA3,\
  A4 = ctx->sA4,\
  A5 = ctx->sA5,\
  A6 = ctx->sA6,\
  A7 = ctx->sA7,\
  A8 = ctx->sA8,\
  A9 = ctx->sA9,\
  A10 = ctx->sA10,\
  A11 = ctx->sA11,\
  A12 = ctx->sA12

/* ------------------------------------------------------------------------- *
 * Macro to save the local state, given by the variables 
 * longkey, A0, A1,..., A12, as the state after key setup is completed
 * ------------------------------------------------------------------------- */

#define SAVE_KEY_STATE()\
  ctx->longkey = longkey,\
  ctx->sA0  = A0,\
  ctx->sA1  = A1,\
  ctx->sA2  = A2,\
  ctx->sA3  = A3,\
  ctx->sA4  = A4,\
  ctx->sA5  = A5,\
  ctx->sA6  = A6,\
  ctx->sA7  = A7,\
  ctx->sA8  = A8,\
  ctx->sA9  = A9,\
  ctx->sA10 = A10,\
  ctx->sA11 = A11,\
  ctx->sA12 = A12

/* ------------------------------------------------------------------------- *
 * Macro to load the current state to the 
 * local variables longkey, A0, A1,..., A12.
 * ------------------------------------------------------------------------- */

#define LOAD_CURRENT_STATE()\
  longkey = ctx->longkey,\
  A0 = ctx->A0,\
  A1 = ctx->A1,\
  A2 = ctx->A2,\
  A3 = ctx->A3,\
  A4 = ctx->A4,\
  A5 = ctx->A5,\
  A6 = ctx->A6,\
  A7 = ctx->A7,\
  A8 = ctx->A8,\
  A9 = ctx->A9,\
  A10 = ctx->A10,\
  A11 = ctx->A11,\
  A12 = ctx->A12
 
/* ------------------------------------------------------------------------- *
 * Macro to save the local state, given by the variables A0, A1,..., A12 
 * as the current state in the context (longkey is not saved).
 * ------------------------------------------------------------------------- */

#define SAVE_CURRENT_STATE()\
  ctx->A0  = A0;\
  ctx->A1  = A1,\
  ctx->A2  = A2,\
  ctx->A3  = A3,\
  ctx->A4  = A4,\
  ctx->A5  = A5,\
  ctx->A6  = A6,\
  ctx->A7  = A7,\
  ctx->A8  = A8,\
  ctx->A9  = A9,\
  ctx->A10 = A10,\
  ctx->A11 = A11,\
  ctx->A12 = A12

/* ------------------------------------------------------------------------- *
 * Key setup.
 * ------------------------------------------------------------------------- */

void ECRYPT_keysetup(
  ECRYPT_ctx *ctx,
  const u8   *key,
  u32        keysize,
  u32        ivsize)
{
  int i;
  u64 keylo;

  /* define local copy of state */
  DEFINE_LOCAL_STATE();

  /* keylength in bytes */
  keysize /= 8;
  longkey = (keysize > 10U);

  /* copy 5 lower key bytes (40 bit) */
  keylo = ((u64)key[4]<<32)
        | ((u64)key[3]<<24)
        | ((u64)key[2]<<16)
        | ((u64)key[1]<<8)
        | ((u64)key[0]);

  /* ----------------------------------------------------------------------- *
   * step 1: load all driving NLFSRs with the first key bits in parallel.
   * ----------------------------------------------------------------------- */
  
  A1  = (u32)(keylo & A1_mask);
  A2  = (u32)(keylo & A2_mask);
  A3  = (u32)(keylo & A3_mask);
  A4  = (u32)(keylo & A4_mask);
  A5  = (u32)(keylo & A5_mask);
  A6  = (u32)(keylo & A6_mask);
  A7  = (u32)(keylo & A7_mask);
  A8  = (u32)(keylo & A8_mask);
  A9  = (u32)(keylo & A9_mask);
  A10 = (u32)(keylo & A10_mask);
  A11 = (u32)(keylo & A11_mask);
  if (longkey) {
    A0  = (u32)(keylo & A0_mask);
    A12 = (u64)(keylo & A12_mask);
  } else { 
    A0  = ZERO;
    A12 = (u64)ZERO; 
  }

  /* ----------------------------------------------------------------------- *
   * step 2: for each driving NLFSR, feed-in the key bits, not already loaded 
   *         into the register in step 1, into the NLFSR. 
   * ----------------------------------------------------------------------- */

  A1_cycle2(A1, keylo>>A1_size);                             /* K22...K23 */
  for (i = 3; i < keysize; ++i)   A1_cycle8(A1, key[i]);     /* K24...Kmax */

  A2_cycle(A2, keylo>>A2_size);                              /* K23 */
  for (i = 3; i < keysize; ++i)   A2_cycle8(A2, key[i]);     /* K24...Kmax */

  for (i = 3; i < keysize; ++i)   A3_cycle8(A3, key[i]);     /* K24...Kmax */

  A4_cycle7(A4, keylo>>A4_size);                             /* K25...K31 */
  for (i = 4; i < keysize; ++i)   A4_cycle8(A4, key[i]);     /* K32...Kmax */

  A5_cycle6(A5, keylo>>A5_size);                             /* K26...K31 */
  for (i = 4; i < keysize; ++i)   A5_cycle8(A5, key[i]);     /* K32...Kmax */

  A6_cycle5(A6, keylo>>A6_size);                             /* K27...K31 */
  for (i = 4; i < keysize; ++i)   A6_cycle8(A6, key[i]);     /* K32...Kmax */

  A7_cycle4(A7, keylo>>A7_size);                             /* K28...K31 */
  for (i = 4; i < keysize; ++i)   A7_cycle8(A7, key[i]);     /* K32...Kmax */

  A8_cycle3(A8, keylo>>A8_size);                             /* K29...K31 */
  for (i = 4; i < keysize; ++i)   A8_cycle8(A8, key[i]);     /* K32...Kmax */

  A9_cycle2(A9, keylo>>A9_size);                             /* K30...K31 */
  for (i = 4; i < keysize; ++i)   A9_cycle8(A9, key[i]);     /* K32...Kmax */

  A10_cycle(A10, keylo>>A10_size);                           /* K31 */
  for (i = 4; i < keysize; ++i)   A10_cycle8(A10, key[i]);   /* K32...Kmax */

  for (i = 4; i < keysize; ++i)   A11_cycle8(A11, key[i]);   /* K32...Kmax */

  if (longkey) {
    A0_cycle3(A0, keylo>>A0_size);                           /* K21...K23 */
    for (i = 3; i < keysize; ++i)   A0_cycle8(A0, key[i]);   /* K24...Kmax */

    A12_cycle7(A12,keylo>>A12_size);                         /* K33...K39 */
    for (i = 5; i < keysize; ++i)   A12_cycle8(A12, key[i]); /* K40...Kmax */
  }

  /* save state after key setup in context */
  SAVE_KEY_STATE();

  /* save size of IV in context */
  ctx->ivsize8 = ivsize/8;
}

/* ------------------------------------------------------------------------- *
 * Boolean combining function 
 *
 * F(x0, x1, ..., x12) = 
 *   x0 + x1 + x2 + x3 + x4 + x5 + x7 + x9 + x11 + x12+ x0*x5 + x2*x10 + x2*x11 
 *   + x4*x8 + x4*x12 + x5*x6 + x6*x8 + x6*x10 + x6*x11 + x6*x12 + x7*x8 
 *   + x7*x12 + x8*x9 + x8*x10 + x9*x10 + x9*x11 + x9*x12 + x10*x12 + x0*x5*x8 
 *   + x0*x5*x10 + x0*x5*x11 + x0*x5*x12 + x1*x2*x8 + x1*x2*x12 + x1*x4*x10 
 *   + x1*x4*x11 + x1*x8*x9 + x1*x9*x10 + x1*x9*x11 + x1*x9*x12 + x2*x3*x8 
 *   + x2*x3*x12 + x2*x4*x8 + x2*x4*x10 + x2*x4*x11 + x2*x4*x12 + x2*x7*x8 
 *   + x2*x7*x12 + x2*x8*x10 + x2*x8*x11 + x2*x9*x10 + x2*x9*x11 + x2*x10*x12
 *   + x2*x11*x12 + x3*x4*x8 + x3*x4*x12 + x3*x8*x9 + x3*x9*x12 + x4*x7*x8 
 *   + x4*x7*x12 + x4*x8*x9 + x4*x9*x12 + x5*x6*x8 + x5*x6*x10 + x5*x6*x11 
 *   + x5*x6*x12 + x6*x8*x10 + x6*x8*x11 + x6*x10*x12 + x6*x11*x12 + x7*x8*x9
 *   + x7*x9*x12 + x8*x9*x10 + x8*x9*x11 + x9*x10*x12 + x9*x11*x12 
 *   + x0*x5*x8*x10 + x0*x5*x8*x11 + x0*x5*x10*x12 + x0*x5*x11*x12 
 *   + x1*x2*x3*x8 + x1*x2*x3*x12 + x1*x2*x7*x8 + x1*x2*x7*x12 
 *   + x1*x3*x5*x8 + x1*x3*x5*x12 + x1*x3*x8*x9 + x1*x3*x9*x12 
 *   + x1*x4*x8*x10 + x1*x4*x8*x11 + x1*x4*x10*x12 + x1*x4*x11*x12 
 *   + x1*x5*x7*x8 + x1*x5*x7*x12 + x1*x7*x8*x9 + x1*x7*x9*x12 
 *   + x1*x8*x9*x10 + x1*x8*x9*x11 + x1*x9*x10*x12 + x1*x9*x11*x12 
 *   + x2*x3*x4*x8 + x2*x3*x4*x12 + x2*x3*x5*x8 + x2*x3*x5*x12 
 *   + x2*x4*x7*x8 + x2*x4*x7*x12 + x2*x4*x8*x10 + x2*x4*x8*x11 
 *   + x2*x4*x10*x12 + x2*x4*x11*x12 + x2*x5*x7*x8 + x2*x5*x7*x12 
 *   + x2*x8*x9*x10 + x2*x8*x9*x11 + x2*x9*x10*x12 + x2*x9*x11*x12 
 *   + x3*x4*x8*x9 + x3*x4*x9*x12  + x4*x7*x8*x9 + x4*x7*x9*x12 
 *   + x5*x6*x8*x10 + x5*x6*x8*x11 + x5*x6*x10*x12 + x5*x6*x11*x12
 *
 *   u32 F(u32 x0, u32 x1, u32 x2, u32 x3, u32 x4, u32 x5, 
 *   	   u32 x6, u32 x7, u32 x8, u32 x9, u32 x10, u32 x11, u32 x12)
 *   {
 *     u32 A = x1^x2,
 *         C = x2^x9,
 *         H = x3^x7,
 *         T = x4^x9,
 *         E = ((x0^x6)&x5)^x6,
 *         R = ((x1^x4)&C)^T,
 *         b = (R^(A&x5)^x2)&H,
 *         a = ((x10^x11)&(C^(A&T)^E))^E,
 *         h = (x8^x12)&(b^a^R^x7^x10),
 *         n = H^A^T^a^h^x0^x5^x6^x11^x12;
 *     return (n);
 *   } 
 *
 * Note, that the (length-16)-th bit of each NLFSR is fed into the 
 * Boolean combining function.
 * ------------------------------------------------------------------------- */

#define KEYSTREAM_BITS(n)\
  u32 A = (A1>>(A1_size-16U))^(A2>>(A2_size-16U)),\
      C = (A2>>(A2_size-16U))^(A9>>(A9_size-16U)),\
      H = (A3>>(A3_size-16U))^(A7>>(A7_size-16U)),\
      T = (A4>>(A4_size-16U))^(A9>>(A9_size-16U)),\
      E = (((A0>>(A0_size-16U))^(A6>>(A6_size-16U)))&(A5>>(A5_size-16U)))^(A6>>(A6_size-16U)),\
      R = (((A1>>(A1_size-16U))^(A4>>(A4_size-16U)))&C)^T,\
      b = (R^(A&(A5>>(A5_size-16U)))^(A2>>(A2_size-16U)))&H,\
      a = (((A10>>(A10_size-16U))^(A11>>(A11_size-16U)))&(C^(A&T)^E))^E,\
      h = ((A8>>(A8_size-16U))^((u32)(A12 >> (A12_size-16U))))&\
           (b^a^R^(A7>>(A7_size-16U))^(A10>>(A10_size-16U)));\
      n = H^A^T^a^h^(A0>>(A0_size-16U))^(A5>>(A5_size-16U))^(A6>>(A6_size-16U))\
          ^(A11>>(A11_size-16U))^((u32)(A12 >> (A12_size-16U)));

/* ------------------------------------------------------------------------- *
 * Macro to step forward all driving NLFSRs by eight clock cycles
 * ------------------------------------------------------------------------- */

#define NLFSR_cycle8(feedin8) \
 { if (longkey) { \
     A0_cycle8(A0, feedin8); \
     A12_cycle8(A12,feedin8); \
   } \
   A1_cycle8(A1, feedin8); \
   A2_cycle8(A2, feedin8); \
   A3_cycle8(A3, feedin8); \
   A4_cycle8(A4, feedin8); \
   A5_cycle8(A5, feedin8); \
   A6_cycle8(A6, feedin8); \
   A7_cycle8(A7, feedin8); \
   A8_cycle8(A8, feedin8); \
   A9_cycle8(A9, feedin8); \
   A10_cycle8(A10,feedin8); \
   A11_cycle8(A11,feedin8); }

/* ------------------------------------------------------------------------- *
 * IV setup.
 *
 * Achterbahn allows for the IV lengths the values 0, 8, 16, ... upto 
 * the key length. (An IV length equal to 0, of course, means that no
 * synchronization is performed.)
 * It is necessary to define the mapping of the bits of the IV to the
 * bit positions in the byte array of argument iv of the function.
 * The following order for storing the bits of the initialization vector
 * IV[0...n], n = 0...keysize, in the argument array of bytes iv[0...k],
 * k = 0 (mod 8), is defined:
 *
 *     IV[keysize],.. , IV[16] ,IV[15], ... , IV[8], IV[7], ..., IV[0]
 *     |                       |                   |                 |
 *     | byte iv[k], ...       |    byte iv[1]     |    byte iv[0]   |
 *
 * The least significant bit of byte iv[0] is bit IV[0], the most significant
 * bit of byte iv[0] is bit IV[7], and so on.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_ivsetup(
  ECRYPT_ctx *ctx,
  const u8   *iv)
{
  u32 i,z;

  /* define local copy of the state */
  DEFINE_LOCAL_STATE();
  LOAD_KEY_STATE();
  
  /* ----------------------------------------------------------------------- *
   * step 3: for each driving NLFSR, feed-in all IV bits, into the NLFSR.
   * ----------------------------------------------------------------------- */

  for (i = 0; i < ctx->ivsize8; ++i)
    NLFSR_cycle8(iv[i]);

  /* ----------------------------------------------------------------------- *
   * step 4: Feed-in the keystream output into each NLFSR for 32 cycles
   * ----------------------------------------------------------------------- */

  for (i = 0; i < 4; ++i) {
    KEYSTREAM_BITS(z);
    NLFSR_cycle8(z);
  }
  
  /* ----------------------------------------------------------------------- *
   * step 5: set the least significant bit of each NLFSR to 1.
   * ----------------------------------------------------------------------- */

  A1  |= ONE;
  A2  |= ONE;
  A3  |= ONE;
  A4  |= ONE;
  A5  |= ONE;
  A6  |= ONE;
  A7  |= ONE;
  A8  |= ONE;
  A9  |= ONE;
  A10 |= ONE;
  A11 |= ONE;
  if (longkey) {
    A0  |= ONE;  
    A12 |= ONE;
  }

  /* ----------------------------------------------------------------------- *
   * step 6: warm-up 64 cycles.
   * ----------------------------------------------------------------------- */

  for (i = 0; i < 8; ++i) 
    NLFSR_cycle8(ZERO);

  /* save current state in context */
  SAVE_CURRENT_STATE();
}

/* ------------------------------------------------------------------------- *
 * Encryption of arbitrary length messages.
 * ------------------------------------------------------------------------- */

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx *ctx,
  const u8   *plaintext,
  u8         *ciphertext,
  u32        msglen)
{
  u32 i,z;

  DEFINE_LOCAL_STATE();
  LOAD_CURRENT_STATE();

  for (i = 0; i < msglen; ++i) {
    KEYSTREAM_BITS(z);
    ciphertext[i] = plaintext[i] ^ z;
    if (i < msglen-1) {
      ++i;
      ciphertext[i] = plaintext[i] ^ (z>>8);
      NLFSR_cycle8(ZERO); 
    }
    NLFSR_cycle8(ZERO); 
  }
 
  SAVE_CURRENT_STATE();
}

/* ------------------------------------------------------------------------- *
 * Decryption of arbitrary length messages.
 * ------------------------------------------------------------------------- */

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx *ctx,
  const u8   *ciphertext,
  u8         *plaintext,
  u32        msglen)
{
  u32 i,z;

  DEFINE_LOCAL_STATE();
  LOAD_CURRENT_STATE();

  for (i = 0; i < msglen; ++i) {
    KEYSTREAM_BITS(z);
    plaintext[i] = ciphertext[i] ^ z;
    if (i < msglen-1) {
      ++i;
      plaintext[i] = ciphertext[i] ^ (z>>8);
      NLFSR_cycle8(ZERO); 
    }
    NLFSR_cycle8(ZERO); 
  }

  SAVE_CURRENT_STATE();
}

/* ------------------------------------------------------------------------- *
 * Generates keystream without having to provide it with a zero plaintext.
 *
 * The following order for storing the bits of the keystream
 * z[0], z[1], ..., z[n] in the output byte array keystream[0...k],
 * is defined:
 *
 *     z[n], ...          , z[16] , z[15],  ...,  z[8], z[7], ...,   z[0]
 *     |                          |                   |                 |
 *     | keystream[k], ...        |   keystream[1]    |  keystream[0]   |
 *
 * The least significant bit of byte keystream[0] is bit z[0],
 * the most significant bit of byte keystream[0] is bit z[7], and so on.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_keystream_bytes(
  ECRYPT_ctx  *ctx,
  u8          *keystream,
  u32         length)
{
  u32 i,z;

  DEFINE_LOCAL_STATE();
  LOAD_CURRENT_STATE();

  for (i = 0; i < length; ++i) {
    KEYSTREAM_BITS(z);
    keystream[i] = z;
    if (i < length-1) {
      ++i;
      keystream[i] = (z>>8);
      NLFSR_cycle8(ZERO); 
    }
    NLFSR_cycle8(ZERO); 
  }

  SAVE_CURRENT_STATE();
}

/* ------------------------------------------------------------------------- *
 * end of implementation
 * ------------------------------------------------------------------------- */



#ifdef COMPILE_TEST_CODE

/* ------------------------------------------------------------------------- *
 *
 *   A C H T E R B A H N  - 128 / 80 Test Suite
 *
 * ------------------------------------------------------------------------- */

typedef u8 key128_t[16];     /* type for max. 128 bit key */
typedef u8  iv128_t[16];     /* type for max. 128 bit IV */

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

static void print_string (const char msg[], u32 len, const u8 *s)
{
  u32 k;
  printf("%s",msg);
  for (k = 0; k < len; ++k)
    printf("%c",s[k]);
  printf("\n");
  fflush(0);
}

static void print_hexstr (const char msg[], u32 len, const u8 *s)
{
  u32 k;
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
  printf("\n                    A C H T E R B A H N - 128 / 80\n");
  printf("\n     A Hardware Oriented Synchroneous Binary Stream Cipher\n");
  printf("\n            Reference implementation V1.2 (optimized) \n");
  printf("\n            %s\n",ECRYPT_AUTHORS);
  printf("\n                              Email:\n\n");
  printf("                        gammel@matpack.de\n");
  printf("                      rainer.goettfert@gmx.de\n");
  printf("                     oliver.kniffler@arcor.de\n");
  printf("\n                     Reference Implementation\n");
  printf("\n* ================================================================ *\n");
}

/* ------------------------------------------------------------------------- *
 * Period tests of all NLFSR components
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_NLFSRs()
{
  u32 seed; 
  u32 i, state, F, M;
  u64 l, lstate, FL, ML;

  printf("* ================================================================ *\n");
  printf("* Period tests of all driving NLFSRs (be patient)\n");
  printf("* ================================================================ *\n");
  fflush(0);

  seed = 0xc1a0be1a;

  printf("  A0(n=%u)...",A0_size); fflush(0);
  state = seed & A0_mask;
  for (i = 0; i < A0_mask; ++i) {
    if (i < A0_mask-7) {
      A0_cycle8(state,ZERO);
      i += 7;
    } else   
      A0_cycle(state,ZERO);
    if (state == (seed&A0_mask)) break;
  }  
  assert(i == A0_mask-1);
  printf("passed **\n");

  printf("  A1(n=%u)...",A1_size); fflush(0);
  state = seed & A1_mask;
  for (i = 0; i < A1_mask; ++i) {
    if (i < A1_mask-7) {
      A1_cycle8(state,ZERO);
      i += 7;
    } else   
      A1_cycle(state,ZERO);
    if (state == (seed&A1_mask)) break;
  }
  assert(i == A1_mask-1);
  printf("passed **\n");

  printf("  A2(n=%u)...",A2_size); fflush(0);
  state = seed & A2_mask;
  for (i = 0; i < A2_mask; ++i) {
    if (i < A2_mask-7) {
      A2_cycle8(state,ZERO);
      i += 7;
    } else
      A2_cycle(state,ZERO);
    if (state == (seed&A2_mask)) break;
  }
  assert(i == A2_mask-1);
  printf("passed **\n");

  printf("  A3(n=%u)...",A3_size); fflush(0);
  state = seed & A3_mask;
  for (i = 0; i < A3_mask; ++i) {
    if (i < A3_mask-7) {
      A3_cycle8(state,ZERO);
      i += 7;
    } else
      A3_cycle(state,ZERO);
    if (state == (seed&A3_mask)) break;
  }
  assert(i == A3_mask-1);
  printf("passed **\n");

  printf("  A4(n=%u)...",A4_size); fflush(0);
  state = seed & A4_mask;
  for (i = 0; i < A4_mask; ++i) {
    if (i < A4_mask-7) {
      A4_cycle8(state,ZERO);
      i += 7;
    } else
      A4_cycle(state,ZERO);
    if (state == (seed&A4_mask)) break;
  }
  assert(i == A4_mask-1);
  printf("passed **\n");

  printf("  A5(n=%u)...",A5_size); fflush(0);
  state = seed & A5_mask;
  for (i = 0; i < A5_mask; ++i) {
    if (i < A5_mask-7) {
      A5_cycle8(state,ZERO);
      i += 7;
    } else
      A5_cycle(state,ZERO);     
    if (state == (seed&A5_mask)) break;
  }
  assert(i == A5_mask-1);
  printf("passed **\n");

  printf("  A6(n=%u)...",A6_size); fflush(0);
  state = seed & A6_mask;
  for (i = 0; i < A6_mask; ++i) {
    if (i < A6_mask-7) {
      A6_cycle8(state,ZERO);
      i += 7;
    } else
      A6_cycle(state,ZERO);
    if (state == (seed&A6_mask)) break;
  }
  assert(i == A6_mask-1);
  printf("passed **\n");

  printf("  A7(n=%u)...",A7_size); fflush(0);
  state = seed & A7_mask;
  for (i = 0; i < A7_mask; ++i) {
    if (i < A7_mask-7) {
      A7_cycle8(state,ZERO);
      i += 7;
    } else
      A7_cycle(state,ZERO);
    if (state == (seed&A7_mask)) break;
  }
  assert(i == A7_mask-1);
  printf("passed **\n");

  printf("  A8(n=%u)...",A8_size); fflush(0);
  state = seed & A8_mask;
  for (i = 0; i < A8_mask; ++i) {
    if (i < A8_mask-7) {
      A8_cycle8(state,ZERO);
      i += 7;
    } else
      A8_cycle(state,ZERO);
    if (state == (seed&A8_mask)) break;
  }
  assert(i == A8_mask-1);
  printf("passed **\n");

  printf("  A9(n=%u)...",A9_size); fflush(0);
  state = seed & A9_mask;
  for (i = 0; i < A9_mask; ++i) {
    if (i < A9_mask-7) {
      A9_cycle8(state,ZERO);
      i += 7;
    } else
      A9_cycle(state,ZERO);
    if (state == (seed&A9_mask)) break;
  }
  assert(i == A9_mask-1);
  printf("passed **\n");

  printf("  A10(n=%u)...",A10_size); fflush(0);
  state = seed & A10_mask;
  for (i = 0; i < A10_mask; ++i) {
    if (i < A10_mask-7) {
      A10_cycle8(state,ZERO);
      i += 7;
    } else
      A10_cycle(state,ZERO);
    if (state == (seed&A10_mask)) break;
  }
  assert(i == A10_mask-1);
  printf("passed **\n");

  printf("  A11(n=%u)...",A11_size); fflush(0);
  state = seed & A11_mask;
  for (i = 0; i < A11_mask; ++i) {
    if (i < A11_mask-7) {
      A11_cycle8(state,ZERO);
      i += 7;
    } else
      A11_cycle(state,ZERO);
    if (state == (seed&A11_mask)) break;
  }
  assert(i == A11_mask-1);
  printf("passed **\n");

  printf("  A12(n=%u)...",A12_size); fflush(0);
  lstate = (u64)seed & A12_mask;
  for (l = 0; l < A12_mask; ++l) {
    if (l < A12_mask-7) {
      A12_cycle8(lstate,ZERO);
      l += 7;
    } else
      A12_cycle(lstate,ZERO);
    if (lstate == ((u64)seed&A12_mask)) break;
  }
  assert(l == (u64)A12_mask-1);
  printf("passed **\n");
}
  
/* ------------------------------------------------------------------------- *
 * Verify the Boolean function implementation
 * ------------------------------------------------------------------------- */

u32 Combiner(u32 x0, u32 x1, u32 x2, u32 x3, u32 x4, u32 x5, 
	     u32 x6, u32 x7, u32 x8, u32 x9, u32 x10, u32 x11, u32 x12)
{
  u32 A = x1^x2,
      C = x2^x9,
      H = x3^x7,
      T = x4^x9,
      E = ((x0^x6)&x5)^x6,
      R = ((x1^x4)&C)^T,
      b = (R^(A&x5)^x2)&H,
      a = ((x10^x11)&(C^(A&T)^E))^E,
      h = (x8^x12)&(b^a^R^x7^x10),
      n = H^A^T^a^h^x0^x5^x6^x11^x12;
  return (n);
} 

static void ACHTERBAHN_verify_BooleanFunction()
{
  u32 N11 = (ONE << 11),
      N13 = (ONE << 13);
  u32 x, ref, t;

  printf("* ================================================================ *\n");
  printf("* Verify Boolean Functions\n");
  printf("* ================================================================ *\n");
  fflush(0);
 
  for (x = 0; x < N13; ++x) {

    /* ANF of 13-variable Boolean function */
    ref = (x>>0) ^ (x>>1) ^ (x>>2) ^ (x>>3) ^ (x>>4) ^ (x>>5) ^ (x>>7) 
        ^ (x>>9) ^ (x>>11) ^ (x>>12) ^ ((x>>0)&(x>>5)) ^ ((x>>2)&(x>>10))
        ^ ((x>>2)&(x>>11)) ^ ((x>>4)&(x>>8)) ^ ((x>>4)&(x>>12)) ^ ((x>>5)&(x>>6)) 
        ^ ((x>>6)&(x>>10)) ^ ((x>>6)&(x>>8)) ^ ((x>>6)&(x>>11)) ^ ((x>>6)&(x>>12)) 
        ^ ((x>>7)&(x>>8)) ^ ((x>>7)&(x>>12)) ^ ((x>>10)&(x>>9)) ^ ((x>>10)&(x>>8)) 
        ^ ((x>>10)&(x>>12)) ^ ((x>>9)&(x>>8)) ^ ((x>>9)&(x>>11)) ^ ((x>>9)&(x>>12)) 
        ^ ((x>>0)&(x>>5)&(x>>10)) ^ ((x>>0)&(x>>5)&(x>>8)) ^ ((x>>0)&(x>>5)&(x>>11)) 
        ^ ((x>>0)&(x>>5)&(x>>12)) ^ ((x>>1)&(x>>2)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>10)) ^ ((x>>1)&(x>>4)&(x>>11)) ^ ((x>>1)&(x>>10)&(x>>9)) 
        ^ ((x>>1)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>9)&(x>>11)) ^ ((x>>1)&(x>>9)&(x>>12)) 
        ^ ((x>>2)&(x>>3)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>12)) ^ ((x>>2)&(x>>4)&(x>>10)) 
        ^ ((x>>2)&(x>>4)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>11)) ^ ((x>>2)&(x>>4)&(x>>12)) 
        ^ ((x>>2)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>7)&(x>>12)) ^ ((x>>2)&(x>>10)&(x>>9)) 
        ^ ((x>>2)&(x>>10)&(x>>8)) ^ ((x>>2)&(x>>10)&(x>>12)) ^ ((x>>2)&(x>>9)&(x>>11)) 
        ^ ((x>>2)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>11)&(x>>12)) ^ ((x>>3)&(x>>4)&(x>>8)) 
        ^ ((x>>3)&(x>>4)&(x>>12)) ^ ((x>>3)&(x>>9)&(x>>8)) ^ ((x>>3)&(x>>9)&(x>>12)) 
        ^ ((x>>4)&(x>>7)&(x>>8)) ^ ((x>>4)&(x>>7)&(x>>12)) ^ ((x>>4)&(x>>9)&(x>>8)) 
        ^ ((x>>4)&(x>>9)&(x>>12)) ^ ((x>>5)&(x>>6)&(x>>10)) ^ ((x>>5)&(x>>6)&(x>>8) )
        ^ ((x>>5)&(x>>6)&(x>>11)) ^ ((x>>5)&(x>>6)&(x>>12)) ^ ((x>>6)&(x>>10)&(x>>8))
        ^ ((x>>6)&(x>>10)&(x>>12)) ^ ((x>>6)&(x>>8)&(x>>11)) ^ ((x>>6)&(x>>11)&(x>>12)) 
        ^ ((x>>7)&(x>>9)&(x>>8)) ^ ((x>>7)&(x>>9)&(x>>12)) ^ ((x>>10)&(x>>9)&(x>>8))
        ^ ((x>>10)&(x>>9)&(x>>12)) ^ ((x>>9)&(x>>8)&(x>>11)) ^ ((x>>9)&(x>>11)&(x>>12))
        ^ ((x>>0)&(x>>5)&(x>>10)&(x>>8)) ^ ((x>>0)&(x>>5)&(x>>10)&(x>>12)) 
        ^ ((x>>0)&(x>>5)&(x>>8)&(x>>11)) ^ ((x>>0)&(x>>5)&(x>>11)&(x>>12)) 
        ^ ((x>>1)&(x>>2)&(x>>3)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>3)&(x>>12)) 
        ^ ((x>>1)&(x>>2)&(x>>7)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>7)&(x>>12)) 
        ^ ((x>>1)&(x>>3)&(x>>5)&(x>>8)) ^ ((x>>1)&(x>>3)&(x>>5)&(x>>12)) 
        ^ ((x>>1)&(x>>3)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>3)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>10)&(x>>8)) ^ ((x>>1)&(x>>4)&(x>>10)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>8)&(x>>11)) ^ ((x>>1)&(x>>4)&(x>>11)&(x>>12)) 
        ^ ((x>>1)&(x>>5)&(x>>7)&(x>>8)) ^ ((x>>1)&(x>>5)&(x>>7)&(x>>12)) 
        ^ ((x>>1)&(x>>7)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>7)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>10)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>10)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>9)&(x>>8)&(x>>11)) ^ ((x>>1)&(x>>9)&(x>>11)&(x>>12)) 
        ^ ((x>>2)&(x>>3)&(x>>4)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>4)&(x>>12))
        ^ ((x>>2)&(x>>3)&(x>>5)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>5)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>7)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>10)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>10)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>4)&(x>>11)&(x>>12)) 
        ^ ((x>>2)&(x>>5)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>5)&(x>>7)&(x>>12)) 
        ^ ((x>>2)&(x>>10)&(x>>9)&(x>>8)) ^ ((x>>2)&(x>>10)&(x>>9)&(x>>12)) 
        ^ ((x>>2)&(x>>9)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>9)&(x>>11)&(x>>12)) 
        ^ ((x>>3)&(x>>4)&(x>>9)&(x>>8)) ^ ((x>>3)&(x>>4)&(x>>9)&(x>>12))  
        ^ ((x>>4)&(x>>7)&(x>>9)&(x>>8)) ^ ((x>>4)&(x>>7)&(x>>9)&(x>>12)) 
        ^ ((x>>5)&(x>>6)&(x>>10)&(x>>8)) ^ ((x>>5)&(x>>6)&(x>>10)&(x>>12)) 
        ^ ((x>>5)&(x>>6)&(x>>8)&(x>>11)) ^ ((x>>5)&(x>>6)&(x>>11)&(x>>12));
    
    t = Combiner((x>>0), (x>>1), (x>>2), (x>>3), (x>>4),  (x>>5), 
		 (x>>6), (x>>7), (x>>8), (x>>9), (x>>10), (x>>11), (x>>12));
    
    assert(ref == t);
  }

  for (x = 0; x < N11; ++x) {

    /* ANF of 11-variable Boolean function */
    ref =  (x>>0) ^ (x>>1) ^ (x>>2) ^ (x>>3) ^ (x>>4) ^ (x>>6) ^ (x>>8) ^ (x>>10) 
         ^ ((x>>1)&(x>>9)) ^ ((x>>1)&(x>>10)) ^ ((x>>3)&(x>>7)) ^ ((x>>4)&(x>>5)) 
         ^ ((x>>5)&(x>>9)) ^ ((x>>5)&(x>>7)) ^ ((x>>5)&(x>>10)) ^ ((x>>6)&(x>>7)) 
         ^ ((x>>9)&(x>>8)) ^ ((x>>9)&(x>>7)) ^ ((x>>8)&(x>>7)) ^ ((x>>8)&(x>>10)) 
         ^ ((x>>0)&(x>>1)&(x>>7)) ^ ((x>>0)&(x>>3)&(x>>9)) ^ ((x>>0)&(x>>3)&(x>>10)) 
         ^ ((x>>0)&(x>>9)&(x>>8)) ^ ((x>>0)&(x>>8)&(x>>7)) ^ ((x>>0)&(x>>8)&(x>>10)) 
         ^ ((x>>1)&(x>>2)&(x>>7)) ^ ((x>>1)&(x>>3)&(x>>9)) ^ ((x>>1)&(x>>3)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>10)) ^ ((x>>1)&(x>>6)&(x>>7)) ^ ((x>>1)&(x>>9)&(x>>8)) 
         ^ ((x>>1)&(x>>9)&(x>>7)) ^ ((x>>1)&(x>>8)&(x>>10)) ^ ((x>>1)&(x>>7)&(x>>10)) 
         ^ ((x>>2)&(x>>3)&(x>>7)) ^ ((x>>2)&(x>>8)&(x>>7)) ^ ((x>>3)&(x>>6)&(x>>7)) 
         ^ ((x>>3)&(x>>8)&(x>>7)) ^ ((x>>4)&(x>>5)&(x>>9)) ^ ((x>>4)&(x>>5)&(x>>7)) 
         ^ ((x>>4)&(x>>5)&(x>>10)) ^ ((x>>5)&(x>>9)&(x>>7)) ^ ((x>>5)&(x>>7)&(x>>10)) 
         ^ ((x>>6)&(x>>8)&(x>>7)) ^ ((x>>9)&(x>>8)&(x>>7)) ^ ((x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>0)&(x>>1)&(x>>2)&(x>>7)) ^ ((x>>0)&(x>>1)&(x>>6)&(x>>7)) 
         ^ ((x>>0)&(x>>2)&(x>>4)&(x>>7)) ^ ((x>>0)&(x>>2)&(x>>8)&(x>>7)) 
         ^ ((x>>0)&(x>>3)&(x>>9)&(x>>7)) ^ ((x>>0)&(x>>3)&(x>>7)&(x>>10)) 
         ^ ((x>>0)&(x>>4)&(x>>6)&(x>>7)) ^ ((x>>0)&(x>>6)&(x>>8)&(x>>7)) 
         ^ ((x>>0)&(x>>9)&(x>>8)&(x>>7)) ^ ((x>>0)&(x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>1)&(x>>2)&(x>>3)&(x>>7)) ^ ((x>>1)&(x>>2)&(x>>4)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>6)&(x>>7)) ^ ((x>>1)&(x>>3)&(x>>9)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>7)&(x>>10)) ^ ((x>>1)&(x>>4)&(x>>6)&(x>>7)) 
         ^ ((x>>1)&(x>>9)&(x>>8)&(x>>7)) ^ ((x>>1)&(x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>2)&(x>>3)&(x>>8)&(x>>7)) ^ ((x>>3)&(x>>6)&(x>>8)&(x>>7)) 
         ^ ((x>>4)&(x>>5)&(x>>9)&(x>>7)) ^ ((x>>4)&(x>>5)&(x>>7)&(x>>10));
    
    t = Combiner(0, (x>>0), (x>>1), (x>>2), (x>>3), (x>>4), 
		    (x>>5), (x>>6), (x>>7), (x>>8), (x>>9), (x>>10), 0);

    assert(ref == t);
  }
  
  printf("** passed **\n\n");
}


/* ------------------------------------------------------------------------- *
 * Verify consistency of ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()
 * ------------------------------------------------------------------------- */

static void  ACHTERBAHN_verify_encrypt()
{
 
  enum  { N = 512 };        /* buffer size */
  u8    plaintext[N],
        ciphertext[N],
        keystream[N];

  ECRYPT_ctx ctx;
  u32   length, i, j, k, ivlen, keylen;

  key128_t key[2] = { "DonaldDuck", "Bertrand Russell" };
  iv128_t  iv     = "Albert Einstein!";

  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  /* create a pseudo-random message */
  for (k = 0; k < N; ++k) plaintext[k] = (u8)(k*16807);

  /* loop over of the key sizes 80 and 128 and all allowed sizes IV */
  for (k = 0; ECRYPT_KEYSIZE(k) <= ECRYPT_MAXKEYSIZE; ++k) {
    keylen = ECRYPT_KEYSIZE(k);
    
    for (i = 0; ECRYPT_IVSIZE(i) <= keylen; ++i) {
      ivlen = ECRYPT_IVSIZE(i);
      
      /* setup key */
      ECRYPT_keysetup(&ctx,key[k],keylen,ivlen);
      printf("  keylen = %d, ivlen=%d\n",keylen,ivlen);
      fflush(0);
      
      /* loop over various message lengths */
      for (length = 0; length <= N; ++length) {
        
        /* generate key stream for given IV */
        ECRYPT_ivsetup(&ctx,iv);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        ECRYPT_keystream_bytes(&ctx,keystream,length);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        
        /* encrypt directly for given IV */
        ECRYPT_ivsetup(&ctx,iv);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        ECRYPT_encrypt_bytes(&ctx,plaintext,ciphertext,length);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        
        /* verify */
        for (j = 0; j < length; ++j)
          assert( ciphertext[j] == (keystream[j] ^ plaintext[j]) );
      }
    }
  }
  
  printf("\n** passed ** \n\n");
}

/* ------------------------------------------------------------------------- *
 * Some encryption/decryption tests
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_encr_decr()
{
  enum  { N  = 1024 };      /* buffer size */
  u8    plaintext[N],
        ciphertext[N];

  ECRYPT_ctx ctx;
  u32   i, k, msglen, ivlen, keylen;

  key128_t key  = "Asterix & Obelix";
  iv128_t  iv   = "Albert Einstein!";

  /* 167 byte message */
  const u8 *original 
        = (u8*)"The wireless telegraph is not difficult to understand.\n"
               "The ordinary telegraph is like a very long cat.\n"
               "You pull the tail in New York, and it meows in Los Angeles.\n"
               "The wireless is the same, only without the cat. (A. Einstein).";

  printf("* ================================================================ *\n");
  printf("* Encryption/decryption test\n");
  printf("* ================================================================ *\n");
  fflush(0);

  msglen = (u32)strlen((char*)original);

  /* loop over all allowed sizes of the key and the IV */
  for (k = 0; ECRYPT_KEYSIZE(k) <= ECRYPT_MAXKEYSIZE; ++k) {
    keylen = ECRYPT_KEYSIZE(k);
    
    for (i = 0; ECRYPT_IVSIZE(i) <= keylen; ++i) {
      ivlen = ECRYPT_IVSIZE(i);
      
      ECRYPT_keysetup(&ctx,key,keylen,ivlen);
      ECRYPT_ivsetup(&ctx,iv);
      if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      ECRYPT_encrypt_bytes(&ctx,original,ciphertext,msglen);
      if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      
      ECRYPT_keysetup(&ctx,key,keylen,ivlen);
      ECRYPT_ivsetup(&ctx,iv);
      if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      ECRYPT_decrypt_bytes(&ctx,ciphertext,plaintext,msglen);
      if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      
      if ((ivlen == keylen)) {
        print_string("\nkey: ",keylen/8,key);
        print_string("IV:  ",ivlen/8,iv);
        print_string("------------------------ original  -------------------------\n",msglen,original);
        print_hexstr("------------------------ encrypted -------------------------\n",msglen,ciphertext);
        print_string("------------------------ decrypted -------------------------\n",msglen,plaintext);
      }
      
      assert( !strncmp((char*)original,(char*)plaintext,msglen) );
    }
  }
  
  printf("\n** passed **\n\n");
}

/* ------------------------------------------------------------------------- *
 * Verify ECRYPT_keystream_bytes()
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_keystream()
{
  u32 k;
  enum  { N  = 256 };       /* buffer size */
  u8 keystream[N];

  u8 reference80[N] = {
    0xE5,0x9C,0xD6,0x32,0xF3,0xF1,0xAF,0x7D,0xA4,0x7E,0x19,0xFF,0x46,0x65,0x1F,0x38,
    0x10,0x3A,0x29,0xF2,0xF6,0x55,0xED,0xC0,0x7F,0x5D,0x6D,0x2D,0xD6,0x2A,0x96,0xAA,
    0x30,0xF6,0x8D,0xAB,0x76,0x3F,0x28,0x5B,0xEC,0x05,0x26,0x5A,0xB5,0xCD,0xE6,0x43,
    0x78,0x9A,0x19,0x11,0x7A,0x92,0xE6,0x1F,0xC5,0x12,0xD9,0xA6,0xA7,0xA4,0x3F,0xC5,
    0x7D,0x16,0xCF,0xD8,0x4B,0x05,0xEC,0x11,0xD7,0x5A,0x03,0x9C,0x40,0xCA,0x49,0x6A,
    0x61,0xB2,0xA1,0x22,0x86,0xB2,0xC3,0xB1,0x5A,0x75,0x8C,0x80,0x63,0x8D,0x48,0xB7,
    0xA2,0x2F,0xAB,0x05,0x15,0xAB,0xBF,0xAB,0xDA,0x03,0x97,0x0F,0x4E,0x30,0x37,0x03,
    0x02,0x2C,0x81,0x79,0xDE,0xE8,0x18,0x52,0xAA,0x3C,0x7C,0xCF,0x8B,0x43,0xE6,0x92,
    0x20,0x5C,0xF2,0x07,0xF4,0xC7,0xEE,0xA2,0xAF,0x9D,0x48,0x4B,0x3B,0x8C,0x6E,0x05,
    0xEF,0x09,0xDF,0xDE,0x12,0x91,0x8C,0x0D,0x2F,0x71,0x9D,0x17,0xCE,0xD9,0x13,0x65,
    0x49,0xCD,0xBB,0x48,0xCD,0x30,0x5F,0x41,0xC4,0xA0,0xA2,0x59,0x9F,0xE2,0x63,0x9A,
    0x7E,0x4B,0x4A,0x50,0x3A,0x3F,0x72,0xA5,0xE0,0x86,0xDB,0x6C,0x6B,0x4D,0x75,0xF1,
    0x8A,0xF9,0x9A,0x57,0x1D,0x65,0x08,0x40,0x8F,0xEA,0xFE,0x96,0x11,0x09,0x2E,0x5B,
    0x87,0x48,0x09,0xBC,0xE8,0xE6,0x74,0x2D,0x29,0xCA,0x36,0x74,0x41,0x07,0xA7,0x72,
    0x76,0x75,0x2F,0x57,0xBE,0xA9,0x26,0xFA,0xD8,0x5E,0x3B,0x79,0x85,0x11,0xB0,0xDB,
    0x10,0x50,0x39,0x69,0x81,0x2A,0x92,0x7D,0xD4,0x61,0x1A,0xA2,0x07,0x55,0xBB,0x12
  };

  u8 reference128[N] = {
    0xDF,0x71,0xF0,0x42,0x73,0x8F,0x6D,0x9E,0xC2,0x1D,0x89,0x6D,0x0C,0xC1,0x2B,0xAF,
    0x54,0xC8,0xCE,0x55,0xA6,0x50,0x7A,0x12,0x43,0xB4,0x71,0xC2,0xCD,0xF0,0xEC,0x42,
    0x86,0xFC,0x01,0x45,0x43,0x80,0x90,0x13,0xDC,0xA4,0xCE,0xDB,0x0F,0x11,0xDA,0xF4,
    0x52,0xBD,0xCA,0x14,0x6F,0x6B,0x65,0x72,0x1D,0xBC,0x79,0x2C,0xD2,0xB6,0x36,0x14,
    0xF8,0xDB,0xE7,0xCB,0x7B,0x16,0x35,0x61,0xE8,0x10,0x4A,0x75,0xBD,0x4B,0x92,0x9E,
    0xA8,0x02,0x87,0x96,0x13,0x93,0x4A,0xB5,0xFD,0x91,0x16,0x1F,0x35,0x00,0xE5,0x3F,
    0x36,0x69,0x85,0x34,0xCC,0x9C,0xE6,0xE9,0xA5,0xC2,0x74,0xC8,0x2D,0x04,0x93,0x2E,
    0x75,0x06,0x40,0x7A,0xDE,0xAF,0x61,0x1F,0x00,0xF7,0xD3,0x8D,0x2F,0x1D,0x38,0x94,
    0xE2,0x12,0xCE,0x9F,0xF8,0xCD,0x9B,0x93,0x70,0x18,0xE2,0x56,0x9B,0xB5,0x54,0x45,
    0xEC,0x60,0xB8,0x52,0xB3,0xE6,0x6D,0x53,0x68,0x9B,0x3E,0x40,0x61,0x0A,0x22,0xA8,
    0xD3,0xA1,0x03,0x5C,0xC1,0x76,0x1D,0x50,0xB0,0x51,0xF4,0xDA,0xB4,0xE9,0x2E,0xA1,
    0x57,0xA3,0xD0,0x5F,0x11,0xDD,0x54,0xCF,0x6A,0x07,0xA4,0x4A,0x21,0x56,0x51,0xA6,
    0x91,0x5F,0xF4,0x77,0x58,0x81,0x90,0x29,0x50,0xE4,0x92,0xEA,0xFB,0x6C,0x66,0x28,
    0x85,0x06,0xF2,0xBD,0x60,0xF8,0x1A,0x73,0x68,0x4A,0xF7,0xCD,0xEC,0xCF,0xD0,0x1A,
    0xCD,0x09,0xA8,0x6B,0xC6,0x43,0x21,0x8A,0xEF,0xDF,0x27,0xC1,0x47,0x25,0xB9,0x06,
    0xA9,0x40,0x44,0x62,0x86,0x61,0x20,0x66,0x1E,0x70,0x76,0xED,0x93,0xEC,0x31,0xA9
  };

  ECRYPT_ctx ctx;
  iv128_t  iv     = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10 };
  key128_t key80  = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a };
  key128_t key128 = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10 };

  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  print_hexstr("\nkey = ",10,key80);
  print_hexstr("iv  = ",10,iv);

  ECRYPT_keysetup(&ctx,key80,80,80);
  ECRYPT_ivsetup(&ctx,iv);
  ECRYPT_keystream_bytes(&ctx,keystream,N);

  print_hexstr ("keystream =\n", N, keystream);

  for (k = 0; k < N; ++k)
    assert(keystream[k] == reference80[k]);
  printf("** passed **\n");

  print_hexstr("\nkey = ",16,key128);
  print_hexstr("iv  = ",16,iv);

  ECRYPT_keysetup(&ctx,key128,128,128);
  ECRYPT_ivsetup(&ctx,iv);
  ECRYPT_keystream_bytes(&ctx,keystream,N);

  print_hexstr ("keystream =\n", N, keystream);

  for (k = 0; k < N; ++k)
    assert(keystream[k] == reference128[k]);

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
  assert(!strcmp(ECRYPT_NAME,"ACHTERBAHN-128/80"));
  assert(ECRYPT_MAXKEYSIZE == 128);
  assert(ECRYPT_KEYSIZE(0) ==  40);
  assert(ECRYPT_KEYSIZE(11)== 128);
  assert(ECRYPT_MAXIVSIZE  == 128);
  assert(ECRYPT_IVSIZE(0)  ==   0);
  assert(ECRYPT_IVSIZE(1)  ==   8);
  assert(ECRYPT_IVSIZE(10) ==  80);
  assert(ECRYPT_IVSIZE(16) == 128);
  
  /* ----------------------------------------------------------------------- *
   * Functional verification of the implementation.
   * ----------------------------------------------------------------------- */

  ACHTERBAHN_verify_keystream();
  ACHTERBAHN_verify_encr_decr();
  ACHTERBAHN_verify_encrypt();
  ACHTERBAHN_verify_BooleanFunction();
  ACHTERBAHN_verify_NLFSRs();

  printf("* ================================================================ *\n");
  printf("* all tests passed - hit return to finish\n");
  printf("* ================================================================ *\n");
  printf("*\n");
  fflush(0);
  
  getchar();
  return 0;
}

/* ------------------------------------------------------------------------- */

#endif
