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
#define d0 0x3DD4254E    /* lower weight bits */
#define d1 0xAF46D590
#define d2 0x8623DC8A
#define d3 0x26619FC5    
#define d4 0xAE985DFF    /* higher weight bits */

#define MAXSHIFT (sizeof (u32) * CHAR_BIT ) - 1
#define MIN_FILTER_LEFT_BIT 0x100000
#define LIMIT_WEIGHT 3

/* Update the shift register and the carry register of the FCSR */
void ECRYPT_clock(
  ECRYPT_ctx* ctx 
);

/* Produce one byte of keystream from the internal state of the register */
u8 ECRYPT_filter(
  ECRYPT_ctx* ctx 
);

void insertIV(
ECRYPT_ctx *ctx, 
const u8*iv
);
