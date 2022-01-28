/* This file contains a simple, "one bit at a time" implementation of functions declared 
   in ecrypt-sync.h so as to realise the keystream generator MICKEY version 1 */

/* Date started      :- 28/4/05 */
/* Date last altered :- 28/4/05 */

/* Include the header file ecrypt-sync.h, edited for this bit-by-bit implementation of MICKEY v1 */
#include "ecrypt-sync.h"

u8 R_Mask[80] = {1,0,1,0,1,0,1,1,1,1,0,0,0,1,1,0,1,1,0,0,1,0,1,0,1,0,1,1,1,0,0,0,0,0,1,1,0,1,0,1,
                 0,1,0,1,0,0,0,0,0,1,0,1,1,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,0,1,0,1,1};
    /* Feedback mask associated with the register R (independent of key or IV) */
u8 COMP0[79]  = {0,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1,0,0,1,0,0,0,0,
		         0,0,0,1,0,1,0,1,0,1,0,0,0,0,1,0,1,0,0,1,1,1,1,0,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0};
u8 COMP1[79]  = {0,1,0,1,1,0,0,1,0,1,1,1,1,0,0,1,0,1,0,0,0,1,1,0,1,0,1,1,1,0,1,1,1,1,0,0,0,1,1,0,
		         1,0,1,1,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,0,0,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1,0};
    /* Determines whether certain bits are complemented when used in the S feedback function */
u8 FB0[80]    = {1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,0,0,0,0,1,
		         1,1,0,0,1,0,0,1,0,1,0,1,0,0,1,0,1,1,1,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,1,0,1,0};
u8 FB1[80]    = {1,1,1,0,1,1,1,0,0,0,0,1,1,1,0,1,0,0,1,1,0,0,0,1,0,0,1,1,0,0,1,0,1,1,0,0,0,1,1,0,
		         0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,1,1,0,1,0,1,0,0,1,0,1,0,0,0,1,1};
	/* Two alternative sets of stages into which s79 is fed back, Galois-style */

/*
 * Key and message independent initialization. This function will be
 * called once when the program starts.
 */

void ECRYPT_init(void)
{
	return;
}


/* The following routine clocks register R in ctx with given input and control bits */

void CLOCK_R(
    ECRYPT_ctx* ctx, 
    u8 input_bit_r,
    u8 control_bit_r)
{
    u8 Feedback_bit;
        /* r_79 ^ input bit */
    int i;
        /* Index variable */

    Feedback_bit = ctx->R[79] ^ input_bit_r;

    if (control_bit_r)
    {
        /* Shift and XOR */
        if (Feedback_bit)
        {
            for (i=79; i>0; i--)
                ctx->R[i] = ctx->R[i-1] ^ ctx->R[i] ^ R_Mask[i];
            ctx->R[0] = R_Mask[0] ^ ctx->R[0];
        }
        else
        {
            for (i=79; i>0; i--)
                ctx->R[i] = ctx->R[i-1] ^ ctx->R[i];
        }
    }
    else
    {   
        /* Shift only */
        if (Feedback_bit)
        {
            for (i=79; i>0; i--)
                ctx->R[i] = ctx->R[i-1] ^ R_Mask[i];
            ctx->R[0] = R_Mask[0];
        }
        else
        {
            for (i=79; i>0; i--)
                ctx->R[i] = ctx->R[i-1];
            ctx->R[0] = 0;
        }
    }
    
}


/* The following routine clocks register S in ctx with given input and control bits */

void CLOCK_S(
    ECRYPT_ctx* ctx, 
    u8 input_bit_s,
    u8 control_bit_s)
{
    u8 s_hat[80];
		/* Intermediate values of the s stages */
	u8 Feedback_bit;
        /* s_79 ^ input bit */
    int i;
        /* Index variable */

    Feedback_bit = ctx->S[79] ^ input_bit_s;

    for (i=78; i>0; i--)
        s_hat[i] = ctx->S[i-1] ^ ((ctx->S[i] ^ COMP0[i]) & (ctx->S[i+1] ^ COMP1[i]));
	s_hat[0] = 0;
	s_hat[79] = ctx->S[78];

    for (i=0; i<80; i++)
		ctx->S[i] = s_hat[i];
	if (Feedback_bit)
    {
        if (control_bit_s)
        {
            for (i=0; i<80; i++)
                ctx->S[i] = s_hat[i] ^ FB1[i];
        }
        else
        {
            for (i=0; i<80; i++)
                ctx->S[i] = s_hat[i] ^ FB0[i];
        }
    }
}


/* The following routine obtains a keysteam bit from the generator, which it then clocks */

int CLOCK_KG(
  ECRYPT_ctx* ctx,
  u8 mixing,
  u8 input_bit)
{
    int Keystream_bit;
        /* Keystream bit to be returned */
	u8 Control_bit_R, Control_bit_S;
		/* Control the variable clocking of the R and S registers */

    Keystream_bit = (ctx->R[0] ^ ctx->S[0]) & 1;
	Control_bit_R = ctx->S[27] ^ ctx->R[53];
	Control_bit_S = ctx->S[53] ^ ctx->R[26];
    if (mixing)
		CLOCK_R (ctx, input_bit ^ ctx->S[40], Control_bit_R);
	else
		CLOCK_R (ctx, input_bit, Control_bit_R);
    CLOCK_S (ctx, input_bit, Control_bit_S);

    return Keystream_bit;
}


/* Key setup: simply save the key in ctx for use during IV setup */

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits. */ 
  u32 ivsize)                 /* IV size in bits. */
{
    int i;
        /* Indexing variable */

    /* Store the key in the algorithm context */
    for (i = 0; i<10; i++)
        ctx->key[i] = key[i];

	/* Remember the IV size */
	ctx->ivsize = ivsize;
}


/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */

/* This routine implements key loading according to the MICKEY specification */

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
    u8 i;
        /* Counting/indexing variable */
    u8 iv_or_key_bit;
        /* Bit being loaded */


    /* Initialise R and S to all zeros */
    for (i=0; i<80; i++)
    {
        ctx->R[i] = 0;
        ctx->S[i] = 0;
    }

    /* Load in IV */
    for (i=0; i<ctx->ivsize; i++)
    {
        iv_or_key_bit = (iv[i/8] >> (7-(i%8))) & 1; /* Adopt usual, perverse, labelling order */
        CLOCK_KG (ctx, 1, iv_or_key_bit);
    }

    /* Load in K */
    for (i=0; i<80; i++)
    {
        iv_or_key_bit = (ctx->key[i/8] >> (7-(i%8))) & 1; /* Adopt usual, perverse, labelling order */
        CLOCK_KG (ctx, 1, iv_or_key_bit);
    }

    /* Preclock */
    for (i=0; i<80; i++)
    {
        CLOCK_KG (ctx, 1, 0);
    }
}


/* Stream cipher a block of data */

void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)                 /* length in bytes */
{
    u32 i, j;
        /* Counting variables */

    for (i=0; i<msglen; i++)
    {
        output[i] = input[i];

        for (j=0; j<8; j++)
            output [i] ^= CLOCK_KG (ctx, 0, 0) << (7-j);
    }
}

/* Generate keystream data */

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length)                 /* Length of keystream in bytes. */
{
    u32 i, j;
        /* Counting variables */

    for (i=0; i<length; i++)
    {
        keystream[i] = 0;

        for (j=0; j<8; j++)
            keystream[i] ^= CLOCK_KG (ctx, 0, 0) << (7-j);
    }
}

