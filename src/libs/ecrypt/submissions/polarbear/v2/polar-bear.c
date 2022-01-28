/* polar-bear.c */

/*
* Polar Bear 2.0 stream cipher.
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
#include "aescrypt.c"
#include "whirltab.c"

/* ------------------------------------------------------------------------- */

/* Tables for multiplication in GF(2^16) */
static u32 TL00[256], TH00[256], TL01[256], TH01[256],
TL10[256], TH10[256], TL11[256], TH11[256];

/* The Rijndael S-box */
static u8 T8[256] =
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

/* Multiplication in GF(2^16) */
#define GF216MUL(TL, TH, x) (TL[x & 0xff] ^ TH[x >> 8]) 

/*
* Multiplies v(y) in GF(2^16) by y where GF(2^16) is defined by 
* y^16 = y^8 + y^7 + y^5 + 1 = 0x1a1
*/
#define YTIMES(v) (((v & 0x7fff) << 1) ^ (((v & 0x8000) >> 15) * 0x1a1))

/* Step R0 two steps */
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

/* Step R1 two steps */
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
* Computes Polar Bear 2.0 key schedule, done once per key.
* Memory is alloacted and a pointer is returned.
*/
void ECRYPT_keysetup(ECRYPT_ctx* ctx, 
					 const u8* key, 
					 u32 keysize,		/* Key size in bits. */ 
					 u32 ivsize)		/* IV size in bits.  */
{
	u8 K0[64];
	u64 K[8];          /* the round key K^r */
	u64 L[8];
	u8 *D8init = ctx->D8init;
	u8 expKey8[768];
	u32 *expKey32 = (u32*)expKey8; 
	u32 *W = (u32*)(expKey8 + 608); 
	u8 temp;
	u32 r,i;
	aes_ctx *ks = &ctx->ks;
	u32 *k_sch = ks->k_sch;

	if (keysize != 128)
	{
		fprintf(stderr,"Key size not implemented\n");
		exit(1);
	}

	if (ivsize > 248)
	{
		fprintf(stderr,"Illegal IV size\n");
		exit(1);
	}
	else
	{
		ctx->ivsize = ivsize;
	}

	/* The expanded key K^0 */
	for(i = 0; i < 16; i++)
	{
		K0[i] = key[i];
	}
	for(i = 16; i < 64; i++)
	{
		K0[i] = T8[ K0[i - 16] ];
	}

	/* The expanded key K^0 */
	K[0] = U8TO64_LITTLE(K0);
	K[1] = U8TO64_LITTLE(K0 + 8);
	K[2] = U8TO64_LITTLE(K0 + 16);
	K[3] = U8TO64_LITTLE(K0 + 24);
	K[4] = U8TO64_LITTLE(K0 + 32);
	K[5] = U8TO64_LITTLE(K0 + 40);
	K[6] = U8TO64_LITTLE(K0 + 48);
	K[7] = U8TO64_LITTLE(K0 + 56);

	/* Calculate K^1...K^12 */	
	for(r = 0; r < 12; r++)
	{
		/* Compute K^r from K^{r-1} */
		L[0] =
			C0[(int)(K[0] >> 56)       ] ^
			C1[(int)(K[7] >> 48) & 0xff] ^
			C2[(int)(K[6] >> 40) & 0xff] ^
			C3[(int)(K[5] >> 32) & 0xff] ^
			C4[(int)(K[4] >> 24) & 0xff] ^
			C5[(int)(K[3] >> 16) & 0xff] ^
			C6[(int)(K[2] >>  8) & 0xff] ^
			C7[(int)(K[1]      ) & 0xff] ^
			rc[r];
		L[1] =
			C0[(int)(K[1] >> 56)       ] ^
			C1[(int)(K[0] >> 48) & 0xff] ^
			C2[(int)(K[7] >> 40) & 0xff] ^
			C3[(int)(K[6] >> 32) & 0xff] ^
			C4[(int)(K[5] >> 24) & 0xff] ^
			C5[(int)(K[4] >> 16) & 0xff] ^
			C6[(int)(K[3] >>  8) & 0xff] ^
			C7[(int)(K[2]      ) & 0xff];
		L[2] =
			C0[(int)(K[2] >> 56)       ] ^
			C1[(int)(K[1] >> 48) & 0xff] ^
			C2[(int)(K[0] >> 40) & 0xff] ^
			C3[(int)(K[7] >> 32) & 0xff] ^
			C4[(int)(K[6] >> 24) & 0xff] ^
			C5[(int)(K[5] >> 16) & 0xff] ^
			C6[(int)(K[4] >>  8) & 0xff] ^
			C7[(int)(K[3]      ) & 0xff];
		L[3] =
			C0[(int)(K[3] >> 56)       ] ^
			C1[(int)(K[2] >> 48) & 0xff] ^
			C2[(int)(K[1] >> 40) & 0xff] ^
			C3[(int)(K[0] >> 32) & 0xff] ^
			C4[(int)(K[7] >> 24) & 0xff] ^
			C5[(int)(K[6] >> 16) & 0xff] ^
			C6[(int)(K[5] >>  8) & 0xff] ^
			C7[(int)(K[4]      ) & 0xff];
		L[4] =
			C0[(int)(K[4] >> 56)       ] ^
			C1[(int)(K[3] >> 48) & 0xff] ^
			C2[(int)(K[2] >> 40) & 0xff] ^
			C3[(int)(K[1] >> 32) & 0xff] ^
			C4[(int)(K[0] >> 24) & 0xff] ^
			C5[(int)(K[7] >> 16) & 0xff] ^
			C6[(int)(K[6] >>  8) & 0xff] ^
			C7[(int)(K[5]      ) & 0xff];
		L[5] =
			C0[(int)(K[5] >> 56)       ] ^
			C1[(int)(K[4] >> 48) & 0xff] ^
			C2[(int)(K[3] >> 40) & 0xff] ^
			C3[(int)(K[2] >> 32) & 0xff] ^
			C4[(int)(K[1] >> 24) & 0xff] ^
			C5[(int)(K[0] >> 16) & 0xff] ^
			C6[(int)(K[7] >>  8) & 0xff] ^
			C7[(int)(K[6]      ) & 0xff];
		L[6] =
			C0[(int)(K[6] >> 56)       ] ^
			C1[(int)(K[5] >> 48) & 0xff] ^
			C2[(int)(K[4] >> 40) & 0xff] ^
			C3[(int)(K[3] >> 32) & 0xff] ^
			C4[(int)(K[2] >> 24) & 0xff] ^
			C5[(int)(K[1] >> 16) & 0xff] ^
			C6[(int)(K[0] >>  8) & 0xff] ^
			C7[(int)(K[7]      ) & 0xff];
		L[7] =
			C0[(int)(K[7] >> 56)       ] ^
			C1[(int)(K[6] >> 48) & 0xff] ^
			C2[(int)(K[5] >> 40) & 0xff] ^
			C3[(int)(K[4] >> 32) & 0xff] ^
			C4[(int)(K[3] >> 24) & 0xff] ^
			C5[(int)(K[2] >> 16) & 0xff] ^
			C6[(int)(K[1] >>  8) & 0xff] ^
			C7[(int)(K[0]      ) & 0xff];

		for(i = 0; i < 8; i++)
		{
			K[i] = L[i];
			*expKey32++ = K[i];
			*expKey32++ = K[i] >> 32;
		}
	}
	
	/* Init ctx->ks */
	ks->n_blk = (16 & ~3) | 1;
	ks->n_rnd = 4;
	for(i = 0; i < 40; i++)
		k_sch[i] = W[i];

	/* Initiaze D8init to T8 */
	memcpy(D8init, T8, 256);

	/* Permutate D8 */
	for(i = 0; i < 768; i++)
	{
		temp = D8init[i % 256];
		D8init[i % 256] = D8init[expKey8[i]];
		D8init[expKey8[i]] = temp;
	}
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

	memcpy(ctx->D8, ctx->D8init, 256);

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

	for (j = 0; j < 32; j++)
	{
		ct[j] = 0;
	}

	/* Four rounds Rijndael */
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
	u32 temp0, temp1, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14;
	u32 *R0 = ctx->R0, *R1 = ctx->R1;
	u8 *D8 = ctx->D8;
	u32 *in = (u32*)input;
	u32 *out = (u32*)output;

	while (msglen >= 60)
	{

		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\
		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\

		/* Calculate alphas and betas */
		alpha0 = R0[8] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha3 = R0[7] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b0 = beta2;
		b0 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b0 ^= (beta3 << 8);
		b0 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b0 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b0 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b1 = beta2;
		b1 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b1 ^= (beta3 << 8);
		b1 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b1 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b1 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b2 = beta2;
		b2 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b2 ^= (beta3 << 8);
		b2 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b2 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b2 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b3 = beta2;
		b3 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b3 ^= (beta3 << 8);
		b3 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b3 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b3 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b4 = beta2;
		b4 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b4 ^= (beta3 << 8);
		b4 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b4 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b4 ^= temp1;;

		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\
		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\

		/* Calculate alphas and betas */
		alpha3 = R0[7] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha0 = R0[8] >> 8;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b5 = beta2;
		b5 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b5 ^= (beta3 << 8);
		b5 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b5 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b5 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b6 = beta2;
		b6 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b6 ^= (beta3 << 8);
		b6 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b6 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b6 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b7 = beta2;
		b7 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b7 ^= (beta3 << 8);
		b7 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b7 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b7 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b8 = beta2;
		b8 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b8 ^= (beta3 << 8);
		b8 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b8 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b8 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b9 = beta2;
		b9 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b9 ^= (beta3 << 8);
		b9 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b9 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b9 ^= temp1;

		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\
		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\

		/* Calculate alphas and betas */
		alpha3 = R0[7] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha0 = R0[8] >> 8;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b10 = beta2;
		b10 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b10 ^= (beta3 << 8);
		b10 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b10 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b10 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b11 = beta2;
		b11 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b11 ^= (beta3 << 8);
		b11 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b11 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b11 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b12 = beta2;
		b12 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b12 ^= (beta3 << 8);
		b12 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b12 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b12 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b13 = beta2;
		b13 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b13 ^= (beta3 << 8);
		b13 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b13 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b13 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b14 = beta2;
		b14 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b14 ^= (beta3 << 8);
		b14 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b14 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b14 ^= temp1;


		/* Output 160 bits */
		*(out++) = *(in++) ^ U32TO32_LITTLE(b0);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b1);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b2);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b3);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b4);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b5);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b6);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b7);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b8);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b9);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b10);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b11);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b12);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b13);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b14);

		msglen -= 60;
	}

	if (msglen >= 40)
	{
		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\
		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\

		/* Calculate alphas and betas */
		alpha0 = R0[8] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha3 = R0[7] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b0 = beta2;
		b0 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b0 ^= (beta3 << 8);
		b0 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b0 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b0 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b1 = beta2;
		b1 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b1 ^= (beta3 << 8);
		b1 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b1 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b1 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b2 = beta2;
		b2 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b2 ^= (beta3 << 8);
		b2 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b2 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b2 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b3 = beta2;
		b3 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b3 ^= (beta3 << 8);
		b3 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b3 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b3 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b4 = beta2;
		b4 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b4 ^= (beta3 << 8);
		b4 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b4 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b4 ^= temp1;;

		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\
		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\

		/* Calculate alphas and betas */
		alpha3 = R0[7] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha0 = R0[8] >> 8;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b5 = beta2;
		b5 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b5 ^= (beta3 << 8);
		b5 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b5 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b5 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b6 = beta2;
		b6 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b6 ^= (beta3 << 8);
		b6 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b6 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b6 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b7 = beta2;
		b7 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b7 ^= (beta3 << 8);
		b7 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b7 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b7 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b8 = beta2;
		b8 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b8 ^= (beta3 << 8);
		b8 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b8 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b8 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b9 = beta2;
		b9 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b9 ^= (beta3 << 8);
		b9 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b9 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b9 ^= temp1;

		/* Output 160 bits */
		*(out++) = *(in++) ^ U32TO32_LITTLE(b0);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b1);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b2);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b3);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b4);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b5);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b6);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b7);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b8);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b9);

		msglen -= 40;
	}

	if (msglen >= 20)
	{
		R0[8] = GF216MUL(TL00, TH00, R0[1]) ^ GF216MUL(TL01, TH01, R0[2]);\
		R0[7] = GF216MUL(TL00, TH00, R0[0]) ^ GF216MUL(TL01, TH01, R0[1]);\

		/* Calculate alphas and betas */
		alpha3 = R0[7] & 0xff;
		alpha2 = R0[7] >> 8;
		alpha1 = R0[8] & 0xff;
		alpha0 = R0[8] >> 8;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b0 = beta2;
		b0 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b0 ^= (beta3 << 8);
		b0 ^= (beta1 << 16);

		R1[9] = GF216MUL(TL10, TH10, R1[0]) ^ GF216MUL(TL11, TH11, R1[5]);\
		R1[0] = GF216MUL(TL10, TH10, R1[1]) ^ GF216MUL(TL11, TH11, R1[6]);\

		/* Calculate alphas and betas */
		alpha0 = R1[0] >> 8;
		alpha1 = R1[0] & 0xff;
		alpha2 = R1[9] >> 8;
		alpha3 = R1[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[5] += temp0;
		R0[5] &= 0xffff;
		b0 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b0 ^= temp1;;

		R0[9] = GF216MUL(TL00, TH00, R0[2]) ^ GF216MUL(TL01, TH01, R0[3]);\
		R0[0] = GF216MUL(TL00, TH00, R0[3]) ^ GF216MUL(TL01, TH01, R0[4]);\

		/* Calculate alphas and betas */
		alpha0 = R0[0] >> 8;
		alpha1 = R0[0] & 0xff;
		alpha2 = R0[9] >> 8;
		alpha3 = R0[9] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b1 = beta2;
		b1 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b1 ^= (beta3 << 8);
		b1 ^= (beta1 << 16);

		R1[1] = GF216MUL(TL10, TH10, R1[2]) ^ GF216MUL(TL11, TH11, R1[7]);\
		R1[2] = GF216MUL(TL10, TH10, R1[3]) ^ GF216MUL(TL11, TH11, R1[8]);\

		/* Calculate alphas and betas */
		alpha2 = R1[1] >> 8;
		alpha3 = R1[1] & 0xff;
		alpha0 = R1[2] >> 8;
		alpha1 = R1[2] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[7] += temp0;
		R0[7] &= 0xffff;
		b1 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b1 ^= temp1;;

		R0[1] = GF216MUL(TL00, TH00, R0[4]) ^ GF216MUL(TL01, TH01, R0[5]);\
		R0[2] = GF216MUL(TL00, TH00, R0[5]) ^ GF216MUL(TL01, TH01, R0[6]);\

		/* Calculate alphas and betas */
		alpha0 = R0[2] >> 8;
		alpha1 = R0[2] & 0xff;
		alpha2 = R0[1] >> 8;
		alpha3 = R0[1] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b2 = beta2;
		b2 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b2 ^= (beta3 << 8);
		b2 ^= (beta1 << 16);

		R1[3] = GF216MUL(TL10, TH10, R1[4]) ^ GF216MUL(TL11, TH11, R1[9]);\
		R1[4] = GF216MUL(TL10, TH10, R1[5]) ^ GF216MUL(TL11, TH11, R1[0]);\

		/* Calculate alphas and betas */
		alpha2 = R1[3] >> 8;
		alpha3 = R1[3] & 0xff;
		alpha0 = R1[4] >> 8;
		alpha1 = R1[4] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[9] += temp0;
		R0[9] &= 0xffff;
		b2 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b2 ^= temp1;;

		R0[3] = GF216MUL(TL00, TH00, R0[6]) ^ GF216MUL(TL01, TH01, R0[7]);\
		R0[4] = GF216MUL(TL00, TH00, R0[7]) ^ GF216MUL(TL01, TH01, R0[8]);\

		/* Calculate alphas and betas */
		alpha0 = R0[4] >> 8;
		alpha1 = R0[4] & 0xff;
		alpha2 = R0[3] >> 8;
		alpha3 = R0[3] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b3 = beta2;
		b3 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b3 ^= (beta3 << 8);
		b3 ^= (beta1 << 16);

		R1[5] = GF216MUL(TL10, TH10, R1[6]) ^ GF216MUL(TL11, TH11, R1[1]);\
		R1[6] = GF216MUL(TL10, TH10, R1[7]) ^ GF216MUL(TL11, TH11, R1[2]);\

		/* Calculate alphas and betas */
		alpha2 = R1[5] >> 8;
		alpha3 = R1[5] & 0xff;
		alpha0 = R1[6] >> 8;
		alpha1 = R1[6] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[1] += temp0;
		R0[1] &= 0xffff;
		b3 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b3 ^= temp1;;
		R0[5] = GF216MUL(TL00, TH00, R0[8]) ^ GF216MUL(TL01, TH01, R0[9]);\
		R0[6] = GF216MUL(TL00, TH00, R0[9]) ^ GF216MUL(TL01, TH01, R0[0]);\

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b4 = beta2;
		b4 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b4 ^= (beta3 << 8);
		b4 ^= (beta1 << 16);

		R1[7] = GF216MUL(TL10, TH10, R1[8]) ^ GF216MUL(TL11, TH11, R1[3]);\
		R1[8] = GF216MUL(TL10, TH10, R1[9]) ^ GF216MUL(TL11, TH11, R1[4]);\

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b4 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b4 ^= temp1;;

		/* Output 32 bits */
		*(out++) = *(in++) ^ U32TO32_LITTLE(b0);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b1);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b2);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b3);
		*(out++) = *(in++) ^ U32TO32_LITTLE(b4);

		msglen -= 20;
	}

	while (msglen)
	{
		STEP_R0_TWO_STEPS;

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b0 = beta2;
		b0 ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b0 ^= (beta3 << 8);
		b0 ^= (beta1 << 16);

		STEP_R1_TWO_STEPS;

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b0 ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b0 ^= temp1;;

		/* Output 32 bits */
		*(out++) = *(in++) ^ U32TO32_LITTLE(b0);
		msglen -= 4;
	}
}

void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{
	u32 alpha0, alpha1, alpha2, alpha3, beta0, beta1, beta2, beta3;
	u32 temp0, temp1, b;
	u32 *R0 = ctx->R0, *R1 = ctx->R1;
	u8 *D8 = ctx->D8;
	u32 *ks = (u32*)keystream;

	while (length)
	{
		STEP_R0_TWO_STEPS;

		/* Calculate alphas and betas */
		alpha0 = R0[6] >> 8;
		alpha1 = R0[6] & 0xff;
		alpha2 = R0[5] >> 8;
		alpha3 = R0[5] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha2] = beta0;
		D8[alpha0] = beta2;
		b = beta2;
		b ^= (beta0 << 24);

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];

		D8[alpha3] = beta1;
		D8[alpha1] = beta3;
		b ^= (beta3 << 8);
		b ^= (beta1 << 16);

		STEP_R1_TWO_STEPS;

		/* Calculate alphas and betas */
		alpha2 = R1[7] >> 8;
		alpha3 = R1[7] & 0xff;
		alpha0 = R1[8] >> 8;
		alpha1 = R1[8] & 0xff;

		/* Swap(D8[alpha0], D8[alpha2]) */
		beta0 = D8[alpha0];
		beta2 = D8[alpha2];
		D8[alpha0] = beta2;
		D8[alpha2] = beta0;

		/* Swap(D8[alpha1], D8[alpha3]) */
		beta1 = D8[alpha1];
		beta3 = D8[alpha3];
		D8[alpha1] = beta3;
		D8[alpha3] = beta1;

		/* Update R0[3] */
		temp0 = beta2 | (beta1 << 8);
		R0[3] += temp0;
		R0[3] &= 0xffff;
		b ^= (temp0 << 16);
		temp1 = beta3 | (beta0 << 8);
		b ^= temp1;;

		/* Output 32 bits */
		*(ks++) = U32TO32_LITTLE(b);
		length -= 4;
	}
}

