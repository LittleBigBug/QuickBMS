/* ecrypt-ssyn-ae.h */

/* 
 * Header file for self-synchronising stream ciphers with
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

#ifndef ECRYPT_SSYN_AE
#define ECRYPT_SSYN_AE

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/* 
 * The name of your cipher.
 */
#define ECRYPT_NAME "SSS"    /* [edit] */ 
#define ECRYPT_PROFILE "SW & HW"

/*
 * Specify which key, IV, and MAC sizes are supported by your
 * cipher. A user should be able to enumerate the supported sizes by
 * running the following code:
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

#define ECRYPT_MAXMACSIZE 128                 /* [edit] */
#define ECRYPT_MACSIZE(i) (32 + (i)*32)       /* [edit] */

/*
 * Specify the number of consecutive bytes of ciphertext that need to
 * to be received correctly in order to synchronise the keystream.
 */
#define ECRYPT_SYNCLENGTH 34                  /* [edit] */

/* ------------------------------------------------------------------------- */

/* Data structures */

/* 
 * ECRYPT_AE_ctx is the structure containing the representation of the
 * internal state of your cipher. 
 *
 * If your cipher also supports unauthenticated encryption, and you
 * want to allow the user to call the unauthenticated functions
 * declared in "ecrypt-ssyn.h" as well, you might want to set the
 * ECRYPT_SUPPORTS_UAE flag and define the ECRYPT_AE_ctx context as an
 * extension of the ECRYPT_ctx structure defined in "ecrypt-ssyn.h".
 */

#define ECRYPT_SUPPORTS_UAE                    /* [edit] */
#ifndef ECRYPT_SUPPORTS_UAE

typedef struct
{
  /* 
   * [edit]
   *
   * In case your implementation does NOT support unauthenticated
   * encryption, put here all state variable needed during the
   * authenticated encryption process. If it does, use the structure
   * below.
   */
} ECRYPT_AE_ctx;

#else

#include "ecrypt-ssyn.h"

typedef struct
{
  /*
   * [edit]
   *
   * In case your implementation does support unauthenticated
   * encryption, put here all additional state variables needed to
   * generate the MAC.
   */
  u32 macsize;
} ECRYPT_macctx;

typedef struct
{
  ECRYPT_ctx ctx;
  ECRYPT_macctx macctx;
} ECRYPT_AE_ctx;

#endif

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
 * keysize, ivsize, and macsize from the set of supported values
 * specified above.
 */
void ECRYPT_AE_keysetup(
  ECRYPT_AE_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize,                 /* IV size in bits. */ 
  u32 macsize);               /* MAC size in bits. */ 

/*
 * IV setup. After having called ECRYPT_AE_keysetup(), the user is
 * allowed to call ECRYPT_AE_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's. This function also initializes the block of ECRYPT_SYNCLENGTH
 * bytes pointed to by 'previous', which will be passed as a parameter
 * in the first call to ECRYPT_AE_encrypt_x().
 */
void ECRYPT_AE_ivsetup(
  ECRYPT_AE_ctx* ctx,
  u8* previous,
  const u8* iv);

/*
 * Authenticated encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of authenticated
 * encrypt/decrypt functions. The ECRYPT_AE_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the ECRYPT_AE_encrypt_blocks() function (defined later) only
 * accepts lengths which are multiples of ECRYPT_SYNCLENGTH.
 * 
 * The user is allowed to make multiple calls to
 * ECRYPT_AE_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called ECRYPT_AE_encrypt_bytes(), unless he starts a new
 * message. Here is an example (N >= 2):
 *
 * u8 plaintext[N * ECRYPT_SYNCLENGTH];
 * u8 ciphertext[N * ECRYPT_SYNCLENGTH];
 *
 * u8* previous = ciphertext + (N - 1) * ECRYPT_SYNCLENGTH;
 *
 * ECRYPT_AE_keysetup(ctx, key, keysize, ...);
 * ECRYPT_AE_ivsetup(ctx, previous, iv);
 *
 * int rest = msglen;
 *
 * while (rest >= N * ECRYPT_SYNCLENGTH)
 *   {
 *     ... [import N new blocks of ECRYPT_SYNCLENGTH bytes in 'plaintext'] ...
 *
 *     ECRYPT_AE_encrypt_blocks(ctx, previous, plaintext, ciphertext, N);
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
 *     ECRYPT_AE_encrypt_bytes(ctx, previous, plaintext, ciphertext, rest);
 *
 *     ... [export leftover] ...
 *   }
 *
 * ECRYPT_AE_finalize(ctx, mac);
 *
 * Note that the implementation cannot assume anything about the
 * relative position of 'previous' and 'ciphertext', except that the
 * ECRYPT_SYNCLENGTH bytes pointed to by 'previous' do not overlap
 * with the first ECRYPT_SYNCLENGTH bytes of 'ciphertext'. When
 * decrypting, 'previous' and 'ciphertext' cannot overlap at all (the
 * user will need to switch between two ciphertext buffers).
 */

/*
 * By default ECRYPT_AE_encrypt_bytes() and ECRYPT_AE_decrypt_bytes()
 * are defined as macros which redirect the call to a single function
 * ECRYPT_AE_process_bytes(). If you want to provide separate
 * encryption and decryption functions, please undef
 * ECRYPT_HAS_SINGLE_BYTE_FUNCTION.
 */
#undef ECRYPT_HAS_SINGLE_BYTE_FUNCTION       /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_BYTE_FUNCTION

#define ECRYPT_AE_encrypt_bytes(ctx, previous, plaintext, ciphertext, msglen) \
  ECRYPT_AE_process_bytes(0, ctx, previous, plaintext, ciphertext, msglen)

#define ECRYPT_AE_decrypt_bytes(ctx, previous, ciphertext, plaintext, msglen) \
  ECRYPT_AE_process_bytes(1, ctx, previous, ciphertext, plaintext, msglen)

void ECRYPT_AE_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* input, 
  u8* output, 
  u32 msglen);                /* Message length in bytes. */ 

#else

void ECRYPT_AE_encrypt_bytes(
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen);                /* Message length in bytes. */ 

void ECRYPT_AE_decrypt_bytes(
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen);                /* Message length in bytes. */ 

#endif

/*
 * MAC output. The ECRYPT_AE_finalize() function produces a MAC, and
 * is to be called both after the encryption and the decryption of a
 * message. The user is supposed to check whether both MAC's are
 * identical and to delete all decrypted text if they are not.
 */
void ECRYPT_AE_finalize(
  ECRYPT_AE_ctx* ctx,
  u8* mac);

/* ------------------------------------------------------------------------- */

/* Optional features */

/* 
 * Please set the ECRYPT_SUPPORTS_AAD flag if your cipher can
 * authenticate associated data (without encrypting it). The function
 * ECRYPT_AE_authenticate_x() will be called (possibly multiple times)
 * before any call to ECRYPT_AE_encrypt_x(). The function has both a
 * byte and a block version (defined later), which are intended to be
 * used in the same way as the encryption function above.
 */

#define ECRYPT_SUPPORTS_AAD                    /* [edit] */
#ifdef ECRYPT_SUPPORTS_AAD

/*
 * If ECRYPT_SUPPORTS_INCREMENTAL_AAD flag is NOT set, the user is
 * only allowed to call ECRYPT_AE_authenticate_x() once. This
 * restriction is important for schemes which need to know the length
 * of the complete AAD in advance.
 */
#define ECRYPT_SUPPORTS_INCREMENTAL_AAD       /* [edit] */

void ECRYPT_AE_authenticate_bytes(
  ECRYPT_AE_ctx* ctx,
  u8* previous,
  const u8* aad,
  u32 aadlen);                /* Length of associated data in bytes. */

#endif

/* ------------------------------------------------------------------------- */

/* Optional optimizations */

/* 
 * By default, the functions in this section are implemented using
 * calls to functions declared above. However, you might want to
 * implement them differently for performance reasons.
 */

/*
 * All-in-one authenticated encryption/decryption of (short) packets.
 *
 * The default definitions of these functions can be found in
 * "ecrypt-ssyn-ae.c". If you want to implement them differently,
 * please undef the ECRYPT_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define ECRYPT_USES_DEFAULT_ALL_IN_ONE        /* [edit] */

/*
 * Undef ECRYPT_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#define ECRYPT_HAS_SINGLE_PACKET_FUNCTION     /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_PACKET_FUNCTION

#define ECRYPT_AE_encrypt_packet(                                     \
    ctx, iv, aad, aadlen, plaintext, ciphertext, mglen, mac)          \
  ECRYPT_AE_process_packet(0,                                         \
    ctx, iv, aad, aadlen, plaintext, ciphertext, mglen, mac)

#define ECRYPT_AE_decrypt_packet(                                     \
    ctx, iv, aad, aadlen, ciphertext, plaintext, mglen, mac)          \
  ECRYPT_AE_process_packet(1,                                         \
    ctx, iv, aad, aadlen, ciphertext, plaintext, mglen, mac)

void ECRYPT_AE_process_packet(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_AE_ctx* ctx, 
  const u8* iv,
  const u8* aad,
  u32 aadlen,
  const u8* input, 
  u8* output, 
  u32 msglen,
  u8* mac);

#else

void ECRYPT_AE_encrypt_packet(
  ECRYPT_AE_ctx* ctx,
  const u8* iv,
  const u8* aad,
  u32 aadlen,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen,
  u8* mac);

void ECRYPT_AE_decrypt_packet(
  ECRYPT_AE_ctx* ctx, 
  const u8* iv,
  const u8* aad,
  u32 aadlen,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen,
  u8* mac);

#endif

/*
 * Authenticated encryption/decryption of blocks.
 * 
 * By default, these functions are defined as macros. If you want to
 * provide a different implementation, please undef the
 * ECRYPT_USES_DEFAULT_BLOCK_MACROS flag and implement the functions
 * declared below.
 */

#define ECRYPT_USES_DEFAULT_BLOCK_MACROS      /* [edit] */
#ifdef ECRYPT_USES_DEFAULT_BLOCK_MACROS

#define ECRYPT_AE_encrypt_blocks(ctx, previous, plaintext, ciphertext, blocks)\
  ECRYPT_AE_encrypt_bytes(ctx, previous, plaintext, ciphertext,               \
    (blocks) * ECRYPT_SYNCLENGTH)

#define ECRYPT_AE_decrypt_blocks(ctx, previous, ciphertext, plaintext, blocks)\
  ECRYPT_AE_decrypt_bytes(ctx, previous, ciphertext, plaintext,               \
    (blocks) * ECRYPT_SYNCLENGTH)

#ifdef ECRYPT_SUPPORTS_AAD

#define ECRYPT_AE_authenticate_blocks(ctx, previous, aad, blocks)             \
  ECRYPT_AE_authenticate_bytes(ctx, previous, aad,                            \
    (blocks) * ECRYPT_SYNCLENGTH)

#endif

#else

/*
 * Undef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#define ECRYPT_HAS_SINGLE_BLOCK_FUNCTION      /* [edit] */
#ifdef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION

#define ECRYPT_AE_encrypt_blocks(ctx, previous, plaintext, ciphertext, blocks)\
  ECRYPT_AE_process_blocks(0, ctx, previous, plaintext, ciphertext, blocks)

#define ECRYPT_AE_decrypt_blocks(ctx, previous, ciphertext, plaintext, blocks)\
  ECRYPT_AE_process_blocks(1, ctx, previous, ciphertext, plaintext, blocks)

void ECRYPT_AE_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* input, 
  u8* output, 
  u32 blocks);                /* Message length in blocks. */

#else

void ECRYPT_AE_encrypt_blocks(
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* plaintext, 
  u8* ciphertext, 
  u32 blocks);                /* Message length in blocks. */ 

void ECRYPT_AE_decrypt_blocks(
  ECRYPT_AE_ctx* ctx, 
  const u8* previous,
  const u8* ciphertext, 
  u8* plaintext, 
  u32 blocks);                /* Message length in blocks. */ 

#endif

#ifdef ECRYPT_SUPPORTS_AAD

void ECRYPT_AE_authenticate_blocks(
  ECRYPT_AE_ctx* ctx,
  const u8* previous,
  const u8* aad,
  u32 blocks);                /* Message length in blocks. */ 

#endif

#endif

/* ------------------------------------------------------------------------- */

#endif
