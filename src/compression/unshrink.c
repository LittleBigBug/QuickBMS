// modified by Luigi Auriemma
/*
  Copyright (c) 1990-2008 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unshrink.c                     version 1.22                     19 Mar 2008


       NOTE:  This code may or may not infringe on the so-called "Welch
       patent" owned by Unisys.  (From reading the patent, it appears
       that a pure LZW decompressor is *not* covered, but this claim has
       not been tested in court, and Unisys is reported to believe other-
       wise.)  It is therefore the responsibility of the user to acquire
       whatever license(s) may be required for legal use of this code.

       THE INFO-ZIP GROUP DISCLAIMS ALL LIABILITY FOR USE OF THIS CODE
       IN VIOLATION OF APPLICABLE PATENT LAW.


  Shrinking is basically a dynamic LZW algorithm with allowed code sizes of
  up to 13 bits; in addition, there is provision for partial clearing of
  leaf nodes.  PKWARE uses the special code 256 (decimal) to indicate a
  change in code size or a partial clear of the code tree:  256,1 for the
  former and 256,2 for the latter.  [Note that partial clearing can "orphan"
  nodes:  the parent-to-be can be cleared before its new child is added,
  but the child is added anyway (as an orphan, as though the parent still
  existed).  When the tree fills up to the point where the parent node is
  reused, the orphan is effectively "adopted."  Versions prior to 1.05 were
  affected more due to greater use of pointers (to children and siblings
  as well as parents).]

  This replacement version of unshrink.c was written from scratch.  It is
  based only on the algorithms described in Mark Nelson's _The Data Compres-
  sion Book_ and in Terry Welch's original paper in the June 1984 issue of
  IEEE _Computer_; no existing source code, including any in Nelson's book,
  was used.

  Memory requirements have been reduced in this version and are now no more
  than the original Sam Smith code.  This is still larger than any of the
  other algorithms:  at a minimum, 8K+8K+16K (stack+values+parents) assuming
  16-bit short ints, and this does not even include the output buffer (the
  other algorithms leave the uncompressed data in the work area, typically
  called slide[]).  For machines with a 64KB data space this is a problem,
  particularly when text conversion is required and line endings have more
  than one character.  UnZip's solution is to use two roughly equal halves
  of outbuf for the ASCII conversion in such a case; the "unshrink" argument
  to flush() signals that this is the case.

  For large-memory machines, a second outbuf is allocated for translations,
  but only if unshrinking and only if translations are required.

              | binary mode  |        text mode
    ---------------------------------------------------
    big mem   |  big outbuf  | big outbuf + big outbuf2  <- malloc'd here
    small mem | small outbuf | half + half small outbuf

  Copyright 1994, 1995 Greg Roelofs.  See the accompanying file "COPYING"
  in UnZip 5.20 (or later) source or binary distributions.

  ---------------------------------------------------------------------------*/


#define __UNSHRINK_C    /* identifies this source module */
#define UNZIP_INTERNAL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#  define __G
#  define __G__
#  define __GDEF
#  define __GPRO    void
#  define __GPRO__
typedef unsigned char   uch;    /* code assumes unsigned bytes; these type-  */
typedef unsigned short  ush;    /*  defs replace byte/UWORD/ULONG (which are */
typedef unsigned long   ulg;    /*  predefined on some systems) & match zip  */
#ifdef PROTO
#  define OF(a) a
#else
#  define OF(a) ()
#endif
#define MAX_BITS    13                 /* used in unshrink() */
#define HSIZE       (1 << MAX_BITS)    /* size of global work area */
#    define WSIZE   0x8000  /* window size--must be a power of two, and */
#ifndef    TRUE
#define    TRUE 1
#endif  /* TRUE */
#ifndef    FALSE
#define    FALSE 0
#endif  /* FALSE */
#define Trace(X)
   int   flush                OF((__GPRO__ uch *buf, ulg size, int unshrink));
       typedef int  shrint;          /* for efficiency/speed, we hope... */
unsigned mask_bits[17] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};
#define READBITS(nbits,zdest) {if(nbits>bits_left) {int temp; zipeof=1;\
  while (bits_left<=8*(int)(sizeof(bitbuf)-1) && (temp=NEXTBYTE)!=-1) {\
  bitbuf|=(ulg)temp<<bits_left; bits_left+=8; zipeof=0;}}\
  zdest=(shrint)((unsigned)bitbuf&mask_bits[nbits]);bitbuf>>=nbits;\
  bits_left-=nbits;}
#  define NEXTBYTE  (incnt-- > 0 ? (int)(*inptr++) : -1)
/**/


#ifndef LZW_CLEAN

static void  partial_clear  OF((__GPRO__ int lastcodeused));

/* HSIZE is defined as 2^13 (8192) in unzip.h (resp. unzpriv.h */
#define BOGUSCODE  256
#define FLAG_BITS  parent        /* upper bits of parent[] used as flag bits */
#define CODE_MASK  (HSIZE - 1)   /* 0x1fff (lower bits are parent's index) */
#define FREE_CODE  HSIZE         /* 0x2000 (code is unused or was cleared) */
#define HAS_CHILD  (HSIZE << 1)  /* 0x4000 (code has a child--do not clear) */
#define PK_ERR  -1

static  shrint parent[HSIZE];    /* (8192 * sizeof(shrint)) == 16KB minimum */
static  uch Value[HSIZE];        /* 8KB */
static  uch stack[HSIZE];        /* 8KB */
//static  uch slide[WSIZE];        /* explode(), inflate(), unreduce() */


/***********************/
/* Function unshrink() */
/***********************/

int unshrink(unsigned char *in, int insz, unsigned char *out, int outsz) {
    uch *stacktop = stack + (HSIZE - 1);
    register uch *newstr;
    uch finalval;
    int codesize=9, len;
    shrint code, oldcode, curcode;
    shrint lastfreecode;
    unsigned int outbufsiz;

    ulg bitbuf      = 0;
    int bits_left   = 0;
    int zipeof      = 0;
    ulg outcnt      = 0,
        incnt       = insz;
    uch *o          = out,
        *inptr      = in;

/*---------------------------------------------------------------------------
    Initialize various variables.
  ---------------------------------------------------------------------------*/

    lastfreecode = BOGUSCODE;

    for (code = 0;  code < BOGUSCODE;  ++code) {
        Value[code] = (uch)code;
        parent[code] = BOGUSCODE;
    }
    for (code = BOGUSCODE+1;  code < HSIZE;  ++code)
        parent[code] = FREE_CODE;

    outbufsiz = outsz;

/*---------------------------------------------------------------------------
    Get and output first code, then loop over remaining ones.
  ---------------------------------------------------------------------------*/

    READBITS(codesize, oldcode)
    if (zipeof)
        goto PK_OK;

    finalval = (uch)oldcode;
    *o++ = finalval;
    ++outcnt;

    while (TRUE) {
        READBITS(codesize, code)
        if (zipeof)
            goto PK_OK;
        if (code == BOGUSCODE) {   /* possible to have consecutive escapes? */
            READBITS(codesize, code)
            if (zipeof)
                goto PK_OK;
            if (code == 1) {
                ++codesize;
                Trace((stderr, " (codesize now %d bits)\n", codesize));
                if (codesize > MAX_BITS) return PK_ERR;
            } else if (code == 2) {
                Trace((stderr, " (partial clear code)\n"));
                /* clear leafs (nodes with no children) */
                partial_clear(__G__ lastfreecode);
                Trace((stderr, " (done with partial clear)\n"));
                lastfreecode = BOGUSCODE; /* reset start of free-node search */
            }
            continue;
        }

    /*-----------------------------------------------------------------------
        Translate code:  traverse tree from leaf back to root.
      -----------------------------------------------------------------------*/

        newstr = stacktop;
        curcode = code;

        if (parent[code] == FREE_CODE) {
            /* or (FLAG_BITS[code] & FREE_CODE)? */
            Trace((stderr, " (found a KwKwK code %d; oldcode = %d)\n", code,
              oldcode));
            *newstr-- = finalval;
            code = oldcode;
        }

        while (code != BOGUSCODE) {
            if (newstr < stack) {
                /* Bogus compression stream caused buffer underflow! */
                Trace((stderr, "unshrink stack overflow!\n"));
                return PK_ERR;
            }
            if (parent[code] == FREE_CODE) {
                /* or (FLAG_BITS[code] & FREE_CODE)? */
                Trace((stderr, " (found a KwKwK code %d; oldcode = %d)\n",
                  code, oldcode));
                *newstr-- = finalval;
                code = oldcode;
            } else {
                *newstr-- = Value[code];
                code = (shrint)(parent[code] & CODE_MASK);
            }
        }

        len = (int)(stacktop - newstr++);
        finalval = *newstr;

    /*-----------------------------------------------------------------------
        Write expanded string in reverse order to output buffer.
      -----------------------------------------------------------------------*/

        Trace((stderr,
          "code %4d; oldcode %4d; char %3d (%c); len %d; string [", curcode,
          oldcode, (int)(*newstr), (*newstr<32 || *newstr>=127)? ' ':*newstr,
          len));

        {
            register uch *p;

            for (p = newstr;  p < newstr+len;  ++p) {
                *o++ = *p;
                if (++outcnt == outbufsiz) {
                    goto PK_OK;
                    //o = realbuf;
                    //outcnt = 0L;
                    //Trace((stderr, "done with flush()\n"));
                }
            }
        }

    /*-----------------------------------------------------------------------
        Add new leaf (first character of newstr) to tree as child of oldcode.
      -----------------------------------------------------------------------*/

        /* search for freecode */
        code = (shrint)(lastfreecode + 1);
        /* add if-test before loop for speed? */
        while ((code < HSIZE) && (parent[code] != FREE_CODE))
            ++code;
        lastfreecode = code;
        Trace((stderr, "]; newcode %d\n", code));
        if (code >= HSIZE)
            /* invalid compressed data caused max-code overflow! */
            return PK_ERR;

        Value[code] = finalval;
        parent[code] = oldcode;
        oldcode = curcode;

    }

/*---------------------------------------------------------------------------
    Flush any remaining data and return to sender...
  ---------------------------------------------------------------------------*/

PK_OK:
    return(outcnt);

} /* end function unshrink() */





/****************************/
/* Function partial_clear() */      /* no longer recursive... */
/****************************/

static void partial_clear(__G__ lastcodeused)
    __GDEF
    int lastcodeused;
{
    register shrint code;

    /* clear all nodes which have no children (i.e., leaf nodes only) */

    /* first loop:  mark each parent as such */
    for (code = BOGUSCODE+1;  code <= lastcodeused;  ++code) {
        register shrint cparent = (shrint)(parent[code] & CODE_MASK);

        if (cparent > BOGUSCODE)
            FLAG_BITS[cparent] |= HAS_CHILD;   /* set parent's child-bit */
    }

    /* second loop:  clear all nodes *not* marked as parents; reset flag bits */
    for (code = BOGUSCODE+1;  code <= lastcodeused;  ++code) {
        if (FLAG_BITS[code] & HAS_CHILD)    /* just clear child-bit */
            FLAG_BITS[code] &= ~HAS_CHILD;
        else {                              /* leaf:  lose it */
            Trace((stderr, "%d\n", code));
            parent[code] = FREE_CODE;
        }
    }

    return;
}

#endif /* !LZW_CLEAN */
