/*
 * REFERENCE IMPLEMENTATION OF STREAM CIPHER GRAIN-128
 *
 * Filename: grain-128.c
 *
 * Author:
 * Martin Hell
 * Dept. of Information Technology
 * P.O. Box 118
 * SE-221 00 Lund, Sweden,
 * email: martin@it.lth.se
 *
 * Synopsis:
 *  This file contains functions that implement the
 *  stream cipher Grain-128. It also implements functions 
 *  specified by the ECRYPT API.
 *
 *  NOTE: This implementation is not optimized for speed
 *  
 */
#include "ecrypt-sync.h"

void ECRYPT_init(void){}
/*
 * Function: grain_keystream
 *
 * Synopsis
 *  Generates a new bit and updates the internal state of the cipher.
 */

u8 grain_keystream(ECRYPT_ctx* ctx) {
	u8 i,NBit,LBit,outbit;
	/* Calculate feedback and output bits */
	outbit = ctx->NFSR[2]^ctx->NFSR[15]^ctx->NFSR[36]^ctx->NFSR[45]^ctx->NFSR[64]^ctx->NFSR[73]^ctx->NFSR[89]^ctx->LFSR[93]^(ctx->NFSR[12]&ctx->LFSR[8])^(ctx->LFSR[13]&ctx->LFSR[20])^(ctx->NFSR[95]&ctx->LFSR[42])^(ctx->LFSR[60]&ctx->LFSR[79])^(ctx->NFSR[12]&ctx->NFSR[95]&ctx->LFSR[95]);
	NBit=ctx->LFSR[0]^ctx->NFSR[0]^ctx->NFSR[26]^ctx->NFSR[56]^ctx->NFSR[91]^ctx->NFSR[96]^(ctx->NFSR[3]&ctx->NFSR[67])^(ctx->NFSR[11]&ctx->NFSR[13])^(ctx->NFSR[17]&ctx->NFSR[18])^(ctx->NFSR[27]&ctx->NFSR[59])^(ctx->NFSR[40]&ctx->NFSR[48])^(ctx->NFSR[61]&ctx->NFSR[65])^(ctx->NFSR[68]&ctx->NFSR[84]);
	LBit=ctx->LFSR[0]^ctx->LFSR[7]^ctx->LFSR[38]^ctx->LFSR[70]^ctx->LFSR[81]^ctx->LFSR[96];
	/* Update registers */
	for (i=1;i<(ctx->keysize);++i) {
		ctx->NFSR[i-1]=ctx->NFSR[i];
		ctx->LFSR[i-1]=ctx->LFSR[i];
	}
	ctx->NFSR[(ctx->keysize)-1]=NBit;
	ctx->LFSR[(ctx->keysize)-1]=LBit;
	return outbit;
}


/* Functions for the ECRYPT API */

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize)				  /* IV size in bits. */ 
{
	ctx->p_key=key;
	ctx->keysize=keysize;
	ctx->ivsize=ivsize;
}

/*
 * Function: ECRYPT_ivsetup
 *
 * Synopsis
 *  Load the key and perform initial clockings.
 *
 * Assumptions
 *  The key is 16 bytes and the IV is 12 bytes. The
 *  registers are loaded in the following way:
 *  
 *  NFSR[0] = lsb of key[0]
 *  ...
 *  NFSR[7] = msb of key[0]
 *  ...
 *  ...
 *  NFSR[120] = lsb of key[16]
 *  ...
 *  NFSR[127] = msb of key[16]
 *  LFSR[0] = lsb of IV[0]
 *  ...
 *  LFSR[7] = msb of IV[0]
 *  ...
 *  ...
 *  LFSR[88] = lsb of IV[12]
 *  ...
 *  LFSR[95] = msb of IV[12]
 */
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
	u32 i,j;
	u8 outbit;
	/* load registers */
	for (i=0;i<(ctx->ivsize)/8;++i) {
		for (j=0;j<8;++j) {
			ctx->NFSR[i*8+j]=((ctx->p_key[i]>>j)&1);  
			ctx->LFSR[i*8+j]=((iv[i]>>j)&1);
		}
	}
	for (i=(ctx->ivsize)/8;i<(ctx->keysize)/8;++i) {
		for (j=0;j<8;++j) {
			ctx->NFSR[i*8+j]=((ctx->p_key[i]>>j)&1);
			ctx->LFSR[i*8+j]=1;
		}
	}
	/* do initial clockings */
	for (i=0;i<256;++i) {
		outbit=grain_keystream(ctx);
		ctx->LFSR[127]^=outbit;
		ctx->NFSR[127]^=outbit;             
	}
}

/*
 * Function: ECRYPT_keystream_bytes
 *
 * Synopsis
 *  Generate keystream in bytes.
 *
 * Assumptions
 *  Bits are generated in order z0,z1,z2,...
 *  The bits are stored in a byte in order:
 *  
 *  lsb of keystream[0] = z0
 *  ...
 *  msb of keystream[0] = z7
 *  ...
 *  lsb of keystream[1] = z8
 *  ...
 *  msb of keystream[1] = z15
 *  ...
 *  ...
 *  ...
 */
void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx, 
  u8* keystream, 
  u32 msglen)
{
	u32 i,j;
	for (i = 0; i < msglen; ++i) {
		keystream[i]=0;
		for (j = 0; j < 8; ++j) {
			keystream[i]|=(grain_keystream(ctx)<<j);
		}
	}
}
void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen)
{
	u32 i,j;
	u8 k=0;
	for (i=0;i<msglen;++i) {
		k=0;
		for (j=0;j<8;++j) {
			k|=(grain_keystream(ctx)<<j);
		}
		ciphertext[i]=plaintext[i]^k;
	}
}

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen)
{
	u32 i,j;
	u8 k=0;
	for (i=0;i<msglen;++i) {
		k=0;
		for (j = 0; j < 8; ++j) {
			k|=(grain_keystream(ctx)<<j);
		}
		plaintext[i]=ciphertext[i]^k;
	}
}

