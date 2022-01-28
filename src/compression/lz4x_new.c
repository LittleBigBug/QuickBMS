// modified by Luigi Auriemma
// LZ4X >= v1.50 (this is v1.60)
// lz4x.c is left as-is because it contains a customization
// working on a specific game and still keeping it compatible
// update: the customization has been added to this code too (decompress: s < 0)
// memory static->alloc

/*

LZ4X - An optimized LZ4 compressor

Written and placed in the public domain by Ilya Muravyov

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



    static unsigned char *g_in_bck, *g_in, *g_out;
    static int flen;
    static int myfread(void *t, int elsz, int tsz, unsigned char *dummy2) {
        tsz *= elsz;
        if(tsz > ((g_in_bck + flen) - g_in)) tsz = ((g_in_bck + flen) - g_in);
        memcpy(t, g_in, tsz);
        g_in += tsz;
        return tsz;
    }
    static int myfwrite(void *t, int elsz, int tsz, unsigned char *dummy2) {
        tsz *= elsz;
        memcpy(g_out, t, tsz);
        g_out += tsz;
        return tsz;
    }
    #ifndef __max
    #define __max(a, b) ((a > b) ? a : b)
    #define __min(a, b) ((a < b) ? a : b)
    #endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef NO_UTIME
#  include <sys/types.h>
#  include <sys/stat.h>

#  ifdef _MSC_VER
#    include <sys/utime.h>
#  else
#    include <utime.h>
#  endif
#endif

#ifndef _MSC_VER
#  define _ftelli64 ftello64
#endif

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

//FILE* g_in;
//FILE* g_out;

#define LZ4_MAGIC 0x184C2102
#define BLOCK_SIZE (8<<20) // 8 MB
#define PADDING_LITERALS 5

#define WINDOW_BITS 16
#define WINDOW_SIZE (1<<WINDOW_BITS)
#define WINDOW_MASK (WINDOW_SIZE-1)

#define MIN_MATCH 4

#define EXCESS (16+(BLOCK_SIZE/255))

static U8 *g_buf = NULL; //[BLOCK_SIZE+BLOCK_SIZE+EXCESS];

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

#define LOAD_16(p) (*(U16*)(&g_buf[p]))
#define LOAD_32(p) (*(U32*)(&g_buf[p]))
#define STORE_16(p, x) (*(U16*)(&g_buf[p])=(x))
#define COPY_32(d, s) (*(U32*)(&g_buf[d])=LOAD_32(s))

#define HASH_BITS 18
#define HASH_SIZE (1<<HASH_BITS)
#define NIL (-1)

#define HASH_32(p) ((LOAD_32(p)*0x9E3779B9)>>(32-HASH_BITS))

static void wild_copy(int d, int s, int n)
{
  COPY_32(d, s);
  COPY_32(d+4, s+4);

  int i;
  for (i=8; i<n; i+=8)
  {
    COPY_32(d+i, s+i);
    COPY_32(d+4+i, s+4+i);
  }
}

static void compress(const int max_chain)
{
  //static int head[HASH_SIZE];
  //static int tail[WINDOW_SIZE];
  static int *head = NULL;
  head = calloc(HASH_SIZE, sizeof(int));
  static int *tail = NULL;
  if(!tail) tail = calloc(WINDOW_SIZE, sizeof(int));

  int n;
  while ((n=myfread(g_buf, 1, BLOCK_SIZE, g_in))>0)
  {
    int i;
    for (i=0; i<HASH_SIZE; ++i)
      head[i]=NIL;

    int op=BLOCK_SIZE;
    int pp=0;

    int p=0;
    while (p<n)
    {
      int best_len=0;
      int dist=0;

      const int max_match=(n-PADDING_LITERALS)-p;
      if (max_match>=MAX(12-PADDING_LITERALS, MIN_MATCH))
      {
        const int limit=MAX(p-WINDOW_SIZE, NIL);
        int chain_len=max_chain;

        int s=head[HASH_32(p)];
        while (s>limit)
        {
          if (g_buf[s+best_len]==g_buf[p+best_len] && LOAD_32(s)==LOAD_32(p))
          {
            int len=MIN_MATCH;
            while (len<max_match && g_buf[s+len]==g_buf[p+len])
              ++len;

            if (len>best_len)
            {
              best_len=len;
              dist=p-s;

              if (len==max_match)
                break;
            }
          }

          if (--chain_len==0)
            break;

          s=tail[s&WINDOW_MASK];
        }
      }

      if (best_len>=MIN_MATCH)
      {
        int len=best_len-MIN_MATCH;
        const int nib=MIN(len, 15);

        if (pp!=p)
        {
          const int run=p-pp;
          if (run>=15)
          {
            g_buf[op++]=(15<<4)+nib;

            int j=run-15;
            for (; j>=255; j-=255)
              g_buf[op++]=255;
            g_buf[op++]=j;
          }
          else
            g_buf[op++]=(run<<4)+nib;

          wild_copy(op, pp, run);
          op+=run;
        }
        else
          g_buf[op++]=nib;

        STORE_16(op, dist);
        op+=2;

        if (len>=15)
        {
          len-=15;
          for (; len>=255; len-=255)
            g_buf[op++]=255;
          g_buf[op++]=len;
        }

        pp=p+best_len;

        while (p<pp)
        {
          const U32 h=HASH_32(p);
          tail[p&WINDOW_MASK]=head[h];
          head[h]=p++;
        }
      }
      else
      {
        const U32 h=HASH_32(p);
        tail[p&WINDOW_MASK]=head[h];
        head[h]=p++;
      }
    }

    if (pp!=p)
    {
      const int run=p-pp;
      if (run>=15)
      {
        g_buf[op++]=15<<4;

        int j=run-15;
        for (; j>=255; j-=255)
          g_buf[op++]=255;
        g_buf[op++]=j;
      }
      else
        g_buf[op++]=run<<4;

      wild_copy(op, pp, run);
      op+=run;
    }

    const int comp_len=op-BLOCK_SIZE;
    myfwrite((void *)&comp_len, 1, sizeof(comp_len), g_out);
    myfwrite((void *)&g_buf[BLOCK_SIZE], 1, comp_len, g_out);

    //fprintf(stderr, "%lld -> %lld\r", _ftelli64(g_in), _ftelli64(g_out));
  }
}

static void compress_optimal()
{
  //static int head[HASH_SIZE];
  static int *head = NULL;
  if(!head) head = calloc(HASH_SIZE, sizeof(int));
  static int nodes[WINDOW_SIZE][2];
  static struct
  {
    int cum;

    int len;
    int dist;
  } path[BLOCK_SIZE+1];

  int n;
  while ((n=myfread(g_buf, 1, BLOCK_SIZE, g_in))>0)
  {
    // Pass 1: Find all matches

    int i;
    for (i=0; i<HASH_SIZE; ++i)
      head[i]=NIL;

    int p;
    for (p=0; p<n; ++p)
    {
      int best_len=0;
      int dist=0;

      const int max_match=(n-PADDING_LITERALS)-p;
      if (max_match>=MAX(12-PADDING_LITERALS, MIN_MATCH))
      {
        const int limit=MAX(p-WINDOW_SIZE, NIL);

        int* left=&nodes[p&WINDOW_MASK][1];
        int* right=&nodes[p&WINDOW_MASK][0];

        int left_len=0;
        int right_len=0;

        const U32 h=HASH_32(p);
        int s=head[h];
        head[h]=p;

        while (s>limit)
        {
          int len=MIN(left_len, right_len);

          if (g_buf[s+len]==g_buf[p+len])
          {
            while (++len<max_match && g_buf[s+len]==g_buf[p+len]);

            if (len>best_len)
            {
              best_len=len;
              dist=p-s;

              if (len==max_match || len>=(1<<16))
                break;
            }
          }

          if (g_buf[s+len]<g_buf[p+len])
          {
            *right=s;
            right=&nodes[s&WINDOW_MASK][1];
            s=*right;
            right_len=len;
          }
          else
          {
            *left=s;
            left=&nodes[s&WINDOW_MASK][0];
            s=*left;
            left_len=len;
          }
        }

        *left=NIL;
        *right=NIL;
      }

      path[p].len=best_len;
      path[p].dist=dist;
    }

    // Pass 2: Build the shortest path

    path[n].cum=0;

    int count=15;

    for (p=n-1; p>0; --p)
    {
      int c0=path[p+1].cum+1;

      if (--count==0)
      {
        count=255;
        ++c0;
      }

      int len=path[p].len;
      if (len>=MIN_MATCH)
      {
        int c1=1<<30;

        const int j=MAX(len-255, MIN_MATCH);
        for (i=len; i>=j; --i)
        {
          int tmp=path[p+i].cum+3;

          if (i>=(15+MIN_MATCH))
            tmp+=1+((i-(15+MIN_MATCH))/255);

          if (tmp<c1)
          {
            c1=tmp;
            len=i;
          }
        }

        if (c1<=c0)
        {
          path[p].cum=c1;
          path[p].len=len;

          count=15;
        }
        else
        {
          path[p].cum=c0;
          path[p].len=0;
        }
      }
      else
        path[p].cum=c0;
    }

    // Pass 3: Output the codes

    int op=BLOCK_SIZE;
    int pp=0;

    while (p<n)
    {
      if (path[p].len>=MIN_MATCH)
      {
        int len=path[p].len-MIN_MATCH;
        const int nib=MIN(len, 15);

        if (pp!=p)
        {
          const int run=p-pp;
          if (run>=15)
          {
            g_buf[op++]=(15<<4)+nib;

            int j=run-15;
            for (; j>=255; j-=255)
              g_buf[op++]=255;
            g_buf[op++]=j;
          }
          else
            g_buf[op++]=(run<<4)+nib;

          wild_copy(op, pp, run);
          op+=run;
        }
        else
          g_buf[op++]=nib;

        STORE_16(op, path[p].dist);
        op+=2;

        if (len>=15)
        {
          len-=15;
          for (; len>=255; len-=255)
            g_buf[op++]=255;
          g_buf[op++]=len;
        }

        p+=path[p].len;

        pp=p;
      }
      else
        ++p;
    }

    if (pp!=p)
    {
      const int run=p-pp;
      if (run>=15)
      {
        g_buf[op++]=15<<4;

        int j=run-15;
        for (; j>=255; j-=255)
          g_buf[op++]=255;
        g_buf[op++]=j;
      }
      else
        g_buf[op++]=run<<4;

      wild_copy(op, pp, run);
      op+=run;
    }

    const int comp_len=op-BLOCK_SIZE;
    myfwrite((void *)&comp_len, 1, sizeof(comp_len), g_out);
    myfwrite((void *)&g_buf[BLOCK_SIZE], 1, comp_len, g_out);

    //fprintf(stderr, "%lld -> %lld\r", _ftelli64(g_in), _ftelli64(g_out));
  }
}

static int decompress()
{
  int comp_len;
  while (myfread(&comp_len, 1, sizeof(comp_len), g_in)>0)
  {
    if (comp_len==LZ4_MAGIC)
      continue;

    if (comp_len<2 || comp_len>(BLOCK_SIZE+EXCESS)
        || myfread(&g_buf[BLOCK_SIZE], 1, comp_len, g_in)!=comp_len)
      return -1;

    int p=0;

    int ip=BLOCK_SIZE;
    const int ip_end=ip+comp_len;

    for (;;)
    {
      const int token=g_buf[ip++];
      if (token>=16)
      {
        int run=token>>4;
        if (run==15)
        {
          for (;;)
          {
            const int c=g_buf[ip++];
            run+=c;
            if (c!=255)
              break;
          }
        }
        if ((p+run)>BLOCK_SIZE)
          return -1;

        wild_copy(p, ip, run);
        p+=run;
        ip+=run;
        if (ip>=ip_end)
          break;
      }

      int s=p-LOAD_16(ip);
      ip+=2;
      //if (s<0)
      //  return -1;

      int len=(token&15)+MIN_MATCH;
      if (len==(15+MIN_MATCH))
      {
        for (;;)
        {
          const int c=g_buf[ip++];
          len+=c;
          if (c!=255)
            break;
        }
      }
      if ((p+len)>BLOCK_SIZE)
        return -1;

      /*
      if ((p-s)>=4)
      {
        wild_copy(p, s, len);
        p+=len;
      }
      else
      {
        while (len--!=0)
          g_buf[p++]=g_buf[s++];
      }
      */
      // https://zenhax.com/viewtopic.php?p=34353#p34353
        while (len--!=0)
          g_buf[p++]=(s<0) ? g_out[s++] : g_buf[s++];
    }

    if (myfwrite(g_buf, 1, p, g_out)!=p)
    {
      return -1;
      //perror("Fwrite() failed");
      //exit(1);
    }
  }

  return 0;
}

int lz4x_new(unsigned char *in, int len, unsigned char *out, int dec_enc) {

    g_in     = in;
    g_out    = out;
    flen     = len;
    g_in_bck = g_in;
    unsigned char *g_out_bck = g_out;
    if(!g_buf) g_buf = calloc(BLOCK_SIZE+BLOCK_SIZE+EXCESS, sizeof(U8));

    if(!dec_enc) {

        decompress();

    } else {

        compress_optimal();

    }

  return g_out - g_out_bck;
}

