    /*
     * F-FCSR-H reference implementation.
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

#ifndef ECRYPT_API
#define FFCSRH_EVALUATE
#endif

#include "ffcsrh-sync.h"
#ifdef FFCSRH_EVALUATE
#include <time.h>
#endif

void ECRYPT_init(void)
{ }

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

        ctx->init[0] = key[9] | key[8]<<8 | key[7]<<16 | key[6]<<24;
        ctx->init[1] = key[5] | key[4]<<8 | key[3]<<16 | key[2]<<24;
        ctx->init[2] = key[1] | key[0]<<8 | 0x00<<16 | 0x00<<24;
}

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
	u32 i=0;

	ctx->state[0] = ctx->init[0];
	ctx->state[1] = ctx->init[1];
	ctx->state[2] = ctx->init[2];

	ctx->state[2] ^= iv[9]<<16 | iv[8]<<24;
	ctx->state[3] = iv[7] | iv[6]<<8 | iv[5]<<16 | iv[4]<<24;
	ctx->state[4] = iv[3] | iv[2]<<8 | iv[1]<<16 | iv[0]<<24;
	
	
	ctx->carry[0] = 0;
	ctx->carry[1] = 0;
	ctx->carry[2] = 0;
	ctx->carry[3] = 0;
	ctx->carry[4] = 0;
	

	for( i=0; i<160 ; i++)
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
	for( i=0 ; i< msglen ; i++)
	{
		ECRYPT_clock(ctx);
		output[i] = input[i] ^ ECRYPT_filter(ctx);
	}
}


/* Update the shift register and the carry register of the FCSR */
void ECRYPT_clock(
  ECRYPT_ctx* ctx 
)
{
	u32 feedback;
	u32 buffer[5];
	
	/* expand the feedback bit */
	
	feedback = ((int) (ctx->state[0] << (MAXSHIFT))) >> (MAXSHIFT); 
	/* shift the state */
	
	ctx->state[0] = ctx->state[0] >> 1;
	ctx->state[0] |= (ctx->state[1] & 0x00000001 ) << (MAXSHIFT);

	ctx->state[1] = ctx->state[1] >> 1;
	ctx->state[1] |= (ctx->state[2] & 0x01 ) << (MAXSHIFT);
	
	ctx->state[2] = ctx->state[2] >> 1;
	ctx->state[2] |= (ctx->state[3] & 0x01 ) << (MAXSHIFT);

	ctx->state[3] >>=1;
	ctx->state[3] |= (ctx->state[4] & 0x01 ) << (MAXSHIFT);
	
	ctx->state[4] >>=1;
	/* update the register */
	
	buffer[0] = ctx->state[0] ^ ctx->carry[0]; 
	
	buffer[1] = ctx->state[1] ^ ctx->carry[1];
	buffer[2] = ctx->state[2] ^ ctx->carry[2];
	buffer[3] = ctx->state[3] ^ ctx->carry[3];
	buffer[4] = ctx->state[4] ^ ctx->carry[4];
	
	ctx->carry[0] &= ctx->state[0];
	ctx->carry[1] &= ctx->state[1];
	ctx->carry[2] &= ctx->state[2];
	ctx->carry[3] &= ctx->state[3];
	ctx->carry[4] &= ctx->state[4];

	ctx->carry[0] ^= buffer[0] & (feedback &d0);
	ctx->carry[1] ^= buffer[1] & (feedback &d1);
	ctx->carry[2] ^= buffer[2] & (feedback &d2);
	ctx->carry[3] ^= buffer[3] & (feedback &d3);
	ctx->carry[4] ^= buffer[4] & (feedback &d4);
	
	buffer[0] ^= feedback & d0;
	buffer[1] ^= feedback & d1;
	buffer[2] ^= feedback & d2;
	buffer[3] ^= feedback & d3;
	buffer[4] ^= feedback & d4;
	
	ctx->state[0] = buffer[0];
	ctx->state[1] = buffer[1];
	ctx->state[2] = buffer[2];
	ctx->state[3] = buffer[3];
	ctx->state[4] = buffer[4];
}

/* Produce one byte of keystream from the internal state of the register */
u8 ECRYPT_filter(
  ECRYPT_ctx* ctx 
)
{
	u32 buffer[5];
	
	buffer[0] = ctx->filter[0] & ctx->state[0];
	buffer[1] = ctx->filter[1] & ctx->state[1];
	buffer[2] = ctx->filter[2] & ctx->state[2];
	buffer[3] = ctx->filter[3] & ctx->state[3];
	buffer[4] = ctx->filter[4] & ctx->state[4];
	buffer[0] ^= buffer[1];
	buffer[2] ^= buffer[3];
	
	buffer[0] ^= buffer[2]^ buffer[4];
	

	buffer[0] ^= ( buffer[0] >> 16 );
	buffer[0] ^= ( buffer[0] >> 8);
	return (u8)buffer[0];
}

#ifdef FFCSRH_EVALUATE
#define MB 1048576
#define NUM_MB 100
int main()
{
	u32 i,j;
	clock_t orig, end;
	double time;
	ECRYPT_ctx ctx;
	
	/* key for test vector */
	u8 testKEY[10] = { 0x00, 0x88 , 0x63 , 0x9d, 0x6b , 0xf8 , 0x47 , 0xed , 0x59 , 0xc6 };
	u8 testIV[10]= { 0x00 , 0x11 , 0x22 , 0x33 , 0x44 , 0x55 , 0x66 , 0x77 , 0x88 , 0x99 };
	u8 encipheredText[9]={ 0x66,0x86,0x54,0xac,0x21,0x4c,0x26,0x94,0xef };
	u8 output[9];
	u8 text[9]={'S','O','S','E','M','A','N','U','K'};
	u8 data=0;
	/* iv for vector test */
	
	printf("\t TEST VECTOR\n");
	
	printf("KEY 0x");
	for(i=0;i<10;i++)
	{
		printf("%x",testKEY[i]);
	}
	printf("\n");
	
	printf("IV 0x");
	for(i=0;i<10;i++)
	{
		printf("%x",testIV[i]);
	}
	printf("\n");
	
	printf("Plain Text: ");
	for(i=0;i<9;i++)
	{
		printf("%c",text[i]);
	}
	printf("\n");
	
	
	ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
	printf("Key loaded\n");
	ECRYPT_ivsetup( &ctx , testIV );
	printf("IV loaded\n");
	ECRYPT_process_bytes( 0, &ctx , text , output , 9); 
	printf("Enciphered Text: 0x");
	for(i=0;i<9;i++)
	{
		if(encipheredText[i] != output[i])
		{
			printf("ERROR\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("%x",output[i]);
		}
	}
	printf("\n");
	
	ECRYPT_keysetup( &ctx , testKEY , ECRYPT_MAXKEYSIZE , ECRYPT_MAXIVSIZE);
	ECRYPT_ivsetup( &ctx , testIV );
	ECRYPT_process_bytes( 1, &ctx , output , output, 9); 
	
	printf("Deciphered Text: ");
	for(i=0;i<9;i++)
	{
		printf("%c",output[i]);
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
		for( j=0 ; j<(MB/256) ; j++)
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
		printf("Load KEY per second: %.0f\n", (double)(MB*NUM_MB) / (time*256));
	}
	printf("data = 0x%x\n", data);
	return 0;
}
#endif





