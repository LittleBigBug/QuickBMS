// http://cnub.ddns.net/deLZW.ashx
/* File: deLZW.c
   I know this is REAL fugly code with all the globals and no comments,
   but, for one, it is an adaption of a highly optimized piece of hand-coded
   8086 assembly code {circa 1992, the machine code for all of this 'C' code
   is only 378 bytes (eat your heart out, GCC and Visual C++ ;) }.
   For two, you can reference a commented copy of the LZW source from
   http://www.dogma.net/markn/articles/lzw/lzw.htm. */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define true    1
#define false   0

typedef          int       BOOL;
typedef unsigned char      BYTE;
typedef unsigned short int WORD;
typedef unsigned long      DWORD;

static BOOL   eof, rep90;
static DWORD* src;   // input  cursor
static BYTE*  dst;   // output cursor
static BYTE*  edst;  // end of dst (original dst + dstSize)
static DWORD  buf;   // input bit buffer
static int    size;  // current number of bits per input code
static DWORD  mask;  // current mask to AND input bits with (== size**2 - 1)
static int    bufSize;  // number of good bits still in buf
static WORD*  table;
static WORD*  lastTableEntry;
static WORD*  newCode;  // points into table where the next bump
                        //   in code size will occur.

static
BOOL
initLZW() {
  table = calloc(0x3D02/2, sizeof(WORD)); // WORD[0x3D02/2];
  return (int)table;
}

static
void
exitLZW() {
  free(table); table = 0;
}

static void
put(int code) {
  if (rep90) {
    rep90 = false;
    *dst++ = (BYTE)code;
  } else if (dst[-1] != 0x90)
    *dst++ = (BYTE)code;
  else if (code == 0)
    rep90 = true;
  else {
    BYTE* d = dst-2;
    BYTE rep = *d++;
    while (--code)
      *d++ = rep;
    dst = d;
  }
}

static void
putCode(int code) {
  if (code <= 0x100)
    put(code);
  else {
    WORD* entryp = (WORD*)((BYTE*)table+code);
    putCode(*entryp++);
    code = *entryp;
    while (code > 0x100)
      code = *(WORD*)((BYTE*)table+code);
    put(code);
  }
}

static int reset();

static int
get() {
  int code;
  if (bufSize < size) {
    DWORD d = *src++;
    code = d << bufSize | buf;
    buf = d >> size - bufSize;
    bufSize += 32 - size;
  } else {
    code = buf;
    buf >>= size;
    bufSize -= size;
  }
  if ((code &= mask) > 0x100)
    code = code * 4 - 0x300;
  else if (code == 0x100) {
    if (lastTableEntry != table + (0x3D00/2)) {
      putCode(*lastTableEntry);
      if (dst == edst) {
        eof = true;
        return 0;
      }
    }
    code = reset();
  }
  return code;
}

static int
reset() {
  mask = 0x1FF;
  newCode = table + 0x500/2;
  buf = 0;
  size = 9; bufSize = 0;
  int code = get();
  *(lastTableEntry = table + 0x104/2) = (WORD)code;
  return code;
}

int
deLZW(void* _src, void *_dst, int dstSize) {
  BOOL preallocated = table != 0;
  if (!preallocated && !initLZW()) return -1;
  edst = (dst = (BYTE*)_dst) + dstSize;
  src = (DWORD*)_src;
  rep90 = true;
  eof = false;
  reset();
  do {
    int code = get();
    if (eof) break;
    if (lastTableEntry == table + (0x3D00/2)) {
      *lastTableEntry = (WORD)code;
      putCode(code);
    } else {
      putCode(*lastTableEntry++);
      *lastTableEntry++ = (WORD)code;
      *lastTableEntry   = (WORD)code;
      if (lastTableEntry == newCode)
        if (size == 12)
          putCode(*lastTableEntry);
        else {
          size++;
          mask = (mask << 1) + 1;
          newCode = (WORD*)((BYTE*)table + (1 << size + 2) - 0x300);
        }
    }
  } while (dst != edst);
  if (!preallocated) exitLZW();
    return (void*)dst - _dst;
}

