/* abc.h */

/*
 * ABC v.2 reference implementation
 *
 * ABC v.2 implementation-specific constants and macros.
 *
 *
 * LEGAL NOTICE
 *
 * The ABC stream cipher is copyright (C) 2005 Vladimir Anashin, Andrey
 * Bogdanov and Ilya Kizhvatov.
 *
 * The authors allow free usage of the ABC stream cipher design for any
 * non-commercial or commercial purposes provided each of the following holds:
 * 
 *   1. The original ABC stream cipher design is not modified in any way.
 *
 *   2. The name "ABC stream cipher" is clearly specified.
 *
 *   3. The names of the ABC stream cipher inventors (Vladimir Anashin, Andrey
 *      Bogdanov, Ilya Kizhvatov) are clearly specified.
 *
 * ABC reference implementation is provided "as is" without any express or
 * implied warranty. The authors are not in any way liable for any use of this
 * software.
 *
 *
 * CONTACT
 *
 *   Vladimir Anashin, Andrey Bogdanov, Ilya Kizhvatov
 *   {anashin, bogdanov, kizhvatov}@rsuh.ru
 *
 *   Faculty of Information Security,
 *   Institute for Information Sciences and Security Technologies,
 *   Russian State University for the Humanities,
 *   Kirovogradskaya Str. 25/2, 117534 Moscow, Russia.
 */

#ifndef ABC_H__
#define ABC_H__

#include "ecrypt-sync.h"


/* Optimization window size and cycle unroll count */
#if (ECRYPT_VARIANT == 10)
    #define ABC_WINDOW_1
    #define ABC_UNROLL_1
#elif (ECRYPT_VARIANT == 9)
    #define ABC_WINDOW_2
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 8)
    #define ABC_WINDOW_2
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 7)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 6)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 5)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_16
#elif (ECRYPT_VARIANT == 4)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 3)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 2)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_16
#elif (ECRYPT_VARIANT == 1)
    #define ABC_WINDOW_12
    #define ABC_UNROLL_16
#endif


/* ------------------------------------------------------------------------- */

/* A, Linear feedback shift registre */
#define ABC_A_MASK U32C(0x00000002) /* Mask for making LFSR state nonzero */

/* ------------------------------------------------------------------------- */

/* B, top function */
#define ABC_B_COEF_AND_MASK U32C(0xfffffffc) /* Mask making coefs 0 (mod 4) */
#define ABC_B_COEF_OR_MASK U32C(0x00000001)  /* Mask making coefs 1 (mod 2) */

/* ------------------------------------------------------------------------- */

/* C, filter function */
#define ABC_C_COEF_NUM 33               /* Number of coefs           */
#define ABC_C_AND_MASK U32C(0xffff0000) /* Mask for 31st coefficient */
#define ABC_C_OR_MASK  U32C(0x00010000) /* Mask for 31st coefficient */


#endif /* ABC_H__ */
