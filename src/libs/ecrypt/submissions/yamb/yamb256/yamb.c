// modified by Luigi Auriemma
/* yamb.c */


#include "ecrypt-sync.h"
#undef _M_IX86
#undef _MSC_VER
#undef __INTEL_COMPILER


#define ALF 0x091B17C9


#define stepOLZ16()\
{\
   abcd = ( ( ( ( s32 )OLZ[ I0 ] ) >> 31 ) & ALF ) ^ ( OLZ[ I0 ] + OLZ[ I0 ] ) ^ OLZ[ I1 ];\
   OLZ[ I2 ] = abcd;\
   I0++;    I0 &= 0xF;\
   I1++;    I1 &= 0xF;\
   I2++;    I2 &= 0xF;\
}


#define stepOLZ64()\
{\
   abcd = ( ( ( ( s32 )OLZ[ I0 ] ) >> 31 ) & ALF ) ^ ( OLZ[ I0 ] + OLZ[ I0 ] ) ^ OLZ[ I1 ];\
   OLZ[ I2 ] = abcd;\
   I0++;    I0 &= 0x3F;\
   I1++;    I1 &= 0x3F;\
   I2++;    I2 &= 0x3F;\
}


#define stepUU()\
{\
   a = ( u8 )abcd;\
   b = ( u8 )( abcd >> 8 );\
   c = ( u8 )( abcd >> 16 );\
   d = ( u8 )( abcd >> 24 );\
   b ^= M[ a ];   M[ a ] += d;\
   c ^= M[ d ];   M[ d ] += b;\
   a ^= M[ b ];   M[ b ] += c;\
   d ^= M[ c ];   M[ c ] += a;\
   c ^= M[ a ];   M[ a ] += d;\
   b ^= M[ d ];   M[ d ] += c;\
   a ^= M[ c ];   M[ c ] += b;\
   d ^= M[ b ];   M[ b ] += a;\
   b ^= M[ a ];   M[ a ] += d;\
   c ^= M[ d ];   M[ d ] += b;\
   a ^= M[ b ];   M[ b ] += c;\
   d ^= M[ c ];   M[ c ] += a;\
   ABCD = ( u32 )a | ( ( ( u32 )b ) << 8 ) | ( ( ( u32 )c ) << 16 ) | ( ( ( u32 )d ) << 24 );\
   ABCD ^= abcd;\
}


void ECRYPT_init( void )
{
}


void ECRYPT_keysetup(
   ECRYPT_ctx* ctx,
   const u8*   key,
   u32         keysize,
   u32         ivsize )
{
   u8 i;
   
   ctx->lenkey = keysize / 8;
   ctx->leniv  = ivsize / 8;

   for( i = 0; i < ctx->lenkey; i++ )
      ctx->key[ i ] = key[ i ];
}


void ECRYPT_ivsetup(
   ECRYPT_ctx* ctx, 
   const u8*   iv )
{
   static const u8   ConstM[ 256 ] =
   {
      0x85, 0x2D, 0x43, 0x2F, 0xA6, 0x90, 0xF8, 0x1B, 0xA9, 0xB4, 0x1C, 0x58, 0xE8, 0xA5, 0xD7, 0x56,
      0x6B, 0x03, 0x38, 0x67, 0x3D, 0xB1, 0x7B, 0x0B, 0xF2, 0xCB, 0x29, 0xFC, 0x53, 0x75, 0x05, 0xCA,
      0x0E, 0xAE, 0xD1, 0x9C, 0xBC, 0xB0, 0xDF, 0x62, 0xCF, 0x3A, 0xFE, 0xDC, 0x20, 0x83, 0x88, 0x68,
      0x41, 0x24, 0x45, 0x01, 0x91, 0xF0, 0xA0, 0xF6, 0x65, 0x02, 0xB5, 0xBE, 0x0F, 0xE5, 0x13, 0xD4,
      0xBF, 0xA4, 0x86, 0x4A, 0x63, 0x82, 0x4B, 0xAC, 0x9E, 0xE2, 0x7F, 0x50, 0x6C, 0xEC, 0x31, 0x44,
      0x09, 0x94, 0x9A, 0x40, 0x06, 0xC3, 0x37, 0xF4, 0x2A, 0x57, 0x7C, 0x25, 0x99, 0xFA, 0x21, 0x3B,
      0xEE, 0x54, 0x3C, 0x22, 0xB8, 0xEB, 0x51, 0x8C, 0x87, 0x66, 0x10, 0x27, 0x6D, 0xAA, 0xCE, 0x39,
      0xE0, 0xBD, 0x8B, 0x9B, 0x69, 0xB6, 0xE7, 0x36, 0xAF, 0xDE, 0x34, 0x93, 0x9F, 0xA2, 0x60, 0x14,
      0x7D, 0xA7, 0x8D, 0x7E, 0x76, 0x48, 0x72, 0x74, 0x23, 0xCD, 0x73, 0xD9, 0x33, 0xD6, 0xB2, 0x78,
      0x9D, 0x3F, 0x32, 0x8E, 0xED, 0x5B, 0x2B, 0x4F, 0xD3, 0xE9, 0x1E, 0x4C, 0x16, 0x4E, 0xB3, 0xC5,
      0xD8, 0xF3, 0x2E, 0x26, 0x28, 0x8A, 0x12, 0x64, 0xFB, 0xA3, 0xFF, 0xAD, 0xE1, 0xB7, 0x1A, 0xD0,
      0xF1, 0xBA, 0x7A, 0xA1, 0x00, 0xD2, 0xE4, 0xC6, 0xC0, 0x30, 0x81, 0x52, 0x92, 0x46, 0x61, 0xC1,
      0x95, 0x1F, 0x2C, 0xC2, 0x4D, 0x42, 0x49, 0x07, 0x5A, 0xFD, 0x0C, 0x70, 0xCC, 0x84, 0xF9, 0xD5,
      0x5E, 0x18, 0xB9, 0x5D, 0xC9, 0x5C, 0xC4, 0x1D, 0x6E, 0x35, 0x59, 0xDB, 0x15, 0x79, 0xDD, 0xE6,
      0xDA, 0xA8, 0x89, 0x80, 0x98, 0x5F, 0xEF, 0x96, 0x19, 0xF7, 0xC7, 0x3E, 0x47, 0x0D, 0x71, 0xEA,
      0x04, 0xBB, 0x55, 0x77, 0xC8, 0x0A, 0x17, 0x97, 0xAB, 0x8F, 0x11, 0x08, 0xE3, 0x6F, 0xF5, 0x6A
   };

   static const u8   ConstOLZ[ 9 ] =
   { 
      0x4C, 0x41, 0x4E, 0x43, 0x72, 0x79, 0x70, 0x74, 0x6F
   };

   u32   i;
   u8    a, b, c, d;
   u32   abcd, ABCD;
   u8    *M, *pu8;
   u32   *OLZ, *RZ;
   u8    I0, I1, I2, J;

   M     = ctx->M; 
   OLZ   = ctx->OLZ; 
   RZ    = ctx->RZ;
   pu8   = ( u8* )OLZ;

   for( i = 0; i < 256; i++ )
      M[ i ] = ConstM[ i ];
 
   for( i = 0; i < 32; i++ )
      pu8[ i ] = ctx->key[ i % ctx->lenkey ];
   
   for( i = 32; i < ( u32 )( 32 + ctx->leniv ); i++ )
      pu8[ i ] = iv[ i - 32 ];
   
   for( i = 32 + ctx->leniv; i < 60; i++ )
      pu8[ i ] = ConstOLZ[ ( i - 32 - ctx->leniv ) % 9 ];

   for( i = 0; i < 15; i++ )  
      OLZ[ i ] = ( ( u32 )pu8[ i * 4 ] ) | 
                 ( ( ( u32 )pu8[ i * 4 + 1 ] ) << 8 ) | 
                 ( ( ( u32 )pu8[ i * 4 + 2 ] ) << 16 ) |
                 ( ( ( u32 )pu8[ i * 4 + 3 ] ) << 24);

   I0 = 0;
   I1 = 8;
   I2 = 15;
   J  = 0;
   
   for( i = 0; i < 225; i++ )
   { 
      stepOLZ16();
      stepUU();
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   } 

   for( i = 0; i < 64; i++ )
   {
      stepOLZ16();
      stepUU();
      abcd = ( ABCD + RZ[ J ] );
      M[ i * 4 ]     ^= ( u8 )abcd;
      M[ i * 4 + 1 ] ^= ( u8 )( abcd >> 8 );
      M[ i * 4 + 2 ] ^= ( u8 )( abcd >> 16 );
      M[ i * 4 + 3 ] ^= ( u8 )( abcd >> 24 );
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   }

   for( i = 0; i < 15; i++ )
   {
      stepOLZ16();
      stepUU();
      RZ[ J ] += ABCD;
      J++;
      J &= 0xF;
   }

   for( i = 1; i < 16; i++ )
      OLZ[ i + 32 ] = RZ[ i ];

   I0 = 33;
   I1 = 41;
   I2 = 48;
   J  = 0;
   
   for( i = 0; i < 16; i++ )
   {
      stepOLZ64();
      stepUU();
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   }

   for( i = 0; i < 64; i++ )
       stepOLZ64();
}


void ECRYPT_process_bytes(
   int         action,
   ECRYPT_ctx* ctx,
   const u8*   input,
   u8*         output,
   u32         msglen )
{
   u8 remsize;
   u8 remainder[ ECRYPT_BLOCKLENGTH ];

   ECRYPT_process_blocks( action, ctx, input, output, ( msglen / ECRYPT_BLOCKLENGTH ) );

   if( (remsize = ( msglen % ECRYPT_BLOCKLENGTH )) )
   {
     u32 i;

      ECRYPT_keystream_blocks( ctx, remainder, 1 );
      for( i = ( msglen - remsize ); i < msglen; i++ )
         output[ i ] = input[ i ] ^ remainder[ i - ( msglen - remsize ) ];
   }
}


void ECRYPT_keystream_bytes(
   ECRYPT_ctx* ctx,
   u8*         keystream,
   u32         length )
{
   u8 remsize, i;
   u8 remainder[ ECRYPT_BLOCKLENGTH ];
   
   ECRYPT_keystream_blocks( ctx, keystream, ( length / ECRYPT_BLOCKLENGTH ) );
   
   if( (remsize = ( length % ECRYPT_BLOCKLENGTH )) )
   {
      ECRYPT_keystream_blocks( ctx, remainder, 1 );
      for( i = ( length - remsize ); i < length; i++ )
         keystream[ i ] = remainder[ i - ( length - remsize ) ];
   }
}


void ECRYPT_process_blocks(
   int         action,
   ECRYPT_ctx* ctx, 
   const u8*   input, 
   u8*         output, 
   u32         blocks )
{
   u32 i;

   ECRYPT_keystream_blocks( ctx, output, blocks );

   for( i = 0; i < ( blocks * ECRYPT_BLOCKLENGTH ); i++ )
      output[ i ] ^= input[ i ];
}


/* x86 */
#if( defined( _M_IX86 ) || defined( _MSC_VER ) || defined( __INTEL_COMPILER ) || defined( i386 ) || defined( __i386 ) )


#define  stepR( I )\
{\
   ABCD = BUF[ I ] ^ OLZ[ I ];\
   *( ( u32* )keystream + I ) = ( ABCD + RZ[ I & 0xF ] );\
   RZ[ I & 0xF ] = ABCD;\
   OLZ[ I & 0x3F ] = \
   ( ( ( ( s32 )OLZ[ ( I + 49 ) & 0x3F ] ) >> 31 ) & ALF ) ^ \
   ( OLZ[ ( I + 49 ) & 0x3F ] + OLZ[ ( I + 49 ) & 0x3F ] ) ^ OLZ[ ( I + 57 ) & 0x3F ];\
}


/* x86: Intel, MS, Borland */
#if( defined( _M_IX86 ) || defined( _MSC_VER ) || defined( __INTEL_COMPILER ) )


#define  stepasmU( Rb, Ra, Rd ) \
{\
	_asm{ movzx eax, byte ptr [ esi + Ra ] };\
	_asm{ xor Rb, eax };\
	_asm{ lea eax, [ eax+ Rd ] };\
	_asm{ mov byte ptr [ esi + Ra ], al };\
}


#define  stepU( I )\
{\
	_asm{ movzx edi, [ esi + 256 + 4 * I ] };\
	_asm{ movzx ebx, [ esi + 256 + 4 * I + 1 ] };\
	_asm{ movzx ecx, [ esi + 256 + 4 * I + 2 ] };\
	_asm{ movzx edx, [ esi + 256 + 4 * I + 3 ] };\
	stepasmU( ebx, edi, edx );\
	stepasmU( ecx, edx, ebx );\
	stepasmU( edi, ebx, ecx );\
	stepasmU( edx, ecx, edi );\
	stepasmU( ecx, edi, edx );\
	stepasmU( ebx, edx, ecx );\
	stepasmU( edi, ecx, ebx );\
	stepasmU( edx, ebx, edi );\
	stepasmU( ebx, edi, edx );\
	stepasmU( ecx, edx, ebx );\
	stepasmU( edi, ebx, ecx );\
	stepasmU( edx, ecx, edi );\
	_asm{ mov eax, edi };\
	_asm{ mov byte ptr BUF[ I * 4 ], al };\
	_asm{ mov byte ptr BUF[ I * 4 + 1 ], bl };\
	_asm{ mov byte ptr BUF[ I * 4 + 2 ], cl };\
	_asm{ mov byte ptr BUF[ I * 4 + 3 ], dl };\
}


/* x86: gcc */
#elif( defined( i386 ) || defined( __i386 ) )


#define  stepU( I )\
{\
   asm(  "movzx %c0( %%esi ), %%edi"\
         : : "n" ( 256 + 4 * I ) );\
   asm(  "movzx %c0( %%esi ), %%ebx"\
         : : "n" ( 256 + 4 * I + 1 ) );\
   asm(  "movzx %c0( %%esi ), %%ecx"\
         : : "n" ( 256 + 4 * I + 2 ) );\
   asm(  "movzx %c0( %%esi ), %%edx"\
         : : "n" ( 256 + 4 * I + 3 ) );\
   asm(  "movzb ( %esi, %edi ), %eax\n"\
         "xor %eax, %ebx\n"\
         "lea ( %eax, %edx ), %eax\n"\
         "movb %al, ( %esi, %edi )\n" );\
   asm(  "movzb ( %esi, %edx ), %eax\n"\
         "xor %eax, %ecx\n"\
         "lea ( %eax, %ebx ), %eax\n"\
         "movb %al, ( %esi, %edx )\n" );\
   asm(  "movzb ( %esi, %ebx ), %eax\n"\
         "xor %eax, %edi\n"\
         "lea ( %eax, %ecx ), %eax\n"\
         "movb %al, ( %esi, %ebx )\n" );\
   asm(  "movzb ( %esi, %ecx ), %eax\n"\
         "xor %eax, %edx\n"\
         "lea ( %eax, %edi ), %eax\n"\
         "movb %al, ( %esi, %ecx )\n" );\
   asm(  "movzb ( %esi, %edi ), %eax\n"\
         "xor %eax, %ecx\n"\
         "lea ( %eax, %edx ), %eax\n"\
         "movb %al, ( %esi, %edi )\n" );\
   asm(  "movzb ( %esi, %edx ), %eax\n"\
         "xor %eax, %ebx\n"\
         "lea ( %eax, %ecx ), %eax\n"\
         "movb %al, ( %esi, %edx )\n" );\
   asm(  "movzb ( %esi, %ecx ), %eax\n"\
         "xor %eax, %edi\n"\
         "lea ( %eax, %ebx ), %eax\n"\
         "movb %al, ( %esi, %ecx )\n" );\
   asm(  "movzb ( %esi, %ebx ), %eax\n"\
         "xor %eax, %edx\n"\
         "lea ( %eax, %edi ), %eax\n"\
         "movb %al, ( %esi, %ebx )\n" );\
   asm(  "movzb ( %esi, %edi ), %eax\n"\
         "xor %eax, %ebx\n"\
         "lea ( %eax, %edx ), %eax\n"\
         "movb %al, ( %esi, %edi )\n" );\
   asm(  "movzb ( %esi, %edx ), %eax\n"\
         "xor %eax, %ecx\n"\
         "lea ( %eax, %ebx ), %eax\n"\
         "movb %al, ( %esi, %edx )\n" );\
   asm(  "movzb ( %esi, %ebx ), %eax\n"\
         "xor %eax, %edi\n"\
         "lea ( %eax, %ecx ), %eax\n"\
         "movb %al, ( %esi, %ebx )\n" );\
   asm(  "movzb ( %esi, %ecx ), %eax\n"\
         "xor %eax, %edx\n"\
         "lea ( %eax, %edi ), %eax\n"\
         "movb %al, ( %esi, %ecx )\n" );\
   asm(  "movl %edi, %eax\n" );\
   asm(  "movb %%al, %0\n"\
         : "=m" ( ( ( u8* )( &BUF ) )[ I * 4 ] ) : );\
   asm(  "movb %%bl, %0\n"\
         : "=m" ( ( ( u8* )( &BUF ) )[ I * 4 + 1 ] ) : );\
   asm(  "movb %%cl, %0\n"\
         : "=m" ( ( ( u8* )( &BUF ) )[ I * 4 + 2 ] ) : );\
   asm(  "movb %%dl, %0\n"\
         : "=m" ( ( ( u8* )( &BUF ) )[ I * 4 + 3 ] ) : );\
}


#endif


/* others */
#else


#define  stepU( I )\
{\
   a = ( u8 )OLZ[ I ];\
   b = ( u8 )( OLZ[ I ] >> 8 );\
   c = ( u8 )( OLZ[ I ] >> 16 );\
   d = ( u8 )( OLZ[ I ] >> 24 );\
   b ^= M[ a ];   M[ a ] += d;\
   c ^= M[ d ];   M[ d ] += b;\
   a ^= M[ b ];   M[ b ] += c;\
   d ^= M[ c ];   M[ c ] += a;\
   c ^= M[ a ];   M[ a ] += d;\
   b ^= M[ d ];   M[ d ] += c;\
   a ^= M[ c ];   M[ c ] += b;\
   d ^= M[ b ];   M[ b ] += a;\
   b ^= M[ a ];   M[ a ] += d;\
   c ^= M[ d ];   M[ d ] += b;\
   a ^= M[ b ];   M[ b ] += c;\
   d ^= M[ c ];   M[ c ] += a;\
   *( ( u8* )BUF + I * 4 )       = a;\
   *( ( u8* )BUF + I * 4 + 1 )   = b;\
   *( ( u8* )BUF + I * 4 + 2 )   = c;\
   *( ( u8* )BUF + I * 4 + 3 )   = d;\
}


#define  stepR( I )\
{\
   ABCD =   ( u32 )P[ I * 4 ] | \
   ( ( u32 )P[ I * 4 + 1 ] << 8 ) | \
   ( ( u32 )P[ I * 4 + 2 ] << 16 ) | \
   ( ( u32 )P[ I * 4 + 3 ] << 24);\
   ABCD  ^= OLZ[I];\
   abcd   = ( ABCD + RZ[ I & 0xF ] );\
   keystream[ I * 4 ]      = ( u8 )abcd;\
   keystream[ I * 4 + 1 ]  = ( u8 )( abcd >> 8 );\
   keystream[ I * 4 + 2 ]  = ( u8 )( abcd >> 16 );\
   keystream[ I * 4 + 3 ]  = ( u8 )( abcd >> 24 );\
   RZ[ I & 0xF ]     = ABCD;\
   OLZ[ I & 0x3F ]   = \
   ( ( ( ( s32 )OLZ[ ( I + 49 ) & 0x3F ] ) >> 31 ) & ALF ) ^ \
   ( OLZ[ ( I + 49 ) & 0x3F ] + OLZ[ ( I + 49 ) & 0x3F ] ) ^ OLZ[ ( I + 57 ) & 0x3F ];\
}


#endif


void ECRYPT_keystream_blocks(
   ECRYPT_ctx* ctx,
   u8*         keystream,
   u32         blocks )
{
#ifdef __APPLE__
    exit(1);    // asm not supported
#else
   u32   ABCD;

   u32   BUF[64];

   u8    *M;
   u32   *OLZ, *RZ;

#if( !defined( _M_IX86 ) && !defined( _MSC_VER ) && !defined( __INTEL_COMPILER ) && !defined( i386 ) && !defined( __i386 ) )

   u8    a, b, c, d;
   u32   abcd;
   u8    *P;
   P     = ( u8* )BUF;

#endif

   M     = ctx->M; 
   OLZ   = ctx->OLZ; 
   RZ    = ctx->RZ;

   while( blocks-- )
   {

#if( defined( _M_IX86 ) || defined( _MSC_VER ) || defined( __INTEL_COMPILER ) )

      _asm{ mov esi, ctx };

#elif( defined( i386 ) || defined( __i386 ) )

      asm( "pusha" );
      asm( "movl %0, %%esi" : : "g" ( ctx ) );

#endif

      stepU( 0 );
      stepU( 1 );
      stepU( 2 );
      stepU( 3 );
      stepU( 4 );
      stepU( 5 );
      stepU( 6 );
      stepU( 7 );
      stepU( 8 );
      stepU( 9 );
      stepU( 10 );
      stepU( 11 );
      stepU( 12 );
      stepU( 13 );
      stepU( 14 );
      stepU( 15 );
      stepU( 16 );
      stepU( 17 );
      stepU( 18 );
      stepU( 19 );
      stepU( 20 );
      stepU( 21 );
      stepU( 22 );
      stepU( 23 );
      stepU( 24 );
      stepU( 25 );
      stepU( 26 );
      stepU( 27 );
      stepU( 28 );
      stepU( 29 );
      stepU( 30 );
      stepU( 31 );
      stepU( 32 );
      stepU( 33 );
      stepU( 34 );
      stepU( 35 );
      stepU( 36 );
      stepU( 37 );
      stepU( 38 );
      stepU( 39 );
      stepU( 40 );
      stepU( 41 );
      stepU( 42 );
      stepU( 43 );
      stepU( 44 );
      stepU( 45 );
      stepU( 46 );
      stepU( 47 );
      stepU( 48 );
      stepU( 49 );
      stepU( 50 );
      stepU( 51 );
      stepU( 52 );
      stepU( 53 );
      stepU( 54 );
      stepU( 55 );
      stepU( 56 );
      stepU( 57 );
      stepU( 58 );
      stepU( 59 );
      stepU( 60 );
      stepU( 61 );
      stepU( 62 );
      stepU( 63 );

#if(  defined( i386 ) || defined( __i386 ) )

      asm( "popa" );

#endif

      stepR( 0 );
      stepR( 1 );
      stepR( 2 );
      stepR( 3 );
      stepR( 4 );
      stepR( 5 );
      stepR( 6 );
      stepR( 7 );
      stepR( 8 );
      stepR( 9 );
      stepR( 10 );
      stepR( 11 );
      stepR( 12 );
      stepR( 13 );
      stepR( 14 );
      stepR( 15 );
      stepR( 16 );
      stepR( 17 );
      stepR( 18 );
      stepR( 19 );
      stepR( 20 );
      stepR( 21 );
      stepR( 22 );
      stepR( 23 );
      stepR( 24 );
      stepR( 25 );
      stepR( 26 );
      stepR( 27 );
      stepR( 28 );
      stepR( 29 );
      stepR( 30 );
      stepR( 31 );
      stepR( 32 );
      stepR( 33 );
      stepR( 34 );
      stepR( 35 );
      stepR( 36 );
      stepR( 37 );
      stepR( 38 );
      stepR( 39 );
      stepR( 40 );
      stepR( 41 );
      stepR( 42 );
      stepR( 43 );
      stepR( 44 );
      stepR( 45 );
      stepR( 46 );
      stepR( 47 );
      stepR( 48 );
      stepR( 49 );
      stepR( 50 );
      stepR( 51 );
      stepR( 52 );
      stepR( 53 );
      stepR( 54 );
      stepR( 55 );
      stepR( 56 );
      stepR( 57 );
      stepR( 58 );
      stepR( 59 );
      stepR( 60 );
      stepR( 61 );
      stepR( 62 );
      stepR( 63 );

      keystream += 256;  
   }
#endif
}
