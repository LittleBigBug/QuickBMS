/* yamb.c */


#include "ecrypt-sync.h"


#define ALF 0x091B17C9


#define stepOLZ()\
{\
   abcd = ( ( ( ( s32 )OLZ[ I0 ] ) >> 31 ) & ALF ) ^ ( OLZ[ I0 ] + OLZ[ I0 ] ) ^ OLZ[ I1 ];\
   OLZ[ I2 ] = abcd;\
   I0++;    I0 &= 0xF;\
   I1++;    I1 &= 0xF;\
   I2++;    I2 &= 0xF;\
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
   ABCD   = ( u32 )a | ( ( ( u32 )b ) << 8 ) | ( ( ( u32 )c ) << 16 ) | ( ( ( u32 )d ) << 24 );\
   ABCD  ^= abcd;\
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
   RZ    = ctx->RZ;
   OLZ   = ctx->OLZ; 
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
                 ( ( ( u32 )pu8[ i * 4 + 3 ] ) << 24 );

   I0 = 0;
   I1 = 8;
   I2 = 15;
   J  = 0;
   
   for( i = 0; i < 225; i++ )
   { 
      stepOLZ();
      stepUU();
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   } 

   for( i = 0; i < 64; i++ )
   {
      stepOLZ();
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
      stepOLZ();
      stepUU();
      RZ[ J ] += ABCD;
      J++;
      J &= 0xF;
   }

   for( i = 0; i < 15; i++ )
      OLZ[ i ] = RZ[ i + 1 ];

   I0 = 0;
   I1 = 8;
   I2 = 15;
   J  = 0;
   
   for( i = 0; i < 16; i++ )
   {
      stepOLZ();
      stepUU();
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   }

   ctx->I0  = I0;
   ctx->I1  = I1;
   ctx->I2  = I2;
   ctx->J   = J;
}


void ECRYPT_process_bytes(
   int         action,
   ECRYPT_ctx* ctx,
   const u8*   input,
   u8*         output,
   u32         msglen )
{
   u8 remsize, i;
   u8 remainder[ ECRYPT_BLOCKLENGTH ];

   ECRYPT_process_blocks( action, ctx, input, output, ( msglen / ECRYPT_BLOCKLENGTH ) );

   if( (remsize = ( msglen % ECRYPT_BLOCKLENGTH )) )
   {
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

   for( i = 0; i < blocks; i++ )
      *( ( u32* )output + i ) ^= *( ( u32* )input + i );
}


void ECRYPT_keystream_blocks(
   ECRYPT_ctx* ctx,
   u8*         keystream,
   u32         blocks )
{
   u8    a, b, c, d;
   u32   abcd, ABCD;
   u8    *M;
   u32   *OLZ, *RZ;
   u8    I0, I1, I2, J;

   M     = ctx->M; 
   RZ    = ctx->RZ;
   OLZ   = ctx->OLZ; 

   I0    = ctx->I0;
   I1    = ctx->I1;
   I2    = ctx->I2;
   J     = ctx->J;

   while( blocks-- )
   {
      stepOLZ();
      stepUU();
      
      abcd = ( ABCD + RZ[ J ] );
      keystream[ 0 ] = ( u8 )abcd;
      keystream[ 1 ] = ( u8 )( abcd >> 8 );
      keystream[ 2 ] = ( u8 )( abcd >> 16 );
      keystream[ 3 ] = ( u8 )( abcd >> 24 );
      keystream += 4; 
      
      RZ[ J ] = ABCD;
      J++;
      J &= 0xF;
   }

   ctx->I0  = I0;
   ctx->I1  = I1;
   ctx->I2  = I2;
   ctx->J   = J;
}
