/* ecrypt-Moustique_cipher.c 
 * version November 08 2005
 * correction in init_ctx(): initialization of certain statebits to 0
 * correction in iterate(): errors are indicated
 */


#include <stdio.h>
#include <stdlib.h>
#include "ecrypt-portable.h"
#include "ecrypt-machine.h"
#include "ecrypt-config.h"
#include "ecrypt-ssyn.h"

#define ONE  ((u8) 0x01)

void init_ctx(ECRYPT_ctx *s)
{
  int i;

  s->C_0 = s->a0;      /* a[1...96]    is CCSR_0[1...96] */
  s->C_1 = s->a0+8;    /* a[97...104]  is CCSR_1[89...96] */
  s->C_2 = s->a0+8+4;  /* a[105...108] is CCSR_2[93...96] */
  s->C_3 = s->a0+8+8;  /* a[109...112] is CCSR_3[93...96] */
  s->C_4 = s->a0+16+2; /* a[113...114] is CCSR_4[95...96] */
  s->C_5 = s->a0+16+4; /* a[115...116] is CCSR_5[95...96] */
  s->C_6 = s->a0+16+6; /* a[117...118] is CCSR_6[95...96] */
  s->C_7 = s->a0+16+8; /* a[119...120] is CCSR_7[95...96] */
  s->C_8 = s->a0+24+1; /* a[121] is CCSR_8[96] */
  s->C_9 = s->a0+24+2; /* a[122] is CCSR_9[96] */
  s->C10 = s->a0+24+3; /* a[123] is CCSR_10[96] */
  s->C11 = s->a0+24+4; /* a[124] is CCSR_11[96] */
  s->C12 = s->a0+24+5; /* a[125] is CCSR_12[96] */
  s->C13 = s->a0+24+6; /* a[126] is CCSR_13[96] */
  s->C14 = s->a0+24+7; /* a[127] is CCSR_14[96] */
  s->C15 = s->a0+24+8; /* a[128] is CCSR_15[96] */

  for( i=53 ; i<56 ; i++ ) { 
       s->a1[i] = s->a2[i] = s->a3[i] = s->a4[i] = s->a5[i] = 0;
       }
  /* these statebits are constant and are 
   * only present to simplify the code of iterate()
   */
}

void byte_to_bits(u8 byt,u8 *bit) /* MSB to bit[0], LSB to bit[7] */
{
   bit[7] = byt&ONE; byt >>= 1;
   bit[6] = byt&ONE; byt >>= 1;
   bit[5] = byt&ONE; byt >>= 1;
   bit[4] = byt&ONE; byt >>= 1;
   bit[3] = byt&ONE; byt >>= 1;
   bit[2] = byt&ONE; byt >>= 1;
   bit[1] = byt&ONE; byt >>= 1;
   bit[0] = byt&ONE;
} 

u8 bits_to_byte(u8* bit)  /* bit[0] to MSB, bit[7] to LSB */
{
   u8 byt;
   byt  = bit[0]; byt <<= 1;
   byt ^= bit[1]; byt <<= 1;
   byt ^= bit[2]; byt <<= 1;
   byt ^= bit[3]; byt <<= 1;
   byt ^= bit[4]; byt <<= 1;
   byt ^= bit[5]; byt <<= 1;
   byt ^= bit[6]; byt <<= 1;
   byt ^= bit[7];
return(byt);   
} 

u8 g0(u8 a, u8 b, u8 c, u8 d)
{
         return( a^b^c^d );
}

u8 g1(u8 a, u8 b, u8 c, u8 d)
{
         return( a^b^(c&(d^ONE))^ ONE );
}
u8 g2(u8 a, u8 b, u8 c, u8 d)
{
         return( (a&(b^ONE))^ (c&(d^ONE)) );
}

u8 iterate(ECRYPT_ctx *s, u8 c_t) 
/* single iteration taking ONE bit of ciphertext and returning ONE bit of keystream */
{
   u8 z_t;
   u16 i,j;

/* the output from stage 8*/
   z_t = s->a8[0];
/* stage 8 from stage 7*/
   s->a8[0] = (s->a7[0]^s->a7[1]^s->a7[2])&ONE ;

/* the stages, same as in Mosquito */
   for( i=0 ; i<3 ; i++ )  s->a7[i] = g0(s->a6[4*i],s->a6[4*i+1],s->a6[4*i+2],s->a6[4*i+3]);
   for( i=0 ; i<12 ; i++ ) s->a6[i] = g1(s->a5[4*i],s->a5[4*i+3],s->a5[4*i+1],s->a5[4*i+2]);
   for( i=0 ; i<53 ; i++ ) s->a5[(4*i)%53] = g1(s->a4[i],s->a4[i+3],s->a4[i+1],s->a4[i+2]);
   for( i=0 ; i<53 ; i++ ) s->a4[(4*i)%53] = g1(s->a3[i],s->a3[i+3],s->a3[i+1],s->a3[i+2]);
   for( i=0 ; i<53 ; i++ ) s->a3[(4*i)%53] = g1(s->a2[i],s->a2[i+3],s->a2[i+1],s->a2[i+2]);
   for( i=0 ; i<53 ; i++ ) s->a2[(4*i)%53] = g1(s->a1[i],s->a1[i+3],s->a1[i+1],s->a1[i+2]);
   for( i=0 ; i<53 ; i++ ) s->a1[(4*i)%53] = g1(s->a0[128-i],s->a0[i+18],s->a0[113-i],s->a0[i+1]);

/* the CCSR, different from Mosquito */
   s->C15[96] = g2(s->C_7[95],s->C_0[95-15],s->C_3[94],s->C_0[94-15]);
   s->C14[96] = g2(s->C_6[95],s->C_0[95-14],s->C_2[94],s->C_0[94-14]);
   s->C13[96] = g2(s->C_5[95],s->C_0[95-13],s->C_1[94],s->C_0[94-13]);
   s->C12[96] = g2(s->C_4[95],s->C_0[95-12],s->C_0[94],s->C_0[94-12]);
   s->C11[96] = g2(s->C_3[95],s->C_0[95-11],s->C_3[94],s->C_0[94-11]);
   s->C10[96] = g2(s->C_2[95],s->C_0[95-10],s->C_2[94],s->C_0[94-10]);
   s->C_9[96] = g2(s->C_1[95],s->C_0[95- 9],s->C_1[94],s->C_0[94- 9]);
   s->C_8[96] = g2(s->C_0[95],s->C_0[95- 8],s->C_0[94],s->C_0[94- 8]);
   s->C_7[96] = g2(s->C_7[95],s->C_0[95- 7],s->C_3[94],s->C_0[94- 7]);
   s->C_6[96] = g2(s->C_6[95],s->C_0[95- 6],s->C_2[94],s->C_0[94- 6]);
   s->C_5[96] = g2(s->C_5[95],s->C_0[95- 5],s->C_1[94],s->C_1[94- 5]);
   s->C_4[96] = g2(s->C_4[95],s->C_0[95- 4],s->C_0[94],s->C_1[94- 4]);
   s->C_3[96] = g2(s->C_3[95],s->C_0[95- 3],s->C_3[94],s->C_1[94- 3]);
   s->C_2[96] = g2(s->C_2[95],s->C_0[95- 2],s->C_2[94],s->C_1[94- 2]);
   s->C_1[96] = g2(s->C_1[95],s->C_0[95- 1],s->C_1[94],s->C_1[94- 1]);
                                                                         /* j-i: x mod 6 or mod 3 */
   s->C_0[96] = g1(s->C_0[95],s->wrkey[95],s->C_0[96-5  ],s->C_0[0   ]); /* 96: 0*/

   s->C_7[95] = g0(s->C_3[94],s->wrkey[94],s->C_0[2*87/3],s->C_3[95-2]);/* 88: 1*/
   s->C_6[95] = g1(s->C_2[94],s->wrkey[94],s->C_0[95-4  ],s->C_2[95-2]);/* 89: 2*/
   s->C_5[95] = g1(s->C_1[94],s->wrkey[94],s->C_1[95-5  ],s->C_0[0   ]);/* 90: 0*/
   s->C_4[95] = g0(s->C_0[94],s->wrkey[94],s->C_0[2*90/3],s->C_0[95-2]);/* 91: 1*/
   s->C_3[95] = g1(s->C_3[94],s->wrkey[94],s->C_1[95-4  ],s->C_3[95-2]);/* 92: 2*/
   s->C_2[95] = g1(s->C_2[94],s->wrkey[94],s->C_0[0     ],s->C_2[95-2]);/* 93: 3*/
   s->C_1[95] = g0(s->C_1[94],s->wrkey[94],s->C_0[2*93/3],s->C_1[95-2]);/* 94: 1*/
   s->C_0[95] = g1(s->C_0[94],s->wrkey[94],s->C_0[95-4  ],s->C_0[95-2]);/* 95: 2*/

   s->C_3[94] = g0(s->C_3[93],s->wrkey[93],s->C_0[2*90/3],s->C_1[94-2]);/* 91: 1*/
   s->C_2[94] = g1(s->C_2[93],s->wrkey[93],s->C_0[94-4  ],s->C_0[94-2]);/* 92: 2*/
   s->C_1[94] = g1(s->C_1[93],s->wrkey[93],s->C_0[0     ],s->C_1[94-2]);/* 93: 3*/
   s->C_0[94] = g0(s->C_0[93],s->wrkey[93],s->C_0[2*93/3],s->C_0[94-2]);/* 94: 1*/

   s->C_3[93] = g1(s->C_1[92],s->wrkey[92],s->C_0[93-5  ],s->C_0[0   ]);/* 90: 0*/
   s->C_2[93] = g0(s->C_0[92],s->wrkey[92],s->C_0[2*90/3],s->C_0[93-2]);/* 91: 1*/
   s->C_1[93] = g1(s->C_1[92],s->wrkey[92],s->C_1[93-4  ],s->C_1[93-2]);/* 92: 2*/
   s->C_0[93] = g1(s->C_0[92],s->wrkey[92],s->C_0[0     ],s->C_0[93-2]);/* 93: 3*/

   s->C_1[92] = g0(s->C_1[91],s->wrkey[91],s->C_0[2*90/3],s->C_1[92-2]);/* 91: 1*/
   s->C_0[92] = g1(s->C_0[91],s->wrkey[91],s->C_0[92-4  ],s->C_0[92-2]);/* 92: 2*/

   s->C_1[91] = g1(s->C_1[90],s->wrkey[90],s->C_0[91-5  ],s->C_0[0   ]);/* 90: 0*/
   s->C_0[91] = g0(s->C_0[90],s->wrkey[90],s->C_0[2*90/3],s->C_0[91-2]);/* 91: 1*/

   s->C_1[90] = g1(s->C_1[89],s->wrkey[89],s->C_0[90-4  ],s->C_0[90-2]);/* 89: 2*/
   s->C_0[90] = g1(s->C_0[89],s->wrkey[89],s->C_0[90-5  ],s->C_0[0   ]);/* 90: 0*/

   s->C_1[89] = g0(s->C_0[88],s->wrkey[88],s->C_0[2*87/3],s->C_0[89-2]);/* 88: 1*/
   s->C_0[89] = g1(s->C_0[88],s->wrkey[88],s->C_0[89-4  ],s->C_0[89-2]);/* 89: 2*/

   for( j=88 ; j>2 ; j-- )
   {
      if( j%3 == 1 ) s->C_0[j] = g0(s->C_0[j-1],s->wrkey[j-1],s->C_0[2*(j-1)/3],s->C_0[j-2]); 
      if( j%3 == 2 ) s->C_0[j] = g1(s->C_0[j-1],s->wrkey[j-1],s->C_0[j-4]      ,s->C_0[j-2]);
      if( j%6 == 3 ) s->C_0[j] = g1(s->C_0[j-1],s->wrkey[j-1],s->C_0[0  ]      ,s->C_0[j-2]);
      if( j%6 == 0 ) s->C_0[j] = g1(s->C_0[j-1],s->wrkey[j-1],s->C_0[j-5]      ,s->C_0[0  ]);
   }

   s->C_0[2] = g1(s->C_0[1],s->wrkey[1],0,0); /* 2: 2*/
   s->C_0[1] = g0(s->C_0[0],s->wrkey[0],0,0); /* 1: 1*/
   s->C_0[0] = c_t&ONE;

   return(z_t);
}

u8 encrypt_bit(ECRYPT_ctx *s, u8 p) 
{
   u8 c;
   c = p ^ iterate(s,0);
   s->C_0[0] = c;
   return(c);
}

u8 decrypt_bit(ECRYPT_ctx *s, u8 c) 
{
   u8 p;
   p = c ^ iterate(s,c);
   return(p);
}


u8 encrypt_byte(ECRYPT_ctx *s, u8 p) 
{
   u8 pbit[8], cbit[8];	
   u16 i;

   byte_to_bits(p,pbit);
   for( i=0 ; i<8 ; i++) cbit[i] = encrypt_bit(s,pbit[i]);
   return(bits_to_byte(cbit));
}

u8 decrypt_byte(ECRYPT_ctx *s, u8 c) 
{
   u8 pbit[8], cbit[8];	
   u16 i;

   byte_to_bits(c,cbit);
   for( i=0 ; i<8 ; i++) pbit[i] = decrypt_bit(s,cbit[i]);
   return(bits_to_byte(pbit));
}


void ECRYPT_init()
{
/* no need for any operations here */
}

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,      /* Key size in bits, ignored as key must be 12 bytes long */ 
  u32 ivsize)       /* IV size in bits, should be a multiple of 8 */ 
{
   u16 i,j;
   u8 tmp[8];
   ctx->IVsize = ivsize;
   for( i=0 ; i<12 ; i++)
   {
      byte_to_bits(key[i],tmp);	
      for( j=0 ; j<8 ; j++) ctx->wrkey[i*8+j] = tmp[j];
   }
} 

void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx,
  u8* previous,
  const u8* iv)
{
   u16 i, IVsizebytes;
   u8 tmpzero;
   tmpzero = 0;
   IVsizebytes = (ctx->IVsize)/8;
   decrypt_bit(ctx, tmpzero);  /* first bit of initialization vector is 0 */
   for( i=0 ; i<IVsizebytes ; i++) decrypt_byte(ctx, iv[i]);   
   for( i=IVsizebytes ; i<13 ; i++) decrypt_byte(ctx, tmpzero);   /* padding with zeroes */
} /* initialises internal state, leaves previous[...] alone */


void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx,     /* no longer constant */
  const u8* previous,  /* not used */
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen)          /* Message length in bytes. */ 
{
   u32 i;
   for( i=0 ; i<msglen ; i++) ciphertext[i] = encrypt_byte(ctx, plaintext[i]);
}

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx, /* no longer constant */
  const u8* previous,    /* not used */
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen)                /* Message length in bytes. */ 
{
   u32 i;
   for( i=0 ; i<msglen ; i++) plaintext[i] = decrypt_byte(ctx, ciphertext[i]);
} 

#ifdef ECRYPT_USES_DEFAULT_ALL_IN_ONE

/*
 * Default implementation of all-in-one encryption/decryption of
 * (short) packets.
 */

void ECRYPT_encrypt_packet(
  ECRYPT_ctx* ctx,
  const u8* iv,
  const u8* plaintext,
  u8* ciphertext,
  u32 msglen)
{
  u8 previous[ECRYPT_SYNCLENGTH];

  ECRYPT_ivsetup(ctx, previous, iv);
  ECRYPT_encrypt_bytes(ctx, previous, plaintext, ciphertext, msglen);
}

void ECRYPT_decrypt_packet(
  ECRYPT_ctx* ctx,
  const u8* iv,
  const u8* ciphertext,
  u8* plaintext,
  u32 msglen)
{
  u8 previous[ECRYPT_SYNCLENGTH];

  ECRYPT_ivsetup(ctx, previous, iv);
  ECRYPT_decrypt_bytes(ctx, previous, ciphertext, plaintext, msglen);
}

#endif
 

