#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#include "../libs/ctw/ctw-settings.h"
#include "../libs/ctw/ctwmath.h"
#include "../libs/ctw/ctwlarc.h"
#include "../libs/ctw/ctwstep.h"
#include "../libs/ctw/ctwtree.h"
#include "../libs/ctw/ctwencdec.h"
#include "../libs/ctw/ctw-header.h"

// it's out_size based so insz is useless
int unctw(unsigned char *in, int insz, unsigned char *out, int outsz)
{
  unsigned char let=0;
  unsigned int  phase, i, o = 0, codebits;

  init_CTWsettings();

  ardecinit(in, insz);
  if (!init_filebuffer()) return(-1);

  for(nrsymbols = 0; nrsymbols <= settings.treedepth && nrsymbols < outsz; nrsymbols++)
  {
    let = 0;
    for(phase = 0; phase < 8; phase++)
      let = (let << 1) | ardec(STEPHALF);
    filebuffer[nrsymbols] = let;
    if(o >= outsz) return(o);
    out[o++] = let;
  }

    if (!init_tree(let)) return(-1);
    init_ctxstring();

    /* start decoding */
    for(i = settings.treedepth + 1; i < outsz; i++)
    {
      let = Decode();
      if(o >= outsz) return(o);
      out[o++] = let;
    }

  ardecexit(&codebits);
  free_memory();
  return(o);
}
