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

/* These can't be changed without changing the encoding and the code below */
#define MMINI_LZL_WINDOW_SIZE 0x7ff /* 11 bits of offset */
#define MMINI_LZL_LENGTH_MAX 0x7f /* 7 bits of length */
#define MMINI_LZL_ESCAPE_THRESHOLD 0x03 /* values less than this are escaped in-stream */

unsigned long mmini_lzl_compress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen)
{
  unsigned long best_match_l = 3,match_l,match_max,offset,tmp;
  const unsigned char *scan_i,*best_scan_match_i = (const unsigned char *)0,*i = in,*ieof = (in + inlen);
  unsigned char *o = out,*oeof = (out + outlen);
  unsigned int slack = 0; /* start optimistic */

  while (i != ieof) {
    tmp = MMINI_LZL_WINDOW_SIZE >> (slack >> 4); /* apply laziness scaling to window size */
    scan_i = (((unsigned long)(i - in)) > tmp) ? (i - tmp) : in;
    tmp = (unsigned long)(ieof - i); /* how far are we from end? */
    match_max = (tmp > MMINI_LZL_LENGTH_MAX) ? MMINI_LZL_LENGTH_MAX : tmp;
    while (scan_i != i) {
      match_l = 0;
      while ((scan_i[match_l] == i[match_l])&&(match_l != match_max))
        ++match_l;
      if (match_l > best_match_l) {
        best_scan_match_i = scan_i;
        best_match_l = match_l;
        if (match_l == match_max)
          break;
      }
      while (*(++scan_i) != *i) {} /* *scan_i == *i when scan_i == i */
    }
    if (best_match_l >= 4) { /* most significant 5 bits of length cannot be zero, otherwise escaping fails (and <= 3 would be zero-gain anyway) */
      offset = (unsigned long)(i - best_scan_match_i);
      if ((o + 3) > oeof) return 0;
      *(o++) = (unsigned char)(best_match_l & 0x03); /* least significant 2 bits of length */
      *(o++) = (unsigned char)(((best_match_l & 0x7c) << 1) | (offset >> 8)); /* most significant 5 bits of length, most significant 3 bits of offset */
      *(o++) = (unsigned char)(offset & 0xff); /* least significant 8 bits of offset */
      i += best_match_l;
      best_match_l = 3;
      slack = 0; /* become optimistic again */
    } else {
      if (*i <= MMINI_LZL_ESCAPE_THRESHOLD) {
        if (o == oeof) return 0;
        *(o++) = *i;
      }
      if (o == oeof) return 0;
      *(o++) = *(i++);
      if (slack < 0x7f) ++slack; /* become increasingly pessimistic */
    }
  }

  return (unsigned long)(o - out);
}

unsigned long mmini_lzl_decompress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen)
{
  unsigned long outptr,i,start,len,end;
  unsigned char tmp;

  outptr = 0;
  i = 0;
  while (i < inlen) {
    if (in[i] <= MMINI_LZL_ESCAPE_THRESHOLD) {
      tmp = in[i]; /* least significant 2 bits of length */
      if (++i >= inlen) return 0;
      if (in[i] <= MMINI_LZL_ESCAPE_THRESHOLD) {
        if (outptr >= outlen) return 0;
        out[outptr++] = in[i];
      } else {
        len = (unsigned long)in[i]; /* most significant 5 bits of length, most significant 3 bits of offset */
        if (++i >= inlen) return 0;
        start = outptr - (((len & 0x07) << 8) | (unsigned long)in[i]); /* least significant 8 bits of offset */
        len = ((len >> 1) & 0x7c) | (unsigned long)tmp; /* mask length with least significant 2 bits from above */
        if ((outptr + len) > outlen) return 0;
        for(end=start+len;start<end;++start)
          out[outptr++] = out[start];
      }
    } else {
      if (outptr >= outlen) return 0;
      out[outptr++] = in[i];
    }
    ++i;
  }

  return outptr;
}
