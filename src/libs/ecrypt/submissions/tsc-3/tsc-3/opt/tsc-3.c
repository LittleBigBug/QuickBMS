/*
 *
 *  Optimized code for Pentium-IV / Microsoft Visual C++ 6.0
 *
 *  Developed on a Pentium-IV 3.2GHz machine running Windows XP SP2.
 *
 */
#include "ecrypt-sync.h"

#if (defined(__alpha) || defined(__sparc) || defined(__hppa))
#error this code manipulates unaligned words and needs to rewritten
#endif

u32 tsc(ECRYPT_ctx* ctx);

u32 tsc(ECRYPT_ctx* ctx)
{
	u32 pH0, pL0, pH1, pL1;
	u32 s0, s1, s2, s3, u0, u1, u2, u3;
	u32 tmpH, tmpL;

	/* parameter calculation */
	pL0  = (ctx->l[3]) & (ctx->l[2]) & (ctx->l[1]) & (ctx->l[0]);
	pH0  = (ctx->h[3]) & (ctx->h[2]) & (ctx->h[1]) & (ctx->h[0]);
	pL0 ^= tmpL = pL0 + 0x89;
	pH0 ^= pH0 + 0x49108910 + (tmpL >> 8);
	pL1  = pL0;
	pH1  = pH0;

	tmpL = (ctx->l[3]) + (ctx->l[2]);
	tmpH = (ctx->h[3]) + (ctx->h[2]);
	pL0 ^=               (tmpL << 1);
	pH0 ^= (tmpH << 1) + (tmpL >> 7);
	pH1 ^= (tmpH << 8) + (tmpL     );

	tmpL = (ctx->l[1]) + (ctx->l[0]);
	tmpH = (ctx->h[1]) + (ctx->h[0]);
	pL1 ^=               (tmpL << 1);
	pH1 ^= (tmpH << 1) + (tmpL >> 7);
	pH0 ^= (tmpH << 8) + (tmpL     );

	/* s-box application */
	s3 =  (ctx->l[1]) ^ ((ctx->l[3]) &  (ctx->l[2]) & ~(ctx->l[0]));
	s2 =  (ctx->l[0]) ^ ((ctx->l[3]) & ~(ctx->l[2]) & ~(ctx->l[1]));
	s0 = ~(ctx->l[3]) ^ ((ctx->l[2]) & ~(ctx->l[1]) &  (ctx->l[0]));
	u3 =  s3 ^ ((ctx->l[3]) |  (ctx->l[2]) | ~(ctx->l[0]));
	u2 =  s2 ^ ((ctx->l[3]) | ~(ctx->l[2]) | ~(ctx->l[1]));
	u0 =  s0 ^ ((ctx->l[2]) | ~(ctx->l[1]) |  (ctx->l[0]));
	u1 =  (ctx->l[2]) ^ ((ctx->l[3]) |  (ctx->l[1]) |  (ctx->l[0]));
	s1 = ~u1 ^ ((ctx->l[3]) &  (ctx->l[1]) &  (ctx->l[0]));

	ctx->l[3] = 0xff & ((pL1 & s3) ^ (~pL1 & u3));
	ctx->l[2] = 0xff & ((pL1 & s2) ^ (~pL1 & u2));
	ctx->l[1] = 0xff & ((pL1 & s1) ^ (~pL1 & u1));
	ctx->l[0] = 0xff & ((pL1 & s0) ^ (~pL1 & u0));

	s3 =  (ctx->h[1]) ^ ((ctx->h[3]) &  (ctx->h[2]) & ~(ctx->h[0]));
	s2 =  (ctx->h[0]) ^ ((ctx->h[3]) & ~(ctx->h[2]) & ~(ctx->h[1]));
	s0 = ~(ctx->h[3]) ^ ((ctx->h[2]) & ~(ctx->h[1]) &  (ctx->h[0]));
	u3 =  s3 ^ ((ctx->h[3]) |  (ctx->h[2]) | ~(ctx->h[0]));
	u2 =  s2 ^ ((ctx->h[3]) | ~(ctx->h[2]) | ~(ctx->h[1]));
	u0 =  s0 ^ ((ctx->h[2]) | ~(ctx->h[1]) |  (ctx->h[0]));
	u1 =  (ctx->h[2]) ^ ((ctx->h[3]) |  (ctx->h[1]) |  (ctx->h[0]));
	s1 = ~u1 ^ ((ctx->h[3]) &  (ctx->h[1]) &  (ctx->h[0]));

	ctx->h[3] = (pH1 & s3) ^ (~pH1 & u3);
	ctx->h[2] = (pH1 & s2) ^ (~pH1 & u2);
	ctx->h[1] = (pH1 & s1) ^ (~pH1 & u1);
	ctx->h[0] = (pH1 & s0) ^ (~pH1 & u0);

	s3 =  (ctx->l[1]) ^ ((ctx->l[3]) &  (ctx->l[2]) & ~(ctx->l[0]));
	s2 =  (ctx->l[0]) ^ ((ctx->l[3]) & ~(ctx->l[2]) & ~(ctx->l[1]));
	s1 =  (ctx->l[2]) ^ ((ctx->l[3]) &  (ctx->l[1]) &  (ctx->l[0])) ^ ~((ctx->l[3]) | (ctx->l[1]) | (ctx->l[0]));
	s0 = ~(ctx->l[3]) ^ ((ctx->l[2]) & ~(ctx->l[1]) &  (ctx->l[0]));

	ctx->l[3] = 0xff & ((pL0 & ctx->l[3]) ^ (~pL0 & s3));
	ctx->l[2] = 0xff & ((pL0 & ctx->l[2]) ^ (~pL0 & s2));
	ctx->l[1] = 0xff & ((pL0 & ctx->l[1]) ^ (~pL0 & s1));
	ctx->l[0] = 0xff & ((pL0 & ctx->l[0]) ^ (~pL0 & s0));

	s3 =  (ctx->h[1]) ^ ((ctx->h[3]) &  (ctx->h[2]) & ~(ctx->h[0]));
	s2 =  (ctx->h[0]) ^ ((ctx->h[3]) & ~(ctx->h[2]) & ~(ctx->h[1]));
	s1 =  (ctx->h[2]) ^ ((ctx->h[3]) &  (ctx->h[1]) &  (ctx->h[0])) ^ ~((ctx->h[3]) | (ctx->h[1]) | (ctx->h[0]));
	s0 = ~(ctx->h[3]) ^ ((ctx->h[2]) & ~(ctx->h[1]) &  (ctx->h[0]));

	ctx->h[3] = (pH0 & ctx->h[3]) ^ (~pH0 & s3);
	ctx->h[2] = (pH0 & ctx->h[2]) ^ (~pH0 & s2);
	ctx->h[1] = (pH0 & ctx->h[1]) ^ (~pH0 & s1);
	ctx->h[0] = (pH0 & ctx->h[0]) ^ (~pH0 & s0);

	/* filter calculation */
	s3 = ctx->h[3];
	s2 = ctx->h[2];
	s1 = ctx->h[1];
	s0 = ctx->h[0];
	if ((1&(ctx->l[0])) == 1) tmpH = s0, s0 = s1, s1 = tmpH;
	if ((1&(ctx->l[2])) == 1) tmpH = s2, s2 = s3, s3 = tmpH;
	if ((1&(ctx->l[1])) == 1) tmpH = s1, s1 = s2, s2 = tmpH;
	if ((1&(ctx->l[3])) == 1) tmpH = s0, s0 = s3, s3 = tmpH;
	tmpH = ROTL32(ROTL32(s0, 7) + ROTR32(s1, 2), 8)
		 + ROTR32(ROTL32(s2, 7) +        s3    , 9);

	return tmpH;
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
	ctx->ivBs = ivsize/8;

	keysize /= 8;
	((u32 *) ctx->k)[ 0] = ((u32 *) key)[ 0];
	((u32 *) ctx->k)[ 1] = ((u32 *) key)[ 1];
	ctx->k[ 8] = key[ 8];
	ctx->k[ 9] = key[ 9];
	ctx->k[10] = key[10%keysize];
	ctx->k[11] = key[11%keysize];
	ctx->k[12] = key[12%keysize];
	ctx->k[13] = key[13%keysize];
	ctx->k[14] = key[14%keysize];
	ctx->k[15] = key[15%keysize];
	ctx->k[16] = key[16%keysize];
	ctx->k[17] = key[17%keysize];
	ctx->k[18] = key[18%keysize];
	ctx->k[19] = key[19%keysize];

	return;
}

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  const u8* iv)
{
	u32 tmp, i;
	u8 v[20];

	tmp = ctx->ivBs;
	for (i = 0; i < 20; i++) v[i] = iv[i%tmp];

	ctx->l[3] = (u32) ((ctx->k[15]) ^ (v [15]));
	ctx->l[2] = (u32) ((ctx->k[10]) ^ (v [10]));
	ctx->l[1] = (u32) ((ctx->k[ 5]) ^ (v [ 5]));
	ctx->l[0] = (u32) ((ctx->k[ 0]) ^ (v [ 0]));
	ctx->h[3] = (((u32 *) (&(ctx->k[16])))[0]) ^ (((u32 *) (&(v[16])))[0]);
	ctx->h[2] = (((u32 *) (&(ctx->k[11])))[0]) ^ (((u32 *) (&(v[11])))[0]);
	ctx->h[1] = (((u32 *) (&(ctx->k[ 6])))[0]) ^ (((u32 *) (&(v[ 6])))[0]);
	ctx->h[0] = (((u32 *) (&(ctx->k[ 1])))[0]) ^ (((u32 *) (&(v[ 1])))[0]);

	tmp = tsc(ctx);
	ctx->h[3] ^=        tmp;
	ctx->l[0] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[2] ^=        tmp;
	ctx->l[3] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[1] ^=        tmp;
	ctx->l[2] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[0] ^=        tmp;
	ctx->l[1] ^= 0xff & tmp;

	ctx->l[3] ^= (u32) v[15];
	ctx->l[2] ^= (u32) v[10];
	ctx->l[1] ^= (u32) v[ 5];
	ctx->l[0] ^= (u32) v[ 0];
	ctx->h[3] ^= ((u32 *) (&(v[16])))[0];
	ctx->h[2] ^= ((u32 *) (&(v[11])))[0];
	ctx->h[1] ^= ((u32 *) (&(v[ 6])))[0];
	ctx->h[0] ^= ((u32 *) (&(v[ 1])))[0];

	tmp = tsc(ctx);
	ctx->h[3] ^=        tmp;
	ctx->l[0] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[2] ^=        tmp;
	ctx->l[3] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[1] ^=        tmp;
	ctx->l[2] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[0] ^=        tmp;
	ctx->l[1] ^= 0xff & tmp;

	ctx->l[3] ^= (u32) v[15];
	ctx->l[2] ^= (u32) v[10];
	ctx->l[1] ^= (u32) v[ 5];
	ctx->l[0] ^= (u32) v[ 0];
	ctx->h[3] ^= ((u32 *) (&(v[16])))[0];
	ctx->h[2] ^= ((u32 *) (&(v[11])))[0];
	ctx->h[1] ^= ((u32 *) (&(v[ 6])))[0];
	ctx->h[0] ^= ((u32 *) (&(v[ 1])))[0];

	tmp = tsc(ctx);
	ctx->h[3] ^=        tmp;
	ctx->l[0] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[2] ^=        tmp;
	ctx->l[3] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[1] ^=        tmp;
	ctx->l[2] ^= 0xff & tmp;
	tmp = ROTL32(tmp, 8);
	ctx->h[0] ^=        tmp;
	ctx->l[1] ^= 0xff & tmp;

	return;
}

void ECRYPT_process_bytes(
  int action,
  ECRYPT_ctx* ctx,
  const u8* input,
  u8* output,
  u32 msglen)
{
	u32 tmp;

	while (msglen > 3)
	{
		((u32 *) output)[0] = ((u32 *) input)[0] ^ (tsc(ctx));
		input += 4; output += 4;
		msglen -= 4;
	}
	if (msglen == 0) return;
	tmp = tsc(ctx);
	output[0] = (u8) (tmp      );
	if (msglen == 1) return;
	output[1] = (u8) (tmp >>  8);
	if (msglen == 2) return;
	output[2] = (u8) (tmp >> 16);

	return;
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)
{
	u32 tmp;

	while (length > 3)
	{
		((u32 *) keystream)[0] = tsc(ctx);
		keystream += 4;
		length -= 4;
	}
	if (length == 0) return;
	tmp = tsc(ctx);
	keystream[0] = (u8) (tmp      );
	if (length == 1) return;
	keystream[1] = (u8) (tmp >>  8);
	if (length == 2) return;
	keystream[2] = (u8) (tmp >> 16);

	return;
}

void ECRYPT_process_blocks(
  int action,
  ECRYPT_ctx* ctx,
  const u8* input,
  u8* output,
  u32 blocks)
{
	while (blocks != 0)
	{
		((u32 *) output)[0] = ((u32 *) input)[0] ^ (tsc(ctx));
		input += 4; output += 4;
		blocks--;
	}
	return;
}

void ECRYPT_keystream_blocks(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 blocks)
{
	while (blocks != 0)
	{
		((u32 *) keystream)[0] = tsc(ctx);
		keystream += 4;
		blocks--;
	}

	return;
}


/**************************************************************************/
/**************************************************************************/
/*
	One more way to implement the nonlinear filter.
	Included here for reference.
*/
/*
	switch(
		((1&(ctx->l[3])) << 3) ^
		((1&(ctx->l[2])) << 2) ^
		((1&(ctx->l[1])) << 1) ^
		( 1&(ctx->l[0])      )
		)
	{
	case 15:
		tmpH = ROTL32(ROTL32(ctx->h[2], 7) + ROTR32(ctx->h[3], 2), 8)
			 + ROTR32(ROTL32(ctx->h[0], 7) +        ctx->h[1]    , 9);
		break;
	case 14:
		tmpH = ROTL32(ROTL32(ctx->h[2], 7) + ROTR32(ctx->h[3], 2), 8)
			 + ROTR32(ROTL32(ctx->h[1], 7) +        ctx->h[0]    , 9);
		break;
	case 13:
		tmpH = ROTL32(ROTL32(ctx->h[2], 7) + ROTR32(ctx->h[0], 2), 8)
			 + ROTR32(ROTL32(ctx->h[3], 7) +        ctx->h[1]    , 9);
		break;
	case 12:
		tmpH = ROTL32(ROTL32(ctx->h[2], 7) + ROTR32(ctx->h[1], 2), 8)
			 + ROTR32(ROTL32(ctx->h[3], 7) +        ctx->h[0]    , 9);
		break;
	case 11:
		tmpH = ROTL32(ROTL32(ctx->h[3], 7) + ROTR32(ctx->h[2], 2), 8)
			 + ROTR32(ROTL32(ctx->h[0], 7) +        ctx->h[1]    , 9);
		break;
	case 10:
		tmpH = ROTL32(ROTL32(ctx->h[3], 7) + ROTR32(ctx->h[2], 2), 8)
			 + ROTR32(ROTL32(ctx->h[1], 7) +        ctx->h[0]    , 9);
		break;
	case 9:
		tmpH = ROTL32(ROTL32(ctx->h[3], 7) + ROTR32(ctx->h[0], 2), 8)
			 + ROTR32(ROTL32(ctx->h[2], 7) +        ctx->h[1]    , 9);
		break;
	case 8:
		tmpH = ROTL32(ROTL32(ctx->h[3], 7) + ROTR32(ctx->h[1], 2), 8)
			 + ROTR32(ROTL32(ctx->h[2], 7) +        ctx->h[0]    , 9);
		break;
	case 7:
		tmpH = ROTL32(ROTL32(ctx->h[1], 7) + ROTR32(ctx->h[3], 2), 8)
			 + ROTR32(ROTL32(ctx->h[0], 7) +        ctx->h[2]    , 9);
		break;
	case 6:
		tmpH = ROTL32(ROTL32(ctx->h[0], 7) + ROTR32(ctx->h[3], 2), 8)
			 + ROTR32(ROTL32(ctx->h[1], 7) +        ctx->h[2]    , 9);
		break;
	case 5:
		tmpH = ROTL32(ROTL32(ctx->h[1], 7) + ROTR32(ctx->h[0], 2), 8)
			 + ROTR32(ROTL32(ctx->h[3], 7) +        ctx->h[2]    , 9);
		break;
	case 4:
		tmpH = ROTL32(ROTL32(ctx->h[0], 7) + ROTR32(ctx->h[1], 2), 8)
			 + ROTR32(ROTL32(ctx->h[3], 7) +        ctx->h[2]    , 9);
		break;
	case 3:
		tmpH = ROTL32(ROTL32(ctx->h[1], 7) + ROTR32(ctx->h[2], 2), 8)
			 + ROTR32(ROTL32(ctx->h[0], 7) +        ctx->h[3]    , 9);
		break;
	case 2:
		tmpH = ROTL32(ROTL32(ctx->h[0], 7) + ROTR32(ctx->h[2], 2), 8)
			 + ROTR32(ROTL32(ctx->h[1], 7) +        ctx->h[3]    , 9);
		break;
	case 1:
		tmpH = ROTL32(ROTL32(ctx->h[1], 7) + ROTR32(ctx->h[0], 2), 8)
			 + ROTR32(ROTL32(ctx->h[2], 7) +        ctx->h[3]    , 9);
		break;
	case 0:
		tmpH = ROTL32(ROTL32(ctx->h[0], 7) + ROTR32(ctx->h[1], 2), 8)
			 + ROTR32(ROTL32(ctx->h[2], 7) +        ctx->h[3]    , 9);
		break;
	}
*/


/*
	A more straightforward way to implement s-box application
	Included here for reference.
*/
/*
	tmpL = ~(ctx->l[3]) | (ctx->l[2]) | (ctx->l[1]);
	v1 = ~(ctx->l[0]);
	s2 = v1 ^ tmpL;
	t1 = (ctx->l[0]) ^ (~(ctx->l[3]) & (ctx->l[2]) & (ctx->l[1]));
	u2 = t1 ^ tmpL;

	tmpL = (ctx->l[3]) | (ctx->l[2]) | ~(ctx->l[0]);
	s3 = (ctx->l[1]) ^ ((ctx->l[3]) & (ctx->l[2]) & ~(ctx->l[0]));
	t0 = s3 ^ tmpL;
	u3 = t0;
	v0 = (ctx->l[1]) ^ ~tmpL;

	tmpL = (ctx->l[3]) | (ctx->l[1]) | (ctx->l[0]);
	v3 = ~(ctx->l[2]);
	u1 = (ctx->l[2]) ^ tmpL;
	t3 = ~u1;
	s1 = t3 ^ (ctx->l[3]) & (ctx->l[1]) & (ctx->l[0]);

	tmpL = (ctx->l[3]) ^ ((ctx->l[2]) & ~(ctx->l[1]) & (ctx->l[0]));
	s0 = ~tmpL;
	t2 = tmpL ^ ((ctx->l[2]) | ~(ctx->l[1]) | (ctx->l[0]));
	u0 = ~t2;
	v2 = (ctx->l[3]) ^ s0 ^ t2;

	ctx->l[3] = 0xff & (((pL1) & (pL0) & s3) ^ ((pL1) & (~pL0) & t3) ^ ((~pL1) & (pL0) & u3) ^ ((~pL1) & (~pL0) & v3));
	ctx->l[2] = 0xff & (((pL1) & (pL0) & s2) ^ ((pL1) & (~pL0) & t2) ^ ((~pL1) & (pL0) & u2) ^ ((~pL1) & (~pL0) & v2));
	ctx->l[1] = 0xff & (((pL1) & (pL0) & s1) ^ ((pL1) & (~pL0) & t1) ^ ((~pL1) & (pL0) & u1) ^ ((~pL1) & (~pL0) & v1));
	ctx->l[0] = 0xff & (((pL1) & (pL0) & s0) ^ ((pL1) & (~pL0) & t0) ^ ((~pL1) & (pL0) & u0) ^ ((~pL1) & (~pL0) & v0));

	tmpH = ~(ctx->h[3]) | (ctx->h[2]) | (ctx->h[1]);
	v1 = ~(ctx->h[0]);
	s2 = v1 ^ tmpH;
	t1 = (ctx->h[0]) ^ (~(ctx->h[3]) & (ctx->h[2]) & (ctx->h[1]));
	u2 = t1 ^ tmpH;

	tmpH = (ctx->h[3]) | (ctx->h[2]) | ~(ctx->h[0]);
	s3 = (ctx->h[1]) ^ ((ctx->h[3]) & (ctx->h[2]) & ~(ctx->h[0]));
	t0 = s3 ^ tmpH;
	u3 = t0;
	v0 = (ctx->h[1]) ^ ~tmpH;

	tmpH = (ctx->h[3]) | (ctx->h[1]) | (ctx->h[0]);
	v3 = ~(ctx->h[2]);
	u1 = (ctx->h[2]) ^ tmpH;
	t3 = ~u1;
	s1 = t3 ^ (ctx->h[3]) & (ctx->h[1]) & (ctx->h[0]);

	tmpH = (ctx->h[3]) ^ ((ctx->h[2]) & ~(ctx->h[1]) & (ctx->h[0]));
	s0 = ~tmpH;
	t2 = tmpH ^ ((ctx->h[2]) | ~(ctx->h[1]) | (ctx->h[0]));
	u0 = ~t2;
	v2 = (ctx->h[3]) ^ s0 ^ t2;

	ctx->h[3] = ((pH1) & (pH0) & s3) ^ ((pH1) & (~pH0) & t3) ^ ((~pH1) & (pH0) & u3) ^ ((~pH1) & (~pH0) & v3);
	ctx->h[2] = ((pH1) & (pH0) & s2) ^ ((pH1) & (~pH0) & t2) ^ ((~pH1) & (pH0) & u2) ^ ((~pH1) & (~pH0) & v2);
	ctx->h[1] = ((pH1) & (pH0) & s1) ^ ((pH1) & (~pH0) & t1) ^ ((~pH1) & (pH0) & u1) ^ ((~pH1) & (~pH0) & v1);
	ctx->h[0] = ((pH1) & (pH0) & s0) ^ ((pH1) & (~pH0) & t0) ^ ((~pH1) & (pH0) & u0) ^ ((~pH1) & (~pH0) & v0);
*/
