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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "mmini.h"

#ifdef BENCH_AGAINST_ZLIB
#include <zlib.h>
#endif

#define TEST_BUF_SIZE ((1024 * 1024 * 4) + 131072)
#define TEST_DATA_MAX (1024 * 1024 * 4)

static inline double now()
{
  struct timeval t;
  gettimeofday(&t,(struct timezone *)0);
  return (((double)t.tv_sec) + (((double)t.tv_usec) / 1000000.0));
}

int main(int argc,char **argv)
{
  unsigned char *in = malloc(TEST_BUF_SIZE),*inbak = malloc(TEST_BUF_SIZE);
  unsigned char *out = malloc(TEST_BUF_SIZE);
  unsigned char *tmp = malloc(TEST_BUF_SIZE);
  unsigned long i,l;
  unsigned char *huffheap = malloc(MMINI_HUFFHEAP_SIZE);
  double start,end;

  srand(time(0));

  printf("LZL: Testing and Benchmarking\n");

  printf("  Highly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)(i & 0xf);

  start = now();
  l = mmini_lzl_compress(in,TEST_DATA_MAX,out,TEST_BUF_SIZE);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_lzl_decompress(out,l,in,TEST_BUF_SIZE);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != (unsigned char)(i & 0xf)) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)\n");
    return -1;
  }

  printf("  Poorly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)((rand() >> 3) & 0xff);
  memcpy(inbak,in,TEST_DATA_MAX);

  start = now();
  l = mmini_lzl_compress(in,TEST_DATA_MAX,out,TEST_BUF_SIZE);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_lzl_decompress(out,l,in,TEST_BUF_SIZE);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != inbak[i]) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)");
    return -1;
  }

  printf("Huffman Coding: Testing and Benchmarking\n");

  printf("  Highly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)(i & 0xf);

  start = now();
  l = mmini_huffman_compress(in,TEST_DATA_MAX,out,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_huffman_decompress(out,l,in,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != (unsigned char)(i & 0xf)) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)\n");
    return -1;
  }

  printf("  Poorly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)((rand() >> 3) & 0xff);
  memcpy(inbak,in,TEST_DATA_MAX);

  start = now();
  l = mmini_huffman_compress(in,TEST_DATA_MAX,out,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_huffman_decompress(out,l,in,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != inbak[i]) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)\n");
    return -1;
  }

  printf("Combined (LZL-Huffman) Coding: Testing and Benchmarking\n");

  printf("  Highly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)(i & 0xf);

  start = now();
  l = mmini_compress(in,TEST_DATA_MAX,tmp,TEST_BUF_SIZE,out,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_decompress(out,l,tmp,TEST_BUF_SIZE,in,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != (unsigned char)(i & 0xf)) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)\n");
    return -1;
  }

  printf("  Poorly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)((rand() >> 3) & 0xff);
  memcpy(inbak,in,TEST_DATA_MAX);

  start = now();
  l = mmini_compress(in,TEST_DATA_MAX,tmp,TEST_BUF_SIZE,out,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  l = mmini_decompress(out,l,tmp,TEST_BUF_SIZE,in,TEST_BUF_SIZE,huffheap);
  end = now();
  if (l == TEST_DATA_MAX) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != inbak[i]) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned wrong size)\n");
    return -1;
  }

#ifdef BENCH_AGAINST_ZLIB
  printf("ZLIB Deflate(1): Testing and Benchmarking\n");

  printf("  Highly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)(i & 0xf);

  start = now();
  l = TEST_BUF_SIZE;
  compress2(out,&l,in,TEST_DATA_MAX,1);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  i = TEST_BUF_SIZE;
  uncompress(in,&i,out,l);
  l = i;
  end = now();
  if (l) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != (unsigned char)(i & 0xf)) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned 0)\n");
    return -1;
  }

  printf("  Poorly compressible data:"); fflush(stdout);

  for(i=0;i<TEST_DATA_MAX;++i)
    in[i] = (unsigned char)((rand() >> 3) & 0xff);
  memcpy(inbak,in,TEST_DATA_MAX);

  start = now();
  l = TEST_BUF_SIZE;
  compress2(out,&l,in,TEST_DATA_MAX,1);
  end = now();
  if (l) {
    printf(" compress: %.2f%%, %f MiB/s,",100.0 - ((((double)l) / (double)TEST_DATA_MAX) * 100.0),(((double)TEST_DATA_MAX) / 1048576.0) / (end - start)); fflush(stdout);
  } else {
    printf(" compress: FAILED (returned 0)\n");
    return -1;
  }

  memset(in,0,TEST_BUF_SIZE);
  start = now();
  i = TEST_BUF_SIZE;
  uncompress(in,&i,out,l);
  l = i;
  end = now();
  if (l) {
    for(i=0;i<TEST_DATA_MAX;++i) {
      if (in[i] != inbak[i]) {
        printf(" decompress: FAILED (result mismatch @%lu)\n",i);
        return -1;
      }
    }
    printf(" decompress: %f MiB/s\n",(((double)TEST_DATA_MAX) / 1048576.0) / (end - start));
  } else {
    printf(" decompress: FAILED (returned 0)\n");
    return -1;
  }
#endif

  return 0;
}
