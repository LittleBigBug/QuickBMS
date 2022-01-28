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

#ifndef ____MMINI_H
#define ____MMINI_H

/* Current version of this software */
#define MMINI_VERSION 0.5

/**
 * Required size of huffheap parameter to compress and decompress
 *
 * Note: if you change any of the data types in the _mmini_huffman_node
 * or _mmini_huffman_encode_table structs in mmini_huffman.c, this also must be
 * changed.
 */
#define MMINI_HUFFHEAP_SIZE ((sizeof(double) * 257) + (((sizeof(void *) * 4) + sizeof(double) + sizeof(int)) * (257 * 3)) + ((sizeof(long) + sizeof(int)) * 257))

/**
 * Huffman encode a block of data
 *
 * @param in Input data
 * @param inlen Input data length
 * @param out Output buffer
 * @param outlen Output buffer length
 * @param huffheap Heap memory to use for compression (must be MMINI_HUFFHEAP_SIZE in size)
 * @return Size of encoded result or 0 on out buffer overrun
 */
extern unsigned long mmini_huffman_compress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen,void *huffheap);

/**
 * Huffman decode a block of data
 *
 * @param in Input data
 * @param inlen Length of input data
 * @param out Output buffer
 * @param outlen Length of output buffer
 * @param huffheap Heap memory to use for decompression (must be MMINI_HUFFHEAP_SIZE in size)
 * @return Size of decoded result or 0 on out buffer overrun or corrupt input data
 */
extern unsigned long mmini_huffman_decompress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen,void *huffheap);

/**
 * Compress a block of data using LZL
 *
 * @param in Input data
 * @param inlen Length of input data
 * @param out Output buffer
 * @param outlen Length of output buffer
 * @return Compressed data length or 0 on output overrun
 */
extern unsigned long mmini_lzl_compress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen);

/**
 * Decompress a block of LZL compressed data
 *
 * @param in Input data
 * @param inlen Length of input data
 * @param out Output buffer
 * @param outlen Length of output buffer
 * @return Decompressed data length or 0 on output overrun
 */
extern unsigned long mmini_lzl_decompress(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long outlen);

/**
 * Maximum size allowed for mmini_compress input block (0xffffff, 24 bits)
 */
#define MMINI_COMPRESS_MAX_BLOCK 16777215

static inline unsigned long mmini_compress(unsigned char *in,unsigned long inlen,unsigned char *tmp,unsigned long tmplen,unsigned char *out,unsigned long outlen,void *huffheap)
{
  unsigned long l;

  if (outlen < (inlen + 3)) return 0;
  if (tmplen < inlen) return 0;

  l = mmini_lzl_compress(in,inlen,tmp,tmplen);
  if ((l)&&(l < inlen)) {
    l = mmini_huffman_compress(tmp,l,out + 3,inlen + 3,huffheap);
    if (l) {
      out[0] = (unsigned char)((inlen >> 16) & 0xff);
      out[1] = (unsigned char)((inlen >> 8) & 0xff);
      out[2] = (unsigned char)(inlen & 0xff);
      return (l + 3);
    }
  }

  out[0] = (unsigned char)0; /* zero size header means stored */
  out[1] = (unsigned char)0;
  out[2] = (unsigned char)0;
  for(l=0;l<inlen;++l)
    out[l+3] = in[l];
  return (inlen + 3);
}

static inline unsigned long mmini_decompress(unsigned char *in,unsigned long inlen,unsigned char *tmp,unsigned long tmplen,unsigned char *out,unsigned long outlen,void *huffheap)
{
  unsigned long dl,l;

  if (inlen < 3) return 0;
  dl = ((unsigned long)((in[0] << 16) & 0xff0000)) | ((unsigned long)((in[1] << 8) & 0xff00)) | ((unsigned long)(in[2] & 0xff));

  if (dl) {
    l = mmini_huffman_decompress(in + 3,inlen - 3,tmp,tmplen,huffheap);
    if (l) {
      l = mmini_lzl_decompress(tmp,l,out,outlen);
      if (l == dl)
        return l;
      else return 0;
    } else return 0;
  } else {
    if (outlen < (inlen - 3)) return 0;
    for(l=3;l<inlen;++l)
      out[l-3] = in[l];
    return (inlen - 3);
  }
}

#endif
