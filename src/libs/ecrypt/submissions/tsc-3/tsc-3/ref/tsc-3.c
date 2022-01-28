/*
	Reference code for TSC-3
*/

#include "ecrypt-sync.h"

const u8 S[16] = {3,5,9,13,1,6,11,15,4,0,8,14,10,7,2,12};

u32 tsc(ECRYPT_ctx* ctx);

u32 tsc(ECRYPT_ctx* ctx)
{
	int i;
	u8 column[40], pi, ci;
	u32 y[4], tmp;
	u64 p[2];

	/* parameter calculation */
	p[0]  = (ctx->r[3]) & (ctx->r[2]) & (ctx->r[1]) & (ctx->r[0]);
	p[0] ^= p[0] + 0x4910891089;
	p[0] ^= ((ctx->r[3]) + (ctx->r[2])) << 1;
	p[0] ^= ((ctx->r[1]) + (ctx->r[0])) << 8;
	p[1]  = (ctx->r[3]) & (ctx->r[2]) & (ctx->r[1]) & (ctx->r[0]);
	p[1] ^= p[1] + 0x4910891089;
	p[1] ^= ((ctx->r[3]) + (ctx->r[2])) << 8;
	p[1] ^= ((ctx->r[1]) + (ctx->r[0])) << 1;

	/* s-box application */
	for (i = 0; i < 40; i++)
	{
		/* parameter at i-th column */
		pi = (u8) (
			  2*(1&(p[1] >> i))
			+ 1*(1&(p[0] >> i))
			);
		/* i-th column of state */
		ci = (u8) (
			  8*(1&(ctx->r[3] >> i))
			+ 4*(1&(ctx->r[2] >> i))
			+ 2*(1&(ctx->r[1] >> i))
			+ 1*(1&(ctx->r[0] >> i))
			);
		/* s-box application to i-th column */
		switch (pi)
		{
		case 3:
			column[i] = S[ci];
			break;
		case 2:
			column[i] = S[S[ci]];
			break;
		case 1:
			column[i] = S[S[S[S[S[ci]]]]];
			break;
		case 0:
			column[i] = S[S[S[S[S[S[ci]]]]]];
			break;
		}
	}
	/* copy results back into state */
	ctx->r[3] = 0;
	ctx->r[2] = 0;
	ctx->r[1] = 0;
	ctx->r[0] = 0;
	for (i = 0; i < 40; i++)
	{
		ctx->r[3] ^= ((u64) (1&(column[i] >> 3))) << i;
		ctx->r[2] ^= ((u64) (1&(column[i] >> 2))) << i;
		ctx->r[1] ^= ((u64) (1&(column[i] >> 1))) << i;
		ctx->r[0] ^= ((u64) (1&(column[i]     ))) << i;
	}

	/* filter calculation */
	/* rip out 32 bits of each word */
	y[3] = (u32) (ctx->r[3] >> 8);
	y[2] = (u32) (ctx->r[2] >> 8);
	y[1] = (u32) (ctx->r[1] >> 8);
	y[0] = (u32) (ctx->r[0] >> 8);
	/* mix words */
	if ((1&(ctx->r[0])) == 1) tmp = y[0], y[0] = y[1], y[1] = tmp;
	if ((1&(ctx->r[2])) == 1) tmp = y[2], y[2] = y[3], y[3] = tmp;
	if ((1&(ctx->r[1])) == 1) tmp = y[1], y[1] = y[2], y[2] = tmp;
	if ((1&(ctx->r[3])) == 1) tmp = y[0], y[0] = y[3], y[3] = tmp;
	/* calculate output */
	tmp = ROTL32(ROTL32(y[0], 7) + ROTR32(y[1], 2), 8)
		+ ROTR32(ROTL32(y[2], 7) +        y[3]    , 9);

	return tmp;
}

void ECRYPT_init(void)
{
	return;
}

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx,
  const u8* key,
  u32 keysize,
  u32 ivsize)
{
	u32 i;

	ctx->ivBs = ivsize/8;
	
	keysize /= 8;
	for (i = 0; i < 20; i++) ctx->k[i] = key[i % keysize];

	return;
}

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
	u32 i, j;
	u64 tmp;

	for (i = 0; i <  4; i++) ctx->r[i] = 0;

	for (i = 0; i < 20; i++)
	{
		ctx->r[i/5] ^= (((u64) ctx->k[i]) << 8*(i - 5*(i/5)));
	}

	for (j = 0; j < 3; j++)
	{
		for (i = 0; i < 20; i++)
		{
			ctx->r[i/5] ^= (((u64) iv[i % (ctx->ivBs)]) << 8*(i - 5*(i/5)));
		}
		tmp = (u64) tsc(ctx);
		tmp ^= tmp << 32;
		ctx->r[3] ^= 0xffffffffff & (tmp >> 24);
		ctx->r[2] ^= 0xffffffffff & (tmp >> 16);
		ctx->r[1] ^= 0xffffffffff & (tmp >>  8);
		ctx->r[0] ^= 0xffffffffff & (tmp      );
	}
	return;
}

void ECRYPT_process_bytes(
  int action,
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)
{
	u32 i, tmp;

	i = 0;
	if (msglen > 3)
	{
		msglen -= 3;
		for(; i < msglen; i += 4)
		{
			tmp = tsc(ctx);
			output[i  ] = input[i  ] ^ ((u8)  tmp       );
			output[i+1] = input[i+1] ^ ((u8) (tmp >>  8));
			output[i+2] = input[i+2] ^ ((u8) (tmp >> 16));
			output[i+3] = input[i+3] ^ ((u8) (tmp >> 24));
		}
		msglen += 3;
	}
	if (i == msglen) return;
	tmp = tsc(ctx);
	output[i] = input[i] ^ ((u8) (tmp      ));
	i++;
	if (i == msglen) return;
	output[i] = input[i] ^ ((u8) (tmp >>  8));
	i++;
	if (i == msglen) return;
	output[i] = input[i] ^ ((u8) (tmp >> 16));
	return;
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)
{
	u32 i, tmp;

	i = 0;
	if (length > 3)
	{
		length -= 3;
		for(; i < length; i += 4)
		{
			tmp = tsc(ctx);
			keystream[i  ] = (u8)  tmp       ;
			keystream[i+1] = (u8) (tmp >>  8);
			keystream[i+2] = (u8) (tmp >> 16);
			keystream[i+3] = (u8) (tmp >> 24);
		}
		length += 3;
	}
	if (i == length) return;
	tmp = tsc(ctx);
	keystream[i] = (u8) (tmp      );
	i++;
	if (i == length) return;
	keystream[i] = (u8) (tmp >>  8);
	i++;
	if (i == length) return;
	keystream[i] = (u8) (tmp >> 16);
	return;
}
