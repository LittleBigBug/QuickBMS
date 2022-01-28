// modified by Luigi auriemma
/*  flzp file compressor (C) 2008, Matt Mahoney.

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create an array p of n elements of type T
template <class T> void alloc(T*&p, int n) {
  p=(T*)calloc(n, sizeof(T));
  if (!p) printf("Out of memory\n"), exit(1);
}

// Input buffer for finding matches
struct Buf {
  enum {BN=(1<<22),    // buffer size
        HN=BN/4};      // hash table size
  unsigned char *buf;  // input buffer
  unsigned int *ht;    // hash table: h -> matched context
  int *enc;            // encoding table: -1=literal, 0=EOB, 1..maxlen=match
  unsigned int h;      // context hash
  int p;               // number of bytes in buf
  int match;           // position of context match
  int len;             // length of match
  int maxlen;          // maximum length
  Buf();                           // initialize
  void update_hash(int c);         // update hash h with c
  void update(int c);              // append c, update ht, h
  void update(int c, unsigned char **out);   // update and write buf to out if full
  void flush(unsigned char **out);           // write buf to out
  void output_match(unsigned char **out);    // compress pending match
  void compress(unsigned char **out, int c); // compress byte c
};

// Initialize everything to 0
Buf::Buf() {
  alloc(buf, BN);
  alloc(ht, HN);
  alloc(enc, 256);
  h=p=match=len=maxlen=0;
}

// Update hash of last L bytes through c
inline void Buf::update_hash(int c) {
  h=h*(3<<5)+c&HN-1;
}

// Update hash table, context hash, buf, p
inline void Buf::update(int c) {
  ht[h]=p;
  update_hash(c);
  buf[p++&BN-1]=c;
}

// Update and also write buf to out if full
inline void Buf::update(int c, unsigned char **out) {
  update(c);
  if (!(p&BN-1)) {
    memcpy(*out, buf, BN);
    *out += BN;
  }
}

// Write buf to out even if not full
inline void Buf::flush(unsigned char **out) {
  if (p&BN-1) {
    memcpy(*out, buf, p&BN-1);
    *out += p&BN-1;
  }
}

// Decompress open file in to out
extern "C" int flzp_decompress(unsigned char *in, int insz, unsigned char *out) {
  unsigned char *out_bck = out;
  unsigned char *inl = in + insz;
  Buf b;
  enum {HEADER, DATA} state=HEADER;
  int dec[256]={0};

  // while there is more input
  while (1) {
    if (state==HEADER) {  // read 32 bytes into decoder table
      int maxlen=-1;
      for (int i=0; i<32; ++i) {
        if(in >= inl) break;
        int c=*in++;
        for (int j=0; j<8; ++j)
          dec[i*8+j]=c&1<<j?-1:++maxlen;
      }
      state=DATA;
    }
    else {  // decode a byte
      if(in >= inl) break;
      int c=*in++;

        int d=dec[c];
        if (d==0)  // EOB?
          state=HEADER;
        else if (d<0)  // literal?
          b.update(c, &out);
        else {  // match length?
          int match=b.ht[b.h];
          for (int i=0; i<d; ++i) {
            c=b.buf[match+i&b.BN-1];
            b.update(c, &out);
          }
        }
    }
  }
  b.flush(&out);
  return out - out_bck;
}


