// modified by Luigi Auriemma
/* SR3.cpp symbol ranking file compressor, 27.10.2007.
(C) 2007, Matt Mahoney, matmahoney@yahoo.com

- ****************************************************
  Update by Nania Francesco Antonio (Italy):
    * added Wave compression;
    * implemented compression Txt,Bmp,Doc etc;
    * require only 64 MB of memory;
- ****************************************************
- ****************************************************
  Update by Andrew Paterson (UK):
    * Changed file id to sR3;
    * "Profile" stored as 1 byte;
    * Additional file types detected
    * Will create sr2 or (new) sr3 compressed files;
    * Will decompress sr2, old sr3 and new sr3 files;
    * Avoid some issues with endianness;
    * Does not use global variables;
    * Small efficiency improvements;
- ****************************************************

    LICENSE for sr3a.cpp

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
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
#include <assert.h>

// 8, 16, 32 bit unsigned types (adjust as appropriate)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

namespace sr3 {

/*
static const char *profiles[] = {
  "",           //  0 - Default
  " (LOG)",     //  1 - Log files
  " (BMP)",     //  2 - Bitmap image
  " (Dict)",    //  3 - Dictionary
  " (EXE)",     //  4 - Executable
  " (HLP)",     //  5 - Help file
  " (ZIP)",     //  6 - ZIP compressed file
  " (PDF)",     //  7 - Adobe Acrobat file
  " (JPEG)",    //  8 - JPEG image
  " (Text)",    //  9 - Text
  " (ISO)",     // 10 - ISO disk image
  " (PGM)",     // 11 - PGM image
  " (DSG)",     // 12 - Doom save game
  " (MP3)",     // 13 - MPEG audio file
  " (DOC)",     // 14 - MS Word file
  " (PNG)",     // 15 - PNG image
  " (GZIP)",    // 16 - GZIP compressed file
  " (BZ2)",     // 17 - BZIP2 compressed file
  " (WAV)",     // 18 - stereo - 16 bit
  " (WAV-M16)", // 19 - mono - 16 bit
  " (WAV-S8)",  // 20 - stereo - 16 bit
  " (WAV-M8)",  // 21 - mono - 8 bit
  " (WAV-S24)", // 22 - stereo HQ - 24 bit
  " (WAV-M24)", // 23 - mono HQ - 24 bit
  " (WAV-S32)", // 24 - stereo HQ - 32 bit
  " (WAV-M32)", // 25 - mono HQ - 32 bit
};
*/

// Identifying bytes for different file types
static const unsigned char BMP_HEADER[]  = { 77 };
static const unsigned char BZ2_HEADER1[] = { 90, 104 };
static const unsigned char BZ2_HEADER2[] = { 49, 65, 89, 38, 83, 89 };
static const unsigned char DSG_HEADER[]  = { 68, 79, 79, 77, 32 };
static const unsigned char EXE_HEADER[]  = { 90, 144, 0, 3, 0, 0, 0 };
static const unsigned char GZP_HEADER[]  = { 139, 8, 0 };
static const unsigned char HLP_HEADER[]  = { 95, 3, 0 };
static const unsigned char JPG_HEADER1[] = { 216, 255, 224 };
static const unsigned char JPG_HEADER2[] = { 74, 70, 73, 70 };
static const unsigned char MPG_HEADER[]  = { 68, 51, 3 };
static const unsigned char PDF_HEADER[]  = { 80, 68, 70 };
static const unsigned char PGM_HEADER[]  = { 53, 10, 55 };
static const unsigned char PNG_HEADER[]  = { 80, 78, 71, 13, 10, 26, 10 };
static const unsigned char DOC_HEADER[]  = { 207, 17, 224, 161, 177, 26, 225 };
static const unsigned char WAV_HEADER[]  = { 73, 70, 70 };
static const unsigned char ZIP_HEADER[]  = { 75, 3, 4, 20, 0, 2, 0, 8, 0 };


// ----------------------------------------------------------------------------
// Get the unsigned 16-bit value starting at the given position

static U16 Peekw(unsigned char *memory)
{
  // U16 result;
  // memcpy(&result,memory,2);
  // return (result);
  U16 lsb = *memory;
  U16 msb = *(memory + 1);
  return ((msb << 8) | lsb);
}

// ----------------------------------------------------------------------------
// Error handler: print message if any, and exit

static void quit(const char* message=0)
{
  if (message) printf("%s\n", message);
  exit(1);
}

// ----------------------------------------------------------------------------
// Get the hash multiplier based on the file type

static int GetMult(int profile)
{
  int shft = 5;
  if (profile == 2) shft = 6; // BMP
  return (5 << shft);
}


// ----------------------------------------------------------------------------
// Get the number of channels based on the file type (default = 1)

static int GetChannels(int profile)
{
  int channels = 1;
       if (profile == 18) channels = 4; // Wave Audio - stereo - 16 bit
  else if (profile == 19) channels = 2; // Wave Audio - mono - 16 bit
  else if (profile == 20) channels = 2; // Wave Audio - stereo - 8 bit
  // else if (profile == 21) channels = 1; // Wave Audio - mono - 8 bit
  else if (profile == 22) channels = 6; // Wave Audio - stereo HQ - 24 bit
  else if (profile == 23) channels = 3; // Wave Audio - mono HQ - 24 bit
  else if (profile == 24) channels = 8; // Wave Audio - stereo HQ - 32 bit
  else if (profile == 25) channels = 4; // Wave Audio - mono HQ - 32 bit
  return channels;
}


// ----------------------------------------------------------------------------
// Create an array p of n elements of type T

template <class T> void alloc(T*&p, int n)
{
  p=(T*)calloc(n, sizeof(T));
  if (!p) quit("out of memory");
}


// ----------------------------------------------------------------------------
// A xStateMap maps a secondary context to a probability.  Methods:
//
// xStateMap sm(n) creates a xStateMap with n contexts using 4*n bytes memory.
// sm.p(cxt) predicts next bit (0..4K-1) in context cxt (0..n-1).
// sm.update(cxt, y) updates model for actual bit y (0..1) in cxt (0..n-1).

class xStateMap
{
protected:
  const int N;  // Number of contexts
  U32 *t;       // cxt -> prediction in high 25 bits, count in low 7 bits
  static int dt[128];  // i -> 1K/(i+2)
public:
  xStateMap(int n);

  // predict next bit in context cxt
  int p(int cxt)
  {
    assert(cxt>=0 && cxt<N);
    return t[cxt]>>20;
  }

  // update model in context cxt for actual bit y
  void update(int cxt, int y)
  {
    assert(cxt>=0 && cxt<N);
    assert(y==0 || y==1);
    int n=t[cxt]&127, p=t[cxt]>>9;  // count, prediction
    if (n<127) ++t[cxt];
    t[cxt]+=((y<<23)-p)*dt[n]&0xffffff80;
  }
};

int xStateMap::dt[128]={0};

xStateMap::xStateMap(int n): N(n)
{
  alloc(t, N);
  for (int i=0; i<N; ++i)
    t[i]=1<<31;
  if (dt[0]==0)
    for (int i=0; i<128; ++i)
      dt[i]=512/(i+2);
}

// ----------------------------------------------------------------------------
// A Decoder does arithmetic decoding in n contexts.  Methods:
// Decoder(f, n) creates decoder for decompression from archive f,
//     which must be open past any header for reading in binary mode.
// code(cxt) returns the next decompressed bit from file f
//   with context cxt in 0..n-1.

class Decoder {
private:
  const int N;     // number of contexts
  unsigned char** archive;   // Compressed data file
  unsigned char *archivel;
  U32 x1, x2;      // Range, initially [0, 1), scaled by 2^32
  U32 x;           // Decompress mode: last 4 input bytes of archive
  xStateMap sm;     // cxt -> p
public:
  Decoder(unsigned char** f, unsigned char *, int n);
  int code(int cxt);  // decompress a bit
};

// Return decompressed bit (0..1) in context cxt (0..n-1)
inline int Decoder::code(int cxt) {
  assert(cxt>=0 && cxt<N);
  int p=sm.p(cxt);
  assert(p>=0 && p<4096);
  U32 xmid=x1 + (x2-x1>>12)*p;
  assert(xmid>=x1 && xmid<x2);
  int y=x<=xmid;
  y ? (x2=xmid) : (x1=xmid+1);
  sm.update(cxt, y);
  while (((x1^x2)&0xff000000)==0) {  // pass equal leading bytes of range
    x1<<=8;
    x2=(x2<<8)+255;
    int c;
    if(*archive >= archivel) c = 0;
    else c = *(*archive)++;
    x=(x<<8)+c;  // EOF is OK
  }
  return y;
}

Decoder::Decoder(unsigned char** f, unsigned char *fl, int n):
    N(n), archive(f), archivel(fl), x1(0), x2(0xffffffff), x(0), sm(n) {
  for (int i=0; i<4; ++i) {
    int c;
    if(*archive >= archivel) c = 0;
    else c = *(*archive)++;
    x=(x<<8)+c;
  }
}


// ----------------------------------------------------------------------------
// Decode one byte
static int decode(Decoder& e, int cxt) {
  int hi=1, lo=1;  // high and low nibbles
  hi+=hi+e.code(cxt+hi);
  hi+=hi+e.code(cxt+hi);
  hi+=hi+e.code(cxt+hi);
  hi+=hi+e.code(cxt+hi);
  cxt+=15*(hi-15);
  lo+=lo+e.code(cxt+lo);
  lo+=lo+e.code(cxt+lo);
  lo+=lo+e.code(cxt+lo);
  lo+=lo+e.code(cxt+lo);
  return hi-16<<4|(lo-16);
}

}


// ----------------------------------------------------------------------------
// Decompress from in to out.  in should be positioned past the header.
extern "C" int sr3_decompress(unsigned char *in, int insz, unsigned char *out, int outsz, int version, int profile) {
    unsigned char *out_bck = out;
    unsigned char *inl;
    if(profile < 0) {
        profile = *in++;
        insz--;
    }
    inl = in + insz;

  sr3::Decoder e(&in, inl, (1024+1024)*258);
  const int cshft = (version == 2)? 24: 20; // Bit shift for context
  const int hmask = (version == 2)? 0xfffff: 0xffffff;  // Hash mask
  const int hmult = sr3::GetMult(profile);       // Multiplier for hashes
  const int maxc = sr3::GetChannels(profile);       // Number of channels
  int channel = 0;                          // Channel for WAV files
  int c1=0; // previous byte
  U32 *t4;  // context -> last 3 bytes in bits 0..23, count in 24..29
  U32 hc[16];                     // Hash of last 4 bytes by channel

  sr3::alloc(t4, 0x1000000);
  memset(hc, 0, sizeof(hc));
  while (in < inl) {
    const U32 h = hc[channel];
    const U32 r=t4[h];  // last byte count, last 3 bytes in this context
    int cxt;  // context

    if (r>=0x4000000) cxt=1024+(r>>cshft);
    else cxt=c1|r>>16&0x3f00;
    cxt*=258;

    // Decompress: 0=p[1], 110=p[2], 111=p[3], 10xxxxxxxx=literal.
    // EOF is marked by p[1] coded as a literal.
    if (e.code(cxt)) {
      if (e.code(cxt+1)) {
        if (e.code(cxt+2)) {  // match third?
          c1=r>>16&0xff;
          t4[h]=r<<8&0xffff00|c1|0x1000000;
        }
        else {  // match second?
          c1=r>>8&0xff;
          t4[h]=r&0xff0000|r<<8&0xff00|c1|0x1000000;
        }
      }
      else {  // literal?
        c1=decode(e, cxt+2);
        if (c1==int(r&0xff)) break;  // EOF?
        t4[h]=r<<8&0xffff00|c1;
      }
    }
    else {  // match first?
      c1=r&0xff;
      if (r<0x3f000000) t4[h]+=0x1000000;  // increment count
    }
    if(out >= out_bck + outsz) break;   // don't return -1
    *out++ = c1;
    hc[channel]=(h*hmult+c1+1)&hmask;
    if (++channel >= maxc) channel=0;
  }
  return out - out_bck;
}

