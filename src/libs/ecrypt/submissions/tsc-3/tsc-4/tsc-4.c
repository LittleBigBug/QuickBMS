/*
	Reference code for TSC-4
*/

#include "ecrypt-sync.h"

const u8 S[16] = {9,2,11,15,3,0,14,4,10,13,12,5,6,8,7,1};

u8 tsc(ECRYPT_ctx* ctx);

u8 tsc(ECRYPT_ctx* ctx)
{
	int i;
	u8 column[2][32], pi, pj, ci, cj, tmp;
	u8 y[6];
	u32 p[2];
	
	/* parameter calculation */
	p[0]  = (ctx->r[3]) & (ctx->r[2]) & (ctx->r[1]) & (ctx->r[0]);
	p[0] ^= p[0] + 0x51291089;
	p[0] ^= (((ctx->r[3]) + (ctx->r[2]) + (ctx->r[1]) + (ctx->r[0])) << 1);

	p[1]  = (ctx->t[3]) & (ctx->t[2]) & (ctx->t[1]) & (ctx->t[0]);
	p[1] ^= p[1] + 0x12910895;
	p[1] ^= (((ctx->t[3]) + (ctx->t[2]) + (ctx->t[1]) + (ctx->t[0])) << 1);

	/* s-box application */
	for (i = 0; i < 32; i++)
	{
		/* parameter0 at i-th column */
		pi = (u8)(1&(p[0] >> i));

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
		case 1:
			column[0][i] = S[ci];
			break;
		case 0:
			column[0][i] = S[S[S[S[S[S[ci]]]]]];
			break;
		}

		/* parameter1 at i-th column */
		pj = (u8) (1&(p[1] >> i));

		/* i-th column of state */
		cj = (u8) (
			  8*(1&(ctx->t[3] >> i))
			+ 4*(1&(ctx->t[2] >> i))
			+ 2*(1&(ctx->t[1] >> i))
			+ 1*(1&(ctx->t[0] >> i))
			);
		/* s-box application to i-th column */
		switch (pj)
		{
		case 1:
			column[1][i] = S[cj];
			break;
		case 0:
			column[1][i] = S[S[S[S[S[S[cj]]]]]];
			break;
		}
	}

	/* copy results back into state */
	ctx->r[3] = 0;
	ctx->r[2] = 0;
	ctx->r[1] = 0;
	ctx->r[0] = 0;

	ctx->t[3] = 0;
	ctx->t[2] = 0;
	ctx->t[1] = 0;
	ctx->t[0] = 0;

	for (i = 0; i < 32; i++)
	{
		ctx->r[3] ^= ((u32) (1&(column[0][i] >> 3))) << i;
		ctx->r[2] ^= ((u32) (1&(column[0][i] >> 2))) << i;
		ctx->r[1] ^= ((u32) (1&(column[0][i] >> 1))) << i;
		ctx->r[0] ^= ((u32) (1&(column[0][i]     ))) << i;

		ctx->t[3] ^= ((u32) (1&(column[1][i] >> 3))) << i;
		ctx->t[2] ^= ((u32) (1&(column[1][i] >> 2))) << i;
		ctx->t[1] ^= ((u32) (1&(column[1][i] >> 1))) << i;
		ctx->t[0] ^= ((u32) (1&(column[1][i]     ))) << i;
	}

	/* filter calculation */
	y[5] = (u8)(ctx->r[0] >> 8) + (u8)(ctx->t[1] >> 24);
	y[4] = (u8)(ctx->r[3] >> 8) + (u8)(ctx->t[2] >> 24);

	y[3] = (u8)(ctx->r[1] >> 16) + (u8)(ctx->t[0] >> 16);
	y[2] = (u8)(ctx->r[2] >> 16) + (u8)(ctx->t[3] >> 16);

	y[1] = (u8)(ctx->r[0] >> 24) + (u8)(ctx->t[2] >> 8);
	y[0] = (u8)(ctx->r[3] >> 24) + (u8)(ctx->t[1] >> 8);

	/* calculate output */
	tmp = y[0] ^ ROTR8(y[1], 5) ^ ROTR8(y[2], 2) ^ ROTR8(y[3], 5)
		 ^ ROTR8(y[4], 6) ^ ROTR8(y[5], 2);

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

	for (i = 0; i < 10; i++) 
	{
		ctx->k[i] = key[i];
	}

	return;
}

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
	u32 i;
	u8 tmp;

	for (i = 0; i < 10; i++) 
	{
		ctx->iv[i] = iv[i];
	}

    ctx->r[0] = (u32)(ctx->k[0]) | ((u32)(ctx->k[1]) << 8) |
		        ((u32)(ctx->k[2]) << 16) | ((u32)(ctx->k[3]) << 24);  

    ctx->r[1] = (u32)(ctx->k[4]) | ((u32)(ctx->k[5]) << 8) |
		        ((u32)(ctx->k[6]) << 16) | ((u32)(ctx->k[7]) << 24);

    ctx->r[2] = (u32)(ctx->iv[0]) | ((u32)(ctx->iv[1]) << 8) |
		        ((u32)(ctx->iv[2]) << 16) | ((u32)(ctx->iv[3]) << 24);

    ctx->r[3] = (u32)(ctx->iv[4]) | ((u32)(ctx->iv[5]) << 8) |
		        ((u32)(ctx->iv[6]) << 16) | ((u32)(ctx->iv[7]) << 24);

    ctx->t[0] = (u32)(ctx->iv[8]) | ((u32)(ctx->iv[9]) << 8) |
		        ((u32)(ctx->iv[0]) << 16) | ((u32)(ctx->iv[1]) << 24);  

    ctx->t[1] = (u32)(ctx->iv[2]) | ((u32)(ctx->iv[3]) << 8) |
		        ((u32)(ctx->iv[4]) << 16) | ((u32)(ctx->iv[5]) << 24);

    ctx->t[2] = (u32)(ctx->k[8]) | ((u32)(ctx->k[9]) << 8) |
		        ((u32)(ctx->k[0]) << 16) | ((u32)(ctx->k[1]) << 24);

    ctx->t[3] = (u32)(ctx->k[2]) | ((u32)(ctx->k[3]) << 8) |
		        ((u32)(ctx->k[4]) << 16) | ((u32)(ctx->k[5]) << 24);

	for (i = 0; i < 8; i++)
	{
  /*
	printf("TSC-4 Warm-up 과정 후 테스트벡터 \n\n");
	printf("  0 번째 T1함수 상태값 :      x0          x1          x2          x3     \n");
	printf("                          %.8x    %.8x    %.8x    %.8x \n", ctx->r[0], ctx->r[1], ctx->r[2], ctx->r[3]);
	printf("  0 번째 T2함수 상태값 :      y0          y1          y2          y3     \n");
	printf("                          %.8x    %.8x    %.8x    %.8x \n", ctx->t[0], ctx->t[1], ctx->t[2], ctx->t[3]);
  */

		tmp = tsc(ctx);

		ctx->r[1] = ROTL32(ctx->r[1], 8);
		ctx->t[0] = ROTL32(ctx->t[0], 8);

		ctx->r[1] ^= tmp;
		ctx->t[0] ^= tmp;
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
	u32 i;
	u8 tmp;

	for(i=0; i < msglen; i ++)
	{
		tmp = tsc(ctx);
		output[i] = input[i] ^ tmp;
	}
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)
{
	u32 i;
	u8 tmp;

	for(i=0; i < length; i ++)
	{
		tmp = tsc(ctx);
		keystream[i] = tmp;
	}
}
