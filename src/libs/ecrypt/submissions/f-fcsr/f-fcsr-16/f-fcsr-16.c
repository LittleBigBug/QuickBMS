    /*
     * F-FCSR-16 reference implementation.
     *
     * (c) 2005 FCSR project. This software is provided 'as-is', without
     * any express or implied warranty. In no event will the authors be held
     * liable for any damages arising from the use of this software.
     *
     * Permission is granted to anyone to use this software for any purpose,
     * including commercial applications, and to alter it and redistribute it
     * freely, subject to no restriction.
     *
     * Technical remarks and questions can be addressed to
     * <cedric.lauradoux@inria.fr>
     */


#include "ffcsr16-sync.h"
#ifdef FFCSR16_EVALUATE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#endif

void ECRYPT_init(void)
{ }

/* Update the shift register and the carry register of the FCSR */
void ECRYPT_clock(
  ECRYPT_ctx* ctx 
)
{
	u32 feedback;
	u32 buffer[8];
	
	/* expand the feedback bit */
	
	feedback = ((int) (ctx->state[0] << (MAXSHIFT))) >> (MAXSHIFT); 
	/* shift the state */
	
	ctx->state[0] = ctx->state[0] >> 1;
	ctx->state[0] |= (ctx->state[1] & 0x00000001 ) << (MAXSHIFT);

	ctx->state[1] = ctx->state[1] >> 1;
	ctx->state[1] |= (ctx->state[2] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[2] = ctx->state[2] >> 1;
	ctx->state[2] |= (ctx->state[3] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[3] = ctx->state[3] >> 1;
	ctx->state[3] |= (ctx->state[4] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[4] = ctx->state[4] >> 1;
	ctx->state[4] |= (ctx->state[5] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[5] = ctx->state[5] >> 1;
	ctx->state[5] |= (ctx->state[6] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[6] = ctx->state[6] >> 1;
	ctx->state[6] |= (ctx->state[7] & 0x00000001 ) << (MAXSHIFT);
	
	ctx->state[7] >>=1;
	/* update the register */
	
	buffer[0] = ctx->state[0] ^ ctx->carry[0]; 
	buffer[1] = ctx->state[1] ^ ctx->carry[1];
	buffer[2] = ctx->state[2] ^ ctx->carry[2];
	buffer[3] = ctx->state[3] ^ ctx->carry[3];
	buffer[4] = ctx->state[4] ^ ctx->carry[4]; 
	buffer[5] = ctx->state[5] ^ ctx->carry[5];
	buffer[6] = ctx->state[6] ^ ctx->carry[6];
	buffer[7] = ctx->state[7] ^ ctx->carry[7];
	
	
	ctx->carry[0] &= ctx->state[0];
	ctx->carry[1] &= ctx->state[1];
	ctx->carry[2] &= ctx->state[2];
	ctx->carry[3] &= ctx->state[3];
	ctx->carry[4] &= ctx->state[4];
	ctx->carry[5] &= ctx->state[5];
	ctx->carry[6] &= ctx->state[6];
	ctx->carry[7] &= ctx->state[7];

	ctx->carry[0] ^= buffer[0] & (feedback &d0);
	ctx->carry[1] ^= buffer[1] & (feedback &d1);
	ctx->carry[2] ^= buffer[2] & (feedback &d2);
	ctx->carry[3] ^= buffer[3] & (feedback &d3);
	ctx->carry[4] ^= buffer[4] & (feedback &d4);
	ctx->carry[5] ^= buffer[5] & (feedback &d5);
	ctx->carry[6] ^= buffer[6] & (feedback &d6);
	ctx->carry[7] ^= buffer[7] & (feedback &d7);
	
	
	buffer[0] ^= feedback & d0;
	buffer[1] ^= feedback & d1;
	buffer[2] ^= feedback & d2;
	buffer[3] ^= feedback & d3;
	buffer[4] ^= feedback & d4;
	buffer[5] ^= feedback & d5;
	buffer[6] ^= feedback & d6;
	buffer[7] ^= feedback & d7;
	
	ctx->state[0] = buffer[0];
	ctx->state[1] = buffer[1];
	ctx->state[2] = buffer[2];
	ctx->state[3] = buffer[3];
	ctx->state[4] = buffer[4];
	ctx->state[5] = buffer[5];
	ctx->state[6] = buffer[6];
	ctx->state[7] = buffer[7];
	
}


void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keyse,                /* Key se in bits. */ 
  u32 ivse)                 /* IV se in bits. */ 
{
	ctx->filter[0]=d0;
	ctx->filter[1]=d1;
	ctx->filter[2]=d2;
	ctx->filter[3]=d3;
	ctx->filter[4]=d4;
	ctx->filter[5]=d5;
	ctx->filter[6]=d6;
	ctx->filter[7]=d7;

	ctx->state[3] = key[3] | key[2]<<8 | key[1]<<16 | key[0]<<24;
	ctx->state[2] = key[7] | key[6]<<8 | key[5]<<16 | key[4]<<24;
	ctx->state[1] = key[11] | key[10]<<8 | key[9]<<16 | key[8]<<24;
	ctx->state[0] = key[15] | key[14]<<8 | key[13]<<16 | key[12]<<24;
	ctx->state[4] = 0;
	ctx->state[5] = 0;
	ctx->state[6] = 0;
	ctx->state[7] = 0;
	ctx->init[0]=ctx->state[0];
	ctx->init[1]=ctx->state[1];
	ctx->init[2]=ctx->state[2];
	ctx->init[3]=ctx->state[3];
}	
	

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
	u32 i;
	u16 S[16];
	
	ctx->carry[0] = 0;
	ctx->carry[1] = 0;
	ctx->carry[2] = 0;
	ctx->carry[3] = 0;
	ctx->carry[4] = 0;
	ctx->carry[5] = 0;
	ctx->carry[6] = 0;
	ctx->carry[7] = 0;

	ctx->state[0] = ctx->init[0];
	ctx->state[1] = ctx->init[1];
	ctx->state[2] = ctx->init[2];
	ctx->state[3] = ctx->init[3];
	ctx->state[4] = iv[15] | iv[14]<<8 | iv[13]<<16 | iv[12]<<24;
	ctx->state[5] = iv[11] | iv[10]<<8 |  iv[9]<<16 | iv[8]<<24;
	ctx->state[6] = iv[7]  | iv[6]<<8  |  iv[5]<<16 | iv[4]<<24;
	ctx->state[7] = iv[3]  | iv[2]<<8  |  iv[1]<<16 | iv[0]<<24;

	for(i=0;i<16;i++)
	{
		ECRYPT_clock(ctx);
		S[i]=ECRYPT_filter(ctx);
	}

	ctx->state[0] = S[0] | S[1]<<16;
	ctx->state[1] = S[2] | S[3]<<16;
	ctx->state[2] = S[4] | S[5]<<16;
	ctx->state[3] = S[6] | S[7]<<16;
	ctx->state[4] = S[8] | S[9]<<16;
	ctx->state[5] = S[10] | S[11]<<16;
	ctx->state[6] = S[12] | S[13]<<16;
	ctx->state[7] = S[14] | S[15]<<16;
	
	ctx->carry[0] = 0;
	ctx->carry[1] = 0;
	ctx->carry[2] = 0;
	ctx->carry[3] = 0;
	ctx->carry[4] = 0;
	ctx->carry[5] = 0;
	ctx->carry[6] = 0;
	ctx->carry[7] = 0;
	
	for(i=0;i<258;i++)
	{
		ECRYPT_clock(ctx);
	}
}

void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)                /* Message length in bytes. */ 
{
	u32 i;
	u16 buffer;
	
	for( i=0 ; i< msglen; i+=2)
	{
		ECRYPT_clock(ctx);
		buffer=ECRYPT_filter(ctx);
		output[i]=(u8)buffer ^ input[i];
		output[i+1]=(u8)(buffer>>8) ^ input[i+1];
	}
}


/* Produce one byte of keystream from the internal state of the register */
u16 ECRYPT_filter(
  ECRYPT_ctx* ctx 
)
{
	u32 buffer[4];
	
	
	buffer[0] = ctx->filter[0] & ctx->state[0];
	buffer[1] = ctx->filter[1] & ctx->state[1];
	buffer[2] = ctx->filter[2] & ctx->state[2];
	buffer[3] = ctx->filter[3] & ctx->state[3];
	buffer[0] ^= ctx->filter[4] & ctx->state[4];
	buffer[1] ^= ctx->filter[5] & ctx->state[5];
	buffer[2] ^= ctx->filter[6] & ctx->state[6];
	buffer[3] ^= ctx->filter[7] & ctx->state[7];
	
	buffer[0] ^= buffer[1];
	buffer[2] ^= buffer[3];
	
	buffer[0] ^= buffer[2];
	buffer[0] ^= ( buffer[0] >> 16 );
	return (u16)buffer[0];
}


#ifdef FFCSR16_EVALUATE
#define MB 1048576
#define NUM_MB 100


int main()
{
	u32 i,j;
	clock_t orig, end;
	
	double time;
	ECRYPT_ctx ctx;
	
	/* key for test vector */
	u8 testKEY[16] = { 0x00, 0x88 , 0x63 , 0x9d, 0x6b , 0xf8 , 0x47 , 0xed , 0x59 , 0xc6 , 0x21 , 0x79 , 0x5d , 0x33 , 0x63 , 0xf1 };
	u8 testIV[16] = { 0x00 , 0x11 , 0x22 , 0x33 , 0x44 , 0x55 , 0x66 , 0x77 , 0x88 , 0x99 , 0xAA , 0xBB , 0xCC , 0xDD , 0xEE , 0xFF};
	u8 encipheredText[10]={ 0x15, 0xfd, 0x4a, 0x9d, 0xe0, 0x0e, 0xde, 0x72, 0x2b, 0x29};
	u8 output[10],input[10];
	u8 text[10]={'F','-','F','C','S','R','-','1','6','!'};
	u16 data=0;
	/* iv for vector test */
	printf("\t TEST VECTOR\n");
	
	printf("KEY 0x");
	for(i=0;i<16;i++)
	{
		printf("%x",testKEY[i]);
	}
	printf("\n");
	
	printf("IV 0x");
	for(i=0;i<16;i++)
	{
		printf("%x",testIV[i]);
	}
	printf("\n");
	
	printf("Plain Text: ");
	for(i=0;i<10;i++)
	{
		printf("%c",text[i]);
	}
	printf("\n");
	
	
	ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
	ECRYPT_ivsetup( &ctx , testIV );

	ECRYPT_process_bytes( 0, &ctx , text , output , 10); 
	
	printf("Enciphered Text: ");
	for(i=0;i<10;i++)
	{
	  if(output[i] == encipheredText[i])
		{
			printf("0x%02x, ",output[i]);
		}else
		{
			printf("ERROR\n");
			exit(0);
		}
	}
	printf("\n");
	
	ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
	ECRYPT_ivsetup( &ctx , testIV );
	ECRYPT_process_bytes( 0, &ctx , output ,input , 10); 
	
	printf("Deciphered Text: ");
	for(i=0;i<10;i++)
	{
		printf("%c",input[i]);
	}
	printf("\n");
	
	printf("SPEED TEST\n");
	ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
	ECRYPT_ivsetup( &ctx , testIV );
	
	
	orig = clock();
	for(i=0;i<(NUM_MB);i++)
	{
		for( j=0 ; j<MB ; j++)
		{
			ECRYPT_clock(&ctx);
			data ^= ECRYPT_filter(&ctx); 
		}
	}
	end = clock();
	
	time = (double)end / CLOCKS_PER_SEC - (double)orig / CLOCKS_PER_SEC;
	if (time <= 1.0) {
		printf("Time resolution too big !\n");
	} else {
		printf("elapsed time: %.4f seconds\n", time);
		printf("byte per second: %.0f\n", (double)(MB*NUM_MB) / time);
	}
	
	orig = clock();
	for(i=0;i<NUM_MB;i++)
	{
		for( j=0 ; j<(MB/256) ; j++)
		{
			ECRYPT_ivsetup( &ctx , testIV );
		}
	}
	end = clock();
	
	time = (double)end / CLOCKS_PER_SEC - (double)orig / CLOCKS_PER_SEC;
	if (time <= 1.0) {
		printf("Time resolution too big !\n");
	} else {
		printf("elapsed time: %.4f seconds\n", time);
		printf("IV reload per second: %.0f\n", (double)(MB*NUM_MB) / (256*time));
	}
		
	orig = clock();
	for(i=0;i<(NUM_MB);i++)
	{
		for( j=0 ; j<(MB/1024) ; j++)
		{
			ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
		}
	}
	end = clock();
	
	time = (double)end / CLOCKS_PER_SEC - (double)orig / CLOCKS_PER_SEC;
	if (time <= 1.0) {
		printf("Time resolution too big !\n");
	} else {
		printf("elapsed time: %.4f seconds\n", time);
		printf("Load KEY per second: %.0f\n", (double)(MB*NUM_MB) / (time*1024));
	}
	printf("data = 0x%x\n", data);
	return 0;
}
#endif





