/* ----------------------------------------------------------------------------
 *
 * hermes Algorithm   (c) 2005  Dr. Ulrich Kaiser, Germany
 *
 * UKA 08.Nov.2005  extract for API only
 *
 * ----------------------------------------------------------------------------
 */

#include "ecrypt-sync.h" 

#define K_LENGTH       10
#define X_LENGTH       23
#define O_LENGTH        8

#define INIT_ROUNDS    10
#define STREAM_ROUNDS   3

#define SBOX S

#define KEY_STEP1       3
#define KEY_STEP2       5
#define KEY_STEP3       7

#define K_MOD(p) ((p) < K_LENGTH ? (p) : (p) - K_LENGTH)
#define X_MOD(p) ((p) < X_LENGTH ? (p) : (p) - X_LENGTH)

/* ---------- AES encryption SBOX ----------- */

static const u8 S[256] = {
  0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
  0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
  0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
  0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
  0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
  0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
  0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
  0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
  0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
  0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
  0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
  0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
  0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
  0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
  0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
  0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
  0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
  0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
  0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
  0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
  0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
  0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
  0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
  0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
  0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
  0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
  0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
  0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
  0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
  0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
  0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
  0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};

/*
 * DESCRIPTION OF ALGORITHM
 *
 * accu is one byte
 * s-box is a table with 256 bytes, i.e. 8 bit input and 8 bit output
 *
 * A state byte and a key byte and the previous result (accu)
 * are EXORed. Then the s-box function is applied. The output is
 * fed into accu and into the oldest state byte.
 * Number of initial rounds: >= 4
 * Number of sub-rounds    : 23 (Hermes8-80)  or  37 (Hermes8-128)
 *
 * Confusion by:   S-Box, non-linear boolean function
 * Diffusion by:   Accu overwriting state byte
 *
 *
 * Notation
 *
 * Hermes8-80
 * State register x consists of 23 bytes x[22]..x[0].
 * Key register k consists of 10 bytes k[9]..k[0].
 * or
 * Hermes8-128
 * State register x consists of 37 bytes x[36]..x[0].
 * Key register k consists of 16 bytes k[15]..k[0].
 *
 */

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* -------------------------------  CORE  ---------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define CORE(ROUNDS)                                                          \
  do {                                                                        \
    int m;                                                                    \
                                                                              \
    for(m = 1; m <= ROUNDS; ++m, ++round)                                     \
      {                                                                       \
        int p1;                                                               \
                                                                              \
	for(p1 = 0; p1 < X_LENGTH; ++p1)                                      \
	  {                                                                   \
	    accu ^= ctx->x[p1] ^ ctx->k[p2];   /* linear operation       */   \
	    accu = SBOX[accu];                 /* NON-LINEAR OPERATION   */   \
	    ctx->x[p1] = accu;                 /* overwrite state byte ! */   \
	                                                                      \
	    /* update the pointers */                                         \
	    p2 = K_MOD(p2 + KEY_STEP1);                                       \
	                                                                      \
	    /* update two key bytes */                                        \
	    if (++src >= KEY_STEP3)                                           \
	      {                                                               \
		const int p3 = K_MOD(p2 + 1);  /* scratch */                  \
		const int p4 = K_MOD(p3 + 1);  /* scratch */                  \
		                                                              \
		u8 tmp;                        /* scratch */                  \
		                                                              \
		tmp = ctx->k[p3] ^ ctx->k[p2];                                \
		ctx->k[p3] = SBOX[tmp];                                       \
		tmp = ctx->k[p4] ^ ctx->k[p2];                                \
		ctx->k[p4] = SBOX[tmp];                                       \
		                                                              \
		src -= KEY_STEP3;                                             \
	      }	                                                              \
	  } /* for j */                                                       \
                                                                              \
	/* key scheduling so that x[] sees different k[] */                   \
	if(round % KEY_STEP2 == 0)                                            \
	  p2 = K_MOD(p2 + 1);                                                 \
	                                                                      \
      } /* for m */                                                           \
  } while (0)

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ---------------------------  ECRYPT  APIs  ------------------------------ */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/**
   Empty function, must be provided for the API...
*/
void ECRYPT_init()
{
}

/**
 loads key[] into ctx->key[]
 stores key size value in record
 stores iv size value in record
 @param  ctx   ptr to record
 @param  key   ptr to array with crypto key
 @param  keysize  in bits
 @param  ivsize   in bits
*/
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx,
  const u8* key,
  u32 keysize,               /* Key size in bits. */
  u32 ivsize )               /* IV size in bits. */ 
{
  int j;
   
  for (j = 0; j < K_LENGTH; ++j)
    ctx->key[j] = key[j];

  ctx->ni = ivsize / 8;

} /* ECRYPT_keysetup ------------------------------------------------------- */

/**
 loads iv[] into x[],
 runs initial rounds in order to hide IV and KEY from
 first key stream output.
 @param  ctx   ptr to record
 @param  iv    ptr to array with initial value IV
*/
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  const u8* iv )
{
  const int ni = ctx->ni;  /* IV size in bytes. */ 

  u8 accu;
  int p1;                  /* pointer to actual state byte */
  int p2;                  /* pointer to actual key byte */
  int src;                 /* sub-round counter */
  int round = 1;           /* round counter */
  
  int j;

  /* load key */
  for (j = 0; j < K_LENGTH; ++j)
    ctx->k[j] = ctx->key[j];

  p1   = (ctx->k[0] ^ ctx->k[1] ^ ctx->k[2]) % X_LENGTH;
  p2   = (ctx->k[3] ^ ctx->k[4] ^ ctx->k[5]) % K_LENGTH;
  accu = (ctx->k[6] ^ ctx->k[7] ^ ctx->k[8]);
  src  = (ctx->k[9] ^ ctx->k[0] ^ ctx->k[3]) % KEY_STEP3;

  /* fill IV into state[] and pad */
  for (j = 0; j < X_LENGTH; ++j, p1 = X_MOD(p1 + 1))
    ctx->x[j] = (p1 < ni ? iv[p1] : 0);

  /* ----------------- start of algorithm ----------------- */

  CORE(INIT_ROUNDS);

  ctx->accu    = accu;
  ctx->p2      = p2;
  ctx->src     = src;
  ctx->counter = round;

} /* ECRYPT_ivsetup -------------------------------------------------------- */

/**
 Encrypts a certain number of bytes of the plaintext.
 @param  in  ctx          ptr to record
 @param  in  plaintext    ptr to array with plaintext
 @param  out ciphertext   ptr to array with ciphertext
 @param  in  msglen       number of bytes to process
*/
void ECRYPT_process_bytes(
  int action,
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)         /* Message length in bytes. */ 
{
  /* ----------------- start of algorithm ----------------- */

  u8  accu  = ctx->accu;
  int p2    = ctx->p2;     /* pointer to actual key byte */
  int src   = ctx->src;    /* sub-round counter */
  int round = ctx->counter;
  
  while ((int)(msglen -= O_LENGTH) >= 0)
    {
      int po;              /* output pointer */
      int j;
      
      CORE(STREAM_ROUNDS);
      
      for (j = po = 0; j < O_LENGTH; ++j, po = X_MOD(po + 2))
	output[j] = input[j] ^ ctx->x[po];

      output += O_LENGTH;
      input += O_LENGTH;
    }

  /* if msglen was not a multile of O_LENGTH, we need one more block */
  if ((msglen += O_LENGTH) > 0)
    {
      int po;              /* output pointer */
      int j;

      CORE(STREAM_ROUNDS);

      for (j = po = 0; j < msglen; ++j, po = X_MOD(po + 2))
	output[j] = input[j] ^ ctx->x[po];
    }
  else
    {
      ctx->accu    = accu;
      ctx->p2      = p2;
      ctx->src     = src;
      ctx->counter = round;
    }

} /* ECRYPT_process_bytes -------------------------------------------------- */

/* ------------------------------------ end -------------------------------- */
