/* ------------------------------------------------------------------------- *
 *
 *   Program:   A C H T E R B A H N  - 128 / 80
 *              Version 1.1
 *
 *   Authors:   Berndt M. Gammel, Email: gammel@matpack.de
 *              Rainer Goettfert, Email: rainer.goettfert@gmx.de
 *              Oliver Kniffler,  Email: oliver.kniffler@arcor.de
 *
 *  Language:   ANSI C99
 *
 *   Sources:   achterbahn128.c
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
 *              gcc 4.0.2, S.u.S.E. Linux 10.0
 *
 * Copyright:   (C) 2005-2006 by Berndt M. Gammel, Rainer Goettfert, 
 *                               and Oliver Kniffler
 *
 * ------------------------------------------------------------------------- */

#ifndef ECRYPT_SYNC
#define ECRYPT_SYNC

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* ------------------------------------------------------------------------- *
 * The name of the cipher.
 * ------------------------------------------------------------------------- */

#define ECRYPT_NAME        "ACHTERBAHN-128/80"
#define ECRYPT_PROFILE "_____"

#define ECRYPT_AUTHORS     "B. M. Gammel, R. Goettfert, O. Kniffler"

/* ------------------------------------------------------------------------- *
 * Specify which key and IV sizes are supported by your cipher. A user
 * should be able to enumerate the supported sizes by running the
 * following code:
 *
 * for (i = 0; ECRYPT_KEYSIZE(i) <= ECRYPT_MAXKEYSIZE; ++i)
 *   {
 *     keysize = ECRYPT_KEYSIZE(i);
 *
 *     ...
 *   }
 *
 * All sizes are in bits.
 * ------------------------------------------------------------------------- */

#define ECRYPT_MAXKEYSIZE 128U        /* constant keysizes 128 or 80 bit */
#define ECRYPT_KEYSIZE(i) (40U+i*8)

#define ECRYPT_MAXIVSIZE 128U         /* variable IV sizes */
#define ECRYPT_IVSIZE(i) (i*8)        /* 0, 8, 16,..., 128 for keysize 128 
				         0, 8, 16,..., 80  for keysize 80 */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* ------------------------------------------------------------------------- *
 * ECRYPT_ctx is the structure containing the representation of the
 * internal state of your cipher.
 * ------------------------------------------------------------------------- */

typedef struct
{
  u32 A0,A1,A2,A3,     /* state of 13 driving NLFSR */
      A4,A5,A6,A7,
      A8,A9,A10,A11;
  u64 A12;
  
  u64 key33;           /* save key bits 0 to 32 in one word */
  u32 keysize;         /* save key length in bits */
  u32 ivsize;          /* save IV length in bits */
  u32 iv_setup_done;   /* flag to mark that setup is done */
  u32 iv[128];         /* save IV  - 1 bit/word for fast access */
  u32 ky[128];         /* save key - 1 bit/word for fast access */
} ECRYPT_ctx;


/* Mandatory functions */

/* ------------------------------------------------------------------------- *
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 * ------------------------------------------------------------------------- */

void ECRYPT_init();

/* ------------------------------------------------------------------------- *
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified above.
 * ------------------------------------------------------------------------- */
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx,
  const u8* key,
  u32 keysize,                /* Key size in bits. */
  u32 ivsize);                /* IV size in bits. */

/* ------------------------------------------------------------------------- *
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different IV's.
 * ------------------------------------------------------------------------- */
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  const u8* iv);

/* ------------------------------------------------------------------------- *
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The ECRYPT_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the ECRYPT_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of ECRYPT_BLOCKLENGTH.
 *
 * The user is allowed to make multiple calls to
 * ECRYPT_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called ECRYPT_encrypt_bytes() (unless he starts a new message
 * of course). For example, this sequence of calls is acceptable:
 *
 * ECRYPT_keysetup();
 *
 * ECRYPT_ivsetup();
 * ECRYPT_encrypt_blocks();
 * ECRYPT_encrypt_blocks();
 * ECRYPT_encrypt_bytes();
 *
 * ECRYPT_ivsetup();
 * ECRYPT_encrypt_blocks();
 * ECRYPT_encrypt_blocks();
 *
 * ECRYPT_ivsetup();
 * ECRYPT_encrypt_bytes();
 *
 * The following sequence is not:
 *
 * ECRYPT_keysetup();
 * ECRYPT_ivsetup();
 * ECRYPT_encrypt_blocks();
 * ECRYPT_encrypt_bytes();
 * ECRYPT_encrypt_blocks();
 * ------------------------------------------------------------------------- */

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx,
  const u8* plaintext,
  u8* ciphertext,
  u32 msglen);                /* Message length in bytes. */

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx,
  const u8* ciphertext,
  u8* plaintext,
  u32 msglen);                /* Message length in bytes. */

/* ------------------------------------------------------------------------- */

/* Optional features */

/* ------------------------------------------------------------------------- *
 * For testing purposes it can sometimes be useful to have a function
 * which immediately generates keystream without having to provide it
 * with a zero plaintext. If your cipher cannot provide this function
 * (e.g., because it is not strictly a synchronous cipher), please
 * reset the ECRYPT_GENERATES_KEYSTREAM flag.
 * ------------------------------------------------------------------------- */

#define ECRYPT_GENERATES_KEYSTREAM
#ifdef ECRYPT_GENERATES_KEYSTREAM

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length);                /* Length of keystream in bytes. */

#endif

/* ------------------------------------------------------------------------- */

/* Optional optimizations */

/* ------------------------------------------------------------------------- *
 * By default, the functions in this section are implemented using
 * calls to functions declared above. However, you might want to
 * implement them differently for performance reasons.
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * All-in-one encryption/decryption of (short) packets.
 *
 * The default definitions of these functions can be found in
 * "ecrypt-sync.c". If you want to implement them differently, please
 * undef the ECRYPT_USES_DEFAULT_ALL_IN_ONE flag.
 * ------------------------------------------------------------------------- */

#define ECRYPT_USES_DEFAULT_ALL_IN_ONE

void ECRYPT_encrypt_packet(
  ECRYPT_ctx* ctx,
  const u8* iv,
  const u8* plaintext,
  u8* ciphertext,
  u32 msglen);

void ECRYPT_decrypt_packet(
  ECRYPT_ctx* ctx,
  const u8* iv,
  const u8* ciphertext,
  u8* plaintext,
  u32 msglen);

/* ------------------------------------------------------------------------- *
 * Encryption/decryption of blocks.
 *
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * ECRYPT_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 * ------------------------------------------------------------------------- */

#define ECRYPT_BLOCKLENGTH 4

#define ECRYPT_USES_DEFAULT_BLOCK_MACROS
#ifdef ECRYPT_USES_DEFAULT_BLOCK_MACROS

#define ECRYPT_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  ECRYPT_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * ECRYPT_BLOCKLENGTH)

#define ECRYPT_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  ECRYPT_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * ECRYPT_BLOCKLENGTH)

#ifdef ECRYPT_GENERATES_KEYSTREAM

#define ECRYPT_keystream_blocks(ctx, keystream, blocks)            \
  ECRYPT_AE_keystream_bytes(ctx, keystream,                        \
    (blocks) * ECRYPT_BLOCKLENGTH)

#endif

#else

void ECRYPT_encrypt_blocks(
  ECRYPT_ctx* ctx,
  const u8* plaintext,
  u8* ciphertext,
  u32 blocks);                /* Message length in blocks. */

void ECRYPT_decrypt_blocks(
  ECRYPT_ctx* ctx,
  const u8* ciphertext,
  u8* plaintext,
  u32 blocks);                /* Message length in blocks. */

#ifdef ECRYPT_GENERATES_KEYSTREAM

void ECRYPT_keystream_blocks(
  ECRYPT_AE_ctx* ctx,
  const u8* keystream,
  u32 blocks);                /* Keystream length in blocks. */

#endif

#endif

/*
 * If your cipher can be implemented in different ways, you can use
 * the ECRYPT_VARIANT parameter to allow the user to choose between
 * them at compile time (e.g., gcc -DECRYPT_VARIANT=3 ...). Please
 * only use this possibility if you really think it could make a
 * significant difference and keep the number of variants
 * (ECRYPT_MAXVARIANT) as small as possible (definitely not more than
 * 10). Note also that all variants should have exactly the same
 * external interface (i.e., the same ECRYPT_BLOCKLENGTH, etc.). 
 */
#define ECRYPT_MAXVARIANT 1                   /* [edit] */

#ifndef ECRYPT_VARIANT
#define ECRYPT_VARIANT 1
#endif

#if (ECRYPT_VARIANT > ECRYPT_MAXVARIANT)
#error this variant does not exist
#endif

/* ------------------------------------------------------------------------- */

#endif
