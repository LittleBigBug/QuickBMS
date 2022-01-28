#include "ecrypt-sync.h"
#include "unrollmain.h"
#include "unrolliv.h"

#define MAGSIZE 128
#define MIXER 0x5656565656565656

void rotate(u64 array[])
{
  u64 temp;
  temp = array[0];
  array[0]=array[1];
  array[1]=array[2];
  array[2]=array[3];
  array[3]=temp;
}

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
		
  for (k = 0; k < 4; k++)
    ctx->key[k] = U8TO64_LITTLE(key + k * 8);
  
} 

void ECRYPT_ivsetup(ECRYPT_ctx* ctx,const u8* iv)
{
  int i;
  
  ctx->cell[0] = U8TO64_LITTLE(iv);

  for (i = 1; i < MAGSIZE; i++)
    ctx->cell[i] = ctx->key[(i - 1) % 4];

  ctx->carry = ctx->cell[MAGSIZE - 1];
  
  UNROLLIV
  
  ctx->pos = 0;
  ctx->maskpos = 0;
    
}
   
void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)   
{
  u32 i, j, m, n;
  u8 celltempbytes[8], masktempbytes[8];
  
  i = ctx->pos;
  m = ctx->maskpos;
  
  for (j = 0; j < 4; j++)
    ctx->mask[j] = ctx->key[j];
  
  for(j = 0; j < m; j++)
    rotate(ctx->mask);
   
  for(; msglen >= 1024; msglen -= 1024) 
  {    
    UNROLLMAIN
    rotate(ctx->mask);
    m++;
  }
   
  for (; msglen >= 8; msglen -= 8, input += 8, output += 8, i++)
  {
    ctx->carry ^= ctx->cell[(i + 1) % MAGSIZE];
    if(ctx->cell[(i + 2) % MAGSIZE] > ctx->cell[(i + 3) % MAGSIZE])
      ctx->carry = ~ctx->carry;
    ctx->cell[i % MAGSIZE] ^= ctx->carry;
    ((u64*)output)[0] = ((u64*)input)[0] ^ ctx->cell[i % MAGSIZE] ^ ctx->mask[i % 4];
  }
  
  if (msglen > 0)
  {
    
    ctx->carry ^= ctx->cell[1]; 
    if(ctx->cell[2] > ctx->cell[3]) 
      ctx->carry = (~ctx->carry); 
    ctx->cell[0] ^= ctx->carry; 
        
    U64TO8_LITTLE(celltempbytes, ctx->cell[0]);
    U64TO8_LITTLE(masktempbytes, ctx->mask[0]);
    
    for (n = 0; n < msglen; n ++)
      output[n] = input[n] ^ celltempbytes[n] ^ masktempbytes[n];
  }
  ctx->pos = i;
  ctx->maskpos = m;
}
