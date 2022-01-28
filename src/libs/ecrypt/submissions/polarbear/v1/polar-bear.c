/* polar-bear.c */

/*
 * "Polar Bear" stream cipher implementaion.
 *
 * This code is provided "as is" with no warranty or support. Requires a
 * (slightly modified) Rijndael (see documentation).
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ecrypt-sync.h"
#include "aes.h"
#include "aestab.c"
#include "aeskey.c"
#include "aescrypt.c"

/* ------------------------------------------------------------------------- */

/* Tables for multiplication in GF(2^16) */
static u32 TL00[256], TH00[256], TL01[256], TH01[256],
           TL10[256], TH10[256], TL11[256], TH11[256];

/* S-box, taken from AES */
static u8 T8_init[256] =
{
  99, 124, 119, 123, 242, 107, 111, 197,  48,   1, 103,  43, 254, 215, 171, 118, 
 202, 130, 201, 125, 250,  89,  71, 240, 173, 212, 162, 175, 156, 164, 114, 192, 
 183, 253, 147,  38,  54,  63, 247, 204,  52, 165, 229, 241, 113, 216,  49,  21, 
   4, 199,  35, 195,  24, 150,   5, 154,   7,  18, 128, 226, 235,  39, 178, 117, 
   9, 131,  44,  26,  27, 110,  90, 160,  82,  59, 214, 179,  41, 227,  47, 132, 
  83, 209,   0, 237,  32, 252, 177,  91, 106, 203, 190,  57,  74,  76,  88, 207, 
 208, 239, 170, 251,  67,  77,  51, 133,  69, 249,   2, 127,  80,  60, 159, 168, 
  81, 163,  64, 143, 146, 157,  56, 245, 188, 182, 218,  33,  16, 255, 243, 210, 
 205,  12,  19, 236,  95, 151,  68,  23, 196, 167, 126,  61, 100,  93,  25, 115, 
  96, 129,  79, 220,  34,  42, 144, 136,  70, 238, 184,  20, 222,  94,  11, 219, 
 224,  50,  58,  10,  73,   6,  36,  92, 194, 211, 172,  98, 145, 149, 228, 121, 
 231, 200,  55, 109, 141, 213,  78, 169, 108,  86, 244, 234, 101, 122, 174,   8, 
 186, 120,  37,  46,  28, 166, 180, 198, 232, 221, 116,  31,  75, 189, 139, 138, 
 112,  62, 181, 102,  72,   3, 246,  14,  97,  53,  87, 185, 134, 193,  29, 158, 
 225, 248, 152,  17, 105, 217, 142, 148, 155,  30, 135, 233, 206,  85,  40, 223, 
 140, 161, 137,  13, 191, 230,  66, 104,  65, 153,  45,  15, 176,  84, 187,  22
};

/* ------------------------------------------------------------------------- */

/* Init of S-box on each message */
#define InitD8(tab) memcpy(tab, T8_init, 256)

/* Multiplication in GF(2^16) */
#define GF216MUL(TL, TH, x) (TL[x & 0xff] ^ TH[x >> 8]) 

/*
 * Multiplies v(y) in GF(2^16) by y where GF(2^16) is defined by 
 * y^16 = y^8 + y^7 + y^5 + 1 = 0x1a1
 */
#define YTIMES(v) (((v & 0x7fff) << 1) ^ (((v & 0x8000) >> 15) * 0x1a1))

/* Steps the registers */
#define STEP_R0_TWO_STEPS {\
	temp0 = R0[0];\
	temp1 = R0[1];\
	R0[0] = R0[2];\
	R0[1] = R0[3];\
	R0[2] = R0[4];\
	R0[3] = R0[5];\
	R0[4] = R0[6];\
	R0[5] = GF216MUL(TL00, TH00, temp0) ^ GF216MUL(TL01, TH01, temp1);\
	R0[6] = GF216MUL(TL00, TH00, temp1) ^ GF216MUL(TL01, TH01, R0[0]);\
	}

#define STEP_R0_THREE_STEPS {\
	temp0 = R0[0];\
	R0[0] = R0[3];\
	R0[3] = R0[6];\
	R0[6] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[0]);\
	temp1 = R0[1];\
	R0[1] = R0[4];\
	R0[4] = GF216MUL(TL00, TH00, temp0) ^ GF216MUL(TL01, TH01, temp1);\
	temp0 = R0[2];\
	R0[2] = R0[5];\
	R0[5] = GF216MUL(TL00, TH00, temp1) ^ GF216MUL(TL01, TH01, temp0);\
	}

#define STEP_R1_TWO_STEPS {\
	temp0 = R1[0];\
	temp1 = R1[1];\
	R1[0] = R1[2];\
	R1[1] = R1[3];\
	R1[2] = R1[4];\
	R1[3] = R1[5];\
	R1[4] = R1[6];\
	R1[5] = R1[7];\
	R1[6] = R1[8];\
	R1[7] = GF216MUL(TL10, TH10, temp0) ^ GF216MUL(TL11, TH11, R1[3]);\
	R1[8] = GF216MUL(TL10, TH10, temp1) ^ GF216MUL(TL11, TH11, R1[4]);\
	}

#define STEP_R1_THREE_STEPS {\
	temp0 = R1[0];\
	R1[0] = R1[3];\
	R1[3] = R1[6];\
	R1[6] = GF216MUL(TL10, TH10, temp0) ^ GF216MUL(TL11, TH11, R1[5]);\
	temp0 = R1[1];\
	R1[1] = R1[4];\
	R1[4] = R1[7];\
	R1[7] = GF216MUL(TL10, TH10, temp0) ^ GF216MUL(TL11, TH11, R1[3]);\
	temp0 = R1[2];\
	R1[2] = R1[5];\
	R1[5] = R1[8];\
	R1[8] = GF216MUL(TL10, TH10, temp0) ^ GF216MUL(TL11, TH11, R1[4]);\
	}

/* ------------------------------------------------------------------------- */

/* Initiates tables for GF(2^16) arithmetic */
void ECRYPT_init(void)
{
	u16 ALOG[65536], DLOG[65536];
	u16 g;
	int i;

	for (i = 0; i <= 0xffff; i++)
	{
		DLOG[i] = ALOG[i] = 0;
	}

	g = 1;
	DLOG[g] = 0;
	ALOG[0] = g;

	for (i = 1; i < 0xffff; i++)
	{
		g = YTIMES(g); /* g = g*y; */
		if (DLOG[g] || ALOG[i])
			fprintf(stderr,"Table error, collision, i = %04x\n",i);
		DLOG[g] = i;
		ALOG[i] = g;
	}

	for (i = 1; i < 0xffff; i++) 
		if (!(DLOG[g] && ALOG[i]))
			fprintf(stderr,"Table error, empty\n");
	for (i = 1; i < 0xffff; i++) 
		if (!(DLOG[g] && ALOG[i]))
			fprintf(stderr,"Table error, empty\n");

	for (i = 0; i <= 0xff; i++)
	{
		g = (DLOG[i] + DLOG[23787]) % 0xffff;
		TL00[i] = ALOG[g];
		TH00[i] = ALOG[(g + 8) % 0xffff];

		g = (DLOG[i] + DLOG[35674]) % 0xffff;
		TL01[i] = ALOG[g];
		TH01[i] = ALOG[(g + 8) % 0xffff];

		g = (DLOG[i] + DLOG[11362]) % 0xffff;
		TL10[i] = ALOG[g];
		TH10[i] = ALOG[(g + 8) % 0xffff];

		g = (DLOG[i] + DLOG[26778]) % 0xffff;
		TL11[i] = ALOG[g];
		TH11[i] = ALOG[(g + 8) % 0xffff];
	}
}


/*
 * Computes Polar Bear key schedule, done once per key. Memory is alloacted 
 * and a pointer is returned.
 */
void ECRYPT_keysetup(ECRYPT_ctx* ctx, 
                     const u8* key, 
                     u32 keysize,		/* Key size in bits. */ 
                     u32 ivsize)		/* IV size in bits.  */ 
{
	if (keysize != 128)
	{
		fprintf(stderr,"Key size not implemented\n");
		exit(1);
	}

	if (ivsize > 240)
	{
		fprintf(stderr,"Illegal IV size\n");
		exit(1);
	}
	else
	{
		ctx->ivsize = ivsize;
	}

	aes_enc_key(key, keysize/8, &ctx->ks);
}


/* Init of Polar Bear on each new message/IV. The context, p, needs to have
 * been allocated and the ks field of the context needs to point to the key
 * schedule before calling
 */
void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
	int j;
	int n_iv = ctx->ivsize / 8;
	u32 *R0 = ctx->R0, *R1 = ctx->R1;
	u8 pt[32], ct[32];
	u16 *ct16 = (u16*)ct;
	InitD8(ctx->D8);
	ctx->shift  = 0;

	/* Pad IV with 0x80 and zeroes */
	for (j = 0; j < n_iv; j++)
	{
		pt[j] = iv[j];
	}

	pt[n_iv] = 0x80;

	for (j = n_iv+1; j < 32; j++)
	{
		pt[j] = 0;
	}

	/* Five rounds Rijndael */
	aes_enc_blk(pt, ct, &ctx->ks);

	/* Initialize R0 and R1 */
	for (j = 0; j < 7; j++)
	{
	  R0[j] = U8TO16_LITTLE(ct16);
	  ct16++;
	}

	for (j = 0; j < 9; j++) {
	  R1[j] = U8TO16_LITTLE(ct16);
	  ct16++;
	}
}


void ECRYPT_process_bytes(int action,		/* 0 = encrypt; 1 = decrypt; */
                          ECRYPT_ctx* ctx, 
                          const u8* input, 
                          u8* output, 
                          u32 msglen)		/* Message length in bytes. */ 
{
	u32 alpha0, alpha1, alpha2, alpha3, beta0, beta1, beta2, beta3;
	u32 temp0, temp1, b;
	u32 *R0 = ctx->R0, *R1 = ctx->R1;
	u32 *S = &ctx->shift;
	u8 *D8 = ctx->D8;
	u32 *in = (u32*)input;
	u32 *out = (u32*)output;

	while (msglen)
	{
		/* Step R0 two or three steps*/
		if (*S & 0x4000)
		{
			STEP_R0_THREE_STEPS;
		}
		else
		{
			STEP_R0_TWO_STEPS;
		}

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];

		/* Swap(D8[alpha0], D8[alpha2]) */
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b = (beta2 << 8);
		b ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		temp0 = D8[alpha3];
		D8[alpha3] = D8[alpha1];
		D8[alpha1] = temp0;
		b ^= beta3;
		b ^= (beta1 << 16);

		/* Step R1 two or three steps*/
		if (*S & 0x8000)
		{
			STEP_R1_THREE_STEPS;
		}
		else
		{
			STEP_R1_TWO_STEPS;
		}

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];

		/* Swap(D8[alpha0], D8[alpha2]) */
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		temp0 = D8[alpha1];
		D8[alpha1] = D8[alpha3];
		D8[alpha3] = temp0;

		/* Calculate gamma1 and update R0[5] */
		temp0 = beta3 | (beta2 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b ^= temp0;
		
		/* Calculate gamma0 and update S */
		temp1 = beta1 | (beta0 << 8);
		*S += temp1;
		b ^= (temp1 << 16);

		/* Output 32 bits */
		*(out++) = *(in++) ^ U32TO32_LITTLE(b);
		msglen -= 4;
	}
}
	
void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{
	u32 alpha0, alpha1, alpha2, alpha3, beta0, beta1, beta2, beta3;
	u32 temp0, temp1, b;
	u32 *R0 = ctx->R0, *R1 = ctx->R1;
	u32 *S = &ctx->shift;
	u8 *D8 = ctx->D8;
	u32 *ks = (u32*)keystream;

	while (length)
	{
		/* Step R0 two or three steps*/
		if (*S & 0x4000)
		{
			STEP_R0_THREE_STEPS;
		}
		else
		{
			STEP_R0_TWO_STEPS;
		}

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];

		/* Swap(D8[alpha0], D8[alpha2]) */
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b = (beta2 << 8);
		b ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		temp0 = D8[alpha3];
		D8[alpha3] = D8[alpha1];
		D8[alpha1] = temp0;
		b ^= beta3;
		b ^= (beta1 << 16);

		/* Step R1 two or three steps*/
		if (*S & 0x8000)
		{
			STEP_R1_THREE_STEPS;
		}
		else
		{
			STEP_R1_TWO_STEPS;
		}

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];

		/* Swap(D8[alpha0], D8[alpha2]) */
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		temp0 = D8[alpha3];
		D8[alpha3] = D8[alpha1];
		D8[alpha1] = temp0;

		/* Calculate gamma1 and update R0[5] */
		temp0 = beta3 | (beta2 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b ^= temp0;
		
		/* Calculate gamma0 and update S */
		temp1 = beta1 | (beta0 << 8);
		*S += temp1;
		b ^= (temp1 << 16);

		/* Output 32 bits */
		*(ks++) = U32TO32_LITTLE(b);
		length -= 4;
	}
}
