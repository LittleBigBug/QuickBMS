/*
 * OPTIMIZED IMPLEMENTATION OF STREAM CIPHER GRAIN-128
 *
 * Filename: grain-128.c
 *
 *
 * Synopsis:
 *  This file contains functions that implement the
 *  stream cipher Grain-128. It also implements functions
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
  *(u64*)(ctx->lfsr)     = *(u64*)(ctx->lfsr+ctx->count-4);
  *(u64*)((ctx->lfsr)+2) = *(u64*)(ctx->lfsr+ctx->count-2);
  *(u64*)(ctx->nfsr)     = *(u64*)(ctx->nfsr+ctx->count-4);
  *(u64*)((ctx->nfsr)+2) = *(u64*)(ctx->nfsr+ctx->count-2);
  ctx->nptr=ctx->nfsr;
  ctx->lptr=ctx->lfsr;
  ctx->count=4;
}

#ifdef ECRYPT_BIG_ENDIAN

u32 GetWord(ECRYPT_ctx* ctx)
{ u8 *pn;
  u64 ln0=U64TO64_LITTLE(*(u64*)(ctx->lptr)),
    nn0=U64TO64_LITTLE(*(u64*)(ctx->nptr)),
    ln1=U64TO64_LITTLE(*(u64*)((ctx->lptr)+1)),
    nn1=U64TO64_LITTLE(*(u64*)((ctx->nptr)+1)),
    ln2=U64TO64_LITTLE(*(u64*)((ctx->lptr)+2)),
    nn2=U64TO64_LITTLE(*(u64*)((ctx->nptr)+2)),
    ln3=U64TO64_LITTLE(*(u64*)((ctx->lptr)+3)),
    nn3=U64TO64_LITTLE(*(u64*)((ctx->nptr)+3));
  ++(ctx->lptr);
  ++(ctx->nptr);
  pn = (u8*)(ctx->nptr);
  ctx->nfsr[ctx->count] = U32TO32_LITTLE( (u32) (nn0^nn3^ln0^(nn0>>26)^U64TO64_LITTLE(*(u64*)(pn+3))^(((nn0&nn1)^nn2)>>27)^
    (U64TO64_LITTLE(*(u64*)(pn+1)) & U64TO64_LITTLE(*(u64*)(pn+2)))^((nn0&nn2)>>3)^((nn1>>29)&(nn2>>1))^
    ((nn0>>17)&(nn0>>18))^((nn2>>4)&(nn2>>20))^((nn0>>11)&(nn0>>13))));
  ctx->lfsr[ctx->count] = U32TO32_LITTLE((u32)((ln0^ln3)^((ln1^ln2)>>6)^(ln0>>7)^(ln2>>17)));
  ln3=ln1>>13;
  nn3=ln0>>7;
  ln3&=ln2;
  nn3&=ln0;
  ++(ctx->count);
  ln3^=nn0;
  nn3^=nn1;
  return  U32TO32_LITTLE( (u32) (nn2^(nn0>>2)^(nn2>>9)^((nn0>>12)&(ln0>>8))^((nn2>>31)&(ln1>>10))^
    (nn3>>13)^(ln2>>29)^(ln3>>15)^(nn2>>25)^(nn1>>4)^((nn0>>12)&((nn2&ln2)>>31))));
}

void ECRYPT_keystream_bytes(
                            ECRYPT_ctx* ctx,
                            u8* keystream,
                            u32 msglen)
{
  u32 itr=msglen/4,i,j=0;
  u32 rem=msglen%4;
  u32 remWord;
  for (i=0;i<itr;++i)
    {   *(u32*)(keystream+j) = GetWord(ctx);
		j+=4;
		if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {	remWord=GetWord(ctx);
		remWord=U32TO32_LITTLE(remWord);	
		for (i=0;i<rem;++i) *(keystream+j++) = (u8)(remWord>>(8*i));
    }
}
void ECRYPT_encrypt_bytes(
                          ECRYPT_ctx* ctx,
                          const u8* plaintext,
                          u8* ciphertext,
                          u32 msglen)
{
  u32 itr=msglen/4,i,j=0;
  u32 rem=msglen%4;
  u32 remWord;
  for (i=0;i<itr;++i)
    {   *(u32*)(ciphertext+j) = GetWord(ctx) ^ (*(u32*)(plaintext+j));
		j+=4;
		if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {	remWord=GetWord(ctx);
		remWord=U32TO32_LITTLE(remWord);
		for (i=0;i<rem;++i)
		{	*(ciphertext+j) = ((u8)(remWord>>(8*i))) ^ *(plaintext+j);
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
  u32 itr=msglen/4,i,j=0;
  u32 rem=msglen%4;
  u32 remWord;
  for (i=0;i<itr;++i)
    {	*(u32*)(plaintext+j) = GetWord(ctx) ^ (*(u32*)(ciphertext+j));
		j+=4;
		if ((ctx->count)>=1024) Reinit_After_NIter(ctx);
    }
  if (rem)
    {	remWord=GetWord(ctx);
		remWord=U32TO32_LITTLE(remWord);
		for (i=0;i<rem;++i)
		{	*(plaintext+j) = ((u8)(remWord>>(8*i))) ^ *(ciphertext+j);
			j++;
		}
    }
}

#else

u32 GetWord(ECRYPT_ctx* ctx)
{	
	u64 ln0=*(u64*)(ctx->lptr),
	  nn0=*(u64*)(ctx->nptr),
	  ln2=*(u64*)((ctx->lptr)+2),
	  nn2=*(u64*)((ctx->nptr)+2),
	  ln1=*(u64*)(++(ctx->lptr)),
	  nn1=*(u64*)(++(ctx->nptr)),
	  ln3=*(u64*)((ctx->lptr)+2),
	  nn3=*(u64*)((ctx->nptr)+2);
	u8 * pn = (u8*)(ctx->nptr);
    ctx->nfsr[ctx->count] = nn0^nn3^ln0^(nn0>>26)^(*(u64*)(pn+3))^(((nn0&nn1)^nn2)>>27)^
			    ((*(u64*)(pn+1)) & (*(u64*)(pn+2)))^((nn0&nn2)>>3)^((nn1>>29)&(nn2>>1))^
			    ((nn0>>17)&(nn0>>18))^((nn2>>4)&(nn2>>20))^((nn0>>11)&(nn0>>13));
	ctx->lfsr[ctx->count] = (ln0^ln3)^((ln1^ln2)>>6)^(ln0>>7)^(ln2>>17);
	ln3=ln1>>13;
	nn3=ln0>>7;
	ln3&=ln2;
    nn3&=ln0;
    ++(ctx->count);
	ln3^=nn0;
	nn3^=nn1;
	return  nn2^(nn0>>2)^(nn2>>9)^((nn0>>12)&(ln0>>8))^((nn2>>31)&(ln1>>10))^
	      (nn3>>13)^(ln2>>29)^(ln3>>15)^(nn2>>25)^(nn1>>4)^((nn0>>12)&((nn2&ln2)>>31));
}
void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx, 
  u8* keystream, 
  u32 msglen)
{
	u32 itr=msglen/4,i,j=0;
	u32 rem=msglen%4;
	u32 remWord;
	for (i=0;i<itr;++i)
	{   *(u32*)(keystream+j) = GetWord(ctx);
		j+=4;
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
	u32 itr=msglen/4,i,j=0;
	u32 rem=msglen%4;
	u32 remWord;
	for (i=0;i<itr;++i)
	{   *(u32*)(ciphertext+j) = GetWord(ctx) ^ (*(u32*)(plaintext+j));
		j+=4;
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
	u32 itr=msglen/4,i,j=0;
	u32 rem=msglen%4;
	u32 remWord;
	for (i=0;i<itr;++i)
	{   *(u32*)(plaintext+j) = GetWord(ctx) ^ (*(u32*)(ciphertext+j));
		j+=4;
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
                     u32 keysize,               
                     u32 ivsize) 
{
  ctx->key=key;
  ctx->keysize=keysize;
  ctx->ivsize=ivsize;
}

void ECRYPT_ivsetup(
                    ECRYPT_ctx* ctx,
                    const u8* iv)
{   int i;
 register u32 pad;
 *(u64*)(ctx->nfsr)     = *(u64*)(ctx->key);
 *(u64*)((ctx->nfsr)+2) = *(u64*)((ctx->key)+8);
 *(u64*)(ctx->lfsr)     = *(u64*)(iv);
 *(u32*)((ctx->lfsr)+2) = *(u32*)(iv+8);
 *(u32*)((ctx->lfsr)+3) =  (u32)0xFFFFFFFF;
 ctx->nptr=ctx->nfsr;
 ctx->lptr=ctx->lfsr;
 ctx->count=4;
 for(i=0; i<8; ++i)
   { pad=GetWord(ctx);
   ctx->nfsr[i+4]^=pad;
   ctx->lfsr[i+4]^=pad;
   }
}
