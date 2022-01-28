#include <stdio.h>
#include <stdlib.h>

#include "ecrypt-Moustique_cipher.c"

int main(int argc, char *argv[])
{
  ECRYPT_ctx s;
  u8 key[12];
  u8 IV[13];
  u8 *previous;
  u8 ip[16];
  u8 op[16];
  u8 funct;
  u16 i,j;
  char finame[40], foname[40];
  FILE *in1, *out1;
  init_ctx(&s);
  printf("give name of inputfile\n") ;
  scanf("%s", finame) ;
  if(( in1=fopen(finame,"r")) == NULL ) printf("problem\n");
  printf("give name of outputfile\n") ;
  scanf("%s", foname) ;
  if(( out1=fopen(foname,"w")) == NULL ) printf("problem\n");
/*  printf("if encryption type 1, if decryption type 0, to stop type anything else\n");
*/
  fscanf(in1,"%d",&funct);
  while( funct < 2 )
  {
/*     printf("give key (12 bytes in hex)\n");
*/
     fscanf(in1,"%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
     key,key+1,key+2,key+3,key+4,key+5,key+6,key+7,key+8,key+9,key+10,key+11);
/*     printf("give IV (13 bytes in hex)\n");
*/
     fscanf(in1,"%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
     IV,IV+1,IV+2,IV+3,IV+4,IV+5,IV+6,IV+7,IV+8,IV+9,IV+10,IV+11,IV+12);
/*     if (funct)
     {
        printf("give plaintext (16 bytes in hex)\n");
     }
     else
     {
         printf("give ciphertext (16 bytes in hex)\n");
     }
*/
     fscanf(in1,"%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
     ip,ip+1,ip+2,ip+3,ip+4,ip+5,ip+6,ip+7,ip+8,ip+9,ip+10,ip+11,ip+12,ip+13,ip+14,ip+15);
     ECRYPT_init();
     ECRYPT_keysetup(&s,key,96,104);
     ECRYPT_ivsetup(&s,previous,IV);
     fprintf(out1,"key:        ");
     fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
     key[0],key[1],key[2],key[3],key[4],key[5],key[6],key[7],key[8],key[9],key[10],key[11]);  
     fprintf(out1,"IV:         ");
     fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
     IV[0],IV[1],IV[2],IV[3],IV[4],IV[5],IV[6],IV[7],IV[8],IV[9],IV[10],IV[11],IV[12]);  
     if (funct)
     {
        ECRYPT_encrypt_bytes(&s,previous,ip,op,16);
        fprintf(out1,"plaintext:  ");
        fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        ip[0],ip[1],ip[2],ip[3],ip[4],ip[5],ip[6],ip[7],ip[8],ip[9],ip[10],ip[11],ip[12],ip[13],ip[14],ip[15]);
        fprintf(out1,"ciphertext: ");
        fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        op[0],op[1],op[2],op[3],op[4],op[5],op[6],op[7],op[8],op[9],op[10],op[11],op[12],op[13],op[14],op[15]);    
     }
     else
     {
         ECRYPT_decrypt_bytes(&s,previous,ip,op,16);
         fprintf(out1,"ciphertext: ");
         fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
         ip[0],ip[1],ip[2],ip[3],ip[4],ip[5],ip[6],ip[7],ip[8],ip[9],ip[10],ip[11],ip[12],ip[13],ip[14],ip[15]);
         fprintf(out1,"plaintext:  ");
         fprintf(out1,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
         op[0],op[1],op[2],op[3],op[4],op[5],op[6],op[7],op[8],op[9],op[10],op[11],op[12],op[13],op[14],op[15]);    
     }
/*  printf("if encryption type 1, if decryption type 0, to stop type anything else\n");
*/
  fprintf(out1,"\n");
  fscanf(in1,"%d",&funct);
  }
fclose(in1);
fclose(out1);
}
