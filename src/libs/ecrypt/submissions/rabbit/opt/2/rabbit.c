/******************************************************************************/
/* File name: rabbit.c                                                        */
/*----------------------------------------------------------------------------*/
/* Rabbit C optimized source code in ECRYPT format                            */
/*----------------------------------------------------------------------------*/
/* Copyright (C) Cryptico ApS. All rights reserved.                           */
/*                                                                            */
/* This software is developed by Cryptico ApS and/or its suppliers.           */
/* All title and intellectual property rights in and to the software,         */
/* including but not limited to patent rights and copyrights, are owned by    */
/* Cryptico ApS and/or its suppliers.                                         */
/*                                                                            */
/* The software may be used solely for non-commercial purposes                */
/* without the prior written consent of Cryptico ApS. For further             */
/* information on licensing terms and conditions please contact Cryptico ApS  */
/* at info@cryptico.com                                                       */
/*                                                                            */
/* Cryptico, CryptiCore, the Cryptico logo and "Re-thinking encryption" are   */
/* either trademarks or registered trademarks of Cryptico ApS.                */
/*                                                                            */
/* Cryptico ApS shall not in any way be liable for any use of this software.  */
/* The software is provided "as is" without any express or implied warranty.  */
/******************************************************************************/

#include "ecrypt-sync.h"
#include "ecrypt-portable.h"

#if defined(__GNUC__) && defined(__i386__)
#define RABBIT_GCC 1
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#define RABBIT_MSC 1
#endif

#if (ECRYPT_VARIANT > 4) && !(defined(RABBIT_GCC) || defined(RABBIT_MSC))
#error this variant does not compile on this platform
#endif

/* -------------------------------------------------------------------------- */
/* 32-bit G function macros */
/* Used in ECRYPT_VARIANT 1 */

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
/* Used in ECRYPT_VARIANT 2 */

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

#if defined(RABBIT_MSC) && (ECRYPT_VARIANT > 2)
   /* COMPILER : MICROSOFT OR INTEL */
   /* PROCESSOR: x86 */

   #define RABBIT_NS_VARS \
      u32 g0, g1, g2, g3, g4, g5, g6, g7;

   #define RABBIT_NS_PRE \
      _asm mov esi, p_instance \
      _asm mov ecx, [esi]RABBIT_ctx.carry

#define RABBIT_G(i) \
      _asm mov edx, [esi]RABBIT_ctx.c##i \
      _asm mov eax, [esi]RABBIT_ctx.x##i \
      _asm add eax, edx \
      _asm mul eax \
      _asm xor edx, eax \
      _asm mov g##i, edx

   #define RABBIT_COUNTER(i, a) \
      _asm mov edx, [esi]RABBIT_ctx.c##i \
      _asm add edx, ecx \
      _asm mov ecx, 0 \
      _asm adc ecx, a \
      _asm mov [esi]RABBIT_ctx.c##i, edx

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

   #define RABBIT_G_AND_LATE_COUNTER(i, a) \
      _asm mov edx, [esi]RABBIT_ctx.c##i \
      _asm mov eax, [esi]RABBIT_ctx.x##i \
      _asm add eax, edx \
      _asm add edx, ecx \
      _asm mov ecx, 0 \
      _asm adc ecx, a \
      _asm mov [esi]RABBIT_ctx.c##i, edx \
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

#elif defined(RABBIT_GCC) && (ECRYPT_VARIANT > 2)
   /* COMPILER : GCC */
   /* PROCESSOR: x86 */

   #if (ECRYPT_VARIANT > 2)
      #define RABBIT_NS_VARS \
         u32 g0, g1, g2, g3, g4, g5, g6, g7, c_a, asm_dummy1;
   #else
      #define RABBIT_NS_VARS \
         u32 g0, g1, g2, g3, g4, g5, g6, g7, c_a;
   #endif

   #define RABBIT_NS_PRE \
      c_a = p_instance->carry;

   #define RABBIT_G(i) \
      __asm ( \
      "movl  %1, %%eax\n\t" \
      "addl  %2, %%eax\n\t" \
      "mull  %%eax\n\t" \
      "xorl  %%eax, %0\n\t" \
      : "=d" (g##i) \
      : "m" (p_instance->c##i), "m" (p_instance->x##i) \
      : "cc", "%eax");

   #define RABBIT_COUNTER(i, a) \
      __asm ( \
      "addl  %2, %1\n\t" \
      "movl  $0, %0\n\t" \
      "adcl  %4, %0\n\t" \
      : "=r" (c_a), "=m" (p_instance->c##i) \
      : "0"  (c_a), "m"  (p_instance->c##i), "i" (a) \
      : "cc");

   #define RABBIT_G_AND_COUNTER(i, a) \
      __asm ( \
      "movl  %3, %%eax\n\t" \
      "addl  %5, %%eax\n\t" \
      "movl  $0, %1\n\t" \
      "adcl  %6, %1\n\t" \
      "movl  %%eax, %2\n\t" \
      "addl  %4, %%eax\n\t" \
      "mull  %%eax\n\t" \
      "xorl  %%eax, %0\n\t" \
      : "=d" (g##i), "=r" (c_a), "=m" (p_instance->c##i) \
      : "m" (p_instance->c##i), "m" (p_instance->x##i), "1" (c_a), "i" (a) \
      : "cc", "%eax");

   #define RABBIT_G_AND_LATE_COUNTER(i, a) \
      __asm ( \
      "movl  %3, %0\n\t" \
      "movl  %4, %%eax\n\t" \
      "addl  %0, %%eax\n\t" \
      "addl  %5, %0\n\t" \
      "movl  $0, %1\n\t" \
      "adcl  %6, %1\n\t" \
      "movl  %0, %2\n\t" \
      "mull  %%eax\n\t" \
      "xorl  %%eax, %0\n\t" \
      : "=d" (g##i), "=r" (c_a), "=m" (p_instance->c##i) \
      : "m" (p_instance->c##i), "m" (p_instance->x##i), "1" (c_a), "i" (a) \
      : "cc", "%eax");

//   #if (ECRYPT_VARIANT > 3)

      #undef RABBIT_GEN_X_EVEN
      #undef RABBIT_GEN_X_ODD

      #define RABBIT_GEN_X_EVEN(i, j, k) \
      __asm ( \
         "roll  $16, %0\n\t" \
         "roll  $16, %4\n\t" \
         "addl  %2, %0\n\t" \
         "addl  %4, %0\n\t" \
         : "=r" (p_instance->x##i), "=r" (asm_dummy1) \
         : "g" (g##i), "0" (g##j), "1" (g##k) \
         : "cc" );

      #define RABBIT_GEN_X_ODD(i, j, k) \
         __asm ( \
         "roll  $8, %0\n\t" \
         "addl  %1, %0\n\t" \
         "addl  %3, %0\n\t" \
         : "=r" (p_instance->x##i) \
         : "g" (g##i), "0" (g##j), "g" (g##k) \
         : "cc" );

//   #endif

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

#if ECRYPT_VARIANT > 3
   /* With counters after next-state */

   /* Calculate new counter values */
   /* Calculate new g values */
   /* Calculate new x values */
   RABBIT_NS_PRE
   RABBIT_G_AND_LATE_COUNTER(0, 0xD34D34D3)
   RABBIT_G_AND_LATE_COUNTER(1, 0x34D34D34)
   RABBIT_G_AND_LATE_COUNTER(2, 0x4D34D34D)
   RABBIT_GEN_X_EVEN(2, 1, 0)
   RABBIT_G_AND_LATE_COUNTER(3, 0xD34D34D3)
   RABBIT_GEN_X_ODD(3, 2, 1)
   RABBIT_G_AND_LATE_COUNTER(4, 0x34D34D34)
   RABBIT_GEN_X_EVEN(4, 3, 2)
   RABBIT_G_AND_LATE_COUNTER(5, 0x4D34D34D)
   RABBIT_GEN_X_ODD(5, 4, 3)
   RABBIT_G_AND_LATE_COUNTER(6, 0xD34D34D3)
   RABBIT_GEN_X_EVEN(6, 5, 4)
   RABBIT_G_AND_LATE_COUNTER(7, 0x4D34D34D)
   RABBIT_GEN_X_ODD(7, 6, 5)
   RABBIT_GEN_X_EVEN(0, 7, 6)
   RABBIT_GEN_X_ODD(1, 0, 7)
   RABBIT_NS_POST
#else
   /* With counters before next-state */

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
#endif
}

/* ------------------------------------------------------------------------- */

#if defined(RABBIT_MSC) && (ECRYPT_VARIANT > 3)
   /* COMPILER : MICROSOFT OR INTEL */
   /* PROCESSOR: x86 */

__declspec(naked) void __stdcall _process_opt(RABBIT_ctx* p_inst, const u8* p_src, s32 src_offset, const u8* src_max)
{
  __asm
  {
      sub     esp, 8
      mov     dword ptr [esp+ 4], ebx
      mov     dword ptr [esp+ 0], esi
      mov     eax, dword ptr [esp+12]
      mov     ebx, dword ptr [esp+16]
      mov     ecx, dword ptr [esp+20]
      mov     edx, dword ptr [esp+24]
      movq    mm0, qword ptr [eax+ 0]
      movq    mm1, qword ptr [eax+ 8]
      movq    mm3, qword ptr [eax+24]
      movq    mm4, qword ptr [eax+32]
      movq    mm5, qword ptr [eax+40]
      mov     esi, dword ptr [eax+64]
      align   16
process_opt_loop:
      paddd   mm0, mm4
      movq    mm6, qword ptr [eax+48]
      add     dword ptr [eax+32], esi
      pshufw  mm7, mm0, 0x4E
      paddd   mm1, mm5
      movq    mm2, qword ptr [eax+16]
      pmuludq mm0, mm0
      paddd   mm2, mm6
      movq    mm6, qword ptr [eax+56]
      pmuludq mm7, mm7
      movq    mm4, mm0
      adc     dword ptr [eax+40], 0xD34D34D3
      paddd   mm3, mm6
      pshufw  mm6, mm1, 0x4E
      punpckhdq mm0, mm7
      pmuludq mm1, mm1
      punpckldq mm4, mm7
      pshufw  mm7, mm2, 0x4E
      pmuludq mm6, mm6
      pxor    mm4, mm0
      pmuludq mm2, mm2
      adc     dword ptr [eax+48], 0x34d34d34
      movq    mm5, mm1
      pmuludq mm7, mm7
      punpckldq mm5, mm6
      punpckhdq mm1, mm6
      pxor    mm5, mm1
      movq    mm6, mm2
      pshufw  mm1, mm3, 0x4E
      pmuludq mm3, mm3
      punpckldq mm6, mm7
      adc     dword ptr [eax+56], 0x4D34D34D
      punpckhdq mm2, mm7
      pmuludq mm1, mm1
      pshufw  mm0, mm4, 0xB1
      movq    mm7, mm3
      pxor    mm6, mm2
      pshufw  mm2, mm5, 0xB1
      paddd   mm2, mm0
      pshufw  mm0, mm6, 0x1B
      punpckldq mm7, mm1
      adc     dword ptr [eax+36], 0xD34D34D3
      punpckhdq mm3, mm1
      movq    mm1, mm4
      paddd   mm0, mm4
      pslld   mm4, 8
      pxor    mm7, mm3
      pshufw  mm3, mm7, 0x1B
      paddd   mm0, mm3
      movq    mm3, mm6
      paddd   mm2, mm6
      adc     dword ptr [eax+44], 0x34D34D34
      psrld   mm1, 24
      por     mm1, mm4
      psrld   mm3, 24
      pslld   mm6, 8
      paddd   mm1, mm5
      pshufw  mm4, mm7, 0x4E
      por     mm3, mm6
      paddd   mm7, mm5
      adc     dword ptr [eax+52], 0x4D34D34D
      paddd   mm1, mm4
      paddd   mm3, mm7
      pshufw  mm7, mm1, 0x27
      pshufw  mm5, mm3, 0x78
      movq    qword ptr [eax+16], mm2
      movq    mm4, mm5
      punpckhwd mm5, mm7
      adc     dword ptr [eax+60], 0xD34D34D3
      punpcklwd mm7, mm4
      movq    mm4, qword ptr [ebx]
      pxor    mm7, mm0
      pxor    mm5, mm2
      pshufw  mm6, mm7, 0xE4
      movq    mm2, qword ptr [ebx+ 8]
      punpckldq mm7, mm5
      mov     esi, 0
      adc     esi, 0x4D34D34D
      add     ebx, 16
      pxor    mm7, mm4
      movq    mm4, qword ptr [eax+32]
      movq    qword ptr [ebx+ecx-16], mm7
      punpckhdq mm6, mm5
      cmp     ebx, edx
      movq    mm5, qword ptr [eax+40]
      pxor    mm6, mm2
      movq    qword ptr [ebx+ecx- 8], mm6
      jb      process_opt_loop
      movq    qword ptr [eax+ 0], mm0
      movq    qword ptr [eax+ 8], mm1
      movq    qword ptr [eax+24], mm3
      mov     dword ptr [eax+64], esi
      mov     ebx, dword ptr [esp+ 4]
      mov     esi, dword ptr [esp+ 0]
      add     esp, 8
      emms
      ret     16
  }
}

/* ------------------------------------------------------------------------- */

__declspec(naked) void __stdcall _keystream_opt(RABBIT_ctx* p_inst, const u8* p_dst, const u8* dst_max)
{
  __asm
  {
      sub     esp, 8
      mov     dword ptr [esp+ 4], ebx
      mov     dword ptr [esp+ 0], esi
      mov     eax, dword ptr [esp+12]
      mov     ebx, dword ptr [esp+16]
      mov     edx, dword ptr [esp+20]
      movq    mm0, qword ptr [eax+ 0]
      movq    mm1, qword ptr [eax+ 8]
      movq    mm3, qword ptr [eax+24]
      movq    mm4, qword ptr [eax+32]
      movq    mm5, qword ptr [eax+40]
      mov     esi, dword ptr [eax+64]
      align   16
keystream_opt_loop:
      paddd   mm0, mm4
      movq    mm6, qword ptr [eax+48]
      add     dword ptr [eax+32], esi
      pshufw  mm7, mm0, 0x4E
      paddd   mm1, mm5
      movq    mm2, qword ptr [eax+16]
      pmuludq mm0, mm0
      paddd   mm2, mm6
      movq    mm6, qword ptr [eax+56]
      pmuludq mm7, mm7
      movq    mm4, mm0
      adc     dword ptr [eax+40], 0xD34D34D3
      paddd   mm3, mm6
      pshufw  mm6, mm1, 0x4E
      punpckhdq mm0, mm7
      pmuludq mm1, mm1
      punpckldq mm4, mm7
      pshufw  mm7, mm2, 0x4E
      pmuludq mm6, mm6
      pxor    mm4, mm0
      pmuludq mm2, mm2
      adc     dword ptr [eax+48], 0x34d34d34
      movq    mm5, mm1
      pmuludq mm7, mm7
      punpckldq mm5, mm6
      punpckhdq mm1, mm6
      pxor    mm5, mm1
      movq    mm6, mm2
      pshufw  mm1, mm3, 0x4E
      pmuludq mm3, mm3
      punpckldq mm6, mm7
      adc     dword ptr [eax+56], 0x4D34D34D
      punpckhdq mm2, mm7
      pmuludq mm1, mm1
      pshufw  mm0, mm4, 0xB1
      movq    mm7, mm3
      pxor    mm6, mm2
      pshufw  mm2, mm5, 0xB1
      paddd   mm2, mm0
      pshufw  mm0, mm6, 0x1B
      punpckldq mm7, mm1
      adc     dword ptr [eax+36], 0xD34D34D3
      punpckhdq mm3, mm1
      movq    mm1, mm4
      paddd   mm0, mm4
      pslld   mm4, 8
      pxor    mm7, mm3
      pshufw  mm3, mm7, 0x1B
      paddd   mm0, mm3
      movq    mm3, mm6
      paddd   mm2, mm6
      adc     dword ptr [eax+44], 0x34D34D34
      psrld   mm1, 24
      por     mm1, mm4
      psrld   mm3, 24
      pslld   mm6, 8
      paddd   mm1, mm5
      pshufw  mm4, mm7, 0x4E
      por     mm3, mm6
      paddd   mm7, mm5
      adc     dword ptr [eax+52], 0x4D34D34D
      paddd   mm1, mm4
      paddd   mm3, mm7
      pshufw  mm7, mm1, 0x27
      pshufw  mm5, mm3, 0x78
      movq    qword ptr [eax+16], mm2
      movq    mm4, mm5
      punpckhwd mm5, mm7
      adc     dword ptr [eax+60], 0xD34D34D3
      punpcklwd mm7, mm4
      pxor    mm7, mm0
      pxor    mm5, mm2
      pshufw  mm6, mm7, 0xE4
      punpckldq mm7, mm5
      mov     esi, 0
      adc     esi, 0x4D34D34D
      add     ebx, 16
      movq    mm4, qword ptr [eax+32]
      movq    qword ptr [ebx-16], mm7
      punpckhdq mm6, mm5
      cmp     ebx, edx
      movq    mm5, qword ptr [eax+40]
      movq    qword ptr [ebx- 8], mm6
      jb      keystream_opt_loop
      movq    qword ptr [eax+ 0], mm0
      movq    qword ptr [eax+ 8], mm1
      movq    qword ptr [eax+24], mm3
      mov     dword ptr [eax+64], esi
      mov     ebx, dword ptr [esp+ 4]
      mov     esi, dword ptr [esp+ 0]
      add     esp, 8
      emms
      ret     12
  }
}

/* ------------------------------------------------------------------------- */

__declspec(naked) void __stdcall _nextstate_opt(RABBIT_ctx* p_inst, int iterations)
{
  __asm
  {
      sub     esp, 8
      mov     dword ptr [esp+ 4], ebx
      mov     dword ptr [esp+ 0], esi
      mov     eax, dword ptr [esp+12]
      mov     ebx, 0
      mov     edx, dword ptr [esp+16]
      movq    mm0, qword ptr [eax+ 0]
      movq    mm1, qword ptr [eax+ 8]
      movq    mm3, qword ptr [eax+24]
      movq    mm4, qword ptr [eax+32]
      movq    mm5, qword ptr [eax+40]
      mov     esi, dword ptr [eax+64]
      align   16
nextstate_opt_loop:
      paddd   mm0, mm4
      movq    mm6, qword ptr [eax+48]
      add     dword ptr [eax+32], esi
      pshufw  mm7, mm0, 0x4E
      paddd   mm1, mm5
      movq    mm2, qword ptr [eax+16]
      pmuludq mm0, mm0
      paddd   mm2, mm6
      movq    mm6, qword ptr [eax+56]
      pmuludq mm7, mm7
      movq    mm4, mm0
      adc     dword ptr [eax+40], 0xD34D34D3
      paddd   mm3, mm6
      pshufw  mm6, mm1, 0x4E
      punpckhdq mm0, mm7
      pmuludq mm1, mm1
      punpckldq mm4, mm7
      pshufw  mm7, mm2, 0x4E
      pmuludq mm6, mm6
      pxor    mm4, mm0
      pmuludq mm2, mm2
      adc     dword ptr [eax+48], 0x34d34d34
      movq    mm5, mm1
      pmuludq mm7, mm7
      punpckldq mm5, mm6
      punpckhdq mm1, mm6
      pxor    mm5, mm1
      movq    mm6, mm2
      pshufw  mm1, mm3, 0x4E
      pmuludq mm3, mm3
      punpckldq mm6, mm7
      adc     dword ptr [eax+56], 0x4D34D34D
      punpckhdq mm2, mm7
      pmuludq mm1, mm1
      pshufw  mm0, mm4, 0xB1
      movq    mm7, mm3
      pxor    mm6, mm2
      pshufw  mm2, mm5, 0xB1
      paddd   mm2, mm0
      pshufw  mm0, mm6, 0x1B
      punpckldq mm7, mm1
      adc     dword ptr [eax+36], 0xD34D34D3
      punpckhdq mm3, mm1
      movq    mm1, mm4
      paddd   mm0, mm4
      pslld   mm4, 8
      pxor    mm7, mm3
      pshufw  mm3, mm7, 0x1B
      paddd   mm0, mm3
      movq    mm3, mm6
      paddd   mm2, mm6
      adc     dword ptr [eax+44], 0x34D34D34
      psrld   mm1, 24
      por     mm1, mm4
      psrld   mm3, 24
      pslld   mm6, 8
      paddd   mm1, mm5
      pshufw  mm4, mm7, 0x4E
      por     mm3, mm6
      paddd   mm7, mm5
      adc     dword ptr [eax+52], 0x4D34D34D
      paddd   mm1, mm4
      paddd   mm3, mm7
      movq    qword ptr [eax+16], mm2
      adc     dword ptr [eax+60], 0xD34D34D3
      mov     esi, 0
      adc     esi, 0x4D34D34D
      add     ebx, 1
      movq    mm4, qword ptr [eax+32]
      cmp     ebx, edx
      movq    mm5, qword ptr [eax+40]
      jb      nextstate_opt_loop
      movq    qword ptr [eax+ 0], mm0
      movq    qword ptr [eax+ 8], mm1
      movq    qword ptr [eax+24], mm3
      mov     dword ptr [eax+64], esi
      mov     ebx, dword ptr [esp+ 4]
      mov     esi, dword ptr [esp+ 0]
      add     esp, 8
      emms
      ret     8
  }
}

/* -------------------------------------------------------------------------- */

#elif defined(RABBIT_GCC) && (ECRYPT_VARIANT > 4)
   /* COMPILER : GCC */
   /* PROCESSOR: x86 */

void _process_opt(RABBIT_ctx* p_inst, const u8* p_src, s32 src_offset, const u8* src_max)
{
  __asm__ __volatile__ (
      ".intel_syntax noprefix;"

      // Push registers
      "sub     esp, 8;"
      "mov     dword ptr [esp+ 4], ebp;"
      "mov     dword ptr [esp+ 0], esi;"

      // Load data
      "movq    mm0, qword ptr [eax+ 0];"  // X4:X0
      "movq    mm1, qword ptr [eax+ 8];"  // X5:X1
      "movq    mm3, qword ptr [eax+24];"  // X7:X3
      "movq    mm4, qword ptr [eax+32];"  // C4:C0 [i+1]
      "movq    mm5, qword ptr [eax+40];"  // C5:C1 [i+1]
      "mov     esi, dword ptr [eax+64];"  // carry + A0 [i+1]

      ".align   16;"
"process_opt_loop:"
      "paddd   mm0, mm4;"                 // X4:X0 + C4:C0
      "movq    mm6, qword ptr [eax+48];"  // C6:C2 [i+1]
      "add     dword ptr [eax+32], esi;"  // C0 += carry + A0
      "pshufw  mm7, mm0, 0x4E;"           // X0:X4 + C0:C4
      "paddd   mm1, mm5;"                 // X5:X1 + C5:C1
      "movq    mm2, qword ptr [eax+16];"  // X6:X2
      "pmuludq mm0, mm0;"                 // (X0+C0)^2
      "paddd   mm2, mm6;"                 // X6:X2 + C6:C2
      "movq    mm6, qword ptr [eax+56];"  // C7:C3
      "pmuludq mm7, mm7;"                 // (X4+C4)^2
      "movq    mm4, mm0;"                 // (X0+C0)^2
      "adc     dword ptr [eax+40], 0xD34D34D3;" // C1 += carry + A1
      "paddd   mm3, mm6;"                 // X7:X3 + C7:C3
      "pshufw  mm6, mm1, 0x4E;"           // X1:X5 + C1:C5
      "punpckhdq mm0, mm7;"               // (X4+C4)^2 high : (X0+C0)^2 high
      "pmuludq mm1, mm1;"                 // (X1+C1)^2
      "punpckldq mm4, mm7;"               // (X4+C4)^2 low : (X0+C0)^2 low
      "pshufw  mm7, mm2, 0x4E;"           // X2:X6 + C2:C6
      "pmuludq mm6, mm6;"                 // (X5+C5)^2
      "pxor    mm4, mm0;"                 // G4:G0
      "pmuludq mm2, mm2;"                 // (X2+C2)^2
      "adc     dword ptr [eax+48], 0x34d34d34;" // C2 += carry + A2
      "movq    mm5, mm1;"                 // (X1+C1)^2
      "pmuludq mm7, mm7;"                 // (X6+C6)^2
      "punpckldq mm5, mm6;"               // (X5+C5)^2 low : (X1+C1)^2 low
      "punpckhdq mm1, mm6;"               // (X5+C5)^2 high : (X1+C1)^2 high
      "pxor    mm5, mm1;"                 // G5:G1
      "movq    mm6, mm2;"                 // (X2+C2)^2
      "pshufw  mm1, mm3, 0x4E;"           // X3:X7 + C3:C7
      "pmuludq mm3, mm3;"                 // (X3+C3)^2
      "punpckldq mm6, mm7;"               // (X6+C6)^2 low : (X2:X2)^2 low
      "adc     dword ptr [eax+56], 0x4D34D34D;" // C3 += carry + A3
      "punpckhdq mm2, mm7;"               // (X6+C6)^2 high : (X2:X2)^2 high
      "pmuludq mm1, mm1;"                 // (X7:C7)^2
      "pshufw  mm0, mm4, 0xB1;"           // G4:G0 <<< 16
      "movq    mm7, mm3;"                 // (X3+C3)^2
      "pxor    mm6, mm2;"                 // G6:G2
      "pshufw  mm2, mm5, 0xB1;"           // G5:G1 <<< 16
      "paddd   mm2, mm0;"                 // (G4:G0 <<< 16) + (G5:G1 <<< 16)
      "pshufw  mm0, mm6, 0x1B;"           // G2:G6 <<< 16
      "punpckldq mm7, mm1;"               // (X7:C7)^2 low : (X3+C3)^2 low
      "adc     dword ptr [eax+36], 0xD34D34D3;" // C4 += carry + A4
      "punpckhdq mm3, mm1;"               // (X7:C7)^2 high : (X3+C3)^2 high
      "movq    mm1, mm4;"                 // G4:G0
      "paddd   mm0, mm4;"                 // G4:G0 + (G2:G6 <<< 16)
      "pslld   mm4, 8;"                   // G4:G0 << 8
      "pxor    mm7, mm3;"                 // G7:G3
      "pshufw  mm3, mm7, 0x1B;"           // G3:G7 <<< 16
      "paddd   mm0, mm3;"                 // X4:X0 [i+1] = G4:G0 + (G3:G7 <<< 16) + (G2:G6 <<< 16)
      "movq    mm3, mm6;"                 // G6:G2
      "paddd   mm2, mm6;"                 // X6:X2 [i+1] = G6:G2 + (G5:G1 <<< 16) + (G4:G0 <<< 16)
      "adc     dword ptr [eax+44], 0x34D34D34;" // C5 += carry + A5
      "psrld   mm1, 24;"                  // G4:G0 >> 24
      "por     mm1, mm4;"                 // G4:G0 <<< 8
      "psrld   mm3, 24;"                  // G6:G2 >> 24
      "pslld   mm6, 8;"                   // G6:G2 << 8
      "paddd   mm1, mm5;"                 // G5:G1 + (G4:G0 <<< 8)
      "pshufw  mm4, mm7, 0x4E;"           // G3:G7
      "por     mm3, mm6;"                 // G6:G2 <<< 8
      "paddd   mm7, mm5;"                 // G7:G3 + G5:G1
      "adc     dword ptr [eax+52], 0x4D34D34D;" // C6 += carry + A6
      "paddd   mm1, mm4;"                 // X5:X1 [i+1] = G5:G1 + (G4:G0 <<< 8) + G3:G7
      "paddd   mm3, mm7;"                 // X7:X3 [i+1] = G7:G3 + (G6:G2 <<< 8) + G5:G1
      "pshufw  mm7, mm1, 0x27;"           // X1l:X5l:X1h:X5h
      "pshufw  mm5, mm3, 0x78;"           // X3h:X7h:X7l:X3l
      "movq    qword ptr [eax+16], mm2;"  // Save X6:X2
      "movq    mm4, mm5;"                 // X3h:X7h:X7l:X3l
      "punpckhwd mm5, mm7;"               // X1l:X3h:H5l:X7h
      "adc     dword ptr [eax+60], 0xD34D34D3;" // C7 += carry + A7
      "punpcklwd mm7, mm4;"               // X7l:X1h:X3l:X5h
      "movq    mm4, qword ptr [ebx];"     // To encrypt [low]
      "pxor    mm7, mm0;"                 // s5:s4:s1:s0 = X4h:X4l:X0h:X0l XOR X7l:X1h:X3l:X5h
      "pxor    mm5, mm2;"                 // s7:s6:s3:s2 = X6h:X6l:X2h:X2l XOR X1l:X3h:H5l:X7h
      "pshufw  mm6, mm7, 0xE4;"           // COPY ???
      "movq    mm2, qword ptr [ebx+ 8];"  // To encrypt [high]
      "punpckldq mm7, mm5;"
      "mov     esi, 0;"
      "adc     esi, 0x4D34D34D;"
      "add     ebx, 16;"
      "pxor    mm7, mm4;"
      "movq    mm4, qword ptr [eax+32];"
      "movq    qword ptr [ebx+ecx-16], mm7;"
      "punpckhdq mm6, mm5;"
      "cmp     ebx, edx;"
      "movq    mm5, qword ptr [eax+40];"
      "pxor    mm6, mm2;"
      "movq    qword ptr [ebx+ecx- 8], mm6;"
      "jb      process_opt_loop;"

      // Save data
      "movq    qword ptr [eax+ 0], mm0;"
      "movq    qword ptr [eax+ 8], mm1;"
      "movq    qword ptr [eax+24], mm3;"
      "mov     dword ptr [eax+64], esi;"

      // Restore registers
      "mov     ebp, dword ptr [esp+ 4];"
      "mov     esi, dword ptr [esp+ 0];"
      "add     esp, 8;"
      "emms;"

      ".att_syntax prefix;"
      :
      : "a" (p_inst), "b" (p_src), "c" (src_offset), "d" (src_max)
      : "%edi", "memory", "cc"
   );
}

/* -------------------------------------------------------------------------- */

void _keystream_opt(RABBIT_ctx* p_inst, const u8* p_dst, const u8* dst_max)
{
  __asm__ __volatile__ (
      ".intel_syntax noprefix;"
      "sub     esp, 8;"
      "mov     dword ptr [esp+ 4], ebp;"
      "mov     dword ptr [esp+ 0], esi;"
      "movq    mm0, qword ptr [eax+ 0];"
      "movq    mm1, qword ptr [eax+ 8];"
      "movq    mm3, qword ptr [eax+24];"
      "movq    mm4, qword ptr [eax+32];"
      "movq    mm5, qword ptr [eax+40];"
      "mov     esi, dword ptr [eax+64];"
      ".align   16;"
"keystream_opt_loop:"
      "paddd   mm0, mm4;"
      "movq    mm6, qword ptr [eax+48];"
      "add     dword ptr [eax+32], esi;"
      "pshufw  mm7, mm0, 0x4E;"
      "paddd   mm1, mm5;"
      "movq    mm2, qword ptr [eax+16];"
      "pmuludq mm0, mm0;"
      "paddd   mm2, mm6;"
      "movq    mm6, qword ptr [eax+56];"
      "pmuludq mm7, mm7;"
      "movq    mm4, mm0;"
      "adc     dword ptr [eax+40], 0xD34D34D3;"
      "paddd   mm3, mm6;"
      "pshufw  mm6, mm1, 0x4E;"
      "punpckhdq mm0, mm7;"
      "pmuludq mm1, mm1;"
      "punpckldq mm4, mm7;"
      "pshufw  mm7, mm2, 0x4E;"
      "pmuludq mm6, mm6;"
      "pxor    mm4, mm0;"
      "pmuludq mm2, mm2;"
      "adc     dword ptr [eax+48], 0x34d34d34;"
      "movq    mm5, mm1;"
      "pmuludq mm7, mm7;"
      "punpckldq mm5, mm6;"
      "punpckhdq mm1, mm6;"
      "pxor    mm5, mm1;"
      "movq    mm6, mm2;"
      "pshufw  mm1, mm3, 0x4E;"
      "pmuludq mm3, mm3;"
      "punpckldq mm6, mm7;"
      "adc     dword ptr [eax+56], 0x4D34D34D;"
      "punpckhdq mm2, mm7;"
      "pmuludq mm1, mm1;"
      "pshufw  mm0, mm4, 0xB1;"
      "movq    mm7, mm3;"
      "pxor    mm6, mm2;"
      "pshufw  mm2, mm5, 0xB1;"
      "paddd   mm2, mm0;"
      "pshufw  mm0, mm6, 0x1B;"
      "punpckldq mm7, mm1;"
      "adc     dword ptr [eax+36], 0xD34D34D3;"
      "punpckhdq mm3, mm1;"
      "movq    mm1, mm4;"
      "paddd   mm0, mm4;"
      "pslld   mm4, 8;"
      "pxor    mm7, mm3;"
      "pshufw  mm3, mm7, 0x1B;"
      "paddd   mm0, mm3;"
      "movq    mm3, mm6;"
      "paddd   mm2, mm6;"
      "adc     dword ptr [eax+44], 0x34D34D34;"
      "psrld   mm1, 24;"
      "por     mm1, mm4;"
      "psrld   mm3, 24;"
      "pslld   mm6, 8;"
      "paddd   mm1, mm5;"
      "pshufw  mm4, mm7, 0x4E;"
      "por     mm3, mm6;"
      "paddd   mm7, mm5;"
      "adc     dword ptr [eax+52], 0x4D34D34D;"
      "paddd   mm1, mm4;"
      "paddd   mm3, mm7;"
      "pshufw  mm7, mm1, 0x27;"
      "pshufw  mm5, mm3, 0x78;"
      "movq    qword ptr [eax+16], mm2;"
      "movq    mm4, mm5;"
      "punpckhwd mm5, mm7;"
      "adc     dword ptr [eax+60], 0xD34D34D3;"
      "punpcklwd mm7, mm4;"
      "pxor    mm7, mm0;"
      "pxor    mm5, mm2;"
      "pshufw  mm6, mm7, 0xE4;"
      "punpckldq mm7, mm5;"
      "mov     esi, 0;"
      "adc     esi, 0x4D34D34D;"
      "add     ebx, 16;"
      "movq    mm4, qword ptr [eax+32];"
      "movq    qword ptr [ebx-16], mm7;"
      "punpckhdq mm6, mm5;"
      "cmp     ebx, edx;"
      "movq    mm5, qword ptr [eax+40];"
      "movq    qword ptr [ebx- 8], mm6;"
      "jb      keystream_opt_loop;"
      "movq    qword ptr [eax+ 0], mm0;"
      "movq    qword ptr [eax+ 8], mm1;"
      "movq    qword ptr [eax+24], mm3;"
      "mov     dword ptr [eax+64], esi;"
      "mov     ebp, dword ptr [esp+ 4];"
      "mov     esi, dword ptr [esp+ 0];"
      "add     esp, 8;"
      "emms;"
      ".att_syntax prefix;"
      :
      : "a" (p_inst), "b" (p_dst), "d" (dst_max)
      : "%edi", "memory", "cc"
   );
}

/* -------------------------------------------------------------------------- */

void _nextstate_opt(RABBIT_ctx* p_inst, int iterations)
{
  __asm__ __volatile__ (
      ".intel_syntax noprefix;"
      "sub     esp, 8;"
      "mov     dword ptr [esp+ 4], ebp;"
      "mov     dword ptr [esp+ 0], esi;"
      "movq    mm0, qword ptr [eax+ 0];"
      "movq    mm1, qword ptr [eax+ 8];"
      "movq    mm3, qword ptr [eax+24];"
      "movq    mm4, qword ptr [eax+32];"
      "movq    mm5, qword ptr [eax+40];"
      "mov     esi, dword ptr [eax+64];"
      "mov     ebx, 0;"
      ".align   16;"
"nextstate_opt_loop:"
      "paddd   mm0, mm4;"
      "movq    mm6, qword ptr [eax+48];"
      "add     dword ptr [eax+32], esi;"
      "pshufw  mm7, mm0, 0x4E;"
      "paddd   mm1, mm5;"
      "movq    mm2, qword ptr [eax+16];"
      "pmuludq mm0, mm0;"
      "paddd   mm2, mm6;"
      "movq    mm6, qword ptr [eax+56];"
      "pmuludq mm7, mm7;"
      "movq    mm4, mm0;"
      "adc     dword ptr [eax+40], 0xD34D34D3;"
      "paddd   mm3, mm6;"
      "pshufw  mm6, mm1, 0x4E;"
      "punpckhdq mm0, mm7;"
      "pmuludq mm1, mm1;"
      "punpckldq mm4, mm7;"
      "pshufw  mm7, mm2, 0x4E;"
      "pmuludq mm6, mm6;"
      "pxor    mm4, mm0;"
      "pmuludq mm2, mm2;"
      "adc     dword ptr [eax+48], 0x34d34d34;"
      "movq    mm5, mm1;"
      "pmuludq mm7, mm7;"
      "punpckldq mm5, mm6;"
      "punpckhdq mm1, mm6;"
      "pxor    mm5, mm1;"
      "movq    mm6, mm2;"
      "pshufw  mm1, mm3, 0x4E;"
      "pmuludq mm3, mm3;"
      "punpckldq mm6, mm7;"
      "adc     dword ptr [eax+56], 0x4D34D34D;"
      "punpckhdq mm2, mm7;"
      "pmuludq mm1, mm1;"
      "pshufw  mm0, mm4, 0xB1;"
      "movq    mm7, mm3;"
      "pxor    mm6, mm2;"
      "pshufw  mm2, mm5, 0xB1;"
      "paddd   mm2, mm0;"
      "pshufw  mm0, mm6, 0x1B;"
      "punpckldq mm7, mm1;"
      "adc     dword ptr [eax+36], 0xD34D34D3;"
      "punpckhdq mm3, mm1;"
      "movq    mm1, mm4;"
      "paddd   mm0, mm4;"
      "pslld   mm4, 8;"
      "pxor    mm7, mm3;"
      "pshufw  mm3, mm7, 0x1B;"
      "paddd   mm0, mm3;"
      "movq    mm3, mm6;"
      "paddd   mm2, mm6;"
      "adc     dword ptr [eax+44], 0x34D34D34;"
      "psrld   mm1, 24;"
      "por     mm1, mm4;"
      "psrld   mm3, 24;"
      "pslld   mm6, 8;"
      "paddd   mm1, mm5;"
      "pshufw  mm4, mm7, 0x4E;"
      "por     mm3, mm6;"
      "paddd   mm7, mm5;"
      "adc     dword ptr [eax+52], 0x4D34D34D;"
      "paddd   mm1, mm4;"
      "paddd   mm3, mm7;"
      "movq    qword ptr [eax+16], mm2;"
      "adc     dword ptr [eax+60], 0xD34D34D3;"
      "mov     esi, 0;"
      "adc     esi, 0x4D34D34D;"
      "add     ebx, 1;"
      "movq    mm4, qword ptr [eax+32];"
      "cmp     ebx, edx;"
      "movq    mm5, qword ptr [eax+40];"
      "jb      nextstate_opt_loop;"
      "movq    qword ptr [eax+ 0], mm0;"
      "movq    qword ptr [eax+ 8], mm1;"
      "movq    qword ptr [eax+24], mm3;"
      "mov     dword ptr [eax+64], esi;"
      "mov     ebp, dword ptr [esp+ 4];"
      "mov     esi, dword ptr [esp+ 0];"
      "add     esp, 8;"
      "emms;"
      ".att_syntax prefix;"
      :
      : "a" (p_inst), "d" (iterations)
      : "%edi", "%ebx", "memory", "cc"
   );
}

#endif

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
   u32 k0, k1, k2, k3;
#if ECRYPT_VARIANT < 5
   u32 i;
#endif
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

#if ((defined(RABBIT_MSC)) || defined(RABBIT_GCC)) && (ECRYPT_VARIANT > 4)

   p_instance = &(ctx->master_ctx);

   RABBIT_NS_PRE
   RABBIT_COUNTER(0, 0xD34D34D3)
   RABBIT_COUNTER(1, 0x34D34D34)
   RABBIT_COUNTER(2, 0x4D34D34D)
   RABBIT_COUNTER(3, 0xD34D34D3)
   RABBIT_COUNTER(4, 0x34D34D34)
   RABBIT_COUNTER(5, 0x4D34D34D)
   RABBIT_COUNTER(6, 0xD34D34D3)
   RABBIT_COUNTER(7, 0x4D34D34D)
   RABBIT_NS_POST

   _nextstate_opt(p_instance, 3);

   RABBIT_NS_PRE
   RABBIT_G(0)
   RABBIT_G(1)
   RABBIT_G(2)
   RABBIT_GEN_X_EVEN(2, 1, 0)
   RABBIT_G(3)
   RABBIT_GEN_X_ODD(3, 2, 1)
   RABBIT_G(4)
   RABBIT_GEN_X_EVEN(4, 3, 2)
   RABBIT_G(5)
   RABBIT_GEN_X_ODD(5, 4, 3)
   RABBIT_G(6)
   RABBIT_GEN_X_EVEN(6, 5, 4)
   RABBIT_G(7)
   RABBIT_GEN_X_ODD(7, 6, 5)
   RABBIT_GEN_X_EVEN(0, 7, 6)
   RABBIT_GEN_X_ODD(1, 0, 7)
   RABBIT_NS_POST

#else
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
#endif
   /* Modify the counters */
   ctx->master_ctx.c0 ^= ctx->master_ctx.x4;
   ctx->master_ctx.c1 ^= ctx->master_ctx.x5;
   ctx->master_ctx.c2 ^= ctx->master_ctx.x6;
   ctx->master_ctx.c3 ^= ctx->master_ctx.x7;
   ctx->master_ctx.c4 ^= ctx->master_ctx.x0;
   ctx->master_ctx.c5 ^= ctx->master_ctx.x1;
   ctx->master_ctx.c6 ^= ctx->master_ctx.x2;
   ctx->master_ctx.c7 ^= ctx->master_ctx.x3;
}

/* ------------------------------------------------------------------------- */

/* IV setup */
void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
   /* Temporary variables */
   u32 i0, i1, i2, i3;
#if (ECRYPT_VARIANT < 5) || !((defined(RABBIT_MSC)))
   u32 i;
   RABBIT_NS_VARS
#endif
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

#if (defined(RABBIT_MSC) || defined(RABBIT_GCC)) && (ECRYPT_VARIANT > 4)
   p_instance = &(ctx->work_ctx);

   RABBIT_NS_PRE
   RABBIT_COUNTER(0, 0xD34D34D3)
   RABBIT_COUNTER(1, 0x34D34D34)
   RABBIT_COUNTER(2, 0x4D34D34D)
   RABBIT_COUNTER(3, 0xD34D34D3)
   RABBIT_COUNTER(4, 0x34D34D34)
   RABBIT_COUNTER(5, 0x4D34D34D)
   RABBIT_COUNTER(6, 0xD34D34D3)
   RABBIT_COUNTER(7, 0x4D34D34D)
   RABBIT_NS_POST

   _nextstate_opt(p_instance, 4);
#else

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

#if ECRYPT_VARIANT > 3
   RABBIT_NS_PRE
   RABBIT_G_AND_COUNTER(0, 0xD34D34D3)
   RABBIT_G_AND_COUNTER(1, 0x34D34D34)
   RABBIT_G_AND_COUNTER(2, 0x4D34D34D)
   RABBIT_G_AND_COUNTER(3, 0xD34D34D3)
   RABBIT_G_AND_COUNTER(4, 0x34D34D34)
   RABBIT_G_AND_COUNTER(5, 0x4D34D34D)
   RABBIT_G_AND_COUNTER(6, 0xD34D34D3)
   RABBIT_G_AND_COUNTER(7, 0x4D34D34D)
   RABBIT_NS_POST
#endif
#endif
}

/* ------------------------------------------------------------------------- */

/* Encrypt/decrypt a message of any size */
void ECRYPT_process_bytes(int action, ECRYPT_ctx* ctx, const u8* input, 
          u8* output, u32 msglen)
{
   u32 i;
   u8 buffer[16];
   /* ECRYPT_ctx* temp = ctx; */

#if (defined(RABBIT_MSC) || defined(RABBIT_GCC)) && (ECRYPT_VARIANT > 4)
   /* RABBIT_ctx* p_instance = &ctx->work_ctx; */
   if (msglen >= 16)
   {
      _process_opt(&(ctx->work_ctx), input, (s32)(output-input), input+(msglen&0xFFFFFFF0U));

      input  += msglen&0xFFFFFFF0U;
      output += msglen&0xFFFFFFF0U;
      msglen -= msglen&0xFFFFFFF0U;
   }
#else
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
#endif
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

#if (defined(RABBIT_MSC) || defined(RABBIT_GCC)) && (ECRYPT_VARIANT > 4)
   if (length >= 16)
   {
      _keystream_opt(&(ctx->work_ctx), keystream, keystream + (length&0xFFFFFFF0U));

      keystream += length&0xFFFFFFF0U;
      length -= length&0xFFFFFFF0U;
   }
#else
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

#endif
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
#if (defined(RABBIT_MSC) || defined(RABBIT_GCC)) && (ECRYPT_VARIANT > 4)
   if (!blocks)
      return;
   _process_opt(&(ctx->work_ctx), input, (s32)(output-input), input+(blocks*16));
#else
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
#endif
}

/* ------------------------------------------------------------------------- */
