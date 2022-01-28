/*   Dynamic Markov Compression (DMC)    Version 0.0.0
 
 
     Copyright 1993, 1987
 
     Gordon V. Cormack
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

/*    This program implements DMC as described in

      "Data Compression using Dynamic Markov Modelling",
      by Gordon Cormack and Nigel Horspool
      in Computer Journal 30:6 (December 1987)

      It uses floating point so it isn't fast.  Converting to fixed point
      isn't too difficult.

      comp() and exp() implement Guazzo's version of arithmetic coding.

      pinit(), predict(), and pupdate() are the DMC predictor.

      pflush() reinitializes the DMC table and reclaims space

      preset() starts the DMC predictor at its start state, but doesn't
               reinitialize the tables.  This is used for packetized
               communications, but not here.

*/

#include <stdio.h>
#include <stdlib.h>

int memsize_g = 0x1000000;

typedef struct nnn {
           float count[2];
           struct nnn    *next[2];
} node;

static int threshold = 2, bigthresh = 2; 

static node *p, *new, nodes[256][256];

static node *nodebuf    = NULL;
static node *nodemaxp   = NULL;
static node *nodesptr   = NULL;

void preset(){
   p = &nodes[0][0];
}

float predict(){
   return   p->count[0] / (p->count[0] + p->count[1]);
}

void pflush(){
   int i,j;
   for (j=0;j<256;j++){
      for (i=0;i<127;i++) {
         nodes[j][i].count[0] = 0.2;
         nodes[j][i].count[1] = 0.2;
         nodes[j][i].next[0] = &nodes[j][2*i + 1];
         nodes[j][i].next[1] = &nodes[j][2*i + 2];
      }
      for (i=127;i<255;i++) {
         nodes[j][i].count[0] = 0.2;
         nodes[j][i].count[1] = 0.2;
         nodes[j][i].next[0] = &nodes[i+1][0];
         nodes[j][i].next[1] = &nodes[i-127][0];
      }
   }
   nodesptr = nodebuf;
   preset();
}
int pinit(memsize)
   int memsize;
{
   //fprintf(stderr,"using %d bytes of predictor memory\n",memsize);
   if(!nodebuf) nodebuf = (node *) malloc (memsize);
   if (nodebuf == (node *) NULL) {
      //fprintf(stderr,"memory alloc failed; try smaller predictor memory\n");
      return(-1);
   }
   nodemaxp = nodebuf + (memsize/sizeof(node)) - 20;
   pflush();
   return(0);
}

void pupdate(b)
   int b;
{
   float r;
   if (p->count[b] >= threshold &&
      p->next[b]->count[0]+p->next[b]->count[1]
       >= bigthresh + p->count[b]){
      new = nodesptr++;
      p->next[b]->count[0] -= new->count[0] =
         p->next[b]->count[0] * 
         (r = p->count[b]/(p->next[b]->count[1]+p->next[b]->count[0]));
      p->next[b]->count[1] -= new->count[1] =
         p->next[b]->count[1] * r;
      new->next[0] = p->next[b]->next[0];
      new->next[1] = p->next[b]->next[1];
      p->next[b] = new;
   }
   p->count[b]++;
   p = p->next[b];
   if (nodesptr > nodemaxp){
      //fprintf(stderr,"flushing ...\n");
      pflush();
   }
}

#define xgetc(X)  if(in >= inl) return(-1); \
                    *in++;
#define xputc(X)  if(o >= outl) return(-1); \
                    *o++ = X;

int undmc(unsigned char *in, int insz, unsigned char *out, int outsz) {
   int max = 0x1000000,
       min = 0,
       mid,
       val,
       i,
       inbytes=3,
       pin=3,
       outbytes=0,
       bit,
       c;

    unsigned char   *inl,
                    *o,
                    *outl;

    inl = in + insz;
    o = out;
    outl = out + outsz;

   if(pinit(memsize_g) < 0) return(-1);

   val = (in[0] << 16) | (in[1] << 8) | in[2];
   in += 3;
   //val = xgetc()<<16;
   //val += xgetc()<<8;
   //val += xgetc();
   while(1) {
      c = 0;
      if (val == (max-1)) {
         //fprintf(stderr,"expand: input %d output %d\n",inbytes,outbytes);
         break;
      }
      for (i=0;i<8;i++){
         mid = min + (max-min-1)*predict();
         if (mid == min) mid++;
         if (mid == (max-1)) mid--;
         if (val >= mid) {
            bit = 1;
            min = mid;
         } else {
            bit = 0;
            max = mid;
         }
         pupdate(bit != 0);
         c = c + c + bit;
         while ((max-min) < 256) {
            if(bit)max--;
            inbytes++;
            if((in + 1) > inl) return(-1);
            val = ((val << 8) & 0xffff00) | (*in & 0xff); in++;
            min = (min << 8) & 0xffff00;
            max = ((max << 8) & 0xffff00 ) ;
            if (min >= max) max = 0x1000000;
         }
      }
      xputc(c);
      if(!(++outbytes & 0xff)){
         if (inbytes - pin > 256) { /* compression was failing */
            pflush();
         }
         pin = inbytes;
      }
   }
   return(o - out);
}
