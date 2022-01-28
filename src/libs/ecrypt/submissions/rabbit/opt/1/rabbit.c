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
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* 32-bit G function macros */

#define RABBIT_G32_VARS \
      u32 x_plus_c, a, b, h, l;

#define RABBIT_G32(i) \
      x_plus_c = U32V(p_instance->x##i+p_instance->c##i); \
      a = x_plus_c&0xFFFF; \
      b = x_plus_c>>16; \
      h = (((U32V(a*a)>>17) + U32V(a*b))>>15) + b*b; \
      l = x_plus_c*x_plus_c; \
      g##i = h^l;

/* -------------------------------------------------------------------------- */
/* 64-bit G function macros */

#define RABBIT_G64_VARS \
      u64 prod; u32 x_plus_c;

#define RABBIT_G64(i) \
      x_plus_c = U32V(p_instance->x##i + p_instance->c##i); \
      prod = (u64)(x_plus_c) * (u64)(x_plus_c); \
      g##i = U32V((u32)prod ^ (u32)(prod>>32));


/* -------------------------------------------------------------------------- */
/* 32-bit counter update macros */

#define RABBIT_COUNTER32_VARS \
      u32 carry, c_old;

#define RABBIT_COUNTER32_PRE \
      carry = p_instance->carry;

#define RABBIT_COUNTER32(i, a) \
      c_old = p_instance->c##i; \
      p_instance->c##i = U32V(p_instance->c##i + carry); \
      carry = U32V((p_instance->c##i < c_old) + a);

#define RABBIT_COUNTER32_POST \
      p_instance->carry = carry;

#define RABBIT_CARRY_INIT 0x4D34D34D

/* -------------------------------------------------------------------------- */
/* Defaunt x[i] generation macros */

#define RABBIT_GEN_X_EVEN(i, j, k) \
      p_instance->x##i = U32V(g##i + ROTL32(g##j,16) + ROTL32(g##k, 16));

#define RABBIT_GEN_X_ODD(i, j, k) \
      p_instance->x##i = U32V(g##i + ROTL32(g##j, 8) + g##k);

/* -------------------------------------------------------------------------- */
/* Macros used in next-state function */

#if defined(_MSC_VER) && defined(_M_IX86) && (ECRYPT_VARIANT > 2)
   /* COMPILER : MICROSOFT OR INTEL */
   /* PROCESSOR: x86 */

   #define RABBIT_NS_VARS \
      u32 g0, g1, g2, g3, g4, g5, g6, g7;

   #define RABBIT_NS_PRE \
      _asm mov esi, p_instance \
      _asm mov ecx, [esi]RABBIT_ctx.carry

   #define RABBIT_G_AND_COUNTER(i, a) \
      _asm mov eax, [esi]RABBIT_ctx.c##i \
      _asm add eax, ecx \
      _asm mov ecx, 0 \
      _asm adc ecx, a \
      _asm mov [esi]RABBIT_ctx.c##i, eax \
      _asm add eax, [esi]RABBIT_ctx.x##i \
      _asm mul eax \
      _asm xor edx, eax \
      _asm mov g##i, edx

   #undef RABBIT_GEN_X_EVEN
   #undef RABBIT_GEN_X_ODD

   #define RABBIT_GEN_X_EVEN(i, j, k) \
      _asm mov ebx, g##k \
      _asm rol ebx, 16 \
      _asm mov edi, g##j \
      _asm rol edi, 16 \
      _asm add ebx, g##i \
      _asm add ebx, edi \
      _asm mov [esi]RABBIT_ctx.x##i, ebx

   #define RABBIT_GEN_X_ODD(i, j, k) \
      _asm mov ebx, g##k \
      _asm mov edi, g##j \
      _asm rol edi, 8 \
      _asm add ebx, g##i \
      _asm add ebx, edi \
      _asm mov [esi]RABBIT_ctx.x##i, ebx

   #define RABBIT_NS_POST \
      _asm mov [esi]RABBIT_ctx.carry, ecx

#elif defined(__GNUC__) && defined(__i386__) && (ECRYPT_VARIANT > 2)
   /* COMPILER : GCC */
   /* PROCESSOR: x86 */

   #if (ECRYPT_VARIANT > 3)
      #define RABBIT_NS_VARS \
         u32 g0, g1, g2, g3, g4, g5, g6, g7, c_a, asm_dummy1;
   #else
      #define RABBIT_NS_VARS \
         u32 g0, g1, g2, g3, g4, g5, g6, g7, c_a;
   #endif

   #define RABBIT_NS_PRE \
      c_a = p_instance->carry;

   #define RABBIT_G_AND_COUNTER(i, a) \
      __asm ( \
      "movl  %3, %%eax  # Load c[i]\n\t" \
      "addl  %5, %%eax  # c[i] += c_a\n\t" \
      "movl  $0, %1     # c_a = 0\n\t" \
      "adcl  %6, %1     # c_a += a[i+1] + carry\n\t" \
      "movl  %%eax, %2  # Save c[i]\n\t" \
      "addl  %4, %%eax  # c[i] += x[i]\n\t" \
      "mull  %%eax      # Square result\n\t" \
      "xorl  %%eax, %0  # Xor result to achieve new x[i]\n\t" \
      : "=d" (g##i), "=r" (c_a), "=m" (p_instance->c##i) \
      : "m" (p_instance->c##i), "m" (p_instance->x##i), "1" (c_a), "i" (a) \
      : "cc", "%eax");

   #if (ECRYPT_VARIANT > 3)

      #undef RABBIT_GEN_X_EVEN
      #undef RABBIT_GEN_X_ODD

      #define RABBIT_GEN_X_EVEN(i, j, k) \
      __asm ( \
         "roll  $16, %0    # g[j]<<<16\n\t" \
         "roll  $16, %4    # g[k]<<<16\n\t" \
         "addl  %2, %0     # g[i] + g[j]<<<16\n\t" \
         "addl  %4, %0     # x[i] = g[i] + g[j]<<<16 + g[k]<<<16\n\t" \
         : "=r" (p_instance->x##i), "=r" (asm_dummy1) \
         : "g" (g##i), "0" (g##j), "1" (g##k) \
         : "cc" );

      #define RABBIT_GEN_X_ODD(i, j, k) \
         __asm ( \
         "roll  $8, %0     # g[j]<<<8\n\t" \
         "addl  %1, %0     # g[i] + g[j]<<<8\n\t" \
         "addl  %3, %0     # x[i] = g[i] + g[j]<<<8 + g[k]\n\t" \
         : "=r" (p_instance->x##i) \
         : "g" (g##i), "0" (g##j), "g" (g##k) \
         : "cc" );
   #endif

   #define RABBIT_NS_POST \
      p_instance->carry = c_a;

#elif (ECRYPT_VARIANT > 1) && defined(I64T)
   /* Pure C with 64-bit G function */

   #define RABBIT_NS_VARS \
      RABBIT_COUNTER32_VARS \
      RABBIT_G64_VARS \
      u32 g0, g1, g2, g3, g4, g5, g6, g7;

   #define RABBIT_NS_PRE \
      RABBIT_COUNTER32_PRE

   #define RABBIT_G_AND_COUNTER(i, a) \
      RABBIT_COUNTER32(i, a) \
      RABBIT_G64(i)

   #define RABBIT_NS_POST \
      RABBIT_COUNTER32_POST

#else
   /* Pure 32-bit C */

   #define RABBIT_NS_VARS \
      RABBIT_COUNTER32_VARS \
      RABBIT_G32_VARS \
      u32 g0, g1, g2, g3, g4, g5, g6, g7;

   #define RABBIT_NS_PRE \
      RABBIT_COUNTER32_PRE

   #define RABBIT_G_AND_COUNTER(i, a) \
      RABBIT_COUNTER32(i, a) \
      RABBIT_G32(i)

   #define RABBIT_NS_POST \
      RABBIT_COUNTER32_POST
#endif

/* ------------------------------------------------------------------------- */

/* Calculate the next internal state */
static void RABBIT_next_state(RABBIT_ctx *p_instance)
{
   /* Temporary variables */
   RABBIT_NS_VARS

   RABBIT_NS_PRE

   /* Calculate new counter values */
   /* Calculate new g values */
   /* Calculate new x values */
   RABBIT_G_AND_COUNTER(0, 0xD34D34D3)
   RABBIT_G_AND_COUNTER(1, 0x34D34D34)
   RABBIT_G_AND_COUNTER(2, 0x4D34D34D)
   RABBIT_GEN_X_EVEN(2, 1, 0)
   RABBIT_G_AND_COUNTER(3, 0xD34D34D3)
   RABBIT_GEN_X_ODD(3, 2, 1)
   RABBIT_G_AND_COUNTER(4, 0x34D34D34)
   RABBIT_GEN_X_EVEN(4, 3, 2)
   RABBIT_G_AND_COUNTER(5, 0x4D34D34D)
   RABBIT_GEN_X_ODD(5, 4, 3)
   RABBIT_G_AND_COUNTER(6, 0xD34D34D3)
   RABBIT_GEN_X_EVEN(6, 5, 4)
   RABBIT_G_AND_COUNTER(7, 0x4D34D34D)
   RABBIT_GEN_X_ODD(7, 6, 5)
   RABBIT_GEN_X_EVEN(0, 7, 6)
   RABBIT_GEN_X_ODD(1, 0, 7)

   RABBIT_NS_POST
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
   RABBIT_NS_VARS
   RABBIT_ctx *p_instance;

   /* Generate four subkeys */
   k0 = U8TO32_LITTLE(key+ 0);
   k1 = U8TO32_LITTLE(key+ 4);
   k2 = U8TO32_LITTLE(key+ 8);
   k3 = U8TO32_LITTLE(key+12);

   /* Generate initial state variables */
   ctx->master_ctx.x0 = k0;
   ctx->master_ctx.x2 = k1;
   ctx->master_ctx.x4 = k2;
   ctx->master_ctx.x6 = k3;
   ctx->master_ctx.x1 = U32V(k3<<16) | (k2>>16);
   ctx->master_ctx.x3 = U32V(k0<<16) | (k3>>16);
   ctx->master_ctx.x5 = U32V(k1<<16) | (k0>>16);
   ctx->master_ctx.x7 = U32V(k2<<16) | (k1>>16);

   /* Generate initial counter values */
   ctx->master_ctx.c0 = ROTL32(k2, 16);
   ctx->master_ctx.c2 = ROTL32(k3, 16);
   ctx->master_ctx.c4 = ROTL32(k0, 16);
   ctx->master_ctx.c6 = ROTL32(k1, 16);
   ctx->master_ctx.c1 = (k0&0xFFFF0000) | (k1&0xFFFF);
   ctx->master_ctx.c3 = (k1&0xFFFF0000) | (k2&0xFFFF);
   ctx->master_ctx.c5 = (k2&0xFFFF0000) | (k3&0xFFFF);
   ctx->master_ctx.c7 = (k3&0xFFFF0000) | (k0&0xFFFF);

   /* Clear carry bit */
   ctx->master_ctx.carry = RABBIT_CARRY_INIT;

   /* Iterate the system four times */
   p_instance = &(ctx->master_ctx);
   for (i=0; i<4; i++)
   {
      /* Calculate new counter values */
      /* Calculate new g values */
      /* Calculate new x values */
      RABBIT_NS_PRE
      RABBIT_G_AND_COUNTER(0, 0xD34D34D3)
      RABBIT_G_AND_COUNTER(1, 0x34D34D34)
      RABBIT_G_AND_COUNTER(2, 0x4D34D34D)
      RABBIT_GEN_X_EVEN(2, 1, 0)
      RABBIT_G_AND_COUNTER(3, 0xD34D34D3)
      RABBIT_GEN_X_ODD(3, 2, 1)
      RABBIT_G_AND_COUNTER(4, 0x34D34D34)
      RABBIT_GEN_X_EVEN(4, 3, 2)
      RABBIT_G_AND_COUNTER(5, 0x4D34D34D)
      RABBIT_GEN_X_ODD(5, 4, 3)
      RABBIT_G_AND_COUNTER(6, 0xD34D34D3)
      RABBIT_GEN_X_EVEN(6, 5, 4)
      RABBIT_G_AND_COUNTER(7, 0x4D34D34D)
      RABBIT_GEN_X_ODD(7, 6, 5)
      RABBIT_GEN_X_EVEN(0, 7, 6)
      RABBIT_GEN_X_ODD(1, 0, 7)
      RABBIT_NS_POST
   }

   /* Modify the counters */
   /* Copy master instance to work instance */
   ctx->work_ctx.c0 = ctx->master_ctx.c0 ^= ctx->work_ctx.x4 = ctx->master_ctx.x4;
   ctx->work_ctx.c1 = ctx->master_ctx.c1 ^= ctx->work_ctx.x5 = ctx->master_ctx.x5;
   ctx->work_ctx.c2 = ctx->master_ctx.c2 ^= ctx->work_ctx.x6 = ctx->master_ctx.x6;
   ctx->work_ctx.c3 = ctx->master_ctx.c3 ^= ctx->work_ctx.x7 = ctx->master_ctx.x7;
   ctx->work_ctx.c4 = ctx->master_ctx.c4 ^= ctx->work_ctx.x0 = ctx->master_ctx.x0;
   ctx->work_ctx.c5 = ctx->master_ctx.c5 ^= ctx->work_ctx.x1 = ctx->master_ctx.x1;
   ctx->work_ctx.c6 = ctx->master_ctx.c6 ^= ctx->work_ctx.x2 = ctx->master_ctx.x2;
   ctx->work_ctx.c7 = ctx->master_ctx.c7 ^= ctx->work_ctx.x3 = ctx->master_ctx.x3;

   ctx->work_ctx.carry = ctx->master_ctx.carry;
}

/* ------------------------------------------------------------------------- */

/* IV setup */
void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
   /* Temporary variables */
   u32 i0, i1, i2, i3, i;
   RABBIT_NS_VARS
   RABBIT_ctx *p_instance;

   /* Generate four subvectors */
   i0 = U8TO32_LITTLE(iv+0);
   i2 = U8TO32_LITTLE(iv+4);
   i1 = (i0>>16) | (i2&0xFFFF0000);
   i3 = (i2<<16) | (i0&0x0000FFFF);

   /* Modify counter values */
   ctx->work_ctx.c0 = ctx->master_ctx.c0 ^ i0;
   ctx->work_ctx.c1 = ctx->master_ctx.c1 ^ i1;
   ctx->work_ctx.c2 = ctx->master_ctx.c2 ^ i2;
   ctx->work_ctx.c3 = ctx->master_ctx.c3 ^ i3;
   ctx->work_ctx.c4 = ctx->master_ctx.c4 ^ i0;
   ctx->work_ctx.c5 = ctx->master_ctx.c5 ^ i1;
   ctx->work_ctx.c6 = ctx->master_ctx.c6 ^ i2;
   ctx->work_ctx.c7 = ctx->master_ctx.c7 ^ i3;

   /* Copy state variables */
   ctx->work_ctx.x0 = ctx->master_ctx.x0;
   ctx->work_ctx.x1 = ctx->master_ctx.x1;
   ctx->work_ctx.x2 = ctx->master_ctx.x2;
   ctx->work_ctx.x3 = ctx->master_ctx.x3;
   ctx->work_ctx.x4 = ctx->master_ctx.x4;
   ctx->work_ctx.x5 = ctx->master_ctx.x5;
   ctx->work_ctx.x6 = ctx->master_ctx.x6;
   ctx->work_ctx.x7 = ctx->master_ctx.x7;

   ctx->work_ctx.carry = ctx->master_ctx.carry;

   /* Iterate the system four times */
   p_instance = &(ctx->work_ctx);
   for (i=0; i<4; i++)
   {
      /* Calculate new counter values */
      /* Calculate new g values */
      /* Calculate new x values */
      RABBIT_NS_PRE
      RABBIT_G_AND_COUNTER(0, 0xD34D34D3)
      RABBIT_G_AND_COUNTER(1, 0x34D34D34)
      RABBIT_G_AND_COUNTER(2, 0x4D34D34D)
      RABBIT_GEN_X_EVEN(2, 1, 0)
      RABBIT_G_AND_COUNTER(3, 0xD34D34D3)
      RABBIT_GEN_X_ODD(3, 2, 1)
      RABBIT_G_AND_COUNTER(4, 0x34D34D34)
      RABBIT_GEN_X_EVEN(4, 3, 2)
      RABBIT_G_AND_COUNTER(5, 0x4D34D34D)
      RABBIT_GEN_X_ODD(5, 4, 3)
      RABBIT_G_AND_COUNTER(6, 0xD34D34D3)
      RABBIT_GEN_X_EVEN(6, 5, 4)
      RABBIT_G_AND_COUNTER(7, 0x4D34D34D)
      RABBIT_GEN_X_ODD(7, 6, 5)
      RABBIT_GEN_X_EVEN(0, 7, 6)
      RABBIT_GEN_X_ODD(1, 0, 7)
      RABBIT_NS_POST
   }
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
      *(u32*)(output+ 0) = *(u32*)(input+ 0) ^ U32TO32_LITTLE(ctx->work_ctx.x0 ^
                (ctx->work_ctx.x5>>16) ^ U32V(ctx->work_ctx.x3<<16));
      *(u32*)(output+ 4) = *(u32*)(input+ 4) ^ U32TO32_LITTLE(ctx->work_ctx.x2 ^ 
                (ctx->work_ctx.x7>>16) ^ U32V(ctx->work_ctx.x5<<16));
      *(u32*)(output+ 8) = *(u32*)(input+ 8) ^ U32TO32_LITTLE(ctx->work_ctx.x4 ^ 
                (ctx->work_ctx.x1>>16) ^ U32V(ctx->work_ctx.x7<<16));
      *(u32*)(output+12) = *(u32*)(input+12) ^ U32TO32_LITTLE(ctx->work_ctx.x6 ^ 
                (ctx->work_ctx.x3>>16) ^ U32V(ctx->work_ctx.x1<<16));

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
      *(u32*)(buffer+ 0) = U32TO32_LITTLE(ctx->work_ctx.x0 ^
                (ctx->work_ctx.x5>>16) ^ U32V(ctx->work_ctx.x3<<16));
      *(u32*)(buffer+ 4) = U32TO32_LITTLE(ctx->work_ctx.x2 ^ 
                (ctx->work_ctx.x7>>16) ^ U32V(ctx->work_ctx.x5<<16));
      *(u32*)(buffer+ 8) = U32TO32_LITTLE(ctx->work_ctx.x4 ^ 
                (ctx->work_ctx.x1>>16) ^ U32V(ctx->work_ctx.x7<<16));
      *(u32*)(buffer+12) = U32TO32_LITTLE(ctx->work_ctx.x6 ^ 
                (ctx->work_ctx.x3>>16) ^ U32V(ctx->work_ctx.x1<<16));

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
      *(u32*)(keystream+ 0) = U32TO32_LITTLE(ctx->work_ctx.x0 ^
                (ctx->work_ctx.x5>>16) ^ U32V(ctx->work_ctx.x3<<16));
      *(u32*)(keystream+ 4) = U32TO32_LITTLE(ctx->work_ctx.x2 ^ 
                (ctx->work_ctx.x7>>16) ^ U32V(ctx->work_ctx.x5<<16));
      *(u32*)(keystream+ 8) = U32TO32_LITTLE(ctx->work_ctx.x4 ^ 
                (ctx->work_ctx.x1>>16) ^ U32V(ctx->work_ctx.x7<<16));
      *(u32*)(keystream+12) = U32TO32_LITTLE(ctx->work_ctx.x6 ^ 
                (ctx->work_ctx.x3>>16) ^ U32V(ctx->work_ctx.x1<<16));

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
      *(u32*)(buffer+ 0) = U32TO32_LITTLE(ctx->work_ctx.x0 ^
                (ctx->work_ctx.x5>>16) ^ U32V(ctx->work_ctx.x3<<16));
      *(u32*)(buffer+ 4) = U32TO32_LITTLE(ctx->work_ctx.x2 ^ 
                (ctx->work_ctx.x7>>16) ^ U32V(ctx->work_ctx.x5<<16));
      *(u32*)(buffer+ 8) = U32TO32_LITTLE(ctx->work_ctx.x4 ^ 
                (ctx->work_ctx.x1>>16) ^ U32V(ctx->work_ctx.x7<<16));
      *(u32*)(buffer+12) = U32TO32_LITTLE(ctx->work_ctx.x6 ^ 
                (ctx->work_ctx.x3>>16) ^ U32V(ctx->work_ctx.x1<<16));

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
      *(u32*)(output+ 0) = *(u32*)(input+ 0) ^ U32TO32_LITTLE(ctx->work_ctx.x0 ^
                (ctx->work_ctx.x5>>16) ^ U32V(ctx->work_ctx.x3<<16));
      *(u32*)(output+ 4) = *(u32*)(input+ 4) ^ U32TO32_LITTLE(ctx->work_ctx.x2 ^ 
                (ctx->work_ctx.x7>>16) ^ U32V(ctx->work_ctx.x5<<16));
      *(u32*)(output+ 8) = *(u32*)(input+ 8) ^ U32TO32_LITTLE(ctx->work_ctx.x4 ^ 
                (ctx->work_ctx.x1>>16) ^ U32V(ctx->work_ctx.x7<<16));
      *(u32*)(output+12) = *(u32*)(input+12) ^ U32TO32_LITTLE(ctx->work_ctx.x6 ^ 
                (ctx->work_ctx.x3>>16) ^ U32V(ctx->work_ctx.x1<<16));

      /* Increment pointers to input and output data */
      input += 16;
      output += 16;
   }
}

/* ------------------------------------------------------------------------- */
