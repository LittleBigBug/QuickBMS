/******************************************************************************/
/* File name: rabbit.c                                                        */
/*----------------------------------------------------------------------------*/
/* Rabbit C source code in ECRYPT format                                      */
/*----------------------------------------------------------------------------*/
/* Copyright (C) Cryptico A/S. All rights reserved.                           */
/*                                                                            */
/* YOU SHOULD CAREFULLY READ THIS LEGAL NOTICE BEFORE USING THIS SOFTWARE.    */
/*                                                                            */
/* This software is developed by Cryptico A/S and/or its suppliers.           */
/* All title and intellectual property rights in and to the software,         */
/* including but not limited to patent rights and copyrights, are owned by    */
/* Cryptico A/S and/or its suppliers.                                         */
/*                                                                            */
/* The software may be used solely for non-commercial purposes                */
/* without the prior written consent of Cryptico A/S. For further             */
/* information on licensing terms and conditions please contact Cryptico A/S  */
/* at info@cryptico.com                                                       */
/*                                                                            */
/* Cryptico, CryptiCore, the Cryptico logo and "Re-thinking encryption" are   */
/* either trademarks or registered trademarks of Cryptico A/S.                */
/*                                                                            */
/* Cryptico A/S shall not in any way be liable for any use of this software.  */
/* The software is provided "as is" without any express or implied warranty.  */
/*                                                                            */
/******************************************************************************/

#include "ecrypt-sync.h"
#include "ecrypt-portable.h"

/* -------------------------------------------------------------------------- */

/* Square a 32-bit unsigned integer to obtain the 64-bit result and return */
/* the upper 32 bits XOR the lower 32 bits */
static u32 RABBIT_g_func(u32 x)
{
   /* Temporary variables */
   u32 a, b, h, l;

   /* Construct high and low argument for squaring */
   a = x&0xFFFF;
   b = x>>16;

   /* Calculate high and low result of squaring */
   h = (((U32V(a*a)>>17) + U32V(a*b))>>15) + b*b;
   l = x*x;

   /* Return high XOR low */
   return U32V(h^l);
}

/* -------------------------------------------------------------------------- */

/* Calculate the next internal state */
static void RABBIT_next_state(RABBIT_ctx *p_instance)
{
   /* Temporary variables */
   u32 g[8], c_old[8], i;

   /* Save old counter values */
   for (i=0; i<8; i++)
      c_old[i] = p_instance->c[i];

   /* Calculate new counter values */
   p_instance->c[0] = U32V(p_instance->c[0] + 0x4D34D34D + p_instance->carry);
   p_instance->c[1] = U32V(p_instance->c[1] + 0xD34D34D3 + (p_instance->c[0] < c_old[0]));
   p_instance->c[2] = U32V(p_instance->c[2] + 0x34D34D34 + (p_instance->c[1] < c_old[1]));
   p_instance->c[3] = U32V(p_instance->c[3] + 0x4D34D34D + (p_instance->c[2] < c_old[2]));
   p_instance->c[4] = U32V(p_instance->c[4] + 0xD34D34D3 + (p_instance->c[3] < c_old[3]));
   p_instance->c[5] = U32V(p_instance->c[5] + 0x34D34D34 + (p_instance->c[4] < c_old[4]));
   p_instance->c[6] = U32V(p_instance->c[6] + 0x4D34D34D + (p_instance->c[5] < c_old[5]));
   p_instance->c[7] = U32V(p_instance->c[7] + 0xD34D34D3 + (p_instance->c[6] < c_old[6]));
   p_instance->carry = (p_instance->c[7] < c_old[7]);
   
   /* Calculate the g-values */
   for (i=0;i<8;i++)
      g[i] = RABBIT_g_func(U32V(p_instance->x[i] + p_instance->c[i]));

   /* Calculate new state values */
   p_instance->x[0] = U32V(g[0] + ROTL32(g[7],16) + ROTL32(g[6], 16));
   p_instance->x[1] = U32V(g[1] + ROTL32(g[0], 8) + g[7]);
   p_instance->x[2] = U32V(g[2] + ROTL32(g[1],16) + ROTL32(g[0], 16));
   p_instance->x[3] = U32V(g[3] + ROTL32(g[2], 8) + g[1]);
   p_instance->x[4] = U32V(g[4] + ROTL32(g[3],16) + ROTL32(g[2], 16));
   p_instance->x[5] = U32V(g[5] + ROTL32(g[4], 8) + g[3]);
   p_instance->x[6] = U32V(g[6] + ROTL32(g[5],16) + ROTL32(g[4], 16));
   p_instance->x[7] = U32V(g[7] + ROTL32(g[6], 8) + g[5]);
}

/* ------------------------------------------------------------------------- */

/* No initialization is needed for Rabbit */
void ECRYPT_init(void)
{
   return;
}

/* ------------------------------------------------------------------------- */

/* Key setup */
void ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
   /* Temporary variables */
   u32 k0, k1, k2, k3, i;

   /* Generate four subkeys */
   k0 = U8TO32_LITTLE(key+ 0);
   k1 = U8TO32_LITTLE(key+ 4);
   k2 = U8TO32_LITTLE(key+ 8);
   k3 = U8TO32_LITTLE(key+12);

   /* Generate initial state variables */
   ctx->master_ctx.x[0] = k0;
   ctx->master_ctx.x[2] = k1;
   ctx->master_ctx.x[4] = k2;
   ctx->master_ctx.x[6] = k3;
   ctx->master_ctx.x[1] = U32V(k3<<16) | (k2>>16);
   ctx->master_ctx.x[3] = U32V(k0<<16) | (k3>>16);
   ctx->master_ctx.x[5] = U32V(k1<<16) | (k0>>16);
   ctx->master_ctx.x[7] = U32V(k2<<16) | (k1>>16);

   /* Generate initial counter values */
   ctx->master_ctx.c[0] = ROTL32(k2, 16);
   ctx->master_ctx.c[2] = ROTL32(k3, 16);
   ctx->master_ctx.c[4] = ROTL32(k0, 16);
   ctx->master_ctx.c[6] = ROTL32(k1, 16);
   ctx->master_ctx.c[1] = (k0&0xFFFF0000) | (k1&0xFFFF);
   ctx->master_ctx.c[3] = (k1&0xFFFF0000) | (k2&0xFFFF);
   ctx->master_ctx.c[5] = (k2&0xFFFF0000) | (k3&0xFFFF);
   ctx->master_ctx.c[7] = (k3&0xFFFF0000) | (k0&0xFFFF);

   /* Clear carry bit */
   ctx->master_ctx.carry = 0;

   /* Iterate the system four times */
   for (i=0; i<4; i++)
      RABBIT_next_state(&(ctx->master_ctx));

   /* Modify the counters */
   for (i=0; i<8; i++)
      ctx->master_ctx.c[i] ^= ctx->master_ctx.x[(i+4)&0x7];

   /* Copy master instance to work instance */
   for (i=0; i<8; i++)
   {
      ctx->work_ctx.x[i] = ctx->master_ctx.x[i];
      ctx->work_ctx.c[i] = ctx->master_ctx.c[i];
   }
   ctx->work_ctx.carry = ctx->master_ctx.carry;
}

/* ------------------------------------------------------------------------- */

/* IV setup */
void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
   /* Temporary variables */
   u32 i0, i1, i2, i3, i;
      
   /* Generate four subvectors */
   i0 = U8TO32_LITTLE(iv+0);
   i2 = U8TO32_LITTLE(iv+4);
   i1 = (i0>>16) | (i2&0xFFFF0000);
   i3 = (i2<<16) | (i0&0x0000FFFF);

   /* Modify counter values */
   ctx->work_ctx.c[0] = ctx->master_ctx.c[0] ^ i0;
   ctx->work_ctx.c[1] = ctx->master_ctx.c[1] ^ i1;
   ctx->work_ctx.c[2] = ctx->master_ctx.c[2] ^ i2;
   ctx->work_ctx.c[3] = ctx->master_ctx.c[3] ^ i3;
   ctx->work_ctx.c[4] = ctx->master_ctx.c[4] ^ i0;
   ctx->work_ctx.c[5] = ctx->master_ctx.c[5] ^ i1;
   ctx->work_ctx.c[6] = ctx->master_ctx.c[6] ^ i2;
   ctx->work_ctx.c[7] = ctx->master_ctx.c[7] ^ i3;

   /* Copy state variables */
   for (i=0; i<8; i++)
      ctx->work_ctx.x[i] = ctx->master_ctx.x[i];
   ctx->work_ctx.carry = ctx->master_ctx.carry;

   /* Iterate the system four times */
   for (i=0; i<4; i++)
      RABBIT_next_state(&(ctx->work_ctx));
}

/* ------------------------------------------------------------------------- */

/* Encrypt/decrypt a message of any size */
void ECRYPT_process_bytes(int action, ECRYPT_ctx* ctx, const u8* input, 
          u8* output, u32 msglen)
{
   /* Temporary variables */
   u32 i;
   u8 buffer[16];

   /* Encrypt/decrypt all full blocks */
   while (msglen >= 16)
   {
      /* Iterate the system */
      RABBIT_next_state(&(ctx->work_ctx));

      /* Encrypt/decrypt 16 bytes of data */
      *(u32*)(output+ 0) = *(u32*)(input+ 0) ^ U32TO32_LITTLE(ctx->work_ctx.x[0] ^
                (ctx->work_ctx.x[5]>>16) ^ U32V(ctx->work_ctx.x[3]<<16));
      *(u32*)(output+ 4) = *(u32*)(input+ 4) ^ U32TO32_LITTLE(ctx->work_ctx.x[2] ^ 
                (ctx->work_ctx.x[7]>>16) ^ U32V(ctx->work_ctx.x[5]<<16));
      *(u32*)(output+ 8) = *(u32*)(input+ 8) ^ U32TO32_LITTLE(ctx->work_ctx.x[4] ^ 
                (ctx->work_ctx.x[1]>>16) ^ U32V(ctx->work_ctx.x[7]<<16));
      *(u32*)(output+12) = *(u32*)(input+12) ^ U32TO32_LITTLE(ctx->work_ctx.x[6] ^ 
                (ctx->work_ctx.x[3]>>16) ^ U32V(ctx->work_ctx.x[1]<<16));

      /* Increment pointers and decrement length */
      input += 16;
      output += 16;
      msglen -= 16;
   }

   /* Encrypt/decrypt remaining data */
   if (msglen)
   {
      /* Iterate the system */
      RABBIT_next_state(&(ctx->work_ctx));

      /* Generate 16 bytes of pseudo-random data */
      *(u32*)(buffer+ 0) = U32TO32_LITTLE(ctx->work_ctx.x[0] ^
                (ctx->work_ctx.x[5]>>16) ^ U32V(ctx->work_ctx.x[3]<<16));
      *(u32*)(buffer+ 4) = U32TO32_LITTLE(ctx->work_ctx.x[2] ^ 
                (ctx->work_ctx.x[7]>>16) ^ U32V(ctx->work_ctx.x[5]<<16));
      *(u32*)(buffer+ 8) = U32TO32_LITTLE(ctx->work_ctx.x[4] ^ 
                (ctx->work_ctx.x[1]>>16) ^ U32V(ctx->work_ctx.x[7]<<16));
      *(u32*)(buffer+12) = U32TO32_LITTLE(ctx->work_ctx.x[6] ^ 
                (ctx->work_ctx.x[3]>>16) ^ U32V(ctx->work_ctx.x[1]<<16));

      /* Encrypt/decrypt the data */
      for (i=0; i<msglen; i++)
         output[i] = input[i] ^ buffer[i];
   }
}

/* ------------------------------------------------------------------------- */

/* Generate keystream */
void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{
   /* Temporary variables */
   u32 i;
   u8 buffer[16];

   /* Generate all full blocks */
   while (length >= 16)
   {
      /* Iterate the system */
      RABBIT_next_state(&(ctx->work_ctx));

      /* Generate 16 bytes of pseudo-random data */
      *(u32*)(keystream+ 0) = U32TO32_LITTLE(ctx->work_ctx.x[0] ^
                (ctx->work_ctx.x[5]>>16) ^ U32V(ctx->work_ctx.x[3]<<16));
      *(u32*)(keystream+ 4) = U32TO32_LITTLE(ctx->work_ctx.x[2] ^ 
                (ctx->work_ctx.x[7]>>16) ^ U32V(ctx->work_ctx.x[5]<<16));
      *(u32*)(keystream+ 8) = U32TO32_LITTLE(ctx->work_ctx.x[4] ^ 
                (ctx->work_ctx.x[1]>>16) ^ U32V(ctx->work_ctx.x[7]<<16));
      *(u32*)(keystream+12) = U32TO32_LITTLE(ctx->work_ctx.x[6] ^ 
                (ctx->work_ctx.x[3]>>16) ^ U32V(ctx->work_ctx.x[1]<<16));

      /* Increment pointers and decrement length */
      keystream += 16;
      length -= 16;
   }

   /* Generate remaining pseudo-random data */
   if (length)
   {
      /* Iterate the system */
      RABBIT_next_state(&(ctx->work_ctx));

      /* Generate 16 bytes of pseudo-random data */
      *(u32*)(buffer+ 0) = U32TO32_LITTLE(ctx->work_ctx.x[0] ^
                (ctx->work_ctx.x[5]>>16) ^ U32V(ctx->work_ctx.x[3]<<16));
      *(u32*)(buffer+ 4) = U32TO32_LITTLE(ctx->work_ctx.x[2] ^ 
                (ctx->work_ctx.x[7]>>16) ^ U32V(ctx->work_ctx.x[5]<<16));
      *(u32*)(buffer+ 8) = U32TO32_LITTLE(ctx->work_ctx.x[4] ^ 
                (ctx->work_ctx.x[1]>>16) ^ U32V(ctx->work_ctx.x[7]<<16));
      *(u32*)(buffer+12) = U32TO32_LITTLE(ctx->work_ctx.x[6] ^ 
                (ctx->work_ctx.x[3]>>16) ^ U32V(ctx->work_ctx.x[1]<<16));

      /* Copy remaining data */
      for (i=0; i<length; i++)
         keystream[i] = buffer[i];
   }
}

/* ------------------------------------------------------------------------- */

/* Encrypt/decrypt a number of full blocks */
void ECRYPT_process_blocks(int action, ECRYPT_ctx* ctx, const u8* input, 
          u8* output, u32 blocks)
{
   /* Temporary variables */
   u32 i;

   for (i=0; i<blocks; i++)
   {
      /* Iterate the system */
      RABBIT_next_state(&(ctx->work_ctx));

      /* Encrypt/decrypt 16 bytes of data */
      *(u32*)(output+ 0) = *(u32*)(input+ 0) ^ U32TO32_LITTLE(ctx->work_ctx.x[0] ^
                (ctx->work_ctx.x[5]>>16) ^ U32V(ctx->work_ctx.x[3]<<16));
      *(u32*)(output+ 4) = *(u32*)(input+ 4) ^ U32TO32_LITTLE(ctx->work_ctx.x[2] ^ 
                (ctx->work_ctx.x[7]>>16) ^ U32V(ctx->work_ctx.x[5]<<16));
      *(u32*)(output+ 8) = *(u32*)(input+ 8) ^ U32TO32_LITTLE(ctx->work_ctx.x[4] ^ 
                (ctx->work_ctx.x[1]>>16) ^ U32V(ctx->work_ctx.x[7]<<16));
      *(u32*)(output+12) = *(u32*)(input+12) ^ U32TO32_LITTLE(ctx->work_ctx.x[6] ^ 
                (ctx->work_ctx.x[3]>>16) ^ U32V(ctx->work_ctx.x[1]<<16));

      /* Increment pointers to input and output data */
      input += 16;
      output += 16;
   }
}

/* ------------------------------------------------------------------------- */
