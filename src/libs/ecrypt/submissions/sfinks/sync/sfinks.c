/* sfinks.c */

/*
 * This is a reference implementation of the Stream cipher SFINKS
 * 
 * SFINKS is a HW-oriented stream cipher. This C implementation is only 
 * intended for simulation and is not optimized in any way. 
 * 
 * Author: Joseph Lano, "joe dot lano at gmail dot com"
 */

#include "ecrypt-sync.h"

/* Two S-boxes, the first is only temporary, the second will contain the 16-bit inversion S-Box*/
u16 POWER[65536];
u16 INV[65536];


/* 
 * lfsr_clock
 * This function clocks the LFSR linearly
 * with a weight 17 primitive feedback polynomial
 */
void lfsr_clock(ECRYPT_ctx* ctx)
{
  u32 newbit;
  u32 i;
  newbit = ctx->LFSR[212]^ctx->LFSR[194]^ctx->LFSR[192]^ctx->LFSR[187]^
    ctx->LFSR[163]^ctx->LFSR[151]^ctx->LFSR[125]^ctx->LFSR[115]^
    ctx->LFSR[107]^ctx->LFSR[85]^ctx->LFSR[66]^ctx->LFSR[64]^
    ctx->LFSR[52]^ctx->LFSR[48]^ctx->LFSR[14]^ctx->LFSR[0];
  for (i=0;i<255;i++) ctx->LFSR[i]=ctx->LFSR[i+1];
  ctx->LFSR[255]=newbit;
}

/* sbox_clock
 * This function calculates the next output of the inversion function. 
 * It updates the virtual pipeline, corresponding to the actual 
 * pipeline in the hardware implementation
 */
void sbox_clock(ECRYPT_ctx* ctx)
{
  u32 i;
  u16 input;
  for (i=0;i<6;i++) ctx->pipe[i]=ctx->pipe[i+1];
  for (i=0;i<6;i++) ctx->linmask[i]=ctx->linmask[i+1];
  input=(ctx->LFSR[255]<<15)^(ctx->LFSR[244]<<14)^(ctx->LFSR[227]<<13)^(ctx->LFSR[193]<<12)^
    (ctx->LFSR[161]<<11)^(ctx->LFSR[134]<<10)^(ctx->LFSR[105]<<9)^(ctx->LFSR[98]<<8)^
    (ctx->LFSR[74]<<7)^(ctx->LFSR[58]<<6)^(ctx->LFSR[44]<<5)^(ctx->LFSR[21]<<4)^
    (ctx->LFSR[19]<<3)^(ctx->LFSR[9]<<2)^(ctx->LFSR[6]<<1)^(ctx->LFSR[1]);
  ctx->pipe[6]=INV[input];
  ctx->linmask[6]=ctx->LFSR[0];
}

/* 
 * GFexp
 * Calculates POWER[i] that defines 2^i in the field GF(2^16), while using the 
 * corresponding primitive polynomial g(x)=X^16+X^5+X^3+X^2+1 
 */
void GFexp()
{
  u32 g = 0x1002D; /* The primitive polynomial X^16+X^5+X^3+X^2+1 */
  const long n = (long)1 << 16;
  u32 x = 1;
  u32 i;
  for (i =0;i<n-1;i++)
  {
    POWER[i] = x;
    x <<= 1;
    if (x & n) x ^= g;
  }
  POWER[n - 1] = 0;
}

/* 
 * GFpower
 * Calculate sbox[i] that defines i^-1 in the field GF(2^16), while using the 
 * corresponding primitive polynomial g(x)=X^16+X^5+X^3+X^2+1 
 */
void GFpower()
{
  const long n = 1 << 16;
  u32 i;
  INV[0] = 0;
  for (i=0;i<n-1;i++)
  {
    INV[(long)POWER[i]] = POWER[(n - 1 - i) % (n - 1)];
  }
}

/*
 * ECRYPT_init
 * Initializes the (16,16) inversion S-box
 */
void ECRYPT_init()
{
  GFexp();
  GFpower();
}

/* 
 * ECRYPT_keysetup
 * copies the key into the state as an array of 80 bits
*/
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize)                /* IV size in bits. */ 
{
  u32 i,j,counter;
  u8 keybyte;
  ctx->keysize=keysize;
  ctx->ivsize=ivsize;
  counter=0;
  for (i=0;i<10;i++)
  {
    keybyte=key[i];
    for (j=0;j<8;j++)
    {
      ctx->key[counter]=keybyte&1;
      counter++;
      keybyte>>=1;
    }    
  }
}

/*
 * ECRYPT_ivsetup
 * - Put LFSR and pipeline to zero
 * - Put key and iv into LFSR
 * - Perform 128 nonlinear resync clocks
*/
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
  u32 i,j,counter;
  u8 ivbyte;
  u16 feedback;
  counter=0;

  /*set LFSR and pipelines to zero*/
  for (i=0;i<256;i++) ctx->LFSR[i]=0;
  for (i=0;i<7;i++) ctx->pipe[i]=0;
  for (i=0;i<7;i++) ctx->linmask[i]=0;

  /* load key into LFSR*/
  for (i=0;i<80;i++) ctx->LFSR[96+i]=ctx->key[i];

  /* load iv into LFSR*/
  for (i=0;i<10;i++)
  {
    ivbyte=iv[i];
    for (j=0;j<8;j++)
    {
      ctx->LFSR[176+counter]=ivbyte&1;
      counter++;
      ivbyte>>=1;
    }
  }

  /* set LFSR bit 95 to one */
  ctx->LFSR[95]=1;

  /*perform 128 nonlinear resync clocks*/
  for (i=0;i<128;i++)
  {
    lfsr_clock(ctx);

    feedback=ctx->pipe[0];
    ctx->LFSR[80]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[17]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[66]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[179]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[52]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[213]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[118]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[247]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[232]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[41]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[154]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[11]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[204]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[173]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[142]^=(feedback&1); 
    feedback>>=1;
    ctx->LFSR[111]^=(feedback&1);

    sbox_clock(ctx);
    
  }
}

/*
 * ECRYPT_keystream_bytes
 * Generate 'length' bytes of keystream
 */
void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)                /* Length of keystream in bytes. */
{
  u32 i,j;
  for (i=0;i<length;i++) 
  {
    for (j=0;j<8;j++) 
    {
      lfsr_clock(ctx);
      sbox_clock(ctx);
      keystream[i]=(keystream[i]<<1)^(ctx->pipe[0]&1)^ctx->linmask[0];
    }
  }
}


/*
 * ECRYPT_process_bytes
 * Generate 'msglen' bytes of ciphertext from plaintext, or vice-versa
 */
void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)                /* Message length in bytes. */
{
  u32 i,j;
  u8 keystreambyte;
  for (i=0;i<msglen;i++) 
  {
    keystreambyte=0;
    for (j=0;j<8;j++) 
    {
      lfsr_clock(ctx);
      sbox_clock(ctx);
      keystreambyte=(keystreambyte<<1)^(ctx->pipe[0]&1)^ctx->linmask[0];
    }
    output[i]=input[i]^keystreambyte;
  }
}
