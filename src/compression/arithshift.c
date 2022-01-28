// modified by Luigi Auriemma
/*   Multiply-free Arithmetic Coding Implementation
 
 
     Copyright Feb. 1993
 
     Gordon V. Cormack  Feb. 1993
     University of Waterloo
     cormack@uwaterloo.ca
 
 
     All rights reserved.
 
     This code and the algorithms herein are the property of Gordon V. Cormack.
 
     Neither the code nor any algorithm herein may be included in any software,
     device, or process which is sold, exchanged for profit, or for which a
     licence or royalty fee is charged.
 
     Permission is granted to use this code for educational, research, or
     commercial purposes, provided this notice is included, and provided this
     code is not used as described in the above paragraph.
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../extra/mem2mem.h"

#define shrt short

static shrt *diff = NULL;
static shrt *inc0tab = NULL;
static shrt *inc1tab = NULL;
static shrt *cnt = NULL;



#define INC0(x) inc0tab[(unsigned shrt)(x)]
#define INC1(x) inc1tab[(unsigned shrt)(x)]
#define DIFF(x) diff[(unsigned shrt)(x)]
#define CNT1(x) ((x&0xff)+1)
#define CNT0(x) ((x>>8)+1)

static void arithshift_init() {
    if(!diff)       diff    = calloc(0x10000, sizeof(shrt));
    if(!inc0tab)    inc0tab = calloc(0x10000, sizeof(shrt));
    if(!inc1tab)    inc1tab = calloc(0x10000, sizeof(shrt));
    if(!cnt)        cnt     = calloc(0x100,   sizeof(shrt));

   int i,j;
   for (i=0;i<256;i++) for (j=0;j<256;j++) {
     if (i != 255)
        inc0tab[(i<<8)+j] = ((i+1)<<8) + j;
     else
        inc0tab[(i<<8)+j] = (127<<8) + (j>>1);
     if (j != 255)
        inc1tab[(i<<8)+j] = (i<<8) + j + 1;
     else
        inc1tab[(i<<8)+j] = ((i>>1)<<8) + 127;
     if (i < j) {
        diff[(i<<8)+j] = 175.0 * (i+1) / (i+j+2);
        if (!diff[(i<<8)+j]) diff[(i<<8)+j]++;
     }else{
        diff[(i<<8)+j] = -175.0 * (j+1) / (i+j+2);
        if (!diff[(i<<8)+j]) diff[(i<<8)+j]--;
     }
   }
}



int arithshift_compress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    mem2mem_init(in, insz, out, outsz);

int count = -8;
unsigned int mask;
 int space = 0xff,
    min = 0,
   index,
    i;
unsigned char c=0,gc = 0;
unsigned int last = 0;
unsigned int val;

arithshift_init();

val = getchar();
gc = getchar();

do{
   c = 0;
   index = 1;
   for(mask=0x80;mask;mask>>=1){
         int l = last  + index;
         int a = cnt[l];
         int x = DIFF(a);
         if (x>0) 
            if (val < min+space-x) { 
               c |= mask;
               space -=x;
               cnt[l] = INC1(a);
               index += index + 1;
            }else { 
               min += space-x;
               space = x;
               cnt[l] = INC0(a);
               index += index;
            }
         else 
            if (val < min+space+x) {
               space +=x; 
               cnt[l] = INC0(a);
               index += index;
            } else { 
               c |= mask;
              min += space+x; 
              space = -x;
              cnt[l] = INC1(a);
              index += index+1;
            }
      while ((space) < 128) {
         space <<= 1;
         min <<= 1;
         val = (val<<1) | (gc>>(7-(count&7)));
         if (!++count) {
            count = -8;
            gc = getchar();
            min &= 0xffffff;
            val &= 0xffffff;
            if (0xffffff - min < space){
                space = 0xffffff - min;
                }
            }
         }
      }
   if (c == (unsigned char)EOF) break;
   putchar(c);
   }
while (1);

    return mem2mem_ret();
}



int arithshift_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    mem2mem_init(in, insz, out, outsz);

 int space = 0xff,
    min = 0,
   index,
    i;
unsigned  char c = 0;

unsigned int last = 0;
int count = -24;

arithshift_init();

do{
   int mask;

   c = 0xff & getchar();
   index = 1;
   for (mask = 0x80;mask;mask>>=1){
         int l = last  + index;
         int a = cnt[l];
         int x = DIFF(a);
         if (x>0) 
            if (c & mask) { 
               space -=x;
               cnt[l] = INC1(a);
               index += index + 1;
            }else { 
               min += space-x;
               space = x;
               cnt[l] = INC0(a);
               index += index;
            }
         else 
            if (!(c & mask)) {
               space +=x; 
               cnt[l] = INC0(a);
               index += index;
            } else { 
              min += space+x; 
              space = -x;
              cnt[l] = INC1(a);
              index += index+1;
            }
      while ((space) < 128) {
         space <<= 1;
         min <<= 1;
         if (!++count) {
            count = -8;
            putchar(min >> 24);
            min &= 0xffffff;
            if (0xffffff - min < space){
                space = 0xffffff - min;
                }
            }
         }
      }
   }
while (c != (unsigned char)EOF);
fprintf(stderr,"count %d min %x minshift %x\n",count&7,min,min<<(8-(count&0x7)));
min <<= 8-(count&7);
putchar(min>>24);
putchar((min>>16) & 0xff);
putchar((min>>8) & 0xff);
putchar(min & 0x00ff);

    return mem2mem_ret();
}

