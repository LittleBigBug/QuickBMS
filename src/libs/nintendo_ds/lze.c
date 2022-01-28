// modified by Luigi Auriemma
/*----------------------------------------------------------------------------*/
/*--  lze.c - LZ Enhanced coding for Nintendo GBA/DS                        --*/
/*--  Copyright (C) 2011 CUE                                                --*/
/*--                                                                        --*/
/*--  This program is free software: you can redistribute it and/or modify  --*/
/*--  it under the terms of the GNU General Public License as published by  --*/
/*--  the Free Software Foundation, either version 3 of the License, or     --*/
/*--  (at your option) any later version.                                   --*/
/*--                                                                        --*/
/*--  This program is distributed in the hope that it will be useful,       --*/
/*--  but WITHOUT ANY WARRANTY; without even the implied warranty of        --*/
/*--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          --*/
/*--  GNU General Public License for more details.                          --*/
/*--                                                                        --*/
/*--  You should have received a copy of the GNU General Public License     --*/
/*--  along with this program. If not, see <http://www.gnu.org/licenses/>.  --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
#define CMD_DECODE    0x00       // decode
#define CMD_CODE_LE   0x654C     // LZE magic number

#define LZE_SHIFT     2          // bits to shift
#define LZE_MASK      0x03       // first bits to check, ((1 << LZE_SHIFT) - 1)
#define LZE_LZS4C     0x0        // 00 binary, short LZ
#define LZE_LZS62     0x1        // 01 binary, normal LZ
#define LZE_COPY1     0x2        // 10 binary, short LZ
#define LZE_COPY3     0x3        // 11 binary, normal LZ

#define LZE_THRESHOLD 2          // max number of bytes to not encode
#define LZE_N1        0x4        // max offset (1 << 2)
#define LZE_F1        0x41       // max coded ((1 << 6) + LZE_THRESHOLD)
#define LZE_N         0x1004     // max offset ((1 << 12) + LZE_N1)
#define LZE_F         0x12       // max coded ((1 << 4) + LZE_THRESHOLD)

#define RAW_MINIM     0x00000000 // empty file, 0 bytes
#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

#define LZE_MINIM     0x00000004 // header only (empty RAW file)
#define LZE_MAXIM     0x01400000 // 0x01200003, padded to 20MB:
                                 // * header, 4
                                 // * length, RAW_MAXIM
                                 // * flags, (RAW_MAXIM + 7) / 8
                                 // 4 + 0x00FFFFFF + 0x00200000 + padding

/*----------------------------------------------------------------------------*/
#define BREAK(text) { printf(text); return; }
#define EXIT(text)  { printf(text); exit(-1); }

/*----------------------------------------------------------------------------*/

void  LZE_Decode(char *filename);
void  LZE_Encode(char *filename);
char *LZE_Code(unsigned char *raw_buffer, int raw_len, int *new_len);

#include "mem2mem.c"

/*----------------------------------------------------------------------------*/
void LZE_Decode(char *filename) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, header, len, pos;
  unsigned int   flags, mask;

  //printf("- decoding '%s'", filename);

  pak_buffer = Load(filename, &pak_len, LZE_MINIM, LZE_MAXIM);

  header = *(unsigned short *)pak_buffer;
  if (header != CMD_CODE_LE) {
    free(pak_buffer);
    BREAK(", WARNING: file is not LZE encoded!\n");
  }

  raw_len = *(unsigned int *)(pak_buffer + 2);
  raw_buffer = (unsigned char *) Memory(raw_len, sizeof(char));

  pak = pak_buffer + 6;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  flags = 0;

  while (raw < raw_end) {
    if ((flags >>= LZE_SHIFT) <= 0xFF) {
      if (pak == pak_end) break;
      flags = 0xFF00 | *pak++;
    }

    mask = flags & LZE_MASK;
    if (mask == LZE_LZS4C) {
      if (pak + 1 >= pak_end) break;
      pos = *pak++;
      pos |= *pak++ << 8;
      len = (pos >> 12) + LZE_THRESHOLD + 1;
      if (raw + len > raw_end) {
        printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }
      pos = (pos & 0xFFF) + LZE_N1 + 1;
      while (len--) { *raw = *(raw - pos); raw++; }
    } else if (mask == LZE_LZS62) {
      if (pak == pak_end) break;
      pos = *pak++;
      len = (pos >> 2) + LZE_THRESHOLD;
      if (raw + len > raw_end) {
        printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }
      pos = (pos & 0x3) + 1;
      while (len--) { *raw = *(raw - pos); raw++; }
    } else if (mask == LZE_COPY1) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (pak == pak_end) break;
      *raw++ = *pak++;
      if (raw == raw_end) break;
      if (pak == pak_end) break;
      *raw++ = *pak++;
      if (raw == raw_end) break;
      if (pak == pak_end) break;
      *raw++ = *pak++;
    }
  }

  raw_len = raw - raw_buffer;

  if (raw != raw_end) printf(", WARNING: unexpected end of encoded file!");

  Save(filename, raw_buffer, raw_len);

  free(raw_buffer);
  free(pak_buffer);

  //printf("\n");
}

/*----------------------------------------------------------------------------*/
void LZE_Encode(char *filename) {
  unsigned char *raw_buffer, *pak_buffer, *new_buffer;
  unsigned int   raw_len, pak_len, new_len;

  //printf("- encoding '%s'", filename);

  raw_buffer = Load(filename, &raw_len, RAW_MINIM, RAW_MAXIM);

  pak_buffer = NULL;
  pak_len = LZE_MAXIM + 1;

  new_buffer = LZE_Code(raw_buffer, raw_len, &new_len);
  if (new_len < pak_len) {
    if (pak_buffer != NULL) free(pak_buffer);
    pak_buffer = new_buffer;
    pak_len = new_len;
  }

  Save(filename, pak_buffer, pak_len);

  free(pak_buffer);
  free(raw_buffer);

  //printf("\n");
}

/*----------------------------------------------------------------------------*/
char *LZE_Code(unsigned char *raw_buffer, int raw_len, int *new_len) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg, store[LZE_N1-1];
  unsigned int   pak_len, len, pos, len_best, pos_best;
  unsigned int   mode, nbits, store_len;

  pak_len = 4 + raw_len + ((raw_len + 7) / 8);
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  *(unsigned short *)pak_buffer = CMD_CODE_LE;
  *(unsigned int *)(pak_buffer + 2) = raw_len;

  pak = pak_buffer + 6;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  nbits = 0;
  store_len = 0;

  while (raw < raw_end) {
    if (!nbits & !store_len) *(flg = pak++) = 0;

    mode = LZE_COPY1;
    len_best = LZE_THRESHOLD - 1;

    pos = raw - raw_buffer >= LZE_N1 ? LZE_N1 : raw - raw_buffer;
    for ( ; pos; pos--) {
      for (len = 0; len < LZE_F1; len++) {
        if (raw + len == raw_end) break;
        if (*(raw + len) != *(raw + len - pos)) break;
      }

      if (len > len_best) {
        mode = LZE_LZS62;
        pos_best = pos;
        if ((len_best = len) == LZE_F1) break;
      }
    }

    if (len_best < LZE_F) {
      pos = raw - raw_buffer >= LZE_N ? LZE_N : raw - raw_buffer;
      for ( ; pos > LZE_N1; pos--) {
        for (len = 0; len < LZE_F; len++) {
          if (raw + len == raw_end) break;
          if (*(raw + len) != *(raw + len - pos)) break;
        }

        if (len > len_best) {
          if (len > LZE_THRESHOLD) {
            mode = LZE_LZS4C;
            pos_best = pos - LZE_N1;
            if ((len_best = len) == LZE_F) break;
          }
        }
      }
    }

    if ((mode == LZE_LZS4C) || (mode == LZE_LZS62)) {
      if (store_len) {
        *pak++ = store[0];
        *flg |= LZE_COPY1 << nbits;
        nbits = (nbits + LZE_SHIFT) & 0x7;
        if (store_len == 2) {
          if (!nbits) *(flg = pak++) = 0;
          *pak++ = store[1];
          *flg |= LZE_COPY1 << nbits;
          nbits = (nbits + LZE_SHIFT) & 0x7;
        }
        if (!nbits) *(flg = pak++) = 0;
        store_len = 0;
      }

      if (mode == LZE_LZS4C) {
        *pak++ = (pos_best - 1) & 0xFF;
        *pak++ = ((len_best - LZE_THRESHOLD - 1) << 4) | ((pos_best - 1) >> 8);
      } else {
        *pak++ = ((len_best - LZE_THRESHOLD) << 2) | (pos_best - 1);
      }
      *flg |= mode << nbits;
      nbits = (nbits + LZE_SHIFT) & 0x7;

      raw += len_best;
    } else {
      store[store_len++] = *raw++;
      if (store_len == 3) {
        *pak++ = store[0];
        *pak++ = store[1];
        *pak++ = store[2];
        *flg |= LZE_COPY3 << nbits;
        nbits = (nbits + LZE_SHIFT) & 0x7;
        store_len = 0;
      }
    }
  }

  if (store_len) {
    *pak++ = store[0];
    *flg |= LZE_COPY1 << nbits;
    nbits = (nbits + LZE_SHIFT) & 0x7;
    if (store_len == 2) {
      if (!nbits) *(flg = pak++) = 0;
      *pak++ = store[1];
      *flg |= LZE_COPY1 << nbits;
      nbits = (nbits + LZE_SHIFT) & 0x7;
    }
  }

  pos = '0';
  while ((pak - pak_buffer) & 3) *pak++ = pos++;

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
