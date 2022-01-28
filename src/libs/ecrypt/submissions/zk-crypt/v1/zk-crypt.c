/* ecrypt-zk-crypt.c, 26.08.2005 */

/* 
 * Reference implementation of the ZK-Crypt Stream Cipher
 *
 * This programm is a simulation of the version 3 ZK-Crypt HW core,
 * 26.08.2005.
 *
 * Simulation of presently available version 5, to be submitted by 
 * 15.09.2005.
 * 
 *
 * All rights reserved to FortressGB Ltd.
 *
 * Author: Vladimir Rivkin
 */

/* ------------------------------------------------------------------------- */

#include "ecrypt-sync.h"
#include "ecrypt-zk-crypt.h"
 
/* ------------------------------------------------------------------------- */
/* internal constant buffers for ZK-Crypt                                    */
/* ------------------------------------------------------------------------- */
const u8 johnson_table [32] = 
{
    0x01, 0x02, 0x04, 0x01, 0x08, 0x01, 0x01, 0x01, /* j=0 */
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x08, 0x04, 0x01, 0x02, 0x01, 0x01, 0x01, /* j=1 */
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 
};

const u8 sfr2_table [8] = 
{                           /* nLSFR 2bits with carry 4C */
    0x03, 0x03, 0x01, 0x02,
    0x03, 0x03, 0x01, 0x03 
};

const u8 counter_table [32] = 
{                           /* counter upto 15 reload values 13C */
    0x00, 0x01, 0x02, 0x03,
    0x01, 0x00, 0x03, 0x02,
    0x02, 0x03, 0x00, 0x01,
    0x03, 0x02, 0x01, 0x00,
    0x08, 0x09, 0x0A, 0x0B,
    0x09, 0x08, 0x0B, 0x0A,
    0x0A, 0x0B, 0x08, 0x09,
    0x0B, 0x0A, 0x09, 0x08 
};

const u8 sfr3_table [8] = 
{                           /* nLSFR 3bits 13C */
    0x01, 0x03, 0x04, 0x07,
    0x00, 0x02, 0x05, 0x06 
};

const u8 sfr5_table [32] = 
{                           /* nLSFR 5bits 14C */
    0x01, 0x02, 0x05, 0x07, 0x08, 0x0A, 0x0D, 0x0F,
    0x10, 0x12, 0x15, 0x17, 0x18, 0x1A, 0x1D, 0x1F,
    0x00, 0x03, 0x04, 0x06, 0x09, 0x0B, 0x0C, 0x0E,
    0x11, 0x13, 0x14, 0x16, 0x19, 0x1B, 0x1C, 0x1E 
};

const u8 sfr6_table [64] = 
{                           /* nLSFR 6bits 15C */
    0x01, 0x03, 0x04, 0x07, 0x08, 0x0B, 0x0C, 0x0F,
    0x10, 0x13, 0x14, 0x17, 0x18, 0x1B, 0x1C, 0x1F,
    0x20, 0x23, 0x24, 0x27, 0x28, 0x2B, 0x2C, 0x2F,
    0x30, 0x33, 0x34, 0x37, 0x38, 0x3B, 0x3C, 0x3F,
    0x00, 0x02, 0x05, 0x06, 0x09, 0x0A, 0x0D, 0x0E,
    0x11, 0x12, 0x15, 0x16, 0x19, 0x1A, 0x1D, 0x1E,
    0x21, 0x22, 0x25, 0x26, 0x29, 0x2A, 0x2D, 0x2E,
    0x31, 0x32, 0x35, 0x36, 0x39, 0x3A, 0x3D, 0x3E 
};


const u8 hash_table_A [32] = 
{ 
    9,18, 5,11,22,12,30,19, 7,15,31,25,28,24, 6, 3,17,13,27,23, 1, 2,26,21, 4,20, 8,16, 0,14,10,29
};
const u8 hash_table_B [32] = 
{
    30,15, 6,12,25,18,16, 9,19, 7, 3,31, 0,29,27,21,14,28,24,17,23, 5,10, 2,11,22,13,26,20, 8, 4, 1
};
const u8 hash_table_C [32] = 
{
    19, 7,14,29, 3,27, 0,13,25,16,15,30,20, 1,26,31, 8, 6, 2, 4, 9,18,12,10,21,11,22, 5,24,23,28,17
};

/* ------------------------------------------------------------------------- */
/* nLSFR                                                                     */
/* ------------------------------------------------------------------------- */

/* declare nLSFR */
const u32   shift_len  [6] = {13, 14, 15, 17, 18, 19};  /* registers length */
const u32   shift_mask [6] = 
{                           /* feedback masks */
    (1<<0)+(1<<3)+(1<<4)+(1<<6)+(1<<9)+(1<<10),     /* 13 */
    (1<<0)+(1<<2)+(1<<5)+(1<<6)+(1<<9)+(1<<11),     /* 14 */
    (1<<0)+(1<<1)+(1<<2)+(1<<6)+(1<<7)+(1<<11),     /* 15 */
    (1<<0)+(1<<2)+(1<<5)+(1<<8)+(1<<10)+(1<<11)+(1<<13)+(1<<14),     /* 17 */
    (1<<0)+(1<<3)+(1<<5)+(1<<7)+(1<<8) +(1<<11)+(1<<12)+(1<<13)+(1<<14)+(1<<16),     /* 18 */
    (1<<0)+(1<<2)+(1<<5)+(1<<7)+(1<<8) +(1<<9) +(1<<10)+(1<<12)+(1<<15)+(1<<17)      /* 19 */
};

/* ------------------------------------------------------------------------- */
/*                      External ECRYPT API Functions                        */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

void ECRYPT_init(void)
{ 
    return;
}

/* ------------------------------------------------------------------------- */

void ECRYPT_keysetup(
    ECRYPT_ctx* ctx, 
    const u8* key, 
    u32 keysize,
    u32 ivsize)
{
    u32 i = 0;
    /*
    1. Reset
    */
    internal_reset (ctx);
    system_reset   (ctx);
    /*
    2. Initialize key, keysize, ivsize.
    */
    ctx->keylen = (keysize + 7) / 8; /*  bytes from bits */
    ctx->ivlen  = (ivsize + 7) / 8;  /*  bytes */

    for (i = 0;i < ctx->keylen; i++)
    {
        ctx->key[i] = key[i];
    }
    return;
}

/* ------------------------------------------------------------------------- */

void ECRYPT_ivsetup(
    ECRYPT_ctx* ctx, 
    const u8* iv)
{
    /*
    1. update the ctx->iv.
    2. create session key.
    3. initialize inner machine state and state from key.
    3. execute 32 empty cicles;
    */

    u32 i = 0;

    for (i = 0; i < ctx->ivlen; i++)
    {
        ctx->iv[i]  = iv[i];
    } 

    for (i = 0; i < ctx->keylen; i++)
    {
        ctx->session_key[i]  = (ctx->key[i]) ^ (ctx->iv[i]);
    } 
    
    ctx->mode = MODE_FB_A;
    ctx->mode |= BOT_TIER_ALWAYS;
    ctx->mode |= MODE_BRAWNIAN;
    ctx->mode |= ENABLE_ODDNS;
    /* ctx->mode |= ENABLE_SINGLE_TIER; */
    /* ctx->mode |= MODE_FB_ENABLE; */
    ctx->mode |= MID_TIER_ALWAYS;
    ctx->mode |= TOP_TIER_ALWAYS;

    ctx->SAMPLE_PERIOD = 0; /* 0 - each clock, N - each N clocks 
                               we use here sample per clock      */

    ZK_CRYPT_StartAlgorithm (ctx);
    
    /* execute 32 machine rounds          */
    /* Dummy Samples with Feedback cycles */
    ctx->type_of_operation = RNG_OPERATION;
    ctx->mode |= MODE_FB_ENABLE;/*enable feedback*/
    for (i = 0; i < 32; i++)
    {
        ZK_CRYPT_NextRandom(ctx);   
    }
    ctx->mode &= ~MODE_FB_ENABLE;/*disable feedback*/
    return;
}

/* ------------------------------------------------------------------------- */
#include <stdio.h>
void ECRYPT_process_bytes(
    int action,
    ECRYPT_ctx* ctx, 
    const u8* input, 
    u8* output, 
    u32 msglen) /*assumed length in bytes*/
{
    u32 i = 0;
    /* we update here the type of operation because */
    ctx->type_of_operation = STREAM_CIPHER_OPERATION;
    
    for (i = 0; i < msglen/4; i++)
    {
        ctx->message_in = U8TO32_LITTLE(input + i * 4);
	ZK_CRYPT_NextRandom(ctx);
        U32TO8_LITTLE(output + i * 4, ctx->data_result);
    }

    if ((i *= 4) < msglen)
      {
	ctx->message_in = 0;
	ZK_CRYPT_NextRandom(ctx);

	for ( ; i < msglen; i++)
	  output[i] = input[i] ^ U8V(ctx->data_result >> (i % 4));
      }

    return;
}

/* ------------------------------------------------------------------------- */

void ECRYPT_keystream_bytes(
    ECRYPT_ctx* ctx,           /* Pointer to the internal patameters structure */
    u8* keystream,             /* Pointer to the keystream.                    */
    u32 length)                /* Length of keystream in bytes.                */
{
    u32 i = 0;
    ctx->type_of_operation = RNG_OPERATION;

    for (i = 0; i < (length>>2); i++)
    {
        ZK_CRYPT_NextRandom(ctx);
        U32TO8_LITTLE(keystream + i * 4, ctx->data_result);
    }

    if ((i *= 4) < length)
      {
	ZK_CRYPT_NextRandom(ctx);

	for ( ; i < length; i++)
	  keystream[i] = U8V(ctx->data_result >> (i % 4));
      }

    return;
}

/* ------------------------------------------------------------------------- */

void ECRYPT_create_digest(
  ECRYPT_ctx* ctx,            /* inner parameters structure                 */
  const u8* msg,              /* pointer to the hashed message              */
  u8* digest,                 /* pointer to the preallocated 160 bit digest */
  u32* header_words,           /* 5 header 32-bit words (x_hdr)              */
  u32 tail_word,              /* the tail 32-bit word x_t                   */
  u32 msglen )                /* length of the message in bytes.            */ 

{
    u32 i = 0;
    
    /*
    RESET
    */

    internal_reset (ctx);
    system_reset   (ctx);

    /*
    INITIALIZE KEY, KEYSIZE, IVSIZE, SESSION_KEY TO ZERO.
    */

    ctx->keylen = 16; /*  bytes */
    ctx->ivlen  = 16; /*  bytes */
    for (i = 0;i < ctx->keylen; i++)
    {
        ctx->key[i] = 0;
        ctx->iv [i]  = 0;
        ctx->session_key[i]  = 0; 
    }

    /* 
    SET THE MODE OF OPERATION FOR MAC
    */

    ctx->mode = MODE_FB_A;
    ctx->mode |= BOT_TIER_ALWAYS;
    ctx->mode |= MODE_BRAWNIAN;
    ctx->mode |= ENABLE_ODDNS;
    /* ctx->mode |= ENABLE_SINGLE_TIER; */
    ctx->mode |= MODE_FB_ENABLE;
    ctx->mode |= MID_TIER_ALWAYS;
    ctx->mode |= TOP_TIER_ALWAYS;
    ctx->type_of_operation = HASH_OPERATION;

    ctx->SAMPLE_PERIOD = 0; 

    ZK_CRYPT_StartAlgorithm (ctx);
    
    /*
    PROCESS THE FIVE HEADER WORDS:
    */

    for (i = 0; i < 5; i++)
    {
        ctx->message_in = header_words[i];
        ZK_CRYPT_NextRandom(ctx);
    }
    
    /*
    DATA AUTHENTIACATION ENCODING
    */
    
    for (i = 0; i < msglen>>2; i++)
    {
        ctx->message_in = ((u32*)msg)[i];
        ZK_CRYPT_NextRandom(ctx);
    }

    /*
    PROCESS THE TAIL WORD:
    */
    
    ctx->message_in = tail_word;
    ZK_CRYPT_NextRandom(ctx);
    
    /*
    MAC SIGNATURE - TAG
    */

    ctx->message_in = 0;

    for (i = 0; i < MAC_SIGNATURE_LEN_32B_WORDS; i++)
    {
       ((u32*)digest)[i] = ZK_CRYPT_NextRandom(ctx);
    }

    return;
}
/* ------------------------------------------------------------------------- */
/*                      Internal ZK-Crypt Functions                          */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
void ZK_CRYPT_StartAlgorithm (ECRYPT_ctx* ctx)
{
	ctx->IterationNumber = 0;
    init_preload (ctx);
    return;
}

u32 ZK_CRYPT_NextRandom(ECRYPT_ctx* ctx)
{
    
    if(ctx->type_of_operation == RNG_OPERATION)
    {
        ctx->message_in = 0; /* for RNG mode zero the message word */
    }
    
    /*
    taking care of sample period:
    */
	if (ctx->SAMPLE_PERIOD != 0)
	{
		/* SAMPLE each N iterations */
		if ((ctx->IterationNumber % ctx->SAMPLE_PERIOD)==0)
		  ctx->mode |= SAMPLE;
		else
		  ctx->mode &= ~SAMPLE;
	}
	else
	{
		/* SAMPLE each iteration */
		ctx->mode |= SAMPLE;     /* sample each clock */
	}

    cypher_machine (ctx);

    tier_controller (ctx);     /* 10S */

	ctx->IterationNumber++;

	return ctx->data_result;
}

void nLSFR_iteration (u32 cipher_word, u32 feedback_word, 
                      int enable_cipher, int slip, int index,
                      ECRYPT_ctx* ctx)
{
    u32 *reg = &ctx->shift_reg[index];
    u32 next_data = 0;
    u32 mask = ((1<<shift_len[index])-1);
    int   feedback_bit = 0;

    if (enable_cipher)
    {
        *reg  = cipher_word;        /* parallel load of top_cipher word */
    }
    else
    {
        next_data  = (*reg << 1);   /* shift left */

        feedback_bit = (*reg >> (shift_len[index]-1));          /* MSB bit */
        feedback_bit = feedback_bit ^ (slip!=0);            /* slip */
        feedback_bit = feedback_bit ^ (((*reg)&(mask>>1))==0);          /* register zero (except MSB bit) */

        if (feedback_bit & 1)
        {
            next_data ^= shift_mask[index];        /* xor mask if feedback */
        }

        next_data ^= feedback_word;

        *reg = next_data & mask;    /* shift iteration */
    }
    return;
}

/* ------------------------------------------------------------------------- */

u32 tier_shift (u32 input)
{
	u32 data = 0;

	data  = input << 1;
	data |= input >> 31;
	data ^= input;

	return data;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* tier control fields */


/* ------------------------------------------------------------------------- */

void top_tier (ECRYPT_ctx* ctx)
{
    u32 mask_13bits   = ((1<<13)-1);
    
    u32 cipher_left   = ctx->top_cipher_word & mask_13bits;      /* bits [12:0] */
    u32 feedback_left = ctx->feedback_word   & mask_13bits;      /* bits [12:0] */

    u32 cipher_right   = ctx->top_cipher_word >> 13;             /* bits [31:13] */
    u32 feedback_right = ctx->feedback_word   >> 13;             /* bits [31:13] */

	if (ctx->top_control & LOAD)
	{
		/* load cipher_word to nLSFR */
		nLSFR_iteration (cipher_left,  0, 1, 0, DEF_LSFR_13,ctx);     /* left  nLSFR */
		nLSFR_iteration (cipher_right, 0, 1, 0, DEF_LSFR_19,ctx);     /* right nLSFR */
	}
	else
	{
		/* iteration with feedback_word */
		nLSFR_iteration (0, feedback_left,  0, ctx->com_control & SLIP_LEFT,  DEF_LSFR_13,ctx);     /* left  nLSFR */
		nLSFR_iteration (0, feedback_right, 0, ctx->com_control & SLIP_RIGHT, DEF_LSFR_19,ctx);     /* right nLSFR */
	}

    ctx->top_result = ctx->shift_reg[DEF_LSFR_13] | (ctx->shift_reg[DEF_LSFR_19] << 13);
    return;
}

/* ------------------------------------------------------------------------- */

void mid_tier (ECRYPT_ctx* ctx)
{
    u32 mask_18bits   = ((1<<18)-1);
    
    u32 cipher_left   = ctx->mid_cipher_word & mask_18bits;      /* bits [17:0] */
    u32 feedback_left = ctx->feedback_word   & mask_18bits;      /* bits [17:0] */

    u32 cipher_right   = ctx->mid_cipher_word >> 18;             /* bits [31:18] */
    u32 feedback_right = ctx->feedback_word   >> 18;             /* bits [31:18] */

	if (ctx->mid_control & LOAD)
	{
		/* load cipher_word to nLSFR */
		nLSFR_iteration (cipher_left,  0, 1, 0, DEF_LSFR_18,ctx);     /* left  nLSFR */
		nLSFR_iteration (cipher_right, 0, 1, 0, DEF_LSFR_14,ctx);     /* right nLSFR */
	}
	else
	{
		/* iteration with feedback_word */
		nLSFR_iteration (0, feedback_left,  0, ctx->com_control & SLIP_LEFT,  DEF_LSFR_18,ctx);     /* left  nLSFR */
		nLSFR_iteration (0, feedback_right, 0, ctx->com_control & SLIP_RIGHT, DEF_LSFR_14,ctx);     /* right nLSFR */
	}

    ctx->mid_result = ctx->shift_reg[DEF_LSFR_18] | (ctx->shift_reg[DEF_LSFR_14] << 18);
    return;
}

/* ------------------------------------------------------------------------- */

void bot_tier (ECRYPT_ctx* ctx)
{
    u32 mask_15bits   = ((1<<15)-1);
    
    u32 cipher_left   = ctx->bot_cipher_word & mask_15bits;      /* bits [14:0] */
    u32 feedback_left = ctx->feedback_word   & mask_15bits;      /* bits [14:0] */

    u32 cipher_right   = ctx->bot_cipher_word >> 15;             /* bits [31:15] */
    u32 feedback_right = ctx->feedback_word   >> 15;             /* bits [31:15] */

	if (ctx->bot_control & LOAD)
	{
		/* load cipher_word to nLSFR */
		nLSFR_iteration (cipher_left,  0, 1, 0, DEF_LSFR_15,ctx);     /* left  nLSFR */
		nLSFR_iteration (cipher_right, 0, 1, 0, DEF_LSFR_17,ctx);     /* right nLSFR */
	}
	else
	{
		/* iteration with feedback_word */
		nLSFR_iteration (0, feedback_left,  0, ctx->com_control & SLIP_LEFT,  DEF_LSFR_15,ctx);     /* left  nLSFR */
		nLSFR_iteration (0, feedback_right, 0, ctx->com_control & SLIP_RIGHT, DEF_LSFR_17,ctx);     /* right nLSFR */
	}

    ctx->bot_result = ctx->shift_reg[DEF_LSFR_15] | (ctx->shift_reg[DEF_LSFR_17] << 15);
    return;
}

/* ------------------------------------------------------------------------- */

u32 majority_2_3 (u32 a, u32 b, u32 c)
{
    u32 a1 = a & b;
    u32 b1 = a & c;
    u32 c1 = b & c;
    
    u32 res = a1 | b1 | c1;

    return (res);
}

/* ------------------------------------------------------------------------- */

/* non-linear feedback shift register bank */

void nLSFR_bank (ECRYPT_ctx* ctx)
{
    u32 top_out = 0, mid_out = 0, bot_out = 0;

    /* top tier */
    if (ctx->top_control & CLOCK)
    {
		top_tier (ctx);
		top_out = tier_shift (ctx->top_result);	/* top tier reversed pseudo brownian motion bits */
    }
    else
    if (ctx->top_control & BROWN)
    {
	    top_out = tier_shift (ctx->top_result);
    }
    else
    {
	    top_out = ctx->top_result;
    }

    /* middle tier */
    if (ctx->mid_control & CLOCK)
    {
		mid_tier (ctx);
		mid_out = tier_shift (ctx->mid_result);	/* mid tier reversed pseudo brownian motion bits */
    }
    else
    if (ctx->mid_control & BROWN)
    {
	    mid_out = tier_shift (ctx->mid_result);
    }
    else
    {
	    mid_out = ctx->mid_result;
    }

    /* bottom tier */
    if (ctx->bot_control & CLOCK)
    {
		bot_tier (ctx);
		bot_out = tier_shift (ctx->bot_result);	/* bot tier reversed pseudo brownian motion bits */
    }
    else
    if (ctx->bot_control & BROWN)
    {
	    bot_out = tier_shift (ctx->bot_result);
    }
    else
    {
	    bot_out = ctx->bot_result;
    }

    /* old version: XOR three tiers output */
    ctx->bank_result = top_out ^ mid_out ^ bot_out;

    /* new version 2005.05.11: majority 2/3 */
    /* bank_result = majority_2_3 (top_out, mid_out, bot_out); */
    return;
}

/* ------------------------------------------------------------------------- */

/* Hash matrix & ODDN complementor */
/* This version without bit permutation */


void hash_mix (const u8 *table,
               ECRYPT_ctx* ctx)
{
    u32 data = 0;
    int i = 0;

    for (i=31; i>=0; i--)
    {
        data <<= 1;
        data |= ((ctx->hash_result >> table[i]) & 1);
    }

    ctx->hash_result = data;
    return;
}

void Hash_matrix (ECRYPT_ctx* ctx) /* hash matrix & oddn permute 17P */
{
    ctx->hash_result = ctx->bank_result;

    /* hash matrix A,B,C */
    if (ctx->hash_vector & VECTOR_A)
        hash_mix (hash_table_A,ctx);
    else
    if (ctx->hash_vector & VECTOR_B)
        hash_mix (hash_table_B,ctx);
    else
    if (ctx->hash_vector & VECTOR_C)
        hash_mix (hash_table_C,ctx);

    /* oddb permute */

    if (ctx->hash_control & TOP_ODDN)
		ctx->hash_result ^= TOP_ODDN_MASK;

    if (ctx->hash_control & MID_ODDN)
		ctx->hash_result ^= MID_ODDN_MASK;

    if (ctx->hash_control & BOT_ODDN)
		ctx->hash_result ^= BOT_ODDN_MASK;

    if (ctx->hash_control & ODDN4)
		ctx->hash_result ^= ODDN4_MASK;
    return;
}

/* ------------------------------------------------------------------------- */

u32 ROL3_func (u32 input)  /* rotate left 3 bits */
{
    u32 data = 0;
	data  = input << 3;
	data |= input >> (32-3);
    return (data);
}

u32 ROR1_func (u32 input)  /* rotate right 1 bits */
{
    u32 data = 0;
	data  = input >> 1;
	data |= input << (32-1);
    return (data);
}

/* Immunizer */



void Immunizer (ECRYPT_ctx* ctx)
{
    ctx->imm_result = ctx->hash_result ^ ctx->imm_data;    /* generate output */

    if (ctx->mode & SAMPLE)
    {
        ctx->imm_data = ctx->hash_result;
    }
    return;
}

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */


void cypher_machine (ECRYPT_ctx* ctx)
{
    /* store MSB of all nLSFR for tier control feedback */

    ctx->top_fbk  = (ctx->shift_reg [DEF_LSFR_14] >> 13) << 2;
    ctx->top_fbk |= (ctx->shift_reg [DEF_LSFR_18] >> 17) << 3;

    ctx->mid_fbk  = (ctx->shift_reg [DEF_LSFR_19] >> 18) << 2;
    ctx->mid_fbk |= (ctx->shift_reg [DEF_LSFR_13] >> 12) << 3;

    ctx->bot_fbk  = (ctx->shift_reg [DEF_LSFR_17] >> 16) << 2;
    ctx->bot_fbk |= (ctx->shift_reg [DEF_LSFR_15] >> 14) << 3;


    nLSFR_bank  (ctx);

    Hash_matrix (ctx);

    Immunizer   (ctx);

    ctx->prev_data_result = ctx->data_result;

    ctx->data_result      = ctx->message_in ^ ctx->imm_result;

    if (ctx->mode & SAMPLE)
    {
        if (ctx->mode & MODE_FB_ENABLE)
        {
            
            /*previous version:*/
            /*
            ctx->feedback_word = (ctx->mode & MODE_FB_A) ? ctx->data_result : ctx->imm_result;
            */

            /*
            new version:
            feedback receives xor between present and past results.
            */
            if((ctx->mode & MODE_FB_A) != 0)
            {
                ctx->feedback_word = ctx->data_result ^ ctx->prev_data_result;
            }
            else
            {
                ctx->feedback_word = ctx->imm_result;
            }
            
        }
    }
    return;
}
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void top_brown (ECRYPT_ctx* ctx)
{
    /* generate brown */
    if (ctx->mode & MODE_BRAWNIAN)
    {
        if (ctx->top_sfr3 & 1)   /* bit 0 of sfr */
            ctx->top_control |= BROWN;
    }
    return;
}

void top_slip (ECRYPT_ctx* ctx)
{
    /* generate slip */
    if (ctx->top_cnt == 15)                      
    {
        if (ctx->top_sfr3 & 4)   /* bit 2 of sfr */
            ctx->top_control |= SLIP_LEFT;
        else
            ctx->top_control |= SLIP_RIGHT;
    }
    return;
}

void top_controller (ECRYPT_ctx* ctx)  /* 13C */
{
    u32 a = 0;

    if (ctx->top_control & SLIP_LEFT)
    {
        ctx->control_cfg_FF ^= 1;        /* switch T-FF if previous SLIP_LEFT was 1 */
    }

    ctx->top_control = 0;

    /* counter behavioral */
    if (ctx->top_cnt < 15)               
    {
        ctx->top_cnt ++;
    }
    else
    {
        a  = ctx->top_sfr3 & 3;
        a |= ctx->top_fbk;       /* sfr14 bit 13, sfr18 bit 17 */
        a |= (ctx->top_sfr3 & 0x02) << 3;
        ctx->top_cnt = counter_table [a];
    }

    /* sfr3 behavioral */
    a = sfr3_table [ctx->top_sfr3];
    ctx->top_sfr3 = a;

    /* generate new controls */
    top_brown (ctx);
    top_slip  (ctx);
    return;
} /* top_controller */

/* ------------------------------------------------------------------------- */

void mid_brown (ECRYPT_ctx* ctx)
{
    /* generate brown */
    if (ctx->mode & MODE_BRAWNIAN)
    {
        if (ctx->mid_sfr5 & 1)   /* bit 0 of sfr */
            ctx->mid_control |= BROWN;
    }
    return;
}

void mid_slip (ECRYPT_ctx* ctx)
{
    /* generate slip */
    if (ctx->mid_cnt == 15)                      
    {
        if (ctx->mid_sfr5 & 16)   /* bit 4 of sfr */
            ctx->mid_control |= SLIP_LEFT;
        else
            ctx->mid_control |= SLIP_RIGHT;
    }
    return;
}

void mid_controller (ECRYPT_ctx* ctx)  /* 14C */
{
    u32 a = 0;

    if (ctx->mid_control & SLIP_LEFT)
    {
        ctx->control_cfg_FF ^= 2;        /* switch T-FF if previous SLIP_LEFT was 1 */
    }

    ctx->mid_control = 0;

    /* counter behavioral */
    if (ctx->mid_cnt < 15)               
    {
        ctx->mid_cnt ++;
    }
    else
    {
        a  = ctx->mid_sfr5 & 3;
        a |= ctx->mid_fbk;       /* sfr19 bit 18, sfr13 bit 12 */
        a |= (ctx->mid_sfr5 & 0x08) << 1;
        ctx->mid_cnt = counter_table [a];
    }

    /* sfr5 behavioral */
    a = sfr5_table [ctx->mid_sfr5];
    ctx->mid_sfr5 = a;

    /* generate new controls */
    mid_brown (ctx);
    mid_slip  (ctx);
    return;
} /* mid_controller */

/* ------------------------------------------------------------------------- */

void bot_brown (ECRYPT_ctx* ctx)
{
    /* generate brown */
    if (ctx->mode & MODE_BRAWNIAN)
    {
        if (ctx->bot_sfr6 & 1)   /* bit 0 of sfr */
            ctx->bot_control |= BROWN;
    }
    return;
}

void bot_slip (ECRYPT_ctx* ctx)
{
    /* generate slip */
    if (ctx->bot_cnt == 15)                      
    {
        if (ctx->bot_sfr6 & 32)   /* bit 5 of sfr */
            ctx->bot_control |= SLIP_LEFT;
        else
            ctx->bot_control |= SLIP_RIGHT;
    }
    return;
}

void bot_controller (ECRYPT_ctx* ctx)  /* 15C */
{
    u32 a = 0;

    if (ctx->bot_control & SLIP_LEFT)
    {
        ctx->control_cfg_FF ^= 4;        /* switch T-FF if previous SLIP_LEFT was 1 (clock delay) */
    }

    ctx->bot_control = 0;

    /* counter behavioral */
    if (ctx->bot_cnt < 15)               
    {
        ctx->bot_cnt ++;
    }
    else
    {
        a  = ctx->bot_sfr6 & 3;
        a |= ctx->bot_fbk;       /* sfr17 bit 16, sfr15 bit 14 */
        a |= (ctx->bot_sfr6 & 0x10) << 0;
        ctx->bot_cnt = counter_table [a];
    }

    /* sfr6 behavioral */
    a = sfr6_table [ctx->bot_sfr6];
    ctx->bot_sfr6 = a;

    /* generate new controls */
    bot_brown (ctx);
    bot_slip  (ctx);
    return;
} /* bot_controller */

/* ------------------------------------------------------------------------- */

int p_random_clock_generator (ECRYPT_ctx* ctx) /* p_random_clock, ODDN4, MJuggle hash toggle */
{
    u32 a = 0 , c = 0 , odd4 = 0;

    /* p_random_clock generation */
/* wrong behavioral because of negedge FF (F2)
    p_random_clock = (clock_t_rand != clock_p_rand);
    clock_p_rand = clock_t_rand;
    if ((clock_sfr5 & 16) || (clock_sfr2 & 2))
    {
        clock_t_rand = (clock_t_rand == 0);
    }
*/

    /* p_random_clock generation */
    ctx->p_random_clock = 0;
    if ((ctx->clock_sfr5 & 16) || (ctx->clock_sfr2 & 2))
    {
        ctx->clock_t_rand = (ctx->clock_t_rand == 0);
        ctx->p_random_clock = 1;
    }

    /* ODDN4 */
    odd4 = ctx->clock_odd4;
    ctx->clock_odd4 = (ctx->clock_sfr5 & 2) ? ODDN4 : 0;

    /* MJuggle hash toggle */
    ctx->mjuggle_hash_toggle = ctx->clock_mjuggle;
    ctx->clock_mjuggle = (ctx->clock_sfr5 & 0x10) == 0;

    /* sfr2 behavioral */
    c = ((ctx->clock_sfr5 & 0x0F) == 0) ? 4 : 0; /* carry */
    a = sfr2_table [ctx->clock_sfr2 + c];
    ctx->clock_sfr2 = a;

    /* sfr5 behavioral */
    a = sfr5_table [ctx->clock_sfr5];
    ctx->clock_sfr5 = a;
    if (ctx->com_control & SLIP_RIGHT)
    {
        ctx->clock_sfr5 ^= 1;        /* random clock slip from 10S */
    }

    return (odd4);
} /* p_random_clock_generator */

/* ------------------------------------------------------------------------- */
void johnson_counter (ECRYPT_ctx* ctx)
{
    u32 a = 0;

    a = ctx->hash_vector + (ctx->mjuggle_hash_toggle << 4);

    ctx->hash_vector = (u32) johnson_table[a];
    return;
}

/* ------------------------------------------------------------------------- */

void tier_controller (ECRYPT_ctx* ctx)   /* 10S */
{
    int oddn_4 = 0;

    if (ctx->p_random_clock)
    {/* generate top_brown, top_left_slip, top_right_slip, top_config_clock */
        top_controller (ctx);      
        mid_controller (ctx);
        bot_controller (ctx);
    }

    /* hash matrix stepper 18S */
    johnson_counter (ctx);

    /* generate (p)random_clock enable */

    oddn_4 = p_random_clock_generator (ctx);  /* p_random_clock,  ODDN4*/

    /* decode 3 config clocks & generate tier clocks */

    if (ctx->mode & ENABLE_SINGLE_TIER)
    {
        /* decode 3 config controls */
        switch (ctx->control_cfg_FF)
        {
        case 0:
        case 3:
        case 5: ctx->top_control |= CLOCK;   /* enable clock iteration */
                ctx->top_control |= BROWN;   /* enable (force) browning */
                break;
        case 4:
        case 7: ctx->mid_control |= CLOCK;   /* enable clock iteration */
                ctx->mid_control |= BROWN;   /* enable (force) browning */
                break;
        case 1:
        case 2:
        case 6: ctx->bot_control |= CLOCK;   /* enable clock iteration */
                ctx->bot_control |= BROWN;   /* enable (force) browning */
                break;
        }
    }
    else
    {
        if (ctx->mode & TOP_TIER_ALWAYS)
            ctx->top_control |= CLOCK;   /* enable clock iteration */
        if (ctx->mode & MID_TIER_ALWAYS)
            ctx->mid_control |= CLOCK;   /* enable clock iteration */
        if (ctx->mode & TOP_TIER_ALWAYS)
            ctx->bot_control |= CLOCK;   /* enable clock iteration */
    }

    /* added 2005.05.11 no all three browns together */
    if ((ctx->top_control & BROWN) && (ctx->mid_control & BROWN) && (ctx->bot_control & BROWN))
    {
        switch (ctx->control_cfg_FF)
        {
            case 0:
            case 3:
            case 5: ctx->top_control &= ~BROWN;   /* disable browning */
                    break;
            case 4:
            case 7: ctx->mid_control &= ~BROWN;    /* disable browning */
                    break;
            case 1:
            case 2:
            case 6: ctx->bot_control &= ~BROWN;    /* disable browning */
                    break;
        }
    }

    /* decode 3 L/R slips & generate left hand slip and right hand slip */

    ctx->com_control = 0;

    if ((ctx->top_control & SLIP_LEFT) || (ctx->mid_control & SLIP_LEFT) || (ctx->bot_control & SLIP_RIGHT))
    {
        ctx->com_control |= SLIP_LEFT;
    }

    if ((ctx->top_control & SLIP_RIGHT) || (ctx->mid_control & SLIP_RIGHT) || (ctx->bot_control & SLIP_LEFT))
    {
        ctx->com_control |= SLIP_RIGHT;
    }

    /* generate 4 ODDN */

    ctx->hash_control = 0;   /* TOP_ODDN */

    if (ctx->mode & ENABLE_ODDNS)
    {
        ctx->hash_control  = ctx->control_cfg_FF;     /* top_oddn = top_config_ff, mid = mid, bot = bot */
        ctx->hash_control |= oddn_4;             /* 4C */
    }
    return;
} /* tier_controller */

/* ------------------------------------------------------------------------- */

void system_reset (ECRYPT_ctx* ctx)
{
	int i=0; /* counter */

	for (i=0; i<6; i++)
	{
		ctx->shift_reg [i] = 0;		/* nLSFR */
	}

    ctx->top_result = 0;
    ctx->mid_result = 0;
    ctx->bot_result = 0;

    ctx->imm_data = 0;        
	ctx->imm_result = 0;       

    ctx->feedback_word = 0;/* also must be 0 if feedback is disabled */

    ctx->top_cnt = 0;      /* control counter 4bit */
    ctx->mid_cnt = 0;
    ctx->bot_cnt = 0;

    ctx->top_sfr3 = 0;     /* control LSFR 3bit */
    ctx->mid_sfr5 = 0;     /* control LSFR 5bit */
    ctx->bot_sfr6 = 0;     /* control LSFR 6bit */

    ctx->top_control    = 0;
    ctx->mid_control    = 0;
    ctx->bot_control    = 0;
    ctx->control_cfg_FF = 0;

    ctx->clock_sfr5     = 0;   /* 4C */
    ctx->clock_sfr2     = 3;   /* 4C */
    ctx->clock_p_rand   = 0;
    ctx->clock_t_rand   = 0;
    ctx->clock_odd4     = 0;
    ctx->clock_mjuggle  = 0;

    ctx->p_random_clock      = 0;         /* p_random_clock enable 4C */
    ctx->mjuggle_hash_toggle = 0;         /* 4C */
    ctx->hash_vector         = VECTOR_D;  /* 18S */

    /* v */
    ctx->top_fbk = 0;
	ctx->mid_fbk = 0;
	ctx->bot_fbk = 0;

    ctx->data_result      = 0;
	ctx->message_in       = 0;
    ctx->prev_data_result = 0;

    ctx->hash_control = 0;     
	ctx->hash_result  = 0;
    
    ctx->bank_result = 0; 
    
    ctx->top_cipher_word = 0;  
	ctx->mid_cipher_word = 0;
	ctx->bot_cipher_word = 0;

    ctx->p_random_clock = 0;

    ctx->com_control = 0;   
    
    return;
} /* system_reset */

/* ------------------------------------------------------------------------- */
/* initial value for nLSFRs from 128 bits crypto key preload                 */
/* ------------------------------------------------------------------------- */
void init_preload (ECRYPT_ctx* ctx)   
{

    system_reset (ctx);

    ctx->top_control = LOAD;
    ctx->top_cipher_word = U8TO32_LITTLE(ctx->session_key + 4);    /* [63:32] */
    top_tier (ctx);
    ctx->top_control = 0;

    ctx->mid_control = LOAD;
    ctx->mid_cipher_word = U8TO32_LITTLE(ctx->session_key + 8);    /* [95:64] */
    mid_tier (ctx);
    ctx->mid_control = 0;

    ctx->bot_control = LOAD;
    ctx->bot_cipher_word = U8TO32_LITTLE(ctx->session_key + 12);    /* [127:96] */
    bot_tier (ctx);
    ctx->bot_control = 0;

    ctx->top_sfr3 = (U8TO32_LITTLE(ctx->session_key) >>  0) & 0x07; /* [ 2: 0] top control LSFR 3bit */
    ctx->top_cnt  = (U8TO32_LITTLE(ctx->session_key) >>  3) & 0x0F; /* [ 6: 3] top control counter 4bit */

    ctx->mid_sfr5 = (U8TO32_LITTLE(ctx->session_key) >>  7) & 0x1F; /* [11: 7] mid control LSFR 5bit */
    ctx->mid_cnt  = (U8TO32_LITTLE(ctx->session_key) >> 12) & 0x0F; /* [15:12] mid control counter 4bit */

    ctx->bot_sfr6 = (U8TO32_LITTLE(ctx->session_key) >> 16) & 0x3F; /* [21:16] bot control LSFR 5bit */
    ctx->bot_cnt  = (U8TO32_LITTLE(ctx->session_key) >> 22) & 0x0F; /* [25:22] bot control counter 4bit */

            /* (crypto_key[0] >> 26) & 0x03;    [27:26] hash mux driver */

    ctx->clock_sfr5 = (U8TO32_LITTLE(ctx->session_key) >> 28) & 0x0F; /* [31:28] (p) random clock LSFR 4bit */

    top_brown (ctx);       /* generate initial browning & slipping */
    top_slip  (ctx);
    mid_brown (ctx);
    mid_slip  (ctx);
    bot_brown (ctx);
    bot_slip  (ctx);

    ctx->top_control |= CLOCK;   /* enable initial clock iteration */
    ctx->top_control |= BROWN;   /* enable (force) browning */
    return;
} /* init_preload */

/* ------------------------------------------------------------------------- */

void internal_reset(ECRYPT_ctx* ctx)
{
	u32 i=0; 

	ctx->top_control = 0;      
	ctx->mid_control = 0;
	ctx->bot_control = 0;
	ctx->com_control = 0;      

	ctx->p_random_clock = 0;   

	ctx->control_cfg_FF = 0;   

	ctx->top_cipher_word = 0;  
	ctx->mid_cipher_word = 0;
	ctx->bot_cipher_word = 0;

	ctx->top_result = 0;       
	ctx->mid_result = 0;
	ctx->bot_result = 0;

	ctx->bank_result = 0;      

	ctx->hash_control = 0;     
	ctx->hash_result  = 0;

	ctx->imm_data   = 0;         
	ctx->imm_result = 0;       

	ctx->data_result      = 0;
	ctx->message_in       = 0;
    ctx->prev_data_result = 0;

	ctx->feedback_word = 0;    

	for (i=0; i<6; i++)
	{
		ctx->shift_reg [i] = 0;		/* nLSFR */
	}

	ctx->mjuggle_hash_toggle = 0; 
	ctx->hash_vector         = 0;

	ctx->top_fbk = 0;
	ctx->mid_fbk = 0;
	ctx->bot_fbk = 0;

	ctx->top_cnt = 0;      
	ctx->mid_cnt = 0;
	ctx->bot_cnt = 0;

	ctx->top_sfr3 = 0;     
	ctx->mid_sfr5 = 0;     
	ctx->bot_sfr6 = 0;     

	ctx->clock_sfr5     = 0;   
	ctx->clock_sfr2     = 0;   
	ctx->clock_p_rand   = 0;
	ctx->clock_t_rand   = 0;
	ctx->clock_odd4     = 0;
	ctx->clock_mjuggle  = 0;

    ctx->type_of_operation = STREAM_CIPHER_OPERATION;

    for (i=0; i<16; i++)
	{
		ctx->key [i]       = 0;
        ctx->iv  [i]       = 0;
        ctx->session_key[i]= 0;
	}
    ctx->SAMPLE_PERIOD   = 0;
    ctx->mode            = 0;
    ctx->IterationNumber = 0;
    return;
  
} /* internal_reset */
