/*
 * DECIM reference implementation.
 *
 * This code is supposed to run on any conforming C implementation (C90
 * or later). 
 *
 * Because DECIM is a hardware candidate, we deliberately made a very 
 * easy to understand and not at all optimised implementation. 
 * 
 * (c) 2005 X-CRYPT project. This software is provided 'as-is', without
 * any express or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * excluding commercial applications, and to alter it and redistribute it
 * freely except for commercial applications.
 *
 * France Telecom believes that the Decim cipher is covered in part by
 * the patent application under reference PCT/FR 04/2070 in co-ownership
 * between France Telecom and l'Universite de Caen Basse-Normandie. 
 * The X-CRYPT project members do not know of other licensed, patented
 * or otherwise legally restricted existing work that the Decim cipher
 * would be based on.
 *
 * Technical remarks and questions can be addressed to
 * <come.berbain@francetelecom.com>
 */


#include"decim.h"


/* Not needed */
void ECRYPT_init(void){}


void ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
  /* save the key */
  memcpy(ctx->key, key, 10);

  /* save the IV size in the cipher's context */
  ctx->iv_size = ivsize;
}



void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
  int i, j;
  u8 piv[10] ; 

  /* reset ABSG internal state */
  ctx->immediate_finding=0;
  ctx->bit_searched=0;
  ctx->searching=0;

  /* clear the output buffer */
  ctx->buffer_end=0;
  for(i=0;i<32;i++) ctx->buffer[i]=0;

  /* key injection */
  for (i=0;i<10;i++)
    for (j=0;j<8;j++)
      ctx->lfsr_state[i*8+j+112] = (ctx->key[i]>>j) & 0x1;

  for (i=0;i<7;i++)
    for (j=0;j<8;j++)
    {
      ctx->lfsr_state[i*8+j    ] = (ctx->key[i]>>j) & 0x1;
      ctx->lfsr_state[i*8+j+56 ] = (ctx->key[i]>>j) & 0x1;
    }

  /* we first copy what we've got                (memcpy)
   * then, we extend the iv with '0's to 80 bits (memset) */
  switch(ctx->iv_size){
    case 32:
      memcpy(piv, iv, 4);
      memset(piv+4, 0, 6);
      break;
    case 64:
      memcpy(piv, iv, 8);
      memset(piv+8, 0, 2);
      break;
    default:
      fprintf(stderr, "Invalid IV length!\n");
      exit(EXIT_FAILURE);
  }

  /* IV expansion */
  for (i=0;i<10;i++)
    for (j=0;j<8;j++)
      ctx->lfsr_state[i*8+j+112] ^= (piv[i]>>j) & 0x1;

  for (i=0;i<7;i++)
    for (j=0;j<8;j++)
    {
      ctx->lfsr_state[i*8+j]    |=  (piv[i]>>j) & 0x1;
      ctx->lfsr_state[i*8+j+56] &= ((piv[i]>>j) & 0x1)^0x1;
    }


  for (j=0;j<192;j++)
    decim_lfsr_init(ctx);

  /* reset ABSG state */
  ctx->immediate_finding=0;
  ctx->bit_searched=0;
  ctx->searching=0;

  /* fill the buffer */
  while(ctx->buffer_end<32)
  {  
    decim_step(ctx);
    decim_step(ctx);
  }

  ctx->bits_in_byte = 0;
  ctx->stream_byte  = 0;
} 

/*
 * Encryption/Decryption 
 * action: silently ignored since this is an involutional cipher
 * ctx   : the context with all the internal state
 * input : the plaintext bytes
 * output: the encrypted bytes
 * msglen: the message's length in bytes
 */
void ECRYPT_process_bytes(int action, ECRYPT_ctx* ctx, const u8* input, u8* output, u32 msglen)
{ 
  u8 stream_byte;
  int is_stream_byte = 0;
  u32 bytes_processed = 0;

  while(bytes_processed < msglen)
  {
    decim_step(ctx);
    decim_step(ctx);
    decim_process_buffer(ctx, &is_stream_byte, &stream_byte);
    if(is_stream_byte){
      output[bytes_processed] = input[bytes_processed] ^ stream_byte;
      bytes_processed++;
    }

    /*
     * manage the case where the buffer is empty
     * this will happen with very low probability
     * */
    if((ctx->buffer_end==0) && (bytes_processed < msglen)){
      int i;
      for(i=0;i<4;i++){
        decim_lfsr_filter(ctx);
        decim_lfsr_clock(ctx);
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_a;
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_b;
        decim_lfsr_filter(ctx);
        decim_lfsr_clock(ctx);
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_a;
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_b;
        decim_process_buffer(ctx, &is_stream_byte, &stream_byte);
        if(is_stream_byte){
          output[bytes_processed] = input[bytes_processed] ^ stream_byte;
          bytes_processed++;
        }
      }
    }/* now, buffer refilled */
  
  }/* end of while: all bytes (en/de)crypted */

}




/*
 * Keystream generation
 * ctx      : the context with all the internal state
 * Keystream: the keystream
 * length   : the keystream size in bytes
 */
void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{ 
  u8 stream_byte;
  int is_stream_byte = 0;
  u32 bytes_outputed = 0;

  while(bytes_outputed < length)
  {
    decim_step(ctx);
    decim_step(ctx);
    decim_process_buffer(ctx, &is_stream_byte, &stream_byte);
    if(is_stream_byte){
      keystream[bytes_outputed] = stream_byte;
      bytes_outputed++;
    }

    /*
     * manage the case where the buffer is empty
     * this will happen with very low probability
     * */
    if((ctx->buffer_end==0) && (bytes_outputed < length)){
      int i;
      for(i=0;i<4;i++){
        decim_lfsr_filter(ctx);
        decim_lfsr_clock(ctx);
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_a;
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_b;
        decim_lfsr_filter(ctx);
        decim_lfsr_clock(ctx);
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_a;
        ctx->buffer_end++; ctx->buffer[ctx->buffer_end]=ctx->bool_b;
        decim_process_buffer(ctx, &is_stream_byte, &stream_byte);
        if(is_stream_byte){
          keystream[bytes_outputed] = stream_byte;
          bytes_outputed++;
        }
      }
    }/* now, buffer refilled */
  
  }/* end of while: all keystream generated */

}





/*
 * Here follows the auxiliary functions
 * */
void decim_permute_a(ECRYPT_ctx *ctx)
{
  u8 aux = ctx->lfsr_state[59];
  ctx->lfsr_state[59]=ctx->lfsr_state[177];
  ctx->lfsr_state[177]=ctx->lfsr_state[5];
  ctx->lfsr_state[5]=aux;
  aux=ctx->lfsr_state[186];
  ctx->lfsr_state[186]=ctx->lfsr_state[31];
  ctx->lfsr_state[31]=ctx->lfsr_state[144];
  ctx->lfsr_state[144]=ctx->lfsr_state[100];
  ctx->lfsr_state[100]=aux;
}

void decim_permute_b(ECRYPT_ctx *ctx)
{
  u8 aux = ctx->lfsr_state[177];
  ctx->lfsr_state[177]=ctx->lfsr_state[31];
  ctx->lfsr_state[31]=ctx->lfsr_state[144];
  ctx->lfsr_state[144]=ctx->lfsr_state[59];
  ctx->lfsr_state[59]=ctx->lfsr_state[186];
  ctx->lfsr_state[186]=ctx->lfsr_state[100];
  ctx->lfsr_state[100]=ctx->lfsr_state[5];
  ctx->lfsr_state[5]=aux;
}

void decim_lfsr_clock(ECRYPT_ctx *ctx)
{
  memcpy(ctx->lfsr_state, ctx->lfsr_state+1, 16);
  memcpy(ctx->lfsr_state+16, ctx->lfsr_state+17, 16);
  memcpy(ctx->lfsr_state+32, ctx->lfsr_state+33, 16);
  memcpy(ctx->lfsr_state+48, ctx->lfsr_state+49, 16);
  memcpy(ctx->lfsr_state+64, ctx->lfsr_state+65, 16);
  memcpy(ctx->lfsr_state+80, ctx->lfsr_state+81, 16);
  memcpy(ctx->lfsr_state+96, ctx->lfsr_state+97, 16);
  memcpy(ctx->lfsr_state+112, ctx->lfsr_state+113, 16);
  memcpy(ctx->lfsr_state+128, ctx->lfsr_state+129, 16);
  memcpy(ctx->lfsr_state+144, ctx->lfsr_state+145, 16);
  memcpy(ctx->lfsr_state+160, ctx->lfsr_state+161, 16);
  memcpy(ctx->lfsr_state+176, ctx->lfsr_state+177, 16);
}

void decim_lfsr_filter(ECRYPT_ctx *ctx)
{
  u8 b;

  /* compute the first boolean function */
  b  = ctx->lfsr_state[1];
  b += ctx->lfsr_state[32];
  b += ctx->lfsr_state[40];
  b += ctx->lfsr_state[101];
  b += ctx->lfsr_state[164];
  b += ctx->lfsr_state[178];
  b += ctx->lfsr_state[187];
  ctx->bool_a = (b>>1)&0x01;

  /* compute the second boolean function */
  b  = ctx->lfsr_state[6];
  b += ctx->lfsr_state[8];
  b += ctx->lfsr_state[60];
  b += ctx->lfsr_state[116];
  b += ctx->lfsr_state[145];
  b += ctx->lfsr_state[181];
  b += ctx->lfsr_state[191];
  ctx->bool_b = (b>>1)&0x01;

  /* compute the next bit for the LFSR*/
  b  = ctx->lfsr_state[0];
  b ^= ctx->lfsr_state[3];
  b ^= ctx->lfsr_state[4];
  b ^= ctx->lfsr_state[23];
  b ^= ctx->lfsr_state[36];
  b ^= ctx->lfsr_state[37];
  b ^= ctx->lfsr_state[60];
  b ^= ctx->lfsr_state[61];
  b ^= ctx->lfsr_state[98];
  b ^= ctx->lfsr_state[115];
  b ^= ctx->lfsr_state[146];
  b ^= ctx->lfsr_state[175];
  b ^= ctx->lfsr_state[176];
  b ^= ctx->lfsr_state[187];
  ctx->lfsr_state[192] = b;   /* next bit */
}

void decim_absg(ECRYPT_ctx *ctx, u8 bit)
{
  ctx->bit_searched = (ctx->searching ? ctx->bit_searched : bit);
  ctx->out  = ctx->searching & (ctx->immediate_finding ^ bit);
  ctx->immediate_finding = ctx->searching & (ctx->bit_searched ^ bit);
  ctx->searching  = !ctx->searching |ctx->immediate_finding;
}

void decim_lfsr_init(ECRYPT_ctx *ctx)
{
  decim_lfsr_filter(ctx);
  
  /* Run ABSG on the output of the two functions
   * and apply the good permutation*/
  decim_absg(ctx, ctx->bool_a);
  ctx->is_permute_a  = (ctx->out & !ctx->searching);

  decim_absg(ctx, ctx->bool_b);
  ctx->is_permute_a |= (ctx->out & !ctx->searching);

  if(ctx->is_permute_a)
    decim_permute_a(ctx);
  else
    decim_permute_b(ctx);

  ctx->lfsr_state[192] ^= (ctx->bool_a ^ ctx->bool_b);
  decim_lfsr_clock(ctx);
}


void decim_step(ECRYPT_ctx *ctx)
{
  decim_lfsr_filter(ctx);
  decim_lfsr_clock(ctx);
  decim_absg(ctx, ctx->bool_a);
  if ( !ctx->searching && (ctx->buffer_end<32)){
    ctx->buffer[ctx->buffer_end]=ctx->out;
    ctx->buffer_end++;
  }
  decim_absg(ctx, ctx->bool_b);
  if ( !ctx->searching && (ctx->buffer_end<32)){
    ctx->buffer[ctx->buffer_end]=ctx->out;
    ctx->buffer_end++;
  }
}


void decim_process_buffer(ECRYPT_ctx *ctx, int *is_stream_byte, u8 *stream_byte)
{
  int i;
  if( (*is_stream_byte = (ctx->bits_in_byte == 8)) )
  {
    *stream_byte = ctx->stream_byte;
    ctx->bits_in_byte = 0;
    ctx->stream_byte = 0;
  }
  ctx->stream_byte |= (ctx->buffer[0])<<ctx->bits_in_byte ;
  ctx->bits_in_byte++ ;
  for(i=0;i<31;i++) ctx->buffer[i]=ctx->buffer[i+1];
  ctx->buffer_end--;
}




#ifdef DECIM_VECTORS

int main (int argc, char **argv)
{
  static u8 pkey[33][10] = { /* a bench of keys */
    { 0xf6, 0x3d, 0xb4, 0x85, 0x4f, 0x85, 0x7d, 0xa9, 0x55, 0x43 } ,
    { 0x0c, 0x54, 0x8c, 0xcd, 0x36, 0xd5, 0x37, 0x7b, 0x25, 0xe8 } ,
    { 0x1f, 0xf6, 0x98, 0x74, 0xd3, 0xa1, 0x8b, 0x38, 0x97, 0xcb } ,
    { 0x39, 0xc2, 0x7f, 0x5e, 0x31, 0x13, 0x33, 0x0f, 0x2f, 0x8c } ,
    { 0xd9, 0xda, 0xbf, 0x32, 0x90, 0x99, 0x7e, 0xb8, 0x9c, 0xef } ,
    { 0x86, 0x1b, 0x51, 0xd2, 0xb4, 0x2c, 0x7b, 0xf9, 0xb0, 0x36 } ,
    { 0x20, 0xd9, 0xb8, 0x1e, 0x4f, 0x93, 0x2f, 0x94, 0x0f, 0x5a } ,
    { 0x45, 0xfd, 0x8e, 0x2d, 0x14, 0xd2, 0xa0, 0x0b, 0xe5, 0xc1 } ,
    { 0x5b, 0xdb, 0xc8, 0x02, 0x8e, 0x52, 0xcb, 0x55, 0xcd, 0xa1 } ,
    { 0x1b, 0xe8, 0x4d, 0x27, 0x99, 0x09, 0xce, 0x48, 0x63, 0x2b } ,
    { 0x93, 0x4e, 0xe8, 0xa0, 0x58, 0x80, 0x0c, 0xcb, 0xc0, 0x98 } ,
    { 0x79, 0x67, 0x70, 0x9f, 0xea, 0x8b, 0xbe, 0xf7, 0x10, 0x83 } ,
    { 0x1b, 0x23, 0x08, 0x75, 0x76, 0xf4, 0xc5, 0xbc, 0x4c, 0x54 } ,
    { 0xa9, 0x11, 0x33, 0xe9, 0xc0, 0x36, 0x4e, 0x43, 0xce, 0xee } ,
    { 0x73, 0xa4, 0x49, 0x68, 0xb4, 0x00, 0x43, 0xf0, 0x01, 0x62 } ,
    { 0xae, 0x31, 0xbd, 0x02, 0x74, 0xa0, 0x03, 0x06, 0xdb, 0x4d } ,
    { 0x1d, 0xd8, 0x47, 0xe2, 0x23, 0xda, 0xe8, 0x0b, 0xfe, 0x20 } ,
    { 0xed, 0x3f, 0x8f, 0x8d, 0xfe, 0x53, 0x29, 0x33, 0x14, 0x1f } ,
    { 0x42, 0x2e, 0x3a, 0xd7, 0x70, 0xb2, 0x11, 0x4d, 0x69, 0xd2 } ,
    { 0xbd, 0x1a, 0xa3, 0xf5, 0xb2, 0x87, 0x39, 0xfe, 0x43, 0x3c } ,
    { 0x84, 0x96, 0x77, 0x98, 0x54, 0xa6, 0x47, 0xd7, 0xef, 0x48 } ,
    { 0x45, 0x1a, 0x91, 0xde, 0xc6, 0x31, 0xe0, 0x44, 0x7a, 0x5e } ,
    { 0x4a, 0xa2, 0x8d, 0xcb, 0x5b, 0xef, 0xda, 0x4e, 0x16, 0x40 } ,
    { 0xc5, 0x4b, 0x88, 0x35, 0xc6, 0x82, 0x63, 0xe3, 0xf0, 0x80 } ,
    { 0xf6, 0xa7, 0x0d, 0x51, 0xca, 0x65, 0x02, 0x29, 0x9a, 0x3d } ,
    { 0x0a, 0x5d, 0x6f, 0x5e, 0xda, 0x77, 0x49, 0xbf, 0x51, 0xd7 } ,
    { 0xcb, 0xf3, 0x95, 0x0b, 0x21, 0x20, 0x91, 0x96, 0x11, 0xff } ,
    { 0xba, 0x46, 0x04, 0x5c, 0x88, 0xbb, 0x62, 0x54, 0xd0, 0x93 } ,
    { 0xcb, 0x10, 0x2d, 0xd4, 0xbd, 0x72, 0x28, 0x70, 0xe9, 0xc2 } ,
    { 0xf5, 0xe5, 0x08, 0x53, 0xfa, 0xb5, 0x7b, 0x68, 0x1a, 0x98 } ,
    { 0x87, 0x58, 0x9a, 0x26, 0x15, 0x96, 0xfb, 0x02, 0x50, 0x77 } ,
    { 0xbb, 0x6e, 0xb8, 0x8e, 0xfb, 0x29, 0xe9, 0x93, 0x17, 0x7d } };


  static u8 piv[32][8] = { /* initialization vectors 64bits */
    { 0x0c, 0x8e, 0xdb, 0x88, 0x01, 0xaa, 0x95, 0xb4 },
    { 0x36, 0x84, 0xba, 0xf4, 0x11, 0xdc, 0x47, 0xdf }, 
    { 0xab, 0x85, 0xb1, 0x6e, 0xa6, 0x4b, 0x3d, 0x53 }, 
    { 0x55, 0x28, 0x6c, 0x63, 0xd6, 0x18, 0x67, 0x0c }, 
    { 0x6c, 0xe5, 0xae, 0xc5, 0xf3, 0x55, 0xf1, 0xa5 }, 
    { 0x08, 0x5b, 0x40, 0xfe, 0x90, 0x14, 0x57, 0xe4 }, 
    { 0x3e, 0x3f, 0xe4, 0x63, 0x0c, 0xc5, 0xac, 0x9a }, 
    { 0xe0, 0xc5, 0x23, 0x1c, 0xe5, 0x6a, 0xed, 0x2d }, 
    { 0x23, 0x64, 0xca, 0xe9, 0x56, 0xff, 0xfa, 0x61 }, 
    { 0x4f, 0x7d, 0xf4, 0x18, 0x06, 0xcc, 0x3a, 0x00 }, 
    { 0xed, 0xf5, 0x3a, 0xe6, 0x97, 0xa0, 0x9b, 0x85 }, 
    { 0x2a, 0xf3, 0x12, 0x1b, 0x16, 0x3d, 0xe2, 0x0c }, 
    { 0x08, 0x9a, 0x15, 0x05, 0x6b, 0xb8, 0x92, 0x5c }, 
    { 0x18, 0x5e, 0x63, 0xd1, 0x0f, 0x19, 0x06, 0xed }, 
    { 0x7c, 0x4e, 0x02, 0xe7, 0x60, 0x14, 0xb5, 0xdf }, 
    { 0x5f, 0xbd, 0xb1, 0x08, 0xeb, 0x97, 0xf7, 0x8d }, 
    { 0xef, 0x01, 0x8f, 0xd8, 0xea, 0x56, 0xcd, 0x50 }, 
    { 0x21, 0x7e, 0xa1, 0xe3, 0x98, 0xcd, 0x2c, 0x63 }, 
    { 0xdb, 0x0f, 0xf5, 0x7c, 0xb8, 0x5e, 0x8a, 0xd9 }, 
    { 0x50, 0xda, 0x56, 0x0a, 0x37, 0xcd, 0xe5, 0x33 }, 
    { 0x06, 0xba, 0x6c, 0xd7, 0xe8, 0x22, 0x6d, 0x4a }, 
    { 0xd9, 0x5d, 0xcf, 0x00, 0x7e, 0x90, 0x6d, 0xf5 }, 
    { 0x6f, 0xb1, 0xe4, 0xc4, 0xd2, 0x86, 0x9f, 0xd8 }, 
    { 0xf0, 0xa5, 0xec, 0xea, 0xe4, 0xaf, 0xae, 0x17 }, 
    { 0x55, 0x90, 0x77, 0x6b, 0xd4, 0xe7, 0xe7, 0x22 }, 
    { 0x73, 0x48, 0x9e, 0xc0, 0x6e, 0x81, 0x14, 0x8b }, 
    { 0xd3, 0xb8, 0xaf, 0xcb, 0x93, 0x04, 0x59, 0x35 }, 
    { 0x7e, 0xae, 0x1d, 0x73, 0x7d, 0xea, 0xab, 0xab }, 
    { 0xfd, 0xd9, 0x1e, 0xac, 0x19, 0x86, 0x2f, 0xd0 }, 
    { 0xe3, 0x3c, 0x9d, 0x49, 0x54, 0x87, 0xd5, 0x4b }, 
    { 0xd6, 0x75, 0xdd, 0xe0, 0x53, 0xa7, 0x03, 0xde }, 
    { 0x33, 0x2c, 0x01, 0x19, 0xee, 0xd4, 0xba, 0x17 } };


  int msglen  = 24; 
  u8 *output  = (u8 *)calloc(msglen, 1);
  u8 *message = (u8 *)calloc(msglen, 1);

  u32 keysize = 80;
  u32 ivsize  = 64;

  int i, j;
  
  ECRYPT_ctx ctx ;
  
  printf("\nGenerating test vector...\n");
  for(j=0;j<32;j++) {    
    const u8 *key = pkey[j];
    const u8 *iv  = piv[j]; 

    printf("\n\nKey: ");
    for (i=0;i<10;i++) printf("%2.2x ",key[i]);
    
    printf("\nIV : ");
    for (i=0;i<8;i++)  printf("%2.2x ",iv[i]);
    
    ECRYPT_keysetup(&ctx, key, keysize, ivsize);
    ECRYPT_ivsetup(&ctx, iv);
    ECRYPT_process_bytes(0, &ctx, message, output, msglen);

    printf("\nOut: ");
    for (i=0; i<msglen; i++)  printf("%2.2x ",output[i]);
  }

  free(message);
  free(output);

  return 0;

}
#elif  DECIM_KEYSTREAM
int main(int argc, char **argv)
{
  u32 msglen; 
  u8 *keystream;
  u32 keysize = 80;
  u32 ivsize  = 64;
  static u8 key[] = { 0xf6, 0x3d, 0xb4, 0x85, 0x4f, 0x85, 0x7d, 0xa9, 0x55, 0x43 };
  static u8 iv[]  = { 0x0c, 0x8e, 0xdb, 0x88, 0x01, 0xaa, 0x95, 0xb4 };
  
  ECRYPT_ctx ctx ;

  if(argc < 2){
    fprintf(stderr, "Please enter the amount of bytes to be generated\n");
    exit(EXIT_FAILURE);
  }
  
  msglen = atoi(argv[1]);
  keystream = (u8 *)calloc(msglen, 1);

  ECRYPT_keysetup(&ctx, key, keysize, ivsize);
  ECRYPT_ivsetup(&ctx, iv);
  ECRYPT_keystream_bytes(&ctx, keystream, msglen);

  return 0;
}
#endif

