/* ctwencdec - contains functions to Encode and Decode a byte. 
   Note: All the data structures have to be initialized first! */
   
#include <stdio.h>
#include "ctw-settings.h"
#include "ctwmath.h"
#include "ctwlarc.h"
#include "ctwstep.h"
#include "ctwtree.h"
#include "ctwencdec.h"


int               phase;                       /* holds current phase */
struct CTWRecord  ctwinfo[MAX_TREEDEPTH+1];    /* used to store CTW info returned by FindPath */
struct CTWRecord  dummy0info[MAX_TREEDEPTH+1]; /* used to store the dummy 0 update */
struct CTWRecord  dummy1info[MAX_TREEDEPTH+1]; /* used to store the dummy 1 update */
int               curdepth;                    /* real depth of current path (may differ from
                                                  settings.treedepth because of pruning) */
unsigned char     ctxstring[MAX_TREEDEPTH+1];  /* holds copy of the current context */


void init_ctxstring() {
  /* copy the first context (treedepth+1 characters) from the filebuffer to ctxstring */
  unsigned int depth;
  for (depth = 0; depth < settings.treedepth + 1; depth++)
    ctxstring[depth + 1] = filebuffer[settings.treedepth + 1 - depth];
}

void Encode(unsigned char u) { /* used only when everything is initialized */
  unsigned char   bit;
  struct CTWProb  ctwprob;
  int             instep, depth;
  boolean         symbsmall;

  for(phase=0; phase<8; phase++) { /* process all 8 bits */
    ctxstring[0] = BytePrefix(u, phase); /* extend the context with one bit */
    bit = ByteBit(u, phase);
    
    /* search the path using the current context and get CTW info of all nodes on the path:
       (function defined in ctwtree.h) */
    FindPath(phase, nrsymbols, ctxstring, &curdepth, ctwinfo);
    
    /* get the logarithm of the weighted probability on the most probable bit symbol
       in the root of the CTW tree in ctwprob; and get a dummy-0 and a dummy-1
       update on the CTW tree in dummy0info[] and dummy1info[]. (function defined in ctwmath.h)
       Note: doing a dummy0 AND dummy1 update is not really necessary in the encoding process
       because the actual value of the current bit is already known! */
    CTWProcess(curdepth, ctwinfo, &ctwprob, dummy0info, dummy1info);
    
    /* calculate instep from ctwprob, and calculate the symbol with the smallest probability.
       instep is a measure of the logarithm of the probability of the most probable symbol.
       (function defined in ctwstep.h) */
    CTWsteps(ctwprob, &instep, &symbsmall);
    
    /* encode bit using arithmetic encoder. Note: instead of using the actual bit value for
       the arithmetich encoding, a 1 is encoded if the current bit has the smallest probability
       and a 0 otherwise; this to limit the size of the accumulator of the arithmetic encoder
       and to minimize calculation time (encoding 1 takes more time than encoding 0).
       [EIDMA par 6.2.5] (function defined in ctwlarc.h) */
    arenc(instep, symbsmall==bit);
    
    /* update CTW tree with appropriate update */
    if(bit)
      UpdatePath(dummy1info);
    else
      UpdatePath(dummy0info);
  }
  /* shift the contents of ctxstring one place further */
  for(depth = settings.treedepth + 1; depth >= 1; depth--)
    ctxstring[depth + 1] = ctxstring[depth];
  ctxstring[1] = u;
  if(!tree_frozen)
    filebuffer[nrsymbols] = u;	/* update the filebuffer, if the tree is not frozen */
  nrsymbols++;
}


unsigned char Decode(void) { /* used only when everything is initialized */
  unsigned char   u, bit;
  struct CTWProb  ctwprob;
  int             instep, depth;
  boolean         symbsmall;

  /* in fact, decoding is exactly the same as encoding; the only difference is that ardec()
     is used to decode a bit instead of arenc() */
  u = 0;
  for(phase=0; phase<8; phase++) {
    ctxstring[0] = BytePrefix(u, phase); /* extend the context with one bit */
    FindPath(phase, nrsymbols, ctxstring, &curdepth, ctwinfo);
    CTWProcess(curdepth, ctwinfo, &ctwprob, dummy0info, dummy1info);
    CTWsteps(ctwprob, &instep, &symbsmall);
    bit = ardec(instep)==symbsmall;
    if(bit)
      UpdatePath(dummy1info);
    else
      UpdatePath(dummy0info);
    u |= bit << (7-phase);
  }
  /* shift the contents of ctxstring one place further */
  for(depth = settings.treedepth + 1; depth >= 1; depth--)
    ctxstring[depth + 1] = ctxstring[depth];
  ctxstring[1] = u;
  if(!tree_frozen)
    filebuffer[nrsymbols] = u;	/* update the filebuffer, if the tree is not frozen */
  nrsymbols++;
  return(u);
}
