// modified by Luigi Auriemma

/*

BCM - A BWT-based file compressor

Copyright (C) 2008-2018 Ilya Muravyov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/libdivsufsort/divsufsort.h"

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long long ulonglong;

static const char BCM_magic[]="BCM!";

unsigned char *bcm_fin;
unsigned char *bcm_fout;

class BCM {

struct Encoder
{
  uint low;
  uint high;
  uint code;

  Encoder()
  {
    low=0;
    high=uint(-1);
    code=0;
  }

  void EncodeBit0(uint p)
  {
#ifdef _WIN64
    low+=((ulonglong(high-low)*p)>>18)+1;
#else
    low+=((ulonglong(high-low)*(p<<(32-18)))>>32)+1;
#endif
    while ((low^high)<(1<<24))
    {
      *bcm_fout++ = low>>24;
      low<<=8;
      high=(high<<8)+255;
    }
  }

  void EncodeBit1(uint p)
  {
#ifdef _WIN64
    high=low+((ulonglong(high-low)*p)>>18);
#else
    high=low+((ulonglong(high-low)*(p<<(32-18)))>>32);
#endif
    while ((low^high)<(1<<24))
    {
      *bcm_fout++ = low>>24;
      low<<=8;
      high=(high<<8)+255;
    }
  }

  void Flush()
  {
    for (int i=0; i<4; ++i)
    {
      *bcm_fout++ = low>>24;
      low<<=8;
    }
  }

  void Init()
  {
    for (int i=0; i<4; ++i)
      code=(code<<8)+(*bcm_fin++);
  }

  int DecodeBit(uint p)
  {
#ifdef _WIN64
    const uint mid=low+((ulonglong(high-low)*p)>>18);
#else
    const uint mid=low+((ulonglong(high-low)*(p<<(32-18)))>>32);
#endif
    const int bit=(code<=mid);
    if (bit)
      high=mid;
    else
      low=mid+1;

    while ((low^high)<(1<<24))
    {
      low<<=8;
      high=(high<<8)+255;
      code=(code<<8)+(*bcm_fin++);
    }

    return bit;
  }
};

template<int RATE>
struct Counter
{
  word p;

  Counter()
  {
    p=1<<15;
  }

  void UpdateBit0()
  {
    p-=p>>RATE;
  }

  void UpdateBit1()
  {
    p+=(p^65535)>>RATE;
  }
};

struct CM: Encoder
{
  Counter<2> counter0[256];
  Counter<4> counter1[256][256];
  Counter<6> counter2[2][256][17];
  int c1;
  int c2;
  int run;

  CM()
  {
    c1=0;
    c2=0;
    run=0;

    for (int i=0; i<2; ++i)
    {
      for (int j=0; j<256; ++j)
      {
        for (int k=0; k<17; ++k)
          counter2[i][j][k].p=(k<<12)-(k==16);
      }
    }
  }

  void Encode32(uint n)
  {
    for (int i=0; i<32; ++i)
    {
      if (n&(1<<31))
        Encoder::EncodeBit1(1<<17);
      else
        Encoder::EncodeBit0(1<<17);
      n+=n;
    }
  }

  uint Decode32()
  {
    uint n=0;
    for (int i=0; i<32; ++i)
      n+=n+Encoder::DecodeBit(1<<17);

    return n;
  }

  void Encode(int c)
  {
    if (c1==c2)
      ++run;
    else
      run=0;
    const int f=(run>2);

    int ctx=1;
    while (ctx<256)
    {
      const int p0=counter0[ctx].p;
      const int p1=counter1[c1][ctx].p;
      const int p2=counter1[c2][ctx].p;
      const int p=((p0+p1)*7+p2+p2)>>4;

      const int j=p>>12;
      const int x1=counter2[f][ctx][j].p;
      const int x2=counter2[f][ctx][j+1].p;
      const int ssep=x1+(((x2-x1)*(p&4095))>>12);

      const int bit=c&128;
      c+=c;

      if (bit)
      {
        Encoder::EncodeBit1(ssep*3+p);
        counter0[ctx].UpdateBit1();
        counter1[c1][ctx].UpdateBit1();
        counter2[f][ctx][j].UpdateBit1();
        counter2[f][ctx][j+1].UpdateBit1();
        ctx+=ctx+1;
      }
      else
      {
        Encoder::EncodeBit0(ssep*3+p);
        counter0[ctx].UpdateBit0();
        counter1[c1][ctx].UpdateBit0();
        counter2[f][ctx][j].UpdateBit0();
        counter2[f][ctx][j+1].UpdateBit0();
        ctx+=ctx;
      }
    }

    c2=c1;
    c1=ctx&255;
  }

  int Decode()
  {
    if (c1==c2)
      ++run;
    else
      run=0;
    const int f=(run>2);

    int ctx=1;
    while (ctx<256)
    {
      const int p0=counter0[ctx].p;
      const int p1=counter1[c1][ctx].p;
      const int p2=counter1[c2][ctx].p;
      const int p=((p0+p1)*7+p2+p2)>>4;

      const int j=p>>12;
      const int x1=counter2[f][ctx][j].p;
      const int x2=counter2[f][ctx][j+1].p;
      const int ssep=x1+(((x2-x1)*(p&4095))>>12);

      const int bit=Encoder::DecodeBit(ssep*3+p);

      if (bit)
      {
        counter0[ctx].UpdateBit1();
        counter1[c1][ctx].UpdateBit1();
        counter2[f][ctx][j].UpdateBit1();
        counter2[f][ctx][j+1].UpdateBit1();
        ctx+=ctx+1;
      }
      else
      {
        counter0[ctx].UpdateBit0();
        counter1[c1][ctx].UpdateBit0();
        counter2[f][ctx][j].UpdateBit0();
        counter2[f][ctx][j+1].UpdateBit0();
        ctx+=ctx;
      }
    }

    c2=c1;
    return c1=ctx&255;
  }
} cm;

struct CRC
{
  uint t[256];
  uint crc;

  CRC()
  {
    for (int i=0; i<256; ++i)
    {
      uint r=i;
      for (int j=0; j<8; ++j)
        r=(r>>1)^(-int(r&1)&0xedb88320);
      t[i]=r;
    }

    crc=uint(-1);
  }

  void Clear()
  {
    crc=uint(-1);
  }

  uint operator()() const
  {
    return ~crc;
  }

  void Update(int c)
  {
    crc=t[(crc^c)&255]^(crc>>8);
  }
} crc;

public:

int compress(unsigned char *buf, int bsize, unsigned char *output, int raw)
{
    bcm_fin  = buf;
    bcm_fout = output;

  int* ptr=(int*)calloc(bsize, sizeof(int));

    if(!raw) {
        memcpy(bcm_fout, BCM_magic, 4);
        bcm_fout += 4;
    }

    // only raw
    /*
  putc(BCM_magic[0], bcm_fout);
  putc(BCM_magic[1], bcm_fout);
  putc(BCM_magic[2], bcm_fout);
  putc(BCM_magic[3], bcm_fout);
    */

  int n;
  while (n=bsize/*(n=fread(buf, 1, bsize, bcm_fin))>0*/)
  {
    for (int i=0; i<n; ++i)
      crc.Update(buf[i]);

    const int idx=divbwt(buf, buf, ptr, n, NULL, NULL, 0);
    if (idx<1)
    {
      perror("Divbwt failed");
      return -1; //exit(1);
    }

    cm.Encode32(n);
    cm.Encode32(idx);

    for (int i=0; i<n; ++i)
      cm.Encode(buf[i]);

  }

  cm.Encode32(0); // EOF
  cm.Encode32(crc());

  cm.Flush();

  free(ptr);
  return bcm_fout - output;
}

int decompress(unsigned char *in, int zsize, unsigned char *output)
{
    bcm_fin  = in;
    bcm_fout = output;

    if(!memcmp(bcm_fin, BCM_magic, 4)) bcm_fin += 4;

    // only raw
    /*
  if (getc(bcm_fin)!=BCM_magic[0]
      ||getc(bcm_fin)!=BCM_magic[1]
      ||getc(bcm_fin)!=BCM_magic[2]
      ||getc(bcm_fin)!=BCM_magic[3])
  {
    fprintf(stderr, "Not in BCM format!\n");
    exit(1);
  }
    */

  cm.Init();

  int bsize=0;
  byte* buf=NULL;
  uint* ptr=NULL;

  int n;
  while ((n=cm.Decode32())>0)
  {
    if (!bsize)
    {
      if ((bsize=n)>=(1<<24)) // 5n
        buf=(byte*)calloc(bsize, sizeof(byte));

      ptr=(uint*)calloc(bsize, sizeof(uint));
    }

    const int idx=cm.Decode32();
    if (n>bsize || idx<1 || idx>n)
    {
      fprintf(stderr, "Corrupt input!\n");
      return -1; //exit(1);
    }

    // Inverse BW-transform
    if (n>=(1<<24)) // 5n
    {
      int t[257]={0};
      for (int i=0; i<n; ++i)
        ++t[(buf[i]=cm.Decode())+1];
      for (int i=1; i<256; ++i)
        t[i]+=t[i-1];
      for (int i=0; i<n; ++i)
        ptr[t[buf[i]]++]=i+(i>=idx);
      for (int p=idx; p;)
      {
        p=ptr[p-1];
        const int c=buf[p-(p>=idx)];
        *bcm_fout++ = c;
        crc.Update(c);
      }
    }
    else // 4n
    {
      int t[257]={0};
      for (int i=0; i<n; ++i)
        ++t[(ptr[i]=cm.Decode())+1];
      for (int i=1; i<256; ++i)
        t[i]+=t[i-1];
      for (int i=0; i<n; ++i)
        ptr[t[ptr[i]&255]++]|=(i+(i>=idx))<<8;
      for (int p=idx; p;)
      {
        p=ptr[p-1]>>8;
        const int c=ptr[p-(p>=idx)]&255;
        *bcm_fout++ = c;
        crc.Update(c);
      }
    }

  }

  if (cm.Decode32()!=crc())
  {
    fprintf(stderr, "CRC error!\n");
    //return -1; //exit(1);
  }

  free(buf);
  free(ptr);
  return bcm_fout - output;
}

};



extern "C" int bcm_compress(unsigned char *in, int insz, unsigned char *out) {
    BCM bcm;
    return bcm.compress(in, insz, out, 1);
}

extern "C" int bcm_decompress(unsigned char *in, int insz, unsigned char *out) {
    BCM bcm;
    return bcm.decompress(in, insz, out);
}


