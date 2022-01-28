/* ecrypt-sync-Edon80.c   v1.0   April 2005
 * Reference ANSI C code
 * author:  v1.0 Danilo Gligoroski
 * Copyright 2005
 * 
 * This code is placed as a reference code for ECRYPT 
 * call for Stream Ciphers.
 */

#include "ecrypt-sync.h"

void ECRYPT_init(void)
{ }

/* Here is the actual definition of ECRYPT_keysetup */
void ECRYPT_keysetup( ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
	u32 i, j, m;    /* Variables i, j and m are internal counters. */
	u32 index;      /* Variable index is local temporal variable. */

	u8 Q[4][4][4]={    /* In the design phase we choose this 4 quasigroups of order 4 */
		               /* out of possible 384 good candidates (out of 64 very good candidates). */
		{			      
			{0, 2, 1, 3}, /* 0: Nr: 61 */
			{2, 1, 3, 0},
			{1, 3, 0, 2},
			{3, 0, 2, 1}
		},
		{
			{1, 3, 0, 2}, /* 1: Nr. 241 */
			{0, 1, 2, 3},
			{2, 0, 3, 1},
			{3, 2, 1, 0}
		},
		{
			{2, 1, 0, 3}, /* 2: Nr. 350 */
			{1, 2, 3, 0},
			{3, 0, 2, 1},
			{0, 3, 1, 2}
		},
		{
			{3, 2, 1, 0}, /* 3: Nr. 564 */
			{1, 0, 3, 2},
			{0, 3, 2, 1},
			{2, 1, 0, 3}
		}
		};

	/* First we store the number of pairs of key bits into ctx->keysize. */
	ctx->keysize=keysize>>1;

	/* Then we transform the received key vector, into a vector of pairs of bits. */
	j=0;
	for (i=0; i<keysize>>3; i++)
	{
		ctx->key[j++]=key[i]>>6;           /* Upper 2 bits of the key[i] */
		ctx->key[j++]=(key[i] & 0x30)>>4;  /* Next  2 bits of the key[i] */
		ctx->key[j++]=(key[i] & 0x0c)>>2;  /* Next  2 bits of the key[i] */
		ctx->key[j++]=key[i] & 0x03;       /* Lower 2 bits of the key[i] */
	}

	/* Then we set the value of ctx->ivsize as a number of iv pairs of bits. */
	ctx->ivsize=ivsize>>1;

	/* Then we set the number of internal states. */
	ctx->NumberOfInternalStates=ECRYPT_MAXKEYSIZE;

	/* Finally we set the working quasigroups Q[][] according to the values of the ctx->key[]. */
	for (m=0; m<keysize>>1; m++)
	{
		index=ctx->key[m];
		for (i=0; i<4; i++) 
			for (j=0; j<4; j++)
			{ 
				ctx->Q[m][i][j]=Q[index][i][j];
				ctx->Q[m+ECRYPT_MAXKEYSIZE/2][i][j]=Q[index][i][j];
			}
	}
}

/* Here is the actual definition of ECRYPT_ivsetup */
void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
	u32 i, j;        /* Variables i and j are internal counters. */
	s32 k;           /* Variable k is internal signed counter. */
	u8 Temp[ECRYPT_MAXKEYSIZE];         /* Temp is a vector of bytes that will temporarily 
									     hold the values of the vector InternalStates[] */

	/* First we transform the received iv vector, into a vector of pairs of bits. */
	j=0;
	for (i=0; i<ctx->ivsize>>2; i++)
	{
		ctx->iv[j++]=iv[i]>>6;           /* Upper 2 bits of the iv[i] */
		ctx->iv[j++]=(iv[i] & 0x30)>>4;  /* Next  2 bits of the iv[i] */
		ctx->iv[j++]=(iv[i] & 0x0c)>>2;  /* Next  2 bits of the iv[i] */
		ctx->iv[j++]=iv[i] & 0x03;       /* Lower 2 bits of the iv[i] */
	}

	/* Then we copy vectors ctx.key[] and ctx.iv[] into vectors ctx->InternalState[] and Temp[] */
	for (i=0; i<ctx->keysize; i++) Temp[i]=ctx->InternalState[i]=ctx->key[i];
	for (j=0; j<ctx->ivsize; j++)
	{
		Temp[i]=ctx->InternalState[i]=ctx->iv[j];
		++i;
	}
	
	/* 
	   Then we pad ctx->InternalState[] and Temp[] with 
	   16 bits: 0xE41B i.e. with 32100123 (in system of base 4) 
	*/
	Temp[i]=ctx->InternalState[i]=3;
	++i;
	Temp[i]=ctx->InternalState[i]=2;
	++i;
	Temp[i]=ctx->InternalState[i]=1;
	++i;
	Temp[i]=ctx->InternalState[i]=0;
	++i;
	Temp[i]=ctx->InternalState[i]=0;
	++i;
	Temp[i]=ctx->InternalState[i]=1;
	++i;
	Temp[i]=ctx->InternalState[i]=2;
	++i;
	Temp[i]=ctx->InternalState[i]=3;
	++i;

	/*	Finaly we transform the vector InternalState[], with 
		Quasigroup e-transformations, with leaders that are 
		initial key[] and iv[] elements (i.e. elements in Temp[]), 
		ordered in reverse order and by quasigroups determined 
		by the value of the key[].
	*/
	for (k=i-1; k>=0; k--)
	{
		ctx->InternalState[0]=ctx->Q[k][Temp[k]][ctx->InternalState[0]];
		for (j=1; j<ctx->NumberOfInternalStates; j++) 
			ctx->InternalState[j]=ctx->Q[k][ctx->InternalState[j-1]][ctx->InternalState[j]];
	}

	/* As a final step in iv setup we prepare the value of internal Counter to 3. */
	ctx->Counter=3;
}

/* Here is the actual definition of ECRYPT_encrypt_bytes */
void ECRYPT_encrypt_bytes(ECRYPT_ctx* ctx, const u8* plaintext, u8* ciphertext, u32 msglen)
{
	u32 i, j;       /* Variables i and j are internal counters. */
	u8 X;           /* We will store produced byte from the keystream in X. */


	for (j=0;j<msglen;j++)
	{
		/* Variable ctx.Counter will varry from 0 to 3, periodically. 
		   It will be the first feed to the e-transformations by the quasigroups. */
		ctx->Counter++;
		ctx->Counter&=0x03;

		/* Obtaining the first 2 bits from the ciher */
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=(ctx->InternalState[i-1])<<6;


		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<4);

		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<2);

		/* Obtaining last 2 bits from the ciher, to form a keystream byte. */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^ctx->InternalState[i-1];

		/* Finally we XOR the plaintext with X */
		ciphertext[j]=plaintext[j]^X;
	}
}

/* Here is the actual definition of ECRYPT_decrypt_bytes */
void ECRYPT_decrypt_bytes(ECRYPT_ctx* ctx, const u8* ciphertext, u8* plaintext, u32 msglen)
{
	u32 i, j;       /* Variables i and j are internal counters. */
	u8 X;           /* We will store produced byte from the keystream in X. */


	for (j=0;j<msglen;j++)
	{
		/* Variable ctx.Counter will varry from 0 to 3, periodically. 
		   It will be the first feed to the e-transformations by the quasigroups. */
		ctx->Counter++;
		ctx->Counter&=0x03;

		/* Obtaining the first 2 bits from the ciher */
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=(ctx->InternalState[i-1])<<6;


		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<4);

		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<2);

		/* Obtaining last 2 bits from the ciher, to form a keystream byte. */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^ctx->InternalState[i-1];

		/* Finally we XOR the plaintext with X */
		plaintext[j]=ciphertext[j]^X;
	}
}

/* Here is the actual definition of ECRYPT_keystream_bytes */
void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{
	u32 i, j;       /* Variables i and j are internal counters. */
	u8 X;           /* We will store produced byte from the keystream in X. */


	for (j=0;j<length;j++)
	{
		/* Variable ctx.Counter will varry from 0 to 3, periodically. 
		   It will be the first feed to the e-transformations by the quasigroups. */
		ctx->Counter++;
		ctx->Counter&=0x03;

		/* Obtaining the first 2 bits from the ciher */
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=(ctx->InternalState[i-1])<<6;


		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<4);

		/* Obtaining next 2 bits from the ciher */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^(ctx->InternalState[i-1]<<2);

		/* Obtaining last 2 bits from the ciher, to form a keystream byte. */
		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		ctx->Counter++;
		ctx->Counter&=0x03;
		ctx->InternalState[0]=ctx->Q[0][ctx->InternalState[0]][ctx->Counter];
		for (i=1;i<ctx->NumberOfInternalStates;i++) 
			ctx->InternalState[i]=ctx->Q[i][ctx->InternalState[i]][ctx->InternalState[i-1]];

		X=X^ctx->InternalState[i-1];

		/* Finally we feed the keystream with X. */
		keystream[j]=X;
	}
}

