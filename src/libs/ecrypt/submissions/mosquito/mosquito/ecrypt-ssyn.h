/* ecrypt-MYG cipher.h */

/* 
 * Header file for self-synchronising stream ciphers without
 * authentication mechanism.
 * 
 * *** Please only edit parts marked with "[edit]". ***
 *
 * Note: while any self-synchronising stream cipher can be implemented
 * with the following API, it might not be very suitable for ciphers
 * which need a lot of ciphertext to synchronise, and/or have a
 * pipelined design. If this is the case, please use the more flexible
 * API of 'ecrypt-sync-ae.h'.
 */

#ifndef ECRYPT_SSYN
#define ECRYPT_SSYN

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define ECRYPT_NAME "MOSQUITO"    /* [edit] */ 
#define ECRYPT_PROFILE "HW"

/*
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
 */

#define ECRYPT_MAXKEYSIZE 96       
#define ECRYPT_KEYSIZE(i) (96 + (i)*1)

#define ECRYPT_MAXIVSIZE 104          
#define ECRYPT_IVSIZE(i) (0 + (i)*8)  

/*
 * Specify the number of consecutive bytes of ciphertext that need to
 * to be received correctly in order to synchronise the keystream.
 */
#define ECRYPT_SYNCLENGTH 14                  /* actually 105 bits */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * ECRYPT_ctx is a structure storing the (expanded) key and the internal state of the
 * cipher.
 */
 
typedef struct
{
  u32 IVsize;
  u8 wrkey[96];
  u8 a0[129];
  u8 a1[56];            /* stage 1, 53 bits, a1[53...55] are always 0 */
  u8 a2[56];            /* stage 2, 53 bits, a2[53...55] are always 0 */
  u8 a3[56];            /* stage 3, 53 bits, a3[53...55] are always 0 */
  u8 a4[56];            /* stage 4, 53 bits, a4[53...55] are always 0 */
  u8 a5[56];            /* stage 5, 53 bits, a5[53...55] are always 0 */
  u8 a6[12];            /* stage 6, 12 bits  */
  u8 a7[3];             /* stage 7, 3 bits  */
  u8 a8[1];             /* stage 8, 1 bit, to mimic additional delay */
  u8 *CCSR_0;           /* a[1...96]    is CCSR_0[1...96] */
  u8 *CCSR_1;           /* a[97...104]  is CCSR_1[89...96] */
  u8 *CCSR_2;           /* a[105...108] is CCSR_2[93...96] */
  u8 *CCSR_3;           /* a[109...112] is CCSR_3[93...96] */
  u8 *CCSR_4;           /* a[113...114] is CCSR_4[95...96] */
  u8 *CCSR_5;           /* a[115...116] is CCSR_5[95...96] */
  u8 *CCSR_6;           /* a[117...118] is CCSR_6[95...96] */
  u8 *CCSR_7;           /* a[119...120] is CCSR_7[95...96] */
  u8 *CCSR_8;           /* a[121] is CCSR_8[96] */
  u8 *CCSR_9;           /* a[122] is CCSR_9[96] */
  u8 *CCSR10;           /* a[123] is CCSR_10[96] */
  u8 *CCSR11;           /* a[124] is CCSR_11[96] */
  u8 *CCSR12;           /* a[125] is CCSR_12[96] */
  u8 *CCSR13;           /* a[126] is CCSR_13[96] */
  u8 *CCSR14;           /* a[127] is CCSR_14[96] */
  u8 *CCSR15;           /* a[128] is CCSR_15[96] */
} ECRYPT_ctx;     /* key and internal state */

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void ECRYPT_init();

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize);                /* IV size in bits. */ 

/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's. This function also initializes the block of ECRYPT_SYNCLENGTH
 * bytes pointed to by 'previous', which will be passed as a parameter
 * in the first call to ECRYPT_encrypt_x().
 */
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  u8* previous,
  const u8* iv);

/*
 * Encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of
 * encrypt/decrypt functions. The ECRYPT_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the ECRYPT_encrypt_blocks() function (defined later) only accepts
 * lengths which are multiples of ECRYPT_SYNCLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * ECRYPT_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called ECRYPT_encrypt_bytes(), unless he starts a new
 * message. Here is an example (N >= 2):
 *
 * u8 plaintext[N * ECRYPT_SYNCLENGTH];
 * u8 ciphertext[N * ECRYPT_SYNCLENGTH];
 *
 * u8* previous = ciphertext + (N - 1) * ECRYPT_SYNCLENGTH;
 *
 * ECRYPT_keysetup(ctx, key, keysize, ...);
 * ECRYPT_ivsetup(ctx, previous, iv);
 *
 * int rest = msglen;
 *
 * while (rest >= N * ECRYPT_SYNCLENGTH)
 *   {
 *     ... [import N new blocks of ECRYPT_SYNCLENGTH bytes in 'plaintext'] ...
 *
 *     ECRYPT_encrypt_blocks(ctx, previous, plaintext, ciphertext, N);
 *
 *     ... [export the N blocks of 'ciphertext'] ...
 *
 *     rest -= N * ECRYPT_SYNCLENGTH;
 *   }
 *
 * if (rest > 0)
 *   {
 *     ... [import leftover] ...
 *
 *     ECRYPT_encrypt_bytes(ctx, previous, plaintext, ciphertext, rest);
 *
 *     ... [export leftover] ...
 *   }
 *
 * Note that the implementation cannot assume anything about the
 * relative position of 'previous' and 'ciphertext', except that the
 * ECRYPT_SYNCLENGTH bytes pointed to by 'previous' do not overlap
 * with the first ECRYPT_SYNCLENGTH bytes of 'ciphertext'. When
 * decrypting, 'previous' and 'ciphertext' cannot overlap at all (the
 * user will need to switch between two ciphertext buffers).
 */

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);                /* Message length in bytes. */ 

/* ------------------------------------------------------------------------- */

/* Optional optimizations */

/* 
 * By default, the functions in this section are implemented using
 * calls to functions declared above. However, you might want to
 * implement them differently for performance reasons.
 */

/*
 * All-in-one encryption/decryption of (short) packets.
 *
 * The default definitions of these functions can be found in
 * "ecrypt-ssyn.c". If you want to implement them differently,
 * please undef the ECRYPT_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define ECRYPT_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

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

/* 
 * initialization of the CCSR pointers, must be executed once 
 * for each instance of ECRYPT_ctx 
 */


void init_ctx(
 ECRYPT_ctx *s);

/*
 * Encryption/decryption of blocks.
 * 
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * ECRYPT_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define ECRYPT_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef ECRYPT_USES_DEFAULT_BLOCK_MACROS

#define ECRYPT_encrypt_blocks(ctx, previous, plaintext, ciphertext, blocks)\
  ECRYPT_encrypt_bytes(ctx, previous, plaintext, ciphertext,               \
    (blocks) * ECRYPT_SYNCLENGTH)

#define ECRYPT_decrypt_blocks(ctx, previous, ciphertext, plaintext, blocks)\
  ECRYPT_decrypt_bytes(ctx, previous, ciphertext, plaintext,               \
    (blocks) * ECRYPT_SYNCLENGTH)

#else

void ECRYPT_encrypt_blocks(
  const ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void ECRYPT_decrypt_blocks(
  const ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

/* ------------------------------------------------------------------------- */

#endif
