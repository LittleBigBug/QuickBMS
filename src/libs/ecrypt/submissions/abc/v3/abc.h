/* abc.h */

/*
 * ABC v.3 reference implementation
 *
 * Implementation-specific constants and macros.
 */

#ifndef ABC_H__
#define ABC_H__

#include "ecrypt-sync.h"

/* Optimization window size and cycle unroll count */
#if (ECRYPT_VARIANT == 1)
    #define ABC_WINDOW_12
    #define ABC_UNROLL_16
#elif (ECRYPT_VARIANT == 2)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_16
#elif (ECRYPT_VARIANT == 3)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 4)
    #define ABC_WINDOW_8
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 5)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_16
#elif (ECRYPT_VARIANT == 6)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 7)
    #define ABC_WINDOW_4
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 8)
    #define ABC_WINDOW_2
    #define ABC_UNROLL_8
#elif (ECRYPT_VARIANT == 9)
    #define ABC_WINDOW_2
    #define ABC_UNROLL_4
#elif (ECRYPT_VARIANT == 10)
    #define ABC_WINDOW_1
    #define ABC_UNROLL_4
#endif

/* ------------------------------------------------------------------------- */

/* A, Linear feedback shift register */
#define ABC_A_MASK U32C(0x00000002) /* Mask for making LFSR state nonzero    */

/* ------------------------------------------------------------------------- */

/* B, top function */
#define ABC_B_COEF_AND_MASK U32C(0xfffffffc) /* Mask making coefs 0 (mod 4)  */
#define ABC_B_COEF_OR_MASK U32C(0x00000001)  /* Mask making coefs 1 (mod 2)  */

/* ------------------------------------------------------------------------- */

/* C, filter function */
#define ABC_C_COEF_NUM 33               /* Number of coefs                   */
#define ABC_C_AND_MASK U32C(0xffff0000) /* AND mask for 31st coefficient     */
#define ABC_C_OR_MASK  U32C(0x00010000) /* OR mask for 31st coefficient      */

#endif /* ABC_H__ */
