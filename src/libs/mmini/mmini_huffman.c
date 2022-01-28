/*
 * Copyright (c) 2011 Adam Ierymenko [adam.ierymenko@gmail.com]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "mmini.h"

struct _mmini_huffman_node
{
  struct _mmini_huffman_node *lr[2];
  struct _mmini_huffman_node *qprev,*qnext;
  double prob;
  unsigned int c;
};

struct _mmini_huffman_encode_table
{
  unsigned long code;
  unsigned int bits;
};

static void _mmini_huffman_write_tree_and_make_encode_table(unsigned char *out,unsigned long *outbitctr,unsigned long outlen,struct _mmini_huffman_encode_table *et,unsigned long code,unsigned int bits,struct _mmini_huffman_node *t)
{
  struct _mmini_huffman_encode_table *eti;
  unsigned int i;
  unsigned long byte_index;

  byte_index = (*outbitctr)++ >> 3;
  byte_index *= (byte_index < outlen);
  if (t->lr[0]) {
    out[byte_index] <<= 1;
    _mmini_huffman_write_tree_and_make_encode_table(out,outbitctr,outlen,et,code,bits + 1,t->lr[0]);
    _mmini_huffman_write_tree_and_make_encode_table(out,outbitctr,outlen,et,code | (1 << bits),bits + 1,t->lr[1]);
  } else {
    out[byte_index] = (out[byte_index] << 1) | 1;
    for(i=0;i<9;++i) {
      byte_index = (*outbitctr)++ >> 3;
      if (byte_index >= outlen) return;
      out[byte_index] = (out[byte_index] << 1) | ((unsigned char)((t->c >> i) & 1));
    }
    eti = &(et[t->c]);
    eti->code = code;
    eti->bits = bits;
  }
}

static struct _mmini_huffman_node *_mmini_huffman_read_tree(const unsigned char *in,unsigned long *inbitctr,unsigned long inlen,unsigned char **heapptr,unsigned char *heapend)
{
  struct _mmini_huffman_node *n;
  unsigned int i;
  unsigned long byte_index;

  n = (struct _mmini_huffman_node *)(*heapptr);
  *heapptr += sizeof(struct _mmini_huffman_node);
  if (*heapptr > heapend) return (struct _mmini_huffman_node *)0;

  byte_index = *inbitctr >> 3;
  byte_index *= (byte_index < inlen);
  if (((in[byte_index] >> (~((*inbitctr)++) & 7)) & 1)) {
    n->lr[0] = (struct _mmini_huffman_node *)0;
    n->lr[1] = (struct _mmini_huffman_node *)0;
    n->c = 0;
    for(i=0;i<9;++i) {
      byte_index = *inbitctr >> 3;
      if (byte_index >= inlen) return (struct _mmini_huffman_node *)0;
      n->c |= (((unsigned int)(in[byte_index] >> (~((*inbitctr)++) & 7))) & 1) << i;
    }
  } else {
    n->lr[0] = _mmini_huffman_read_tree(in,inbitctr,inlen,heapptr,heapend);
    n->lr[1] = _mmini_huffman_read_tree(in,inbitctr,inlen,heapptr,heapend);
    if (!((n->lr[0])&&(n->lr[1])))
      return (struct _mmini_huffman_node *)0;
  }

  return n;
}

unsigned long mmini_huffman_compress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen,void *huffheap)
{
  struct _mmini_huffman_encode_table *et,*eti;
  struct _mmini_huffman_node *t,*n;
  struct _mmini_huffman_node *pair[2];
  unsigned char *heapptr = (unsigned char *)huffheap;
  unsigned long i,code,byte_index,outbitctr;
  unsigned int bits,b;
  double *counts,lowest_prob,total_symbols;

  counts = (double *)heapptr;
  heapptr += (sizeof(double) * 257);
  for(i=0;i<256;++i)
    counts[i] = 0.0;
  counts[256] = 1.0; /* one stop code at end */
  for(i=0;i<inlen;++i)
    counts[(unsigned long)in[i]] += 1.0;

  t = (struct _mmini_huffman_node *)0;
  total_symbols = (double)(inlen + 1);
  for(i=0;i<=256;++i) {
    if (counts[i] > 0.0) {
      n = (struct _mmini_huffman_node *)heapptr;
      heapptr += sizeof(struct _mmini_huffman_node);
      if (t)
        t->qprev = n;
      n->qprev = (struct _mmini_huffman_node *)0;
      n->qnext = t;
      n->lr[0] = (struct _mmini_huffman_node *)0;
      n->lr[1] = (struct _mmini_huffman_node *)0;
      n->prob = counts[i] / total_symbols;
      n->c = (unsigned int)i;
      t = n;
    }
  }

  while (t->qnext) {
    for(i=0;i<2;++i) {
      lowest_prob = 1.0;
      pair[i] = (struct _mmini_huffman_node *)0;
      n = t;
      while (n) {
        if (n->prob <= lowest_prob) {
          lowest_prob = n->prob;
          pair[i] = n;
        }
        n = n->qnext;
      }
      if (pair[i]->qprev)
        pair[i]->qprev->qnext = pair[i]->qnext;
      else t = pair[i]->qnext;
      if (pair[i]->qnext)
        pair[i]->qnext->qprev = pair[i]->qprev;
    }
    n = (struct _mmini_huffman_node *)heapptr;
    heapptr += sizeof(struct _mmini_huffman_node);
    n->lr[0] = pair[0];
    n->lr[1] = pair[1];
    n->prob = pair[0]->prob + pair[1]->prob;
    if (t)
      t->qprev = n;
    n->qnext = t;
    t = n;
  }

  et = (struct _mmini_huffman_encode_table *)heapptr;
  heapptr += (sizeof(struct _mmini_huffman_encode_table) * 257);
  outbitctr = 0;
  _mmini_huffman_write_tree_and_make_encode_table(out,&outbitctr,outlen,et,0,0,t);

  for(i=0;i<inlen;++i) {
    eti = &(et[(unsigned long)in[i]]);
    code = eti->code;
    bits = eti->bits;
    for(b=0;b<bits;++b) {
      byte_index = outbitctr++ >> 3;
      if (byte_index >= outlen) return 0;
      out[byte_index] = (out[byte_index] << 1) | (unsigned char)(code & 1);
      code >>= 1;
    }
  }
  code = et[256].code;
  bits = et[256].bits;
  for(b=0;b<bits;++b) {
    byte_index = outbitctr++ >> 3;
    if (byte_index >= outlen) return 0;
    out[byte_index] = (out[byte_index] << 1) | (unsigned char)(code & 1);
    code >>= 1;
  }

  if (outbitctr > (outlen << 3))
    return 0;
  else if ((outbitctr & 7)) {
    out[i = (outbitctr >> 3)] <<= 8 - (outbitctr & 7);
    return (i + 1);
  } else return (outbitctr >> 3);
}

unsigned long mmini_huffman_decompress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen,void *huffheap)
{
  struct _mmini_huffman_node *t,*n;
  unsigned char *heapptr = (unsigned char *)huffheap;
  unsigned long inbitctr,outptr,byte_index = 0;

  inbitctr = 0;
  t = _mmini_huffman_read_tree(in,&inbitctr,inlen,&heapptr,heapptr + MMINI_HUFFHEAP_SIZE);
  if (!t) return 0;
  outptr = 0;
  for(;;) {
    n = t;
    while (n->lr[0]) {
      byte_index = inbitctr >> 3;
      if (byte_index >= inlen) return 0;
      n = n->lr[((unsigned long)(in[byte_index] >> (~(inbitctr++) & 7))) & 1];
    }
    if (n->c == 256) return outptr;
    if (outptr == outlen) return 0;
    out[outptr++] = (unsigned char)n->c;
  }
}
