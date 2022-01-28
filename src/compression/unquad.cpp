// original code by ilia muraviev
// modifications for implementing it as memory2memory decompression only by Luigi Auriemma

// quad.cpp, version 1.12, author: ilia muraviev
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace std {}
using namespace std;

static unsigned char    *in   = NULL;
static unsigned char    *out  = NULL;
static unsigned char    *inl  = NULL;
static unsigned char    *outl = NULL;

//static FILE *in; // input file stream
//static FILE *out; // output file stream
static int xgetc(void *X) {
    if(in >= inl) return(-1);
    return(*in++);
}

class TDecoder {
private:
  unsigned int range;
  unsigned int buffer;

public:
  TDecoder(): range(0xffffffff) {}

  void Init() {
    for (int i = 0; i < 5; i++)
      buffer = (buffer << 8) + xgetc(in);
  }

  unsigned int GetCount(unsigned int total) {
    unsigned int count = buffer / (range /= total);
    if (count >= total)
      //fprintf(stderr, "data error\n"), exit(1);
      return(0);
    return (count);
  }

  void Update(unsigned int cum, unsigned int cnt) {
    buffer -= (cum * range);
    range *= cnt;
    while (range < (1 << 24)) {
      range <<= 8;
      buffer = (buffer << 8) + xgetc(in);
    }
  }
};

#define NUM 511 // number of symbols
#define TABSIZE 64

class TCounter {
private:
  inline void Rescale() {
    total = 0;
    for (int i = 0; i < NUM; i++)
      total += (cnt[i] -= (cnt[i] >> 1));
  }

public:
  unsigned short total; // 2 + (NUM * 2) = 1k
  unsigned short cnt[NUM];

  TCounter(): total(0) {
    memset(&cnt, 0, sizeof(cnt));
  }

  void Init(int n = NUM - 1) {
    for (int i = 0; i < n; i++)
      cnt[i] = 1;
    total = n;
  }

  void Add(int s) {
    total += 4;
    if ((cnt[s] += 4) >= 256)
      Rescale();
  }

  void AddX(int m) {
    cnt[m] += 8;
    if ((total += 8) >= (1 << 15))
      Rescale();
  }
};

static class TPPM {
private:
  enum {
    ESC = 510 // escape symbol
  };
  TCounter counter2[1024];
  TCounter counter0;
  TCounter counterE;
  TCounter counterX;

  int Decode(TCounter &counter) {
    unsigned int count = decoder.GetCount(counter.total);
    unsigned int cum = 0;

    int s = 0;
    while ((cum += counter.cnt[s]) <= count)
      s++;

    decoder.Update((cum - counter.cnt[s]), counter.cnt[s]);

    return (s);
  }

  void Set(TCounter &counter) {
    counterE.total = 0;
    for (int i = 0; i < NUM - 1; i++) {
      if (!counter.cnt[i])
        counterE.total += (counterE.cnt[i] = counter0.cnt[i]);
      else
        counterE.cnt[i] = 0;
    }
    //if (!counterE.total)
      //fprintf(stderr, "stream error\n"), exit(1);
  }

public:
  TDecoder decoder;

  TPPM() {
    counter0.Init();
    counterX.Init(TABSIZE);
  }

  int Decode(int x) {
    int s;
    if (counter2[x].total) {
      if ((s = Decode(counter2[x])) == ESC) {
        Set(counter2[x]);
        counter0.Add(s = Decode(counterE));
        counter2[x].Add(ESC);
      }
    }
    else {
      counter0.Add(s = Decode(counter0));
      counter2[x].Add(ESC);
    }
    counter2[x].Add(s);

    return (s);
  }

  int DecodeIndex() {
    int m = Decode(counterX);
    counterX.AddX(m);

    return (m);
  }
} ppm;

#define N (1 << 24) // buffer size: 16m
#define X (1 << 16)
#define MINMATCH 3
#define MAXMATCH 256

//static unsigned char buf[N + 4];
//static unsigned int  tab[X][TABSIZE];
#define buf     out

static void e8e9transform(int t, int n) {
  for (int i = 0; i < (n - 4); ) {
    switch (buf[i++]) {
    case 0xe8: // call
    case 0xe9: // jmp
      int addr = *(int *)(buf + i);

      if (t) {
        if ((addr >= -i) && (addr < (n - i)))
          addr += i;
        else if ((addr > 0) && (addr < n))
          addr -= n;
      }
      else {
        if (addr < 0) {
          if ((addr + i) >= 0)
            addr += n;
        }
        else if (addr < n)
          addr -= i;
      }
      *(int *)(buf + i) = addr;

      i += 4;
    }
  }
}

static void decode() {
  static unsigned int  **tab = NULL;
  static unsigned char *r = NULL;
  int   i;

  if(!r) r = (unsigned char *)malloc(X);
  if(!tab) {
    tab = new unsigned int *[X];
    for(i = 0; i < (X); i++) {
      tab[i] = new unsigned int[TABSIZE];
    }
  }

  int size = outl - out;
  //xread(&size, 1, sizeof(size), in);
  //if (size < 0)
    //fprintf(stderr, "size error\n"), exit(1);

  ppm.decoder.Init(); // [!]

  while (size > 0) {
    i = 0;
    for (; (i < 2) && (--size >= 0); i++)
      buf[i] = ppm.Decode(0);

    while ((i < N) && (--size >= 0)) {
      if((buf + i) >= outl) {   // only a lame check, nothing really important
        size = 0;   // force the exiting
        break;
      }

      int x = *(unsigned short *)(buf + (i - 2));

      int j = i;
      int s;
      if ((s = ppm.Decode(x >> 6)) >= 256) {
        size -= ((s -= (256 - MINMATCH)) - 1);

        int p = tab[x][(r[x] - ppm.DecodeIndex()) & (TABSIZE - 1)];
        while (--s >= 0)
          buf[i++] = buf[p++];
      }
      else
        buf[i++] = s;

      tab[x][++r[x] & (TABSIZE - 1)] = j;
    }
    e8e9transform(0, i);

    buf += i;
  }
}

extern "C" int unquad(unsigned char *src, int srcsz, unsigned char *dst, int dstsz) {
    in   = src;
    inl  = src + srcsz;
    out  = dst;
    outl = dst + dstsz;
    decode();
    return(out - dst);
}
