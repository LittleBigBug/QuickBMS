/******************************************************************************

                 Synchronous Stream Cipher :  DICING

                         author:  Li An-Ping


******************************************************************************/

#include "ecrypt-sync.h"

#define SL 64

static const u8 sbox[256] = {
  0xd5, 0xbd, 0x05, 0x00, 0xf0, 0x68, 0x03, 0xb3, 
  0xe5, 0x6b, 0xa3, 0xef, 0x92, 0x3b, 0x36, 0xdb, 
  0xc7, 0x98, 0x01, 0xe8, 0xb9, 0xf1, 0x7a, 0xb0, 
  0x50, 0x4f, 0xbf, 0x34, 0x4e, 0xf9, 0xfd, 0x78, 
  0x2c, 0xf8, 0x59, 0xc6, 0x82, 0x8c, 0x2b, 0xe0, 
  0x55, 0x3f, 0xb7, 0x84, 0x85, 0xf6, 0x61, 0xc3, 
  0xaf, 0x20, 0x2f, 0xdc, 0x6f, 0xc8, 0xb5, 0x1b, 
  0x8b, 0x0c, 0x12, 0xac, 0xdd, 0xe3, 0x1f, 0x49, 
  0x26, 0xba, 0xf7, 0x74, 0x97, 0x21, 0x60, 0xb1, 
  0xb6, 0x0f, 0x4d, 0x4c, 0x5b, 0x8e, 0xd1, 0xd2, 
  0x69, 0xaa, 0x67, 0x58, 0xd9, 0x75, 0xde, 0x3d, 
  0x47, 0xa9, 0x83, 0xc9, 0x9c, 0xa0, 0x11, 0xed, 
  0x3a, 0x4a, 0x48, 0x1a, 0xca, 0x57, 0xfb, 0xee, 
  0x5d, 0x39, 0x8a, 0x96, 0x13, 0xf5, 0xf2, 0x28, 
  0xe9, 0xe4, 0x62, 0x3c, 0x30, 0xfc, 0x5f, 0xcf, 
  0xa1, 0xd3, 0x66, 0xcd, 0xfa, 0xe2, 0xb4, 0x27, 
  0xd7, 0x15, 0x6a, 0x63, 0x33, 0x38, 0x08, 0x9d, 
  0xd8, 0x51, 0xe7, 0x7c, 0xe1, 0x44, 0x6d, 0x16, 
  0xa2, 0x88, 0x2a, 0x70, 0x5a, 0x52, 0x73, 0xa4, 
  0x71, 0x2d, 0xfe, 0x46, 0x7d, 0x29, 0xec, 0x41, 
  0x1e, 0x7f, 0x17, 0x42, 0x31, 0x23, 0x37, 0xea, 
  0x72, 0x89, 0x94, 0xae, 0xc5, 0xa7, 0xab, 0x9b, 
  0xd6, 0x76, 0x19, 0xd0, 0x9e, 0x91, 0x53, 0x81, 
  0x7e, 0x8f, 0x93, 0x7b, 0x18, 0xa5, 0x40, 0xf3, 
  0x4b, 0x35, 0x2e, 0x6e, 0x45, 0x80, 0x32, 0xa6, 
  0xad, 0xda, 0xd4, 0x10, 0x9f, 0xbb, 0x54, 0xe6, 
  0x14, 0x04, 0x07, 0xbc, 0x79, 0xff, 0x43, 0xeb, 
  0xcb, 0xa8, 0x5c, 0x64, 0xb8, 0x1c, 0x0e, 0x86, 
  0x0d, 0xc2, 0xb2, 0x56, 0x24, 0x3e, 0x5e, 0x09, 
  0x25, 0x6c, 0x0a, 0x06, 0x1d, 0x99, 0x02, 0xf4, 
  0x77, 0x87, 0x90, 0x95, 0xcc, 0x0b, 0xc4, 0xbe, 
  0x9a, 0xc1, 0x8d, 0xc0, 0x65, 0x22, 0xce, 0xdf
};

static const u8 ct[32] = {
  0xf8, 0x17, 0x72, 0xb1,
  0xd5, 0x53, 0x9f, 0x8c,
  0xbf, 0x0f, 0x02, 0xce,
  0x72, 0x95, 0x13, 0xf9,
  0xbc, 0x1d, 0x77, 0x99,
  0x5a, 0x21, 0x28, 0xa4,
  0x10, 0x5e, 0x53, 0xb5,
  0x30, 0xb0, 0x71, 0xbc
};

void initiate(ECRYPT_ctx* ctx, const u8* iv);
void selfcycl(ECRYPT_ctx* ctx);
int stream(ECRYPT_ctx* ctx, u8 *rkey);
void extendsbox(ECRYPT_ctx* ctx);

void ECRYPT_init()
{

}

void ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
  int i;
  int words = keysize / 32;

  ctx->key_size = keysize;
  ctx->iv_size = ivsize;
  
  for(i = 0; i < words; ++i)
    ((u32*)ctx->key)[i] = ((u32*)key)[i];
}

void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
  ctx->cyl = 0;
  initiate(ctx, iv);
  selfcycl(ctx);
}

void ECRYPT_process_bytes(
  int action,
  ECRYPT_ctx* ctx,
  const u8* input,
  u8* output,
  u32 msglen)
{
  u8 tmp[16];
  int i;

  while (msglen >= 16)
    if (stream(ctx, tmp) > 0)
      {
	for(i = 0; i < 4; ++i)
	  ((u32*)output)[i] = ((u32*)input)[i] ^ ((u32*)tmp)[i];
	
	msglen -= 16;
	input += 16;
	output += 16;
      }

  if (msglen > 0)
    if(stream(ctx, tmp) > 0)
      for(i = 0; i < msglen; i++)
	output[i] = input[i] ^ tmp[i];
}

/******************************************************************************

                            initialize function

******************************************************************************/

void initiate(ECRYPT_ctx* ctx, const u8* iv)
{
  int i;

  u8* ckey1 = (u8*)ctx->ckey1;
  u8* ckey2 = (u8*)ctx->ckey2;

  u32 tmp = 0;

  for(i = 0; i < 16; ++i)
    {
      ((u32*)ctx->skey1)[i] = 0;
      ((u32*)ctx->skey2)[i] = 0;
    }

  for (i = 0; i < 4; ++i)
    ((u32*)ctx->skey1)[i + 16] = 
      ((u32*)iv)[i] ^ ((u32*)ctx->key)[i] ^ ((u32*)ct)[i];

  if (ctx->key_size == 256)
    for (i = 0; i < 4; ++i)
      ((u32*)ctx->skey2)[i + 16] = 
	((u32*)iv)[i + 4] ^ ((u32*)ctx->key)[i + 4] ^ ((u32*)ct)[i + 4];
  else
    for (i = 0; i < 4; ++i)
      ((u32*)ctx->skey2)[i + 16] = 
	((u32*)iv)[i] ^ ~((u32*)ctx->key)[i] ^ ((u32*)ct)[i + 4];

  for (i = 0; i < 16; ++i)
    {
      ctx->skey1[i + 64] = sbox[ctx->skey1[i + 64]];
      ctx->skey2[i + 64] = sbox[ctx->skey2[i + 64]];
    }

  for (i = 0; i < 4; ++i)
    tmp ^= ((u32*)ctx->skey1)[i + 16] ^ ((u32*)ctx->skey2)[i + 16];

  tmp ^= tmp >> 16;
  tmp ^= tmp >> 8;
  tmp &= 0xFF;
  tmp = tmp | (tmp << 8) | (tmp << 16) | (tmp << 24);
  
  for(i = 0; i < 4; ++i)
    {
      ((u32*)ckey1)[i] = ((u32*)ctx->skey1)[i + 16] ^ tmp ^ ~((u32*)ct)[i];
      ((u32*)ckey2)[i] = ((u32*)ctx->skey2)[i + 16] ^ tmp ^ ~((u32*)ct)[i + 4];
    }
  
  ctx->skey1[79] &= 0x7F;
  ctx->skey2[79] &= 0x3F;
  
  for(i = 0; i < 16; i += 4)
    {
      ctx->ckey1[i / 4] = 
	(sbox[ckey1[i    ]]      ) |
	(sbox[ckey1[i + 1]] <<  8) |
	(sbox[ckey1[i + 2]] << 16) |
	(sbox[ckey1[i + 3]] << 24);
      
      ctx->ckey2[i / 4] = 
	(sbox[ckey2[i    ]]      ) |
	(sbox[ckey2[i + 1]] <<  8) |
	(sbox[ckey2[i + 2]] << 16) |
	(sbox[ckey2[i + 3]] << 24);
    }
}

/******************************************************************************

                           self-cycling function

******************************************************************************/

void selfcycl(ECRYPT_ctx* ctx)
{
  int i, k;
  int cyl = 64;
  int tmp, n1, n2, n;
  u8 *ptr1, *ptr2;

  for (k = 0; k < 4; k++)
    {
      ctx->var1[k] = 0;
      ctx->var2[k] = 0;
      ctx->mkey[k] = 0;
    }

  for (k = 0; k < 64; k++)
    {
      if ( k <= 32)
	{
	  ptr1 = ctx->skey1 + cyl;
	  ptr2 = ctx->skey2 + cyl;

	  for (i = 0; i < 4; i++)
	    {
	      ctx->mkey[i] &=
		((u32)(ptr1[0] ^ ptr2[0])      ) ^
		((u32)(ptr1[1] ^ ptr2[1]) <<  8) ^
		((u32)(ptr1[2] ^ ptr2[2]) << 16) ^
		((u32)(ptr1[3] ^ ptr2[3]) << 24);

	      ctx->mkey[i] ^= ctx->ckey1[i] ^ ctx->ckey2[i];

	      ptr1 += 4;
	      ptr2 += 4;
	    }

	  if (k == 32)
	    {	      
	      for (i = 0; i < 4; i++)
		ctx->ch[i] = ctx->ckey1[i] ^ ctx->ckey2[i];
	      
	      extendsbox(ctx);
	    }
	}

      cyl--;

      ptr1 = ctx->skey1 + cyl;

      n1 = tmp = ((u32)ptr1[15] >> 7) | ((u32)ptr1[16] << 1);
      tmp ^= tmp << 3;

      ptr1[ 0] ^= U8V(tmp);
      ptr1[ 1] ^= U8V(tmp >> 8);

      tmp <<= 1;

      ptr1[ 5] ^= U8V(tmp);
      ptr1[11] ^= U8V(tmp);

      tmp >>= 8;

      ptr1[ 6] ^= U8V(tmp);
      ptr1[12] ^= U8V(tmp);

      ptr1[15] &= 0x7F;
      ptr1[16] = 0;

      ptr2 = ctx->skey2 + cyl;

      n2 = tmp= ((u32)ptr2[15] >> 6) | ((u32)ptr2[16] << 2);
      tmp ^= tmp << 7;

      ptr2[0] ^= U8V(tmp);
      ptr2[1] ^= U8V(tmp >> 8);

      tmp <<= 3;

      ptr2[ 4] ^= U8V(tmp);
      ptr2[10] ^= U8V(tmp);

      tmp >>= 8;
      ptr2[ 5] ^= U8V(tmp);
      ptr2[11] ^= U8V(tmp);

      tmp >>= 8;
      ptr2[ 6] ^= U8V(tmp);
      ptr2[12] ^= U8V(tmp);

      ptr2[15] &= 0x3F;
      ptr2[16] = 0;

      n = (n1 ^ n2) & 0xF;

      if (n > 0)
	{
	  u32* c1 = ctx->ckey1;
	  u32 z = c1[3] >> (32 - n);

	  z = z ^ (z << 2) ^ (z << 7);
	  
	  c1[3] = ((c1[3] << n) | (c1[2] >> (32 - n))) ^ (z >> 6);
	  c1[2] = ((c1[2] << n) | (c1[1] >> (32 - n))) ^ (z >> 7) ^ (z << 26);
	  c1[1] = ((c1[1] << n) | (c1[0] >> (32 - n))) ^ (z << 3) ^ (z << 25);
	  c1[0] = ((c1[0] << n)) ^ z;
	}

      n = (n1 ^ n2) >> 4;

      if (n > 0)
	{
	  u32* c2 = ctx->ckey2;
	  u32 z = c2[3] >> (32 - n);

	  z = z ^ (z << 1) ^ (z << 7);

	  c2[3] = ((c2[3] << n) | (c2[2] >> (32 - n))) ^ (z << 1);
	  c2[2] = ((c2[2] << n) | (c2[1] >> (32 - n))) ^ (z >> 6);
	  c2[1] = ((c2[1] << n) | (c2[0] >> (32 - n))) ^ (z >> 5) ^ (z << 26);
	  c2[0] = ((c2[0] << n)) ^ z ^ (z << 27);
	}

      for (i = 0; i < 4; i++)
	{
	  ctx->var1[i] ^= ctx->ckey1[i];
	  ctx->var2[i] ^= ctx->ckey2[i];
	}
    }
}

/******************************************************************************

                              keystream function

******************************************************************************/

int stream(ECRYPT_ctx* ctx,u8 *rkey)
{
  int i;
  int tmp, n1, n2, n;
  u8 *ptr1, *ptr2;

  /*......Updading States......*/

  if (ctx->cyl == 0)
    {
      for(i = 0; i < 4; ++i)
	{
	  ((u32*)ctx->skey1)[i + SL / 4] = ((u32*)ctx->skey1)[i];
	  ((u32*)ctx->skey1)[i] = 0;
	  ((u32*)ctx->skey2)[i + SL / 4] = ((u32*)ctx->skey2)[i];
	  ((u32*)ctx->skey2)[i] = 0;
	}

      ctx->cyl = SL;
    }

  ctx->cyl--;

  ptr1 = ctx->skey1 + ctx->cyl;

  n1 = tmp = ((u32)ptr1[15] >> 7) | ((u32)ptr1[16] << 1);
  tmp ^= tmp << 3;
  
  ptr1[ 0] ^= U8V(tmp);
  ptr1[ 1] ^= U8V(tmp >> 8);
  
  tmp <<= 1;

  ptr1[ 5] ^= U8V(tmp);
  ptr1[11] ^= U8V(tmp);
  
  tmp >>= 8;
  
  ptr1[ 6] ^= U8V(tmp);
  ptr1[12] ^= U8V(tmp);
  
  ptr1[15] &= 0x7F;
  ptr1[16] = 0;
  
  ptr2 = ctx->skey2 + ctx->cyl;
  
  n2 = tmp= ((u32)ptr2[15] >> 6) | ((u32)ptr2[16] << 2);
  tmp ^= tmp << 7;
  
  ptr2[0] ^= U8V(tmp);
  ptr2[1] ^= U8V(tmp >> 8);

  tmp <<= 3;

  ptr2[ 4] ^= U8V(tmp);
  ptr2[10] ^= U8V(tmp);

  tmp >>= 8;
  ptr2[ 5] ^= U8V(tmp);
  ptr2[11] ^= U8V(tmp);

  tmp >>= 8;
  ptr2[ 6] ^= U8V(tmp);
  ptr2[12] ^= U8V(tmp);

  ptr2[15] &= 0x3F;
  ptr2[16] = 0;

  n = (n1 ^ n2) & 0xF;

  if (n > 0)
    {
      u32* c1 = ctx->ckey1;
      u32 z = c1[3] >> (32 - n);

      z = z ^ (z << 2) ^ (z << 7);
	  
      c1[3] = ((c1[3] << n) | (c1[2] >> (32 - n))) ^ (z >> 6);
      c1[2] = ((c1[2] << n) | (c1[1] >> (32 - n))) ^ (z >> 7) ^ (z << 26);
      c1[1] = ((c1[1] << n) | (c1[0] >> (32 - n))) ^ (z << 3) ^ (z << 25);
      c1[0] = ((c1[0] << n)) ^ z;
    }

  n = (n1 ^ n2) >> 4;

  if (n > 0)
    {
      u32* c2 = ctx->ckey2;
      u32 z = c2[3] >> (32 - n);

      z = z ^ (z << 1) ^ (z << 7);

      c2[3] = ((c2[3] << n) | (c2[2] >> (32 - n))) ^ (z << 1);
      c2[2] = ((c2[2] << n) | (c2[1] >> (32 - n))) ^ (z >> 6);
      c2[1] = ((c2[1] << n) | (c2[0] >> (32 - n))) ^ (z >> 5) ^ (z << 26);
      c2[0] = ((c2[0] << n)) ^ z ^ (z << 27);
    }

  for (i = 0; i < 4; i++)
    {
      ctx->var1[i] ^= ctx->ckey1[i];
      ctx->var2[i] ^= ctx->ckey2[i];
    }

  /*  ......  Updating end  ......  */

  /*......Combining sub-process......*/

  if (n1 > n2)
    {
      u32 d0 = ctx->var1[0];
      u32 c0 = ctx->var2[0] ^
	ctx->sbox0[U8V(d0      )] ^ ctx->sbox1[U8V(d0 >>  8)] ^
	ctx->sbox2[U8V(d0 >> 16)] ^ ctx->sbox3[U8V(d0 >> 24)];

      u32 d1 = ctx->var1[1];
      u32 c1 = ctx->var2[1] ^
	ctx->sbox0[U8V(d1      )] ^ ctx->sbox1[U8V(d1 >>  8)] ^
	ctx->sbox2[U8V(d1 >> 16)] ^ ctx->sbox3[U8V(d1 >> 24)];

      u32 d2 = ctx->var1[2];
      u32 c2 = ctx->var2[2] ^
	ctx->sbox0[U8V(d2      )] ^ ctx->sbox1[U8V(d2 >>  8)] ^
	ctx->sbox2[U8V(d2 >> 16)] ^ ctx->sbox3[U8V(d2 >> 24)];

      u32 d3 = ctx->var1[3];
      u32 c3 = ctx->var2[3] ^
	ctx->sbox0[U8V(d3      )] ^ ctx->sbox1[U8V(d3 >>  8)] ^
	ctx->sbox2[U8V(d3 >> 16)] ^ ctx->sbox3[U8V(d3 >> 24)];

      U32TO8_LITTLE(rkey     , ctx->ch[0] ^
	ctx->sbox0[U8V(c0      )] ^ ctx->sbox1[U8V(c1 >>  8)] ^
	ctx->sbox2[U8V(c2 >> 16)] ^ ctx->sbox3[U8V(c3 >> 24)]);	

      U32TO8_LITTLE(rkey +  4, ctx->ch[1] ^
	ctx->sbox0[U8V(c1      )] ^ ctx->sbox1[U8V(c2 >>  8)] ^
	ctx->sbox2[U8V(c3 >> 16)] ^ ctx->sbox3[U8V(c0 >> 24)]);	

      U32TO8_LITTLE(rkey +  8, ctx->ch[2] ^
	ctx->sbox0[U8V(c2      )] ^ ctx->sbox1[U8V(c3 >>  8)] ^
	ctx->sbox2[U8V(c0 >> 16)] ^ ctx->sbox3[U8V(c1 >> 24)]);	

      U32TO8_LITTLE(rkey + 12, ctx->ch[3] ^
	ctx->sbox0[U8V(c3      )] ^ ctx->sbox1[U8V(c0 >>  8)] ^
	ctx->sbox2[U8V(c1 >> 16)] ^ ctx->sbox3[U8V(c2 >> 24)]);	

      return 1 ;
    }
  else if(n1 < n2)
    {
      u32 d0 = ctx->var2[0];
      u32 c0 = ctx->var1[0] ^
	ctx->sbox0[U8V(d0      )] ^ ctx->sbox1[U8V(d0 >>  8)] ^
	ctx->sbox2[U8V(d0 >> 16)] ^ ctx->sbox3[U8V(d0 >> 24)];

      u32 d1 = ctx->var2[1];
      u32 c1 = ctx->var1[1] ^
	ctx->sbox0[U8V(d1      )] ^ ctx->sbox1[U8V(d1 >>  8)] ^
	ctx->sbox2[U8V(d1 >> 16)] ^ ctx->sbox3[U8V(d1 >> 24)];

      u32 d2 = ctx->var2[2];
      u32 c2 = ctx->var1[2] ^
	ctx->sbox0[U8V(d2      )] ^ ctx->sbox1[U8V(d2 >>  8)] ^
	ctx->sbox2[U8V(d2 >> 16)] ^ ctx->sbox3[U8V(d2 >> 24)];

      u32 d3 = ctx->var2[3];
      u32 c3 = ctx->var1[3] ^
	ctx->sbox0[U8V(d3      )] ^ ctx->sbox1[U8V(d3 >>  8)] ^
	ctx->sbox2[U8V(d3 >> 16)] ^ ctx->sbox3[U8V(d3 >> 24)];

      U32TO8_LITTLE(rkey     , ctx->ch[0] ^
	ctx->sbox0[U8V(c0      )] ^ ctx->sbox1[U8V(c1 >>  8)] ^
	ctx->sbox2[U8V(c2 >> 16)] ^ ctx->sbox3[U8V(c3 >> 24)]);	

      U32TO8_LITTLE(rkey +  4, ctx->ch[1] ^
	ctx->sbox0[U8V(c1      )] ^ ctx->sbox1[U8V(c2 >>  8)] ^
	ctx->sbox2[U8V(c3 >> 16)] ^ ctx->sbox3[U8V(c0 >> 24)]);	

      U32TO8_LITTLE(rkey +  8, ctx->ch[2] ^
	ctx->sbox0[U8V(c2      )] ^ ctx->sbox1[U8V(c3 >>  8)] ^
	ctx->sbox2[U8V(c0 >> 16)] ^ ctx->sbox3[U8V(c1 >> 24)]);	

      U32TO8_LITTLE(rkey + 12, ctx->ch[3] ^
	ctx->sbox0[U8V(c3      )] ^ ctx->sbox1[U8V(c0 >>  8)] ^
	ctx->sbox2[U8V(c1 >> 16)] ^ ctx->sbox3[U8V(c2 >> 24)]);	

      return 1 ;
    }
  else 
    return 0;
}

/******************************************************************************

                         extending S-boxes function

******************************************************************************/

void extendsbox(ECRYPT_ctx* ctx)
{
  int i, k;

  u8 w[2][8] = {{0}};
  u8 y[2][8] = {{0}};

  u32 tabl[256] = {0};

  u32 c1 = (ctx->mkey[0] & 0x08040201) ^ (ctx->mkey[1] & 0x80402010);
  u32 c2 = (ctx->mkey[2] & 0x08040201) ^ (ctx->mkey[3] & 0x80402010);

  c1 ^= ROTL32(c1, 16); c1 ^= ROTL32(c1, 8);
  c2 ^= ROTL32(c2, 16); c2 ^= ROTL32(c2, 8);

  tabl[0] = (c1 & 0xFFFF00FF) ^ (c2 & 0xFF00FF00);

  U32TO8_LITTLE(w[1] + 0, (ctx->mkey[0] & 0xF0F8FCFE) ^ 0x08040201);
  U32TO8_LITTLE(w[1] + 4, (ctx->mkey[1] & 0x0080C0E0) ^ 0x80402010);

  U32TO8_LITTLE(y[1] + 0, (ctx->mkey[2] & 0xF0F8FCFE) ^ 0x08040201);
  U32TO8_LITTLE(y[1] + 4, (ctx->mkey[3] & 0x0080C0E0) ^ 0x80402010);

  for (k = 7; k >= 0; k--)
    {
      u32 x = ctx->mkey[k / 4    ] >> ((k & 0x3) * 8);
      u32 z = ctx->mkey[k / 4 + 2] >> ((k & 0x3) * 8);

      for (i = 0; i < k; i++, x >>= 1, z >>= 1)
	{
	  w[1][k] ^= w[x & 1][i];
	  y[1][k] ^= y[z & 1][i];
	}
    }

  for (k = 0; k < 8; k++)
    {
      const int n = 1 << k;
      const u32 x = w[1][k];
      const u32 z = y[1][k];

      const u32 tmp = x | (z << 8) | (x << 16) | ((x ^ z) << 24);

      for(i = 0; i < n; i++)
	tabl[i + n] = tabl[i] ^ tmp;
    }

  for (k = 0; k < 256; k++)
    {
      const u32 v = tabl[sbox[k]];

      ctx->sbox0[k] = v;
      ctx->sbox1[k] = ROTR32(v,  8);
      ctx->sbox2[k] = ROTR32(v, 16);
      ctx->sbox3[k] = ROTR32(v, 24);
    }
}





