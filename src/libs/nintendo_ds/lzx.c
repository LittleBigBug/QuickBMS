// modified by Luigi Auriemma
/*----------------------------------------------------------------------------*/
/*--  lzx.c - LZ eXtended coding for Nintendo GBA/DS                        --*/
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
#define CMD_CODE_11   0x11       // LZX big endian magic number
#define CMD_CODE_40   0x40       // LZX low endian magic number

#define LZX_WRAM      0x00       // VRAM file not compatible (0)
#define LZX_VRAM      0x01       // VRAM file compatible (1)

#define LZX_SHIFT     1          // bits to shift
#define LZX_MASK      0x80       // first bit to check
                                 // ((((1 << LZX_SHIFT) - 1) << (8 - LZX_SHIFT)

#define LZX_THRESHOLD 2          // max number of bytes to not encode
#define LZX_N         0x1000     // max offset (1 << 12)
#define LZX_F         0x10       // max coded (1 << 4)
#define LZX_F1        0x110      // max coded ((1 << 4) + (1 << 8))
#define LZX_F2        0x10110    // max coded ((1 << 4) + (1 << 8) + (1 << 16))

#define RAW_MINIM     0x00000000 // empty file, 0 bytes
#define RAW_MAXIM     0x00FFFFFF // 3-bytes length, 16MB - 1

#define LZX_MINIM     0x00000004 // header only (empty RAW file)
#define LZX_MAXIM     0x01400000 // 0x01200006, padded to 20MB:
                                 // * header, 4
                                 // * length, RAW_MAXIM
                                 // * flags, (RAW_MAXIM + 7) / 8
                                 // * 3 (flag + 2 end-bytes)
                                 // 4 + 0x00FFFFFF + 0x00200000 + 3 + padding

/*----------------------------------------------------------------------------*/
int lzx_vram;

/*----------------------------------------------------------------------------*/
#define BREAK(text) { printf(text); return; }
#define EXIT(text)  { printf(text); exit(-1); }

/*----------------------------------------------------------------------------*/

void  LZX_Decode(char *filename);
void  LZX_Encode(char *filename, int cmd, int vram);
char *LZX_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int cmd);

#include "mem2mem.c"

/*----------------------------------------------------------------------------*/
void LZX_Decode(char *filename) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, header, len, pos, threshold, tmp;
  unsigned char  flags, mask;

  //printf("- decoding '%s'", filename);

  pak_buffer = Load(filename, &pak_len, LZX_MINIM, LZX_MAXIM);

  header = *pak_buffer;
  if ((header != CMD_CODE_11) && ((header != CMD_CODE_40))){
    free(pak_buffer);
    BREAK(", WARNING: file is not LZX encoded!\n");
  }

  raw_len = *(unsigned int *)pak_buffer >> 8;
  raw_buffer = (unsigned char *) Memory(raw_len, sizeof(char));

  pak = pak_buffer + 4;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZX_SHIFT)) {
      if (pak == pak_end) break;
      flags = *pak++;
      if (header == CMD_CODE_40) flags = -flags;
      mask = LZX_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (header == CMD_CODE_11) {
        if (pak + 1 >= pak_end) break;
        pos = *pak++;
        pos = (pos << 8) | *pak++;

        tmp = pos >> 12;
        if (tmp < LZX_THRESHOLD) {
          pos &= 0xFFF;
          if (pak == pak_end) break;
          pos = (pos << 8) | *pak++;
          threshold = LZX_F;
          if (tmp) {
            if (pak == pak_end) break;
            pos = (pos << 8) | *pak++;
            threshold = LZX_F1;
          }
        } else {
          threshold = 0;
        }

        len = (pos >> 12) + threshold + 1;
        pos = (pos & 0xFFF) + 1;
      } else {
        if (pak + 1 == pak_end) break;
        pos = *pak++;
        pos |= *pak++ << 8;

        tmp = pos & 0xF;
        if (tmp < LZX_THRESHOLD) {
          if (pak == pak_end) break;
          len = *pak++;
          threshold = LZX_F;
          if (tmp) {
            if (pak == pak_end) break;
            len = (*pak++ << 8) | len;
            threshold = LZX_F1;
          }
        } else {
          len = tmp;
          threshold = 0;
        }

        len += threshold;
        pos >>= 4;
      }

      if (raw + len > raw_end) {
        printf(", WARNING: wrong decoded length!");
        len = raw_end - raw;
      }

      while (len--) { *raw = *(raw - pos); raw++; }
    }
  }

  if (header == CMD_CODE_40) pak += *pak == 0x80 ? 3 : 2;

  raw_len = raw - raw_buffer;

  if (raw != raw_end) printf(", WARNING: unexpected end of encoded file!");

  Save(filename, raw_buffer, raw_len);

  free(raw_buffer);
  free(pak_buffer);

  //printf("\n");
}

/*----------------------------------------------------------------------------*/
void LZX_Encode(char *filename, int cmd, int vram) {
  unsigned char *raw_buffer, *pak_buffer, *new_buffer;
  unsigned int   raw_len, pak_len, new_len;

  lzx_vram = vram;

  //printf("- encoding '%s'", filename);

  raw_buffer = Load(filename, &raw_len, RAW_MINIM, RAW_MAXIM);

  pak_buffer = NULL;
  pak_len = LZX_MAXIM + 1;

  new_buffer = LZX_Code(raw_buffer, raw_len, &new_len, cmd);
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
char *LZX_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int cmd) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg;
  unsigned int   pak_len, max, len, pos, len_best, pos_best;
  unsigned int   len_next, pos_next, len_post, pos_post;
  unsigned char  mask;

#define SEARCH(l,p) {                                                          \
                    l = LZX_THRESHOLD - 1;                                     \
                                                                               \
                    max = raw - raw_buffer >= LZX_N ? LZX_N-1 : raw-raw_buffer;\
                    for (pos = lzx_vram + 1; pos <= max; pos++) {              \
                      for (len = 0; len < LZX_F2 - 1; len++) {                 \
                        if (raw + len == raw_end) break;                       \
                        if (*(raw + len) != *(raw + len - pos)) break;         \
                      }                                                        \
                                                                               \
                      if (len > l) {                                           \
                        p = pos;                                               \
                        if ((l = len) == LZX_F2 - 1) break;                    \
                      }                                                        \
                    }                                                          \
                    }

  pak_len = 4 + raw_len + ((raw_len + 7) / 8) + 3;
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  *(unsigned int *)pak_buffer = cmd | (raw_len << 8);

  pak = pak_buffer + 4;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  mask = 0;

//------------------------------------------------------------------------------
//LZ11: - if x>1: xA BC <-------- copy ('x'   +  0x1) bytes from -('ABC'+1)
//      - if x=0: 0a bA BC <----- copy ('ab'  + 0x11) bytes from -('ABC'+1)
//      - if x=1: 1a bc dA BC <-- copy ('abcd'+0x111) bytes from -('ABC'+1)
//------------------------------------------------------------------------------
  if (cmd == CMD_CODE_11) {
    while (raw < raw_end) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      len_best = LZX_THRESHOLD;

      pos = raw - raw_buffer >= LZX_N ? LZX_N : raw - raw_buffer;
      for ( ; pos > lzx_vram; pos--) {
        for (len = 0; len < LZX_F2; len++) {
          if (raw + len == raw_end) break;
          if (*(raw + len) != *(raw + len - pos)) break;
        }

        if (len > len_best) {
          pos_best = pos;
          if ((len_best = len) == LZX_F2) break;
        }
      }

      if (len_best > LZX_THRESHOLD) {
        raw += len_best;
        *flg |= mask;
        if (len_best > LZX_F1) {
          len_best -= LZX_F1 + 1;
          *pak++ = 0x10 | (len_best >> 12);
          *pak++ = (len_best >> 4) & 0xFF;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        } else if (len_best > LZX_F) {
          len_best -= LZX_F + 1;
          *pak++ = len_best >> 4;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        } else{
          len_best--;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        }
      } else {
        *pak++ = *raw++;
      }
    }
//------------------------------------------------------------------------------
//LZ40: - if x>1: Cx AB <-------- copy ('x'   +  0x0) bytes from -('ABC'+0)
//      - if x=0: C0 AB ab <----- copy ('ab'  + 0x10) bytes from -('ABC'+0)
//      - if x=1: C1 AB cd ab <-- copy ('abcd'+0x110) bytes from -('ABC'+0)
//------------------------------------------------------------------------------
  } else {
    while (raw < raw_end) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      SEARCH(len_best, pos_best);

      if (len_best >= LZX_THRESHOLD) {
        raw += len_best;
        SEARCH(len_next, pos_next);
        raw -= len_best - 1;
        SEARCH(len_post, pos_post);
        raw--;

        if (len_best + len_next <= 1 + len_post) len_best = 1;
      }

      if (len_best >= LZX_THRESHOLD) {
        raw += len_best;
        *flg = -(-*flg | mask);
        if (len_best > LZX_F1 - 1) {
          len_best -= LZX_F1;
          *pak++ = ((pos_best & 0xF) << 4) | 1;
          *pak++ = pos_best >> 4;
          *pak++ = len_best & 0xFF;
          *pak++ = len_best >> 8;
        } else if (len_best > LZX_F - 1) {
          len_best -= LZX_F;
          *pak++ = (pos_best & 0xF) << 4;
          *pak++ = pos_best >> 4;
          *pak++ = len_best;
        } else {
          *pak++ = ((pos_best & 0xF) << 4) | len_best;
          *pak++ = pos_best >> 4;
        }
      } else {
        *pak++ = *raw++;
      }
    }

    if (cmd == CMD_CODE_40) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      *flg = -(-*flg | mask);
      *pak++ = 0;
      *pak++ = 0;
    }
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
