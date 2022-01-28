#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define ui8     u8
#define ui16    u16
#define ui32    u32
#define ui64    u64



// http://blog.gamedeff.com/?p=371
//extern "C" void __cdecl __CxxFrameHandler3( void ) {}

#include <stdio.h>
#include <stdlib.h>
//#include <vector>
//#include <memory.h>
#include <time.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

//#include "yappy.hpp"

// These 2 arrays are supposed to be filled by FillTables(). After that they stay constant.
static ui8 Yappy_maps[32][16];
static size_t Yappy_infos[256];

void /*inline*/ Yappy_Copy(const ui8 *data, ui8 *to) {
#ifdef __SSE2__
  _mm_storeu_si128((__m128i *)(to), _mm_loadu_si128((const __m128i *)(data)));
#else
    *(ui64*)to = *(const ui64*)data;
    *(((ui64*)to) + 1) = *((const ui64*)data + 1);
#endif
}

int YappyUnCompress(const ui8 *data, const ui8 *end, ui8 *to) {
  ui8 *start = to;
  while(data < end) {
      size_t index = data[0];
      if (index < 32) {
          Yappy_Copy(data + 1, to);
          if (index > 15) {
              Yappy_Copy(data + 17, to + 16);
          }
          to += index + 1;
          data += index + 2;
      } else {
          size_t info = Yappy_infos[index];
          size_t length = info & 0x00ff;
          size_t offset = (info & 0xff00) + (size_t)(data[1]);

          Yappy_Copy(to - offset, to);
          if (length > 16) {
              Yappy_Copy(to - offset + 16, to + 16);
          }
          to += length;
          data += 2;
      }
 }
 return to - start;
}

void YappyFillTables() {

  memset(&Yappy_maps[0][0], 0, sizeof(Yappy_maps));
  ui64 step = 1 << 16;
  size_t i, j;

  for (i = 0; i < 16; ++i) {
      ui64 value = 65535;
      step = (step * 67537) >> 16;
      while(value < (29UL << 16)) {
         Yappy_maps[value >> 16][i] = 1;
         value = (value * step) >> 16;
      }
  }

  int cntr = 0;
  for (i = 0; i < 29; ++i) {
      for (j = 0; j < 16; ++j) {
          if (Yappy_maps[i][j]) {
              Yappy_infos[32 + cntr] = i + 4 + (j << 8);
              Yappy_maps[i][j] = 32 + cntr;
              ++cntr;
          } else {
              if (i == 0)
                 exit(0); //throw("i == 0");
              Yappy_maps[i][j] = Yappy_maps[i - 1][j];
          }
      }
  }
  if (cntr != 256 - 32) {
     exit(0); //throw("init error");
  }

/*  for (uint32_t i = 32; i < 256; i++)
  {
	  uint32_t info = Yappy_infos[i];
      uint32_t length = info & 0x00ff;
      uint32_t offset = (info & 0xff00);
	  printf("i=%d len=%d off=%d\n", i, length, offset);
  }*/
}

int /*inline*/ Yappy_Match(const ui8 *data, size_t i, size_t j, size_t size) {
  if (*(ui32 *)(data + i) != *(ui32 *)(data + j))
      return 0;
  size_t k = 4;
  size_t bound = i - j;
  bound = bound > size ? size : bound;
  bound = bound > 32 ? 32 : bound;
  for (;k < bound && data[i + k] == data[j + k]; ++k);
  return k < bound ? k : bound;
}


ui64 /*inline*/ Yappy_Hash(ui64 value) {
  return ((value * 912367421UL) >> 24) & 4095;
}


void /*inline*/ Yappy_Link(size_t *hashes, size_t *nodes, size_t i, const ui8 *data) {
   int idx = Yappy_Hash(*(const u32 *)(data + i));
   size_t Yappy_HashValue = hashes[idx];
   nodes[i & 4095] = Yappy_HashValue;
   hashes[idx] = i;
}


int YappyCompress(const ui8 *data, ui8 *to, size_t len, int level) {

  size_t hashes[4096];
  size_t nodes[4096];
  ui8 end = 0xff;
  ui8 *optr = &end;
  ui8 *start = to;
  size_t i;

  for (i = 0; i < 4096; ++i) {
      hashes[i] = (size_t)(-1);
  }

  for (i = 0; i < len;) {
      ui8 coded = data[i];
      Yappy_Link(hashes, nodes, i, data);

      size_t bestMatch = 3;
      ui16 bestCode = 0;

      size_t ptr = i;
      int tries = 0;

      while(1) {
          size_t newPtr = nodes[ptr & 4095];
          if (newPtr >= ptr || i - newPtr >= 4095 || ((level >= 0) && (tries > level))) {
              break;
          }
          ptr = newPtr;
          size_t match = Yappy_Match(data, i, ptr, len - i);

          if (bestMatch < match) {
              ui8 code = Yappy_maps[match - 4][(i - ptr) >> 8];
              match = Yappy_infos[code] & 0xff;

              if (bestMatch < match) {
                  bestMatch = match;
                  bestCode = code + (((i - ptr) & 0xff) << 8);
                  if (bestMatch == 32)
                      break;
              }
          }

          tries += match > 3;
      }

      if (optr[0] > 30) {
          optr = &end;
      }

      if (bestMatch > 3) {
          *(ui16 *)to = bestCode;

          int k;
          for (k = 1; k < bestMatch; ++k)
             Yappy_Link(hashes, nodes, i + k, data);

          i += bestMatch;
          to += 2;
          optr = &end;
      } else {
          if (optr[0] == 0xff) {
             optr = to;
             optr[0] = 0xff;
             ++to;
          }
          ++optr[0];
          to[0] = coded;
          ++to;
          ++i;
      }
  }
  return to - start;
}



int yappy_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    YappyFillTables();
    return YappyUnCompress(in, in + insz, out);
}



int yappy_compress(unsigned char *in, int insz, unsigned char *out, int outsz, int level) {
    YappyFillTables();
    return YappyCompress(in, out, insz, level);
}



#undef ui8
#undef ui16
#undef ui32
#undef ui64

