    /*
     * F-FCSR-16 reference implementation.
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
#define d0 0x390002C6    /* lower weight bits */
#define d1 0xEFB55A6E
#define d2 0xBAF08F39
#define d3 0x2102F996    
#define d4 0xC8C9CEDB
#define d5 0x780CAA2E
#define d6 0xAD4F7E66
#define d7 0xCB5E129F    /* higher weight bits */


#define MAXSHIFT (sizeof (u32) * CHAR_BIT ) - 1
#define LIMIT_WEIGHT 3

/* Update the shift register and the carry register of the FCSR */
void ECRYPT_clock(
  ECRYPT_ctx* ctx 
);

/* Produce one word of keystream from the internal state of the register */
u16 ECRYPT_filter(
  ECRYPT_ctx* ctx 
);
