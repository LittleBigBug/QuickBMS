// modified by Luigi Auriemma 

// http://www.five-ten-sg.com/libpst/
/*
	 This program is free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation; either version 2 of the License, or
	 (at your option) any later version.

	 You should have received a copy of the GNU General Public License
	 along with this program; if not, write to the Free Software Foundation,
	 Inc., 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include "define.h"
//#include "lzfu.h"


#if BYTE_ORDER == BIG_ENDIAN
#  define LZFU_LE64_CPU(x) \
  x = ((((x) & UINT64_C(0xff00000000000000)) >> 56) | \
       (((x) & UINT64_C(0x00ff000000000000)) >> 40) | \
       (((x) & UINT64_C(0x0000ff0000000000)) >> 24) | \
       (((x) & UINT64_C(0x000000ff00000000)) >> 8 ) | \
       (((x) & UINT64_C(0x00000000ff000000)) << 8 ) | \
       (((x) & UINT64_C(0x0000000000ff0000)) << 24) | \
       (((x) & UINT64_C(0x000000000000ff00)) << 40) | \
       (((x) & UINT64_C(0x00000000000000ff)) << 56));
#  define LZFU_LE32_CPU(x) \
  x = ((((x) & 0xff000000) >> 24) | \
       (((x) & 0x00ff0000) >> 8 ) | \
       (((x) & 0x0000ff00) << 8 ) | \
       (((x) & 0x000000ff) << 24));
#  define LZFU_LE16_CPU(x) \
  x = ((((x) & 0xff00) >> 8) | \
       (((x) & 0x00ff) << 8));
#elif BYTE_ORDER == LITTLE_ENDIAN
#  define LZFU_LE64_CPU(x) {}
#  define LZFU_LE32_CPU(x) {}
#  define LZFU_LE16_CPU(x) {}
#else
#  error "Byte order not supported by this library"
#endif // BYTE_ORDER


#define LZFU_COMPRESSED 		0x75465a4c
#define LZFU_UNCOMPRESSED		0x414c454d

// initital dictionary
#define LZFU_INITDICT	"{\\rtf1\\ansi\\mac\\deff0\\deftab720{\\fonttbl;}" \
						"{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscrip" \
						"t \\fdecor MS Sans SerifSymbolArialTimes Ne" \
						"w RomanCourier{\\colortbl\\red0\\green0\\blue0" \
						"\r\n\\par \\pard\\plain\\f0\\fs20\\b\\i\\u\\tab" \
						"\\tx"
// initial length of dictionary
#define LZFU_INITLENGTH 207

// header for compressed rtf
typedef struct _lzfuheader {
	uint32_t cbSize;
	uint32_t cbRawSize;
	uint32_t dwMagic;
	uint32_t dwCRC;
} lzfuheader;


char* pst_lzfu_decompress(char* rtfcomp, uint32_t compsize, size_t *size, int header) {
	unsigned char dict[4096];       // the dictionary buffer
	unsigned int dict_length = 0;   // the dictionary pointer
	lzfuheader lzfuhdr;             // the header of the lzfu block
	unsigned char flags;            // 8 bits of flags (1=2byte block pointer into the dict, 0=1 byte literal)
	unsigned char flag_mask;        // look at one flag bit each time thru the loop
	uint32_t i;
	char    *out_buf;
	uint32_t out_ptr = 0;
	uint32_t out_size;
	uint32_t in_ptr;
	uint32_t in_size;

	memcpy(dict, LZFU_INITDICT, LZFU_INITLENGTH);
    memset(dict + LZFU_INITLENGTH, 0, sizeof(dict) - LZFU_INITLENGTH);
	dict_length = LZFU_INITLENGTH;

    if(!header) {
        memset(&lzfuhdr, 0, sizeof(lzfuhdr));
        lzfuhdr.cbSize      = compsize;
        lzfuhdr.cbRawSize   = *size;
        lzfuhdr.dwMagic     = LZFU_COMPRESSED;
        lzfuhdr.dwCRC       = 0;
        in_ptr              = 0;
        in_size             = compsize;
    } else {
        memcpy(&lzfuhdr, rtfcomp, sizeof(lzfuhdr));
        LZFU_LE32_CPU(lzfuhdr.cbSize);
        LZFU_LE32_CPU(lzfuhdr.cbRawSize);
        LZFU_LE32_CPU(lzfuhdr.dwMagic);
        LZFU_LE32_CPU(lzfuhdr.dwCRC);
        //printf("total size: %d\n", lzfuhdr.cbSize+4);
        //printf("raw size  : %d\n", lzfuhdr.cbRawSize);
        //printf("compressed: %s\n", (lzfuhdr.dwMagic == LZFU_COMPRESSED ? "yes" : "no"));
        //printf("CRC       : %#x\n", lzfuhdr.dwCRC);
        //printf("\n");
        in_ptr	 = sizeof(lzfuhdr);
        // Make sure to correct lzfuhdr.cbSize with 4 bytes before comparing
        // to compsize
        in_size  = (lzfuhdr.cbSize + 4 < compsize) ? lzfuhdr.cbSize + 4 : compsize;
    }
        out_size = lzfuhdr.cbRawSize;
        out_buf  = (char*)malloc(out_size);

	while (in_ptr < in_size) {
		flags = (unsigned char)(rtfcomp[in_ptr++]);
		flag_mask = 1;
		while (flag_mask) {
			if (flag_mask & flags) {
				// two bytes available?
				if (in_ptr+1 < in_size) {
					// read 2 bytes from input
					uint16_t blkhdr, offset, length;
					memcpy(&blkhdr, rtfcomp+in_ptr, 2);
					LZFU_LE16_CPU(blkhdr);
					in_ptr += 2;
					/* swap the upper and lower bytes of blkhdr */
					blkhdr = (((blkhdr&0xFF00)>>8)+
							  ((blkhdr&0x00FF)<<8));
					/* the offset is the first 12 bits of the 16 bit value */
					offset = (blkhdr&0xFFF0)>>4;
					/* the length of the dict entry are the last 4 bits */
					length = (blkhdr&0x000F)+2;
					// add the value we are about to print to the dictionary
					for (i=0; i < length; i++) {
						unsigned char c1;
						c1 = dict[(offset+i)%4096];
						dict[dict_length] = c1;
						dict_length = (dict_length+1) % 4096;
						if (out_ptr < out_size) out_buf[out_ptr++] = (char)c1;
						// required for dictionary wrap around
						// otherwise 0 byte values are referenced incorrectly
						dict[dict_length] = 0;
					}
				}
			} else {
				// one byte available?
				if (in_ptr < in_size) {
					// uncompressed chunk (single byte)
					char c1 = rtfcomp[in_ptr++];
					dict[dict_length] = c1;
					dict_length = (dict_length+1)%4096;
					if (out_ptr < out_size) out_buf[out_ptr++] = (char)c1;
					// required for dictionary wrap around
					// otherwise 0 byte values are referenced incorrect
					dict[dict_length] = 0;
				}
			}
			flag_mask <<= 1;
		}
	}
    *size = out_ptr;
	return out_buf;
}
