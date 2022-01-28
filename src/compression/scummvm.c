// modified by Luigi Auriemma
// most are untested

/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MIN(a,b) ((a) <= (b) ? (a) : (b))

typedef uint8_t     byte;
typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;

typedef unsigned int    uint;

typedef int bool;
#define true    1
#define false   0

typedef struct {
    byte    data;
    int     next;
} Tokenlist;

		static inline uint16 READ_UINT16(const void *ptr) {
			const uint8 *b = (const uint8 *)ptr;
			return (b[1] << 8) | b[0];
		}
		static inline uint32 READ_UINT32(const void *ptr) {
			const uint8 *b = (const uint8 *)ptr;
			return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | (b[0]);
		}
		static inline void WRITE_UINT16(void *ptr, uint16 value) {
			uint8 *b = (uint8 *)ptr;
			b[0] = (uint8)(value >> 0);
			b[1] = (uint8)(value >> 8);
		}
		static inline void WRITE_UINT32(void *ptr, uint32 value) {
			uint8 *b = (uint8 *)ptr;
			b[0] = (uint8)(value >>  0);
			b[1] = (uint8)(value >>  8);
			b[2] = (uint8)(value >> 16);
			b[3] = (uint8)(value >> 24);
		}

	#define READ_LE_UINT16(a) READ_UINT16(a)
	#define READ_LE_UINT32(a) READ_UINT32(a)

	#define WRITE_LE_UINT16(a, v) WRITE_UINT16(a, v)
	#define WRITE_LE_UINT32(a, v) WRITE_UINT32(a, v)

	#define FROM_LE_32(a) ((uint32)(a))
	#define FROM_LE_16(a) ((uint16)(a))

	#define FROM_BE_32(a) SWAP_BYTES_32(a)
	#define FROM_BE_16(a) SWAP_BYTES_16(a)

	#define TO_LE_32(a) ((uint32)(a))
	#define TO_LE_16(a) ((uint16)(a))

	#define TO_BE_32(a) SWAP_BYTES_32(a)
	#define TO_BE_16(a) SWAP_BYTES_16(a)

	#define CONSTANT_LE_32(a) ((uint32)(a))
	#define CONSTANT_LE_16(a) ((uint16)(a))

	#define CONSTANT_BE_32(a) SWAP_CONSTANT_32(a)
	#define CONSTANT_BE_16(a) SWAP_CONSTANT_16(a)

static inline uint32 READ_LE_UINT24(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[2] << 16) | (b[1] << 8) | (b[0]);
}

static inline uint32 READ_BE_UINT24(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 16) | (b[1] << 8) | (b[2]);
}

		static inline uint16 READ_BE_UINT16(const void *ptr) {
			const uint8 *b = (const uint8 *)ptr;
			return (b[0] << 8) | b[1];
		}
		static inline uint32 READ_BE_UINT32(const void *ptr) {
			const uint8 *b = (const uint8 *)ptr;
			return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3]);
		}
		static inline void WRITE_BE_UINT16(void *ptr, uint16 value) {
			uint8 *b = (uint8 *)ptr;
			b[0] = (uint8)(value >> 8);
			b[1] = (uint8)(value >> 0);
		}
		static inline void WRITE_BE_UINT32(void *ptr, uint32 value) {
			uint8 *b = (uint8 *)ptr;
			b[0] = (uint8)(value >> 24);
			b[1] = (uint8)(value >> 16);
			b[2] = (uint8)(value >>  8);
			b[3] = (uint8)(value >>  0);
		}




/*****************/



// from ScummVM - Graphic Adventure Engine
// https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-0-0rc1/engines/sword1/screen.cpp $
int Screen__decompressTony(byte *src, int compSize, byte *dest, int destsz) {
	byte *endOfData = src + compSize;
    byte *d = dest,
       *dl = dest + destsz;
	while (src < endOfData) {
		byte numFlat = *src++;
		if (numFlat) {
            if((d + numFlat) > dl) return(-1);
			memset(d, *src, numFlat);
			src++;
			d += numFlat;
		}
		if (src < endOfData) {
			byte numNoFlat = *src++;
            if((d + numNoFlat) > dl) return(-1);
			memcpy(d, src, numNoFlat);
			src += numNoFlat;
			d += numNoFlat;
		}
	}
    return(d - dest);
}

int Screen__decompressRLE7(byte *src, int compSize, byte *dest, int destsz) {
	byte *compBufEnd = src + compSize;
    byte *d = dest,
       *dl = dest + destsz;
	while (src < compBufEnd) {
		byte code = *src++;
		if ((code > 127) || (code == 0)) {
            if(d >= dl) return(-1);
			*d++ = code;
		} else {
			code++;
            if((d + code) > dl) return(-1);
			memset(d, *src++, code);
			d += code;
		}
	}
    return(d - dest);
}

int Screen__decompressRLE0(byte *src, int compSize, byte *dest, int destsz) {
	byte *srcBufEnd = src + compSize;
    byte *d = dest,
       *dl = dest + destsz;
	while (src < srcBufEnd) {
		byte color = *src++;
		if (color) {
            if(d >= dl) return(-1);
			*d++ = color;
		} else {
			byte skip = *src++;
            if((d + skip) > dl) return(-1);
			memset(d, 0, skip);
			d += skip;
		}
	}
    return(d - dest);
}

int Screen__decompressHIF(uint8 *src, uint8 *dest) {
    byte    *bck = dest;
	for (;;) { //Main loop
		byte control_byte = *src++;
		uint32 byte_count = 0;
		while (byte_count < 8) {
			if (control_byte & 0x80) {
				uint16 info_word = READ_BE_UINT16(src); //Read the info word
				src += 2;
				if (info_word == 0xFFFF) goto quit; //Got 0xFFFF code, finished.

				int32 repeat_count = (info_word >> 12) + 2; //How many time data needs to be refetched
				while (repeat_count >= 0) {
					uint8 *old_data_src = dest - ((info_word & 0xFFF) + 1);
					*dest++ = *old_data_src;
					repeat_count--;
				}
			} else
				*dest++ = *src++;
			byte_count++;
			control_byte <<= 1; //Shifting left the control code one bit
		}
	}
    quit:
    return dest - bck;
}


/*****************/



uint32 decompressSPCN(byte *src, byte *dst, uint32 dstsize) {
	//debugC(1, kDebugTools, "decompressSPCN(src, dst, %d)", dstsize);

	byte *srcp = src;
	byte *dstp = dst, *dste = dst + dstsize;
	byte val;
	uint16 len, ofs;
	if (!(*srcp & 0x80)) srcp++;
	while (dstp < dste) {
		val = *(srcp++);
		if (val & 0x80) {
			if (val & 0x40) {
				if (val == 0xFE) {
					len = READ_LE_UINT16(srcp);
					while (len--)
						*(dstp++) = srcp[2];
					srcp += 3;
				} else {
					if (val == 0xFF) {
						len = READ_LE_UINT16(srcp);
						srcp += 2;
					} else {
						len = (val & 0x3F) + 3;
					}
					ofs = READ_LE_UINT16(srcp);
					srcp += 2;
					while (len--) {
						*dstp = *(byte *)(dstp - ofs);
						dstp++;
					}
				}
			} else {
				len = val & 0x3F;
				while (len--)
					*(dstp++) = *(srcp++);
			}
		} else {
			len = (val >> 4) + 3;
			ofs = ((val & 0x0F) << 8) | *(srcp++);
			while (len--) {
				*dstp = *(byte *)(dstp - ofs);
				dstp++;
			}
		}
	}
	return (dstp - dst);
}




// this stupid RNC doesn't work, I use the old algorithm


#define RNC1_SIGNATURE   0x524E4301 // "RNC\001"
#define RNC2_SIGNATURE   0x524E4302 // "RNC\002"

#define READ_LE_INT16(x) (int16) READ_LE_UINT16(x)
#define READ_LE_INT32(x) (int32) READ_LE_UINT32(x)

#define WRITE_LE_INT16(x, y)  WRITE_LE_UINT16(x, (int16)y)
#define WRITE_LE_INT32(x, y)  WRITE_LE_UINT32(x, (int32)y)

	static uint16 RncDecoder__rawTable[64];
	static uint16 RncDecoder__posTable[64];
	static uint16 RncDecoder__lenTable[64];
	static uint16 RncDecoder__crcTable[256];

	static uint16 RncDecoder__bitBuffl;
	static uint16 RncDecoder__bitBuffh;
	static uint8 RncDecoder__bitCount;

	static const uint8 *RncDecoder__srcPtr;
	static uint8 *RncDecoder__dstPtr;

	static int16 RncDecoder__inputByteLeft;

//#define RncDecoder_TABLE_SIZE  (16 * 8)
#define RncDecoder_MIN_LENGTH  2

//RncDecoder__~RncDecoder() { }

static void RncDecoder__initCrc() {
	//debugC(1, kDebugTools, "initCrc()");

	uint16 cnt = 0;
	uint16 tmp1 = 0;
	uint16 tmp2 = 0;

	for (tmp2 = 0; tmp2 < 0x100; tmp2++) {
		tmp1 = tmp2;
		for (cnt = 8; cnt > 0; cnt--) {
			if (tmp1 % 2) {
				tmp1 >>= 1;
				tmp1 ^= 0x0a001;
			} else
				tmp1 >>= 1;
		}
		RncDecoder__crcTable[tmp2] = tmp1;
	}
}

static void RncDecoder__RncDecoder() {
	RncDecoder__initCrc();

	RncDecoder__bitBuffl = 0;
	RncDecoder__bitBuffh = 0;
	RncDecoder__bitCount = 0;
	RncDecoder__srcPtr = NULL;
	RncDecoder__dstPtr = NULL;
	RncDecoder__inputByteLeft = 0;
}

//calculate 16 bit crc of a block of memory
static uint16 RncDecoder__crcBlock(const uint8 *block, uint32 size) {
	//debugC(1, kDebugTools, "crcBlock(block, %d)", size);

	uint16 crc = 0;
	uint8 *crcTable8 = (uint8 *)RncDecoder__crcTable; //make a uint8* to crc_table
	uint8 tmp;
	uint32 i;

	for (i = 0; i < size; i++) {
		tmp = *block++;
		crc ^= tmp;
		tmp = (uint8)((crc >> 8) & 0x00FF);
		crc &= 0x00FF;
		crc = *(uint16 *)&crcTable8[crc << 1];
		crc ^= tmp;
	}

	return crc;
}

static uint16 RncDecoder__inputBits(uint8 amount) {
	//debugC(5, kDebugTools, "inputBits(%d)", amount);

	uint16 newBitBuffh = RncDecoder__bitBuffh;
	uint16 newBitBuffl = RncDecoder__bitBuffl;
	int16 newBitCount = RncDecoder__bitCount;
	uint16 remBits, returnVal;

	returnVal = ((1 << amount) - 1) & newBitBuffl;
	newBitCount -= amount;

	if (newBitCount < 0) {
		newBitCount += amount;
		remBits = (newBitBuffh << (16 - newBitCount));
		newBitBuffh >>= newBitCount;
		newBitBuffl >>= newBitCount;
		newBitBuffl |= remBits;
		RncDecoder__srcPtr += 2;

		// added some more check here to prevent reading in the buffer
		// if there are no bytes anymore.
		RncDecoder__inputByteLeft -= 2;
		if (RncDecoder__inputByteLeft <= 0)
			newBitBuffh = 0;
		else if (RncDecoder__inputByteLeft == 1)
			newBitBuffh = *RncDecoder__srcPtr;
		else
			newBitBuffh = READ_LE_UINT16(RncDecoder__srcPtr);
		amount -= newBitCount;
		newBitCount = 16 - amount;
	}
	remBits = (newBitBuffh << (16 - amount));
	RncDecoder__bitBuffh = newBitBuffh >> amount;
	RncDecoder__bitBuffl = (newBitBuffl >> amount) | remBits;
	RncDecoder__bitCount = (uint8)newBitCount;

	return returnVal;
}

static void RncDecoder__makeHufftable(uint16 *table) {
	//debugC(1, kDebugTools, "makeHufftable(table)");

	uint16 bitLength, i, j;
	uint16 numCodes = RncDecoder__inputBits(5);

	if (!numCodes)
		return;

	uint8 huffLength[16];
	for (i = 0; i < numCodes; i++)
		huffLength[i] = (uint8)(RncDecoder__inputBits(4) & 0x00FF);

	uint16 huffCode = 0;

	for (bitLength = 1; bitLength < 17; bitLength++) {
		for (i = 0; i < numCodes; i++) {
			if (huffLength[i] == bitLength) {
				*table++ = (1 << bitLength) - 1;

				uint16 b = huffCode >> (16 - bitLength);
				uint16 a = 0;

				for (j = 0; j < bitLength; j++)
					a |= ((b >> j) & 1) << (bitLength - j - 1);
				*table++ = a;

				*(table + 0x1e) = (huffLength[i] << 8) | (i & 0x00FF);
				huffCode += 1 << (16 - bitLength);
			}
		}
	}
}

static uint16 RncDecoder__inputValue(uint16 *table) {
	//debugC(5, kDebugTools, "inputValue(table)");

	uint16 valOne, valTwo, value = RncDecoder__bitBuffl;

	do {
		valTwo = (*table++) & value;
		valOne = *table++;
	} while (valOne != valTwo);

	value = *(table + 0x1e);
	RncDecoder__inputBits((uint8)((value >> 8) & 0x00FF));
	value &= 0x00FF;

	if (value >= 2) {
		value--;
		valOne = RncDecoder__inputBits((uint8)value & 0x00FF);
		valOne |= (1 << value);
		value = valOne;
	}

	return value;
}

static int RncDecoder__getbit() {
	//debugC(6, kDebugTools, "getbits()");

	if (RncDecoder__bitCount == 0) {
		RncDecoder__bitBuffl = *RncDecoder__srcPtr++;
		RncDecoder__bitCount = 8;
	}
	byte temp = (RncDecoder__bitBuffl & 0x80) >> 7;
	RncDecoder__bitBuffl <<= 1;
	RncDecoder__bitCount--;
	return temp;
}

int32 RncDecoder__unpackM1(const void *input, uint32 inputSize, void *output, uint32 unpackLen) {
    RncDecoder__RncDecoder();
    int RncDecoder_HEADER_LEN = 18;
	//debugC(1, kDebugTools, "unpackM1(input, output)");

	uint8 *outputLow, *outputHigh;
	const uint8 *inputHigh, *inputptr = (const uint8 *)input;

	//uint32 unpackLen = 0;
	uint32 packLen = 0;
	uint16 counts = 0;
	uint16 crcUnpacked = 0;
	uint16 crcPacked = 0;


	RncDecoder__inputByteLeft = inputSize;
	RncDecoder__bitBuffl = 0;
	RncDecoder__bitBuffh = 0;
	RncDecoder__bitCount = 0;

    uint8 blocks;
	//Check for "RNC "
	if (READ_BE_UINT32(inputptr) != RNC1_SIGNATURE) {
        RncDecoder_HEADER_LEN = 0;
        packLen = inputSize;
        blocks = -1;
    } else {

	inputptr += 4;

	// read unpacked/packed file length
	unpackLen = READ_BE_UINT32(inputptr);
	inputptr += 4;
	packLen = READ_BE_UINT32(inputptr);
	inputptr += 4;

	blocks = *(inputptr + 5);

	//read CRC's
	crcUnpacked = READ_BE_UINT16(inputptr);
	inputptr += 2;
	crcPacked = READ_BE_UINT16(inputptr);
	inputptr += 2;
	inputptr = (inputptr + RncDecoder_HEADER_LEN - 16);

	if (RncDecoder__crcBlock(inputptr, packLen) != crcPacked)
		return -2;
    }

	inputptr = (((const uint8 *)input) + RncDecoder_HEADER_LEN);
	RncDecoder__srcPtr = inputptr;

	inputHigh = ((const uint8 *)input) + packLen + RncDecoder_HEADER_LEN;
	outputLow = (uint8 *)output;
	outputHigh = *(((const uint8 *)input) + 16) + unpackLen + outputLow;

	if (!((inputHigh <= outputLow) || (outputHigh <= inputHigh))) {
		RncDecoder__srcPtr = inputHigh;
		RncDecoder__dstPtr = outputHigh;
		memcpy((RncDecoder__dstPtr - packLen), (RncDecoder__srcPtr - packLen), packLen);
		RncDecoder__srcPtr = (RncDecoder__dstPtr - packLen);
	}

	RncDecoder__inputByteLeft -= RncDecoder_HEADER_LEN;

	RncDecoder__dstPtr = (uint8 *)output;
	RncDecoder__bitCount = 0;


	RncDecoder__bitBuffl = READ_LE_UINT16(RncDecoder__srcPtr);
	RncDecoder__inputBits(2);

	do {
		RncDecoder__makeHufftable(RncDecoder__rawTable);
		RncDecoder__makeHufftable(RncDecoder__posTable);
		RncDecoder__makeHufftable(RncDecoder__lenTable);

		counts = RncDecoder__inputBits(16);

		do {
			uint32 inputLength = RncDecoder__inputValue(RncDecoder__rawTable);
			uint32 inputOffset;

			if (inputLength) {
				memcpy(RncDecoder__dstPtr, RncDecoder__srcPtr, inputLength); //memcpy is allowed here
				RncDecoder__dstPtr += inputLength;
				RncDecoder__srcPtr += inputLength;
				RncDecoder__inputByteLeft -= inputLength;
				uint16 a;
				if (RncDecoder__inputByteLeft <= 0)
					a = 0;
				else if (RncDecoder__inputByteLeft == 1)
					a = *RncDecoder__srcPtr;
				else
					a = READ_LE_UINT16(RncDecoder__srcPtr);

				uint16 b;
				if (RncDecoder__inputByteLeft <= 2)
					b = 0;
				else if (RncDecoder__inputByteLeft == 3)
					b = *(RncDecoder__srcPtr + 2);
				else
					b = READ_LE_UINT16(RncDecoder__srcPtr + 2);

				RncDecoder__bitBuffl &= ((1 << RncDecoder__bitCount) - 1);
				RncDecoder__bitBuffl |= (a << RncDecoder__bitCount);
				RncDecoder__bitBuffh = (a >> (16 - RncDecoder__bitCount)) | (b << RncDecoder__bitCount);
			}

			if (counts > 1) {
				inputOffset = RncDecoder__inputValue(RncDecoder__posTable) + 1;
				inputLength = RncDecoder__inputValue(RncDecoder__lenTable) + RncDecoder_MIN_LENGTH;

				// Don't use memcpy here! because input and output overlap.
				uint8 *tmpPtr = (RncDecoder__dstPtr - inputOffset);
				while (inputLength--)
					*RncDecoder__dstPtr++ = *tmpPtr++;
			}
		} while (--counts);
	} while (--blocks);

    unpackLen = (void *)RncDecoder__dstPtr - (void *)output;

    if(RncDecoder_HEADER_LEN) {
	if (RncDecoder__crcBlock((uint8 *)output, unpackLen) != crcUnpacked)
		return -3;
    }

	// all is done..return the amount of unpacked bytes
	return unpackLen;
}

int32 RncDecoder__unpackM2(const void *input, uint32 packLen, void *output, uint32 unpackLen) {
    int RncDecoder_HEADER_LEN = 18;
    RncDecoder__RncDecoder();
	//debugC(1, kDebugTools, "unpackM2(input, output)");

	const uint8 *inputptr = (const uint8 *)input;

	//uint32 unpackLen = 0;
	//uint32 packLen = 0;
	uint16 crcUnpacked = 0;
	uint16 crcPacked = 0;

	RncDecoder__bitBuffl = 0;
	RncDecoder__bitCount = 0;

	//Check for "RNC "
	if (READ_BE_UINT32(inputptr) != RNC2_SIGNATURE) {
        RncDecoder_HEADER_LEN = 0;
    } else {
	inputptr += 4;

	// read unpacked/packed file length
	unpackLen = READ_BE_UINT32(inputptr);
	inputptr += 4;
	packLen = READ_BE_UINT32(inputptr);
	inputptr += 4;

	//read CRC's
	crcUnpacked = READ_BE_UINT16(inputptr);
	inputptr += 2;
	crcPacked = READ_BE_UINT16(inputptr);
	inputptr += 2;
	inputptr = (inputptr + RncDecoder_HEADER_LEN - 16);

	if (RncDecoder__crcBlock(inputptr, packLen) != crcPacked)
		return -2;
    }

	inputptr = (((const uint8 *)input) + RncDecoder_HEADER_LEN);
	RncDecoder__srcPtr = inputptr;
	RncDecoder__dstPtr = (uint8 *)output;

	uint16 ofs, len;
	byte ofs_hi, ofs_lo;

	len = 0;
	ofs_hi = 0;
	ofs_lo = 0;

	RncDecoder__getbit();
	RncDecoder__getbit();

	while (1) {

		bool loadVal = false;

		while (RncDecoder__getbit() == 0)
			*RncDecoder__dstPtr++ = *RncDecoder__srcPtr++;

		len = 2;
		ofs_hi = 0;
		if (RncDecoder__getbit() == 0) {
			len = (len << 1) | RncDecoder__getbit();
			if (RncDecoder__getbit() == 1) {
				len--;
				len = (len << 1) | RncDecoder__getbit();
				if (len == 9) {
					len = 4;
					while (len--)
						ofs_hi = (ofs_hi << 1) | RncDecoder__getbit();
					len = (ofs_hi + 3) * 4;
					while (len--)
						*RncDecoder__dstPtr++ = *RncDecoder__srcPtr++;
					continue;
				}
			}
			loadVal = true;
		} else {
			if (RncDecoder__getbit() == 1) {
				len++;
				if (RncDecoder__getbit() == 1) {
					len = *RncDecoder__srcPtr++;
					if (len == 0) {
						if (RncDecoder__getbit() == 1)
							continue;
						else
							break;
					}
					len += 8;
				}
				loadVal = true;
			} else {
				loadVal = false;
			}
		}

		if (loadVal) {
			if (RncDecoder__getbit() == 1) {
				ofs_hi = (ofs_hi << 1) | RncDecoder__getbit();
				if (RncDecoder__getbit() == 1) {
					ofs_hi = ((ofs_hi << 1) | RncDecoder__getbit()) | 4;
					if (RncDecoder__getbit() == 0)
						ofs_hi = (ofs_hi << 1) | RncDecoder__getbit();
				} else if (ofs_hi == 0) {
					ofs_hi = 2 | RncDecoder__getbit();
				}
			}
		}

		ofs_lo = *RncDecoder__srcPtr++;
		ofs = (ofs_hi << 8) | ofs_lo;
		while (len--) {
			*RncDecoder__dstPtr = *(byte *)(RncDecoder__dstPtr - ofs - 1);
			RncDecoder__dstPtr++;
		}

	}

    unpackLen = (void *)RncDecoder__dstPtr - (void *)output;

    if(RncDecoder_HEADER_LEN) {
	if (RncDecoder__crcBlock((uint8 *)output, unpackLen) != crcUnpacked)
		return -3;
    }

	// all is done..return the amount of unpacked bytes
	return unpackLen;
}



/*****************/



int CineUnpacker__unpack(const byte *src, uint srcLen, byte *dst, uint dstLen) {
#if defined(__GNUC__) && !defined(__clang__)
	uint32 _crc;      ///< Error-detecting code (This should be zero after successful unpacking)
	uint32 _chunk32b; ///< The current internal 32-bit chunk of source data
	byte *_dst;       ///< Pointer to the current position in the destination buffer
	const byte *_src; ///< Pointer to the current position in the source buffer

	// These are used for detecting errors (e.g. out of bounds issues) during unpacking
	bool _error;           ///< Did an error occur during unpacking?
	const byte *_srcBegin; ///< Source buffer's beginning
	const byte *_srcEnd;   ///< Source buffer's end
	byte *_dstBegin;       ///< Destination buffer's beginning
	byte *_dstEnd;         ///< Destination buffer's end

uint32 CineUnpacker__readSource() {
	if (_src < _srcBegin || _src + 4 > _srcEnd) {
		_error = true;
		return 0; // The source pointer is out of bounds, returning a default value
	}
	uint32 value = READ_BE_UINT32(_src);
	_src -= 4;
	return value;
}

uint CineUnpacker__rcr(bool inputCarry) {
	uint outputCarry = (_chunk32b & 1);
	_chunk32b >>= 1;
	if (inputCarry) {
		_chunk32b |= 0x80000000;
	}
	return outputCarry;
}

uint CineUnpacker__nextBit() {
	uint carry = CineUnpacker__rcr(false);
	// Normally if the chunk becomes zero then the carry is one as
	// the end of chunk marker is always the last to be shifted out.
	if (_chunk32b == 0) {
		_chunk32b = CineUnpacker__readSource();
		_crc ^= _chunk32b;
		carry = CineUnpacker__rcr(true); // Put the end of chunk marker in the most significant bit
	}
	return carry;
}

uint CineUnpacker__getBits(uint numBits) {
	uint c = 0;
	while (numBits--) {
		c <<= 1;
		c |= CineUnpacker__nextBit();
	}
	return c;
}

void CineUnpacker__unpackRawBytes(uint numBytes) {
	if (_dst >= _dstEnd || _dst - numBytes + 1 < _dstBegin) {
		_error = true;
		return; // Destination pointer is out of bounds for this operation
	}
	while (numBytes--) {
		*_dst = (byte)CineUnpacker__getBits(8);
		--_dst;
	}
}

void CineUnpacker__copyRelocatedBytes(uint offset, uint numBytes) {
	if (_dst + offset >= _dstEnd || _dst - numBytes + 1 < _dstBegin) {
		_error = true;
		return; // Destination pointer is out of bounds for this operation
	}
	while (numBytes--) {
		*_dst = *(_dst + offset);
		--_dst;
	}
}

	// Initialize variables used for detecting errors during unpacking
	_error    = false;
	_srcBegin = src;
	_srcEnd   = src + srcLen;
	_dstBegin = dst;
	_dstEnd   = dst + dstLen;

	// Handle already unpacked data here
	if (srcLen == dstLen) {
		// Source length is same as destination length so the source
		// data is already unpacked. Let's just copy it then.
		memcpy(dst, src, srcLen);
		return true;
	}

	// Initialize other variables
	_src = _srcBegin + srcLen - 4;
	uint32 unpackedLength = CineUnpacker__readSource(); // Unpacked length in bytes
	_dst = _dstBegin + unpackedLength - 1;
	_crc = CineUnpacker__readSource();
	_chunk32b = CineUnpacker__readSource();
	_crc ^= _chunk32b;

	while (_dst >= _dstBegin && !_error) {
		/*
		Bits  => Action:
		0 0   => unpackRawBytes(3 bits + 1)              i.e. unpackRawBytes(1..8)
		1 1 1 => unpackRawBytes(8 bits + 9)              i.e. unpackRawBytes(9..264)
		0 1   => copyRelocatedBytes(8 bits, 2)           i.e. copyRelocatedBytes(0..255, 2)
		1 0 0 => copyRelocatedBytes(9 bits, 3)           i.e. copyRelocatedBytes(0..511, 3)
		1 0 1 => copyRelocatedBytes(10 bits, 4)          i.e. copyRelocatedBytes(0..1023, 4)
		1 1 0 => copyRelocatedBytes(12 bits, 8 bits + 1) i.e. copyRelocatedBytes(0..4095, 1..256)
		*/
		if (!CineUnpacker__nextBit()) { // 0...
			if (!CineUnpacker__nextBit()) { // 0 0
				uint numBytes = CineUnpacker__getBits(3) + 1;
				CineUnpacker__unpackRawBytes(numBytes);
			} else { // 0 1
				uint numBytes = 2;
				uint offset   = CineUnpacker__getBits(8);
				CineUnpacker__copyRelocatedBytes(offset, numBytes);
			}
		} else { // 1...
			uint c = CineUnpacker__getBits(2);
			if (c == 3) { // 1 1 1
				uint numBytes = CineUnpacker__getBits(8) + 9;
				CineUnpacker__unpackRawBytes(numBytes);
			} else if (c < 2) { // 1 0 x
				uint numBytes = c + 3;
				uint offset   = CineUnpacker__getBits(c + 9);
				CineUnpacker__copyRelocatedBytes(offset, numBytes);
			} else { // 1 1 0
				uint numBytes = CineUnpacker__getBits(8) + 1;
				uint offset   = CineUnpacker__getBits(12);
				CineUnpacker__copyRelocatedBytes(offset, numBytes);
			}
		}
	}
	//return !_error && (_crc == 0);
    if(!_error) return unpackedLength;
    return -1;
#else
    return -1;
#endif
}



/*****************/



int delphineUnpack(byte *dst, int datasize, byte *src, int len) {
#if defined(__GNUC__) && !defined(__clang__)
typedef struct {
	int size, datasize;
	uint32 crc;
	uint32 chk;
	byte *dst;
	/*const*/ byte *src;
} UnpackCtx;

int delphine__rcr(UnpackCtx *uc, int CF) {
	int rCF = (uc->chk & 1);
	uc->chk >>= 1;
	if (CF) {
		uc->chk |= 0x80000000;
	}
	return rCF;
}

int delphine__nextChunk(UnpackCtx *uc) {
	int CF = delphine__rcr(uc, 0);
	if (uc->chk == 0) {
		uc->chk = READ_BE_UINT32(uc->src);
		uc->src -= 4;
		uc->crc ^= uc->chk;
		CF = delphine__rcr(uc, 1);
	}
	return CF;
}

uint16 delphine__getCode(UnpackCtx *uc, byte numChunks) {
	uint16 c = 0;
	while (numChunks--) {
		c <<= 1;
		if (delphine__nextChunk(uc)) {
			c |= 1;
		}
	}
	return c;
}

void delphine__unpackHelper1(UnpackCtx *uc, byte numChunks, byte addCount) {
	uint16 count = delphine__getCode(uc, numChunks) + addCount + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = (byte)delphine__getCode(uc, 8);
		--uc->dst;
	}
}

void delphine__unpackHelper2(UnpackCtx *uc, byte numChunks) {
	uint16 i = delphine__getCode(uc, numChunks);
	uint16 count = uc->size + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = *(uc->dst + i);
		--uc->dst;
	}
}

	UnpackCtx uc;
	uc.src = src + len - 4;
    uc.datasize = datasize;
    // yeah, leave the original code!
	uc.datasize = READ_BE_UINT32(uc.src);
	uc.src -= 4;
	uc.dst = dst + uc.datasize - 1;
	uc.size = 0;
	uc.crc = READ_BE_UINT32(uc.src);
	uc.src -= 4;
	uc.chk = READ_BE_UINT32(uc.src);
	uc.src -= 4;
	uc.crc ^= uc.chk;
	do {
		if (!delphine__nextChunk(&uc)) {
			uc.size = 1;
			if (!delphine__nextChunk(&uc)) {
				delphine__unpackHelper1(&uc, 3, 0);
			} else {
				delphine__unpackHelper2(&uc, 8);
			}
		} else {
			uint16 c = delphine__getCode(&uc, 2);
			if (c == 3) {
				delphine__unpackHelper1(&uc, 8, 8);
			} else if (c < 2) {
				uc.size = c + 2;
				delphine__unpackHelper2(&uc, c + 9);
			} else {
				uc.size = delphine__getCode(&uc, 8);
				delphine__unpackHelper2(&uc, 12);
			}
		}
	} while (uc.datasize > 0);
	//return uc.crc == 0;
    return datasize;
#else
    return -1;
#endif
}



/*****************/



// LZSS?
int DataIO__unpackChunk(byte *src, byte *dest, uint32 size) {
	byte tmpBuf[4114];
    byte *bck = dest;

	uint32 counter = size;

    int i;

	for (i = 0; i < 4078; i++)
		tmpBuf[i] = 0x20;
	uint16 tmpIndex = 4078;

	uint16 cmd = 0;
	while (1) {
		cmd >>= 1;
		if ((cmd & 0x0100) == 0)
			cmd = *src++ | 0xFF00;

		if ((cmd & 1) != 0) { /* copy */
			byte tmp = *src++;

			*dest++ = tmp;
			tmpBuf[tmpIndex] = tmp;

			tmpIndex++;
			tmpIndex %= 4096;
			counter--;
			if (counter == 0)
				break;
		} else { /* copy string */
			byte tmp1 = *src++;
			byte tmp2 = *src++;

			int16 off = tmp1 | ((tmp2 & 0xF0) << 4);
			byte  len =         (tmp2 & 0x0F) + 3;

			for (i = 0; i < len; i++) {
				*dest++ = tmpBuf[(off + i) % 4096];
				counter--;
				if (counter == 0) {
					goto quit;
				}
				tmpBuf[tmpIndex] = tmpBuf[(off + i) % 4096];
				tmpIndex++;
				tmpIndex %= 4096;
			}

		}
	}

    quit:
    return dest - bck;
}



/*****************/



#define ENABLE_SCI32
enum ResourceErrorCodes {
	SCI_ERROR_NONE = 0,
	SCI_ERROR_IO_ERROR = 1,
	SCI_ERROR_EMPTY_RESOURCE = 2,
	SCI_ERROR_RESMAP_INVALID_ENTRY = 3,	/**< Invalid resource.map entry */
	SCI_ERROR_RESMAP_NOT_FOUND = 4,
	SCI_ERROR_NO_RESOURCE_FILES_FOUND = 5,	/**< No resource at all was found */
	SCI_ERROR_UNKNOWN_COMPRESSION = 6,
	SCI_ERROR_DECOMPRESSION_ERROR = 7,	/**< sanity checks failed during decompression */
	SCI_ERROR_RESOURCE_TOO_BIG = 8	/**< Resource size exceeds SCI_MAX_RESOURCE_SIZE */
};

//typedef unsigned char   byte;

	static uint32 _dwBits;		///< bits buffer
	static byte _nBits;		///< number of unread bits in _dwBits
	static uint32 _szPacked;	///< size of the compressed data
	static uint32 _szUnpacked;	///< size of the decompressed data
	static uint32 _dwRead;		///< number of bytes read from _src
	static uint32 _dwWrote;	///< number of bytes written to _dest
	static byte *_src;
	static byte *_dest;

	static byte *_nodes;

	static uint16 _numbits;
	static uint16 _curtoken, _endtoken;
	static int _compression;

enum ResourceCompression {
	kCompUnknown = -1,
	kCompNone = 0,
	kCompLZW,
	kCompHuffman,
	kCompLZW1,			// LZW-like compression used in SCI01 and SCI1
	kCompLZW1View,		// Comp3 + view Post-processing
	kCompLZW1Pic,		// Comp3 + pic Post-processing
#ifdef ENABLE_SCI32
	kCompSTACpack,	// ? Used in SCI32
#endif
	kCompDCL
};



	static bool Decompressor__isFinished() {
		return (_dwWrote == _szUnpacked) && (_dwRead >= _szPacked);
	}

static void Decompressor__init(byte *src, byte *dest, uint32 nPacked,
                        uint32 nUnpacked) {
	_src = src;
	_dest = dest;
	_szPacked = nPacked;
	_szUnpacked = nUnpacked;
	_nBits = 0;
	_dwRead = _dwWrote = 0;
	_dwBits = 0;
}

static void Decompressor__fetchBitsMSB() {
	while (_nBits <= 24) {
		_dwBits |= ((uint32)*_src++) << (24 - _nBits);
		_nBits += 8;
		_dwRead++;
	}
}

static uint32 Decompressor__getBitsMSB(int n) {
	// fetching more data to buffer if needed
	if (_nBits < n)
		Decompressor__fetchBitsMSB();
	uint32 ret = _dwBits >> (32 - n);
	_dwBits <<= n;
	_nBits -= n;
	return ret;
}

static byte Decompressor__getByteMSB() {
	return Decompressor__getBitsMSB(8);
}

static void Decompressor__fetchBitsLSB() {
	while (_nBits <= 24) {
		_dwBits |= ((uint32)*_src++) << _nBits;
		_nBits += 8;
		_dwRead++;
	}
}

static uint32 Decompressor__getBitsLSB(int n) {
	// fetching more data to buffer if needed
	if (_nBits < n)
		Decompressor__fetchBitsLSB();
	uint32 ret = (_dwBits & ~((~0) << n));
	_dwBits >>= n;
	_nBits -= n;
	return ret;
}

static byte Decompressor__getByteLSB() {
	return Decompressor__getBitsLSB(8);
}

static void Decompressor__putByte(byte b) {
	_dest[_dwWrote++] = b;
}
//-------------------------------
//  Huffman decompressor
//-------------------------------

static int16 DecompressorHuffman__getc2() {
	byte *node = _nodes;
	int16 next;
	while (node[1]) {
		if (Decompressor__getBitsMSB(1)) {
			next = node[1] & 0x0F; // use lower 4 bits
			if (next == 0)
				return Decompressor__getByteMSB() | 0x100;
		} else
			next = node[1] >> 4; // use higher 4 bits
		node += next << 1;
	}
	return (int16)(*node | (node[1] << 8));
}
int DecompressorHuffman__unpack(byte *src, byte *dest, uint32 nPacked,
								uint32 nUnpacked) {
	Decompressor__init(src, dest, nPacked, nUnpacked);
	byte numnodes;
	int16 c;
	uint16 terminator;

	numnodes = *_src++;
	terminator = *_src++ | 0x100;
	_nodes = malloc(numnodes << 1);
    memcpy(_nodes, _src, numnodes << 1);
    _src += numnodes << 1;

	while ((c = DecompressorHuffman__getc2()) != terminator && (c >= 0) && !Decompressor__isFinished())
		Decompressor__putByte(c);

	free(_nodes);
	//return _dwWrote == _szUnpacked ? 0 : 1;
    return _dwWrote;
}

//-------------------------------
// LZW Decompressor for SCI0/01/1
//-------------------------------
static void DecompressorLZW__init(byte *src, byte *dest, uint32 nPacked, uint32 nUnpacked) {
	Decompressor__init(src, dest, nPacked, nUnpacked);

	_numbits = 9;
	_curtoken = 0x102;
	_endtoken = 0x1ff;
}

/*
int DecompressorLZW__unpack(byte *src, byte *dest, uint32 nPacked,
								uint32 nUnpacked) {
	byte *buffer = NULL;

	switch (_compression) {
	case kCompLZW:	// SCI0 LZW compression
		return DecompressorLZW__unpackLZW(src, dest, nPacked, nUnpacked);
		break;
	case kCompLZW1: // SCI01/1 LZW compression
		return DecompressorLZW__unpackLZW1(src, dest, nPacked, nUnpacked);
		break;
	case kCompLZW1View:
		buffer = malloc(nUnpacked);
		DecompressorLZW__unpackLZW1(src, buffer, nPacked, nUnpacked);
		reorderView(buffer, dest);
		break;
	case kCompLZW1Pic:
		buffer = malloc(nUnpacked);
		DecompressorLZW__unpackLZW1(src, buffer, nPacked, nUnpacked);
		reorderPic(buffer, dest, nUnpacked);
		break;
	}
	if(buffer) free(buffer);
	return 0;
}
*/

int DecompressorLZW__unpackLZW(byte *src, byte *dest, uint32 nPacked,
                                uint32 nUnpacked) {
	Decompressor__init(src, dest, nPacked, nUnpacked);

    int i;
	uint16 token; // The last received value
	uint16 tokenlastlength = 0;

	uint16 *tokenlist = (uint16 *)malloc(4096 * sizeof(uint16)); // pointers to dest[]
	uint16* tokenlengthlist = (uint16 *)malloc(4096 * sizeof(uint16)); // char length of each token
	if (!tokenlist || !tokenlengthlist) {
		free(tokenlist);
		free(tokenlengthlist);

        return -1;  //error("[DecompressorLZW__unpackLZW] Cannot allocate token memory buffers");
	}

	while (!Decompressor__isFinished()) {
		token = Decompressor__getBitsLSB(_numbits);

		if (token == 0x101) {
			free(tokenlist);
			free(tokenlengthlist);

			return 0; // terminator
		}

		if (token == 0x100) { // reset command
			_numbits = 9;
			_endtoken = 0x1FF;
			_curtoken = 0x0102;
		} else {
			if (token > 0xff) {
				if (token >= _curtoken) {
					printf("unpackLZW: Bad token %x", token);

					free(tokenlist);
					free(tokenlengthlist);

					return SCI_ERROR_DECOMPRESSION_ERROR;
				}
				tokenlastlength = tokenlengthlist[token] + 1;
				if (_dwWrote + tokenlastlength > _szUnpacked) {
					// For me this seems a normal situation, It's necessary to handle it
					printf("unpackLZW: Trying to write beyond the end of array(len=%d, destctr=%d, tok_len=%d)",
					        _szUnpacked, _dwWrote, tokenlastlength);
					for ( i = 0; _dwWrote < _szUnpacked; i++)
						Decompressor__putByte(dest[tokenlist[token] + i]);
				} else
					for ( i = 0; i < tokenlastlength; i++)
						Decompressor__putByte(dest[tokenlist[token] + i]);
			} else {
				tokenlastlength = 1;
				if (_dwWrote >= _szUnpacked)
					printf("unpackLZW: Try to write single byte beyond end of array");
				else
					Decompressor__putByte(token);
			}
			if (_curtoken > _endtoken && _numbits < 12) {
				_numbits++;
				_endtoken = (_endtoken << 1) + 1;
			}
			if (_curtoken <= _endtoken) {
				tokenlist[_curtoken] = _dwWrote - tokenlastlength;
				tokenlengthlist[_curtoken] = tokenlastlength;
				_curtoken++;
			}

		}
	}

	free(tokenlist);
	free(tokenlengthlist);

	//return _dwWrote == _szUnpacked ? 0 : SCI_ERROR_DECOMPRESSION_ERROR;
    return _dwWrote;
}

int DecompressorLZW__unpackLZW1(byte *src, byte *dest, uint32 nPacked,
                                uint32 nUnpacked) {
	Decompressor__init(src, dest, nPacked, nUnpacked);

	byte *stak = (byte *)malloc(0x1014);
	unsigned int tokensSize = 0x1004 * sizeof(Tokenlist);
	Tokenlist *tokens = (Tokenlist *)malloc(tokensSize);
	if (!stak || !tokens) {
		free(stak);
		free(tokens);

		return -1; //error("[DecompressorLZW__unpackLZW1] Cannot allocate decompression buffers");
	}

	memset(tokens, 0, tokensSize);

	byte lastchar = 0;
	uint16 stakptr = 0, lastbits = 0;

	byte decryptstart = 0;
	uint16 bitstring;
	uint16 token;
	bool bExit = false;

	while (!Decompressor__isFinished() && !bExit) {
		switch (decryptstart) {
		case 0:
			bitstring = Decompressor__getBitsMSB(_numbits);
			if (bitstring == 0x101) {// found end-of-data signal
				bExit = true;
				continue;
			}
			Decompressor__putByte(bitstring);
			lastbits = bitstring;
			lastchar = (bitstring & 0xff);
			decryptstart = 1;
			break;

		case 1:
			bitstring = Decompressor__getBitsMSB(_numbits);
			if (bitstring == 0x101) { // found end-of-data signal
				bExit = true;
				continue;
			}
			if (bitstring == 0x100) { // start-over signal
				_numbits = 9;
				_curtoken = 0x102;
				_endtoken = 0x1ff;
				decryptstart = 0;
				continue;
			}

			token = bitstring;
			if (token >= _curtoken) { // index past current point
				token = lastbits;
				stak[stakptr++] = lastchar;
			}
			while ((token > 0xff) && (token < 0x1004)) { // follow links back in data
				stak[stakptr++] = tokens[token].data;
				token = tokens[token].next;
			}
			lastchar = stak[stakptr++] = token & 0xff;
			// put stack in buffer
			while (stakptr > 0) {
				Decompressor__putByte(stak[--stakptr]);
				if (_dwWrote == _szUnpacked) {
					bExit = true;
					continue;
				}
			}
			// put token into record
			if (_curtoken <= _endtoken) {
				tokens[_curtoken].data = lastchar;
				tokens[_curtoken].next = lastbits;
				_curtoken++;
				if (_curtoken == _endtoken && _numbits < 12) {
					_numbits++;
					_endtoken = (_endtoken << 1) + 1;
				}
			}
			lastbits = bitstring;
			break;
		}
	}

	free(stak);
	free(tokens);

	//return _dwWrote == _szUnpacked ? 0 : SCI_ERROR_DECOMPRESSION_ERROR;
    return _dwWrote;
}

#define PAL_SIZE 1284
#define EXTRA_MAGIC_SIZE 15
#define VIEW_HEADER_COLORS_8BIT 0x80

static int DecompressorLZW__decodeRLE(byte **rledata, byte **pixeldata, byte *outbuffer, int size) {
	int pos = 0;
	byte nextbyte;
	byte *rd = *rledata;
	byte *ob = outbuffer;
	byte *pd = *pixeldata;

	while (pos < size) {
		nextbyte = *rd++;
		*ob++ = nextbyte;
		pos++;
		switch (nextbyte & 0xC0) {
		case 0x40:
		case 0x00:
			memcpy(ob, pd, nextbyte);
			pd += nextbyte;
			ob += nextbyte;
			pos += nextbyte;
			break;
		case 0xC0:
			break;
		case 0x80:
			nextbyte = *pd++;
			*ob++ = nextbyte;
			pos++;
			break;
		}
	}

	*rledata = rd;
	*pixeldata = pd;
    return ob - outbuffer;
}

/**
 * Does the same this as decodeRLE, only to determine the length of the
 * compressed source data.
 */
static int DecompressorLZW__getRLEsize(byte *rledata, int dsize) {
	int pos = 0;
	byte nextbyte;
	int size = 0;

	while (pos < dsize) {
		nextbyte = *(rledata++);
		pos++;
		size++;

		switch (nextbyte & 0xC0) {
		case 0x40:
		case 0x00:
			pos += nextbyte;
			break;
		case 0xC0:
			break;
		case 0x80:
			pos++;
			break;
		}
	}

	return size;
}

	enum {
		PIC_OPX_EMBEDDED_VIEW = 1,
		PIC_OPX_SET_PALETTE = 2,
		PIC_OP_OPX = 0xfe
	};

int DecompressorLZW__reorderPic(byte *src, byte *dest, int dsize) {
	uint16 view_size, view_start, cdata_size;
	int i;
	byte *seeker = src;
	byte *writer = dest;
	char viewdata[7];
	byte *cdata, *cdata_start;

	*writer++ = PIC_OP_OPX;
	*writer++ = PIC_OPX_SET_PALETTE;

	for (i = 0; i < 256; i++) /* Palette translation map */
		*writer++ = i;

	WRITE_LE_UINT32(writer, 0); /* Palette stamp */
	writer += 4;

	view_size = READ_LE_UINT16(seeker);
	seeker += 2;
	view_start = READ_LE_UINT16(seeker);
	seeker += 2;
	cdata_size = READ_LE_UINT16(seeker);
	seeker += 2;

	memcpy(viewdata, seeker, sizeof(viewdata));
	seeker += sizeof(viewdata);

	memcpy(writer, seeker, 4*256); /* Palette */
	seeker += 4*256;
	writer += 4*256;

	if (view_start != PAL_SIZE + 2) { /* +2 for the opcode */
		memcpy(writer, seeker, view_start-PAL_SIZE-2);
		seeker += view_start - PAL_SIZE - 2;
		writer += view_start - PAL_SIZE - 2;
	}

	if (dsize != view_start + EXTRA_MAGIC_SIZE + view_size) {
		memcpy(dest + view_size + view_start + EXTRA_MAGIC_SIZE, seeker,
		       dsize - view_size - view_start - EXTRA_MAGIC_SIZE);
		seeker += dsize - view_size - view_start - EXTRA_MAGIC_SIZE;
	}

	cdata_start = cdata = (byte *)malloc(cdata_size);
	memcpy(cdata, seeker, cdata_size);
	seeker += cdata_size;

	writer = dest + view_start;
	*writer++ = PIC_OP_OPX;
	*writer++ = PIC_OPX_EMBEDDED_VIEW;
	*writer++ = 0;
	*writer++ = 0;
	*writer++ = 0;
	WRITE_LE_UINT16(writer, view_size + 8);
	writer += 2;

	memcpy(writer, viewdata, sizeof(viewdata));
	writer += sizeof(viewdata);

	*writer++ = 0;

	DecompressorLZW__decodeRLE(&seeker, &cdata, writer, view_size);

	free(cdata_start);
    return writer - dest;
}

static void DecompressorLZW__buildCelHeaders(byte **seeker, byte **writer, int celindex, int *cc_lengths, int max) {
    int c;
	for (c = 0; c < max; c++) {
		memcpy(*writer, *seeker, 6);
		*seeker += 6;
		*writer += 6;
		int w = *((*seeker)++);
		WRITE_LE_UINT16(*writer, w); /* Zero extension */
		*writer += 2;

		*writer += cc_lengths[celindex];
		celindex++;
	}
}

int DecompressorLZW__reorderView(byte *src, byte *dest) {
	byte *cellengths;
	int loopheaders;
	int lh_present;
	int lh_mask;
	int pal_offset;
	int cel_total;
	int unknown;
	byte *seeker = src;
	char celcounts[100];
	byte *writer = dest;
	byte *lh_ptr;
	byte *rle_ptr, *pix_ptr;
	int l, lb, c, celindex, lh_last = -1;
	int chptr;
	int w;
	int *cc_lengths;
	byte **cc_pos;

	/* Parse the main header */
	cellengths = src + READ_LE_UINT16(seeker) + 2;
	seeker += 2;
	loopheaders = *seeker++;
	lh_present = *seeker++;
	lh_mask = READ_LE_UINT16(seeker);
	seeker += 2;
	unknown = READ_LE_UINT16(seeker);
	seeker += 2;
	pal_offset = READ_LE_UINT16(seeker);
	seeker += 2;
	cel_total = READ_LE_UINT16(seeker);
	seeker += 2;

	cc_pos = (byte **) malloc(sizeof(byte *) * cel_total);
	cc_lengths = (int *) malloc(sizeof(int) * cel_total);

	for (c = 0; c < cel_total; c++)
		cc_lengths[c] = READ_LE_UINT16(cellengths + 2 * c);

	*writer++ = loopheaders;
	*writer++ = VIEW_HEADER_COLORS_8BIT;
	WRITE_LE_UINT16(writer, lh_mask);
	writer += 2;
	WRITE_LE_UINT16(writer, unknown);
	writer += 2;
	WRITE_LE_UINT16(writer, pal_offset);
	writer += 2;

	lh_ptr = writer;
	writer += 2 * loopheaders; /* Make room for the loop offset table */

	pix_ptr = writer;

	memcpy(celcounts, seeker, lh_present);
	seeker += lh_present;

	lb = 1;
	celindex = 0;

	rle_ptr = pix_ptr = cellengths + (2 * cel_total);
	w = 0;

	for (l = 0; l < loopheaders; l++) {
		if (lh_mask & lb) { /* The loop is _not_ present */
			if (lh_last == -1) {
				printf("Error: While reordering view: Loop not present, but can't re-use last loop");
				lh_last = 0;
			}
			WRITE_LE_UINT16(lh_ptr, lh_last);
			lh_ptr += 2;
		} else {
			lh_last = writer - dest;
			WRITE_LE_UINT16(lh_ptr, lh_last);
			lh_ptr += 2;
			WRITE_LE_UINT16(writer, celcounts[w]);
			writer += 2;
			WRITE_LE_UINT16(writer, 0);
			writer += 2;

			/* Now, build the cel offset table */
			chptr = (writer - dest) + (2 * celcounts[w]);

			for (c = 0; c < celcounts[w]; c++) {
				WRITE_LE_UINT16(writer, chptr);
				writer += 2;
				cc_pos[celindex+c] = dest + chptr;
				chptr += 8 + READ_LE_UINT16(cellengths + 2 * (celindex + c));
			}

			DecompressorLZW__buildCelHeaders(&seeker, &writer, celindex, cc_lengths, celcounts[w]);

			celindex += celcounts[w];
			w++;
		}

		lb = lb << 1;
	}

	if (celindex < cel_total) {
		printf("View decompression generated too few (%d / %d) headers", celindex, cel_total);
		free(cc_pos);
		free(cc_lengths);
		return -1;
	}

	/* Figure out where the pixel data begins. */
	for (c = 0; c < cel_total; c++)
		pix_ptr += DecompressorLZW__getRLEsize(pix_ptr, cc_lengths[c]);

	rle_ptr = cellengths + (2 * cel_total);
	for (c = 0; c < cel_total; c++)
		DecompressorLZW__decodeRLE(&rle_ptr, &pix_ptr, cc_pos[c] + 8, cc_lengths[c]);

	if (pal_offset) {
		*writer++ = 'P';
		*writer++ = 'A';
		*writer++ = 'L';

		for (c = 0; c < 256; c++)
			*writer++ = c;

		seeker -= 4; /* The missing four. Don't ask why. */
		memcpy(writer, seeker, 4*256 + 4);
	}

	free(cc_pos);
	free(cc_lengths);
    return writer - dest;
}

//----------------------------------------------
// DCL decompressor for SCI1.1
//----------------------------------------------

/*
int DecompressorDCL__unpack(byte *src, byte *dest, uint32 nPacked,
                            uint32 nUnpacked) {
	return Common__decompressDCL(src, dest, nPacked, nUnpacked) ? 0 : SCI_ERROR_DECOMPRESSION_ERROR;
}
*/

#ifdef ENABLE_SCI32

//----------------------------------------------
// STACpack/LZS decompressor for SCI32
// Based on Andre Beck's code from http://micky.ibh.de/~beck/stuff/lzs4i4l/
//----------------------------------------------

static int DecompressorLZS__unpackLZS() {
#if defined(__GNUC__) && !defined(__clang__)
uint32 Decompressor__getCompLen() {
	uint32 clen;
	int nibble;
	// The most probable cases are hardcoded
	switch (Decompressor__getBitsMSB(2)) {
	case 0:
		return 2;
	case 1:
		return 3;
	case 2:
		return 4;
	default:
		switch (Decompressor__getBitsMSB(2)) {
		case 0:
			return 5;
		case 1:
			return 6;
		case 2:
			return 7;
		default:
		// Ok, no shortcuts anymore - just get nibbles and add up
			clen = 8;
			do {
				nibble = Decompressor__getBitsMSB(4);
				clen += nibble;
			} while (nibble == 0xf);
			return clen;
		}
	}
}

void Decompressor__copyComp(int offs, uint32 clen) {
	int hpos = _dwWrote - offs;

	while (clen--)
		Decompressor__putByte(_dest[hpos++]);
}


	uint16 offs = 0;
	uint32 clen;

	while (!Decompressor__isFinished()) {
		if (Decompressor__getBitsMSB(1)) { // Compressed bytes follow
			if (Decompressor__getBitsMSB(1)) { // Seven bit offset follows
				offs = Decompressor__getBitsMSB(7);
				if (!offs) // This is the end marker - a 7 bit offset of zero
					break;
				if (!(clen = Decompressor__getCompLen())) {
					printf("lzsDecomp: length mismatch");
					return SCI_ERROR_DECOMPRESSION_ERROR;
				}
				Decompressor__copyComp(offs, clen);
			} else { // Eleven bit offset follows
				offs = Decompressor__getBitsMSB(11);
				if (!(clen = Decompressor__getCompLen())) {
					printf("lzsDecomp: length mismatch");
					return SCI_ERROR_DECOMPRESSION_ERROR;
				}
				Decompressor__copyComp(offs, clen);
			}
		} else // Literal byte follows
			Decompressor__putByte(Decompressor__getByteMSB());
	} // end of while ()
	//return _dwWrote == _szUnpacked ? 0 : SCI_ERROR_DECOMPRESSION_ERROR;
    return _dwWrote;
#else
    return -1;
#endif
}

int DecompressorLZS__unpack(byte *src, byte *dest, uint32 nPacked, uint32 nUnpacked) {
	Decompressor__init(src, dest, nPacked, nUnpacked);
	return DecompressorLZS__unpackLZS();
}
#endif	// #ifdef ENABLE_SCI32



/*****************/



int decodeSRLE(uint8 *dst, byte *dataStream, int unpackedSize) {
    uint8 colorMap[32];
    byte    *bck = dst;

    memcpy(colorMap, dataStream, 32);
    dataStream += 32;

	while (unpackedSize > 0) {
		int size, code = *dataStream++;
		if ((code & 1) == 0) {
			if ((code & 2) == 0) {
				size = (code >> 2) + 1;
				dst += size;
				unpackedSize -= size;
			} else {
				if ((code & 4) == 0) {
					*dst++ = colorMap[code >> 3];
					--unpackedSize;
				} else {
					code >>= 3;
					if (code == 0) {
						size = 1 + *dataStream++;
					} else {
						size = code;
					}
					memset(dst, *dataStream++, MIN(unpackedSize, size));
					dst += size;
					unpackedSize -= size;
				}
			}
		} else {
			code >>= 1;
			if (code == 0) {
				code = 1 + ((dataStream[0] << 8) | dataStream[1]);
                dataStream += 2;
			}
			dst += code;
			unpackedSize -= code;
		}
	}

    return dst - bck;
}



/*****************/



int Screen__unpackRle(byte *source, byte *dest, int32 size) {
    byte    *bck = dest;

	while (size > 0) {
		byte a = *source++;
		byte b = *source++;
		if (a == 0) {
			dest += b;
			size -= b;
		} else {
			b = ((b << 4) & 0xF0) | ((b >> 4) & 0x0F);
			memset(dest, b, a);
			dest += a;
			size -= a;
		}
	}
    return dest - bck;
}



/*****************/



uint16 PS2Icon__decompressData(byte *in, int insz, byte *out, int outsz) {
	uint16 inPos = 1;
	uint16 *rleData = (void *)in;
	uint16 resSize = outsz; //rleData[0];
	uint16 *resData = (void *)out;
	uint16 outPos = 0;

	while (outPos < resSize) {
	uint16 len = rleData[inPos++];
	while (len--)
		resData[outPos++] = 0x7FFF;
	len = rleData[inPos++];
	while (len--)
		resData[outPos++] = rleData[inPos++];
	}

	return outPos;
}



/*****************/



/***************************************************************************
** decomp.c
**
** Routines that deal with AGI version 3 specific features.
** The original LZW code is from DJJ, October 1989, p.86.
** It has been modified to handle AGI compression.
**
** (c) 1997  Lance Ewing
***************************************************************************/



/**
 * Uncompress the data contained in the input buffer and store
 * the result in the output buffer. The fileLength parameter says how
 * many bytes to uncompress. The compression itself is a form of LZW that
 * adjusts the number of bits that it represents its codes in as it fills
 * up the available codes. Two codes have special meaning:
 *
 *  code 256 = start over
 *  code 257 = end of data
 */
int LZWDecoder__lzwExpand(uint8 *in, uint8 *out, int32 len) {

#if defined(__GNUC__) && !defined(__clang__)
	enum {
		MAXBITS		= 12,
		TABLE_SIZE	= 18041,	// strange number
		START_BITS	= 9
	};

	int32 BITS, MAX_VALUE, MAX_CODE;
	uint32 *prefixCode;
	uint8 *appendCharacter;
	uint8 *decodeStack;
	int32 inputBitCount;	// Number of bits in input bit buffer
	uint32 inputBitBuffer;


void LZWDecoder__LZWDecoder(void) {
	decodeStack = (uint8 *)calloc(1, 8192);
	prefixCode = (uint32 *)malloc(TABLE_SIZE * sizeof(uint32));
	appendCharacter = (uint8 *)malloc(TABLE_SIZE * sizeof(uint8));
	inputBitCount = 0;	// Number of bits in input bit buffer
	inputBitBuffer = 0L;
}

void LZWDecoder__xLZWDecoder(void) {
	free(decodeStack);
	free(prefixCode);
	free(appendCharacter);
}

/**
 * Adjust the number of bits used to store codes to the value passed in.
 */
int LZWDecoder__setBits(int32 value) {
	if (value == MAXBITS)
		return true;

	BITS = value;
	MAX_VALUE = (1 << BITS) - 1;
	MAX_CODE = MAX_VALUE - 1;

	return false;
}

/**
 * Return the string that the code taken from the input buffer
 * represents. The string is returned as a stack, i.e. the characters are
 * in reverse order.
 */
uint8 *LZWDecoder__decodeString(uint8 *buffer, uint32 code) {
	uint32 i;

	for (i = 0; code > 255;) {
		*buffer++ = appendCharacter[code];
		code = prefixCode[code];
		if (i++ >= 4000) {
            return NULL; //error("lzw: error in code expansion");
		}
	}
	*buffer = code;

	return buffer;
}

/**
 * Return the next code from the input buffer.
 */
uint32 LZWDecoder__inputCode(uint8 **input) {
	uint32 r;

	while (inputBitCount <= 24) {
		inputBitBuffer |= (uint32) * (*input)++ << inputBitCount;
		inputBitCount += 8;
	}
	r = (inputBitBuffer & 0x7FFF) % (1 << BITS);
	inputBitBuffer >>= BITS;
	inputBitCount -= BITS;

	return r;
}


    byte *bck = out;
	int32 c, lzwnext, lzwnew, lzwold;
	uint8 *s, *end;

	//LZWDecoder d;
    LZWDecoder__LZWDecoder();

	LZWDecoder__setBits(START_BITS);	// Starts at 9-bits
	lzwnext = 257;		// Next available code to define

	end = (uint8 *)(out + (uint32)len);

	lzwold = LZWDecoder__inputCode(&in);	// Read in the first code
	c = lzwold;
	lzwnew = LZWDecoder__inputCode(&in);

	while ((out < end) && (lzwnew != 0x101)) {
		if (lzwnew == 0x100) {
			// Code to "start over"
			lzwnext = 258;
			LZWDecoder__setBits(START_BITS);
			lzwold = LZWDecoder__inputCode(&in);
			c = lzwold;
			*out++ = (char)c;
			lzwnew = LZWDecoder__inputCode(&in);
		} else {
			if (lzwnew >= lzwnext) {
				// Handles special LZW scenario
				*decodeStack = c;
				s = LZWDecoder__decodeString(decodeStack + 1, lzwold);
			} else
				s = LZWDecoder__decodeString(decodeStack, lzwnew);

			// Reverse order of decoded string and
			// store in out buffer
			c = *s;
			while (s >= decodeStack)
				*out++ = *s--;

			if (lzwnext > MAX_CODE)
				LZWDecoder__setBits(BITS + 1);

			prefixCode[lzwnext] = lzwold;
			appendCharacter[lzwnext] = c;
			lzwnext++;
			lzwold = lzwnew;

			lzwnew = LZWDecoder__inputCode(&in);
		}
	}

    LZWDecoder__xLZWDecoder();

    return out - bck;
#else
    return -1;
#endif
}



/*****************/



int uncompressPlane(const byte *plane, byte *outptr, int length) {
    length /= 2;    // they are all 16bits
    byte *bck = outptr;
    int i;
	while (length != 0) {
		int wordlen;
		signed char x = *plane++;
		if (x >= 0) {
			wordlen = MIN(x + 1, length);
			uint16 w = READ_UINT16(plane); plane += 2;
			for (i = 0; i < wordlen; ++i) {
				WRITE_UINT16(outptr, w); outptr += 2;
			}
		} else {
			wordlen = MIN(-x, length);
			memcpy(outptr, plane, wordlen * 2);
			outptr += wordlen * 2;
			plane += wordlen * 2;
		}
		length -= wordlen;
	}
    return outptr - bck;
}



/*****************/



int unbarchive(byte *data, int datasz, byte *dst) {
	byte current, what;
	byte stopper = 0x00;    //_files[i]._stopper;
	uint repeat;
	uint len = 0; // Sanity check (counts uncompressed bytes)

    byte *data_bck = data;
    byte *bck = dst;
	current = *data++; // Read initial byte
	while ((data - data_bck) < datasz) {
		if (current != stopper) {
			*dst++ = current;
			++len;
		} else {
			// Inflate block
			repeat = *data++;
			what = *data++;
			len += repeat;
            uint j;
			for (j = 0; j < repeat; ++j) {
				*dst++ = what;
			}
		}

		current = *data++;
	}
    return dst - bck;
}



/*****************/



int32 Screen__decompressRLE256(byte *dst, byte *src, int32 decompSize) {
    byte    *bck = dst;
	// PARAMETERS:
	// source	points to the start of the sprite data for input
	// decompSize	gives size of decompressed data in bytes
	// dest		points to start of destination buffer for decompressed
	//		data

	byte headerByte;			// block header byte
	byte *endDest = dst + decompSize;	// pointer to byte after end of decomp buffer
	int32 rv;

	while (1) {
		// FLAT block
		// read FLAT block header & increment 'scan' to first pixel
		// of block
		headerByte = *src++;

		// if this isn't a zero-length block
		if (headerByte) {
			if (dst + headerByte > endDest) {
				rv = 1;
				break;
			}

			// set the next 'headerByte' pixels to the next color
			// at 'source'
			memset(dst, *src, headerByte);

			// increment destination pointer to just after this
			// block
			dst += headerByte;

			// increment source pointer to just after this color
			src++;

			// if we've decompressed all of the data
			if (dst == endDest) {
				rv = 0;		// return "OK"
				break;
			}
		}

		// RAW block
		// read RAW block header & increment 'scan' to first pixel of
		// block
		headerByte = *src++;

		// if this isn't a zero-length block
		if (headerByte) {
			if (dst + headerByte > endDest) {
				rv = 1;
				break;
			}

			// copy the next 'headerByte' pixels from source to
			// destination
			memcpy(dst, src, headerByte);

			// increment destination pointer to just after this
			// block
			dst += headerByte;

			// increment source pointer to just after this block
			src += headerByte;

			// if we've decompressed all of the data
			if (dst == endDest) {
				rv = 0;		// return "OK"
				break;
			}
		}
	}

    if(rv) return -1;
	//return rv;
    return dst - bck;
}



uint32 Screen__decompressHIF_other(byte *src, byte *dst/*, uint32 *skipData*/) {
	uint32 decompSize = 0;
	uint32 readByte = 0;

	for (;;) { // Main loop
		byte control_byte = *src++;
		readByte++;
		uint32 byte_count = 0;
		while (byte_count < 8) {
			if (control_byte & 0x80) {
				uint16 info_word = READ_BE_UINT16(src); // Read the info word
				src += 2;
				readByte += 2;
				if (info_word == 0xFFFF) { // Got 0xFFFF code, finished.
					//if (skipData != NULL) *(skipData) = readByte;
					return decompSize;
				}

				int32 repeat_count = (info_word >> 12) + 2; // How many time data needs to be refetched
				while (repeat_count >= 0) {
					uint16 refetchData = (info_word & 0xFFF) + 1;
					if (refetchData > decompSize) return 0; // We have a problem here...
					uint8 *old_data_src = dst - refetchData;
					*dst++ = *old_data_src;
					decompSize++;
					repeat_count--;
				}
			} else {
				*dst++ = *src++;
				readByte++;
				decompSize++;
			}
			byte_count++;
			control_byte <<= 1; // Shifting left the control code one bit
		}
	}
    return -1;
}



/*****************/



int FileExpander__process(uint8 *dst, uint8 *src, uint32 outsize, uint32 compressedSize) {
#if defined(__GNUC__) && !defined(__clang__)
	const uint8 *_dataPtr;
	const uint8 *_endofBuffer;
	uint16 _key;
	int8 _bitsLeft;
	uint8 _index;

	uint8 FileExpander__getKeyLower(void) { return _key & 0xFF; }
	void FileExpander__setIndex(uint8 index) { _index = index; }

void FileExpanderSource__advSrcBitsBy1() {
	_key >>= 1;
	if (!--_bitsLeft) {
		if (_dataPtr < _endofBuffer)
			_key = ((*_dataPtr++) << 8) | (_key & 0xFF);
		_bitsLeft = 8;
	}
}

void FileExpanderSource__advSrcBitsByIndex(uint8 newIndex) {
	_index = newIndex;
	_bitsLeft -= _index;
	if (_bitsLeft <= 0) {
		_key >>= (_index + _bitsLeft);
		_index = -_bitsLeft;
		_bitsLeft = 8 - _index;
		if (_dataPtr < _endofBuffer)
			_key = (*_dataPtr++ << 8) | (_key & 0xFF);
	}
	_key >>= _index;
}

uint16 FileExpanderSource__getKeyMasked(uint8 newIndex) {
	static const uint8 mskTable[] = { 0x0F, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };
	_index = newIndex;
	uint16 res = 0;

	if (_index > 8) {
		newIndex = _index - 8;
		res = (_key & 0xFF) & mskTable[8];
		FileExpanderSource__advSrcBitsByIndex(8);
		_index = newIndex;
		res |= (((_key & 0xFF) & mskTable[_index]) << 8);
		FileExpanderSource__advSrcBitsByIndex(_index);
	} else {
		res = (_key & 0xFF) & mskTable[_index];
		FileExpanderSource__advSrcBitsByIndex(_index);
	}

	return res;
}

void FileExpanderSource__copyBytes(uint8 ** ret_dst) {
    uint8 *dst = *ret_dst;

	FileExpanderSource__advSrcBitsByIndex(_bitsLeft);
	uint16 r = (READ_LE_UINT16(_dataPtr) ^ _key) + 1;
	_dataPtr += 2;

	if (r)
		exit(1); //error("decompression failure");

	memcpy(dst, _dataPtr, _key);
	_dataPtr += _key;
	dst += _key;
    *ret_dst = dst;
}

uint16 FileExpanderSource__keyMaskedAlign(uint16 val) {
	val -= 0x101;
	_index = (val & 0xFF) >> 2;
	int16 b = ((_bitsLeft << 8) | _index) - 1;
	_bitsLeft = b >> 8;
	_index = b & 0xFF;
	uint16 res = (((val & 3) + 4) << _index) + 0x101;
	return res + FileExpanderSource__getKeyMasked(_index);
}

void FileExpanderSource__advSrcRefresh(void) {
	_key = READ_LE_UINT16(_dataPtr);
	if (_dataPtr < _endofBuffer - 1)
		_dataPtr += 2;
	_bitsLeft = 8;
}

	//FileExpanderSource *_src;
	uint8 *_tables[9]={NULL};
	uint16 *_tables16[3]={NULL};

void FileExpander__FileExpander(void) {
    _src = NULL;
	_tables[0] = calloc(3914, 1);
	//assert(_tables[0]);

	_tables[1] = _tables[0] + 320;
	_tables[2] = _tables[0] + 352;
	_tables[3] = _tables[0] + 864;
	_tables[4] = _tables[0] + 2016;
	_tables[5] = _tables[0] + 2528;
	_tables[6] = _tables[0] + 2656;
	_tables[7] = _tables[0] + 2736;
	_tables[8] = _tables[0] + 2756;

	_tables16[0] = (uint16 *)(_tables[0] + 3268);
	_tables16[1] = (uint16 *)(_tables[0] + 3302);
	_tables16[2] = (uint16 *)(_tables[0] + 3338);
}

void FileExpander__xFileExpander(void) {
	free(_src);
	free(_tables[0]);
}

void FileExpander__generateTables(uint8 srcIndex, uint8 dstIndex, uint8 dstIndex2, int cnt) {
	uint8 *tbl1 = _tables[srcIndex];
	uint8 *tbl2 = _tables[dstIndex];
	uint8 *tbl3 = dstIndex2 == 0xFF ? 0 : _tables[dstIndex2];

	if (!cnt)
		return;

	const uint8 *s = tbl1;
	memset(_tables16[0], 0, 32);

    int i;
	for ( i = 0; i < cnt; i++)
		_tables16[0][(*s++)]++;

	_tables16[1][1] = 0;

    int r;
	for ( i = 1, r = 0; i < 16; i++) {
		r = (r + _tables16[0][i]) << 1;
		_tables16[1][i + 1] = r;
	}

	if (_tables16[1][16]) {
		uint16 r = 0;
		for ( i = 1; i < 16; i++)
			r += _tables16[0][i];
		if (r > 1)
			exit(1); //error("decompression failure");
	}

	s = tbl1;
	uint16 *d = _tables16[2];
	for ( i = 0; i < cnt; i++) {
		uint16 t = *s++;
		if (t) {
			_tables16[1][t]++;
			t = _tables16[1][t] - 1;
		}
		*d++ = t;
	}

	s = tbl1;
	d = _tables16[2];
	for (i = 0; i < cnt; i++) {
		int8 t = ((int8)(*s++)) - 1;
		if (t > 0) {
			uint16 v1 = *d;
			uint16 v2 = 0;

			do {
				v2 = (v2 << 1) | (v1 & 1);
				v1 >>= 1;
			} while (--t && v1);

			t++;
			uint8 c1 = (v1 & 1);
			while (t--) {
				uint8 c2 = v2 >> 15;
				v2 = (v2 << 1) | c1;
				c1 = c2;
			};

			*d++ = v2;
		} else {
			d++;
		}
	}

	memset(tbl2, 0, 512);

	cnt--;
	s = tbl1 + cnt;
	d = &_tables16[2][cnt];
	uint16 *bt = (uint16 *)tbl3;
	uint16 inc = 0;
	uint16 cnt2 = 0;

	do {
		uint8 t = *s--;
		uint16 *s2 = (uint16 *)tbl2;

		if (t && t < 9) {
			inc = 1 << t;
			uint16 o = *d;

			do {
				s2[o] = cnt;
				o += inc;
			} while (!(o & 0xF00));

		} else if (t > 8) {
			if (!bt)
				exit(1); //error("decompression failure");

			t -= 8;
			uint8 shiftCnt = 1;
			uint8 v = (*d) >> 8;
			s2 = &((uint16 *)tbl2)[*d & 0xFF];

			do {
				if (!*s2) {
					*s2 = (uint16)(~cnt2);
					*(uint32 *)&bt[cnt2] = 0;
					cnt2 += 2;
				}

				s2 = &bt[(uint16)(~*s2)];
				if (v & shiftCnt)
					s2++;

				shiftCnt <<= 1;
			} while (--t);
			*s2 = cnt;
		}
		d--;
	} while (--cnt >= 0);
}

uint8 FileExpander__calcCmdAndIndex(const uint8 *tbl, int16 *ret_para) {
    int16 para = ret_para ? *ret_para : 0;
	const uint16 *t = (const uint16 *)tbl;
	/*_src->*/ FileExpanderSource__advSrcBitsByIndex(8);
	uint8 newIndex = 0;
	uint16 v = /*_src->*/ FileExpander__getKeyLower();

	do {
		newIndex++;
		para = t[((~para) & 0xFFFE) | (v & 1)];
		v >>= 1;
	} while (para < 0);

    if(ret_para) *ret_para = para;
	return newIndex;
}


	static const uint8 indexTable[] = {
		0x10, 0x11, 0x12, 0x00, 0x08, 0x07, 0x09, 0x06, 0x0A,
		0x05, 0x0B, 0x04, 0x0C, 0x03, 0x0D, 0x02, 0x0E, 0x01, 0x0F
	};

	memset(_tables[0], 0, 3914);

	uint8 *d = dst;
	uint16 tableSize0 = 0;
	uint16 tableSize1 = 0;
	bool needrefresh = true;
	bool postprocess = false;

    FileExpander__FileExpander();

	_src = src; //_src = new FileExpanderSource(src, compressedSize);

	while (d < dst + outsize) {

		if (needrefresh) {
			needrefresh = false;
			/*_src->*/ FileExpanderSource__advSrcRefresh();
		}

		/*_src->*/ FileExpanderSource__advSrcBitsBy1();

		int mode = /*_src->*/ FileExpanderSource__getKeyMasked(2) - 1;
		if (mode == 1) {
			tableSize0 = /*_src->*/ FileExpanderSource__getKeyMasked(5) + 257;
			tableSize1 = /*_src->*/ FileExpanderSource__getKeyMasked(5) + 1;
			memset(_tables[7], 0, 19);

			const uint8 *itbl = indexTable;
			int numbytes = /*_src->*/ FileExpanderSource__getKeyMasked(4) + 4;

			while (numbytes--)
				_tables[7][*itbl++] = /*_src->*/ FileExpanderSource__getKeyMasked(3);

			FileExpander__generateTables(7, 8, 255, 19);

			int cnt = tableSize0 + tableSize1;
			uint8 *tmp = _tables[0];

			while (cnt) {
				uint16 cmd = /*_src->*/ FileExpander__getKeyLower();
				cmd = READ_LE_UINT16(&_tables[8][cmd << 1]);
				/*_src->*/ FileExpanderSource__advSrcBitsByIndex(_tables[7][cmd]);

				if (cmd < 16) {
					*tmp++ = cmd;
					cnt--;
				} else {
					uint8 tmpI = 0;
					if (cmd == 16) {
						cmd = /*_src->*/ FileExpanderSource__getKeyMasked(2) + 3;
						tmpI = *(tmp - 1);
					} else if (cmd == 17) {
						cmd = /*_src->*/ FileExpanderSource__getKeyMasked(3) + 3;
					} else {
						cmd = /*_src->*/ FileExpanderSource__getKeyMasked(7) + 11;
					}
					/*_src->*/ FileExpander__setIndex(tmpI);
					memset(tmp, tmpI, cmd);
					tmp += cmd;

					cnt -= cmd;
					if (cnt < 0)
						exit(1); //error("decompression failure");
				}
			}

			memcpy(_tables[1], _tables[0] + tableSize0, tableSize1);
			FileExpander__generateTables(0, 2, 3, tableSize0);
			FileExpander__generateTables(1, 4, 5, tableSize1);
			postprocess = true;
		} else if (mode < 0) {
			/*_src->*/ FileExpanderSource__copyBytes(&d);
			postprocess = false;
			needrefresh = true;
		} else if (mode == 0) {
			uint8 *d2 = _tables[0];
			memset(d2, 8, 144);
			memset(d2 + 144, 9, 112);
			memset(d2 + 256, 7, 24);
			memset(d2 + 280, 8, 8);
			d2 = _tables[1];
			memset(d2, 5, 32);
			tableSize0 = 288;
			tableSize1 = 32;

			FileExpander__generateTables(0, 2, 3, tableSize0);
			FileExpander__generateTables(1, 4, 5, tableSize1);
			postprocess = true;
		} else {
			exit(1); //error("decompression failure");
		}

		if (!postprocess)
			continue;

		int16 cmd = 0;

		do  {
			cmd = ((int16 *)_tables[2])[/*_src->*/ FileExpander__getKeyLower()];
			/*_src->*/ FileExpanderSource__advSrcBitsByIndex(cmd < 0 ? FileExpander__calcCmdAndIndex(_tables[3], &cmd) : _tables[0][cmd]);

			if (cmd == 0x11D) {
				cmd = 0x200;
			} else if (cmd > 0x108) {
				cmd = /*_src->*/ FileExpanderSource__keyMaskedAlign(cmd);
			}

			if (!(cmd >> 8)) {
				*d++ = cmd & 0xFF;
			} else if (cmd != 0x100) {
				cmd -= 0xFE;
				int16 offset = ((int16 *)_tables[4])[/*_src->*/ FileExpander__getKeyLower()];
				/*_src->*/ FileExpanderSource__advSrcBitsByIndex(offset < 0 ? FileExpander__calcCmdAndIndex(_tables[5], &offset) : _tables[1][offset]);
				if ((offset & 0xFF) >= 4) {
					uint8 newIndex = ((offset & 0xFF) >> 1) - 1;
					offset = (((offset & 1) + 2) << newIndex);
					offset += /*_src->*/ FileExpanderSource__getKeyMasked(newIndex);
				}

				uint8 *s2 = d - 1 - offset;
				if (s2 >= dst) {
					while (cmd--)
						*d++ = *s2++;
				} else {
					uint32 pos = dst - s2;
					s2 += (d - dst);

					if (pos < (uint32) cmd) {
						cmd -= pos;
						while (pos--)
							*d++ = *s2++;
						s2 = dst;
					}
					while (cmd--)
						*d++ = *s2++;
				}
			}
		} while (cmd != 0x100);
	}

	//delete _src;
	//_src = 0;

    FileExpander__xFileExpander();
	//return true;
    return d - dst;
#else
    return -1;
#endif
}



/*****************/



int SoundTownsPC98_v2__voicePlay(byte *src, byte *dst, int outsize) {
    uint32 i;
		for ( i = outsize; i;) {
			uint8 cnt = *src++;
			if (cnt & 0x80) {
				cnt &= 0x7F;
				memset(dst, *src++, cnt);
			} else {
				memcpy(dst, src, cnt);
				src += cnt;
			}
			dst += cnt;
			i -= cnt;
		}
    return outsize;
}



/*****************/



#define scummvm_CLIP(v, amin, amax) \
    ((v < amin) ? amin : ((v > amax) ? amax : v))

int VQAMovie__decodeSND1(byte *inbuf, uint32 insize, byte *outbuf, uint32 outsize) {
    byte *bck = outbuf;

//inline int16 CLIP(int16 v, int16 amin, int16 amax)
//		{ if (v < amin) return amin; else if (v > amax) return amax; else return v; }

	const int8 WSTable2Bit[] = { -2, -1, 0, 1 };
	const int8 WSTable4Bit[] = {
		-9, -8, -6, -5, -4, -3, -2, -1,
		 0,  1,  2,  3,  4,  5,  6,  8
	};

	byte code;
	int8 count;
	uint16 input;

	int16 curSample = 0x80;

	while (outsize > 0) {
		input = *inbuf++ << 2;
		code = (input >> 8) & 0xFF;
		count = (input & 0xFF) >> 2;

		switch (code) {
		case 2:
			if (count & 0x20) {
				/* NOTE: count is signed! */
				count <<= 3;
				curSample += (count >> 3);
				*outbuf++ = curSample;
				outsize--;
			} else {
				for (; count >= 0; count--) {
					*outbuf++ = *inbuf++;
					outsize--;
				}
				curSample = *(outbuf - 1);
			}
			break;
		case 1:
			for (; count >= 0; count--) {
				code = *inbuf++;

				curSample += WSTable4Bit[code & 0x0F];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				curSample += WSTable4Bit[code >> 4];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				outsize -= 2;
			}
			break;
		case 0:
			for (; count >= 0; count--) {
				code = *inbuf++;

				curSample += WSTable2Bit[code & 0x03];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				curSample += WSTable2Bit[(code >> 2) & 0x03];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				curSample += WSTable2Bit[(code >> 4) & 0x03];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				curSample += WSTable2Bit[(code >> 6) & 0x03];
				curSample = scummvm_CLIP(curSample, 0, 255);
				*outbuf++ = curSample;

				outsize -= 4;
			}
			break;
		default:
			for (; count >= 0; count--) {
				*outbuf++ = curSample;
				outsize--;
			}
		}
	}
    return outbuf - bck;
}



/*****************/



int Background__decodeComponent(byte *in, uint32 inSize, byte *out, uint32 outSize) {
	// Initialize the decoding
	memset(out, 0, outSize);
	uint32 inPos = 0;
	uint32 outPos = 0;
    int i;

	// Decode
	while (inPos < inSize) {
		byte inByte = *in++;
		inPos++;

		if (inByte < 0x80) {
			// Direct decompression (RLE)
			byte len = (inByte >> 5) + 1;
			byte data = inByte & 0x1f;
			for ( i = 0; i < len && outPos < outSize; i++)
				out[outPos++] = data;
		} else {
			// Buffer back reference, 4096 byte window
			// Take inByte and the following value as a big endian
			// OfsLen while zeroing the first bit
			uint16 ofsLen = ((inByte & 0x7F) << 8) | *in++;
			inPos++;

			int32 len = (ofsLen >> 12) + 3;
			int32 hisPos = (int32)(outPos + (ofsLen & 0x0FFF) - 4096);
			for ( i = 0; i < len && outPos < outSize; i++)
				out[outPos++] = out[hisPos++];
		}
	}

	return outPos;
}



/*****************/



int AnimFrame__decomp34(byte *in, byte *p, uint32 size, byte mask, byte shift) {
    int _palSize = 1;
	//byte *p = (byte *)_image.getPixels();

	uint32 skip = 0; //f.initialSkip / 2;
	//uint32 size = f.decompressedEndOffset / 2;
	//printf("skip: %d, %d", skip % 640, skip / 640);
	//printf("size: %d, %d", size % 640, size / 640);
	//assert (f.yPos1 == skip / 640);
	//assert (f.yPos2 == size / 640);

	uint32 numBlanks = 640 - 0; //(f.xPos2 - f.xPos1);

	//in->seek((int)f.dataOffset);
    uint32 out;
	for ( out = skip; out < size; ) {
		uint16 opcode = *in++;

		if (opcode & 0x80) {
			if (opcode & 0x40) {
				opcode &= 0x3f;
				out += numBlanks + opcode + 1;
			} else {
				opcode &= 0x3f;
				if (opcode & 0x20) {
					opcode = ((opcode & 0x1f) << 8) + *in++;
					if (opcode & 0x1000) {
						out += opcode & 0xfff;
						continue;
					}
				}
				out += opcode + 2;
			}
		} else {
			byte value = opcode & mask;
			opcode >>= shift;
			if (_palSize <= value)
				_palSize = value + 1;
			if (!opcode)
				opcode = *in++;
            int i;
			for ( i = 0; i < opcode; i++, out++) {
				p[out] = value;
			}
		}
	}
    return out;
}

int AnimFrame__decomp5(byte *in, byte *p, uint32 size) {
    int _palSize = 1;
	//byte *p = (byte *)_image.getPixels();

	uint32 skip = 0; //f.initialSkip / 2;
	//uint32 size = f.decompressedEndOffset / 2;
	//printf("skip: %d, %d", skip % 640, skip / 640);
	//printf("size: %d, %d", size % 640, size / 640);
	//assert (f.yPos1 == skip / 640);
	//assert (f.yPos2 == size / 640);

	//in->seek((int)f.dataOffset);
    uint32 out;
	for (out = skip; out < size; ) {
		uint16 opcode = *in++;
		if (!(opcode & 0x1f)) {
			opcode = (uint16)((opcode << 3) + *in++);
			if (opcode & 0x400) {
				// skip these 10 bits
				out += (opcode & 0x3ff);
			} else {
				out += opcode + 2;
			}
		} else {
			byte value = opcode & 0x1f;
			opcode >>= 5;
			if (_palSize <= value)
				_palSize = value + 1;
			if (!opcode)
				opcode = *in++;
            int i;
			for ( i = 0; i < opcode; i++, out++) {
				p[out] = value;
			}
		}
	}
    return out;
}

int AnimFrame__decomp7(byte *in, byte *p, uint32 size) {
    int _palSize = 1;
	//byte *p = (byte *)_image.getPixels();

	uint32 skip = 0; //f.initialSkip / 2;
	//uint32 size = f.decompressedEndOffset / 2;
	//printf("skip: %d, %d", skip % 640, skip / 640);
	//printf("size: %d, %d", size % 640, size / 640);
	//assert (f.yPos1 == skip / 640);
	//assert (f.yPos2 == size / 640);

	uint32 numBlanks = 640 - 0; //(f.xPos2 - f.xPos1);

	//in->seek((int)f.dataOffset);
    uint32 out;
	for ( out = skip; out < size; ) {
		uint16 opcode = *in++;
		if (opcode & 0x80) {
			if (opcode & 0x40) {
				if (opcode & 0x20) {
					opcode &= 0x1f;
					out += numBlanks + opcode + 1;
				} else {
					opcode &= 0x1f;
					if (opcode & 0x10) {
						opcode = ((opcode & 0xf) << 8) + *in++;
						if (opcode & 0x800) {
							// skip these 11 bits
							out += (opcode & 0x7ff);
							continue;
						}
					}

					// skip these 4 bits
					out += opcode + 2;
				}
			} else {
				opcode &= 0x3f;
				byte value = *in++;
				if (_palSize <= value)
					_palSize = value + 1;
                int i;
				for ( i = 0; i < opcode; i++, out++) {
					p[out] = value;
				}
			}
		} else {
			if (_palSize <= opcode)
				_palSize = opcode + 1;
			// set the given value
			p[out] = (byte)opcode;
			out++;
		}
	}
    return out;
}

int AnimFrame__decompFF(byte *in, byte *p, uint32 size) {
    int _palSize = 1;
	//byte *p = (byte *)_image.getPixels();

	uint32 skip = 0; //f.initialSkip / 2;
	//uint32 size = f.decompressedEndOffset / 2;

    int i;

	//in->seek((int)f.dataOffset);
    uint32 out;
	for ( out = skip; out < size; ) {
		uint16 opcode = *in++;

		if (opcode < 0x80) {
			if (_palSize <= opcode)
				_palSize = opcode + 1;
			// set the given value
			p[out] = (byte)opcode;
			out++;
		} else {
			if (opcode < 0xf0) {
				if (opcode < 0xe0) {
					// copy old part
					uint32 old = out + ((opcode & 0x7) << 8) + *in++ - 2048;
					opcode = ((opcode >> 3) & 0xf) + 3;
					for ( i = 0; i < opcode; i++, out++, old++) {
						p[out] = p[old];
					}
				} else {
					opcode = (opcode & 0xf) + 1;
					byte value = *in++;
					if (_palSize <= value)
						_palSize = value + 1;
					for ( i = 0; i < opcode; i++, out++) {
						p[out] = value;
					}
				}
			} else {
				out += ((opcode & 0xf) << 8) + *in++;
			}
		}
	}
    return out;
}



/*****************/



uint32 SavegameStream__readCompressed(byte *in, void *dataPtr, uint32 dataSize) {
    int 	_valueCount = 0,
            _previousValue=0,
            _repeatCount=0;
	byte *data = (byte *)dataPtr;

	while (dataSize) {
		switch (_valueCount) {
		default:
			return -1; //error("[SavegameStream__readCompressed] Invalid value count (%d)", _valueCount);

		case 0:
		case 1: {
			// Read control code
			byte control = *in++;

			switch (control) {
			default:
				// Data value
				*data++ = control;
				break;

			case 0xFB:
				_repeatCount = 2;
				_previousValue = 0;
				*data++ = 0;
				_valueCount = 2;
				break;

			case 0xFC:
				_repeatCount = 254;
				_previousValue = 0;
				*data++ = 0;
				_valueCount = 2;
				break;

			case 0xFD:
				_repeatCount = *in++ - 1;
				_previousValue = 0;
				*data++ = 0;
				_valueCount = 2;
				break;

			case 0xFE:
				*data++ = *in++;
				break;

			case 0xFF:
				_repeatCount = *in++ - 1;
				_previousValue = *in++;
				*data++ = _previousValue;
				_valueCount = 2;
				break;
			}
			}
			break;

		case 2:
			*data++ = _previousValue;
			_repeatCount--;
			if (!_repeatCount)
				_valueCount = 1;
			break;
		}

		--dataSize;
	}

	return data - (byte *)dataPtr;
}



/*****************/



#define compDecode_NextBit                            \
	do {                                   \
		bit = mask & 1;                    \
		mask >>= 1;                        \
		if (!--bitsleft) {                 \
			mask = READ_LE_UINT16(srcptr); \
			srcptr += 2;                   \
			bitsleft = 16;                 \
		}                                  \
	} while (0)

int32 dimuse_compDecode(byte *src, byte *dst) {
	byte *result, *srcptr = src, *dstptr = dst;
	int data, size, bit, bitsleft = 16, mask = READ_LE_UINT16(srcptr);
	srcptr += 2;

	for (;;) {
		compDecode_NextBit;
		if (bit) {
			*dstptr++ = *srcptr++;
		} else {
			compDecode_NextBit;
			if (!bit) {
				compDecode_NextBit;
				size = bit << 1;
				compDecode_NextBit;
				size = (size | bit) + 3;
				data = *srcptr++ | 0xffffff00;
			} else {
				data = *srcptr++;
				size = *srcptr++;

				data |= 0xfffff000 + ((size & 0xf0) << 4);
				size = (size & 0x0f) + 3;

				if (size == 3)
					if (((*srcptr++) + 1) == 1)
						return dstptr - dst;
			}
			result = dstptr + data;
			while (size--)
				*dstptr++ = *result++;
		}
	}
}
#undef compDecode_NextBit


int32 decompressADPCM(byte *compInput, byte *compOutput, int channels) {

//inline int32 CLIP(int32 v, int32 amin, int32 amax)
//		{ if (v < amin) return amin; else if (v > amax) return amax; else return v; }

const int16 _imaTable[89] = {
		7,    8,    9,   10,   11,   12,   13,   14,
	   16,   17,   19,   21,   23,   25,   28,   31,
	   34,   37,   41,   45,   50,   55,   60,   66,
	   73,   80,   88,   97,  107,  118,  130,  143,
	  157,  173,  190,  209,  230,  253,  279,  307,
	  337,  371,  408,  449,  494,  544,  598,  658,
	  724,  796,  876,  963, 1060, 1166, 1282, 1411,
	 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	 7132, 7845, 8630, 9493,10442,11487,12635,13899,
	15289,16818,18500,20350,22385,24623,27086,29794,
	32767
};

// This table is the "big brother" of Audio__ADPCMStream___stepAdjustTable.
static const byte imxOtherTable[6][64] = {
	{
		0xFF,
		4
	},

	{
		0xFF, 0xFF,
		   2,    8
	},

	{
		0xFF, 0xFF, 0xFF, 0xFF,
		   1,    2,    4,    6
	},

	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		   1,    2,    4,    6,    8,   12,   16,   32
	},

	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		   1,    2,    4,    6,    8,   10,   12,   14,
		  16,   18,   20,   22,   24,   26,   28,   32
	},

	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		   1,    2,    3,    4,    5,    6,    7,    8,
		   9,   10,   11,   12,   13,   14,   15,   16,
		  17,   18,   19,   20,   21,   22,   23,   24,
		  25,   26,   27,   28,   29,   30,   31,   32
	}
};

    static byte    *_destImcTable = NULL;
    static uint32  *_destImcTable2 = NULL;

//void initializeImcTables() {
	int pos;

	if (!_destImcTable) _destImcTable = (byte *)calloc(89, sizeof(byte));
	if (!_destImcTable2) _destImcTable2 = (uint32 *)calloc(89 * 64, sizeof(uint32));

	for (pos = 0; pos <= 88; ++pos) {
		byte put = 1;
		int32 tableValue = ((_imaTable[pos] * 4) / 7) / 2;
		while (tableValue != 0) {
			tableValue /= 2;
			put++;
		}
		if (put < 3) {
			put = 3;
		}
		if (put > 8) {
			put = 8;
		}
		_destImcTable[pos] = put - 1;
	}

    int n;
	for (n = 0; n < 64; n++) {
		for (pos = 0; pos <= 88; ++pos) {
			int32 count = 32;
			int32 put = 0;
			int32 tableValue = _imaTable[pos];
			do {
				if ((count & n) != 0) {
					put += tableValue;
				}
				count /= 2;
				tableValue /= 2;
			} while (count != 0);
			_destImcTable2[n + pos * 64] = put;
		}
	}
//}
    //initializeImcTables();

	byte *src;

	// Decoder for the the IMA ADPCM variants used in COMI.
	// Contrary to regular IMA ADPCM, this codec uses a variable
	// bitsize for the encoded data.

	//const int 2 /*MAX_CHANNELS*/ = 2;
	int32 outputSamplesLeft;
	int32 destPos;
	int16 firstWord;
	byte initialTablePos[2 /*MAX_CHANNELS*/] = {0, 0};
	//int32 initialimcTableEntry[2 /*MAX_CHANNELS*/] = {7, 7};
	int32 initialOutputWord[2 /*MAX_CHANNELS*/] = {0, 0};
	int32 totalBitOffset, curTablePos, outputWord;
	byte *dst;
	int i;

	// We only support mono and stereo
	//assert(channels == 1 || channels == 2);

	src = compInput;
	dst = compOutput;
	outputSamplesLeft = 0x1000;

	// Every data packet contains 0x2000 bytes of audio data
	// when extracted. In order to encode bigger data sets,
	// one has to split the data into multiple blocks.
	//
	// Every block starts with a 2 byte word. If that word is
	// non-zero, it indicates the size of a block of raw audio
	// data (not encoded) following it. That data we simply copy
	// to the output buffer and then proceed by decoding the
	// remaining data.
	//
	// If on the other hand the word is zero, then what follows
	// are 7*channels bytes containing seed data for the decoder.
	firstWord = READ_BE_UINT16(src);
	src += 2;
	if (firstWord != 0) {
		// Copy raw data
		memcpy(dst, src, firstWord);
		dst += firstWord;
		src += firstWord;
		//assert((firstWord & 1) == 0);
		outputSamplesLeft -= firstWord / 2;
	} else {
		// Read the seed values for the decoder.
		for (i = 0; i < channels; i++) {
			initialTablePos[i] = *src;
			src += 1;
			//initialimcTableEntry[i] = READ_BE_UINT32(src);
			src += 4;
			initialOutputWord[i] = READ_BE_UINT32(src);
			src += 4;
		}
	}

	totalBitOffset = 0;
	// The channels are encoded separately.
    int chan;
	for ( chan = 0; chan < channels; chan++) {
		// Read initial state (this makes it possible for the data stream
		// to be split & spread across multiple data chunks.
		curTablePos = initialTablePos[chan];
		//imcTableEntry = initialimcTableEntry[chan];
		outputWord = initialOutputWord[chan];

		// We need to interleave the channels in the output; we achieve
		// that by using a variables dest offset:
		destPos = chan * 2;

		const int bound = (channels == 1)
							? outputSamplesLeft
							: ((chan == 0)
								? (outputSamplesLeft+1) / 2
								: outputSamplesLeft / 2);
		for (i = 0; i < bound; ++i) {
			// Determine the size (in bits) of the next data packet
			const int32 curTableEntryBitCount = _destImcTable[curTablePos];
			//assert(2 <= curTableEntryBitCount && curTableEntryBitCount <= 7);

			// Read the next data packet
			const byte *readPos = src + (totalBitOffset >> 3);
			const uint16 readWord = (uint16)(READ_BE_UINT16(readPos) << (totalBitOffset & 7));
			const byte packet = (byte)(readWord >> (16 - curTableEntryBitCount));

			// Advance read position to the next data packet
			totalBitOffset += curTableEntryBitCount;

			// Decode the data packet into a delta value for the output signal.
			const byte signBitMask = (1 << (curTableEntryBitCount - 1));
			const byte dataBitMask = (signBitMask - 1);
			const byte data = (packet & dataBitMask);

			const int32 tmpA = (data << (7 - curTableEntryBitCount));
			const int32 imcTableEntry = _imaTable[curTablePos] >> (curTableEntryBitCount - 1);
			int32 delta = imcTableEntry + _destImcTable2[tmpA + (curTablePos * 64)];

			// The topmost bit in the data packet tells is a sign bit
			if ((packet & signBitMask) != 0) {
				delta = -delta;
			}

			// Accumulate the delta onto the output data
			outputWord += delta;

			// Clip outputWord to 16 bit signed, and write it into the destination stream
			outputWord = scummvm_CLIP(outputWord, -0x8000, 0x7fff);
			WRITE_BE_UINT16(dst + destPos, outputWord);
			destPos += channels << 1;

			// Adjust the curTablePos
			curTablePos += (int8)imxOtherTable[curTableEntryBitCount - 2][data];
            #define decompressADPCM_ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))
			curTablePos = scummvm_CLIP(curTablePos, 0, decompressADPCM_ARRAYSIZE(_imaTable) - 1);
		}
	}

	//return 0x2000;
    return dst - compOutput;
}



/*****************/



int MohawkBitmap__unpackRiven(byte *data, int datasz, byte *dst) {
#if defined(__GNUC__) && !defined(__clang__)
byte getLastTwoBits(byte c) {
	return (c & 0x03);
}

byte getLastThreeBits(byte c) {
	return (c & 0x07);
}

byte getLastFourBits(byte c) {
	return (c & 0x0f);
}

#define B_BYTE()				\
	*dst = *_data++;	\
	dst++

#define B_LASTDUPLET()			\
	*dst = *(dst - 2);			\
	dst++

#define B_LASTDUPLET_PLUS_M()	\
	*dst = *(dst - 2) + m;		\
	dst++

#define B_LASTDUPLET_MINUS_M()	\
	*dst = *(dst - 2) - m;		\
	dst++

#define B_LASTDUPLET_PLUS(m)	\
	*dst = *(dst - 2) + (m);	\
	dst++

#define B_LASTDUPLET_MINUS(m)	\
	*dst = *(dst - 2) - (m);	\
	dst++

#define B_PIXEL_MINUS(m)		\
	*dst = *(dst - (m));		\
	dst++

#define B_NDUPLETS(n)													\
	uint16 m1 = ((getLastTwoBits(cmd) << 8) + *_data++);		\
		for ( j = 0; j < (n); j++) {								\
			*dst = *(dst - m1);											\
			dst++;														\
		}																\
		void dummyFuncToAllowTrailingSemicolon()

void MohawkBitmap__handleRivenSubcommandStream(byte **ret_data, byte count, byte **ret_dst) {
    byte *_data = *ret_data;
    uint16 j;
    byte *dst = *ret_dst;
    byte    i;
	for ( i = 0; i < count; i++) {
		byte cmd = *_data++;
		uint16 m = getLastFourBits(cmd);
		//debug (9, "Riven Pack Subcommand %02x", cmd);

		// Notes: p = value of the next byte, m = last four bits of the command

		// Arithmetic operations
		if (cmd >= 0x01 && cmd <= 0x0f) {
			// Repeat duplet at relative position of -m duplets
			B_PIXEL_MINUS(m * 2);
			B_PIXEL_MINUS(m * 2);
		} else if (cmd == 0x10) {
			// Repeat last duplet, but set the value of the second pixel to p
			B_LASTDUPLET();
			B_BYTE();
		} else if (cmd >= 0x11 && cmd <= 0x1f) {
			// Repeat last duplet, but set the value of the second pixel to the value of the -m pixel
			B_LASTDUPLET();
			B_PIXEL_MINUS(m);
		} else if (cmd >= 0x20 && cmd <= 0x2f) {
			// Repeat last duplet, but add x to second pixel
			B_LASTDUPLET();
			B_LASTDUPLET_PLUS_M();
		} else if (cmd >= 0x30 && cmd <= 0x3f) {
			// Repeat last duplet, but subtract x from second pixel
			B_LASTDUPLET();
			B_LASTDUPLET_MINUS_M();
		} else if (cmd == 0x40) {
			// Repeat last duplet, but set the value of the first pixel to p
			B_BYTE();
			B_LASTDUPLET();
		} else if (cmd >= 0x41 && cmd <= 0x4f) {
			// Output pixel at relative position -m, then second pixel of last duplet
			B_PIXEL_MINUS(m);
			B_LASTDUPLET();
		} else if (cmd == 0x50) {
			// Output two absolute pixel values, p1 and p2
			B_BYTE();
			B_BYTE();
		} else if (cmd >= 0x51 && cmd <= 0x57) {
			// Output pixel at relative position -m, then absolute pixel value p
			// m is the last 3 bits of cmd here, not last 4
			B_PIXEL_MINUS(getLastThreeBits(cmd));
			B_BYTE();
		} else if (cmd >= 0x59 && cmd <= 0x5f) {
			// Output absolute pixel value p, then pixel at relative position -m
			// m is the last 3 bits of cmd here, not last 4
			B_BYTE();
			B_PIXEL_MINUS(getLastThreeBits(cmd));
		} else if (cmd >= 0x60 && cmd <= 0x6f) {
			// Output absolute pixel value p, then (second pixel of last duplet) + x
			B_BYTE();
			B_LASTDUPLET_PLUS_M();
		} else if (cmd >= 0x70 && cmd <= 0x7f) {
			// Output absolute pixel value p, then (second pixel of last duplet) - x
			B_BYTE();
			B_LASTDUPLET_MINUS_M();
		} else if (cmd >= 0x80 && cmd <= 0x8f) {
			// Repeat last duplet adding x to the first pixel
			B_LASTDUPLET_PLUS_M();
			B_LASTDUPLET();
		} else if (cmd >= 0x90 && cmd <= 0x9f) {
			// Output (first pixel of last duplet) + x, then absolute pixel value p
			B_LASTDUPLET_PLUS_M();
			B_BYTE();
		} else if (cmd == 0xa0) {
			// Repeat last duplet, adding first 4 bits of the next byte
			// to first pixel and last 4 bits to second
			byte pattern = *_data++;
			B_LASTDUPLET_PLUS(pattern >> 4);
			B_LASTDUPLET_PLUS(getLastFourBits(pattern));
		} else if (cmd == 0xb0) {
			// Repeat last duplet, adding first 4 bits of the next byte
			// to first pixel and subtracting last 4 bits from second
			byte pattern = *_data++;
			B_LASTDUPLET_PLUS(pattern >> 4);
			B_LASTDUPLET_MINUS(getLastFourBits(pattern));
		} else if (cmd >= 0xc0 && cmd <= 0xcf) {
			// Repeat last duplet subtracting x from first pixel
			B_LASTDUPLET_MINUS_M();
			B_LASTDUPLET();
		} else if (cmd >= 0xd0 && cmd <= 0xdf) {
			// Output (first pixel of last duplet) - x, then absolute pixel value p
			B_LASTDUPLET_MINUS_M();
			B_BYTE();
		} else if (cmd == 0xe0) {
			// Repeat last duplet, subtracting first 4 bits of the next byte
			// to first pixel and adding last 4 bits to second
			byte pattern = *_data++;
			B_LASTDUPLET_MINUS(pattern >> 4);
			B_LASTDUPLET_PLUS(getLastFourBits(pattern));
		} else if (cmd == 0xf0 || cmd == 0xff) {
			// Repeat last duplet, subtracting first 4 bits from the next byte
			// to first pixel and last 4 bits from second
			byte pattern = *_data++;
			B_LASTDUPLET_MINUS(pattern >> 4);
			B_LASTDUPLET_MINUS(getLastFourBits(pattern));

		// Repeat operations
		// Repeat n duplets from relative position -m (given in pixels, not duplets).
		// If r is 0, another byte follows and the last pixel is set to that value
		} else if (cmd >= 0xa4 && cmd <= 0xa7) {
			B_NDUPLETS(3);
			B_BYTE();
		} else if (cmd >= 0xa8 && cmd <= 0xab) {
			B_NDUPLETS(4);
		} else if (cmd >= 0xac && cmd <= 0xaf) {
			B_NDUPLETS(5);
			B_BYTE();
		} else if (cmd >= 0xb4 && cmd <= 0xb7) {
			B_NDUPLETS(6);
		} else if (cmd >= 0xb8 && cmd <= 0xbb) {
			B_NDUPLETS(7);
			B_BYTE();
		} else if (cmd >= 0xbc && cmd <= 0xbf) {
			B_NDUPLETS(8);
		} else if (cmd >= 0xe4 && cmd <= 0xe7) {
			B_NDUPLETS(9);
			B_BYTE();
		} else if (cmd >= 0xe8 && cmd <= 0xeb) {
			B_NDUPLETS(10); // 5 duplets
		} else if (cmd >= 0xec && cmd <= 0xef) {
			B_NDUPLETS(11);
			B_BYTE();
		} else if (cmd >= 0xf4 && cmd <= 0xf7) {
			B_NDUPLETS(12);
		} else if (cmd >= 0xf8 && cmd <= 0xfb) {
			B_NDUPLETS(13);
			B_BYTE();
		} else if (cmd == 0xfc) {
			byte b1 = *_data++;
			byte b2 = *_data++;
			uint16 m1 = ((getLastTwoBits(b1) << 8) + b2);

			for ( j = 0; j < ((b1 >> 3) + 1); j++) { // one less iteration
				B_PIXEL_MINUS(m1);
				B_PIXEL_MINUS(m1);
			}

			// last iteration
			B_PIXEL_MINUS(m1);

			if ((b1 & (1 << 2)) == 0) {
				B_BYTE();
			} else {
				B_PIXEL_MINUS(m1);
			}
		} else
			printf("Unknown Riven Pack Subcommand 0x%02x", cmd);
	}
    *ret_dst = dst;
    *ret_data = _data;
}

    byte    *datal = data + datasz;
    byte    *bck = dst;
    byte    i;
	while (data < datal) {
		byte cmd = *data++;
		//debug (8, "Riven Pack Command %02x", cmd);

		if (cmd == 0x00) {                       // End of stream
			break;
		} else if (cmd >= 0x01 && cmd <= 0x3f) { // Simple Pixel Duplet Output
			for ( i = 0; i < cmd; i++) {
				*dst++ = *data++;
				*dst++ = *data++;
			}
		} else if (cmd >= 0x40 && cmd <= 0x7f) { // Simple Repetition of last 2 pixels (cmd - 0x40) times
			byte pixel[] = { *(dst - 2), *(dst - 1) };

			for ( i = 0; i < (cmd - 0x40); i++) {
				*dst++ = pixel[0];
				*dst++ = pixel[1];
			}
		} else if (cmd >= 0x80 && cmd <= 0xbf) { // Simple Repetition of last 4 pixels (cmd - 0x80) times
			byte pixel[] = { *(dst - 4), *(dst - 3), *(dst - 2), *(dst - 1) };

			for ( i = 0; i < (cmd - 0x80); i++) {
				*dst++ = pixel[0];
				*dst++ = pixel[1];
				*dst++ = pixel[2];
				*dst++ = pixel[3];
			}
		} else {                                 // Subcommand Stream of (cmd - 0xc0) subcommands
			MohawkBitmap__handleRivenSubcommandStream(&data, cmd - 0xc0, &dst);
		}
	}
    return dst - bck;
#else
    return -1;
#endif
}



int MohawkBitmap__drawRLE8(byte *in, byte *out, int remaining) {
    byte    *dst = out;
		while (remaining > 0) {
			byte code = *in++;
			uint16 runLen = (code & 0x7F) + 1;

			if (runLen > remaining)
				runLen = remaining;

			if (code & 0x80) {
				byte val = *in++;
				memset(dst, val, runLen);
			} else {
				memcpy(dst, in, runLen);
                in += runLen;
			}

			dst += runLen;
			remaining -= runLen;
		}
    return dst - out;
}



/*****************/



int LzhDecompressor__decompress(byte *source, byte *dest, uint32 sourceLen, uint32 destLen) {
#if defined(__GNUC__) && !defined(__clang__)
    byte *bck = dest;

const uint BITBUFSIZ = 16;
const uint DICBIT = 13;
const uint DICSIZ = 1 << DICBIT;
//const uint MATCHBIT = 8;
const uint MAXMATCH = 256;
const uint THRESHOLD = 3;
const uint NC = 255 + MAXMATCH + 2 - THRESHOLD;
const uint CBIT = 9;
const uint CODE_BIT = 16;
const uint NP = DICBIT + 1;
const int NT = CODE_BIT + 3;
const uint PBIT = 4;
const uint TBIT = 5;
const uint NPT = NT;

	uint32 _compSize, _blockPos;

	uint16 _bitbuf;
	uint _subbitbuf;
	int _bitcount;
	uint16 _left[2 * NC - 1], _right[2 * NC - 1];
	byte _c_len[NC], _pt_len[NPT];
	uint _blocksize;
	uint16 _c_table[4096], _pt_table[256];
	int tree_n, heapsize;
	short heap[NC + 1];
	uint16 *freq, *sortptr, len_cnt[17];
	byte *len_table;

	int decode_i, decode_j;
	int count_len_depth;

	int bufsize;
	byte* buffer;

	buffer = (byte *) malloc(DICSIZ);

	byte *_source = source;
	_compSize = sourceLen;

	count_len_depth = 0;

	_blockPos = 0;

byte LzhDecompressor__readByte() {
	if (_blockPos == 0xFFE) {
		_blockPos = 0;
		_source += 2; //_source->skip(2); // skip unknown value
	}
	byte temp = *_source++;
	_blockPos++;
	return temp;
}

void LzhDecompressor__fillbuf(int count) {
	_bitbuf <<= count;
	while (count > _bitcount) {
		_bitbuf |= _subbitbuf << (count -= _bitcount);
		if (_compSize != 0) {
			_compSize--;
			_subbitbuf = LzhDecompressor__readByte();
		} else _subbitbuf = 0;
		_bitcount = 8;
	}
	_bitbuf |= _subbitbuf >> (_bitcount -= count);
}

uint LzhDecompressor__getbits(int count) {
	uint x;
	x = _bitbuf >> (BITBUFSIZ - count);
	LzhDecompressor__fillbuf(count);
	return x;
}

void LzhDecompressor__init_getbits() {
	_bitbuf = 0;
	_subbitbuf = 0;
	_bitcount = 0;
	LzhDecompressor__fillbuf(BITBUFSIZ);
}

void LzhDecompressor__huf_decode_start() {
	LzhDecompressor__init_getbits();
	_blocksize = 0;
}

void LzhDecompressor__decode_start() {
	LzhDecompressor__huf_decode_start();
	decode_j = 0;
}

void LzhDecompressor__make_table(uint nchar, byte bitlen[], uint tablebits, uint16 table[]) {
	uint16 count[17], weight[17], start[18], *p;
	uint i, k, len, ch, jutbits, avail, nextcode, mask;
	for (i = 1; i <= 16; i++) count[i] = 0;
	for (i = 0; i < nchar; i++) count[bitlen[i]]++;
	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));
	if (start[17] != (uint16)(1U << 16))
		exit(1); //error("LzhDecompressor__make_table() Bad table");
	jutbits = 16 - tablebits;
	for (i = 1; i <= tablebits; i++) {
		start[i] >>= jutbits;
		weight[i] = 1U << (tablebits - i);
	}
	for (; i <= 16; i++) {
		weight[i] = 1U << (16 - i);
	}
	i = start[tablebits + 1] >> jutbits;
	if (i != (uint16)(1U << 16)) {
		k = 1U << tablebits;
		while (i != k) table[i++] = 0;
	}
	avail = nchar;
	mask = 1U << (15 - tablebits);
	for (ch = 0; ch < nchar; ch++) {
		if ((len = bitlen[ch]) == 0) continue;
		nextcode = start[len] + weight[len];
		if (len <= tablebits) {
			for (i = start[len]; i < nextcode; i++) table[i] = ch;
		} else {
			k = start[len];
			p = &table[k >> jutbits];
			i = len - tablebits;
			while (i != 0) {
				if (*p == 0) {
					_right[avail] = _left[avail] = 0;
					*p = avail++;
				}
				if (k & mask) p = &_right[*p];
				else		  p = &_left[*p];
				k <<= 1;  i--;
			}
			*p = ch;
		}
		start[len] = nextcode;
	}
}

void LzhDecompressor__read_pt_len(int nn, int nbit, int i_special) {
	int i, c, v;
	unsigned int mask;
	v = LzhDecompressor__getbits(nbit);
	if (v == 0) {
		c = LzhDecompressor__getbits(nbit);
		for (i = 0; i < nn; i++) _pt_len[i] = 0;
		for (i = 0; i < 256; i++) _pt_table[i] = c;
	} else {
		i = 0;
		while (i < v) {
			c = _bitbuf >> (BITBUFSIZ - 3);
			if (c == 7) {
				mask = 1U << (BITBUFSIZ - 1 - 3);
				while (mask & _bitbuf) {  mask >>= 1;  c++;  }
			}
			LzhDecompressor__fillbuf((c < 7) ? 3 : c - 3);
			_pt_len[i++] = c;
			if (i == i_special) {
				c = LzhDecompressor__getbits(2);
				while (--c >= 0) _pt_len[i++] = 0;
			}
		}
		while (i < nn) _pt_len[i++] = 0;
		LzhDecompressor__make_table(nn, _pt_len, 8, _pt_table);
	}
}

void LzhDecompressor__read_c_len() {
	uint i, v;
	int c;
	unsigned int mask;
	v = LzhDecompressor__getbits(CBIT);
	if (v == 0) {
		c = LzhDecompressor__getbits(CBIT);
		for (i = 0; i < NC; i++) _c_len[i] = 0;
		for (i = 0; i < 4096; i++) _c_table[i] = c;
	} else {
		i = 0;
		while (i < v) {
			c = _pt_table[_bitbuf >> (BITBUFSIZ - 8)];
			if (c >= NT) {
				mask = 1U << (BITBUFSIZ - 1 - 8);
				do {
					if (_bitbuf & mask) c = _right[c];
					else			   c = _left [c];
					mask >>= 1;
				} while (c >= NT);
			}
			LzhDecompressor__fillbuf(_pt_len[c]);
			if (c <= 2) {
				if	  (c == 0) c = 1;
				else if (c == 1) c = LzhDecompressor__getbits(4) + 3;
				else			 c = LzhDecompressor__getbits(CBIT) + 20;
				while (--c >= 0) _c_len[i++] = 0;
			} else _c_len[i++] = c - 2;
		}
		while (i < NC) _c_len[i++] = 0;
		LzhDecompressor__make_table(NC, _c_len, 12, _c_table);
	}
}

unsigned int LzhDecompressor__decode_c() {
	uint j, mask;
	if (_blocksize == 0) {
		_blocksize = LzhDecompressor__getbits(16);
		LzhDecompressor__read_pt_len(NT, TBIT, 3);
		LzhDecompressor__read_c_len();
		LzhDecompressor__read_pt_len(NP, PBIT, -1);
	}
	_blocksize--;
	j = _c_table[_bitbuf >> (BITBUFSIZ - 12)];
	if (j >= NC) {
		mask = 1U << (BITBUFSIZ - 1 - 12);
		do {
			if (_bitbuf & mask) j = _right[j];
			else			   j = _left [j];
			mask >>= 1;
		} while (j >= NC);
	}
	LzhDecompressor__fillbuf(_c_len[j]);
	return j;
}

unsigned int LzhDecompressor__decode_p() {
	unsigned int j, mask;
	j = _pt_table[_bitbuf >> (BITBUFSIZ - 8)];
	if (j >= NP) {
		mask = 1U << (BITBUFSIZ - 1 - 8);
		do {
			if (_bitbuf & mask) j = _right[j];
			else			   j = _left [j];
			mask >>= 1;
		} while (j >= NP);
	}
	LzhDecompressor__fillbuf(_pt_len[j]);
	if (j != 0) j = (1U << (j - 1)) + LzhDecompressor__getbits(j - 1);
	return j;
}

/* call with i = root */
void LzhDecompressor__count_len(int i) {
	if (i < tree_n)
		len_cnt[(count_len_depth < 16) ? count_len_depth : 16]++;
	else {
		count_len_depth++;
		LzhDecompressor__count_len(_left [i]);
		LzhDecompressor__count_len(_right[i]);
		count_len_depth--;
	}
}

void LzhDecompressor__make_len(int root) {
	int i, k;
	uint cum;
	for (i = 0; i <= 16; i++) len_cnt[i] = 0;
	LzhDecompressor__count_len(root);
	cum = 0;
	for (i = 16; i > 0; i--)
		cum += len_cnt[i] << (16 - i);
	while (cum != (1U << 16)) {
		len_cnt[16]--;
		for (i = 15; i > 0; i--) {
			if (len_cnt[i] != 0) {
				len_cnt[i]--;
				len_cnt[i+1] += 2;
				break;
			}
		}
		cum--;
	}
	for (i = 16; i > 0; i--) {
		k = len_cnt[i];
		while (--k >= 0) len_table[*sortptr++] = i;
	}
}

void LzhDecompressor__downheap(int i) {
	int j, k;
	k = heap[i];
	while ((j = 2 * i) <= heapsize) {
		if (j < heapsize && freq[heap[j]] > freq[heap[j + 1]])
			j++;
		if (freq[k] <= freq[heap[j]]) break;
		heap[i] = heap[j];  i = j;
	}
	heap[i] = k;
}

void LzhDecompressor__make_code(int n, byte len[], uint16 code[]) {
	int	i;
	uint16 start[18];
	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = (start[i] + len_cnt[i]) << 1;
	for (i = 0; i < n; i++) code[i] = start[len[i]]++;
}

/* make tree, calculate len[], return root */
int LzhDecompressor__make_tree(int nparm, uint16 freqparm[], byte lenparm[], uint16 codeparm[]) {
	int i, j, k, avail;

	tree_n = nparm;
	freq = freqparm;
	len_table = lenparm;
	avail = tree_n;
	heapsize = 0;
	heap[1] = 0;
	for (i = 0; i < tree_n; i++) {
		len_table[i] = 0;
		if (freq[i]) heap[++heapsize] = i;
	}
	if (heapsize < 2) {
		codeparm[heap[1]] = 0;
		return heap[1];
	}
	for (i = heapsize / 2; i >= 1; i--)
		LzhDecompressor__downheap(i);  /* make priority queue */
	sortptr = codeparm;
	do {  /* while queue has at least two entries */
		i = heap[1];  /* take out least-freq entry */
		if (i < tree_n) *sortptr++ = i;
		heap[1] = heap[heapsize--];
		LzhDecompressor__downheap(1);
		j = heap[1];  /* next least-freq entry */
		if (j < tree_n) *sortptr++ = j;
		k = avail++;  /* generate new node */
		freq[k] = freq[i] + freq[j];
		heap[1] = k;
		LzhDecompressor__downheap(1);  /* put into queue */
		_left[k] = i;
		_right[k] = j;
	} while (heapsize > 1);
	sortptr = codeparm;
	LzhDecompressor__make_len(k);
	LzhDecompressor__make_code(nparm, lenparm, codeparm);
	return k;  /* return root */
}

void LzhDecompressor__decode(uint count, byte buffer[]) {
	uint r, c;
	r = 0;
	while (--decode_j >= 0) {
		buffer[r] = buffer[decode_i];
		decode_i = (decode_i + 1) & (DICSIZ - 1);
		if (++r == count) return;
	}
	for ( ; ; ) {
		c = LzhDecompressor__decode_c();
		if (c <= 255) {
			buffer[r] = c;
			if (++r == count) return;
		} else {
			decode_j = c - (255 + 1 - THRESHOLD);
			decode_i = (r - LzhDecompressor__decode_p() - 1) & (DICSIZ - 1);
			while (--decode_j >= 0) {
				buffer[r] = buffer[decode_i];
				decode_i = (decode_i + 1) & (DICSIZ - 1);
				if (++r == count) return;
			}
		}
	}
}



	LzhDecompressor__decode_start();
	while (destLen > 0) {
		bufsize = ((destLen > DICSIZ) ? DICSIZ : destLen);
		LzhDecompressor__decode(bufsize, buffer);
		memcpy(dest, buffer, bufsize);
		dest += bufsize;
		destLen -= bufsize;
	}

	free(buffer);

	return dest - bck;
#else
    return -1;
#endif
}



/*****************/



uint32 AnimationDecoder__decode_data(byte *src, int srcsz, byte *dest) {
#if defined(__GNUC__) && !defined(__clang__)
    byte *pSrc = src;
    byte *pDest = dest;

void AnimationDecoder__rcl(uint16 *value, bool *carry) {
	bool result = (*value & 0x8000) != 0;
	*value = (*value << 1) + (*carry ? 1 : 0);
	*carry = result;
}

#define AnimationDecoder__GET_BYTE currData = (currData & 0xff00) | *pSrc++
#define AnimationDecoder__BX_VAL(x) *((byte *) (dest + tableOffset + x))
#define AnimationDecoder__SET_HI_BYTE(x,v) x = (x & 0xff) | ((v) << 8);
#define AnimationDecoder__SET_LO_BYTE(x,v) x = (x & 0xff00) | (v);

void AnimationDecoder__decode_data_2(byte *src, int srcsz, byte **ret_pSrc, uint16 *ret_currData,
									 uint16 *bitCtr, uint16 *dx, bool *carry) {

    uint16 currData = *ret_currData;

    byte    *pSrc = *ret_pSrc;

	AnimationDecoder__SET_HI_BYTE(*dx, currData >> 8);

    int v;
	for ( v = 0; v < 8; ++v) {
		AnimationDecoder__rcl(&currData, carry);
		if (--*bitCtr == 0) {
			uint32 offset = (uint32) (pSrc - src);
			if (offset >= srcsz)
				// Beyond end of source, so read in a 0 value
				currData &= 0xff00;
			else
				AnimationDecoder__GET_BYTE;
			*bitCtr = 8;
		}
	}

    *ret_pSrc = pSrc;
    *ret_currData = currData;
}

	uint16 v;
	bool carry = false;
	uint16 currData, bitCtr, dx;
	byte tableOffset;
	uint16 tempReg1, tempReg2;

	// Handle splitting up 16 bytes into individual nibbles
    int numBytes;
	for ( numBytes = 0; numBytes < 16; ++numBytes, ++pDest) {
		// Split up next byte to pDest and pDest+0x10
		currData = *pSrc++;
		*(pDest + 0x10) = currData & 0xf;
		*pDest = (currData >> 4) & 0xf;

		// Split up next byte to pDest+0x20 and pDest+0x30
		currData = *pSrc++;
		*(pDest + 0x30) = currData & 0xf;
		*(pDest + 0x20) = (currData >> 4) & 0xf;
	}

	pDest += 0x40; //pDest = (byte *) (dest + 0x40);
	currData = READ_BE_UINT16(pSrc);
	pSrc += sizeof(uint16);

	bitCtr = 4;
	*pDest = (currData >> 8) & 0xf0;
	tableOffset = currData >> 12;
	currData <<= 4;
	dx = 1;

	// Main loop
	bool loopFlag = true;
	while (loopFlag) {
		for (;;) {
			carry = false;
			AnimationDecoder__rcl(&currData, &carry);
			if (--bitCtr == 0) {
				AnimationDecoder__GET_BYTE;
				bitCtr = 8;
			}
			if (!carry) {
				tableOffset = AnimationDecoder__BX_VAL(0);
				break;
			}

			AnimationDecoder__rcl(&currData, &carry);
			if (--bitCtr == 0) {
				AnimationDecoder__GET_BYTE;
				bitCtr = 8;
			}
			if (!carry) {
				AnimationDecoder__rcl(&currData, &carry);
				if (--bitCtr == 0) {
					AnimationDecoder__GET_BYTE;
					bitCtr = 8;
				}

				if (!carry) {
					tableOffset = AnimationDecoder__BX_VAL(0x10);
				} else {
					tableOffset = AnimationDecoder__BX_VAL(0x20);
				}
				break;
			}

			AnimationDecoder__rcl(&currData, &carry);
			if (--bitCtr == 0) {
				AnimationDecoder__GET_BYTE;
				bitCtr = 8;
			}
			if (!carry) {
				tableOffset = AnimationDecoder__BX_VAL(0x30);
				break;
			}

			AnimationDecoder__SET_HI_BYTE(dx, currData >> 12);
			carry = false;
            int ctr;
			for (ctr = 0; ctr < 4; ++ctr) {
				AnimationDecoder__rcl(&currData, &carry);
				if (--bitCtr == 0) {
					AnimationDecoder__GET_BYTE;
					bitCtr = 8;
				}
			}

			byte dxHigh = dx >> 8;
			if (dxHigh == AnimationDecoder__BX_VAL(0)) {
				tempReg1 = bitCtr;
				tempReg2 = dx;
				AnimationDecoder__decode_data_2(src, srcsz, &pSrc, &currData, &bitCtr, &dx, &carry);

				AnimationDecoder__SET_LO_BYTE(dx, dx >> 8);
				AnimationDecoder__decode_data_2(src, srcsz, &pSrc, &currData, &bitCtr, &dx, &carry);
				AnimationDecoder__SET_HI_BYTE(bitCtr, dx & 0xff);
				AnimationDecoder__SET_LO_BYTE(bitCtr, dx >> 8);
				dx = tempReg2;

				if (bitCtr == 0) {
					// End of decompression
					loopFlag = false;
					break;
				}
			} else if (dxHigh == AnimationDecoder__BX_VAL(0x10)) {
				tempReg1 = bitCtr;
				AnimationDecoder__decode_data_2(src, srcsz, &pSrc, &currData, &bitCtr, &dx, &carry);
				bitCtr = dx >> 8;

			} else if (dxHigh == AnimationDecoder__BX_VAL(0x20)) {
				AnimationDecoder__SET_HI_BYTE(dx, currData >> 10);

				for (v = 0; v < 6; ++v) {
					AnimationDecoder__rcl(&currData, &carry);
					if (--bitCtr == 0) {
						AnimationDecoder__GET_BYTE;
						bitCtr = 8;
					}
				}

				tempReg1 = bitCtr;
				bitCtr = dx >> 8;

			} else if (dxHigh == AnimationDecoder__BX_VAL(0x30)) {
				AnimationDecoder__SET_HI_BYTE(dx, currData >> 11);

				for (v = 0; v < 5; ++v) {
					AnimationDecoder__rcl(&currData, &carry);
					if (--bitCtr == 0) {
						AnimationDecoder__GET_BYTE;
						bitCtr = 8;
					}
				}

				tempReg1 = bitCtr;
				bitCtr = dx >> 8;

			} else {
				tableOffset = dx >> 8;
				break;
			}

			if ((dx & 1) == 1) {
				*pDest++ |= tableOffset;
				--bitCtr;
				dx &= 0xfffe;
			}

			AnimationDecoder__SET_HI_BYTE(dx, tableOffset << 4);
			tableOffset |= dx >> 8;

			v = bitCtr >> 1;
			while (v-- > 0) *pDest++ = tableOffset;

			bitCtr &= 1;
			if (bitCtr != 0) {
				*pDest = tableOffset & 0xf0;
				dx |= 1; //dx.l
			}

			bitCtr = tempReg1;
			tableOffset &= 0x0f;
		}

		if (loopFlag) {
			dx ^= 1;
			if ((dx & 1) != 0) {
				AnimationDecoder__SET_HI_BYTE(dx, tableOffset << 4);
				*pDest = dx >> 8;
			} else {
				*pDest++ |= tableOffset;
			}
		}
	}

	// Return number of bytes written
	return pDest - dest;
#else
    return -1;
#endif
}



/*****************/



int MusicPlayerMac_t7g__decompressMidi(byte *stream, int streamsz, byte *output, int size) {
    byte *streaml = stream + streamsz;

	// Initialize an output buffer of the given size

	byte *current = output;
	uint32 decompBytes = 0;
	while ((decompBytes < size) && (stream < streaml)) {
		// 8 flags
		byte flags = *stream++;

        byte i;
		for ( i = 0; (i < 8) && (stream < streaml); i++) {
			if (flags & 1) {
				// 1: Next byte is a literal
				*(current++) = *stream++;
				if (stream >= streaml)
					continue;
				decompBytes++;
			} else {
				// 0: It's a reference to part of the history
				uint16 args = (stream[0] << 8) | stream[1]; stream += 2; //stream->readUint16BE();
				if (stream >= streaml)
					continue;

				// Length = 4bit unsigned (3 minimal)
				uint8 length = (args >> 12) + 3;

				// Offset = 12bit signed (all values are negative)
				int16 offset = (args & 0xFFF) | 0xF000;

				// Copy from the past decompressed bytes
				decompBytes += length;
				while (length > 0) {
					*(current) = *(current + offset);
					current++;
					length--;
				}
			}
			flags = flags >> 1;
		}
	}

	return decompBytes;
}



/*****************/



int StuffItArchive__decompress14(byte *src, int srcsz, byte *dst, uint32 uncompressedSize) {
#if defined(__GNUC__) && !defined(__clang__)
    int isMSB2LSB = 0;
    int isLE = 0;

	uint32 _value;   ///< Current value.
	uint8  _inValue; ///< Position within the current value.
    int valueBits = 0;

    byte    *_stream;

	/** Read a data value. */
	uint32 StuffItArchive__readData() {
        uint32  tmp;
		if (isLE) {
			if (valueBits ==  8)
				{ tmp = *_stream; _stream += 1; return tmp; }  //_stream->readByte();
			if (valueBits == 16)
				{ tmp = READ_LE_UINT16(_stream); _stream += 2; return tmp; } //_stream->readUint16LE();
			if (valueBits == 32)
				{ tmp = READ_LE_UINT32(_stream); _stream += 4; return tmp; } //_stream->readUint32LE();
		} else {
			if (valueBits ==  8)
				{ tmp = *_stream; _stream += 1; return tmp; }  //_stream->readByte();
			if (valueBits == 16)
				{ tmp = READ_BE_UINT16(_stream); _stream += 2; return tmp; } //_stream->readUint16BE();
			if (valueBits == 32)
				{ tmp = READ_BE_UINT32(_stream); _stream += 4; return tmp; } //_stream->readUint32BE();
		}

		//assert(false);
		return 0;
	}

	/** Read the next data value. */
	void StuffItArchive__readValue() {
		//if ((size() - pos()) < valueBits)
			//error("BitStreamImpl__readValue(): End of bit stream reached");

		_value = StuffItArchive__readData();
		//if (_stream->err() || _stream->eos())
			//error("BitStreamImpl__readValue(): Read error");

		// If we're reading the bits MSB first, we need to shift the value to that position
		if (isMSB2LSB)
			_value <<= 32 - valueBits;
		}

	/** Read a bit from the bit stream. */
	uint32 StuffItArchive__getBit() {
		// Check if we need the next value
		if (_inValue == 0)
			StuffItArchive__readValue();

		// Get the current bit
		int b = 0;
		if (isMSB2LSB)
			b = ((_value & 0x80000000) == 0) ? 0 : 1;
		else
			b = ((_value & 1) == 0) ? 0 : 1;

		// Shift to the next bit
		if (isMSB2LSB)
			_value <<= 1;
		else
			_value >>= 1;

		// Increase the position within the current value
		_inValue = (_inValue + 1) % valueBits;

		return b;
	}

	/**
	 * Read a multi-bit value from the bit stream.
	 *
	 * The value is read as if just taken as a whole from the bitstream.
	 *
	 * For example:
	 * Reading a 4-bit value from an 8-bit bitstream with the contents 01010011:
	 * If the bitstream is MSB2LSB, the 4-bit value would be 0101.
	 * If the bitstream is LSB2MSB, the 4-bit value would be 0011.
	 */
	uint32 StuffItArchive__getBits(uint8 n) {
		if (n == 0)
			return 0;

		if (n > 32)
			exit(1); //error("BitStreamImpl__getBits(): Too many bits requested to be read");

		// Read the number of bits
		uint32 v = 0;

		if (isMSB2LSB) {
			while (n-- > 0)
				v = (v << 1) | StuffItArchive__getBit();
		} else {
            uint32 i;
			for ( i = 0; i < n; i++)
				v = (v >> 1) | (((uint32) StuffItArchive__getBit()) << 31);

			v >>= (32 - n);
		}

		return v;
	}

void StuffItArchive__update14(uint16 first, uint16 last, byte *code, uint16 *freq) {
    #define StuffItArchive__SWAP(X,Y) { int tmp; tmp = X; X = Y; Y = tmp; }

	uint16 i, j;

	while (last - first > 1) {
		i = first;
		j = last;

		do {
			while (++i < last && code[first] > code[i])
				;

			while (--j > first && code[first] < code[j])
				;

			if (j > i) {
				StuffItArchive__SWAP(code[i], code[j]);
				StuffItArchive__SWAP(freq[i], freq[j]);
			}
		} while (j > i);

		if (first != j) {
			StuffItArchive__SWAP(code[first], code[j]);
			StuffItArchive__SWAP(freq[first], freq[j]);

			i = j + 1;

			if (last - i <= j - first) {
				StuffItArchive__update14(i, last, code, freq);
				last = j;
			} else {
				StuffItArchive__update14(first, j, code, freq);
				first = i;
			}
		} else {
			++first;
		}
	}
}

typedef struct {
	byte code[308];
	byte codecopy[308];
	uint16 freq[308];
	uint32 buff[308];

	byte var1[52];
	uint16 var2[52];
	uint16 var3[75 * 2];

	byte var4[76];
	uint32 var5[75];
	byte var6[1024];
	uint16 var7[308 * 2];
	byte var8[0x4000];

	byte window[0x40000];
} SIT14Data;

// Realign to a byte boundary
#define STUFFIT_ALIGN_BITS(b) \
    if(_inValue) _inValue = 0;

    /*
	if (b->pos() & 7) \
		b->skip(8 - (b->pos() & 7))
    */

void StuffItArchive__readTree14(byte *bits, SIT14Data *dat, uint16 codesize, uint16 *result) {
	uint32 i, l, n;
	uint32 k = StuffItArchive__getBit();
	uint32 j = StuffItArchive__getBits(2) + 2;
	uint32 o = StuffItArchive__getBits(3) + 1;
	uint32 size = 1 << j;
	uint32 m = size - 1;
	k = k ? (m - 1) : 0xFFFFFFFF;

	if (StuffItArchive__getBits(2) & 1) { // skip 1 bit!
		// requirements for this call: dat->buff[32], dat->code[32], dat->freq[32*2]
		StuffItArchive__readTree14(bits, dat, size, dat->freq);

		for (i = 0; i < codesize; ) {
			l = 0;

			do {
				l = dat->freq[l + StuffItArchive__getBit()];
				n = size << 1;
			} while (n > l);

			l -= n;

			if (k != l) {
				if (l == m) {
					l = 0;

					do {
						l = dat->freq[l + StuffItArchive__getBit()];
						n = size <<  1;
					} while (n > l);

					l += 3 - n;

					while (l--) {
						dat->code[i] = dat->code[i - 1];
						++i;
					}
				} else {
					dat->code[i++] = l + o;
				}
			} else {
				dat->code[i++] = 0;
			}
		}
	} else {
		for (i = 0; i < codesize; ) {
			l = StuffItArchive__getBits(j);

			if (k != l) {
				if  (l == m) {
					l = StuffItArchive__getBits(j) + 3;

					while (l--) {
						dat->code[i] = dat->code[i - 1];
						++i;
					}
				} else {
					dat->code[i++] = l+o;
				}
			} else {
				dat->code[i++] = 0;
			}
		}
	}

	for (i = 0; i < codesize; ++i) {
		dat->codecopy[i] = dat->code[i];
		dat->freq[i] = i;
	}

	StuffItArchive__update14(0, codesize, dat->codecopy, dat->freq);

	for (i = 0; i < codesize && !dat->codecopy[i]; ++i)
		; // find first nonempty

	for (j = 0; i < codesize; ++i, ++j) {
		if (i)
			j <<= (dat->codecopy[i] - dat->codecopy[i - 1]);

		k = dat->codecopy[i]; m = 0;

		for (l = j; k--; l >>= 1)
			m = (m << 1) | (l & 1);

		dat->buff[dat->freq[i]] = m;
	}

	for (i = 0; i < (uint32)codesize * 2; ++i)
		result[i] = 0;

	j = 2;

	for (i = 0; i < codesize; ++i) {
		l = 0;
		m = dat->buff[i];

		for (k = 0; k < dat->code[i]; ++k) {
			l += (m & 1);

			if (dat->code[i] - 1 <= (int32)k) {
				result[l] = codesize * 2 + i;
			} else {
				if (!result[l]) {
					result[l] = j;
					j += 2;
				}

				l = result[l];
			}

			m >>= 1;
		}
	}

	STUFFIT_ALIGN_BITS(bits);
}

#define StuffItArchive__OUTPUT_VAL(x) \
	*dst++ = x; \
	dat->window[j++] = x; \
	j &= 0x3FFFF


	byte *bits = src;
    _stream = src;

	uint32 i, j, k, l, m, n;

	SIT14Data *dat = calloc(1, sizeof(SIT14Data));

	// initialization
	for (i = k = 0; i < 52; ++i) {
		dat->var2[i] = k;
		k += (1 << (dat->var1[i] = ((i >= 4) ? ((i - 4) >> 2) : 0)));
	}

	for (i = 0; i < 4; ++i)
		dat->var8[i] = i;

	for (m = 1, l = 4; i < 0x4000; m <<= 1) // i is 4
		for (n = l+4; l < n; ++l)
			for (j = 0; j < m; ++j)
				dat->var8[i++] = l;

	for (i = 0, k = 1; i < 75; ++i) {
		dat->var5[i] = k;
		k += (1 << (dat->var4[i] = (i >= 3 ? ((i - 3) >> 2) : 0)));
	}

	for (i = 0; i < 4; ++i)
		dat->var6[i] = i - 1;

	for (m = 1, l = 3; i < 0x400; m <<= 1) // i is 4
		for (n = l + 4; l < n; ++l)
			for (j = 0; j < m; ++j)
				dat->var6[i++] = l;

	m = StuffItArchive__getBits(16); // number of blocks
	j = 0; // window position

    byte *StuffItArchive__eos = src + srcsz;

	while (m-- && (_stream < StuffItArchive__eos)) {
		StuffItArchive__getBits(16); // skip crunched block size
		StuffItArchive__getBits(16);
		n = StuffItArchive__getBits(16); // number of uncrunched bytes
		n |= StuffItArchive__getBits(16) << 16;
		StuffItArchive__readTree14(bits, dat, 308, dat->var7);
		StuffItArchive__readTree14(bits, dat, 75, dat->var3);

		while (n && (_stream < StuffItArchive__eos)) {
			for (i = 0; i < 616;)
				i = dat->var7[i + StuffItArchive__getBit()];

			i -= 616;

			if (i < 0x100) {
				StuffItArchive__OUTPUT_VAL(i);
				--n;
			} else {
				i -= 0x100;
				k = dat->var2[i]+4;
				i = dat->var1[i];

				if (i)
					k += StuffItArchive__getBits(i);

				for (i = 0; i < 150;)
					i = dat->var3[i + StuffItArchive__getBit()];

				i -= 150;
				l = dat->var5[i];
				i = dat->var4[i];

				if (i)
					l += StuffItArchive__getBits(i);

				n -= k;
				l = j + 0x40000 - l;

				while (k--) {
					l &= 0x3FFFF;
					StuffItArchive__OUTPUT_VAL(dat->window[l]);
					l++;
				}
			}
		}

		STUFFIT_ALIGN_BITS(bits);
	}

	free(dat);

	return uncompressedSize;
#else
    return -1;
#endif
}



/*****************/



int decompressIconPlanar(byte *icon_pln, int outsz, byte *src) {
	byte *i, *o;
	byte x;

		// Decode RLE planar icon data
		i = src;
		o = icon_pln;
		while (o < (icon_pln + outsz)) {
			x = *i++;
			if (x < 128) {
				do {
					*o++ = *i++;
					*o++ = *i++;
					*o++ = *i++;
				} while (x-- > 0);
			} else {
				x = 256 - x;
				do {
					*o++ = i[0];
					*o++ = i[1];
					*o++ = i[2];
				} while (x-- > 0);
				i += 3;
			}
		}
    return o - icon_pln;
}




/*****************/



int DrasculaEngine__decodeRLE(byte* srcPtr, byte* dstPtr/*, uint16 pitch*/) {
    byte *bck = dstPtr;
    uint16 pitch = 320;

	bool stopProcessing = false;
	byte pixel;
	uint repeat;
	int curByte = 0, curLine = 0;
	pitch -= 320;

	while (!stopProcessing) {
		pixel = *srcPtr++;
		repeat = 1;
		if ((pixel & 192) == 192) {
			repeat = (pixel & 63);
			pixel = *srcPtr++;
		}
        uint j;
		for (j = 0; j < repeat; j++) {
			*dstPtr++ = pixel;
			if (++curByte >= 320) {
				curByte = 0;
				dstPtr += pitch;
				if (++curLine >= 200) {
					stopProcessing = true;
					break;
				}
			}
		}
	}
    return dstPtr - bck;
}



/*****************/



int bompDecodeLine(byte *dst, const byte *src, int len) {
    byte *bck = dst;
	//assert(len > 0);

	int num;
	byte code, color;

	while (len > 0) {
		code = *src++;
		num = (code >> 1) + 1;
		if (num > len)
			num = len;
		len -= num;
		if (code & 1) {
			color = *src++;
			memset(dst, color, num);
		} else {
			memcpy(dst, src, num);
			src += num;
		}
		dst += num;
	}
    return dst - bck;
}

int bompDecodeLineReverse(byte *dst, const byte *src, int len) {
    byte *bck = dst;
	//assert(len > 0);

	dst += len;

	int num;
	byte code, color;

	while (len > 0) {
		code = *src++;
		num = (code >> 1) + 1;
		if (num > len)
			num = len;
		len -= num;
		dst -= num;
		if (code & 1) {
			color = *src++;
			memset(dst, color, num);
		} else {
			memcpy(dst, src, num);
			src += num;
		}
	}
    return dst - bck;
}



/*****************/



int ToucheEngine__res_decodeScanLineImageRLE(uint8 *_fData, uint8 *dst, int lineWidth) {
	int w = 0;
	while (w < lineWidth) {
		uint8 code = *_fData++;
		if ((code & 0xC0) == 0xC0) {
			int len = code & 0x3F;
			uint8 color = *_fData++;
			memset(dst, color, len);
			dst += len;
			w += len;
		} else {
			*dst = code;
			++dst;
			++w;
		}
	}
    return w;
}



/*****************/



int AnimationPlayer__rleDecode(const byte *pSrc, byte *pDest, int size) {
    byte *bck = pDest;
	while (size > 0) {
		byte v = *pSrc++;
		if (!(v & 0x80)) {
			// Following uncompressed set of bytes
			memcpy(pDest, pSrc, v);
			pSrc += v;
			pDest += v;
			size -= v;
		} else {
			int count = v & 0x3F;
			size -= count;

			if (!(v & 0x40)) {
				// Skip over a number of bytes
				pDest += count;
			} else {
				// Replicate a number of bytes
				memset(pDest, *pSrc++, count);
				pDest += count;
			}
		}
	}
    return pDest - bck;
}



/*****************/



int Graphics__decodeRLE(uint8 *dst, int size, const uint8 *src) {
	int code = 0;
	int color = 0;
    int x;
	for(x = 0; x < size; x++) {
			if (code == 0) {
				color = *src++;
				if (color == 0) {
					code = *src++;
				}
			}
			if (color != 0) {
				dst[x] = color;
			} else {
				--code;
			}
    }
    return x;
}



/*****************/



int MSRLEDecoder__decode8(byte *stream, int streamsz, byte *data, int size) {
    byte *streaml = stream + streamsz;
    int i;

	int x = 0;

	byte *output     = data;
	byte *output_end = data + size;

	while (stream < streaml) {
		byte count = *stream++;
		byte value = *stream++;

		if (count == 0) {
			if (value == 0) {
				// End of line

				x = 0;

			} else if (value == 1) {
				// End of image

				return output - data;
			} else if (value == 2) {
				// Skip

				count = *stream++;
				value = *stream++;

				x += count;

			} else {
				// Copy data

				if (output + value > output_end) {
                    stream += value;
					continue;
				}

				for ( i = 0; i < value; i++)
					*output++ = *stream++;

				if (value & 1)
					stream += 1;

				x += value;
			}

		} else {
			// Run data

			if (output + count > output_end)
				continue;

			for ( i = 0; i < count; i++, x++)
				*output++ = value;
		}

	}

	return -1;
}



/*****************/



int unarj(byte *_compressed, int _compressed_size, byte *_outstream, int _outstream_size, int method4) {
#if defined(__GNUC__) && !defined(__clang__)
#define ARJ_CBIT		 9
#define ARJ_PBIT		 5
#define ARJ_TBIT		 5

#define ARJ_UCHAR_MAX 255
#define ARJ_CHAR_BIT 8

#define ARJ_COMMENT_MAX 2048
#define ARJ_FILENAME_MAX 512

#define ARJ_CODE_BIT 16
#define ARJ_THRESHOLD 3
#define ARJ_DICSIZ 26624
#define ARJ_FDICSIZ ARJ_DICSIZ
#define ARJ_MAXDICBIT   16
#define ARJ_MAXMATCH   256
#define ARJ_NC (ARJ_UCHAR_MAX + ARJ_MAXMATCH + 2 - ARJ_THRESHOLD)
#define ARJ_NP (ARJ_MAXDICBIT + 1)
#define ARJ_NT (ARJ_CODE_BIT + 3)

#if ARJ_NT > ARJ_NP
#define ARJ_NPT ARJ_NT
#else
#define ARJ_NPT ARJ_NP
#endif

#define ARJ_CTABLESIZE 4096
#define ARJ_PTABLESIZE 256

	uint16 _bitbuf;
	uint16 _bytebuf;
	int32 _compsize;
	//byte _subbitbuf;
	int _bitcount;

	byte  _ntext[ARJ_FDICSIZ];

	//int16  _getlen;
	//int16  _getbuf;

	uint16 _left[2 * ARJ_NC - 1];
	uint16 _right[2 * ARJ_NC - 1];
	byte  _c_len[ARJ_NC];
	byte  _pt_len[ARJ_NPT];

	uint16 _c_table[ARJ_CTABLESIZE];
	uint16 _pt_table[ARJ_PTABLESIZE];
	uint16 _blocksize;

// Source for fillbuf, getbits: decode.c
void ArjDecoder__fillbuf(int n) {
	while (_bitcount < n) {
		_bitbuf = (_bitbuf << _bitcount) | (_bytebuf >> (8 - _bitcount));
		n -= _bitcount;
		if (_compsize > 0) {
			_compsize--;
			_bytebuf = *_compressed++;
		} else {
			_bytebuf = 0;
		}
		_bitcount = 8;
	}
	_bitcount -= n;
	_bitbuf = ( _bitbuf << n) | (_bytebuf >> (8-n));
	_bytebuf <<= n;
}

// Source for init_getbits: arj_file.c (decode_start_stub)
void ArjDecoder__init_getbits() {
	_bitbuf = 0;
	_bytebuf = 0;
	_bitcount = 0;
	ArjDecoder__fillbuf(ARJ_CHAR_BIT * 2);
}

// Reads a series of bits into the input buffer */
uint16 ArjDecoder__getbits(int n) {
	uint16 rc;

	rc = _bitbuf >> (ARJ_CODE_BIT - n);
	ArjDecoder__fillbuf(n);
	return rc;
}

// Huffman decode routines
// Source: decode.c

// Creates a table for decoding
void ArjDecoder__make_table(int nchar, byte *bitlen, int tablebits, uint16 *table, int tablesize) {
	uint16 count[17], weight[17], start[18];
	uint16 *p;
	uint i, k, len, ch, jutbits, avail, nextcode, mask;

	for (i = 1; i <= 16; i++)
		count[i] = 0;
	for (i = 0; (int)i < nchar; i++)
		count[bitlen[i]]++;

	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));
	if (start[17] != (uint16) (1 << 16))
		exit(1); //error("ArjDecoder__make_table(): bad file data");

	jutbits = 16 - tablebits;
	for (i = 1; (int)i <= tablebits; i++) {
		start[i] >>= jutbits;
		weight[i] = 1 << (tablebits - i);
	}
	while (i <= 16) {
		weight[i] = 1 << (16 - i);
		i++;
	}

	i = start[tablebits + 1] >> jutbits;
	if (i != (uint16) (1 << 16)) {
		k = 1 << tablebits;
		while (i != k)
			table[i++] = 0;
	}

	avail = nchar;
	mask = 1 << (15 - tablebits);
	for (ch = 0; (int)ch < nchar; ch++) {
		if ((len = bitlen[ch]) == 0)
			continue;
		k = start[len];
		nextcode = k + weight[len];
		if ((int)len <= tablebits) {
			if (nextcode > (uint)tablesize)
				exit(1); //error("ArjDecoder__make_table(): bad file data");
			for (i = start[len]; i < nextcode; i++)
				table[i] = ch;
		} else {
			p = &table[k >> jutbits];
			i = len - tablebits;
			while (i != 0) {
				if (*p == 0) {
					_right[avail] = _left[avail] = 0;
					*p = avail;
					avail++;
				}
				if (k & mask)
					p = &_right[*p];
				else
					p = &_left[*p];
				k <<= 1;
				i--;
			}
			*p = ch;
		}
		start[len] = nextcode;
	}
}

// Reads length of data pending
void ArjDecoder__read_pt_len(int nn, int nbit, int i_special) {
	int i, n;
	int16 c;
	uint16 mask;

	n = ArjDecoder__getbits(nbit);
	if (n == 0) {
		c = ArjDecoder__getbits(nbit);
		for (i = 0; i < nn; i++)
			_pt_len[i] = 0;
		for (i = 0; i < 256; i++)
			_pt_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = _bitbuf >> 13;
			if (c == 7) {
				mask = 1 << 12;
				while (mask & _bitbuf) {
					mask >>= 1;
					c++;
				}
			}
			ArjDecoder__fillbuf((c < 7) ? 3 : (int)(c - 3));
			_pt_len[i++] = (byte)c;
			if (i == i_special) {
				c = ArjDecoder__getbits(2);
				while (--c >= 0)
					_pt_len[i++] = 0;
			}
		}
		while (i < nn)
			_pt_len[i++] = 0;
		ArjDecoder__make_table(nn, _pt_len, 8, _pt_table, ARJ_PTABLESIZE);
	}
}

// Reads a character table
void ArjDecoder__read_c_len() {
	int16 i, c, n;
	uint16 mask;

	n = ArjDecoder__getbits(ARJ_CBIT);
	if (n == 0) {
		c = ArjDecoder__getbits(ARJ_CBIT);
		for (i = 0; i < ARJ_NC; i++)
			_c_len[i] = 0;
		for (i = 0; i < ARJ_CTABLESIZE; i++)
			_c_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = _pt_table[_bitbuf >> (8)];
			if (c >= ARJ_NT) {
				mask = 1 << 7;
				do {
					if (_bitbuf & mask)
						c = _right[c];
					else
						c = _left[c];
					mask >>= 1;
				} while (c >= ARJ_NT);
			}
			ArjDecoder__fillbuf((int)(_pt_len[c]));
			if (c <= 2) {
				if (c == 0)
					c = 1;
				else if (c == 1) {
					c = ArjDecoder__getbits(4);
					c += 3;
				} else {
					c = ArjDecoder__getbits(ARJ_CBIT);
					c += 20;
				}
				while (--c >= 0)
					_c_len[i++] = 0;
			}
			else
				_c_len[i++] = (byte)(c - 2);
		}
		while (i < ARJ_NC)
			_c_len[i++] = 0;
		ArjDecoder__make_table(ARJ_NC, _c_len, 12, _c_table, ARJ_CTABLESIZE);
	}
}

// Decodes a single character
uint16 ArjDecoder__decode_c() {
	uint16 j, mask;

	if (_blocksize == 0) {
		_blocksize = ArjDecoder__getbits(ARJ_CODE_BIT);
		ArjDecoder__read_pt_len(ARJ_NT, ARJ_TBIT, 3);
		ArjDecoder__read_c_len();
		ArjDecoder__read_pt_len(ARJ_NP, ARJ_PBIT, -1);
	}
	_blocksize--;
	j = _c_table[_bitbuf >> 4];
	if (j >= ARJ_NC) {
		mask = 1 << 3;
		do {
			if (_bitbuf & mask)
				j = _right[j];
			else
				j = _left[j];
			mask >>= 1;
		} while (j >= ARJ_NC);
	}
	ArjDecoder__fillbuf((int)(_c_len[j]));
	return j;
}

// Decodes a control character
uint16 ArjDecoder__decode_p() {
	uint16 j, mask;

	j = _pt_table[_bitbuf >> 8];
	if (j >= ARJ_NP) {
		mask = 1 << 7;
		do {
			if (_bitbuf & mask)
				j = _right[j];
			else
				j = _left[j];
			mask >>= 1;
		} while (j >= ARJ_NP);
	}
	ArjDecoder__fillbuf((int)(_pt_len[j]));
	if (j != 0) {
		j--;
		j = (1 << j) + ArjDecoder__getbits((int)j);
	}
	return j;
}

// Initializes memory for decoding
void ArjDecoder__decode_start() {
	_blocksize = 0;
	ArjDecoder__init_getbits();
}

// Decodes the entire file
void ArjDecoder__decode(int32 origsize) {
	int16 i;
	int16 r;
	int16 c;
	int16 j;
	int32 count;

	ArjDecoder__decode_start();
	count = origsize;
	r = 0;

	while (count > 0) {
		if ((c = ArjDecoder__decode_c()) <= ARJ_UCHAR_MAX) {
			_ntext[r] = (byte) c;
			count--;
			if (++r >= ARJ_DICSIZ) {
				r = 0;
				memcpy(_outstream, _ntext, ARJ_DICSIZ);
                _outstream += ARJ_DICSIZ;
			}
		} else {
			j = c - (ARJ_UCHAR_MAX + 1 - ARJ_THRESHOLD);
			count -= j;
			i = r - ArjDecoder__decode_p() - 1;
			if (i < 0)
				i += ARJ_DICSIZ;
			if (r > i && r < ARJ_DICSIZ - ARJ_MAXMATCH - 1) {
				while (--j >= 0)
					_ntext[r++] = _ntext[i++];
			} else {
				while (--j >= 0) {
					_ntext[r] = _ntext[i];
					if (++r >= ARJ_DICSIZ) {
						r = 0;
						memcpy(_outstream, _ntext, ARJ_DICSIZ);
                        _outstream += ARJ_DICSIZ;
					}
					if (++i >= ARJ_DICSIZ)
						i = 0;
				}
			}
		}
	}
	if (r > 0) {
        memcpy(_outstream, _ntext, r);
        _outstream += r;
    }
}

// Backward pointer decoding
int16 ArjDecoder__decode_ptr() {
	int16 c = 0;
	int16 width;
	int16 plus;
	int16 pwr;

	plus = 0;
	pwr = 1 << 9;
	for (width = 9; width < 13; width++) {
		c = ArjDecoder__getbits(1);
		if (c == 0)
			break;
		plus += pwr;
		pwr <<= 1;
	}
	if (width != 0)
		c = ArjDecoder__getbits(width);
	c += plus;
	return c;
}

// Reference length decoding
int16 ArjDecoder__decode_len() {
	int16 c = 0;
	int16 width;
	int16 plus;
	int16 pwr;

	plus = 0;
	pwr = 1;
	for (width = 0; width < 7; width++) {
		c = ArjDecoder__getbits(1);
		if (c == 0)
			break;
		plus += pwr;
		pwr <<= 1;
	}
	if (width != 0)
		c = ArjDecoder__getbits(width);
	c += plus;
	return c;
}

// Decodes the entire file, using method 4
void ArjDecoder__decode_f(int32 origsize) {
	int16 i;
	int16 j;
	int16 c;
	int16 r;
	uint32 ncount;

	ArjDecoder__init_getbits();
	ncount = 0;
	//_getlen = _getbuf = 0;
	r = 0;

	while (ncount < (uint32)origsize) {
		c = ArjDecoder__decode_len();
		if (c == 0) {
			ncount++;
			_ntext[r] = (byte)ArjDecoder__getbits(8);
			if (++r >= ARJ_FDICSIZ) {
				r = 0;
                memcpy(_outstream, _ntext, ARJ_FDICSIZ);
                _outstream += ARJ_FDICSIZ;
			}
		} else {
			j = c - 1 + ARJ_THRESHOLD;
			ncount += j;
			if ((i = r - ArjDecoder__decode_ptr() - 1) < 0)
				i += ARJ_FDICSIZ;
			while (j-- > 0) {
				_ntext[r] = _ntext[i];
				if (++r >= ARJ_FDICSIZ) {
					r = 0;
                    memcpy(_outstream, _ntext, ARJ_FDICSIZ);
                    _outstream += ARJ_FDICSIZ;
				}
				if (++i >= ARJ_FDICSIZ)
					i = 0;
			}
		}
	}
	if (r != 0) {
        memcpy(_outstream, _ntext, r);
        _outstream += r;
    }
}


    byte    *bck = _outstream;

    if(method4) {
        ArjDecoder__decode_f(_outstream_size);
    } else {
        ArjDecoder__decode(_outstream_size);
    }

    return _outstream - bck;

#else
    return -1;
#endif
}




/*****************/



    static int FabDecompressor__bitsLeft;
    static uint32 FabDecompressor__bitBuffer;
	static const byte *FabDecompressor__srcData, *FabDecompressor__srcP;
	static int FabDecompressor__srcSize;

static int FabDecompressor__getBit() {
	FabDecompressor__bitsLeft--;
	if (FabDecompressor__bitsLeft == 0) {
		if (FabDecompressor__srcP - FabDecompressor__srcData == FabDecompressor__srcSize)
			return -1; //error(FabInputExceededError);

		FabDecompressor__bitBuffer = (READ_LE_UINT16(FabDecompressor__srcP) << 1) | (FabDecompressor__bitBuffer & 1);
		FabDecompressor__srcP += 2;
		FabDecompressor__bitsLeft = 16;
	}

	int bit = FabDecompressor__bitBuffer & 1;
	FabDecompressor__bitBuffer >>= 1;
	return bit;
}

int FabDecompressor__decompress(const byte *srcData, int srcSize, byte *destData, int destSize) {
	byte copyLen, copyOfsShift, copyOfsMask, copyLenMask;
	unsigned long copyOfs;
	byte *destP;

	// Validate that the data starts with the FAB header
	if (strncmp((const char *)srcData, "FAB", 3) != 0)
		return -1; //error("FabDecompressor - Invalid compressed data");

	int shiftVal = srcData[3];
	if ((shiftVal < 10) || (shiftVal > 13))
		return -1; //error("FabDecompressor - Invalid shift start");

	copyOfsShift = 16 - shiftVal;
	copyOfsMask = 0xFF << (shiftVal - 8);
	copyLenMask = (1 << copyOfsShift) - 1;
	copyOfs = 0xFFFF0000;
	destP = destData;

	// Initialize data fields
	FabDecompressor__srcData = srcData;
	FabDecompressor__srcP = FabDecompressor__srcData + 6;
	FabDecompressor__srcSize = srcSize;
	FabDecompressor__bitsLeft = 16;
	FabDecompressor__bitBuffer = READ_LE_UINT16(srcData + 4);

	for (;;) {
		if (FabDecompressor__getBit() == 0) {
			if (FabDecompressor__getBit() == 0) {
				copyLen = ((FabDecompressor__getBit() << 1) | FabDecompressor__getBit()) + 2;
				copyOfs = *FabDecompressor__srcP++ | 0xFFFFFF00;
			} else {
				copyOfs = (((FabDecompressor__srcP[1] >> copyOfsShift) | copyOfsMask) << 8) | FabDecompressor__srcP[0];
				copyLen = FabDecompressor__srcP[1] & copyLenMask;
				FabDecompressor__srcP += 2;
				if (copyLen == 0) {
					copyLen = *FabDecompressor__srcP++;
					if (copyLen == 0)
						break;
					else if (copyLen == 1)
						continue;
					else
						copyLen++;
				} else {
					copyLen += 2;
				}
				copyOfs |= 0xFFFF0000;
			}
			while (copyLen-- > 0) {
				if (destP - destData == destSize)
					return -1; //error(FabOutputExceededError);

				*destP = destP[(signed int)copyOfs];
				destP++;
			}
		} else {
			if (FabDecompressor__srcP - srcData == srcSize)
				return -1; //error(FabInputExceededError);
			if (destP - destData == destSize)
				return -1; //error(FabOutputExceededError);

			*destP++ = *FabDecompressor__srcP++;
		}
	}

    return destP - destData;
}



/*****************/

