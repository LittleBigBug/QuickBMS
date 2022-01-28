/* py.c */

/* Inventors: Eli Biham and Jennifer Seberry     */

/* This cipher is submitted as a response to the */
/* Ecrypt call for stream ciphers                */

/* The designers/authors of Py (pronounced Roo)  */
/* keep their rights on the design and the name  */

/* However, no royalty will be necessary for use */
/* of Py, nor for using the submitted code.      */


#include "ecrypt-sync.h"
#include <string.h>


/* This is a permutation of all the values between 0 and 255, */
/* used by the key setup and IV setup.                        */
/* It is computed by the init function. It can also be        */
/* computed in advance and put into the code.                 */
u8 internal_permutation[256];

/*
 * Key setup. It is the user's responsibility to select a legal
 * keysize and ivsize, from the set of supported value.
 */
void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize)                 /* IV size in bits. */ 
{ 
  int i, j;
  u32 s;

  ctx->keysize=keysize;
  ctx->ivsize=ivsize;
  int keysizeb=(keysize+7)>>3;
  int ivsizeb=(ivsize+7)>>3;

#define Y(x) (ctx->KPY[(x)-(YMININD)][0])

  s = (u32)internal_permutation[keysizeb-1];
  s = (s<<8) | (u32)internal_permutation[(s ^ (ivsizeb-1))&0xFF];
  s = (s<<8) | (u32)internal_permutation[(s ^ key[0])&0xFF];
  s = (s<<8) | (u32)internal_permutation[(s ^ key[keysizeb-1])&0xFF];

  for(j=0; j<keysizeb; j++)
    {
      s = s + key[j];
      u8 s0 = internal_permutation[s&0xFF];
      s = ROTL32(s, 8) ^ (u32)s0;
    }
  /* Again */
  for(j=0; j<keysizeb; j++)
    {
      s = s + key[j];
      u8 s0 = internal_permutation[s&0xFF];
      s ^= ROTL32(s, 8) + (u32)s0;
    }

  for(i=YMININD, j=0; i<=YMAXIND; i++)
    {
      s = s + key[j];
      u8 s0 = internal_permutation[s&0xFF];
      s = ROTL32(s, 8) ^ (u32)s0;
      Y(i) = s;
      j++;
      if(j<keysizeb)
	continue;
      j=0;
    }
}
#undef P
#undef Y

/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
  int i;
  u32 s;

  int keysize=ctx->keysize;
  int ivsize=ctx->ivsize;
  int ivsizeb=(ivsize+7)>>3;

#define P(x) (((u8*)&(ctx->KPY[x][1]))[0])
#define KY(x) (ctx->KPY[(x)-(YMININD)][0])
#define EIV(x) (((u8*)&(ctx->KPY[x+256-ivsizeb][1]))[2])

  /* Create an initial permutation */
  u8 v= iv[0] ^ ((KY(0)>>16)&0xFF);
  u8 d=(iv[1%ivsizeb] ^ ((KY(1)>>16)&0xFF))|1;
  for(i=0; i<256; i++)
    {
      P(i)=internal_permutation[v];
      v+=d;
    }

#ifdef DEBUG
  /* Check that P is really a permutation */
  u8 permcheck[256];
  int j;

  for(j=0; j<256; j++)
    permcheck[j] = 0;
  for(j=0; j<256; j++)
    permcheck[P(j)]++;
  for(j=0; j<256; j++)
    if(permcheck[j] != 1)
      fprintf(stderr,
	      "Error in creation of the permutation in ivsetup: %02X appears %d times\n",
	      j, permcheck[j]);
#endif

  /* Initial s */
  s = ((u32)v<<24)^((u32)d<<16)^((u32)P(254)<<8)^((u32)P(255));
  s ^= KY(YMININD)+KY(YMAXIND);
  for(i=0; i<ivsizeb; i++)
    {
      s = s + iv[i] + KY(YMININD+i);
      u8 s0 = P(s&0xFF);
      EIV(i) = s0;
      s = ROTL32(s, 8) ^ (u32)s0;
    }
  /* Again, but with the last words of KY, and update EIV */
  for(i=0; i<ivsizeb; i++)
    {
      s = s + iv[i] + KY(YMAXIND-i);
      u8 s0 = P(s&0xFF);
      EIV(i) += s0;
      s = ROTL32(s, 8) ^ (u32)s0;
    }

#undef P
#undef Y
#undef EIV
#define P(i8,j) (((u8*)ctx->KPY)[(i8)+8*(j)+4])
  /* access P[i+j] where i8=8*i. */
  /* P is byte 4 of the 8-byte record */
#define EIV(i8,j) (((u8*)ctx->KPY)[(i8)+8*(j+256-ivsizeb)+6])
  /* access P[i+j] where i8=8*i. */
  /* EIV is byte 6 of the 8-byte record */
#define Y(i8,j) (((u32*)&(((u8*)ctx->KPY)[(i8)+8*((j)-(YMININD))]))[0])
  /* access Y[i+j+1] where i8=8*i. */
  /* Y is word 0 of the 2-word record */

  for(i=0; i<PYSIZE*8; i+=8)
    {
      u32 x0 = EIV(i,ivsizeb) = EIV(i,0)^(s&0xFF);
      P(i,256)=P(i,x0);
      P(i,x0)=P(i,0);
      Y(i,YMAXIND+1)=s=(s^Y(i,YMININD))+Y(i,x0);
    }

  s=s+Y(i,26)+Y(i,153)+Y(i,208);
  if(s==0)
    s=keysize+(ivsize<<16)+0x87654321;
  ctx->s=s;

#ifdef DEBUG
  /* Check that P is really a permutation                    */
  /* Assumes that i was not changed after the last loop      */

  for(j=0; j<256; j++)
    permcheck[j] = 0;
  for(j=0; j<256; j++)
    permcheck[P(i, j)]++;
  for(j=0; j<256; j++)
    if(permcheck[j] != 1)
      fprintf(stderr,
	      "Error in creation of the permutation in ivsetup: %02X appears %d times\n",
	      j, permcheck[j]);
#endif
}

#undef P
#undef Y

#define NUMBLOCKSATONCE 4000 /* A near-optimal value for the static array */
/* All computations are made in multiples of                 */
/* NUMBLOCKSATONCE*8 bytes, if possible                      */

static u32 PY[(NUMBLOCKSATONCE+PYSIZE)*2];
#define P(i8,j) (((u8*)PY)[(i8)+8*(j)+4])
	/* access P[i+j] where i8=8*i. */
	/* P is byte 4 of the 8-byte record */
#define Y(i8,j) (((u32*)&(((u8*)PY)[(i8)+8*((j)-(YMININD))]))[0])
	/* access Y[i+j+1] where i8=8*i. */
	/* Y is word 0 of the 2-word record */


void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)                /* Message length in bytes.     */ 
     /* If the msglen is not a multiple of 8, this must be   */
     /* the last call for this stream, or either you will    */
     /* loose a few bytes                                    */
{
  int i;

  u32 s=ctx->s;

  while(msglen>=NUMBLOCKSATONCE*8)
    {
      memcpy(PY, ctx->PY, PYSIZE*8);

      for(i=0; i<NUMBLOCKSATONCE*8; i+=8)
	{
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  u8 output1b[4];
	  U32TO8_LITTLE(output1b, output1);
	  ((u32*)(output+i))[0]=((u32*)output1b)[0]^((u32*)(input+i))[0];
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  u8 output2b[4];
	  U32TO8_LITTLE(output2b, output2);
	  ((u32*)(output+i+4))[0]=((u32*)output2b)[0]^((u32*)(input+i+4))[0];
	}

      memcpy(ctx->PY, ((u8*)PY)+i, PYSIZE*8);
      msglen-=NUMBLOCKSATONCE*8;
      input+=NUMBLOCKSATONCE*8;
      output+=NUMBLOCKSATONCE*8;
    }
  if(msglen>0)
    {

      memcpy(PY, ctx->PY, PYSIZE*8);

      for(i=0; i<(msglen&(~7)); i+=8)
	{
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  u8 output1b[4];
	  U32TO8_LITTLE(output1b, output1);
	  ((u32*)(output+i))[0]=((u32*)output1b)[0]^((u32*)(input+i))[0];
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  u8 output2b[4];
	  U32TO8_LITTLE(output2b, output2);
	  ((u32*)(output+i+4))[0]=((u32*)output2b)[0]^((u32*)(input+i+4))[0];
	}

      if(msglen&7)
	{
	  int ii;
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  u8 outputb[8];
	  U32TO8_LITTLE(outputb, output1);
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  U32TO8_LITTLE(outputb+4, output2);
	  for(ii=0; ii<(msglen&7); ii++)
	    (output+i)[ii]=outputb[ii]^(input+i)[ii];
	}

      memcpy(ctx->PY, ((u8*)PY)+i, PYSIZE*8);
    }

  ctx->s=s;
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)                /* Length of keystream in bytes. */
     /* If the length is not a multiple of 8, this must be the */
     /* last call for this stream, or either you will loose a few bytes */
{
  int i;

  u32 s=ctx->s;

  while(length>=NUMBLOCKSATONCE*8)
    {
      memcpy(PY, ctx->PY, PYSIZE*8);

      for(i=0; i<NUMBLOCKSATONCE*8; i+=8)
	{
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  U32TO8_LITTLE(keystream+i, output1);
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  U32TO8_LITTLE(keystream+i+4, output2);
	}

      memcpy(ctx->PY, ((u8*)PY)+i, PYSIZE*8);
      length-=NUMBLOCKSATONCE*8;
      keystream+=NUMBLOCKSATONCE*8;
    }
  if(length>0)
    {

      memcpy(PY, ctx->PY, PYSIZE*8);

      for(i=0; i<(length&(~7)); i+=8)
	{
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  U32TO8_LITTLE(keystream+i, output1);
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  U32TO8_LITTLE(keystream+i+4, output2);
	}

      if(length&7)
	{
	  int ii;
	  u32 x0=(Y(i,185)&0xFF);
	  P(i,256)=P(i,x0);
	  P(i,x0)=P(i,0);
	  s+=Y(i,P(i,1+72));
	  s-=Y(i,P(i,1+239));
	  s=ROTL32(s, (P(i,1+116)&31));
	  Y(i,YMAXIND+1)=(s^Y(i,YMININD))+Y(i,P(i,1+153));
	  s=ROTL32(s,11);
	  u32 output1=(s^Y(i,256))+Y(i,P(i,1+26));
	  u8 outputb[8];
	  U32TO8_LITTLE(outputb, output1);
	  s=ROTL32(s,7);
	  u32 output2=(s^Y(i,-1))+Y(i,P(i,1+208));
	  U32TO8_LITTLE(outputb+4, output2);
	  for(ii=0; ii<(length&7); ii++)
	    (keystream+i)[ii]=outputb[ii];
	}

      memcpy(ctx->PY, ((u8*)PY)+i, PYSIZE*8);
    }

  ctx->s=s;
}
#undef P
#undef Y


void ECRYPT_init(void)
{
  int i;
  u8 j=0;
  static u8 str[] =
    "This is the seed for generating the fixed internal permutation for Py. "
    "The permutation is used in the key setup and IV setup as a source of nonlinearity. "
    "The shifted special keys on a keyboard are ~!@#$%^&*()_+{}:|<>?";
  u8 *p=str;

  for(i=0; i<256; i++)
    internal_permutation[i] = i;

  for(i=0; i<256*16; i++)
    {
      j+=p[0];
      u8 tmp=internal_permutation[i&0xFF];
      internal_permutation[i&0xFF] = internal_permutation[j&0xFF];
      internal_permutation[j&0xFF] = tmp;
      p++;
      if(p[0] == 0)
	p=str;
    }
}
