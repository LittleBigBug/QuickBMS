/* CryptMT Stream Cipher, Relying Mersenne Twister */
/* By Hagita-Matsumoto-Nishimura-Saito */
/* MT included */
/* 2005/04/16 */

#include <stdio.h>

#include "ecrypt-config.h"
#include "ecrypt-machine.h"
#include "ecrypt-portable.h"

#include "ecrypt-sync.h"


/**********Start of MT ************/
/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A U32C(0x9908B0DF)   /* constant vector a */
#define UPPER_MASK U32C(0x80000000) /* most significant w-r bits */
#define LOWER_MASK U32C(0x7FFFFFFF) /* least significant r bits */

/* initializes mt[N] with a seed */
void init_genrand(ECRYPT_ctx* ctx, u32 s)
{
    ctx->mt[0]= s & U32C(0xFFFFFFFF);
    for (ctx->mti=1; ctx->mti<N; ctx->mti++) {
        ctx->mt[ctx->mti] = 
	    (U32C(1812433253) * (ctx->mt[ctx->mti-1] ^ (ctx->mt[ctx->mti-1] >> 30)) + ctx->mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(ECRYPT_ctx* ctx, u32 init_key[], int key_length)
{
    int i, j, k;
    init_genrand(ctx, U32C(19650218));
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        ctx->mt[i] = (ctx->mt[i] ^ ((ctx->mt[i-1] ^ (ctx->mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        i++; j++;
        if (i>=N) { ctx->mt[0] = ctx->mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        ctx->mt[i] = (ctx->mt[i] ^ ((ctx->mt[i-1] ^ (ctx->mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        i++;
        if (i>=N) { ctx->mt[0] = ctx->mt[N-1]; i=1; }
    }

    ctx->mt[0] = U32C(0x80000000); /* MSB is 1; assuring non-zero initial array */ 
}


/* generates whole array of random numbers in [0,0xffffffff]-interval */
void genrand_whole_array(ECRYPT_ctx* ctx)
{
    u32 y;
    static u32 mag01[2]={U32C(0x0), MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    int kk;
    
    for (kk=0;kk<N-M;kk++) {
      y = (ctx->mt[kk]&UPPER_MASK)|(ctx->mt[kk+1]&LOWER_MASK);
      ctx->mt[kk] = ctx->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (;kk<N-1;kk++) {
      y = (ctx->mt[kk]&UPPER_MASK)|(ctx->mt[kk+1]&LOWER_MASK);
      ctx->mt[kk] = ctx->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (ctx->mt[N-1]&UPPER_MASK)|(ctx->mt[0]&LOWER_MASK);
    ctx->mt[N-1] = ctx->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];
    
    ctx->mti = 0;
    return ;
}


/* generate 32-bit random integer */
u32 genrand_int32(ECRYPT_ctx* ctx)
{
    if (ctx->mti >= N) genrand_whole_array(ctx);
    return ctx->mt[ctx->mti++];  /* No Tempering */
}
/********* END of MT **********/

/* This is a stream cipher. The algorithm is as follows. */
/* Generate 32 bit nonsecure random numbers by MT. */
/* Multiply three consequtive words, and use only */
/* the most significant 8 bits. */ 

void ECRYPT_init()
{
    /* do nothing */
}

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize)                /* IV size in bits. */ 
{
    s32 i;

    ctx->keysize = keysize;
    ctx->ivsize = ivsize;
    
    for (i=0; i<keysize/8; i++)
	ctx->key[i] = key[i];

    ctx->mti = N + 1; /* mti==N+1 means mt[N] is not initialized */
}


void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
    s32 i,j,k,t,s;
    u32 x, init_array[(ECRYPT_MAXKEYSIZE+ECRYPT_MAXIVSIZE)/32];

    for (i=0; i<ctx->ivsize/8; i++)
	ctx->iv[i] = iv[i];

    j = 0;
    t = ctx->keysize/32;
    for (i=0; i<t; i++) {
	x = (u32)ctx->key[j++];
	x |= ((u32)ctx->key[j++]) << 8;
	x |= ((u32)ctx->key[j++]) << 16;
	x |= ((u32)ctx->key[j++]) << 24;
	init_array[i] = x;
    }
    if ( ctx->keysize % 32 != 0 ) {
	x = 0;
	k = (ctx->keysize % 32)/8;
	for (i=0; i<k; i++) {
	    x |= ((u32)ctx->key[j++]) << (8*k);
	}
	init_array[t++] = x;
    }

    j = 0;
    s = ctx->ivsize/32;
    for (i=0; i<s; i++) {
	x = (u32)ctx->iv[j++];
	x |= ((u32)ctx->iv[j++]) << 8;
	x |= ((u32)ctx->iv[j++]) << 16;
	x |= ((u32)ctx->iv[j++]) << 24;
	init_array[t+i] = x;
    }
    if ( ctx->ivsize % 32 != 0 ) {
	x = 0;
	k = (ctx->ivsize % 32)/8;
	for (i=0; i<k; i++) {
	    x |= ((u32)ctx->iv[j++]) << (8*k);
	}
	init_array[t+(s++)] = x;
    }
    init_by_array(ctx, init_array, t+s);

    ctx->accum=1;
    for (i=0; i<64; i++) { /* idling 64 times */
	ctx->accum *= (genrand_int32(ctx) | 0x1);
    }
}

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen)                /* Message length in bytes. */ 
{
    s32 i;
    for (i=0; i< msglen; i++) {
      ctx->accum *= (genrand_int32(ctx) | 0x1);
      ciphertext[i] = plaintext[i] ^ (ctx->accum >> 24);
    }
}

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen)                /* Message length in bytes. */ 
{
    s32 i;
    for (i=0; i< msglen; i++) {
      ctx->accum *= (genrand_int32(ctx) | 0x1);
      plaintext[i] = ciphertext[i] ^ (ctx->accum >> 24);
    }
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 msglen)
{
    s32 i;
    for (i=0; i< msglen; i++) {
      ctx->accum *= (genrand_int32(ctx) | 0x1);
      keystream[i] = (ctx->accum >> 24);
    }
}

#ifndef ECRYPT_API

int main(void) 
{
    int i;
    ECRYPT_ctx x;
    u8 plaintext[128], plaintext2[128], ciphertext[128];

    for (i=0; i<128; i++){
      plaintext[i]=0;
    }

    ECRYPT_keysetup(&x, "1234567812345678", 128, 128);
    ECRYPT_ivsetup(&x,  "8765432187654321");
    ECRYPT_encrypt_bytes(&x, plaintext, ciphertext, 128);
    for (i=0; i<16; i++)
	printf("%2x ",  ciphertext[i]);
    printf("\n");

    ECRYPT_ivsetup(&x,  "8765432187654321");
    ECRYPT_decrypt_bytes(&x, ciphertext, plaintext2, 128);
    for (i=0; i<16; i++)
	printf("%2x ",  plaintext2[i]);
    printf("\n");

    return 0;
}

#endif
