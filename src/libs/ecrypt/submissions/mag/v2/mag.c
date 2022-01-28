#include "ecrypt-sync.h"

#define MAGSIZE 128
#define ARRAYSIZE 64
#define MIXER 0x65

void ECRYPT_init(void)
{
 
}

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,               
  u32 ivsize)
{
  int k;
		
  for (k = 0; k < 16; k++)
    ctx->key[k] = key[k]; /* key is 16 Bytes */
} 

void ECRYPT_ivsetup(ECRYPT_ctx* ctx,const u8* iv)
{
  int i;
  u8 setupiv[MAGSIZE];
  u8 setupcarry;
  
  for (i = 0; i < 4; i++)
    ctx->key[i + 16] = iv[i]; /* iv is 4 Bytes */
  
  for (i = 0; i < MAGSIZE; i++)
    setupiv[i] = ctx->key[i % 20];

  setupcarry = setupiv[MAGSIZE - 1];
  
  for (i = 0; i < 512; i++)
  {
    setupcarry ^= setupiv[(i + 1) % MAGSIZE];
    if(setupiv[(i + 2) % MAGSIZE] > setupiv[(i + 3) % MAGSIZE])
      setupcarry = ~setupcarry;
    setupiv[i % MAGSIZE] ^= setupcarry;
    setupcarry += MIXER;	  
  }
  
  for (i = 0; i < ARRAYSIZE; i++)
  {
    ctx->cellA[i] = setupiv[i];
    ctx->cellB[i] = setupiv[i + ARRAYSIZE];
  }
    
  ctx->carryA = setupiv[MAGSIZE - 1];
  ctx->carryB = setupiv[MAGSIZE - 65];
     
  ctx->pos = 0;
     
}
   
void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)   
{
  u32 i;
  u32 pos = ctx->pos;
  
  for(i = 0; i < msglen; i++, pos = (pos + 1) % ARRAYSIZE)
  {
    ctx->carryA ^= ctx->cellA[(pos + 1) % ARRAYSIZE];
    if(ctx->cellA[(pos + 2) % ARRAYSIZE] > ctx->cellA[(pos + 3) % ARRAYSIZE])
      ctx->carryA = ~ctx->carryA;
    ctx->cellA[pos % ARRAYSIZE] ^= ctx->carryA;
    
    ctx->carryB ^= ctx->cellB[(pos + 1) % ARRAYSIZE];
    if(ctx->cellB[(pos + 2) % ARRAYSIZE] > ctx->cellB[(pos + 3) % ARRAYSIZE])
      ctx->carryB = ~ctx->carryB;
    ctx->cellB[pos % ARRAYSIZE] ^= ctx->carryB;
    
    output[i] = input[i] ^ ctx->cellA[pos % ARRAYSIZE] ^ ctx->cellB[pos % ARRAYSIZE];
  }
  
  ctx->pos = pos;
  
}
