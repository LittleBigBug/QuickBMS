/* CryptMT Stream Cipher, Relying PMT */
/* By Hagita-Matsumoto-Nishimura-Saito */
/* PMT included */
/* 2005/12/22 */

#include <stdio.h>
#include <string.h>

#include "ecrypt-config.h"
#include "ecrypt-machine.h"
#include "ecrypt-portable.h"

#include "ecrypt-sync.h"


/**********Start of PMT ************/
/* Period parameters */
#define N 623
#define M 609
#define R 7 /* grot1 */
#define S 3 /* gs3 */

/* prototypes */
static void genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
				u32 len);
static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[], 
			  u32 len);
static void genrand_keystream_bytes_first(ECRYPT_ctx * ctx, u8 cipher[],
					  u32 len);
static void genrand_keystream_bytes(ECRYPT_ctx * ctx, u8 cipher[], u32 len);
static void genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
				const u8 plain[]);
static void genrand_block(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[]);
static void genrand_keystream_block_first(ECRYPT_ctx * ctx, u8 cipher[]);
static void genrand_keystream_block(ECRYPT_ctx * ctx, u8 cipher[]);
static void array_fill(u32 to_fill[], s32 gen_length, s32 init_length);
static void array_fill_withkey(u32 to_fill[], s32 gen_length, s32 init_length,
			u32 key_extended[]);

static void genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len)
{
    u32 accum;
    int kk;
    u32 *to_fill, *key_extended;
    u32 x, y, lung;
    s32 j3, s;
	
    s = ctx->ivsize;
    to_fill = ctx->booter_state + 2*s;
    key_extended = ctx->key_extended + 2*s;
    
    accum = ctx->accum;
    j3 = s;
    lung = to_fill[1] | 1;
    for (kk=0; len>0; kk++, len--) {
	y = to_fill[j3-2];
	y ^= key_extended[kk];
	x = to_fill[kk];
	to_fill[kk] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16)) + (y << 1); /* reading key */
	
	accum *= to_fill[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    /* ctx->f = 1; */
}

static void genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len)
{
    u32 accum;
    int kk;
    u32 y;
    u32 *pmt;

    pmt = ctx->pmt;
    accum = ctx->accum;
    y = pmt[N];
    for (kk = 0; (kk < N - M) && (len > 0); kk++, len--) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M];
	pmt[kk] ^= y ^ (y >> S);
	accum *= pmt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    if (len <= 0) {
	/* pmt[N] = y; */
	/* ctx->accum = accum; */
	return;
    }
    for (; (kk < N) && (len > 0); kk++, len--) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M-N];
	pmt[kk] ^= y ^ (y >> S);
	
	accum *= pmt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    /* pmt[N] = y; */
    /* ctx->accum = accum; */
}

static void genrand_keystream_bytes_first(ECRYPT_ctx * ctx, u8 cipher[], 
					  u32 len)
{
    u32 y, accum;
    int kk;
    
    u32 *to_fill, *key_extended;
    u32 x, lung;
    s32 j3, s;
    
    s = ctx->ivsize;
    to_fill = ctx->booter_state + 2*s;
    key_extended = ctx->key_extended + 2*s;

    accum = ctx->accum;
    j3 = s;
    lung = to_fill[1] | 1;
    for (kk=0; len>0; kk++, len--) {
	y = to_fill[j3-2];
	y ^= key_extended[kk];
	x = to_fill[kk];
	to_fill[kk] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16)) + (y << 1); /* reading key */
	
	accum *= to_fill[kk] | 0x1;
	*cipher++ = (u8)(accum >> 24);
    }
}

static void genrand_keystream_bytes(ECRYPT_ctx * ctx, u8 cipher[], u32 len)
{
    u32 y, accum;
    u32 *pmt;
    int kk;
    
    pmt = ctx->pmt;
    accum = ctx->accum;
    y = pmt[N];
    for (kk = 0; (kk < N - M) && (len > 0); kk++, len--) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M];
	pmt[kk] ^= y ^ (y >> S);
	  
	accum *= pmt[kk] | 0x1;
	cipher[kk] = (u8)(accum >> 24);
    }
    if (len <= 0) {
	/* pmt[N] = y; */
	/* ctx->accum = accum; */
	return;
    }
    for (; (kk < N) && (len > 0); kk++, len--) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M-N];
	pmt[kk] ^= y ^ (y >> S);
	accum *= pmt[kk] | 0x1;
	cipher[kk] = (u8)(accum >> 24);
    }
    /* pmt[N] = y; */
    /* ctx->accum = accum; */
}

static void genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
				const u8 plain[])
{
    u32 y, accum;
    int kk;
    u32 *to_fill, *key_extended;
    u32 x, lung;
    s32 j3, s;
	
    s = ctx->ivsize;
    to_fill = ctx->booter_state + 2*s;
    key_extended = ctx->key_extended + 2*s;
    
    accum = ctx->accum;
    j3 = s;
    lung = to_fill[1] | 1;
    for (kk=0; kk<N; kk++) {
	y = to_fill[j3-2];
	y ^= key_extended[kk];
	x = to_fill[kk];
	to_fill[kk] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16))  + (y << 1); /* reading key */
	
	accum *= to_fill[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    ctx->pmt = to_fill;
    ctx->pmt[N] = U32C(0x4d484e53);
    ctx->accum = accum;
}

static void genrand_block(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[])
{
    u32 y, accum;
    u32 *pmt;
    int kk;

    pmt = ctx->pmt;
    accum = ctx->accum;
    y = pmt[N];
    for (kk=0; kk<(N-M); kk++) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M];
	pmt[kk] ^= y ^ (y >> S);
	
	accum *= pmt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    for (; kk < N ; kk++) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M-N];
	pmt[kk] ^= y ^ (y >> S);
	
	accum *= pmt[kk] | 0x1;
	cipher[kk] = plain[kk] ^ (u8)(accum >> 24);
    }
    pmt[N] = y;
    ctx->accum = accum;
}

static void genrand_keystream_block_first(ECRYPT_ctx * ctx, u8 cipher[])
{
    u32 y, accum;
    int kk;
    u32 *to_fill, *key_extended;
    u32 x, lung;
    s32 j3, s;
	
    s = ctx->ivsize;
    to_fill = ctx->booter_state + 2*s;
    key_extended = ctx->key_extended + 2*s;

    accum = ctx->accum;
    j3 = s;
    lung = to_fill[1] | 1;
    for (kk=0; kk<N; kk++) {
	y = to_fill[j3-2];
	y ^= key_extended[kk];
	x = to_fill[kk];
	to_fill[kk] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16)) + (y << 1); /* reading key */

	accum *= to_fill[kk] | 0x1;
	cipher[kk] = (u8)(accum >> 24);
    }
    ctx->pmt = to_fill;
    ctx->pmt[N] = U32C(0x4d484e53);
    ctx->accum = accum;
}

static void genrand_keystream_block(ECRYPT_ctx * ctx, u8 cipher[])
{
    u32 y, accum;
    u32 *pmt;
    int kk;

    pmt = ctx->pmt;
    accum = ctx->accum;
    y = pmt[N];
    for (kk=0; kk<(N-M); kk++) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M];
	pmt[kk] ^= y ^ (y >> S);

	accum *= pmt[kk] | 0x1;
	cipher[kk] = (u8)(accum >> 24);
    }
    for (; kk < N ; kk++) {
	y ^= pmt[kk] << R;
	y ^= pmt[kk+M-N];
	pmt[kk] ^= y ^ (y >> S);
	
	accum = pmt[kk] | 0x1;
	cipher[kk] = (u8)(accum >> 24);
    }
    pmt[N] = y;
    ctx->accum = accum;
}

/********* END of PMT **********/


void ECRYPT_init(void)
{
    /* do nothing */
}

static void array_fill(u32 to_fill[], s32 gen_length, s32 init_length) 
{
    s32 i, j2, j3;
    u32 lung;

    j2=init_length - 2;
    j3=init_length;
    lung = to_fill[1] | 1;
    for (i=0; i< gen_length; i++) {
	u32 x, y;
	y = to_fill[j2++];
	y ^= j2;
	x = to_fill[i];
	to_fill[i] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16)) + (y << 1); /* reading "key"*/
    }
}

/* Key size in bits. */
/* IV size in bits. */
void ECRYPT_keysetup(ECRYPT_ctx * ctx, const u8 * key, u32 keysize,
		     u32 ivsize)
{
    s32 i;
    u32 expander_state[2*N+4*(ECRYPT_MAXKEYSIZE/32)+2*(ECRYPT_MAXIVSIZE/32)];

    memset(expander_state, 0, sizeof(expander_state));
    memset(ctx, 0, sizeof(ctx));
    ctx->keysize = keysize / 32;
    ctx->ivsize = ivsize / 32;

    for (i = 0; i < ctx->keysize; i++) {
	expander_state[i] = U8TO32_LITTLE(key);
	key += 4;
    }
    
    array_fill(expander_state, 2*N + 4*(ivsize/32) + keysize/32, keysize/32);
    memcpy(ctx->key_extended, expander_state + 2*(keysize/32), 
	   sizeof(u32)*(2*N + 4*(ivsize/32)));
}

static void array_fill_withkey(u32 to_fill[], s32 gen_length, s32 init_length,
			u32 key_extended[]) 
{
    s32 i, j3;
    u32 lung;
    j3=init_length;
    lung = to_fill[1] | 1;
    for (i=0; i< gen_length; i++) {
	u32 x, y;
	y = to_fill[j3-2];
	y ^= key_extended[i];
	x = to_fill[i];
	to_fill[i] ^= y;
	lung *= (y | 1);
	x ^= lung;
	to_fill[j3++] ^= (x ^ (~x >> 16)) + (y << 1); /* reading "key"*/
    }
}

void ECRYPT_ivsetup(ECRYPT_ctx * ctx, const u8 * iv)
{
    s32 i, s;
 
    s = ctx->ivsize;
    for (i = 0; i < s; i++) {
	ctx->booter_state[i] = U8TO32_LITTLE(iv);
	iv += 4;
    }
    
    memcpy(ctx->booter_state + s, ctx->key_extended+N+2*s, sizeof(u32)*(N+2*s));
    array_fill_withkey(ctx->booter_state, 2*s, s, ctx->key_extended);
    ctx->accum = ctx->booter_state[s-1] | 1;
    ctx->f = 0;
}

/* Message length in bytes. */
void ECRYPT_encrypt_bytes(ECRYPT_ctx * ctx,
			  const u8 * plaintext,
			  u8 * ciphertext, u32 msglen)
{
    if (!ctx->f) {
	if (msglen >= ECRYPT_BLOCKLENGTH) {
	    	genrand_block_first(ctx, ciphertext, plaintext);
		ciphertext += ECRYPT_BLOCKLENGTH;
		plaintext += ECRYPT_BLOCKLENGTH;
		msglen -= ECRYPT_BLOCKLENGTH;
		ctx->f = 1;
	} else {
	    genrand_bytes_first(ctx, ciphertext, plaintext, msglen);
	    return;
	}
    }
    while (msglen >= ECRYPT_BLOCKLENGTH) {
	genrand_block(ctx, ciphertext, plaintext);
	ciphertext += ECRYPT_BLOCKLENGTH;
	plaintext += ECRYPT_BLOCKLENGTH;
	msglen -= ECRYPT_BLOCKLENGTH;
    }
    if (msglen != 0) {
	genrand_bytes(ctx, ciphertext, plaintext, msglen);
    }
}

/* Message length in bytes. */
void ECRYPT_decrypt_bytes(ECRYPT_ctx * ctx,
			  const u8 * ciphertext,
			  u8 * plaintext, u32 msglen)
{
    ECRYPT_encrypt_bytes(ctx, ciphertext, plaintext, msglen);
}

/* Message length in blocks. */
void ECRYPT_encrypt_blocks(ECRYPT_ctx * ctx,
			   const u8 * plaintext,
			   u8 * ciphertext, u32 blocks)
{
    s32 i;

    if (!ctx->f) {
	genrand_block_first(ctx, ciphertext, plaintext);
	ciphertext += ECRYPT_BLOCKLENGTH;
	plaintext += ECRYPT_BLOCKLENGTH;
	blocks--;
	ctx->f = 1;
    }
    for (i = 0; i < blocks; i++) {
	genrand_block(ctx, ciphertext, plaintext);
	ciphertext += ECRYPT_BLOCKLENGTH;
	plaintext += ECRYPT_BLOCKLENGTH;
    }
}

/* Message length in blocks. */
void ECRYPT_decrypt_blocks(ECRYPT_ctx * ctx,
			   const u8 * ciphertext,
			   u8 * plaintext, u32 blocks)
{
    ECRYPT_encrypt_blocks(ctx, ciphertext, plaintext, blocks);
}


/* keystream length in bytes */
void ECRYPT_keystream_bytes(ECRYPT_ctx * ctx, u8 * keystream, u32 msglen)
{
    if (!ctx->f) {
	if (msglen >= ECRYPT_BLOCKLENGTH) {
	    	genrand_keystream_block_first(ctx, keystream);
		keystream += ECRYPT_BLOCKLENGTH;
		msglen -= ECRYPT_BLOCKLENGTH;
		ctx->f = 1;
	} else {
	    genrand_keystream_bytes_first(ctx, keystream, msglen);
	    return;
	}
    }
    while (msglen >= ECRYPT_BLOCKLENGTH) {
	genrand_keystream_block(ctx, keystream);
	keystream += ECRYPT_BLOCKLENGTH;
	msglen -= ECRYPT_BLOCKLENGTH;
    }
    if (msglen != 0) {
	genrand_keystream_bytes(ctx, keystream, msglen);
    }
}

/* Keystream length in blocks. */
void ECRYPT_keystream_blocks(ECRYPT_ctx * ctx, u8 * keystream, u32 blocks)
{
    s32 i;

    if (!ctx->f) {
	genrand_keystream_block_first(ctx, keystream);
	keystream += ECRYPT_BLOCKLENGTH;
	blocks--;
	ctx->f = 1;
    }
    for (i = 0; i < blocks; i++) {
	genrand_keystream_block(ctx, keystream);
	keystream += ECRYPT_BLOCKLENGTH;
    }
}
