/*******************************************************************************

                 Synchronous Stream Cipher :  DICING

                         author:  Li An-Ping


*******************************************************************************/
#include "ecrypt-sync.h"
#include "ecrypt-portable.h"


#define  SL  64


static const u32 sbox[256]={
0xd5,0xbd,0x5,0x0,0xf0,0x68,0x3,0xb3,0xe5,0x6b,
0xa3,0xef,0x92,0x3b,0x36,0xdb,0xc7,0x98,0x1,0xe8,
0xb9,0xf1,0x7a,0xb0,0x50,0x4f,0xbf,0x34,0x4e,0xf9,
0xfd,0x78,0x2c,0xf8,0x59,0xc6,0x82,0x8c,0x2b,0xe0,
0x55,0x3f,0xb7,0x84,0x85,0xf6,0x61,0xc3,0xaf,0x20,
0x2f,0xdc,0x6f,0xc8,0xb5,0x1b,0x8b,0xc,0x12,0xac,
0xdd,0xe3,0x1f,0x49,0x26,0xba,0xf7,0x74,0x97,0x21,
0x60,0xb1,0xb6,0xf,0x4d,0x4c,0x5b,0x8e,0xd1,0xd2,
0x69,0xaa,0x67,0x58,0xd9,0x75,0xde,0x3d,0x47,0xa9,
0x83,0xc9,0x9c,0xa0,0x11,0xed,0x3a,0x4a,0x48,0x1a,
0xca,0x57,0xfb,0xee,0x5d,0x39,0x8a,0x96,0x13,0xf5,
0xf2,0x28,0xe9,0xe4,0x62,0x3c,0x30,0xfc,0x5f,0xcf,
0xa1,0xd3,0x66,0xcd,0xfa,0xe2,0xb4,0x27,0xd7,0x15,
0x6a,0x63,0x33,0x38,0x8,0x9d,0xd8,0x51,0xe7,0x7c,
0xe1,0x44,0x6d,0x16,0xa2,0x88,0x2a,0x70,0x5a,0x52,
0x73,0xa4,0x71,0x2d,0xfe,0x46,0x7d,0x29,0xec,0x41,
0x1e,0x7f,0x17,0x42,0x31,0x23,0x37,0xea,0x72,0x89,
0x94,0xae,0xc5,0xa7,0xab,0x9b,0xd6,0x76,0x19,0xd0,
0x9e,0x91,0x53,0x81,0x7e,0x8f,0x93,0x7b,0x18,0xa5,
0x40,0xf3,0x4b,0x35,0x2e,0x6e,0x45,0x80,0x32,0xa6,
0xad,0xda,0xd4,0x10,0x9f,0xbb,0x54,0xe6,0x14,0x4,
0x7,0xbc,0x79,0xff,0x43,0xeb,0xcb,0xa8,0x5c,0x64,
0xb8,0x1c,0xe,0x86,0xd,0xc2,0xb2,0x56,0x24,0x3e,
0x5e,0x9,0x25,0x6c,0xa,0x6,0x1d,0x99,0x2,0xf4,
0x77,0x87,0x90,0x95,0xcc,0xb,0xc4,0xbe,0x9a,0xc1,
0x8d,0xc0,0x65,0x22,0xce,0xdf
};

const u8 ct[32]={
0x9a,0x4,0x4d,0xcc,0x2c,0x81,0xf9,0x28,
0x65,0x87,0xc0,0x50,0x28,0x25,0x41,0xe1,
0x4,0x94,0x95,0xa3,0xc6,0x9e,0x39,0xa5,
0xbf,0x93,0xb9,0x92,0xb5,0x61,0x8e,0xf3
};


void stream(ECRYPT_ctx* ctx,u8 *rkey);
void extendsbox(ECRYPT_ctx* ctx);


void ECRYPT_init()
{

}


void ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
   int i,length;
   ctx->key_size=keysize;
	ctx->iv_size=ivsize;
   length = keysize>>3;

	for(i=0;i<length;i+=4)
   *(ctx->key+(i>>2))=U8TO32_LITTLE(key+i);

   if(keysize==128)
   {
   for(i=0;i<4;i++)
   *(ctx->key+4+i)=~*(ctx->key+i);
   }


   if(keysize==256)
   {
      for(i=0;i<16;i+=4)
      *((u32*)(ctx->mkey+i))=U8TO32_LITTLE(key+i)^U8TO32_LITTLE(key+16+i);
   }
   else
   {
      for(i=0;i<16;i+=4)
      *((u32*)(ctx->mkey+i))=U8TO32_LITTLE(key+i);
   }

   extendsbox(ctx);

}



void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{

ctx->cyl=SL;

int i,k;

for(i=0;i<SL;i+=4)
{
*((u32*)(ctx->skey1+i))=0;
*((u32*)(ctx->skey2+i))=0;
}

u32 x[8]={0};
for(i=0;i<4;i++)
{
x[i]=~*(ctx->key+4+i);
x[4+i]=~*(ctx->key+i);
}

u8 u[32]={0},v[32]={0};
for(i=0;i<32;i+=4)
*((u32*)(u+i))=U8TO32_LITTLE(iv+i);

k=0;

do{

for(i=0;i<32;i+=4)
*((u32*)(v+i))=*((u32*)(u+i))^U8TO32_LITTLE(ct+i);

u8 *ptr1,*ptr2;

switch(k){

case 0:
ptr1=(u8*)(ctx->ch);

break;

case 1:
ptr1=(u8*)(ctx->var1);
ptr2=(u8*)(ctx->var2);
break;

case 2:
ptr1=ctx->skey1+SL;
ptr2=ctx->skey2+SL;
break;

case 3:
ptr1=(u8*)(ctx->ckey1);
ptr2=(u8*)(ctx->ckey2);
break;

}


u8 *pt=v;
for(i=0;i<32;i+=8)
{
*((u32*)(u+i))=ctx->sbox0[*(pt)]^ctx->sbox1[*(pt+4)]^ctx->sbox2[*(pt+8)]^ctx->sbox3[*(pt+12)]^*(ctx->key+(i>>2));
*((u32*)(u+i+4))=ctx->sbox0[*(pt+16)]^ctx->sbox1[*(pt+20)]^ctx->sbox2[*(pt+24)]^ctx->sbox3[*(pt+28)]^*(ctx->key+(i>>2)+1);
pt++;
}

pt=u;
for(i=0;i<32;i+=8)
{
*((u32*)(v+i))=ctx->sbox0[*(pt)]^ctx->sbox1[*(pt+4)]^ctx->sbox2[*(pt+8)]^ctx->sbox3[*(pt+12)]^*(x+(i>>2));
*((u32*)(v+i+4))=ctx->sbox0[*(pt+16)]^ctx->sbox1[*(pt+20)]^ctx->sbox2[*(pt+24)]^ctx->sbox3[*(pt+28)]^*(x+(i>>2)+1);
pt++;
}

pt=v;
for(i=0;i<32;i+=8)
{
*((u32*)(u+i))=ctx->sbox0[*(pt)]^ctx->sbox1[*(pt+4)]^ctx->sbox2[*(pt+8)]^ctx->sbox3[*(pt+12)];
*((u32*)(u+i+4))=ctx->sbox0[*(pt+16)]^ctx->sbox1[*(pt+20)]^ctx->sbox2[*(pt+24)]^ctx->sbox3[*(pt+28)];
pt++;
}

if(k==0)
{
for(i=0;i<16;i+=4)
*((u32*)(ptr1+i))=*((u32*)(u+i))^*((u32*)(u+16+i));
}

else{
for(i=0;i<16;i+=4)
{
*((u32*)(ptr1+i))=*((u32*)(u+i));
*((u32*)(ptr2+i))=*((u32*)(u+16+i));
}
}


k++;

}while(k<4);


*(ctx->skey1+79)&=127;
*(ctx->skey2+79)&=63;


for(i=0;i<4;i++)
{
if(*(ctx->ckey1+i)>0)break;
if(*(ctx->ckey2+i)>0)break;
}

if(i==4){
*(ctx->ckey1)=*(ctx->key  ),*(ctx->ckey1+1)=*(ctx->key+1),*(ctx->ckey1+2)=*(ctx->key+2),*(ctx->ckey1+3)=*(ctx->key+3);
*(ctx->ckey2)=*(ctx->key+4),*(ctx->ckey2+1)=*(ctx->key+5),*(ctx->ckey2+2)=*(ctx->key+6),*(ctx->ckey2+3)=*(ctx->key+7);
}


}


void ECRYPT_keystream_bytes(  ECRYPT_ctx* ctx, u8* keystream, u32 length)
{

   while(length>=16){
           stream(ctx,keystream);
           length-=16;
           keystream+=16;

   }

   while(length>0){
         u8 temp[16]={0};
         int i;
            stream(ctx,temp);
            for(i=0;i<length;i++)
            keystream[i]=temp[i];
            length=0;

   }

}



void ECRYPT_encrypt_bytes(ECRYPT_ctx* ctx,const u8* plaintext,u8* ciphertext,u32 msglen)
{
   u8 temp[16];
   int i;
   while(msglen>=16){
             stream(ctx,temp);
             for(i=0;i<16;i+=4)
             *((u32*)(ciphertext+i))=*((u32*)(plaintext+i))^*((u32*)(temp+i));
             msglen-=16;
             plaintext+=16;
             ciphertext+=16;


   }

   while(msglen>0){
             stream(ctx,temp);
             for(i=0;i<msglen;i++)
             *(ciphertext+i)=*(plaintext+i)^*(temp+i);
             msglen=0;

   }

}

void ECRYPT_decrypt_bytes(ECRYPT_ctx* ctx,const u8* ciphertext,u8* plaintext,u32 msglen)
{
   u8 temp[16];
   int i;
   while(msglen>=16){
             stream(ctx,temp);
             for(i=0;i<16;i+=4)
             *((u32*)(plaintext+i))=*((u32*)(ciphertext+i))^*((u32*)(temp+i));
             msglen-=16;
             plaintext+=16;
             ciphertext+=16;
   }

   while(msglen>0){
            stream(ctx,temp);
            for(i=0;i<msglen;i++)
            *(plaintext+i)=*(ciphertext+i)^*(temp+i);
            msglen=0;

   }

}



/*******************************************************************************

                              keystream function

*******************************************************************************/


void stream(ECRYPT_ctx* ctx,u8 *rkey)
{


int i;

             /*......Updading States......*/

if(ctx->cyl==0)
{
for(i=0;i<16;i+=4)
{
*((u32*)(ctx->skey1+i+SL))=*((u32*)(ctx->skey1+i));
*((u32*)(ctx->skey2+i+SL))=*((u32*)(ctx->skey2+i));

*((u32*)(ctx->skey1+i))=0;
*((u32*)(ctx->skey2+i))=0;

}

ctx->cyl=SL;
}

ctx->cyl--;


u32 d,temp;

u8 *pt;

pt=ctx->skey1+ctx->cyl;

temp=(pt[15]>>7)|(pt[16]<<1);

d=temp;

temp^=(temp<<3);

pt[0]^=(u8)(temp);
pt[1]^=(u8)(temp>>8);

temp<<=1;

pt[5]^=(u8)(temp);
pt[11]^=(u8)(temp);

temp>>=8;

pt[6]^=(u8)(temp);
pt[12]^=(u8)(temp);

pt[15]&=127;
pt[16]=0;


pt=ctx->skey2+ctx->cyl;

temp=(pt[15]>>6)|(pt[16]<<2);
d^=temp;
temp^=(temp<<7);

pt[0]^=(u8)(temp);
pt[1]^=(u8)(temp>>8);

temp<<=3;

pt[4]^=(u8)(temp);
pt[10]^=(u8)(temp);


temp>>=8;
pt[5]^=(u8)(temp);
pt[11]^=(u8)(temp);

temp>>=8;
pt[6]^=(u8)(temp);
pt[12]^=(u8)(temp);


pt[15]&=63;
pt[16]=0;


int n;
n=1+(d&15);

u32 *ptr=ctx->ckey1+3;

temp = *(ptr)>>(32-n);

temp = temp^(temp<<3);

*(ptr )  = ((*(ptr)  <<n)|(*(ptr-1)>>(32-n)))^temp;
*(ptr-1) = ((*(ptr-1)<<n)|(*(ptr-2)>>(32-n)))^(temp<<3);
*(ptr-2) = ((*(ptr-2)<<n)|(*(ptr-3)>>(32-n)))^temp;
*(ptr-3) = (*(ptr-3)<<n)^temp;


n=1+(d>>4);

ptr=ctx->ckey2+3;

temp = *(ptr)>>(32-n);

temp=temp^(temp<<5)^(temp<<7);

*(ptr)   = ((*(ptr)  <<n)|(*(ptr-1) >>(32-n)))^temp;
*(ptr-1) = ((*(ptr-1)<<n)|(*(ptr-2) >>(32-n)))^temp;
*(ptr-2) = ((*(ptr-2)<<n)|(*(ptr-3) >>(32-n)))^(temp<<5);
*(ptr-3) = (*(ptr-3)<<n)^temp;



*(ctx->var1)  ^=*(ctx->ckey1);
*(ctx->var1+1)^=*(ctx->ckey1+1);
*(ctx->var1+2)^=*(ctx->ckey1+2);
*(ctx->var1+3)^=*(ctx->ckey1+3);
*(ctx->var2)  ^=*(ctx->ckey2);
*(ctx->var2+1)^=*(ctx->ckey2+1);
*(ctx->var2+2)^=*(ctx->ckey2+2);
*(ctx->var2+3)^=*(ctx->ckey2+3);



                /*  ......  Updating state end  ......  */

                  /*......Combining sub-process......*/



u32 c0,c1,c2,c3;
u32 d0,d1,d2,d3;


d0=*(ctx->var1),d1=*(ctx->var1+1),d2=*(ctx->var1+2),d3=*(ctx->var1+3);

c0=(ctx->sbox0[d0&0xff])^(ctx->sbox1[(d0>>8)&0xff])^(ctx->sbox2[(d0>>16)&0xff])^(ctx->sbox3[d0>>24])^*(ctx->var2);

c1=(ctx->sbox0[d1&0xff])^(ctx->sbox1[(d1>>8)&0xff])^(ctx->sbox2[(d1>>16)&0xff])^(ctx->sbox3[d1>>24])^*(ctx->var2+1);

c2=(ctx->sbox0[d2&0xff])^(ctx->sbox1[(d2>>8)&0xff])^(ctx->sbox2[(d2>>16)&0xff])^(ctx->sbox3[d2>>24])^*(ctx->var2+2);

c3=(ctx->sbox0[d3&0xff])^(ctx->sbox1[(d3>>8)&0xff])^(ctx->sbox2[(d3>>16)&0xff])^(ctx->sbox3[d3>>24])^*(ctx->var2+3);

pt=rkey;
d0=(ctx->sbox0[c0&0xff])^(ctx->sbox1[c1&0xff])^(ctx->sbox2[c2&0xff])^(ctx->sbox3[c3&0xff])^*(ctx->ch);
U32TO8_LITTLE(pt,d0);
c0>>=8,c1>>=8,c2>>=8,c3>>=8;
d0=(ctx->sbox0[c0&0xff])^(ctx->sbox1[c1&0xff])^(ctx->sbox2[c2&0xff])^(ctx->sbox3[c3&0xff])^*(ctx->ch+1);
U32TO8_LITTLE(pt+4,d0);
c0>>=8,c1>>=8,c2>>=8,c3>>=8;
d0=(ctx->sbox0[c0&0xff])^(ctx->sbox1[c1&0xff])^(ctx->sbox2[c2&0xff])^(ctx->sbox3[c3&0xff])^*(ctx->ch+2);
U32TO8_LITTLE(pt+8,d0);
c0>>=8,c1>>=8,c2>>=8,c3>>=8;
d0=(ctx->sbox0[c0])^(ctx->sbox1[c1])^(ctx->sbox2[c2])^(ctx->sbox3[c3])^*(ctx->ch+3);
U32TO8_LITTLE(pt+12,d0);



}


/*******************************************************************************

                         extending S-boxes function

*******************************************************************************/


void extendsbox(ECRYPT_ctx* ctx)
{

int i,k;
u8 x,z;

u8 mkey[16]={0},w[16]={0},y[16]={0};

for(i=0;i<16;i+=4)
*((u32*)(mkey+i))=*((u32*)(ctx->mkey+i));


x=1,z=254;
for(i=0;i<8;i++)
{
w[i]=(x^(mkey[i]&z));
y[i]=(x^(mkey[8+i]&z));
x<<=1,z^=x;
}


for(k=7;k>=0;k--)
{
x=(u8 )(~mkey[k]),z=(u8 )(~mkey[8+k]);

for(i=0;i<k;i++)
{
*(w+k)^=*(w+i+((x&1)<<3)),*(y+k)^=*(y+i+((z&1)<<3));
x>>=1,z>>=1;
}

}

u8 c1=0,c2=0;

x=1;

for(i=0;i<8;i++)
{
c1^=(mkey[i]&x),c2^=(mkey[8+i]&x);
x<<=1;
}

x=(u8)((c2<<1)^(c2>>7));

c2^=c1;
c1^=x;

u32 tabl[256]={0};

tabl[0]=c2|(c2<<16)|(c2<<24);

int n=1;
for(k=0;k<8;k++)
{
x= *(w+k),z=*(y+k);

u32 temp=x|(z<<8)|(x<<16)|((x^z)<<24);

u32 *pt=tabl+n;

for(i=0;i<n;i++)
{
*(pt+i)=*(tabl+i)^temp;
}
n<<=1;
}

const u32 *pt;
pt=sbox;

c2=(u8)(~c1);

for(k=0;k<256;k++)
{
int m=k^c1;

u32 v=tabl[*(pt)^c2];

*(ctx->sbox0+m)=v;
*(ctx->sbox1+m)=ROTR32(v,8);
*(ctx->sbox2+m)=ROTR32(v,16);
*(ctx->sbox3+m)=ROTR32(v,24);

pt++;

}

}



