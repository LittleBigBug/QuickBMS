// modified by Luigi Auriemma
/********************************************************************\
*                                                                    *
* C specification of the threeway block cipher                       *
*                                                                    *
\********************************************************************/

#include <stdlib.h>
#include <stdint.h>

#define   STRT_E   0x0b0b /* round constant of first encryption round */ 
#define   STRT_D   0xb1b1 /* round constant of first decryption round */
#define     NMBR       11 /* number of rounds is 11                   */

//#ifdef __alpha  /* Any other 64-bit machines? */
typedef uint32_t word32;
//#else
//typedef unsigned long word32;
//#endif

/* the program only works correctly if long = 32bits */

static void mu(word32 *a)       /* inverts the order of the bits of a */
{
int i ;
word32 b[3] ;

b[0] = b[1] = b[2] = 0 ;
for( i=0 ; i<32 ; i++ )
   {
   b[0] <<= 1 ; b[1] <<= 1 ; b[2] <<= 1 ;
   if(a[0]&1) b[2] |= 1 ;
   if(a[1]&1) b[1] |= 1 ;
   if(a[2]&1) b[0] |= 1 ;
   a[0] >>= 1 ; a[1] >>= 1 ; a[2] >>= 1 ;
   }

a[0] = b[0] ; a[1] = b[1] ; a[2] = b[2] ;
}

static void way3_gamma(word32 *a)   /* the nonlinear step */
{
word32 b[3] ;

b[0] = a[0] ^ (a[1]|(~a[2])) ; 
b[1] = a[1] ^ (a[2]|(~a[0])) ; 
b[2] = a[2] ^ (a[0]|(~a[1])) ; 

a[0] = b[0] ; a[1] = b[1] ; a[2] = b[2] ;
}


static void theta(word32 *a)    /* the linear step */
{
word32 b[3];

b[0] = a[0] ^  (a[0]>>16) ^ (a[1]<<16) ^     (a[1]>>16) ^ (a[2]<<16) ^
               (a[1]>>24) ^ (a[2]<<8)  ^     (a[2]>>8)  ^ (a[0]<<24) ^
               (a[2]>>16) ^ (a[0]<<16) ^     (a[2]>>24) ^ (a[0]<<8)  ;
b[1] = a[1] ^  (a[1]>>16) ^ (a[2]<<16) ^     (a[2]>>16) ^ (a[0]<<16) ^
               (a[2]>>24) ^ (a[0]<<8)  ^     (a[0]>>8)  ^ (a[1]<<24) ^
               (a[0]>>16) ^ (a[1]<<16) ^     (a[0]>>24) ^ (a[1]<<8)  ;
b[2] = a[2] ^  (a[2]>>16) ^ (a[0]<<16) ^     (a[0]>>16) ^ (a[1]<<16) ^
               (a[0]>>24) ^ (a[1]<<8)  ^     (a[1]>>8)  ^ (a[2]<<24) ^
               (a[1]>>16) ^ (a[2]<<16) ^     (a[1]>>24) ^ (a[2]<<8)  ;

a[0] = b[0] ;      a[1] = b[1] ;      a[2] = b[2] ;
}

static void pi_1(word32 *a)   
{
a[0] = (a[0]>>10) ^ (a[0]<<22);  
a[2] = (a[2]<<1)  ^ (a[2]>>31);
}

static void pi_2(word32 *a)   
{
a[0] = (a[0]<<1)  ^ (a[0]>>31);
a[2] = (a[2]>>10) ^ (a[2]<<22);
}

static void rho(word32 *a)    /* the round function       */
{
theta(a) ; 
pi_1(a) ; 
way3_gamma(a) ; 
pi_2(a) ;
}

static void rndcon_gen(word32 strt,word32 *rtab)
{                           /* generates the round constants */
int i ;

for(i=0 ; i<=NMBR ; i++ )
   {
   rtab[i] = strt ;
   strt <<= 1 ; 
   if( strt&0x10000 ) strt ^= 0x11011 ;
   }
}

void threeway_encrypt_block(word32 *a, word32 *k)
{
int i ;
word32 rcon[NMBR+1] ;

rndcon_gen(STRT_E,rcon) ; 
for( i=0 ; i<NMBR ; i++ )   
   {
   a[0] ^= k[0] ^ (rcon[i]<<16) ; 
   a[1] ^= k[1] ; 
   a[2] ^= k[2] ^ rcon[i] ;
   rho(a) ;
   }
a[0] ^= k[0] ^ (rcon[NMBR]<<16) ; 
a[1] ^= k[1] ; 
a[2] ^= k[2] ^ rcon[NMBR] ;
theta(a) ;
}


void threeway_decrypt_block(word32 *a, word32 *k)
{             
int i ;
word32 ki[3] ;          /* the `inverse' key             */
word32 rcon[NMBR+1] ;   /* the `inverse' round constants */

ki[0] = k[0] ; ki[1] = k[1] ; ki[2] = k[2] ; 
theta(ki) ;
mu(ki) ;

rndcon_gen(STRT_D,rcon) ; 

mu(a) ;
for( i=0 ; i<NMBR ; i++ )
   {
   a[0] ^= ki[0] ^ (rcon[i]<<16) ; 
   a[1] ^= ki[1] ; 
   a[2] ^= ki[2] ^ rcon[i] ;
   rho(a) ;
   }
a[0] ^= ki[0] ^ (rcon[NMBR]<<16) ; 
a[1] ^= ki[1] ; 
a[2] ^= ki[2] ^ rcon[NMBR] ;
theta(a) ;
mu(a) ;
}



int threeway_setkey(word32 *key, unsigned char *data, int datalen) {
    if(datalen < 12) return(-1);
    key[0] = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);  data += 4;
    key[1] = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);  data += 4;
    key[2] = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);  data += 4;
    return(0);
}

void threeway_encrypt(word32 *key, unsigned char *data, int datalen) {
    for(datalen /= 12; datalen; datalen--) {
        threeway_encrypt_block((word32 *)data, key);
        data += 12;
    }
}

void threeway_decrypt(word32 *key, unsigned char *data, int datalen) {
    for(datalen /= 12; datalen; datalen--) {
        threeway_decrypt_block((word32 *)data, key);
        data += 12;
    }
}

