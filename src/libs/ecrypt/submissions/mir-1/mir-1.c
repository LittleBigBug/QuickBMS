#include "mir1.h"


/* ------------------------------------------------------------------------- */
/* Functions Implementation */
/* ------------------------------------------------------------------------- */
void ECRYPT_init()
{
}


/* ------------------------------------------------------------------------- */
void ECRYPT_keysetup(
           ECRYPT_ctx* ctx, 
           const u8* key, 
           u32 keysize,                /* Key size in bits. */ 
           u32 ivsize)                 /* IV size in bits. */ 
{ 
  int i,j;
  for(i=0; i<256; ++i)
    for(ctx->S[i]=i, j=0; j<16; ++j)
      ctx->S[i]=SR[ctx->S[i]^key[j]];

  for(i=0; i<16; ++i)
    ctx->key[i] = key[i];
}


/* ------------------------------------------------------------------------- */
void ECRYPT_ivsetup(
           ECRYPT_ctx* ctx, 
           const u8* IV)
{ 
  int i;

  ctx->a.v  = ctx->x1.v = U8TO64_LITTLE(ctx->key);
  ctx->b.v  = ctx->x3.v = U8TO64_LITTLE(ctx->key + 8);
  ctx->x0.v = C0;
  ctx->x2.v = C1;
  for(i=0; i<8; ++i) { Mir1_STATE_UPDATE }

  ctx->x0.b.b4 ^= ctx->S[IV[0]] ^ ctx->S[IV[1]] ^ ctx->S[IV[2]];
  ctx->x1.b.b4 ^= ctx->S[IV[0]] ^ ctx->S[IV[3]] ^ ctx->S[IV[4]];
  ctx->x2.b.b4 ^= ctx->S[IV[2]] ^ ctx->S[IV[5]] ^ ctx->S[IV[7]];
  ctx->x3.b.b4 ^= ctx->S[IV[3]] ^ ctx->S[IV[6]] ^ ctx->S[IV[7]];
  ctx->x0.b.b0 ^= ctx->S[IV[3]] ^ ctx->S[IV[5]];
  ctx->x1.b.b0 ^= ctx->S[IV[7]] ^ ctx->S[IV[6]];
  ctx->x2.b.b0 ^= ctx->S[IV[0]] ^ ctx->S[IV[1]];
  ctx->x3.b.b0 ^= ctx->S[IV[2]] ^ ctx->S[IV[4]];
  ctx->a.b.b0  ^= ctx->S[IV[0]] ^ ctx->S[IV[5]] ^ ctx->S[IV[6]];
  ctx->a.b.b4  ^= ctx->S[IV[1]] ^ ctx->S[IV[3]] ^ ctx->S[IV[5]];
  ctx->b.b.b0  ^= ctx->S[IV[1]] ^ ctx->S[IV[4]] ^ ctx->S[IV[7]];
  ctx->b.b.b4  ^= ctx->S[IV[2]] ^ ctx->S[IV[4]] ^ ctx->S[IV[6]];
  { Mir1_STATE_UPDATE }
  { Mir1_STATE_UPDATE }
}


/* ------------------------------------------------------------------------- */
// NOTE: msglen MUST BE divisible by 8
void ECRYPT_encrypt_bytes(
           ECRYPT_ctx* ctx, 
           const u8* plaintext, 
           u8* ciphertext, 
           u32 msglen)                /* Message length in bytes. */ 
{ 
  int i, k=msglen & ~7; /* divisible by 8 */
  
  for(i=0; i<k; i += 8)
    { Mir1_STATE_UPDATE
      U64TO8_LITTLE(ciphertext + i, U8TO64_LITTLE(plaintext + i) ^ ctx->b.v); 
    }

  if (k < msglen)
    { Mir1_STATE_UPDATE
	for (i=k; i<msglen; ++i)
	  {
	    ciphertext[i] = plaintext[i] ^ U8V(ctx->b.v);
	    ctx->b.v >>= 8;
	  }
    }
}


/* ------------------------------------------------------------------------- */
void ECRYPT_decrypt_bytes(
           ECRYPT_ctx* ctx, 
           const u8* ciphertext, 
           u8* plaintext, 
           u32 msglen)                /* Message length in bytes. */ 

{ 
  int i, k=msglen & ~7; /* divisible by 8 */
  
  for(i=0; i<k; i += 8)
    { Mir1_STATE_UPDATE
      U64TO8_LITTLE(plaintext + i, U8TO64_LITTLE(ciphertext + i) ^ ctx->b.v); 
    }

  if (k < msglen)
    { Mir1_STATE_UPDATE
	for (i=k; i<msglen; ++i)
	  {
	    plaintext[i] = ciphertext[i] ^ U8V(ctx->b.v);
	    ctx->b.v >>= 8;
	  }
    }
}

/* ------------------------------------------------------------------------- */
void ECRYPT_keystream_bytes(
           ECRYPT_ctx* ctx,
           u8* keystream,
           u32 length)                /* Length of keystream in bytes. */
{ 
  int i, k=length & ~7;  /* divisible by 8 */

  for(i=0; i<k; i += 8)
    { Mir1_STATE_UPDATE
      U64TO8_LITTLE(keystream + i, ctx->b.v); 
    }

  if (k < length)
    { Mir1_STATE_UPDATE
	for (i=k; i<length; ++i)
	  {
	    keystream[i] = U8V(ctx->b.v);
	    ctx->b.v >>= 8;
	  }
    }
}

/* ------------------------------------------------------------------------- */

