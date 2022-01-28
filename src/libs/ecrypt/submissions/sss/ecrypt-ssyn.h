/* ecrypt-ssyn.h */

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
#define ECRYPT_NAME "SSS"    /* [edit] */ 
#define ECRYPT_PROFILE "SW & HW"

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

#define ECRYPT_MAXKEYSIZE 128                 /* [edit] */
#define ECRYPT_KEYSIZE(i) (128 + (i)*32)      /* [edit] */

#define ECRYPT_MAXIVSIZE 128                  /* [edit] */
#define ECRYPT_IVSIZE(i) (32 + (i)*32)        /* [edit] */

/*
 * Specify the number of consecutive bytes of ciphertext that need to
 * to be received correctly in order to synchronise the keystream.
 */
#define ECRYPT_SYNCLENGTH 34                  /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * ECRYPT_ctx is a structure storing the (expanded) key and iv of the
 * cipher.
 */

#include "sss.h"
typedef struct
{
  /* 
   * [edit]
   *
   * Put here variables representing the (expanded) key and iv. These
   * variables are set during the key and iv setup, and are not
   * supposed to change during the encryption of a message.
   */
  sss_ctx ctx;
  u32 ivsize;
} ECRYPT_ctx;

/* ------------------------------------------------------------------------- */

/* Mandatory functions */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void ECRYPT_init(void);

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

/*
 * By default ECRYPT_encrypt_bytes() and ECRYPT_decrypt_bytes() are
 * defined as macros which redirect the call to a single function
 * ECRYPT_process_bytes(). If you want to provide separate encryption
 * and decryption functions, please undef
 * ECRYPT_HAS_SINGLE_BYTE_FUNCTION.
 */
#undef ECRYPT_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_BYTE_FUNCTION

#define ECRYPT_encrypt_bytes(ctx, previous, plaintext, ciphertext, msglen) \
  ECRYPT_process_bytes(0, ctx, previous, plaintext, ciphertext, msglen)

#define ECRYPT_decrypt_bytes(ctx, previous, ciphertext, plaintext, msglen) \
  ECRYPT_process_bytes(1, ctx, previous, ciphertext, plaintext, msglen)

void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

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

#endif

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

/*
 * Undef ECRYPT_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define ECRYPT_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_PACKET_FUNCTION

#define ECRYPT_encrypt_packet(                                        \
    ctx, iv, plaintext, ciphertext, mglen)                            \
  ECRYPT_process_packet(0,                                            \
    ctx, iv, plaintext, ciphertext, mglen)

#define ECRYPT_decrypt_packet(                                        \
    ctx, iv, ciphertext, plaintext, mglen)                            \
  ECRYPT_process_packet(1,                                            \
    ctx, iv, ciphertext, plaintext, mglen)

void ECRYPT_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* iv,
  const u8* input, 
  u8* output, 
  u32 msglen);

#else

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

#endif

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

/*
 * Undef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define ECRYPT_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION

#define ECRYPT_encrypt_blocks(ctx, previous, plaintext, ciphertext, blocks) \
  ECRYPT_process_blocks(0, ctx, previous, plaintext, ciphertext, blocks)

#define ECRYPT_decrypt_blocks(ctx, previous, ciphertext, plaintext, blocks) \
  ECRYPT_process_blocks(1, ctx, previous, ciphertext, plaintext, blocks)

void ECRYPT_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* previous,
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

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

#endif

/* ------------------------------------------------------------------------- */

#endif
