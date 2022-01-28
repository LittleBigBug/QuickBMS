// modified by Luigi Auriemma
/*----------------------------------------------------------------------------*/
/*--  blz.c - Bottom LZ coding for Nintendo GBA/DS                          --*/
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
#define CMD_ENCODE    0x01       // encode

#define BLZ_NORMAL    0          // normal mode
#define BLZ_BEST      1          // best mode

#define BLZ_SHIFT     1          // bits to shift
#define BLZ_MASK      0x80       // bits to check:
                                 // ((((1 << BLZ_SHIFT) - 1) << (8 - BLZ_SHIFT)

#define BLZ_THRESHOLD 2          // max number of bytes to not encode
#define BLZ_N         0x1002     // max offset ((1 << 12) + 2)
#define BLZ_F         0x12       // max coded ((1 << 4) + BLZ_THRESHOLD)

#define RAW_MINIM     0x00000000 // empty file, 0 bytes
#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

#define BLZ_MINIM     0x00000004 // header only (empty RAW file)
#define BLZ_MAXIM     0x01400000 // 0x0120000A, padded to 20MB:
                                 // * length, RAW_MAXIM
                                 // * flags, (RAW_MAXIM + 7) / 8
                                 // * header, 11
                                 // 0x00FFFFFF + 0x00200000 + 12 + padding

/*----------------------------------------------------------------------------*/
unsigned int arm9;

/*----------------------------------------------------------------------------*/
#define BREAK(text)   { printf(text); return; }
#define EXIT(text)    { printf(text); exit(-1); }

/*----------------------------------------------------------------------------*/

void  BLZ_Decode(char *filename);
void  BLZ_Encode(char *filename, int mode);
char *BLZ_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int best);
void  BLZ_Invert(char *buffer, int length);
short BLZ_CRC16(unsigned char *buffer, unsigned int length);

#include "mem2mem.c"

/*----------------------------------------------------------------------------*/
void BLZ_Decode(char *filename) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, len, pos, inc_len, hdr_len, enc_len, dec_len;
  unsigned char  flags, mask;

  //printf("- decoding '%s'", filename);

  pak_buffer = Load(filename, &pak_len, BLZ_MINIM, BLZ_MAXIM);

  inc_len = *(unsigned int *)(pak_buffer + pak_len - 4);
  if (!inc_len) {
    printf(", WARNING: not coded file!");
    enc_len = 0;
    dec_len = pak_len;
    pak_len = 0;
    raw_len = dec_len;
  } else {
    if (pak_len < 8) EXIT("\nFile has a bad header\n");
    hdr_len = pak_buffer[pak_len - 5];
    if ((hdr_len < 0x08) || (hdr_len > 0x0B)) EXIT("\nBad header length\n");
    if (pak_len <= hdr_len) EXIT("\nBad length\n");
    enc_len = *(unsigned int *)(pak_buffer + pak_len - 8) & 0x00FFFFFF;
    dec_len = pak_len - enc_len;
    pak_len = enc_len - hdr_len;
    raw_len = dec_len + enc_len + inc_len;
    if (raw_len > RAW_MAXIM) EXIT("\nBad decoded length\n");
  }

  raw_buffer = (unsigned char *) Memory(raw_len, sizeof(char));

  pak = pak_buffer;
  raw = raw_buffer;
  pak_end = pak_buffer + dec_len + pak_len;
  raw_end = raw_buffer + raw_len;

  for (len = 0; len < dec_len; len++) *raw++ = *pak++;

  BLZ_Invert(pak_buffer + dec_len, pak_len);

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= BLZ_SHIFT)) {
      if (pak == pak_end) break;
      flags = *pak++;
      mask = BLZ_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (pak + 1 >= pak_end) break;
      pos = *pak++ << 8;
      pos |= *pak++;
      len = (pos >> 12) + BLZ_THRESHOLD + 1;
      if (raw + len > raw_end) {
        printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }
      pos = (pos & 0xFFF) + 3;
      while (len--) { *raw = *(raw - pos); raw++; }
    }
  }

  BLZ_Invert(raw_buffer + dec_len, raw_len - dec_len);

  raw_len = raw - raw_buffer;

  if (raw != raw_end) printf(", WARNING: unexpected end of encoded file!");

  Save(filename, raw_buffer, raw_len);

  free(raw_buffer);
  free(pak_buffer);

  //printf("\n");
}

/*----------------------------------------------------------------------------*/
void BLZ_Encode(char *filename, int mode) {
  unsigned char *raw_buffer, *pak_buffer, *new_buffer;
  unsigned int   raw_len, pak_len, new_len;

  //printf("- encoding '%s'", filename);

  raw_buffer = Load(filename, &raw_len, RAW_MINIM, RAW_MAXIM);

  pak_buffer = NULL;
  pak_len = BLZ_MAXIM + 1;

  new_buffer = BLZ_Code(raw_buffer, raw_len, &new_len, mode);
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
char *BLZ_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int best) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg, *tmp;
  unsigned int   pak_len, inc_len, hdr_len, enc_len, len, pos, max;
  unsigned int   len_best, pos_best, len_next, pos_next, len_post, pos_post;
  unsigned int   pak_tmp, raw_tmp, raw_new;
  unsigned short crc;
  unsigned char  mask;

#define SEARCH(l,p) { \
  l = BLZ_THRESHOLD;                                          \
                                                              \
  max = raw - raw_buffer >= BLZ_N ? BLZ_N : raw - raw_buffer; \
  for (pos = 3; pos <= max; pos++) {                          \
    for (len = 0; len < BLZ_F; len++) {                       \
      if (raw + len == raw_end) break;                        \
      if (len >= pos) break;                                  \
      if (*(raw + len) != *(raw + len - pos)) break;          \
    }                                                         \
                                                              \
    if (len > l) {                                            \
      p = pos;                                                \
      if ((l = len) == BLZ_F) break;                          \
    }                                                         \
  }                                                           \
}

  pak_tmp = 0;
  raw_tmp = raw_len;

  pak_len = raw_len + ((raw_len + 7) / 8) + 11;
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  raw_new = raw_len;
  if (arm9) {
    if (raw_len < 0x4000) {
      printf(", WARNING: ARM9 must be greater as 16KB, switch [9] disabled");
    } else if (
      (*(unsigned int   *)(raw_buffer + 0x0) != 0xE7FFDEFF) ||
      (*(unsigned int   *)(raw_buffer + 0x4) != 0xE7FFDEFF) ||
      (*(unsigned int   *)(raw_buffer + 0x8) != 0xE7FFDEFF) ||
      (*(unsigned short *)(raw_buffer + 0xC) != 0xDEFF)
    ) {
      printf(", WARNING: invalid Secure Area ID, switch [9] disabled");
    } else if (*(short *)(raw_buffer + 0x7FE)) {
      printf(", WARNING: invalid Secure Area 2KB end, switch [9] disabled");
    } else {
      crc = (unsigned short)BLZ_CRC16(raw_buffer + 0x10, 0x07F0);
      if (*(unsigned short *)(raw_buffer + 0x0E) != crc) {
        printf(", WARNING: CRC16 Secure Area 2KB do not match");
        *(unsigned short *)(raw_buffer + 0x0E) = crc;
      }
      raw_new -= 0x4000;
    }
  }

  BLZ_Invert(raw_buffer, raw_len);

  pak = pak_buffer;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_new;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= BLZ_SHIFT)) {
      *(flg = pak++) = 0;
      mask = BLZ_MASK;
    }

    SEARCH(len_best, pos_best);

    // LZ-CUE optimization start
    if (best) {
      if (len_best > BLZ_THRESHOLD) {
        if (raw + len_best < raw_end) {
          raw += len_best;
          SEARCH(len_next, pos_next);
          raw -= len_best - 1;
          SEARCH(len_post, pos_post);
          raw--;

          if (len_next <= BLZ_THRESHOLD) len_next = 1;
          if (len_post <= BLZ_THRESHOLD) len_post = 1;

          if (len_best + len_next <= 1 + len_post) len_best = 1;
        }
      }
    }
    // LZ-CUE optimization end

    *flg <<= 1;
    if (len_best > BLZ_THRESHOLD) {
      raw += len_best;
      *flg |= 1;
      *pak++ = ((len_best - (BLZ_THRESHOLD+1)) << 4) | ((pos_best - 3) >> 8);
      *pak++ = (pos_best - 3) & 0xFF;
    } else {
      *pak++ = *raw++;
    }

#if 1
    if (pak - pak_buffer + raw_len - (raw - raw_buffer) < pak_tmp + raw_tmp) {
#else
    if (
      (((pak - pak_buffer + raw_len - (raw - raw_buffer)) + 3) & -4)
      <
      pak_tmp + raw_tmp
    ) {
#endif
      pak_tmp = pak - pak_buffer;
      raw_tmp = raw_len - (raw - raw_buffer);
    }
  }

  while (mask && (mask != 1)) {
    mask >>= BLZ_SHIFT;
    *flg <<= 1;
  }

  pak_len = pak - pak_buffer;

  BLZ_Invert(raw_buffer, raw_len);
  BLZ_Invert(pak_buffer, pak_len);

  if (!pak_tmp || (raw_len + 4 < ((pak_tmp + raw_tmp + 3) & -4) + 8)) {
    pak = pak_buffer;
    raw = raw_buffer;
    raw_end = raw_buffer + raw_len;

    while (raw < raw_end) *pak++ = *raw++;

    while ((pak - pak_buffer) & 3) *pak++ = 0;

    *(unsigned int *)pak = 0; pak += 4;
  } else {
    tmp = (unsigned char *) Memory(raw_tmp + pak_tmp + 11, sizeof(char));

    for (len = 0; len < raw_tmp; len++)
      tmp[len] = raw_buffer[len];

    for (len = 0; len < pak_tmp; len++)
      tmp[raw_tmp + len] = pak_buffer[len + pak_len - pak_tmp];

    pak = pak_buffer;
    pak_buffer = tmp;

    free(pak);

    pak = pak_buffer + raw_tmp + pak_tmp;

    enc_len = pak_tmp;
    hdr_len = 8;
    inc_len = raw_len - pak_tmp - raw_tmp;

    while ((pak - pak_buffer) & 3) {
      *pak++ = 0xFF;
      hdr_len++;
    }

    *(unsigned int *)pak = enc_len + hdr_len; pak += 3;
    *pak++ = hdr_len;
    *(unsigned int *)pak = inc_len - hdr_len; pak += 4;
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
void BLZ_Invert(char *buffer, int length) {
  char *bottom, ch;

  bottom = buffer + length - 1;

  while (buffer < bottom) {
    ch = *buffer;
    *buffer++ = *bottom;
    *bottom-- = ch;
  }
}

/*----------------------------------------------------------------------------*/
short BLZ_CRC16(unsigned char *buffer, unsigned int length) {
  unsigned short crc;
  unsigned int   nbits;

  crc = 0xFFFF;
  while (length--) {
    crc ^= *buffer++;
    nbits = 8;
    while (nbits--) {
      if (crc & 1) { crc = (crc >> 1) ^ 0xA001; }
      else           crc =  crc >> 1;
    }
  }

  return(crc);
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
