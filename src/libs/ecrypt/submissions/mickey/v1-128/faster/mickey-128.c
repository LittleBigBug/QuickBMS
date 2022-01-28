/* This file contains an implementation of functions declared in ecrypt-sync.h so
   as to realise the keystream generator MICKEY-128 version 1 */

/* Date started      :- 28/4/05 */
/* Date last altered :- 29/4/05 */

/* Include the header file ecrypt-sync.h, edited for MICKEY-128 v 1 */
#include "ecrypt-sync.h"

/* Declare static variables, independent of key or IV */

u32 R_Mask[4];
    /* Feedback mask associated with the register R */
u32 Comp0[4];
    /* Input mask associated with register S */
u32 Comp1[4];
    /* Second input mask associated with register S */
u32 S_Mask0[4];
    /* Feedback mask associated with the register S for clock control bit = 0 */
u32 S_Mask1[4];
    /* Feedback mask associated with the register S for clock control bit = 1 */


/*
 * Key and message independent initialization. This function will be
 * called once when the program starts.
 */
   

void ECRYPT_init(void)
{
    /* Initialise the feedback mask associated with register R */
    R_Mask[0] = 0x9c80facf;
    R_Mask[1] = 0xe0d18929;
    R_Mask[2] = 0xc6be76e1;
    R_Mask[3] = 0x730c5d51;

    /* Initialise Comp0 */
    Comp0[0]  = 0x5dd6f25f;
    Comp0[1]  = 0x79260955;
    Comp0[2]  = 0x79007062;
    Comp0[3]  = 0x37afd931;

    /* Initialise Comp1 */
    Comp1[0]  = 0x7d191f31;
    Comp1[1]  = 0xfeb63c98;
    Comp1[2]  = 0x7c00c3e0;
    Comp1[3]  = 0x6660e345;

    /* Initialise the feedback masks associated with register S */
    S_Mask0[0] = 0xc43c1faf;
    S_Mask0[1] = 0x0e2fa322;
    S_Mask0[2] = 0x66e54d81;
    S_Mask0[3] = 0xd4544b91;

    S_Mask1[0] = 0x9bf477ab;
    S_Mask1[1] = 0x70798c90;
    S_Mask1[2] = 0x6f9a18b6;
    S_Mask1[3] = 0x6c4b7ee7;
}

/* The following routine clocks register R in ctx with given input and control bits */

void CLOCK_R(
  ECRYPT_ctx* ctx, 
  int input_bit,
  int control_bit)
{
    int Feedback_bit;
        /* r_127 ^ input bit */
    int Carry0, Carry1, Carry2;
        /* Respectively, carry from R[0] into R[1], carry from R[1] into R[2] and carry from R[2] into R[3] */

    /* Initialise the variables */
    Feedback_bit = ((ctx->R[3] >> 31) & 1) ^ input_bit;
    Carry0 = (ctx->R[0] >> 31) & 1;
    Carry1 = (ctx->R[1] >> 31) & 1;
    Carry2 = (ctx->R[2] >> 31) & 1;

    if (control_bit)
    {
        /* Shift and xor */
        ctx->R[0] ^= (ctx->R[0] << 1);
        ctx->R[1] ^= (ctx->R[1] << 1) ^ Carry0;
        ctx->R[2] ^= (ctx->R[2] << 1) ^ Carry1;
        ctx->R[3] ^= (ctx->R[3] << 1) ^ Carry2;
    }
    else
    {
        /* Shift only */
        ctx->R[0] = (ctx->R[0] << 1);
        ctx->R[1] = (ctx->R[1] << 1) ^ Carry0;
        ctx->R[2] = (ctx->R[2] << 1) ^ Carry1;
        ctx->R[3] = (ctx->R[3] << 1) ^ Carry2;
    }

    /* Implement feedback into the various register stages */
    if (Feedback_bit)
    {
        ctx->R[0] ^= R_Mask[0];
        ctx->R[1] ^= R_Mask[1];
        ctx->R[2] ^= R_Mask[2];
        ctx->R[3] ^= R_Mask[3];
    }
}

/* The following routine clocks register S in ctx with given input and control bits */

void CLOCK_S(
  ECRYPT_ctx* ctx, 
  int input_bit,
  int control_bit)
{
    int Feedback_bit;
        /* s_127 ^ input bit */
    int Carry0, Carry1, Carry2;
        /* Respectively, carry from S[0] into S[1], carry from S[1] into S[2] and carry from S[2] into S[3] */

    /* Compute the feedback and carry bits */
    Feedback_bit = ((ctx->S[3] >> 31) & 1) ^ input_bit;
    Carry0 = (ctx->S[0] >> 31) & 1;
    Carry1 = (ctx->S[1] >> 31) & 1;
    Carry2 = (ctx->S[2] >> 31) & 1;

    /* Derive "s hat" according to the MICKEY-128 v 0.4 specification */
    ctx->S[0] = (ctx->S[0] << 1) ^ ((ctx->S[0] ^ Comp0[0]) & ((ctx->S[0] >> 1) ^ (ctx->S[1] << 31) ^ Comp1[0]) & 0xfffffffe);
    ctx->S[1] = (ctx->S[1] << 1) ^ ((ctx->S[1] ^ Comp0[1]) & ((ctx->S[1] >> 1) ^ (ctx->S[2] << 31) ^ Comp1[1])) ^ Carry0;
    ctx->S[2] = (ctx->S[2] << 1) ^ ((ctx->S[2] ^ Comp0[2]) & ((ctx->S[2] >> 1) ^ (ctx->S[3] << 31) ^ Comp1[2])) ^ Carry1;
    ctx->S[3] = (ctx->S[3] << 1) ^ ((ctx->S[3] ^ Comp0[3]) & ((ctx->S[3] >> 1) ^ Comp1[3]) & 0x7fffffff) ^ Carry2;

    /* Apply suitable feedback from s_127 */
    if (Feedback_bit)
    {
        if (control_bit)
        {
            ctx->S[0] ^= S_Mask1[0];
            ctx->S[1] ^= S_Mask1[1];
            ctx->S[2] ^= S_Mask1[2];
            ctx->S[3] ^= S_Mask1[3];
        }
        else
        {
            ctx->S[0] ^= S_Mask0[0];
            ctx->S[1] ^= S_Mask0[1];
            ctx->S[2] ^= S_Mask0[2];
            ctx->S[3] ^= S_Mask0[3];
        }
    }
}

/* The following routine implements a clock of the keystream generator.  The parameter mixing is set to 0
   or a non-zero value to determine whether mixing (from s_64) is not/is applied; the parameter input_bit
   is used to specify any input bit to the generator */

int CLOCK_KG (
  ECRYPT_ctx* ctx,
  int mixing,
  int input_bit)
{
    int Keystream_bit;
        /* Keystream bit to be returned (only valid if mixing = 0 and input_bit = 0 */
    int control_bit_r;
        /* The control bit for register R */
    int control_bit_s;
        /* The control bit for register S */

    Keystream_bit = (ctx->R[0] ^ ctx->S[0]) & 1;
    control_bit_r = ((ctx->S[1] >> 11) ^ (ctx->R[2] >> 21)) & 1;
    control_bit_s = ((ctx->R[1] >> 10) ^ (ctx->S[2] >> 21)) & 1;

    if (mixing)
        CLOCK_R (ctx, ((ctx->S[2]) & 1) ^ input_bit, control_bit_r);
    else
        CLOCK_R (ctx, input_bit, control_bit_r);
    CLOCK_S (ctx, input_bit, control_bit_s);

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
    for (i = 0; i<16; i++)
        ctx->key[i] = key[i];

    /* Store the iv size in the context too */
    ctx->ivsize = ivsize;
}

/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */

/* This routine implements key loading according to the MICKEY v1 specification */

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
    int i;
        /* Counting/indexing variable */
    int iv_or_key_bit;
        /* Bit being loaded */


    /* Initialise R and S to all zeros */
    for (i=0; i<4; i++)
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
    for (i=0; i<128; i++)
    {
        iv_or_key_bit = (ctx->key[i/8] >> (7-(i%8))) & 1; /* Adopt usual, perverse, labelling order */
        CLOCK_KG (ctx, 1, iv_or_key_bit);
    }

    /* Preclock */
    for (i=0; i<128; i++)
        CLOCK_KG (ctx, 1, 0);
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

