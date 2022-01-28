/*
 * OPTIMIZED IMPLEMENTATION OF STREAM CIPHER GRAIN-V1
 *
 * Filename: grain-v1.c
 *
 *
 * Synopsis:
 *  This file contains functions that implement the
 *  stream cipher Grain-v1. It also implements functions
 *  specified by the ECRYPT API.
 *
 *
 */
#include "ecrypt-sync.h"

#if (defined(__alpha) || defined(__sparc) || defined(__hppa))
#error this code manipulates unaligned words
#endif

void ECRYPT_init(void)
{ }

void Reinit_After_NIter(ECRYPT_ctx* ctx)
{
  *(u64*)(ctx->lfsr)     = *(u64*)(ctx->lfsr+ctx->count-5);
  *(u16*)((ctx->lfsr)+4) = *(u16*)(ctx->lfsr+ctx->count-1);
  *(u64*)(ctx->nfsr)     = *(u64*)(ctx->nfsr+ctx->count-5);
  *(u16*)((ctx->nfsr)+4) = *(u16*)(ctx->nfsr+ctx->count-1);
  ctx->nptr=ctx->nfsr;
  ctx->lptr=ctx->lfsr;
  ctx->count=5;
}

#ifdef ECRYPT_BIG_ENDIAN

u16 GetWord(ECRYPT_ctx* ctx)
{u32 ln0=U32TO32_LITTLE(*(u32*)(ctx->lptr)),
   nn0=U32TO32_LITTLE(*(u32*)(ctx->nptr)),
   ln1=U32TO32_LITTLE(*(u32*)((ctx->lptr)+1)),
   nn1=U32TO32_LITTLE(*(u32*)((ctx->nptr)+1)),
   ln2=U32TO32_LITTLE(*(u32*)((ctx->lptr)+2)),
   nn2=U32TO32_LITTLE(*(u32*)((ctx->nptr)+2)),
   ln3=U32TO32_LITTLE(*(u32*)((ctx->lptr)+3)),
   nn3=U32TO32_LITTLE(*(u32*)((ctx->nptr)+3)),
   ln4=U32TO32_LITTLE(*(u32*)((ctx->lptr)+4));
 ++(ctx->lptr);
 ++(ctx->nptr);
 ctx->nfsr[ctx->count] = U16TO16_LITTLE( (u16) (ln0^nn0^((nn0^nn3)>>14)^(nn2>>5)^(nn3>>4)^
   ((nn3>>12)&~(nn3>>15))^
   ((nn0>>9)&(((nn1>>12)&(nn2>>13)&(nn3>>15))^(~(nn0>>15))))^
   ((nn2>>1)&~((nn2>>5)&~((nn3>>4)&(nn3>>12))))^
   ((nn1>>5)&~(((nn0&nn3)>>15)&(nn3>>12)))^
   ((nn2>>13)&~((nn3>>4)&((nn3>>12)&(~((nn3>>15)&(nn2>>5))))))^
   ((nn1>>12)&~(((nn1>>5)&(nn2>>1))&(((nn2>>5)&(nn2>>13)&(nn3>>4))^~((nn0>>9)&(nn0>>15)))))));
 ctx->lfsr[ctx->count] = U16TO16_LITTLE( (u16) (ln0^(ln0>>13)^(ln1>>7)^(ln2>>6)^(ln3>>3)^(ln3>>14)));
 ++(ctx->count);
 return U16TO16_LITTLE( (u16) ((nn0>>1)^(nn0>>2)^(nn0>>4)^(nn0>>10)^(nn1>>15)^(nn2>>11)^(nn3>>8)^
   ((ln1>>9)&~((ln0>>3)&(ln2>>14)))^
   ((nn3>>15)&(((ln2>>14)&((ln0>>3)^(ln1>>9)^ln4))^(~ln4)))^
   (ln4&((ln0>>3)|(ln2>>14)))));
}

void ECRYPT_keystream_bytes(
                            ECRYPT_ctx* ctx,
                            u8* keystream,
                            u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(keystream+j) = GetWord(ctx);
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
       remWord=U16TO16_LITTLE(remWord);
       for (i=0;i<rem;++i) *(keystream+j++) = (u8)(remWord>>(8*i));
    }
}

void ECRYPT_encrypt_bytes(
                          ECRYPT_ctx* ctx,
                          const u8* plaintext,
                          u8* ciphertext,
                          u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(ciphertext+j) = GetWord(ctx) ^ (*(u16*)(plaintext+j));
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
	remWord=U16TO16_LITTLE(remWord);
    for (i=0;i<rem;++i)
      { *(ciphertext+j) = ((u8)(remWord>>(8*i))) ^ *(plaintext+j);
      j++;
      }
    }
}

void ECRYPT_decrypt_bytes(
                          ECRYPT_ctx* ctx,
                          const u8* ciphertext,
                          u8* plaintext,
                          u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(plaintext+j) = GetWord(ctx) ^ (*(u16*)(ciphertext+j));
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
    remWord=U16TO16_LITTLE(remWord);
    for (i=0;i<rem;++i)
      {  *(plaintext+j) = ((u8)(remWord>>(8*i))) ^ *(ciphertext+j);
      j++;
      }
    }
}

#else

u16 GetWord(ECRYPT_ctx* ctx)
{u32 ln0=*(u32*)(ctx->lptr),
   nn0=*(u32*)(ctx->nptr),
   ln2=*(u32*)((ctx->lptr)+2),
   nn2=*(u32*)((ctx->nptr)+2),
   ln1=*(u32*)(++(ctx->lptr)),
   nn1=*(u32*)(++(ctx->nptr)),
   ln3=*(u32*)((ctx->lptr)+2),
   nn3=*(u32*)((ctx->nptr)+2),
   ln4=*(u32*)((ctx->lptr)+3);
 ctx->nfsr[ctx->count] = ln0^nn0^((nn0^nn3)>>14)^(nn2>>5)^(nn3>>4)^
   ((nn3>>12)&~(nn3>>15))^
   ((nn0>>9)&(((nn1>>12)&(nn2>>13)&(nn3>>15))^(~(nn0>>15))))^
   ((nn2>>1)&~((nn2>>5)&~((nn3>>4)&(nn3>>12))))^
   ((nn1>>5)&~(((nn0&nn3)>>15)&(nn3>>12)))^
   ((nn2>>13)&~((nn3>>4)&((nn3>>12)&(~((nn3>>15)&(nn2>>5))))))^
   ((nn1>>12)&~(((nn1>>5)&(nn2>>1))&(((nn2>>5)&(nn2>>13)&(nn3>>4))^~((nn0>>9)&(nn0>>15)))));
 ctx->lfsr[ctx->count] = ln0^(ln0>>13)^(ln1>>7)^(ln2>>6)^(ln3>>3)^(ln3>>14);
 ++(ctx->count);
 return (nn0>>1)^(nn0>>2)^(nn0>>4)^(nn0>>10)^(nn1>>15)^(nn2>>11)^(nn3>>8)^
   ((ln1>>9)&~((ln0>>3)&(ln2>>14)))^
   ((nn3>>15)&(((ln2>>14)&((ln0>>3)^(ln1>>9)^ln4))^(~ln4)))^
   (ln4&((ln0>>3)|(ln2>>14)));
}

void ECRYPT_keystream_bytes(
                            ECRYPT_ctx* ctx,
                            u8* keystream,
                            u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(keystream+j) = GetWord(ctx);
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
    for (i=0;i<rem;++i) *(keystream+j++) = (u8)(remWord>>(8*i));
    }
}

void ECRYPT_encrypt_bytes(
                          ECRYPT_ctx* ctx,
                          const u8* plaintext,
                          u8* ciphertext,
                          u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(ciphertext+j) = GetWord(ctx) ^ (*(u16*)(plaintext+j));
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
    for (i=0;i<rem;++i)
      { *(ciphertext+j) = ((u8)(remWord>>(8*i))) ^ *(plaintext+j);
      j++;
      }
    }
}


void ECRYPT_decrypt_bytes(
                          ECRYPT_ctx* ctx,
                          const u8* ciphertext,
                          u8* plaintext,
                          u32 msglen)
{
  u32 itr=msglen>>1,i,j=0;
  u32 rem=msglen&1;
  u16 remWord;
  for (i=0;i<itr;++i)
    {   *(u16*)(plaintext+j) = GetWord(ctx) ^ (*(u16*)(ciphertext+j));
    j+=2;
    if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {  remWord=GetWord(ctx);
    for (i=0;i<rem;++i)
      {  *(plaintext+j) = ((u8)(remWord>>(8*i))) ^ *(ciphertext+j);
      j++;
      }
    }
}

#endif

void ECRYPT_keysetup(
                     ECRYPT_ctx* ctx,
                     const u8* key,
                     u32 keysize,                /* Key size in bits. */
                     u32 ivsize)  /* IV size in bits. */
{
  ctx->key=key;
  ctx->keysize=keysize;
  ctx->ivsize=ivsize;
}


void ECRYPT_ivsetup(
                    ECRYPT_ctx* ctx,
                    const u8* iv)
{   int i;
 register u16 pad;
 *(u64*)(ctx->nfsr)     = *(u64*)(ctx->key);
 *(u16*)((ctx->nfsr)+4) = *(u16*)((ctx->key)+8);
 *(u64*)(ctx->lfsr)     = *(u64*)(iv);
 *(u16*)((ctx->lfsr)+4) =  (u16)0xFFFF;
 ctx->nptr=ctx->nfsr;
 ctx->lptr=ctx->lfsr;
 ctx->count=5;
 for(i=0; i<10; ++i)
   { pad=GetWord(ctx);
   ctx->nfsr[i+5]^=pad;
   ctx->lfsr[i+5]^=pad;
   }
}





