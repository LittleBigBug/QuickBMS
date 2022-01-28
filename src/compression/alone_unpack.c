/*
   UnPAK, PAK file unpacker
   By Cyril VOILA (cvoila@free.fr)

   Part of "explode" code from Mark Adler implementation (30 Mars 1992)

   vers    date          who           what
   ----  ---------  --------------  ------------------------------------
    1.0  05 Oct 04  C. Voila        First release
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "common.h"

#define _WINDOWS
//#define ZLIB_DLL
//#include "zlib.h"

// --------------------------------------------------------------
// Explode unpacking functions & types
// --------------------------------------------------------------
#define PAK_BMAX 16
#define PAK_N_MAX 288
#define PAK_WSIZE 0x8000

typedef struct {
  unsigned long csize;
  unsigned long ucsize;
  unsigned char * buf_src;
  unsigned char * buf_dst;
  unsigned long off_src;
  unsigned long off_dst;
  unsigned short flags;
} PAK_stream;

typedef struct PAK_huft {
  unsigned short e;     // number of PAK_extra bits or operation
  unsigned short b;     // number of bits in this code or subcode
  union {
    unsigned short n;   // literal, length base, or distance base
    struct PAK_huft *t;     // pointer to next level of table
  } v;
} PAK_huft;


static unsigned char PAK_slide[PAK_WSIZE];
static unsigned PAK_mask_bits[17] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff };

/* Tables for length and distance */
static unsigned short cplen2[] = {
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65
};

static unsigned short cplen3[] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
        53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66
};

static unsigned char extra[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8
};

static unsigned short cpdist4[] = {
        1, 65, 129, 193, 257, 321, 385, 449, 513, 577, 641, 705,
        769, 833, 897, 961, 1025, 1089, 1153, 1217, 1281, 1345, 1409, 1473,
        1537, 1601, 1665, 1729, 1793, 1857, 1921, 1985, 2049, 2113, 2177,
        2241, 2305, 2369, 2433, 2497, 2561, 2625, 2689, 2753, 2817, 2881,
        2945, 3009, 3073, 3137, 3201, 3265, 3329, 3393, 3457, 3521, 3585,
        3649, 3713, 3777, 3841, 3905, 3969, 4033
};

static unsigned short cpdist8[] = {
        1, 129, 257, 385, 513, 641, 769, 897, 1025, 1153, 1281,
        1409, 1537, 1665, 1793, 1921, 2049, 2177, 2305, 2433, 2561, 2689,
        2817, 2945, 3073, 3201, 3329, 3457, 3585, 3713, 3841, 3969, 4097,
        4225, 4353, 4481, 4609, 4737, 4865, 4993, 5121, 5249, 5377, 5505,
        5633, 5761, 5889, 6017, 6145, 6273, 6401, 6529, 6657, 6785, 6913,
        7041, 7169, 7297, 7425, 7553, 7681, 7809, 7937, 8065
};

#define PAK_NEXTBYTE ((pG->off_src<pG->csize)?(pG->buf_src[pG->off_src++]):0)
#define PAK_FLUSH(size) { memcpy(pG->buf_dst + pG->off_dst, PAK_slide, size); pG->off_dst += size; }

#define PAK_NEEDBITS(n) {while(k<(n)){b|=((unsigned long)PAK_NEXTBYTE)<<k;k+=8;}}
#define PAK_DUMPBITS(n) {b>>=(n);k-=(n);}
#define PAK_DECODEHUFT(htab, bits, mask) {\
  PAK_NEEDBITS((unsigned)(bits))\
  t = (htab) + ((~(unsigned)b)&(mask));\
  while(1) {\
    PAK_DUMPBITS(t->b)\
    if((e=t->e) <= 32) break;\
    if(e==99) return 1;\
    e &= 31;\
    PAK_NEEDBITS(e)\
    t = t->v.t + ((~(unsigned)b)&PAK_mask_bits[e]);\
  }\
}

void PAK_huft_free(PAK_stream * pG, PAK_huft * t) {
  register PAK_huft *p, *q;
  p = t;
  while(p != (PAK_huft *)NULL) {
    q = (--p)->v.t;
    free((void *)p);
    p = q;
  }
}

int PAK_huft_build(PAK_stream * pG, unsigned * b, unsigned n, unsigned s, unsigned short * d, unsigned char * e, PAK_huft * t[], unsigned * m) {
  unsigned a;                   /* counter for codes of length k */
  unsigned c[PAK_BMAX+1];       /* bit length count table */
  unsigned el;                  /* length of EOB code (value 256) */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  register unsigned i;          /* counter, current code */
  register unsigned j;          /* counter */
  register int k;               /* number of bits in current code */
  int lx[PAK_BMAX+1];           /* memory for l[-1..PAK_BMAX-1] */
  int *l = lx+1;                /* stack of bits per table */
  register unsigned *p;         /* pointer into c[], b[], or v[] */
  register PAK_huft *q;         /* points to current table */
  PAK_huft r;                   /* table entry for structure assignment */
  PAK_huft *u[PAK_BMAX];        /* table stack */
  unsigned v[PAK_N_MAX];        /* values in order of bit length */
  register int w;               /* bits before this table == (l * h) */
  unsigned x[PAK_BMAX+1];       /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */

  /* Generate counts for each bit length */
  el = n > 256 ? b[256] : PAK_BMAX; /* set length of EOB code, if any */
  memset((char *)c, 0, sizeof(c));
  p = (unsigned *)b;  i = n;
  do {
    c[*p]++; p++;               /* assume all entries <= PAK_BMAX */
  } while(--i);
  if(c[0] == n)                /* null input--all zero length codes */
  {
    *t = (PAK_huft *)NULL;
    *m = 0;
    return(0);
  }

  /* Find minimum and maximum length, bound *m by those */
  for(j = 1; j <= PAK_BMAX; j++)
    if(c[j])
      break;
  k = j;                        /* minimum code length */
  if(*m < j) *m = j;
  for(i = PAK_BMAX; i; i--) {
    if(c[i]) break;
  }
  g = i;                        /* maximum code length */
  if(*m > i) *m = i;

  /* Adjust last length count to fill out codes, if needed */
  for(y = 1 << j; j < i; j++, y <<= 1) {
    if((y -= c[j]) < 0) return(2);  /* bad input: more codes than bits */
  }
  if((y -= c[i]) < 0) return(2);
  c[i] += y;

  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while(--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }

  /* Make a table of values in order of bit lengths */
  memset((char *)v, 0, sizeof(v));
  p = (unsigned *)b;  i = 0;
  do {
    if((j = *p++) != 0) v[x[j]++] = i;
  } while(++i < n);
  n = x[g];                     /* set n to length of v */

  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = l[-1] = 0;                /* no bits decoded yet */
  u[0] = (PAK_huft *)NULL;      /* just to keep compilers happy */
  q = (PAK_huft *)NULL;         /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for(; k <= g; k++) {
    a = c[k];
    while(a--) {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while(k > w + l[h]) {
        w += l[h++];            /* add bits already decoded */

        /* compute minimum size table less than or equal to *m bits */
        z = (z = g - w) > *m ? *m : z;                  /* upper limit */
        if((f = 1 << (j = k - w)) > a + 1) {    /* try a k-w bit table */
                                /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          while(++j < z) {      /* try smaller tables up to z bits */
            if((f <<= 1) <= *++xp) break; /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        if((unsigned)w + j > el && (unsigned)w < el) j = el - w; /* make EOB code end at table */
        z = 1 << j;             /* table entries for j-bit table */
        l[h] = j;               /* set table size in stack */

        /* allocate and link in new table */
        if((q = (PAK_huft *)malloc((z + 1)*sizeof(PAK_huft))) == (PAK_huft *)NULL) {
          if(h) PAK_huft_free(pG, u[0]);
          return(3);            /* not enough memory */
        }

        *t = q + 1;             /* link to list for PAK_huft_free() */
        *(t = &(q->v.t)) = (PAK_huft *)NULL;
        u[h] = ++q;             /* table starts after link */

        /* connect to last table, if there is one */
        if(h) {
          x[h] = i;             /* save pattern for backing up */
          r.b = (unsigned char)l[h-1];    /* bits to dump before this table */
          r.e = (unsigned char)(32 + j);  /* bits in this table */
          r.v.t = q;            /* pointer to this table */
          j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
          u[h-1][j] = r;        /* connect to last table */
        }
      }

      /* set up table entry in r */
      r.b = (unsigned char)(k - w);
      if(p >= v + n) {
        r.e = 99;     /* out of values--invalid code */
      } else if(*p < s) {
        r.e = (unsigned char)(*p < 256 ? 32 : 31);  /* 256 is end-of-block code */
        r.v.n = (unsigned short)*p++;               /* simple code is just the value */
      } else {
        r.e = e[*p - s];        /* non-simple--look up in lists */
        r.v.n = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for(j = i >> w; j < z; j += f) {
        q[j] = r;
      }

      /* backwards increment the k-bit code i */
      for(j = 1 << (k - 1); i & j; j >>= 1) {
        i ^= j;
      }
      i ^= j;

      /* backup over finished tables */
      while((i & ((1 << w) - 1)) != x[h]) {
        w -= l[--h];            /* don't need to update q */
      }
    }
  }

  /* return actual size of base table */
  *m = l[0];

  /* Return true (1) if we were given an incomplete table */
  return((y!=0) && (g!=1));
}

/* Get the bit lengths for a code representation from the compressed stream. */
int PAK_get_tree(PAK_stream * pG, unsigned * l, unsigned n) {
  unsigned i;           /* unsigned chars remaining in list */
  unsigned k;           /* lengths entered */
  unsigned j;           /* number of codes */
  unsigned b;           /* bit length for those codes */

  /* get bit lengths */
  i = PAK_NEXTBYTE + 1;                 /* length/count pairs to read */
  k = 0;                                /* next code */
  do {
    b = ((j = PAK_NEXTBYTE) & 0xf) + 1; /* bits in code (1..16) */
    j = ((j & 0xf0) >> 4) + 1;          /* codes with those bits (1..16) */
    if(k + j > n) return(4);            /* don't overflow l[] */
    do {
      l[k++] = b;
    } while(--j);
  } while(--i);
  return((k!=n)?4:0);                   /* should have read n of them */
}

/* Decompress the imploded data using coded literals and a sliding window (of size 2^(6+bdl) bytes). */
int PAK_explode_lit(PAK_stream * pG, PAK_huft * tb, PAK_huft * tl, PAK_huft * td, unsigned bb, unsigned bl, unsigned bd, unsigned bdl) {
  unsigned long s;      /* bytes to decompress */
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  PAK_huft *t;          /* pointer to table entry */
  unsigned mb, ml, md;  /* masks for bb, bl, and bd bits */
  unsigned mdl;         /* mask for bdl (distance lower) bits */
  register unsigned long b; /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unPAK_FLUSHed */

  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unPAK_FLUSHed */
  mb = PAK_mask_bits[bb];       /* precompute masks for speed */
  ml = PAK_mask_bits[bl];
  md = PAK_mask_bits[bd];
  mdl = PAK_mask_bits[bdl];
  s = pG->ucsize;
  while(s>0) {                 /* do until ucsize bytes uncompressed */
    PAK_NEEDBITS(1)
    if(b & 1) {                /* then literal--decode it */
      PAK_DUMPBITS(1)
      s--;
      PAK_DECODEHUFT(tb, bb, mb)    /* get coded literal */
      PAK_slide[w++] = (unsigned char)t->v.n;
      if(w == PAK_WSIZE) {
        PAK_FLUSH(w)
        w = u = 0;
      }
    } else {                    /* else distance/length */
      PAK_DUMPBITS(1)
      PAK_NEEDBITS(bdl)             /* get distance low bits */
      d = (unsigned)b & mdl;
      PAK_DUMPBITS(bdl)
      PAK_DECODEHUFT(td, bd, md)    /* get coded distance high bits */
      d = w - d - t->v.n;       /* construct offset */
      PAK_DECODEHUFT(tl, bl, ml)    /* get coded length */
      n = t->v.n;
      if(e) {                    /* get length extra bits */
        PAK_NEEDBITS(8)
        n += (unsigned)b & 0xff;
        PAK_DUMPBITS(8)
      }
      s = (s > (unsigned long)n ? s - (unsigned long)n : 0);
      do {
        e = PAK_WSIZE - ((d &= PAK_WSIZE-1) > w ? d : w);
        if(e>n) e = n;
        n -= e;
        if(u && (w<=d)) {
          memset(PAK_slide + w, 0, e);
          w += e;
          d += e;
        } else {
          if(w-d >= e) {    // Fast memcopy for large block
            memcpy(PAK_slide + w, PAK_slide + d, e);
            w += e;
            d += e;
          } else {          // Slow memcopy for small block
            do {
              PAK_slide[w++] = PAK_slide[d++];
            } while(--e);
          }
        }
        if(w == PAK_WSIZE) {
          PAK_FLUSH(w)
          w = u = 0;
        }
      } while(n);
    }
  }
  PAK_FLUSH(w)
  return(0);
}

/* Decompress the imploded data using uncoded literals and a sliding window (of size 2^(6+bdl) bytes). */
int PAK_explode_nolit(PAK_stream * pG, PAK_huft * tl, PAK_huft * td, unsigned bl, unsigned bd, unsigned bdl) {
  unsigned long s;      /* unsigned chars to decompress */
  register unsigned e;  /* table entry flag/number of PAK_extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  PAK_huft *t;          /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  unsigned mdl;         /* mask for bdl (distance lower) bits */
  register unsigned long b; /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unPAK_FLUSHed */

  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unPAK_FLUSHed */
  ml = PAK_mask_bits[bl];           /* precompute masks for speed */
  md = PAK_mask_bits[bd];
  mdl = PAK_mask_bits[bdl];
  s = pG->ucsize;
  while(s>0) {
    PAK_NEEDBITS(1)
    if(b & 1) {                 /* then literal--get eight bits */
      PAK_DUMPBITS(1)
      s--;
      PAK_NEEDBITS(8)
      PAK_slide[w++] = (unsigned char)b;
      if(w==PAK_WSIZE) {
        PAK_FLUSH(w)
        w = u = 0;
      }
      PAK_DUMPBITS(8)
    } else {
      PAK_DUMPBITS(1)
      PAK_NEEDBITS(bdl)             /* get distance low bits */
      d = (unsigned)b & mdl;
      PAK_DUMPBITS(bdl)
      PAK_DECODEHUFT(td, bd, md)    /* get coded distance high bits */
      d = w - d - t->v.n;       /* conPAK_huftoffset */
      PAK_DECODEHUFT(tl, bl, ml)    /* get coded length */
      n = t->v.n;
      if(e) {                   /* get length PAK_extra bits */
        PAK_NEEDBITS(8)
        n += (unsigned)b & 0xff;
        PAK_DUMPBITS(8)
      }
      s = (s > (unsigned long)n ? s - (unsigned long)n : 0);
      do {
        e = PAK_WSIZE - ((d &= PAK_WSIZE-1) > w ? d : w);
        if(e > n) e = n;
        n -= e;
        if(u && w <= d) {
          memset(PAK_slide + w, 0, e);
          w += e;
          d += e;
        } else {
          if(w-d >= e) {    // Fast memcopy for large block
            memcpy(PAK_slide + w, PAK_slide + d, e);
            w += e;
            d += e;
          } else {          // Slow memcopy for small block
            do {
              PAK_slide[w++] = PAK_slide[d++];
            } while(--e);
          }
        }
        if(w==PAK_WSIZE) {
          PAK_FLUSH(w)
          w = u = 0;
        }
      } while(n);
    }
  }
  PAK_FLUSH(w)
  return(0);
}

// --------------------------------------------------------------
// Wrapper to explode
// --------------------------------------------------------------

int PAK_explode(unsigned char * srcBuffer, unsigned char * dstBuffer, unsigned int compressedSize, unsigned int uncompressedSize, unsigned short flags)
{
  PAK_huft * tb;        /* literal code table */
  PAK_huft * tl;        /* length code table */
  PAK_huft * td;        /* distance code table */
  unsigned bb;          /* bits for tb */
  unsigned bl;          /* bits for tl */
  unsigned bd;          /* bits for td */
  unsigned bdl;         /* number of uncoded lower distance bits */
  unsigned l[256];      /* bit lengths for codes */

  PAK_stream G;
  G.buf_src = srcBuffer;
  G.buf_dst = dstBuffer;
  G.off_src = 0;
  G.off_dst = 0;
  G.csize = compressedSize;
  G.ucsize = uncompressedSize;

  bl = 7;
  bd = (compressedSize > 200000L) ? 8 : 7; // TODO : Totalement FOIREUX, à vérifier

  if(flags & 4) {    // With literal tree--minimum match length is 3
    bb = 9;
    PAK_get_tree(&G, l, 256);
    PAK_huft_build(&G, l, 256, 256, NULL, NULL, &tb, &bb);
    PAK_get_tree(&G, l, 64);
    PAK_huft_build(&G, l, 64, 0, cplen3, extra, &tl, &bl);
  } else {                // No literal tree--minimum match length is 2
    tb = (PAK_huft *) NULL;
    PAK_get_tree(&G, l, 64);
    PAK_huft_build(&G, l, 64, 0, cplen2, extra, &tl, &bl);
  }

  PAK_get_tree(&G, l, 64);

  if(flags & 2) {     /* true if 8K */
    bdl = 7;
    PAK_huft_build(&G, l, 64, 0, cpdist8, extra, &td, &bd);
  } else {                 /* else 4K */
    bdl = 6;
    PAK_huft_build(&G, l, 64, 0, cpdist4, extra, &td, &bd);
  }

  if(tb!=NULL) {
    PAK_explode_lit(&G, tb, tl, td, bb, bl, bd, bdl);
    PAK_huft_free(&G, tb);
  } else {
    PAK_explode_nolit(&G, tl, td, bl, bd, bdl);
  }

  PAK_huft_free(&G, td);
  PAK_huft_free(&G, tl);

  return(0);
}

