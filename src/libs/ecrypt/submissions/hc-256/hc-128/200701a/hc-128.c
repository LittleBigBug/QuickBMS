
#include "ecrypt-sync.h"

/* =====================================================================
 *     The following defines the keystream generation function          
 *======================================================================*/

/*h1 function*/
#define h1(ctx, x, y) {    \
     u8 a,c;               \
     a = (u8) (x);         \
     c = (u8) ((x) >> 16);  \
     y = (ctx->T[512+a])+(ctx->T[512+256+c]); \
}

/*h2 function*/
#define h2(ctx, x, y) {    \
     u8 a,c;               \
     a = (u8) (x);         \
     c = (u8) ((x) >> 16); \
     y = (ctx->T[a])+(ctx->T[256+c]); \
}

/*one step of HC-128, update P and generate 32 bits keystream*/
#define step_P(ctx,u,v,a,b,c,d,n){    \
     u32 tem0,tem1,tem3,tem2,tem4;         \
     tem0 = ROTR32((ctx->T[(v)]),23);           \
     tem1 = ROTR32((ctx->X[(c)]),10);           \
     tem2 = ROTR32((ctx->X[(b)]),8);  \
     tem4 = tem0 ^ tem1;              \
     (ctx->T[(u)]) += tem2+tem4;       \
     (ctx->X[(a)]) = (ctx->T[(u)]);             \
     h1((ctx),(ctx->X[(d)]),tem3);              \
     (n) = tem3 ^ (ctx->T[(u)]) ;               \
}       



/*one step of HC-128, update Q and generate 32 bits keystream*/
#define step_Q(ctx,u,v,a,b,c,d,n){      \
     u32 tem0,tem1,tem2,tem3,tem4;           \
     tem0 = ROTR32((ctx->T[(v)]),(32-23));      \
     tem1 = ROTR32((ctx->Y[(c)]),(32-10));      \
     tem2 = ROTR32((ctx->Y[(b)]),(32-8));       \
     tem4 = tem0 ^ tem1; \
     (ctx->T[(u)]) += tem2 + tem4;     \
     (ctx->Y[(a)]) = (ctx->T[(u)]);             \
     h2((ctx),(ctx->Y[(d)]),tem3);              \
     (n) = tem3 ^ (ctx->T[(u)]) ;               \
}


/*one step of HC-128, update P and generate 32 bits keystream*/
/*#define step_P(ctx,u,v,a,b,c,d,n){    \
     u32 tem0,tem1,tem2,tem3;         \
     h1((ctx),(ctx->X[(d)]),tem3);              \
     tem0 = ROTR32((ctx->T[(v)]),23);           \
     tem1 = ROTR32((ctx->X[(c)]),10);           \
     tem2 = ROTR32((ctx->X[(b)]),8);            \
     (ctx->T[(u)]) += tem2+(tem0 ^ tem1);       \
     (ctx->X[(a)]) = (ctx->T[(u)]);             \
     (n) = tem3 ^ (ctx->T[(u)]) ;               \
}
*/


/*one step of HC-128, update Q and generate 32 bits keystream*/
/*#define step_Q(ctx,u,v,a,b,c,d,n){      \
     u32 tem0,tem1,tem2,tem3;           \
     h2((ctx),(ctx->Y[(d)]),tem3);              \
     tem0 = ROTR32((ctx->T[(v)]),(32-23));      \
     tem1 = ROTR32((ctx->Y[(c)]),(32-10));      \
     tem2 = ROTR32((ctx->Y[(b)]),(32-8));       \
     (ctx->T[(u)]) += tem2 + (tem0 ^ tem1);     \
     (ctx->Y[(a)]) = (ctx->T[(u)]);             \
     (n) = tem3 ^ (ctx->T[(u)]) ;               \
} */  


/*16 steps of HC-128, generate 512 bits keystream*/
void generate_keystream(ECRYPT_ctx* ctx, u32* keystream)  
{
   u32 cc,dd;
   cc = ctx->counter1024 & 0x1ff;
   dd = (cc+16)&0x1ff;

   if (ctx->counter1024 < 512)	
   {   		
      ctx->counter1024 = (ctx->counter1024 + 16) & 0x3ff;
      step_P(ctx, cc+0, cc+1, 0, 6, 13,4, keystream[0]);
      step_P(ctx, cc+1, cc+2, 1, 7, 14,5, keystream[1]);
      step_P(ctx, cc+2, cc+3, 2, 8, 15,6, keystream[2]);
      step_P(ctx, cc+3, cc+4, 3, 9, 0, 7, keystream[3]);
      step_P(ctx, cc+4, cc+5, 4, 10,1, 8, keystream[4]);
      step_P(ctx, cc+5, cc+6, 5, 11,2, 9, keystream[5]);
      step_P(ctx, cc+6, cc+7, 6, 12,3, 10,keystream[6]);
      step_P(ctx, cc+7, cc+8, 7, 13,4, 11,keystream[7]);
      step_P(ctx, cc+8, cc+9, 8, 14,5, 12,keystream[8]);
      step_P(ctx, cc+9, cc+10,9, 15,6, 13,keystream[9]);
      step_P(ctx, cc+10,cc+11,10,0, 7, 14,keystream[10]);
      step_P(ctx, cc+11,cc+12,11,1, 8, 15,keystream[11]);
      step_P(ctx, cc+12,cc+13,12,2, 9, 0, keystream[12]);
      step_P(ctx, cc+13,cc+14,13,3, 10,1, keystream[13]);
      step_P(ctx, cc+14,cc+15,14,4, 11,2, keystream[14]);
      step_P(ctx, cc+15,dd+0, 15,5, 12,3, keystream[15]);
   }
   else				    
   {
	ctx->counter1024 = (ctx->counter1024 + 16) & 0x3ff;
      step_Q(ctx, 512+cc+0, 512+cc+1, 0, 6, 13,4, keystream[0]);
      step_Q(ctx, 512+cc+1, 512+cc+2, 1, 7, 14,5, keystream[1]);
      step_Q(ctx, 512+cc+2, 512+cc+3, 2, 8, 15,6, keystream[2]);
      step_Q(ctx, 512+cc+3, 512+cc+4, 3, 9, 0, 7, keystream[3]);
      step_Q(ctx, 512+cc+4, 512+cc+5, 4, 10,1, 8, keystream[4]);
      step_Q(ctx, 512+cc+5, 512+cc+6, 5, 11,2, 9, keystream[5]);
      step_Q(ctx, 512+cc+6, 512+cc+7, 6, 12,3, 10,keystream[6]);
      step_Q(ctx, 512+cc+7, 512+cc+8, 7, 13,4, 11,keystream[7]);
      step_Q(ctx, 512+cc+8, 512+cc+9, 8, 14,5, 12,keystream[8]);
      step_Q(ctx, 512+cc+9, 512+cc+10,9, 15,6, 13,keystream[9]);
      step_Q(ctx, 512+cc+10,512+cc+11,10,0, 7, 14,keystream[10]);
      step_Q(ctx, 512+cc+11,512+cc+12,11,1, 8, 15,keystream[11]);
      step_Q(ctx, 512+cc+12,512+cc+13,12,2, 9, 0, keystream[12]);
      step_Q(ctx, 512+cc+13,512+cc+14,13,3, 10,1, keystream[13]);
      step_Q(ctx, 512+cc+14,512+cc+15,14,4, 11,2, keystream[14]);
      step_Q(ctx, 512+cc+15,512+dd+0, 15,5, 12,3, keystream[15]);
   }
}


/*======================================================*/
/*   The following defines the initialization functions */
/*======================================================*/

#define f1(x)  (ROTR32((x),7) ^ ROTR32((x),18) ^ ((x) >> 3))
#define f2(x)  (ROTR32((x),17) ^ ROTR32((x),19) ^ ((x) >> 10))

/*update table P*/
#define update_P(ctx,u,v,a,b,c,d){      \
     u32 tem0,tem1,tem2,tem3;           \
     tem0 = ROTR32((ctx->T[(v)]),23);           \
     tem1 = ROTR32((ctx->X[(c)]),10);           \
     tem2 = ROTR32((ctx->X[(b)]),8);            \
     h1((ctx),(ctx->X[(d)]),tem3);              \
     (ctx->T[(u)]) = ((ctx->T[(u)]) + tem2+(tem0^tem1)) ^ tem3;         \
     (ctx->X[(a)]) = (ctx->T[(u)]);             \
}  

/*update table Q*/
#define update_Q(ctx,u,v,a,b,c,d){      \
     u32 tem0,tem1,tem2,tem3;      \
     tem0 = ROTR32((ctx->T[(v)]),(32-23));             \
     tem1 = ROTR32((ctx->Y[(c)]),(32-10));             \
     tem2 = ROTR32((ctx->Y[(b)]),(32-8));            \
     h2((ctx),(ctx->Y[(d)]),tem3);              \
     (ctx->T[(u)]) = ((ctx->T[(u)]) + tem2+(tem0^tem1)) ^ tem3; \
     (ctx->Y[(a)]) = (ctx->T[(u)]);                       \
}     

/*16 steps of HC-128, without generating keystream, */
/*but use the outputs to update P and Q*/
void setup_update(ECRYPT_ctx* ctx)  /*each time 16 steps*/
{
   u32 cc,dd;
   cc = ctx->counter1024 & 0x1ff;
   dd = (cc+16)&0x1ff;

   if (ctx->counter1024 < 512)	
   {   		
      ctx->counter1024 = (ctx->counter1024 + 16) & 0x3ff;
      update_P(ctx, cc+0, cc+1, 0, 6, 13, 4);
      update_P(ctx, cc+1, cc+2, 1, 7, 14, 5);
      update_P(ctx, cc+2, cc+3, 2, 8, 15, 6);
      update_P(ctx, cc+3, cc+4, 3, 9, 0,  7);
      update_P(ctx, cc+4, cc+5, 4, 10,1,  8);
      update_P(ctx, cc+5, cc+6, 5, 11,2,  9);
      update_P(ctx, cc+6, cc+7, 6, 12,3,  10);
      update_P(ctx, cc+7, cc+8, 7, 13,4,  11);
      update_P(ctx, cc+8, cc+9, 8, 14,5,  12);
      update_P(ctx, cc+9, cc+10,9, 15,6,  13);
      update_P(ctx, cc+10,cc+11,10,0, 7,  14);
      update_P(ctx, cc+11,cc+12,11,1, 8,  15);
      update_P(ctx, cc+12,cc+13,12,2, 9,  0);
      update_P(ctx, cc+13,cc+14,13,3, 10, 1);
      update_P(ctx, cc+14,cc+15,14,4, 11, 2);
      update_P(ctx, cc+15,dd+0, 15,5, 12, 3);   
   }
   else				    
   {
      ctx->counter1024 = (ctx->counter1024 + 16) & 0x3ff;
      update_Q(ctx, 512+cc+0, 512+cc+1, 0, 6, 13, 4);
      update_Q(ctx, 512+cc+1, 512+cc+2, 1, 7, 14, 5);
      update_Q(ctx, 512+cc+2, 512+cc+3, 2, 8, 15, 6);
      update_Q(ctx, 512+cc+3, 512+cc+4, 3, 9, 0,  7);
      update_Q(ctx, 512+cc+4, 512+cc+5, 4, 10,1,  8);
      update_Q(ctx, 512+cc+5, 512+cc+6, 5, 11,2,  9);
      update_Q(ctx, 512+cc+6, 512+cc+7, 6, 12,3,  10);
      update_Q(ctx, 512+cc+7, 512+cc+8, 7, 13,4,  11);
      update_Q(ctx, 512+cc+8, 512+cc+9, 8, 14,5,  12);
      update_Q(ctx, 512+cc+9, 512+cc+10,9, 15,6,  13);
      update_Q(ctx, 512+cc+10,512+cc+11,10,0, 7,  14);
      update_Q(ctx, 512+cc+11,512+cc+12,11,1, 8,  15);
      update_Q(ctx, 512+cc+12,512+cc+13,12,2, 9,  0);
      update_Q(ctx, 512+cc+13,512+cc+14,13,3, 10, 1);
      update_Q(ctx, 512+cc+14,512+cc+15,14,4, 11, 2);
      update_Q(ctx, 512+cc+15,512+dd+0, 15,5, 12, 3); 
   }       
}

void ECRYPT_init(void) {
}  /* No operation performed */

/* for the 128-bit key:  key[0]...key[15]
*  key[0] is the least significant byte of ctx->key[0] (K_0);
*  key[3] is the most significant byte of ctx->key[0]  (K_0);
*  ...
*  key[12] is the least significant byte of ctx->key[3] (K_3)
*  key[15] is the most significant byte of ctx->key[3]  (K_3)
*
*  for the 128-bit iv:  iv[0]...iv[15]
*  iv[0] is the least significant byte of ctx->iv[0] (IV_0);
*  iv[3] is the most significant byte of ctx->iv[0]  (IV_0);
*  ...
*  iv[12] is the least significant byte of ctx->iv[3] (IV_3)
*  iv[15] is the most significant byte of ctx->iv[3]  (IV_3)
*/

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,                /* Key size in bits (128+128*i) */ 
  u32 ivsize)                 /* IV size in bits  (128+128*i)*/
{ 
  u32 i;  

  ctx->keysize = keysize;  
  ctx->ivsize = ivsize;

  /* Key size in bits 128 */ 
  for (i = 0; i < (keysize >> 5); i++) ctx->key[i] = U32TO32_LITTLE (((u32*)key)[i]);
 
  for ( ; i < 8 ; i++) ctx->key[i] = ctx->key[i-4];
  
} /* initialize the key, save the iv size*/


void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{ 
    u32 i;
	
    /* initialize the iv */
    /* IV size in bits  128*/

	for (i = 0; i < (ctx->ivsize >> 5); i++)  ctx->iv[i] = U32TO32_LITTLE(((u32*)iv)[i]);
	
    for (; i < 8; i++) ctx->iv[i] = ctx->iv[i-4];
  
    /* expand the key and IV into the table T */ 
    /* (expand the key and IV into the table P and Q) */ 
	
	for (i = 0; i < 8;  i++)   ctx->T[i] = ctx->key[i];
	for (i = 8; i < 16; i++)   ctx->T[i] = ctx->iv[i-8];

    for (i = 16; i < (256+16); i++) 
		ctx->T[i] = f2(ctx->T[i-2]) + ctx->T[i-7] + f1(ctx->T[i-15]) + ctx->T[i-16]+i;
    
	for (i = 0; i < 16;  i++)  ctx->T[i] = ctx->T[256+i];

	for (i = 16; i < 1024; i++) 
		ctx->T[i] = f2(ctx->T[i-2]) + ctx->T[i-7] + f1(ctx->T[i-15]) + ctx->T[i-16]+256+i;
    
    /* initialize counter1024, X and Y */
	ctx->counter1024 = 0;
	for (i = 0; i < 16; i++) ctx->X[i] = ctx->T[512-16+i];
    for (i = 0; i < 16; i++) ctx->Y[i] = ctx->T[512+512-16+i];
    
    /* run the cipher 1024 steps before generating the output */
	for (i = 0; i < 64; i++)  setup_update(ctx);  
}

/*========================================================
 *  The following defines the encryption of data stream
 *========================================================
 */

void ECRYPT_process_bytes(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 msglen)                /* Message length in bytes. */ 
{
  u32 i, keystream[16];

  for ( ; msglen >= 64; msglen -= 64, input += 64, output += 64)
  {
	  generate_keystream(ctx, keystream);

      /*for (i = 0; i < 16; ++i)
	      ((u32*)output)[i] = ((u32*)input)[i] ^ U32TO32_LITTLE(keystream[i]); */

	  ((u32*)output)[0]  = ((u32*)input)[0]  ^ U32TO32_LITTLE(keystream[0]);
	  ((u32*)output)[1]  = ((u32*)input)[1]  ^ U32TO32_LITTLE(keystream[1]);
	  ((u32*)output)[2]  = ((u32*)input)[2]  ^ U32TO32_LITTLE(keystream[2]);
	  ((u32*)output)[3]  = ((u32*)input)[3]  ^ U32TO32_LITTLE(keystream[3]);
	  ((u32*)output)[4]  = ((u32*)input)[4]  ^ U32TO32_LITTLE(keystream[4]);
	  ((u32*)output)[5]  = ((u32*)input)[5]  ^ U32TO32_LITTLE(keystream[5]);
	  ((u32*)output)[6]  = ((u32*)input)[6]  ^ U32TO32_LITTLE(keystream[6]);
	  ((u32*)output)[7]  = ((u32*)input)[7]  ^ U32TO32_LITTLE(keystream[7]);
	  ((u32*)output)[8]  = ((u32*)input)[8]  ^ U32TO32_LITTLE(keystream[8]);
	  ((u32*)output)[9]  = ((u32*)input)[9]  ^ U32TO32_LITTLE(keystream[9]);
	  ((u32*)output)[10] = ((u32*)input)[10] ^ U32TO32_LITTLE(keystream[10]);
	  ((u32*)output)[11] = ((u32*)input)[11] ^ U32TO32_LITTLE(keystream[11]);
	  ((u32*)output)[12] = ((u32*)input)[12] ^ U32TO32_LITTLE(keystream[12]);
	  ((u32*)output)[13] = ((u32*)input)[13] ^ U32TO32_LITTLE(keystream[13]);
	  ((u32*)output)[14] = ((u32*)input)[14] ^ U32TO32_LITTLE(keystream[14]);
	  ((u32*)output)[15] = ((u32*)input)[15] ^ U32TO32_LITTLE(keystream[15]);
  }

  if (msglen > 0)
  {
      generate_keystream(ctx, keystream);

      for (i = 0; i < msglen; i ++)
	      output[i] = input[i] ^ ((u8*)keystream)[i];
  }

}
