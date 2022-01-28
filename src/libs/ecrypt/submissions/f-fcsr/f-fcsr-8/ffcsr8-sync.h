    /*
     * F-FCSR-8 reference implementation.
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
#include "ecrypt-sync.h"
#include <stdio.h>
#include <stdlib.h>


/* feedback polynomial d */
#define d0 0x1856EC4A    /* lower weight bits */
#define d1 0x9E86369A
#define d2 0xB7E25FD6
#define d3 0xB9C6A9EA    /* higher weight bits */

u32 ivD0Load[14]={  1 , 3 , 6 , 10 , 11 , 13 , 14 , 15 , 17 , 18 , 20 , 22 , 27 , 28 };
u32 ivD1Load[16]={  1 , 3 , 4 , 7 , 9 , 10 , 12 , 13 , 17 , 18 , 23 , 25 , 26 , 27 , 28 , 31 };
u32 ivD2Load[21]={  1 , 2 , 4 , 6 , 7 , 8 , 9 , 10 , 11 , 12 , 14 , 17 , 21 , 22 , 23 , 24 , 25 , 26 , 28 , 29 , 31 };
u32 ivD3Load[17]={  1 , 3 , 5 , 6 , 7 , 8 , 11 , 13 , 15 , 17 , 18 , 22 , 23 , 24 , 27 , 28 , 29 };

#define MAXSHIFT (sizeof (u32) * CHAR_BIT ) - 1
#define LIMIT_WEIGHT 3

/* Update the shift register and the carry register of the FCSR */
void ECRYPT_clock(
  ECRYPT_ctx* ctx 
);

/* Produce one byte of keystream from the internal state of the register */
u8 ECRYPT_filter(
  ECRYPT_ctx* ctx 
);

/* Compute the number of 1 in a 32 bit word */
u32 hammingWeight (
u32 n
);


u8 goodfilter (
ECRYPT_ctx *ctx
);

void insertIV(
ECRYPT_ctx *ctx, 
const u8*iv
);
