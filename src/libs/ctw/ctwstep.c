#include <stdio.h>
#include "ctw-settings.h"
#include "ctwmath.h"
#include "ctwlarc.h"
#include "ctwstep.h"

/* calculates *instep and *symbsmall from ctwprob. *instep is the stepsize that corresponds to
   the probability of the most probable symbol. In *symbsmall, the symbol with the smallest
   probability is returned */
void CTWsteps(struct CTWProb ctwprob, int *instep, boolean *symbsmall) {
  
  /* calculate the stepsize from ctwprob.logpwroot (the logarithm of the weighted probability on
     the most probable symbol in the root of the tree) */
  *instep = -ctwprob.logpwroot * (ARENTRIES / ACCURACY);

  /* calculate the least probable symbol from ctwprob.predsymb (the most probable symbol) */
  *symbsmall = 1 - ctwprob.predsymb;


  /* The stepsize is not allowed to be smaller than 3, because the size of the interval (in the
     arithmetic en-/decoder) for one of the symbols can be zero if it is. See [EIDMA] par 6.4.3
     for more information on the minimum stepsize. */
  if(*instep < 3) *instep = 3;

  return;
}
