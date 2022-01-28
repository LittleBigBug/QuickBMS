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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mmini.h"

static void print_help(int argc,char **argv)
{
  printf("Usage: %s <operation> <infile> <outfile>\n\n",argv[0]);
  printf("Operations:\n");
  printf("  c               - Compress with LZL and Huffman\n");
  printf("  d               - Decompress with LZL and Huffman\n");
  printf("  H               - Compress with Huffman only\n");
  printf("  h               - Decompress with Huffman only\n");
  printf("  L               - Compress with LZL only\n");
  printf("  l               - Decompress with LZL only\n");
}

int main(int argc,char **argv)
{
  unsigned long r;
  long n,inptr = 0,insize = 65536;
  unsigned char *in,*out,*tmp,*huffheap;
  FILE *inf,*outf;

  if (argc != 4) {
    print_help(argc,argv);
    return -1;
  }

  inf = fopen(argv[2],"r");
  if (!inf) {
    printf("Could not open %s for reading.\n",argv[2]);
    return -1;
  }
  outf = fopen(argv[3],"w");
  if (!outf) {
    fclose(inf);
    printf("Could not open %s for writing.\n",argv[3]);
    return -1;
  }

  in = malloc(insize);
  while ((n = fread(in + inptr,1,insize - inptr,inf)) > 0) {
    inptr += n;
    if ((insize - inptr) <= 16384)
      in = realloc(in,insize *= 2);
  }

  huffheap = malloc(MMINI_HUFFHEAP_SIZE);
  tmp = malloc(insize + 4096);
  out = malloc(insize + 4096);

  switch(*argv[1]) {
    case 'c':
      r = mmini_compress(in,(unsigned long)inptr,tmp,(unsigned long)insize + 4096,out,(unsigned long)insize + 4096,huffheap);
      if (!r) {
        printf("Compression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    case 'd':
      r = mmini_decompress(in,(unsigned long)inptr,tmp,(unsigned long)insize + 4096,out,(unsigned long)insize + 4096,huffheap);
      if (!r) {
        printf("Decompression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    case 'H':
      r = mmini_huffman_compress(in,(unsigned long)inptr,out,(unsigned long)insize + 4096,huffheap);
      if (!r) {
        printf("Compression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    case 'h':
      r = mmini_huffman_decompress(in,(unsigned long)inptr,out,(unsigned long)insize + 4096,huffheap);
      if (!r) {
        printf("Decompression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    case 'L':
      r = mmini_lzl_compress(in,(unsigned long)inptr,out,(unsigned long)insize + 4096);
      if (!r) {
        printf("Compression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    case 'l':
      r = mmini_lzl_decompress(in,(unsigned long)inptr,out,(unsigned long)insize + 4096);
      if (!r) {
        printf("Decompression failed!\n");
        return -1;
      } else fwrite(out,1,r,outf);
      break;
    default:
      print_help(argc,argv);
      return -1;
  }

  fclose(inf);
  fclose(outf);
  free(out);
  free(tmp);
  free(huffheap);
  free(in);

  return 0;
}
