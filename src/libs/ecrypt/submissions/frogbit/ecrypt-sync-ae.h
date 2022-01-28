/* ecrypt-sync-ae.h */

/*
 * Header file for synchronous stream ciphers with authentication
 * mechanism.
 *
 * *** Please only edit parts marked with "[edit]". ***
 */

#ifndef ECRYPT_SYNC_AE
#define ECRYPT_SYNC_AE

#include "ecrypt-portable.h"

/* ------------------------------------------------------------------------- */

/* Cipher parameters */

/*
 * The name of your cipher.
 */
#define ECRYPT_NAME "Frogbit"
#define ECRYPT_PROFILE "_____"

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

#define ECRYPT_MAXKEYSIZE 128
#define ECRYPT_KEYSIZE(i) (128 + (i)*64)

#define ECRYPT_MAXIVSIZE 128
#define ECRYPT_IVSIZE(i) (64 + (i)*64)

#define ECRYPT_MAXMACSIZE 128
#define ECRYPT_MACSIZE(i) (32 + (i)*32)

/* ------------------------------------------------------------------------- */

/* Data structures */

/*
 * ECRYPT_AE_ctx is the structure containing the representation of the
 * internal state of your cipher.
 *
 * If your cipher also supports unauthenticated encryption, and you
 * want to allow the user to call the unauthenticated functions
 * declared in "ecrypt-sync.h" as well, you might want to set the
 * ECRYPT_SUPPORTS_UAE flag and define the ECRYPT_AE_ctx context as an
 * extension of the ECRYPT_ctx structure defined in "ecrypt-sync.h".
 */

#undef ECRYPT_SUPPORTS_UAE
#ifndef ECRYPT_SUPPORTS_UAE

/* Some Frogbit parameters */

#define FROGBIT10 (10)
#define FROGBIT_MAC_SECRETLEN (32)

#if !defined(MTGFSR_F)

/* Some MTGFSR parameters */
#define MTGFSR_F (2)
#define MTGFSR_siz (4)

typedef struct
{
 int ind;
 u32 bits[MTGFSR_siz];
 u32 table[1<<MTGFSR_F]; /* actually constant data is placed here */
} ECRYPT_AE_frogbit_prng;
#endif

typedef struct
{
/*--------------------------------------------------------------------*/
/*---------------- the state of the Frogbit algorithm ----------------*/

 int T[FROGBIT10]; /* the current permutation table, T(i-1-r'(i-1))   */
 int d_;           /* the current key stream number, d'(i-1)          */

 int s;            /* the previous bit, s{i-1}                        */

 /* local variables for the run length process                        */
 int r_;           /* the current bounded run count, r'(i-1)          */
 int acc_d;        /* accumulated binary representation K2{i-1}, k2{i}*/

/*--------------------------------------------------------------------*/
/*---------- the state of the 10 PRNGs (minimal secret key) ----------*/

  ECRYPT_AE_frogbit_prng prng[FROGBIT10];

/*--------------------------------------------------------------------*/
/*------------------- the ecrypt application data --------------------*/
  u8 key[(ECRYPT_MAXKEYSIZE+ECRYPT_MAXIVSIZE)/CHAR_BIT];
  u8 mac_secret[FROGBIT_MAC_SECRETLEN/CHAR_BIT];
  int keysize;                /* Key size in bits. */
  int ivsize;                 /* IV size in bits. */
  int macsize;                /* MAC size in bits. */

} ECRYPT_AE_ctx;

#else

#include "ecrypt-sync.h"

typedef struct
{
  /* not applicable */
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
 * IV's.
 */
void ECRYPT_AE_ivsetup(
  ECRYPT_AE_ctx* ctx,
  const u8* iv);

/*
 * Authenticated encryption/decryption of arbitrary length messages.
 *
 * For efficiency reasons, the API provides two types of authenticated
 * encrypt/decrypt functions. The ECRYPT_AE_encrypt_bytes() function
 * (declared here) encrypts byte strings of arbitrary length, while
 * the ECRYPT_AE_encrypt_blocks() function (defined later) only
 * accepts lengths which are multiples of ECRYPT_BLOCKLENGTH.
 *
 * The user is allowed to make multiple calls to
 * ECRYPT_AE_encrypt_blocks() to incrementally encrypt a long message,
 * but he is NOT allowed to make additional encryption calls once he
 * has called ECRYPT_AE_encrypt_bytes() (unless he starts a new
 * message of course). For example, this sequence of calls is
 * acceptable:
 *
 * ECRYPT_AE_keysetup();
 *
 * ECRYPT_AE_ivsetup();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_encrypt_bytes();
 * ECRYPT_AE_finalize();
 *
 * ECRYPT_AE_ivsetup();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_finalize();
 *
 * ECRYPT_AE_ivsetup();
 * ECRYPT_AE_encrypt_bytes();
 * ECRYPT_AE_finalize();
 *
 * The following sequence is not:
 *
 * ECRYPT_AE_keysetup();
 * ECRYPT_AE_ivsetup();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_encrypt_bytes();
 * ECRYPT_AE_encrypt_blocks();
 * ECRYPT_AE_finalize();
 */

/*
 * By default ECRYPT_AE_encrypt_bytes() and ECRYPT_AE_decrypt_bytes()
 * are defined as macros which redirect the call to a single function
 * ECRYPT_AE_process_bytes(). If you want to provide separate
 * encryption and decryption functions, please undef
 * ECRYPT_HAS_SINGLE_BYTE_FUNCTION.
 */
#undef ECRYPT_HAS_SINGLE_BYTE_FUNCTION
#ifdef ECRYPT_HAS_SINGLE_BYTE_FUNCTION

#define ECRYPT_AE_encrypt_bytes(ctx, plaintext, ciphertext, msglen)   \
  ECRYPT_AE_process_bytes(0, ctx, plaintext, ciphertext, msglen)

#define ECRYPT_AE_decrypt_bytes(ctx, ciphertext, plaintext, msglen)   \
  ECRYPT_AE_process_bytes(1, ctx, ciphertext, plaintext, msglen)

void ECRYPT_AE_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_AE_ctx* ctx,
  const u8* input,
  u8* output,
  u32 msglen);                /* Message length in bytes. */

#else

void ECRYPT_AE_encrypt_bytes(
  ECRYPT_AE_ctx* ctx,
  const u8* plaintext,
  u8* ciphertext,
  u32 msglen);                /* Message length in bytes. */

void ECRYPT_AE_decrypt_bytes(
  ECRYPT_AE_ctx* ctx,
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

#define ECRYPT_SUPPORTS_AAD
#ifdef ECRYPT_SUPPORTS_AAD

/*
 * If ECRYPT_SUPPORTS_INCREMENTAL_AAD flag is NOT set, the user is
 * only allowed to call ECRYPT_AE_authenticate_x() once. This
 * restriction is important for schemes which need to know the length
 * of the complete AAD in advance.
 */
#define ECRYPT_SUPPORTS_INCREMENTAL_AAD

void ECRYPT_AE_authenticate_bytes(
  ECRYPT_AE_ctx* ctx,
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
 * "ecrypt-sync-ae.c". If you want to implement them differently,
 * please undef the ECRYPT_USES_DEFAULT_ALL_IN_ONE flag.
 */
#define ECRYPT_USES_DEFAULT_ALL_IN_ONE

/*
 * Undef ECRYPT_HAS_SINGLE_PACKET_FUNCTION if you want to provide
 * separate packet encryption and decryption functions.
 */
#undef ECRYPT_HAS_SINGLE_PACKET_FUNCTION
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

#define ECRYPT_BLOCKLENGTH 4

#define ECRYPT_USES_DEFAULT_BLOCK_MACROS
#ifdef ECRYPT_USES_DEFAULT_BLOCK_MACROS

#define ECRYPT_AE_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  ECRYPT_AE_encrypt_bytes(ctx, plaintext, ciphertext,                 \
    (blocks) * ECRYPT_BLOCKLENGTH)

#define ECRYPT_AE_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  ECRYPT_AE_decrypt_bytes(ctx, ciphertext, plaintext,                 \
    (blocks) * ECRYPT_BLOCKLENGTH)

#ifdef ECRYPT_SUPPORTS_AAD

#define ECRYPT_AE_authenticate_blocks(ctx, aad, blocks)               \
  ECRYPT_AE_authenticate_bytes(ctx, aad,                              \
    (blocks) * ECRYPT_BLOCKLENGTH)

#endif

#else

/*
 * Undef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION if you want to provide
 * separate block encryption and decryption functions.
 */
#undef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION
#ifdef ECRYPT_HAS_SINGLE_BLOCK_FUNCTION

#define ECRYPT_AE_encrypt_blocks(ctx, plaintext, ciphertext, blocks)  \
  ECRYPT_AE_process_blocks(0, ctx, plaintext, ciphertext, blocks)

#define ECRYPT_AE_decrypt_blocks(ctx, ciphertext, plaintext, blocks)  \
  ECRYPT_AE_process_blocks(1, ctx, ciphertext, plaintext, blocks)

void ECRYPT_AE_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_AE_ctx* ctx,
  const u8* input,
  u8* output,
  u32 blocks);                /* Message length in blocks. */

#else

void ECRYPT_AE_encrypt_blocks(
  ECRYPT_AE_ctx* ctx,
  const u8* plaintext,
  u8* ciphertext,
  u32 blocks);                /* Message length in blocks. */

void ECRYPT_AE_decrypt_blocks(
  ECRYPT_AE_ctx* ctx,
  const u8* ciphertext,
  u8* plaintext,
  u32 blocks);                /* Message length in blocks. */

#endif

#ifdef ECRYPT_SUPPORTS_AAD

void ECRYPT_AE_authenticate_blocks(
  ECRYPT_AE_ctx* ctx,
  const u8* aad,
  u32 blocks);                /* Message length in blocks. */

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
