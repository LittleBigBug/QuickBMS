#include "ecrypt-sync.h"

#define MAGSIZE 127
#define MIXER 286331153

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
    ctx->key[k] = U8TO32_LITTLE(key + k * 4);
} 

void ECRYPT_ivsetup(ECRYPT_ctx* ctx,const u8* iv)
{
  int i;
  u32 pos;
  u32 carry;

  ctx->cell[0] = U8TO32_LITTLE(iv);

  for (i = 1; i < MAGSIZE; i++)
    ctx->cell[i] = ctx->key[(i - 1) % 4];

  pos = 0;
  carry = ctx->cell[MAGSIZE - 1];

  for(i = 0; i < 192; i++, pos = (pos + 1) % MAGSIZE)
    {
      u32 out = ctx->cell[pos];

      const u32 in = ctx->cell[(pos + 1) % MAGSIZE];
      const u32 a  = ctx->cell[(pos + 2) % MAGSIZE];
      const u32 b  = ctx->cell[(pos + 3) % MAGSIZE];

      if (a > b)
	carry ^= in;
      else
	carry ^= ~in;

      ctx->cell[pos] = (out ^= carry);
      carry += MIXER;
    }

  ctx->pos = 0;
  ctx->carry = carry;
}
   
void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)   
{
  int i;
  u32 pos = ctx->pos;
  u32 carry = ctx->carry;

  for(i = 0; i < msglen / 2; i++, pos = (pos + 1) % MAGSIZE)
    {
      u32 out = ctx->cell[pos];
      
      const u32 in = ctx->cell[(pos + 1) % MAGSIZE];
      const u32 a  = ctx->cell[(pos + 2) % MAGSIZE];
      const u32 b  = ctx->cell[(pos + 3) % MAGSIZE];

      const int s = ((pos * 2    ) * 8) % 32;
      const int t = ((pos * 2 + 1) * 8) % 32;

      if (a > b)
	carry ^= in;
      else
	carry ^= ~in;

      ctx->cell[pos] = (out ^= carry);

      output[i * 2    ] = U8V(out >> s) ^ input[i * 2    ];
      output[i * 2 + 1] = U8V(out >> t) ^ input[i * 2 + 1]; 
    }

  if ((i *= 2) < msglen)
    {
      u32 out = ctx->cell[pos];
      
      const u32 in = ctx->cell[(pos + 1) % MAGSIZE];
      const u32 a  = ctx->cell[(pos + 2) % MAGSIZE];
      const u32 b  = ctx->cell[(pos + 3) % MAGSIZE];

      if (a > b)
	carry ^= in;
      else
	carry ^= ~in;

      output[i] = U8V((out ^ carry) >> (((pos * 2) * 8) % 32)) ^ input[i];

      return;
    }

  ctx->pos = pos;
  ctx->carry = carry;
}
