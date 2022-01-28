// modified by Luigi Auriemma for memory2memory decompression
/* expand.c */
/* Copyright 1994 by Philip Gage */

#include <stdio.h>

static int xgetc(unsigned char **in, unsigned char *inl) {
    int     ret;
    if(*in >= inl) return(-1);
    ret = **in;
    (*in)++;
    return(ret);
}
/*
static int xputc(int chr, unsigned char **out, unsigned char *outl) {
    if(*out >= outl) return(-1);
    **out = chr;
    (*out)++;
    return(chr);
}
*/

/* decompress data from input to output */
int bpe_expand (unsigned char *in, int insz, unsigned char *out, int outsz) {
  unsigned char left[256], right[256], stack[256]; // the original stack of 30 is not enough!
  short int c, count, i, size, n;

    unsigned char   *inl,
                    *o,
                    *outl;

    inl  = in + insz;
    o    = out;
    outl = out + outsz;

  /* unpack each block until end of file */
  while (( count = xgetc(&in, inl)) >= 0 ) {
    /* set left to itself as literal flag */
    for ( i = 0 ; i < 256; i++ ) {
      left[i] = i;
    }

    /* read pair table */
    for ( c = 0 ; ; ) {
      /* skip range of literal bytes */
      if ( count > 127 ) {
        c += count -127;
        count = 0;
      }
      if ( c >= 256 ) break;
   
      /* read pairs, skip right if literal */
      for ( i = 0; i <= count; i++, c++ ) {
        if((n = xgetc(&in, inl)) < 0) break;
        if(c >= 256) return(-1);
        left[c] = n;
        if ( c != left[c] ) {
          if((n = xgetc(&in, inl)) < 0) break;
          if(c >= 256) return(-1);
          right[c] = n;
        }
      }
      if ( c >= 256 ) break;
      count = xgetc(&in, inl);
      if(count < 0) break;
    }
    
    /* calculate packed data block size */
    if((n = xgetc(&in, inl)) < 0) break;
    size = n << 8;
    if((n = xgetc(&in, inl)) < 0) break;
    size |= n;

    /* unpack data block */
    for ( i = 0 ; ; ) {
      /* pop byte from stack or read byte */
      if ( i ) { 
        c = stack[--i];
      } else {
        if ( !size--) break;
        c = xgetc(&in, inl);
        if(c < 0) break;
      }

      /* output byte or push pair on stack */
      if(c >= 256) return(-1);
      if ( c == left[c] ) {
        //if(xputc(c, &o, outl) < 0) return(-1);
        if(o >= outl) return(-1);
        *o++ = c;
      } else {
        if((i + 2) > sizeof(stack)) return(-1);
        if(c >= 256) return(-1); // valid for both l&r
        stack[i++] = right[c];
        stack[i++] = left[c];
      }
    }
  }
    return(o - out);
}

