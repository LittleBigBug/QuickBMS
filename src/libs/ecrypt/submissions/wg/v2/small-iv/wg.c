/* This files containsthe implementation
* of functions defined in ecrypt-synch.h
* The IVs of length 32 and 64 bits are allowed ***
*/

#include "ecrypt-sync.h"

#define ROTL29(v, n) \
	(unsigned)(((v) << (n)) | ((v) >> (29 - (n))))& 0xFFFFFFF8
#define ROTR29(v, n) ROTL29(v, 29 - (n))

void mult(u32, u32, u32*);

void ECRYPT_init()
{
}

/****************************************************/

void ECRYPT_keysetup(
					 ECRYPT_ctx* ctx, 
					 const u8* key, 
					 u32 keysize,                /* Key size in bits. */ 
					 u32 ivsize)                /* IV size in bits. */ 
{


	u32 tmp, l, m, i, p, j, n, gamma;
	/* For WG transformation */
	u32 alpha[4];
	u32 q[4]; 
	u32 alpha_inv, alpha_inv9; 

	ctx->KEY_LEN=keysize;
	ctx->IV_LEN=ivsize;

	l=0;
	for(i=0; i < keysize; i=i+8){
		ctx->KEY[l]=key[l]; l++;
	}

	for(p=0;p<11;p++){
		ctx->S[p]=0x00000000;
	}
	
	/* put key in LFSR*/
	l=0; m=0;
	for(j=0;j<keysize;j=j+16){
		tmp=(unsigned int)key[m]&0x000000FF; tmp<<=24; ctx->S[l]^=tmp;
		tmp=(unsigned int)key[m+1]&0x000000FF; tmp<<=16; ctx->S[l]^=tmp;
		l=l+1; m=m+2;
	}
	tmp=(unsigned int)key[0]&0x000000FF; tmp<<=24; ctx->S[8]^=tmp;
	tmp=(unsigned int)key[1]&0x000000FF; tmp<<=16; ctx->S[8]^=tmp;
	tmp=(unsigned int)key[2]&0x000000FF; tmp<<=24; tmp^=0xFF000000; ctx->S[9]^=tmp;
	tmp=(unsigned int)key[3]&0x000000FF; tmp<<=16; tmp^=0x00FF0000; ctx->S[9]^=tmp;
	tmp=(unsigned int)key[4]&0x000000FF; tmp<<=24; ctx->S[9]^=tmp;
	tmp=(unsigned int)key[5]&0x000000FF; tmp<<=16; ctx->S[9]^=tmp;

	gamma=0x7F3FC9B0; /* in LFSR feedback pol */
	
	/* Key Expansion */
	for(n=0;n<44;n++){

		/* WG  transformation of alpha[0] */
		alpha[0] = ctx->S[10];	
		alpha[0]^= 0xFFFFFFF8; /* add 1 */
		alpha[1] = ROTR29(alpha[0], 9);
		alpha[2] = ROTR29(alpha[0], 10);
		alpha[3] = ROTR29(alpha[0], 19);

		/* find alpha^-1  and alpha^(-2^9) here */
		alpha_inv=alpha[0];
		/*4*/
		alpha_inv=ROTL29(alpha_inv, 16);
		mult(alpha_inv, ROTR29(alpha_inv, 8), &alpha_inv);
		mult(alpha_inv, alpha[0], &alpha_inv);
		/*3*/
		alpha_inv=ROTL29(alpha_inv, 8);
		mult(alpha_inv, ROTR29(alpha_inv, 4), &alpha_inv);
		mult(alpha_inv, alpha[0], &alpha_inv);
		/*2*/
		alpha_inv=ROTL29(alpha_inv, 4);
		mult(alpha_inv, ROTR29(alpha_inv, 2), &alpha_inv);
		/*1*/
		mult(alpha_inv, ROTR29(alpha_inv, 1), &alpha_inv);

		alpha_inv9 = ROTR29(alpha_inv, 9); /* alpha^(-2^9) */


		/* WG exponents coputation */
		mult(alpha[2], alpha[0], &q[0]); /*q1*/
		mult(alpha[3], alpha[0], &q[1]);
		mult(q[1], alpha_inv9, &q[2]); /*q3*/
		mult(q[1], alpha[1], &q[1]);     /*q2*/
		mult(alpha[3], alpha[2], &q[3]);
		mult(q[3], alpha_inv, &q[3]); /*q4*/


		/* alpha+alpha^q1+alpha^q2+alpha^q3+alpha^q4 */
		alpha[0]^=q[0];
		alpha[0]^=q[1];
		alpha[0]^=q[2];
		alpha[0]^=q[3];

		alpha[0]^= 0xFFFFFFF8; /* add 1 */


		/* clock the LFSR */
		mult(ctx->S[10], gamma, &ctx->S[10]); /* S[10]*gamma */

		/* x^11+x^10+x^9+x^6+x^3+x+alpha */
		ctx->S[10]^=ctx->S[9]; ctx->S[10]^=ctx->S[7]; ctx->S[10]^=ctx->S[4];ctx->S[10]^=ctx->S[1];ctx->S[10]^=ctx->S[0];
		ctx->S[10]^=alpha[0]; alpha[1]=ctx->S[10];

		ctx->S[10]=ctx->S[9]; ctx->S[9]=ctx->S[8]; ctx->S[8]=ctx->S[7]; ctx->S[7]=ctx->S[6];
		ctx->S[6]=ctx->S[5]; ctx->S[5]=ctx->S[4]; ctx->S[4]=ctx->S[3]; ctx->S[3]=ctx->S[2];
		ctx->S[2]=ctx->S[1];ctx->S[1]=ctx->S[0]; ctx->S[0]=alpha[1]; /*LFSR clocked*/
		
	}



}

/*****************************************************/

void ECRYPT_ivsetup(
					ECRYPT_ctx* ctx, 
					const u8* iv)
{

	u32 tmp, l, m, p, j, r, n, gamma;
	/* for WG transformation */
	u32 alpha[4];
	u32 q[4];
	u32 alpha_inv, alpha_inv9;

	for(p=0;p<11;p++)ctx->S[p]=0x00000000;
	
	l=0; m=0;
	for(j=0;j<ctx->KEY_LEN;j=j+16){
		tmp=(unsigned int)ctx->KEY[m]&0x000000FF; tmp<<=24; ctx->S[l]^=tmp;
		tmp=(unsigned int)ctx->KEY[m+1]&0x000000FF; tmp<<=16; ctx->S[l]^=tmp;
		l=l+1; m=m+2;
	}
	tmp=(unsigned int)ctx->KEY[0]&0x000000FF; tmp<<=24; ctx->S[8]^=tmp;
	tmp=(unsigned int)ctx->KEY[1]&0x000000FF; tmp<<=16; ctx->S[8]^=tmp;
	tmp=(unsigned int)ctx->KEY[2]&0x000000FF; tmp<<=24; tmp^=0xFF000000; ctx->S[9]^=tmp;
	tmp=(unsigned int)ctx->KEY[3]&0x000000FF; tmp<<=16; tmp^=0x00FF0000; ctx->S[9]^=tmp;
	tmp=(unsigned int)ctx->KEY[4]&0x000000FF; tmp<<=24; ctx->S[10]^=tmp;
	tmp=(unsigned int)ctx->KEY[5]&0x000000FF; tmp<<=16; ctx->S[10]^=tmp;
	

	/* put IV in lfsr */
	l=0;
	for(r=0; r<ctx->IV_LEN; r=r+8){
		tmp=(unsigned int)iv[l]&0x000000FF; tmp<<=8; ctx->S[l]^=tmp;
		l=l+1;
	}

	gamma=0x7F3FC9B0; /* in LFSR feedback pol */

	/* Key Expansion */
	for(n=0;n<44;n++){

		/* WG  transformation of alpha[0] */
		alpha[0] = ctx->S[10];	
		alpha[0]^= 0xFFFFFFF8; /* add 1 */
		alpha[1] = ROTR29(alpha[0], 9);
		alpha[2] = ROTR29(alpha[0], 10);
		alpha[3] = ROTR29(alpha[0], 19);

		/* find alpha^-1  and alpha^(-2^9) here */
		alpha_inv=alpha[0];
		/*4*/
		alpha_inv=ROTL29(alpha_inv, 16);
		mult(alpha_inv, ROTR29(alpha_inv, 8), &alpha_inv);
		mult(alpha_inv, alpha[0], &alpha_inv);
		/*3*/
		alpha_inv=ROTL29(alpha_inv, 8);
		mult(alpha_inv, ROTR29(alpha_inv, 4), &alpha_inv);
		mult(alpha_inv, alpha[0], &alpha_inv);
		/*2*/
		alpha_inv=ROTL29(alpha_inv, 4);
		mult(alpha_inv, ROTR29(alpha_inv, 2), &alpha_inv);
		/*1*/
		mult(alpha_inv, ROTR29(alpha_inv, 1), &alpha_inv);

		alpha_inv9 = ROTR29(alpha_inv, 9); /* alpha^(-2^9) */


		/* WG exponents coputation */
		mult(alpha[2], alpha[0], &q[0]); /*q1*/
		mult(alpha[3], alpha[0], &q[1]);
		mult(q[1], alpha_inv9, &q[2]); /*q3*/
		mult(q[1], alpha[1], &q[1]);     /*q2*/
		mult(alpha[3], alpha[2], &q[3]);
		mult(q[3], alpha_inv, &q[3]); /*q4*/


		/* alpha+alpha^q1+alpha^q2+alpha^q3+alpha^q4 */
		alpha[0]^=q[0];
		alpha[0]^=q[1];
		alpha[0]^=q[2];
		alpha[0]^=q[3];

		alpha[0]^= 0xFFFFFFF8; /* add 1 */

		
		/* clock the LFSR */
		mult(ctx->S[10], gamma, &ctx->S[10]); /* S[10]*gamma */

		/* x^11+x^10+x^9+x^6+x^3+x+alpha */
		ctx->S[10]^=ctx->S[9]; ctx->S[10]^=ctx->S[7]; ctx->S[10]^=ctx->S[4];ctx->S[10]^=ctx->S[1];ctx->S[10]^=ctx->S[0];
		ctx->S[10]^=alpha[0]; alpha[1]=ctx->S[10];

		ctx->S[10]=ctx->S[9]; ctx->S[9]=ctx->S[8]; ctx->S[8]=ctx->S[7]; ctx->S[7]=ctx->S[6];
		ctx->S[6]=ctx->S[5]; ctx->S[5]=ctx->S[4]; ctx->S[4]=ctx->S[3]; ctx->S[3]=ctx->S[2];
		ctx->S[2]=ctx->S[1];ctx->S[1]=ctx->S[0]; ctx->S[0]=alpha[1]; /*LFSR clocked*/
		
	}




}

/*****************************************************/

void ECRYPT_encrypt_bytes(
						  ECRYPT_ctx* ctx, 
						  const u8* plaintext, 
						  u8* ciphertext, 
						  u32 msglen)                /* Message length in bytes. */ 
{

	int i;
	ECRYPT_keystream_bytes(ctx, ciphertext, msglen);
	for(i=0;i<msglen;i++)ciphertext[i]^=plaintext[i];


}

/****************************************************/

void ECRYPT_decrypt_bytes(
						  ECRYPT_ctx* ctx, 
						  const u8* ciphertext, 
						  u8* plaintext, 
						  u32 msglen)                /* Message length in bytes. */ 
{

	int i;
	ECRYPT_keystream_bytes(ctx, plaintext, msglen);
	for(i=0;i<msglen;i++)plaintext[i]^=ciphertext[i];

}

/**************************************************/

void ECRYPT_keystream_bytes(
							ECRYPT_ctx* ctx,
							u8* keystream,
							u32 length)
{
	u32 alpha[4]; /* powers of alpha [alpha^x, x=1,9,10,19] */
	u32 q[4]; /* intermediate results of multiplication in WG */
	u32 alpha_inv, alpha_inv9; /* alpha^-1  & alpha^(-2^9) */
	u32 trace,gamma, j, i; 
	u8 key8, one;

	gamma=0x7F3FC9B0; /* in LFSR feedback pol */

	for(j=0;j<length;j++){
		key8=0; one=0x80;
		for(i=0;i<8;i++){
			trace=0; 

			/* clock the LFSR */
			mult(ctx->S[10], gamma, &ctx->S[10]); /* S[10]*gamma */

			/* x^11+x^10+x^9+x^6+x^3+x+alpha */
			ctx->S[10]^=ctx->S[9]; ctx->S[10]^=ctx->S[7]; ctx->S[10]^=ctx->S[4];ctx->S[10]^=ctx->S[1];ctx->S[10]^=ctx->S[0];
			alpha[0] = ctx->S[10];

			ctx->S[10]=ctx->S[9]; ctx->S[9]=ctx->S[8]; ctx->S[8]=ctx->S[7]; ctx->S[7]=ctx->S[6];
			ctx->S[6]=ctx->S[5]; ctx->S[5]=ctx->S[4]; ctx->S[4]=ctx->S[3]; ctx->S[3]=ctx->S[2];
			ctx->S[2]=ctx->S[1];ctx->S[1]=ctx->S[0]; ctx->S[0]=alpha[0]; /*LFSR clocked*/


			/* WG  transformation of S[10] */
			alpha[0] = ctx->S[10];
			alpha[0]^= 0xFFFFFFF8; /* add 1 */
			alpha[1] = ROTR29(alpha[0], 9);
			alpha[2] = ROTR29(alpha[0], 10);
			alpha[3] = ROTR29(alpha[0], 19);

			/* find alpha^-1  and alpha^(-2^9) here */
			alpha_inv=alpha[0];
			/*4*/
			alpha_inv=ROTL29(alpha_inv, 16);
			mult(alpha_inv, ROTR29(alpha_inv, 8), &alpha_inv);
			mult(alpha_inv, alpha[0], &alpha_inv);
			/*3*/
			alpha_inv=ROTL29(alpha_inv, 8);
			mult(alpha_inv, ROTR29(alpha_inv, 4), &alpha_inv);
			mult(alpha_inv, alpha[0], &alpha_inv);
			/*2*/
			alpha_inv=ROTL29(alpha_inv, 4);
			mult(alpha_inv, ROTR29(alpha_inv, 2), &alpha_inv);
			/*1*/
			mult(alpha_inv, ROTR29(alpha_inv, 1), &alpha_inv);

			alpha_inv9 = ROTR29(alpha_inv, 9); /* alpha^(-2^9) */


			/* WG exponents coputation */
			mult(alpha[2], alpha[0], &q[0]); /*q1*/
			mult(alpha[3], alpha[0], &q[1]);
			mult(q[1], alpha_inv9, &q[2]); /*q3*/
			mult(q[1], alpha[1], &q[1]);     /*q2*/
			mult(alpha[3], alpha[2], &q[3]);
			mult(q[3], alpha_inv, &q[3]); /*q4*/


			/* alpha+alpha^q1+alpha^q2+alpha^q3+alpha^q4 */
			alpha[0]^=q[0];
			alpha[0]^=q[1];
			alpha[0]^=q[2];
			alpha[0]^=q[3];

			alpha[0]^= 0xFFFFFFF8; /* add 1 */

			/* Trace */
			trace=alpha[0];
			trace^=(alpha[0]<<1);trace^=(alpha[0]<<2);trace^=(alpha[0]<<3);trace^=(alpha[0]<<4);
			trace^=(alpha[0]<<5);trace^=(alpha[0]<<6);trace^=(alpha[0]<<7);trace^=(alpha[0]<<8);
			trace^=(alpha[0]<<9);trace^=(alpha[0]<<10);trace^=(alpha[0]<<11);trace^=(alpha[0]<<12);
			trace^=(alpha[0]<<13);trace^=(alpha[0]<<14);trace^=(alpha[0]<<15);trace^=(alpha[0]<<16);
			trace^=(alpha[0]<<17);trace^=(alpha[0]<<18);trace^=(alpha[0]<<19);trace^=(alpha[0]<<20);
			trace^=(alpha[0]<<21);trace^=(alpha[0]<<22);trace^=(alpha[0]<<23);trace^=(alpha[0]<<24);
			trace^=(alpha[0]<<25);trace^=(alpha[0]<<26);trace^=(alpha[0]<<27);trace^=(alpha[0]<<28);
			/* output bit is 0th bit in trace*/
			trace>>=31;
			if(trace)key8^=(one>>i);

		}
		keystream[j]=key8;
	}


}

/***************************************************************/
/* multipies two field elements represented in 
* optimal normal basis in GF(2^29). For base 
* element that generates ONB see paper (The WG stream cipher)
*/
void mult(u32 a, u32 b, u32* c){

	u32 A[29], B[29];

	/*precomputation*/
	A[0]=a&0xFFFFFFF8; B[0]=b&0xFFFFFFF8; /* remove & operation afterwards */
	A[1]=ROTL29(A[0], 1); B[1]=ROTL29(B[0],1);
	A[2]=ROTL29(A[0], 2); B[2]=ROTL29(B[0],2);
	A[3]=ROTL29(A[0], 3); B[3]=ROTL29(B[0],3);
	A[4]=ROTL29(A[0], 4); B[4]=ROTL29(B[0],4);
	A[5]=ROTL29(A[0], 5); B[5]=ROTL29(B[0],5);
	A[6]=ROTL29(A[0], 6); B[6]=ROTL29(B[0],6);
	A[7]=ROTL29(A[0], 7); B[7]=ROTL29(B[0],7);
	A[8]=ROTL29(A[0], 8); B[8]=ROTL29(B[0],8);
	A[9]=ROTL29(A[0], 9); B[9]=ROTL29(B[0],9);
	A[10]=ROTL29(A[0], 10); B[10]=ROTL29(B[0],10);
	A[11]=ROTL29(A[0], 11); B[11]=ROTL29(B[0],11);
	A[12]=ROTL29(A[0], 12); B[12]=ROTL29(B[0],12);
	A[13]=ROTL29(A[0], 13); B[13]=ROTL29(B[0],13);
	A[14]=ROTL29(A[0], 14); B[14]=ROTL29(B[0],14);
	A[15]=ROTL29(A[0], 15); B[15]=ROTL29(B[0],15);
	A[16]=ROTL29(A[0], 16); B[16]=ROTL29(B[0],16);
	A[17]=ROTL29(A[0], 17); B[17]=ROTL29(B[0],17);
	A[18]=ROTL29(A[0], 18); B[18]=ROTL29(B[0],18);
	A[19]=ROTL29(A[0], 19); B[19]=ROTL29(B[0],19);
	A[20]=ROTL29(A[0], 20); B[20]=ROTL29(B[0],20);
	A[21]=ROTL29(A[0], 21); B[21]=ROTL29(B[0],21);
	A[22]=ROTL29(A[0], 22); B[22]=ROTL29(B[0],22);
	A[23]=ROTL29(A[0], 23); B[23]=ROTL29(B[0],23);
	A[24]=ROTL29(A[0], 24); B[24]=ROTL29(B[0],24);
	A[25]=ROTL29(A[0], 25); B[25]=ROTL29(B[0],25);
	A[26]=ROTL29(A[0], 26); B[26]=ROTL29(B[0],26);
	A[27]=ROTL29(A[0], 27); B[27]=ROTL29(B[0],27);
	A[28]=ROTL29(A[0], 28); B[28]=ROTL29(B[0],28);

	/* multiplication */
	*c=A[0] & B[1];
	*c^= A[1] & (B[0] ^ B[21]);
	*c^= A[2] & (B[6] ^ B[21]);
	*c^= A[3] & (B[13] ^ B[18]);
	*c^= A[4] & (B[11] ^ B[27]);
	*c^= A[5] & (B[17] ^ B[20]);
	*c^= A[6] & (B[2] ^ B[22]);
	*c^= A[7] & (B[13] ^ B[25]);
	*c^= A[8] & (B[9] ^ B[10]);
	*c^= A[9] & (B[8] ^ B[14]);
	*c^= A[10] & (B[8] ^ B[26]);
	*c^= A[11] & (B[4] ^ B[14]);
	*c^= A[12] & (B[17] ^ B[24]);
	*c^= A[13] & (B[3] ^ B[7]);
	*c^= A[14] & (B[9] ^ B[11]);
	*c^= A[15] & (B[24] ^ B[26]);
	*c^= A[16] & (B[19] ^ B[23]);
	*c^= A[17] & (B[5] ^ B[12]);
	*c^= A[18] & (B[3] ^ B[22]);
	*c^= A[19] & (B[16] ^ B[27]);
	*c^= A[20] & (B[5] ^ B[28]);
	*c^= A[21] & (B[1] ^ B[2]);
	*c^= A[22] & (B[6] ^ B[18]);
	*c^= A[23] & (B[16] ^ B[25]);
	*c^= A[24] & (B[12] ^ B[15]);
	*c^= A[25] & (B[7] ^ B[23]);
	*c^= A[26] & (B[10] ^ B[15]);
	*c^= A[27] & (B[4] ^ B[19]);
	*c^= A[28] & (B[20] ^ B[28]);
}


/*********************************************************/
