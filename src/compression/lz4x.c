// modified by Luigi Auriemma
// important improvement related to negative 's'!
// memory static->alloc

/*

LZ4X - An optimized LZ4 compressor

Written and placed in the public domain by Ilya Muravyov

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


    static unsigned char *fin_bck, *fin, *fout;
    static int flen;
    static int myfread(void *t, int elsz, int tsz, unsigned char *dummy2) {
        tsz *= elsz;
        if(tsz > ((fin_bck + flen) - fin)) tsz = ((fin_bck + flen) - fin);
        memcpy(t, fin, tsz);
        fin += tsz;
        return tsz;
    }
    static int myfwrite(void *t, int elsz, int tsz, unsigned char *dummy2) {
        tsz *= elsz;
        memcpy(fout, t, tsz);
        fout += tsz;
        return tsz;
    }
    #ifndef __max
    #define __max(a, b) ((a > b) ? a : b)
    #define __min(a, b) ((a < b) ? a : b)
    #endif


typedef unsigned char byte;
typedef unsigned int uint;

//#define LZ4_MAGIC 0x184C2102
#define BLOCK_SIZE (8<<20) // 8 MB
#define PADDING_LITERALS 8

#define WLOG 16
#define WSIZE (1<<WLOG)
#define WMASK (WSIZE-1)

#define MIN_MATCH 4

#define COMPRESS_BOUND (16+BLOCK_SIZE+(BLOCK_SIZE/255))

//static byte buf[BLOCK_SIZE+COMPRESS_BOUND];
static byte *buf = NULL;

#define HASH_LOG 18
#define HASH_SIZE (1<<HASH_LOG)
#define NIL (-1)

#ifdef FORCE_UNALIGNED
#  define load32(p) (*((const uint*)&buf[p]))
#else
  static uint load32(int p)
  {
    uint x;
    memcpy(&x, &buf[p], sizeof(uint));
    return x;
  }
#endif
#define hash32(p) ((load32(p)*0x125A517D)>>(32-HASH_LOG))
#define copy128(p, s) memcpy(&buf[p], &buf[s], 16)

#define get_byte() buf[BLOCK_SIZE+(bp++)]
#define put_byte(c) (buf[BLOCK_SIZE+(bsize++)]=(c))

static void compress(const int max_chain)
{
  //static int head[HASH_SIZE];
  static int *head = NULL;
  head = calloc(HASH_SIZE, sizeof(int));
  //static int tail[WSIZE];
  static int *tail = NULL;
  if(!tail) tail = calloc(WSIZE, sizeof(int));

#ifdef LZ4_MAGIC
  const uint magic=LZ4_MAGIC;
  myfwrite(&magic, 1, sizeof(magic), fout);
#endif

  //_fseeki64(fin, 0, SEEK_END);
  //const long long flen=_ftelli64(fin);
  //_fseeki64(fin, 0, SEEK_SET);

  int n;
  while ((n=myfread(buf, 1, BLOCK_SIZE, fin))>0)
  {
    int i;
    for (i=0; i<HASH_SIZE; ++i)
      head[i]=NIL;

    int bsize=0;
    int pp=0;

    int p=0;
    while (p<n)
    {
      int best_len=0;
      int dist;

      const int max_match=(n-PADDING_LITERALS)-p;
      if (max_match>=MIN_MATCH)
      {
        int chain_len=max_chain;
        const int wstart=__max(p-WSIZE, NIL);

        int s=head[hash32(p)];
        while (s>wstart)
        {
          if (buf[s+best_len]==buf[p+best_len] && load32(s)==load32(p))
          {
            int len=MIN_MATCH;
            while (len<max_match && buf[s+len]==buf[p+len])
              ++len;

            if (len>best_len)
            {
              best_len=len;
              dist=p-s;

              if (len==max_match)
                break;
            }
          }

          if (!--chain_len)
            break;

          s=tail[s&WMASK];
        }
      }

      if (best_len>=MIN_MATCH)
      {
        int len=best_len-MIN_MATCH;
        const int ml=__min(len, 15);

        if (pp<p)
        {
          int run=p-pp;
          if (run>=15)
          {
            put_byte((15<<4)+ml);

            run-=15;
            for (; run>=255; run-=255)
              put_byte(255);
            put_byte(run);
          }
          else
            put_byte((run<<4)+ml);

          while (pp<p)
            put_byte(buf[pp++]);
        }
        else
          put_byte(ml);

        put_byte(dist);
        put_byte(dist>>8);

        if (len>=15)
        {
          len-=15;
          for (; len>=255; len-=255)
            put_byte(255);
          put_byte(len);
        }

        pp=p+best_len;

        while (p<pp)
        {
          const uint h=hash32(p);
          tail[p&WMASK]=head[h];
          head[h]=p++;
        }
      }
      else
      {
        const uint h=hash32(p);
        tail[p&WMASK]=head[h];
        head[h]=p++;
      }
    }

    if (pp<p)
    {
      int run=p-pp;
      if (run>=15)
      {
        put_byte(15<<4);

        run-=15;
        for (; run>=255; run-=255)
          put_byte(255);
        put_byte(run);
      }
      else
        put_byte(run<<4);

      while (pp<p)
        put_byte(buf[pp++]);
    }

    myfwrite(&bsize, 1, sizeof(bsize), fout);
    myfwrite(&buf[BLOCK_SIZE], 1, bsize, fout);

    //if (flen>0)
    //  fprintf(stderr, "%3d%%\r", int((_ftelli64(fin)*100)/flen));
  }
}

static void compress_optimal()
{
  //static int head[HASH_SIZE];
  static int *head = NULL;
  if(!head) head = calloc(HASH_SIZE, sizeof(int));
  static int nodes[WSIZE][2];
  //static int **nodes = NULL;
  //if(!nodes) nodes = calloc(WSIZE * 2, sizeof(int));
  typedef struct
  {
    int cum;

    int len;
    int dist;
  } path_t;
  static path_t *path = NULL;
  if(!path) path = calloc(BLOCK_SIZE+1, sizeof(path_t));

#ifdef LZ4_MAGIC
  const uint magic=LZ4_MAGIC;
  myfwrite(&magic, 1, sizeof(magic), fout);
#endif

  //_fseeki64(fin, 0, SEEK_END);
  //const long long flen=_ftelli64(fin);
  //_fseeki64(fin, 0, SEEK_SET);

  int n,p;
  while ((n=myfread(buf, 1, BLOCK_SIZE, fin))>0)
  {
    // Pass 1: Find all matches

    int i;
    for ( i=0; i<HASH_SIZE; ++i)
      head[i]=NIL;

    for (p=0; p<n; ++p)
    {
      int best_len=0;
      int dist=0;

      const int max_match=__min(1<<14, (n-PADDING_LITERALS)-p); // [!]
      if (max_match>=MIN_MATCH)
      {
        int* left=&nodes[p&WMASK][1];
        int* right=&nodes[p&WMASK][0];

        int left_len=0;
        int right_len=0;

        const int wstart=__max(p-WSIZE, NIL);

        const uint h=hash32(p);
        int s=head[h];
        head[h]=p;

        while (s>wstart)
        {
          int len=__min(left_len, right_len);

          if (buf[s+len]==buf[p+len])
          {
            while (++len<max_match && buf[s+len]==buf[p+len]);

            if (len>best_len)
            {
              best_len=len;
              dist=p-s;

              if (len==max_match)
                break;
            }
          }

          if (buf[s+len]<buf[p+len])
          {
            *right=s;
            right=&nodes[s&WMASK][1];
            s=*right;
            right_len=len;
          }
          else
          {
            *left=s;
            left=&nodes[s&WMASK][0];
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

    int cnt=15;

    for (p=n-1; p>0; --p)
    {
      int c0=path[p+1].cum+1;

      if (!--cnt)
      {
        cnt=255;
        ++c0;
      }

      if (path[p].len>=MIN_MATCH)
      {
        path[p].cum=1<<30;

        for (i=path[p].len; i>=MIN_MATCH; --i)
        {
          int c1=path[p+i].cum+3;

          int len=i-MIN_MATCH;
          if (len>=15)
          {
            len-=15;
            for (; len>=255; len-=255)
              ++c1;
            ++c1;
          }

          if (c1<path[p].cum)
          {
            path[p].cum=c1;
            path[p].len=i;
          }
        }

        if (c0<path[p].cum)
        {
          path[p].cum=c0;
          path[p].len=0;
        }
        else
          cnt=15;
      }
      else
        path[p].cum=c0;
    }

    // Pass 3: Output the codes

    int bsize=0;
    int pp=0;

    p=0;
    while (p<n)
    {
      if (path[p].len>=MIN_MATCH)
      {
        int len=path[p].len-MIN_MATCH;
        const int ml=__min(len, 15);

        if (pp<p)
        {
          int run=p-pp;
          if (run>=15)
          {
            put_byte((15<<4)+ml);

            run-=15;
            for (; run>=255; run-=255)
              put_byte(255);
            put_byte(run);
          }
          else
            put_byte((run<<4)+ml);

          while (pp<p)
            put_byte(buf[pp++]);
        }
        else
          put_byte(ml);

        put_byte(path[p].dist);
        put_byte(path[p].dist>>8);

        if (len>=15)
        {
          len-=15;
          for (; len>=255; len-=255)
            put_byte(255);
          put_byte(len);
        }

        p+=path[p].len;

        pp=p;
      }
      else
        ++p;
    }

    if (pp<p)
    {
      int run=p-pp;
      if (run>=15)
      {
        put_byte(15<<4);

        run-=15;
        for (; run>=255; run-=255)
          put_byte(255);
        put_byte(run);
      }
      else
        put_byte(run<<4);

      while (pp<p)
        put_byte(buf[pp++]);
    }

    myfwrite(&bsize, 1, sizeof(bsize), fout);
    myfwrite(&buf[BLOCK_SIZE], 1, bsize, fout);

    //if (flen>0)
    //  fprintf(stderr, "%3d%%\r", int((_ftelli64(fin)*100)/flen));
  }
}

static int decompress()
{
    int i;
#ifdef LZ4_MAGIC
  uint magic;
  myfread(&magic, 1, sizeof(magic), fin);
  if (magic!=LZ4_MAGIC)
    return 2;
#endif

  int bsize;
  while (myfread(&bsize, 1, sizeof(bsize), fin)>0)
  {
#ifdef LZ4_MAGIC
    if (bsize==LZ4_MAGIC)
      continue;
#endif

    if (bsize<0 || bsize>COMPRESS_BOUND
        || myfread(&buf[BLOCK_SIZE], 1, bsize, fin)!=bsize)
      return 1;

    int p=0;

    int bp=0;
    while (bp<bsize)
    {
      const int tag=get_byte();
      if (tag>=16)
      {
        int run=tag>>4;
        if (run==15)
        {
          for (;;)
          {
            const int c=get_byte();
            run+=c;
            if (c!=255)
              break;
          }

          for ( i=0; i<run; i+=16)
            copy128(p+i, BLOCK_SIZE+bp+i);
        }
        else
          copy128(p, BLOCK_SIZE+bp);

        p+=run;
        bp+=run;

        if (bp>=bsize)
          break;
      }

      int s=p-get_byte();
      s-=get_byte()<<8;

      int len=tag&15;
      if (len==15)
      {
        for (;;)
        {
          const int c=get_byte();
          len+=c;
          if (c!=255)
            break;
        }
      }
      len+=4;

    /*
      if ((p-s)>=16)
      {
        for ( i=0; i<len; i+=16)
          copy128(p+i, s+i);
        p+=len;
      }
      else
      {
        while (len--)
          buf[p++]=buf[s++];
      }
    */
      // https://zenhax.com/viewtopic.php?p=34353#p34353
      while (len--)
        buf[p++] = (s<0) ? fout[s++] : buf[s++];

    }

    if (bp!=bsize)
      return 1;

    myfwrite(buf, 1, p, fout);
  }

  return 0;
}




int lz4x(unsigned char *in, int len, unsigned char *out, int dec_enc) {

    fin     = in;
    fout    = out;
    flen    = len;
    fin_bck = fin;
    unsigned char *fout_bck = fout;
    if(!buf) buf = calloc(BLOCK_SIZE+COMPRESS_BOUND, sizeof(byte));
    
    if(!dec_enc) {

        decompress();

    } else {

        compress_optimal();

    }

  return fout - fout_bck;
}

