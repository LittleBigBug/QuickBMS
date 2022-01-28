/* ecrypt-frogbit.c */

#include "ecrypt-sync-ae.h"

#include "ecrypt-frogbit-prng.h"

#ifdef ECRYPT_API
#include "ecrypt-frogbit-prng.c"
#endif

#include <string.h>

#undef TESTVECTOR_FP
/* #define TESTVECTOR_FP stdout */ /* Un-comment this file to get a test-vector
                                       generation program. */

#if defined(TESTVECTOR_FP)
#include <stdio.h>

#define TEST_VECTOR_VERSION "1.0"

#define TRACE_STATE(IN,OUT) { if (trace_on) { \
fprintf(TESTVECTOR_FP,"%c%c%c(%c%c)%c%c%c[%d%d%d%d%d%d%d%d%d%d]%d\n", \
         "01"[1&(IN)],                                       \
         "01"[s_i_],                                         \
         "01"[1&(OUT)],                                      \
         "01"[1&k],                                          \
         "01"[1&(k>>1)],                                     \
         "01"[ctx->s],                                       \
         "01"[ctx->r_],                                      \
         ((ctx->r_!=0)?"01"[ctx->acc_d]:'-'),                \
         ctx->T[0],ctx->T[1],ctx->T[2],ctx->T[3],ctx->T[4],  \
         ctx->T[5],ctx->T[6],ctx->T[7],ctx->T[8],ctx->T[9],  \
         ctx->d_); } }

static int trace_on;

#endif

/*
 * Key setup. According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_keysetup(
  ECRYPT_AE_ctx* ctx,
  const u8* key,
  u32 keysize,                /* Key size in bits. */
  u32 ivsize,                 /* IV size in bits. */
  u32 macsize)                /* MAC size in bits. */
{
   int id, ik;

   ctx->keysize=keysize;
   ctx->ivsize=ivsize;
   ctx->macsize=macsize;
   memcpy(ctx->mac_secret,key,FROGBIT_MAC_SECRETLEN/CHAR_BIT);

   /* The "Bit Interleaving" part of the key schedule, part 1 of 2:
      place secret key bits in their interleaved positions. */
   memset(ctx->key,0,keysize/CHAR_BIT+ivsize/CHAR_BIT);
   ik=0;
   switch (ctx->ivsize) {
   case ECRYPT_IVSIZE(0): /* 64 */
      for (id=0;id<(ECRYPT_KEYSIZE(0)+ECRYPT_IVSIZE(0));id+=2) {
         ctx->key[id/CHAR_BIT]|=  (1&(key[ik/CHAR_BIT]>>(ik%CHAR_BIT)))
                                <<(id%CHAR_BIT);
         ik++;

         id++;
         if (id>=(ECRYPT_KEYSIZE(0)+ECRYPT_IVSIZE(0)))
            break;

         ctx->key[id/CHAR_BIT]|=  (1&(key[ik/CHAR_BIT]>>(ik%CHAR_BIT)))
                                <<(id%CHAR_BIT);
         ik++;
      }
      break;
   case ECRYPT_IVSIZE(1): /* 128 */
      for (id=0;id<(ECRYPT_KEYSIZE(0)+ECRYPT_IVSIZE(1));id+=2) {
         ctx->key[id/CHAR_BIT]|=  (1&(key[ik/CHAR_BIT]>>(ik%CHAR_BIT)))
                                <<(id%CHAR_BIT);
         ik++;
      }
      break;
   }
}

static int encipher_bit(ECRYPT_AE_ctx* ctx,int m_i_);
static int decipher_bit(ECRYPT_AE_ctx* ctx,int e_i_);

/*
 * IV setup.  According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_ivsetup(
  ECRYPT_AE_ctx* ctx,
  const u8* iv)
{
   int id, ik, iiv;
   ik=0;
   iiv=0;

   /* Core Frogbit state initialization. */
   ctx->s=0;
   ctx->r_=0;
   for (ik=0;ik<FROGBIT10;ik++) {
      ctx->T[ik]=ik;
   }
   ctx->d_=0;

   /* The "Bit Interleaving" part of the key schedule, part 2 of 2:
      place initial value bits in their interleaved positions. */
   switch (ctx->ivsize) {
   case ECRYPT_IVSIZE(0): /* 64 */
      for (id=2;id<(ECRYPT_KEYSIZE(0)+ECRYPT_IVSIZE(0));id+=3) {
         ctx->key[id/CHAR_BIT]= ( ctx->key[id/CHAR_BIT]
                                 &~(1<<(id%CHAR_BIT))
                                )
                               |(  (1&(iv[iiv/CHAR_BIT]>>(iiv%CHAR_BIT)))
                                 <<(id%CHAR_BIT)
                                );
         iiv++;
      }
      break;
   case ECRYPT_IVSIZE(1): /* 128 */
      for (id=1;id<(ECRYPT_KEYSIZE(0)+ECRYPT_IVSIZE(1));id+=2) {
         ctx->key[id/CHAR_BIT]= ( ctx->key[id/CHAR_BIT]
                                 &~(1<<(id%CHAR_BIT))
                                )
                               |(  (1&(iv[iiv/CHAR_BIT]>>(iiv%CHAR_BIT)))
                                 <<(id%CHAR_BIT)
                                );
         iiv++;
      }
      break;
   }

   /* Other parts of the key schedule are implemented elsewhere:
      "Nibble Distribution to First State Word in Each PRNG"
      "Other State Words in Each PRNG" */
   ik=ECRYPT_frogbit_key_shed(ctx->prng,ctx->keysize+ctx->ivsize,ctx->key);

   /* The last part of the key schedule, "Frogbit Encryption of Unused
      Interleaved Bits" */
   for (;ik<ctx->keysize/CHAR_BIT+ctx->ivsize/CHAR_BIT;ik++) {
      for (id=0;id<CHAR_BIT;id++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"I");
         #endif
         encipher_bit(ctx,ctx->key[ik]>>id);
      }
   }
}

/*
 * Encryption.  According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_encrypt_bytes(
  ECRYPT_AE_ctx* ctx,
  const u8* plaintext,
  u8* ciphertext,
  u32 msglen)                /* Message length in bytes. */
{
   u32 i;
   for (i=0;i<msglen;i++) {
      int ibit;
      ciphertext[i]=0;
      for (ibit=0;ibit<CHAR_BIT;ibit++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"C");
         #endif
         ciphertext[i]|=encipher_bit(ctx,plaintext[i]>>ibit)<<ibit;
      }
   }
}

/*
 * Decryption.  According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_decrypt_bytes(
  ECRYPT_AE_ctx* ctx,
  const u8* ciphertext,
  u8* plaintext,
  u32 msglen)                /* Message length in bytes. */
{
   u32 i;
   for (i=0;i<msglen;i++) {
      int ibit;
      plaintext[i]=0;
      for (ibit=0;ibit<CHAR_BIT;ibit++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"D");
         #endif
         plaintext[i]|=decipher_bit(ctx,ciphertext[i]>>ibit)<<ibit;
      }
   }
}

/*
 * Authenticate associated data. According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_authenticate_bytes(
  ECRYPT_AE_ctx* ctx,
  const u8* aad,
  u32 aadlen)                /* Length of associated data in bytes. */
{
   u32 i;
   for (i=0;i<aadlen;i++) {
      int ibit;
      for (ibit=0;ibit<CHAR_BIT;ibit++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"A");
         #endif
         encipher_bit(ctx,aad[i]>>ibit);
      }
   }
}

/*
 * MAC output. According to ECRYPT NoE API requirements.
 */
void ECRYPT_AE_finalize(
  ECRYPT_AE_ctx* ctx,
  u8* mac)
{
   u32 i;
   int ibit;
   for (i=0;i<FROGBIT_MAC_SECRETLEN/CHAR_BIT;i++) {
      for (ibit=0;ibit<CHAR_BIT;ibit++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"Z");
         #endif
         encipher_bit(ctx,ctx->mac_secret[i]>>ibit);
      }
   }
   memset(mac,0,ctx->macsize/CHAR_BIT);
   for (i=0;i<ctx->macsize/CHAR_BIT;i++) {
      for (ibit=0;ibit<CHAR_BIT;ibit++) {
         #if defined(TESTVECTOR_FP)
         if (trace_on) fprintf(TESTVECTOR_FP,"M");
         #endif
         mac[i]|=encipher_bit(ctx,0)<<ibit;
      }
   }
}

/*--------------------------------------------------------------------*/
/*-------------- the Frogbit index permutation process ---------------*/

static void index_permutation(ECRYPT_AE_ctx* ctx,int j)
{
  int temp;
  int temp_d_= ctx->T[j];
  switch((ctx->d_<<1)+ctx->s)
  {
  case 0:
     temp=ctx->T[0];ctx->T[0]=ctx->T[5];ctx->T[5]=ctx->T[2];ctx->T[2]=ctx->T[7];
                    ctx->T[7]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[8];ctx->T[8]=ctx->T[6];ctx->T[6]=ctx->T[4];
                    ctx->T[4]=ctx->T[3];ctx->T[3]=ctx->T[9];ctx->T[9]=temp;
    break;
  case 1:
     temp=ctx->T[0];ctx->T[0]=ctx->T[2];ctx->T[2]=ctx->T[7];ctx->T[7]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[3];ctx->T[3]=ctx->T[6];ctx->T[6]=ctx->T[9];
                    ctx->T[9]=ctx->T[4];ctx->T[4]=ctx->T[8];ctx->T[8]=ctx->T[5];
                    ctx->T[5]=temp;
    break;
  case 2:
     temp=ctx->T[0];ctx->T[0]=ctx->T[6];ctx->T[6]=ctx->T[8];ctx->T[8]=ctx->T[9];
                    ctx->T[9]=ctx->T[5];ctx->T[5]=ctx->T[7];ctx->T[7]=ctx->T[1];
                    ctx->T[1]=temp;
     temp=ctx->T[2];ctx->T[2]=ctx->T[4];ctx->T[4]=ctx->T[3];ctx->T[3]=temp;
    break;
  case 3:
     temp=ctx->T[0];ctx->T[0]=ctx->T[6];ctx->T[6]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[5];ctx->T[5]=ctx->T[3];ctx->T[3]=ctx->T[4];
                    ctx->T[4]=ctx->T[2];ctx->T[2]=ctx->T[8];ctx->T[8]=ctx->T[7];
                    ctx->T[7]=ctx->T[9];ctx->T[9]=temp;
    break;
  case 4:
     temp=ctx->T[0];ctx->T[0]=ctx->T[3];ctx->T[3]=ctx->T[6];ctx->T[6]=ctx->T[5];
                    ctx->T[5]=ctx->T[1];ctx->T[1]=ctx->T[2];ctx->T[2]=temp;
     temp=ctx->T[4];ctx->T[4]=ctx->T[8];ctx->T[8]=ctx->T[7];ctx->T[7]=ctx->T[9];
                    ctx->T[9]=temp;
    break;
  case 5:
     temp=ctx->T[0];ctx->T[0]=ctx->T[6];ctx->T[6]=ctx->T[7];ctx->T[7]=ctx->T[3];
                    ctx->T[3]=ctx->T[1];ctx->T[1]=ctx->T[2];ctx->T[2]=ctx->T[9];
                    ctx->T[9]=temp;
     temp=ctx->T[4];ctx->T[4]=ctx->T[5];ctx->T[5]=ctx->T[8];ctx->T[8]=temp;
    break;
  case 6:
     temp=ctx->T[0];ctx->T[0]=ctx->T[4];ctx->T[4]=ctx->T[3];ctx->T[3]=ctx->T[7];
                    ctx->T[7]=ctx->T[6];ctx->T[6]=ctx->T[1];ctx->T[1]=ctx->T[8];
                    ctx->T[8]=ctx->T[5];ctx->T[5]=temp;
     temp=ctx->T[2];ctx->T[2]=ctx->T[9];ctx->T[9]=temp;
    break;
  case 7:
     temp=ctx->T[0];ctx->T[0]=ctx->T[5];ctx->T[5]=ctx->T[4];ctx->T[4]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[9];ctx->T[9]=ctx->T[8];ctx->T[8]=ctx->T[6];
                    ctx->T[6]=ctx->T[2];ctx->T[2]=ctx->T[3];ctx->T[3]=ctx->T[7];
                    ctx->T[7]=temp;
    break;
  case 8:
     temp=ctx->T[0];ctx->T[0]=ctx->T[8];ctx->T[8]=ctx->T[9];ctx->T[9]=ctx->T[5];
                    ctx->T[5]=ctx->T[3];ctx->T[3]=ctx->T[2];ctx->T[2]=ctx->T[1];
                    ctx->T[1]=temp;
     temp=ctx->T[4];ctx->T[4]=ctx->T[6];ctx->T[6]=ctx->T[7];ctx->T[7]=temp;
    break;
  case 9:
     temp=ctx->T[0];ctx->T[0]=ctx->T[9];ctx->T[9]=ctx->T[7];ctx->T[7]=ctx->T[8];
                    ctx->T[8]=ctx->T[1];ctx->T[1]=ctx->T[4];ctx->T[4]=ctx->T[2];
                    ctx->T[2]=ctx->T[6];ctx->T[6]=temp;
     temp=ctx->T[3];ctx->T[3]=ctx->T[5];ctx->T[5]=temp;
    break;
  case 10:
     temp=ctx->T[0];ctx->T[0]=ctx->T[7];ctx->T[7]=ctx->T[2];ctx->T[2]=ctx->T[4];
                    ctx->T[4]=ctx->T[1];ctx->T[1]=ctx->T[6];ctx->T[6]=ctx->T[5];
                    ctx->T[5]=ctx->T[9];ctx->T[9]=temp;
     temp=ctx->T[3];ctx->T[3]=ctx->T[8];ctx->T[8]=temp;
    break;
  case 11:
     temp=ctx->T[0];ctx->T[0]=ctx->T[1];ctx->T[1]=ctx->T[4];ctx->T[4]=ctx->T[7];
                    ctx->T[7]=ctx->T[5];ctx->T[5]=ctx->T[2];ctx->T[2]=ctx->T[8];
                    ctx->T[8]=ctx->T[3];ctx->T[3]=temp;
     temp=ctx->T[6];ctx->T[6]=ctx->T[9];ctx->T[9]=temp;
    break;
  case 12:
     temp=ctx->T[0];ctx->T[0]=ctx->T[7];ctx->T[7]=ctx->T[6];ctx->T[6]=ctx->T[3];
                    ctx->T[3]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[4];ctx->T[4]=ctx->T[9];ctx->T[9]=ctx->T[2];
                    ctx->T[2]=ctx->T[5];ctx->T[5]=ctx->T[8];ctx->T[8]=temp;
    break;
  case 13:
     temp=ctx->T[0];ctx->T[0]=ctx->T[3];ctx->T[3]=ctx->T[5];ctx->T[5]=ctx->T[2];
                    ctx->T[2]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[9];ctx->T[9]=ctx->T[7];ctx->T[7]=ctx->T[4];
                    ctx->T[4]=ctx->T[6];ctx->T[6]=ctx->T[8];ctx->T[8]=temp;
    break;
  case 14:
     temp=ctx->T[0];ctx->T[0]=ctx->T[8];ctx->T[8]=ctx->T[4];ctx->T[4]=ctx->T[2];
                    ctx->T[2]=ctx->T[1];ctx->T[1]=ctx->T[9];ctx->T[9]=ctx->T[3];
                    ctx->T[3]=temp;
     temp=ctx->T[5];ctx->T[5]=ctx->T[7];ctx->T[7]=ctx->T[6];ctx->T[6]=temp;
    break;
  case 15:
     temp=ctx->T[0];ctx->T[0]=ctx->T[9];ctx->T[9]=ctx->T[6];ctx->T[6]=ctx->T[3];
                    ctx->T[3]=ctx->T[1];ctx->T[1]=ctx->T[5];ctx->T[5]=temp;
     temp=ctx->T[2];ctx->T[2]=ctx->T[4];ctx->T[4]=ctx->T[7];ctx->T[7]=ctx->T[8];
                    ctx->T[8]=temp;
    break;
  case 16:
     temp=ctx->T[0];ctx->T[0]=ctx->T[7];ctx->T[7]=ctx->T[2];ctx->T[2]=ctx->T[6];
                    ctx->T[6]=ctx->T[1];ctx->T[1]=ctx->T[3];ctx->T[3]=ctx->T[9];
                    ctx->T[9]=ctx->T[8];ctx->T[8]=temp;
     temp=ctx->T[4];ctx->T[4]=ctx->T[5];ctx->T[5]=temp;
    break;
  case 17:
     temp=ctx->T[0];ctx->T[0]=ctx->T[4];ctx->T[4]=ctx->T[1];ctx->T[1]=ctx->T[7];
                    ctx->T[7]=ctx->T[8];ctx->T[8]=temp;
     temp=ctx->T[2];ctx->T[2]=ctx->T[9];ctx->T[9]=ctx->T[3];ctx->T[3]=ctx->T[5];
                    ctx->T[5]=ctx->T[6];ctx->T[6]=temp;
    break;
  case 18:
     temp=ctx->T[0];ctx->T[0]=ctx->T[2];ctx->T[2]=ctx->T[3];ctx->T[3]=ctx->T[8];
                    ctx->T[8]=temp;
     temp=ctx->T[1];ctx->T[1]=ctx->T[7];ctx->T[7]=ctx->T[5];ctx->T[5]=ctx->T[6];
                    ctx->T[6]=ctx->T[4];ctx->T[4]=ctx->T[9];ctx->T[9]=temp;
    break;
  case 19:
     temp=ctx->T[0];ctx->T[0]=ctx->T[1];ctx->T[1]=ctx->T[6];ctx->T[6]=ctx->T[7];
                    ctx->T[7]=ctx->T[3];ctx->T[3]=ctx->T[4];ctx->T[4]=temp;
     temp=ctx->T[2];ctx->T[2]=ctx->T[5];ctx->T[5]=ctx->T[9];ctx->T[9]=ctx->T[8];
                    ctx->T[8]=temp;
    break;
  }
  ctx->d_ = temp_d_;
}

/*--------------------------------------------------------------------*/
/*-------------------- the core Frogbit algorithm --------------------*/

static int encipher_bit(ECRYPT_AE_ctx* ctx,int m_i_)
{
   int k;
   int s_i_;
   int base;

   MTGFSR_DRAW(ctx->prng+ctx->d_,k)
   s_i_        = 1&(m_i_^k);

   if (ctx->s!=s_i_)
   if (ctx->r_==0) base=0;
   else            base=(ctx->acc_d+1)<<1;
   else if (ctx->r_!=0) base=(ctx->acc_d+3)<<1;
   else
   {
     ctx->acc_d=1&(k>>1);
     ctx->r_=1;
     #if defined(TESTVECTOR_FP)
     TRACE_STATE(m_i_,s_i_^(k>>1));
     #endif
     return 1&(s_i_^(k>>1));
   }

   index_permutation(ctx,base+(1&(k>>1)));
   ctx->r_= 0;
   ctx->s=s_i_;

   #if defined(TESTVECTOR_FP)
   TRACE_STATE(m_i_,s_i_^(k>>1))
   #endif
   return 1&(s_i_^(k>>1));
}

static int decipher_bit(ECRYPT_AE_ctx* ctx,int e_i_)
{
   int k;
   int s_i_;
   int base;

   MTGFSR_DRAW(ctx->prng+ctx->d_,k)
   s_i_        = 1&(e_i_^(k>>1));

   if (ctx->s!=s_i_)
   if (ctx->r_==0) base=0;
   else            base=(ctx->acc_d+1)<<1;
   else if (ctx->r_!=0) base=(ctx->acc_d+3)<<1;
   else
   {
     ctx->acc_d=1&(k>>1);
     ctx->r_=1;
     #if defined(TESTVECTOR_FP)
     TRACE_STATE(s_i_^k,e_i_)
     #endif
     return 1&(s_i_^k);
   }

   index_permutation(ctx,base+(1&(k>>1)));
   ctx->r_= 0;
   ctx->s=s_i_;

   #if defined(TESTVECTOR_FP)
   TRACE_STATE(s_i_^k,e_i_)
   #endif
   return 1&(s_i_^k);
}

#if defined(TESTVECTOR_FP)

/*--------------------------------------------------------------------*/
/*---------- A utility function for test vector generation -----------*/

int printraw(FILE *fp, const char *prefix, const u8 *buf, int len)
{
   int return_value;
   int i;
   unsigned char trbuf[1000];
   return_value=strlen(prefix);
   if (return_value>(sizeof(trbuf)-1)) {
      return_value=sizeof(trbuf)-1;
   }
   memcpy(trbuf,prefix,return_value);
   if (len>((sizeof(trbuf)-1-return_value)/2)) {
      len=(sizeof(trbuf)-1-return_value)/2;
   }
   for (i=0;i<len;i++) {
      trbuf[return_value++]="0123456789ABCDEF"[buf[i]>>4];
      trbuf[return_value++]="0123456789ABCDEF"[buf[i]&0x0F];
   }
   trbuf[return_value]=0;
   fprintf(fp,"%s\n",trbuf);
   return return_value;
}

/*--------------------------------------------------------------------*/
/*------------------ Test vector generation program ------------------*/

int main(int argc, char *argv[])
{
   int i;
   static const u8 Key[ECRYPT_MAXKEYSIZE/CHAR_BIT]=
        {0x24,0x3F,0x6A,0x88,0x85,0xA3,0x08,0xD3,
         0x13,0x19,0x8A,0x2E,0x03,0x70,0x73,0x44};
   static const u8 Iv[2][ECRYPT_MAXIVSIZE/CHAR_BIT]=
       {{0xA4,0x09,0x38,0x22,0x29,0x9F,0x31,0xD0,
         0x08,0x2E,0xFA,0x98,0xEC,0x4E,0x6C,0x89}
       ,{0x45,0x28,0x21,0xE6,0x38,0xD0,0x13,0x77,
         0xBE,0x54,0x66,0xCF,0x34,0xE9,0x0C,0x6C}};
   #define AUTH_LEN (7)
   static const u8 auth_data[AUTH_LEN+1]="FROGBIT";
   #define TEXT_LEN (24)
   static const u8 text_data[TEXT_LEN+1]="HYDROCHARIS MORSUS-RANAE";

   struct {
      int klen;
      int ivlen;
      int maclen;
      int iv_ind;
   } tests[] =
   {{ECRYPT_KEYSIZE(0),ECRYPT_IVSIZE(0),ECRYPT_MACSIZE(0),0}
   ,{ECRYPT_KEYSIZE(0),ECRYPT_MAXIVSIZE,ECRYPT_MACSIZE(0),0}
   ,{ECRYPT_KEYSIZE(0),ECRYPT_IVSIZE(0),ECRYPT_MAXMACSIZE,1}
   ,{ECRYPT_KEYSIZE(0),ECRYPT_MAXIVSIZE,ECRYPT_MAXMACSIZE,1}
   };

   fprintf(TESTVECTOR_FP,"Frogbit cipher test vectors, version "
                           TEST_VECTOR_VERSION "\n\n");

   printraw(TESTVECTOR_FP,"Secret key: ",Key,ECRYPT_KEYSIZE(0)/CHAR_BIT);
   fprintf(TESTVECTOR_FP,"Authenticated text: \"%s\"\n",auth_data);
   printraw(TESTVECTOR_FP,"Authenticated text: ",auth_data,AUTH_LEN);
   fprintf(TESTVECTOR_FP,"Plaintext: \"%s\"\n",text_data);
   printraw(TESTVECTOR_FP,"Plaintext: ",text_data,TEXT_LEN);

   for (i=0;i<2*(sizeof(tests)/sizeof(tests[0]));i++) {

      int klen, ivlen, maclen, iv_ind;
      ECRYPT_AE_ctx ctx;
      u8 output_data[TEXT_LEN];
      u8 recov_data[TEXT_LEN];
      u8 mac_value_en[ECRYPT_MAXMACSIZE/CHAR_BIT];
      u8 mac_value_de[ECRYPT_MAXMACSIZE/CHAR_BIT];

      klen=  tests[i%(sizeof(tests)/sizeof(tests[0]))].klen;
      ivlen= tests[i%(sizeof(tests)/sizeof(tests[0]))].ivlen;
      maclen=tests[i%(sizeof(tests)/sizeof(tests[0]))].maclen;
      iv_ind=tests[i%(sizeof(tests)/sizeof(tests[0]))].iv_ind;

      trace_on=i>=(sizeof(tests)/sizeof(tests[0]));

      fprintf(TESTVECTOR_FP,"\nTest %d, Authenticate-encrypt test, "
                  "key length %d, "
                  "IV length %d, "
                  "MAC length %d.\n"
                  ,i+1,klen,ivlen,maclen);
      printraw(TESTVECTOR_FP,"Initial value: ",Iv[iv_ind],ivlen/CHAR_BIT);

      ECRYPT_AE_keysetup(
         &ctx,
         Key,
         klen,
         ivlen,
         maclen);

      ECRYPT_AE_encrypt_packet(
         &ctx,
         Iv[iv_ind],
         auth_data,AUTH_LEN,
         text_data,output_data,TEXT_LEN,
         mac_value_en);

      printraw(TESTVECTOR_FP,"Ciphertext: ",output_data,TEXT_LEN);
      printraw(TESTVECTOR_FP,"MAC value: ",mac_value_en,maclen/CHAR_BIT);

      trace_on=0;

      ECRYPT_AE_decrypt_packet(
         &ctx,
         Iv[iv_ind],
         auth_data,AUTH_LEN,
         output_data,recov_data,TEXT_LEN,
         mac_value_de);

      if (0!=memcmp(text_data,recov_data,TEXT_LEN)) {
         fprintf(TESTVECTOR_FP,"Decryption error.\n");
      }
      else if (0!=memcmp(mac_value_en,mac_value_de,maclen/CHAR_BIT)) {
         fprintf(TESTVECTOR_FP,"Authentication error.\n");
      }
      else {
         fprintf(TESTVECTOR_FP,"Decrypt-verify done.\n");
      }
   }
}
#endif

