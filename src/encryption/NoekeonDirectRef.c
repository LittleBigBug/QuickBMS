// modified by Luigi Auriemma
/*************************************************************************************/
/* NoekeonDirectRef.c
 *
 * Last Modified: 00/09/26             Created: 00/08/30
 *
 * Project    : Nessie Proposal: NESSIE
 *
 * Authors    : Joan Daemen, Michael Peeters, Vincent Rijmen, Gilles Van Assche
 *
 * Written by : Michael Peeters
 *
 * References : [NESSIE] see http://cryptonessie.org/ for information about 
 *                       interface conventions and definition of portable C.
 *
 * Description: Reference implementation of NESSIE interfaces for the block cipher 
 *              NESSIE in DIRECT-KEY MODE
 *
 *              This code is NOT optimised. rather clarity was favorised
 *
 *   The text vectors should be as follows:
 * 
 *                      k = 00000000 00000000 00000000 00000000
 *                      a = 00000000 00000000 00000000 00000000
 * after NOEKEONencrypt, b = b1656851 699e29fa 24b70148 503d2dfc
 * after NOEKEONdecrypt, a?= 00000000 00000000 00000000 00000000
 * 
 *                      k = ffffffff ffffffff ffffffff ffffffff
 *                      a = ffffffff ffffffff ffffffff ffffffff
 * after NOEKEONencrypt, b = 2a78421b 87c7d092 4f26113f 1d1349b2
 * after NOEKEONdecrypt, a?= ffffffff ffffffff ffffffff ffffffff
 * 
 *                      k = b1656851 699e29fa 24b70148 503d2dfc
 *                      a = 2a78421b 87c7d092 4f26113f 1d1349b2
 * after NOEKEONencrypt, b = e2f687e0 7b75660f fc372233 bc47532c
 * after NOEKEONdecrypt, a?= 2a78421b 87c7d092 4f26113f 1d1349b2
 *
*************************************************************************************/
 
//#include "Nessie.h"
/* Definition of minimum-width integer types
 * 
 * u8   -> unsigned integer type, at least 8 bits, equivalent to unsigned char
 * u16  -> unsigned integer type, at least 16 bits
 * u32  -> unsigned integer type, at least 32 bits
 *
 * s8, s16, s32  -> signed counterparts of u8, u16, u32
 *
 * Always use macro's T8(), T16() or T32() to obtain exact-width results,
 * i.e., to specify the size of the result of each expression.
 */

typedef signed char s8;
typedef unsigned char u8;

#if UINT_MAX >= 4294967295UL

typedef signed short s16;
typedef signed int s32;
typedef unsigned short u16;
typedef unsigned int u32;

#define ONE32   0xffffffffU

#else

typedef signed int s16;
typedef signed long s32;
typedef unsigned int u16;
typedef unsigned long u32;

#define ONE32   0xffffffffUL

#endif

#define ONE8    0xffU
#define ONE16   0xffffU

#define T8(x)   ((x) & ONE8)
#define T16(x)  ((x) & ONE16)
#define T32(x)  ((x) & ONE32)

/*
 * If you want 64-bit values, uncomment the following lines; this
 * reduces portability.
 */
/*
  #if ((1UL << 31) * 2UL) != 0UL
  typedef unsigned long u64;
  typedef signed long s64;
  #define ONE64   0xffffffffffffffffUL
  #else
  typedef unsigned long long u64;
  typedef signed long long s64;
  #define ONE64   0xffffffffffffffffULL
  #endif
  #define T64(x)  ((x) & ONE64)
*/
/*
 * Note: the test is used to detect native 64-bit architectures;
 * if the unsigned long is strictly greater than 32-bit, it is
 * assumed to be at least 64-bit. This will not work correctly
 * on (old) 36-bit architectures (PDP-11 for instance).
 *
 * On non-64-bit architectures, "long long" is used.
 */

/*
 * U8TO32_BIG(c) returns the 32-bit value stored in big-endian convention
 * in the unsigned char array pointed to by c.
 */
#define U8TO32_BIG(c)  (((u32)T8(*(c)) << 24) | ((u32)T8(*((c) + 1)) << 16) |\
                       ((u32)T8(*((c) + 2)) << 8) | ((u32)T8(*((c) + 3))))

/*
 * U8TO32_LITTLE(c) returns the 32-bit value stored in little-endian convention
 * in the unsigned char array pointed to by c.
 */
#define U8TO32_LITTLE(c)  (((u32)T8(*(c))) | ((u32)T8(*((c) + 1)) << 8) |\
                      (u32)T8(*((c) + 2)) << 16) | ((u32)T8(*((c) + 3)) << 24))

/*
 * U8TO32_BIG(c, v) stores the 32-bit-value v in big-endian convention
 * into the unsigned char array pointed to by c.
 */
#define U32TO8_BIG(c, v)    do { \
		u32 x = (v); \
		u8 *d = (c); \
		d[0] = T8(x >> 24); \
		d[1] = T8(x >> 16); \
		d[2] = T8(x >> 8); \
		d[3] = T8(x); \
	} while (0)

/*
 * U8TO32_LITTLE(c, v) stores the 32-bit-value v in little-endian convention
 * into the unsigned char array pointed to by c.
 */
#define U32TO8_LITTLE(c, v)    do { \
		u32 x = (v); \
		u8 *d = (c); \
		d[0] = T8(x); \
		d[1] = T8(x >> 8); \
		d[2] = T8(x >> 16); \
		d[3] = T8(x >> 24); \
	} while (0)

/*
 * ROTL32(v, n) returns the value of the 32-bit unsigned value v after
 * a rotation of n bits to the left. It might be replaced by the appropriate
 * architecture-specific macro.
 *
 * It evaluates v and n twice.
 *
 * The compiler might emit a warning if n is the constant 0. The result
 * is undefined if n is greater than 31.
 */
#define ROTL32(v, n)   (T32((v) << (n)) | ((v) >> (32 - (n))))


/*************************************************************************************/
/* 
 * The following deals with Noekeon data structures
 *
*************************************************************************************/

typedef struct {
  u32 k[4];
} NOEKEONstruct;

/*==================================================================================*/
/* Number of computation rounds in the block cipher
 *----------------------------------------------------------------------------------*/
#define   NROUND		16	
/*----------------------------------------------------------------------------------*/
/* Round Constants are : 80,1B,36,6C,D8,AB,4D,9A,2F,5E,BC,63,C6,97,35,6A,D4 (encrypt)
 *----------------------------------------------------------------------------------*/
#define RC1ENCRYPTSTART  T8 (0x80)
#define RC2DECRYPTSTART  T8 (0xD4) 


/*==================================================================================*/
/* Null Vector
 *----------------------------------------------------------------------------------*/
u32 NullVector[4] = {0,0,0,0};


/*==================================================================================*/
void Theta (u32 const * const k,u32 * const a)
/*----------------------------------------------------------------------------------*/
/* DIFFUSION - Linear step THETA, involution
 *==================================================================================*/
{
  u32 tmp;

  tmp  = a[0]^a[2]; 
  tmp ^= ROTL32(tmp,8)^ROTL32(tmp,24); 
  a[1]^= tmp; 
  a[3]^= tmp; 

  a[0] ^= k[0]; a[1] ^= k[1]; a[2] ^= k[2]; a[3] ^= k[3]; 

  tmp  = a[1]^a[3]; 
  tmp ^= ROTL32(tmp,8)^ROTL32(tmp,24); 
  a[0]^= tmp; 
  a[2]^= tmp; 

} /* Theta */


/*==================================================================================*/
void Pi1(u32 * const a)
/*----------------------------------------------------------------------------------*/
/* DISPERSION - Rotations Pi1
 *==================================================================================*/
{ a[1] = ROTL32 (a[1], 1); 
  a[2] = ROTL32 (a[2], 5); 
  a[3] = ROTL32 (a[3], 2); 
}  /* Pi1 */


/*==================================================================================*/
void Pi2(u32 * const a)
/*----------------------------------------------------------------------------------*/
/* DISPERSION - Rotations Pi2
 *==================================================================================*/
{ a[1] = ROTL32 (a[1], 31);
  a[2] = ROTL32 (a[2], 27); 
  a[3] = ROTL32 (a[3], 30); 
}  /* Pi2 */


/*==================================================================================*/
void Gamma(u32 * const a)
/*----------------------------------------------------------------------------------*/
/* NONLINEAR - gamma, involution
 *----------------------------------------------------------------------------------*/
/* Input of i_th s-box = (i3)(i2)(i1)(i0), with (i3) = i_th bit of a[3]
 *                                              (i2) = i_th bit of a[2]
 *                                              (i1) = i_th bit of a[1]
 *                                              (i0) = i_th bit of a[0]
 *
 * gamma = NLIN o LIN o NLIN : (i3)(i2)(i1)(i0) --> (o3)(o2)(o1)(o0)
 *
 * NLIN ((i3) = (o3) = (i3)                     NLIN is an involution
 *       (i2)   (o2)   (i2)                      i.e. evaluation order of i1 & i0
 *       (i1)   (o1)   (i1+(~i3.~i2))                 can be swapped
 *       (i0))  (o0)   (i0+(i2.i1))
 * 
 *  LIN ((i3) = (o3) = (0.i3+0.i2+0.i1+  i0)    LIN is an involution
 *       (i2)   (o2)   (  i3+  i2+  i1+  i0)    
 *       (i1)   (o1)   (0.i3+0.i2+  i1+0.i0)    
 *       (i0))  (o0)   (  i3+0.i2+0.i1+0.i0)    
 *
 *==================================================================================*/
{ u32 tmp;

  /* first non-linear step in gamma */
  a[1] ^= ~a[3] & ~a[2];
  a[0] ^=   a[2] & a[1];

  /* linear step in gamma */
  tmp   = a[3];
  a[3]  = a[0];
  a[0]  = tmp;
  a[2] ^= a[0]^a[1]^a[3];

  /* last non-linear step in gamma */
  a[1] ^= ~a[3] & ~a[2];
  a[0] ^=   a[2] & a[1];
} /* Gamma */


/*==================================================================================*/
void Round (u32 const * const k,u32 * const a,u8 const RC1,u8 const RC2)
/*----------------------------------------------------------------------------------*/
/* The round function, common to both encryption and decryption
 * - Round constants is added to the rightmost byte of the leftmost 32-bit word (=a0)
 *==================================================================================*/
{ 
  a[0] ^= RC1;
  Theta(k,a); 
  a[0] ^= RC2;
  Pi1(a); 
  Gamma(a); 
  Pi2(a); 
}  /* Round */

/*==================================================================================*/
void RCShiftRegFwd (u8 * const RC)
/*----------------------------------------------------------------------------------*/
/* The shift register that computes round constants - Forward Shift
 *==================================================================================*/
{ 

  if ((*RC)&0x80) (*RC)=((*RC)<<1) ^ 0x1B; else (*RC)<<=1;
  
} /* RCShiftRegFwd */

/*==================================================================================*/
void RCShiftRegBwd (u8 * const RC)
/*----------------------------------------------------------------------------------*/
/* The shift register that computes round constants - Backward Shift
 *==================================================================================*/
{ 

  if ((*RC)&0x01) (*RC)=((*RC)>>1) ^ 0x8D; else (*RC)>>=1;
  
} /* RCShiftRegBwd */

/*==================================================================================*/
void CommonLoop (u32 const * const k,u32 * const a, u8 RC1, u8 RC2)
/*----------------------------------------------------------------------------------*/
/* loop - several round functions, ended by theta
 *==================================================================================*/
{ 
  unsigned i;

  for(i=0 ; i<NROUND ; i++) {
    Round(k,a,RC1,RC2); 
    RCShiftRegFwd(&RC1);
    RCShiftRegBwd(&RC2);
  }
  a[0]^=RC1;
  Theta(k,a); 
  a[0]^=RC2;

} /* CommonLoop */


/*==================================================================================*/
void NOEKEONencrypt(const NOEKEONstruct * const structpointer, 
                   const unsigned char * const plaintext,
                   unsigned char * const ciphertext)
/*==================================================================================*/
{ u32 const *k=structpointer->k;
  u32 state[4];
  

  state[0]=U8TO32_BIG(plaintext   );
  state[1]=U8TO32_BIG(plaintext+4 );
  state[2]=U8TO32_BIG(plaintext+8 );
  state[3]=U8TO32_BIG(plaintext+12);

  CommonLoop (k,state,RC1ENCRYPTSTART,0);
  
  U32TO8_BIG(ciphertext   , state[0]);
  U32TO8_BIG(ciphertext+4 , state[1]);
  U32TO8_BIG(ciphertext+8 , state[2]);
  U32TO8_BIG(ciphertext+12, state[3]);
} /* NOEKEONencrypt */

/*==================================================================================*/
void NOEKEONdecrypt(const NOEKEONstruct * const structpointer,
                   const unsigned char * const ciphertext,
                   unsigned char * const plaintext)
/*==================================================================================*/
{ u32 const *kencrypt=structpointer->k;
  u32 k[4],state[4];

  state[0]=U8TO32_BIG(ciphertext   );
  state[1]=U8TO32_BIG(ciphertext+4 );
  state[2]=U8TO32_BIG(ciphertext+8 );
  state[3]=U8TO32_BIG(ciphertext+12);

  k[0]=kencrypt[0];
  k[1]=kencrypt[1];
  k[2]=kencrypt[2];
  k[3]=kencrypt[3];
  Theta(NullVector,k);

  CommonLoop (k,state,0,RC2DECRYPTSTART);

  U32TO8_BIG(plaintext   , state[0]);
  U32TO8_BIG(plaintext+4 , state[1]);
  U32TO8_BIG(plaintext+8 , state[2]);
  U32TO8_BIG(plaintext+12, state[3]);
} /* NOEKEONdecrypt */


/*==================================================================================*/
void NOEKEONkeysetup(const unsigned char * const key, 
                    NOEKEONstruct * const structpointer)
/*----------------------------------------------------------------------------------*/
/* PRE:
 * 128-bit key value in byte array key [16 bytes]
 *
 * key: [00] [01] [02] [03] [04] [05] [06] [07] [08] [09] [10] [11] [12] [13] [14] [15]
 *      ----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----
 *
 * POST:
 * key value written in 32-bit word array k in NOEKEONstruct
 *      -------------------+-------------------+-------------------+-------------------
 *              k[0]                k[1]                k[2]                k[3]
 *==================================================================================*/
{ u32 *k=structpointer->k;
  
  k[0]=U8TO32_BIG(key   );
  k[1]=U8TO32_BIG(key+4 );
  k[2]=U8TO32_BIG(key+8 );
  k[3]=U8TO32_BIG(key+12);

} /* NOEKEONkeysetup */








