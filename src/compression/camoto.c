// modified by Luigi Auriemma


/**
 * @file  filter-ddave-rle.cpp
 * @brief Filter implementation for decompressing Dangerous Dave tilesets.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/Dangerous_Dave_Graphics_Format
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../extra/mybits.h"    // QuickBMS

typedef unsigned char u8;
typedef int bool;
#define true 1
#define false 0



int filter_bash_unrle(uint8_t *out, int lenOut,
	const uint8_t *in, int lenIn)
{
    int     count = 0;
    int     prev = 0;
	int r = 0, w = 0;

	// While there's more space to write, and either more data to read or
	// more data to write
	while (              // while there is...
		(w < lenOut)      // more space to write into, and
		&& (
			(r < lenIn)     // more data to read, or
			|| (count) // more data to write
		)
	) {
		// If there is an RLE decode in progress
		if (count) {
			*out++ = prev;
			count--;
			w++;
		} else {
			// Otherwise no RLE decode in progress, keep reading
			if (*in == 0x90) { // RLE trigger byte
				if (r + 2 > lenIn) {
					// Not enough to read this byte this time
					if (r == 0) {
						// Haven't read anything yet, this is it
						return -1;//throw filter_error("Data ended on RLE code byte before giving a count!");
					}
					break;
				}
				in++;
				r++;
				count = *in++;
				r++;
				if (count == 0) {
					// Count of zero means a single 0x90 char
					prev = 0x90;
					// We could set count to 1 here and let the loop
					// take care of it, but this is quicker and we
					// wouldn't be here unless there was at least one
					// more byte of space in the output buffer.
					*out++ = 0x90;
					w++;
				} else count--; // byte we already wrote before the 0x90 is included in count
			} else { // normal byte
				prev = *in;
				*out++ = *in++;
				r++;
				w++;
			}
		}
	}

	lenIn = r;
	lenOut = w;
	return lenOut;
}



int filter_ddave_unrle(uint8_t *out, int lenOut,
	const uint8_t *in, int lenIn)
{
		int count;         ///< How many times to repeat prev
		uint8_t countByte; ///< Byte being repeated count times
		int copying;       ///< Number of bytes left to copy unchanged

	count = 0;
	copying = 0;

	int r = 0, w = 0;
	// While there's more space to write, and either more data to read or
	// more data to write
	while (              // while there is...
		(w < lenOut)      // more space to write into, and
		&& (
			(r < lenIn)     // more data to read, or
			|| (count) // more data to write
		)
	) {
		// If there is an RLE decode in progress
		if (count) {
			*out++ = countByte;
			count--;
			w++;
		} else {
			// Otherwise no RLE decode in progress, keep reading
			if (copying) {
				*out++ = *in++;
				w++;
				r++;
				copying--;
			} else if (*in & 0x80) { // high bit set
				copying = 1 + (*in & 0x7F);
				in++;
				r++;
			} else { // high bit unset
				if (r + 2 > lenIn) {
					// Not enough for this process, try again next time
					break;
				}
				count = 3 + *in++;
				countByte = *in++;
				r += 2;
			}
		}
	}

	lenIn = r;
	lenOut = w;
	return lenOut;
}




int filter_got_unlzss(uint8_t *out, int lenOut,
	const uint8_t *in, int lenIn)
{

#define ADD_DICT(c) \
	dictionary[dictPos] = c; \
	dictPos = (dictPos + 1) % GOT_DICT_SIZE;

    static int GOT_DICT_SIZE = 4096;
		uint8_t flags; ///< Flags for next eight blocks
		unsigned int blocksLeft; ///< Number of blocks left
		unsigned int lzssDictPos = 0;
		unsigned int lzssLength;
		uint8_t dictionary[GOT_DICT_SIZE];
		unsigned int dictPos;
		unsigned int lenDecomp; ///< Target output size
		unsigned int numDecomp; ///< Current output size
		enum {
			S0_READ_LEN,     ///< Read the header
			S1_READ_FLAGS,   ///< Read a flags byte
			S2_LITERAL,      ///< Copy a byte
			S3_GET_OFFSET,   ///< Read the LZSS offset/length data
			S4_COPY_OFFSET,  ///< Copy data from the dictionary
		} state;

	flags = 0;
	blocksLeft = 0;
	state = S0_READ_LEN;
	lzssLength = 0;
	memset(dictionary, 0, sizeof(uint8_t) * GOT_DICT_SIZE);
	dictPos = 0;
	lenDecomp = 0;
	numDecomp = 0;

	int r = 0, w = 0;

    // LA: I want the raw decompression algorithm without reading of size!!!
    lenDecomp = lenOut;
    state = S1_READ_FLAGS;

	// While there's more space to write, and either more data to read or
	// more data to write
	while (              // while there is...
		(w < lenOut)      // more space to write into, and
		&& (
			(r < lenIn)     // more data to read, or
			|| (lzssLength) // more data to write, and
		) && (
			(lenDecomp == 0) || // we haven't read the header yet, or
			(numDecomp < lenDecomp) // we have read it, and there's more data to decompress
		)
	) {
		bool needMoreData = false;

		switch (state) {
			case S0_READ_LEN:
				if (lenIn - r < 4) {
					needMoreData = true;
					break;
				}
				lenDecomp = *in++;
				lenDecomp |= *in++ << 8;
				r += 2;
				state = S1_READ_FLAGS;
				// Skip the other two bytes
				in += 2;
				r += 2;
				break;

			case S1_READ_FLAGS:
				if (blocksLeft == 0) {
					// Read the next lot of flags
					flags = *in++;
					r++;
					blocksLeft = 8;
				}
				if (flags & 1) {
					state = S2_LITERAL;
				} else {
					state = S3_GET_OFFSET;
				}
				flags >>= 1;
				blocksLeft--;
				break;

			case S2_LITERAL:
				ADD_DICT(*in);
				*out++ = *in++;
				r++;
				w++;
				numDecomp++;
				state = S1_READ_FLAGS;
				break;

			case S3_GET_OFFSET: {
				if (lenIn - r < 2) {
					needMoreData = true;
					break;
				}
				// Now we have at least two bytes to read
				unsigned int code = *in++;
				code |= *in++ << 8;
				r += 2;
				lzssLength = (code >> 12) + 2;
				lzssDictPos = (GOT_DICT_SIZE + dictPos - (code & 0x0FFF)) % GOT_DICT_SIZE;
				state = S4_COPY_OFFSET;
				break;
			}

			case S4_COPY_OFFSET:
				// Put this first in case we ever get an offset of zero
				if (lzssLength == 0) {
					state = S1_READ_FLAGS;
					break;
				}

				*out = dictionary[lzssDictPos++];
				ADD_DICT(*out);
				out++;
				w++;
				numDecomp++;
				lzssDictPos %= GOT_DICT_SIZE;
				lzssLength--;
				break;

		}
		if (needMoreData) break;
	}

	lenIn = r;
	lenOut = w;
	return lenOut;
#undef ADD_DICT
}



int filter_skyroads_unlzs(uint8_t *out, int lenOut,
	/*const*/ uint8_t *in, int lenIn)
{
		unsigned int width1=0, width2=0, width3=0;
		unsigned int dist=0;
		unsigned int lzsDictPos=0;
		unsigned int lzsLength=0;
		unsigned int dictPos=0;
		enum {
			S0_READ_LEN,     ///< Read the header
			S1_READ_FLAG1,   ///< Read the first code/flag
			S2_READ_FLAG2,   ///< Read the second code/flag
			S3_DECOMP_SHORT, ///< Short-length decompression
			S4_DECOMP_LONG,  ///< Long-length decompression
			S5_COPY_BYTE,    ///< Copy a literal byte
			S6_GET_COUNT,    ///< Read the LZS length data
			S7_COPY_OFFSET,  ///< Copy data from the dictionary
		} state;

#define SKYROADS_DICT_SIZE 4096

#define ADD_DICT(c) \
	dictionary[dictPos] = c; \
	dictPos = (dictPos + 1) % SKYROADS_DICT_SIZE;

	state = S0_READ_LEN;
	lzsLength = 0;
	uint8_t dictionary[SKYROADS_DICT_SIZE];
    memset(dictionary, 0, SKYROADS_DICT_SIZE);
	dictPos = 0;

	int r = 0, w = 0;

    //unsigned char bitchr=0,bitpos=0;
    mybits_ctx_t    ctx;
    mybits_init(&ctx, in, lenIn, NULL);
	//fn_getnextchar cbNext = std::bind(bitstreamFilterNextChar, &in, lenIn, &r,
	//	std::placeholders::_1);

	// While there's more space to write, and either more data to read or
	// more data to write
	while (              // while there is...
		(w < lenOut)      // more space to write into, and
		&& (
			(r < lenIn)     // more data to read, or
			|| (lzsLength) // more data to write, and
		)
	) {
		bool needMoreData = false;
		unsigned int /*bitsRead,*/ code;

		switch (state) {
			case S0_READ_LEN:
				if (lenIn - r < 3) {
					needMoreData = true;
					break;
				}
                width1 = mybits_read(&ctx, 8, 0);
                width2 = mybits_read(&ctx, 8, 0);
                width3 = mybits_read(&ctx, 8, 0);
                /*
				data.changeEndian(bitstream::littleEndian);
				bitsRead = data.read(cbNext, 8, &width1);
				bitsRead = data.read(cbNext, 8, &width2);
				bitsRead = data.read(cbNext, 8, &width3);
				data.changeEndian(bitstream::bigEndian);
                */

				state = S1_READ_FLAG1;
				break;

			case S1_READ_FLAG1:
                code = mybits_read(&ctx, 1, 1);
				/*bitsRead = data.read(cbNext, 1, &code);
				if (bitsRead == 0) {
					needMoreData = true;
					break;
				}*/
				if (code == 0) {
					state = S3_DECOMP_SHORT;
				} else {
					state = S2_READ_FLAG2;
				}
				break;

			case S2_READ_FLAG2:
                code = mybits_read(&ctx, 1, 1);
				/*bitsRead = data.read(cbNext, 1, &code);
				if (bitsRead == 0) {
					needMoreData = true;
					break;
				}*/
				if (code == 0) {
					state = S4_DECOMP_LONG;
				} else {
					state = S5_COPY_BYTE;
				}
				break;

			case S3_DECOMP_SHORT:
                code = mybits_read(&ctx, width2, 1);
				/*bitsRead = data.read(cbNext, width2, &code);
				if (bitsRead != width2) {
					needMoreData = true;
					break;
				}*/
				dist = 2 + code;
				state = S6_GET_COUNT;
				break;

			case S4_DECOMP_LONG:
                code = mybits_read(&ctx, width3, 1);
				/*bitsRead = data.read(cbNext, width3, &code);
				if (bitsRead != width3) {
					needMoreData = true;
					break;
				}*/
				dist = 2 + (1 << width2) + code;
				state = S6_GET_COUNT;
				break;

			case S5_COPY_BYTE:
                code = mybits_read(&ctx, 8, 1);
				/*bitsRead = data.read(cbNext, 8, &code);
				if (bitsRead != 8) {
					needMoreData = true;
					break;
				}*/
				ADD_DICT(code);
				*out++ = code;
				w++;
				state = S1_READ_FLAG1;
				break;

			case S6_GET_COUNT:
                code = mybits_read(&ctx, width1, 1);
				/*bitsRead = data.read(cbNext, width1, &code);
				if (bitsRead != width1) {
					needMoreData = true;
					break;
				}*/
				lzsLength = 2 + code;

                lzsLength %= SKYROADS_DICT_SIZE; // my fix
                /*
				if (lzsLength > SKYROADS_DICT_SIZE) {
					std::cerr << "lzs-skyroads: Length is > dict size, data is probably "
						"corrupt, aborting." << std::endl;
					throw stream::error("SkyRoads compressed data has backreference "
						"larger than dictionary length.  Data is probably corrupt or not "
						"in this compression format.");
				}
                */

				state = S7_COPY_OFFSET;
				lzsDictPos = (SKYROADS_DICT_SIZE + dictPos - dist) % SKYROADS_DICT_SIZE;
				break;

			case S7_COPY_OFFSET:
				// Put this first in case we ever get an offset of zero
				if (lzsLength == 0) {
					state = S1_READ_FLAG1;
					break;
				}

				*out = dictionary[lzsDictPos++];
				ADD_DICT(*out);
				out++;
				w++;
				lzsDictPos %= SKYROADS_DICT_SIZE;
				lzsLength--;
				break;

		}
		if (needMoreData) break;
	}

	lenIn = r;
	lenOut = w;
	return lenOut;
#undef ADD_DICT
}



int filter_z66_decompress(uint8_t *out, int lenOut,
	/*const*/ uint8_t *in, int lenIn)
{
		int state;

		unsigned int code=0, curCode=0;

		//std::stack<int> stack;
        int stack_idx = 0;
        static int stack[65536];

		int codeLength, curDicIndex, maxDicIndex;

		struct {
			unsigned int code, nextCode;
		} nodes[8192];

		unsigned int totalWritten; ///< Number of bytes written out so far overall
		unsigned int outputLimit;  ///< Maximum number of bytes to write out overall

	outputLimit = 4; // need to allow enough to read the length field
	totalWritten = 0;
	state = 0;
	codeLength = 9;
	curDicIndex = 0;
	maxDicIndex = 255;

    int i;
	for (i = 0; i < 8192; i++) {
		nodes[i].code = 0;
		nodes[i].nextCode = 0;
	}


	int r = 0, w = 0;

    unsigned char *inl = in + lenIn;
    //unsigned char bitchr=0,bitpos=0;
    mybits_ctx_t    ctx;
    mybits_init(&ctx, in, lenIn, NULL);

	//fn_getnextchar cbNext = std::bind(&filter_z66_decompress::nextChar, this,
	//	&in, lenIn, &r, std::placeholders::_1);

    // LA: I want the raw decompression algorithm without reading of size!!!
    state = 1;
    outputLimit = lenOut;

	while (
		(w < lenOut)  // while there is more space to write into
		&& (
			(r + 2 < lenIn) // and there's at least two more bytes to read
			|| (
				(lenIn < 10)   // or there's less than 10 bytes to read in this buffer (i.e. near EOF)
				&& (r < lenIn) // and there's at least one more byte to read
			)
			|| (state > 1) // or we're still processing what we read previously
		)
		&& (totalWritten < outputLimit) // and we haven't reached the target file size yet
	) {
		switch (state) {
			case 0: {
				// Read the first four bytes (decompressed size) so we can limit the
				// output size appropriately.
                outputLimit = mybits_read(&ctx, 32, 0);
                /*
				data.changeEndian(bitstream::littleEndian);
				data.read(cbNext, 32, &outputLimit);
				data.changeEndian(bitstream::bigEndian);
                */
				state++;
				break;
			}
			case 1: {
                code = mybits_read(&ctx, codeLength, 1);
                if(in > inl) goto done; // yes use > and not >=
            	/*if (data.read(cbNext, codeLength, &code)
					!= codeLength
				) {
					goto done;
				}*/
				curCode = code;
				state++;
				break;
			}
			case 2:
				if (curCode < 256) {
					*out++ = curCode;
					w++;
					totalWritten++;
					if (stack_idx > 0) {
						curCode = stack[--stack_idx];
					} else {
						state++;
						break;
					}
				} else {
					curCode -= 256;
					stack[stack_idx++] = nodes[curCode].nextCode;
					curCode = nodes[curCode].code;
					if (stack_idx > 65534) {
						return -1; //throw filter_error("Corrupted Zone 66 data - token stack > 64k");
					}
				}
				break;
			case 3: {
				unsigned int value;
                value = mybits_read(&ctx, 8, 1);
                if(in > inl) goto done; // yes use > and not >=
				//if (data.read(cbNext, 8, &value) != 8) goto done;
				*out++ = value;
				w++;
				totalWritten++;

				if (code >= 0x100u + curDicIndex) {
					// This code hasn't been put in the dictionary yet (tpal.z66)
					code = 0x100;
				}
				nodes[curDicIndex].code = code;
				nodes[curDicIndex].nextCode = value;
				curDicIndex++;

				if (curDicIndex >= maxDicIndex) {
					codeLength++;
					if (codeLength == 13) {
						codeLength = 9;
						curDicIndex = 64;
						maxDicIndex = 255;
					} else {
						maxDicIndex = (1 << codeLength) - 257;
					}
				}
				state = 1;
				break;
			} // case 4
		} // switch(state)
	} // while (more data to be read)

done:
	lenIn = r;
	lenOut = w;
	return lenOut;
}



int filter_stargunner_decompress(const uint8_t* in,
	unsigned int expanded_size, uint8_t* out)
{
	uint8_t tableA[256], tableB[256];
	unsigned int inpos = 0;
	unsigned int outpos = 0;
    int i;

	while (outpos < expanded_size) {
		// Initialise the dictionary so that no bytes are codewords (or if you
		// prefer, each byte expands to itself only.)
		for ( i = 0; i < 256; i++) tableA[i] = i;

		//
		// Read in the dictionary
		//

		uint8_t code;
		unsigned int tablepos = 0;
		do {
			code = in[inpos++];

			// If the code has the high bit set, the lower 7 bits plus one is the
			// number of codewords that will be skipped from the dictionary.  (Those
			// codewords were initialised to expand to themselves in the loop above.)
			if (code > 127) {
				tablepos += code - 127;
				code = 0;
			}
			if (tablepos == 256) break;

			// Read in the indicated number of codewords.
			for ( i = 0; i <= code; i++) {
				if (tablepos >= 256) {
					return -1; //throw filter_error("Dictionary was larger than 256 bytes");
				}
				uint8_t data = in[inpos++];
				tableA[tablepos] = data;
				if (tablepos != data) {
					// If this codeword didn't expand to itself, store the second byte
					// of the expansion pair.
					tableB[tablepos] = in[inpos++];
				}
				tablepos++;
			}
		} while (tablepos < 256);

		// Read the length of the data encoded with this dictionary
		int len = in[inpos++];
		len |= in[inpos++] << 8;

		//
		// Decompress the data
		//

		int expbufpos = 0;
		// This is the maximum number of bytes a single codeword can expand to.
		uint8_t expbuf[32];
		while (1) {
			if (expbufpos) {
				// There is data in the expansion buffer, use that
				code = expbuf[--expbufpos];
			} else {
				// There is no data in the expansion buffer, use the input data
				if (--len == -1) break; // no more input data
				code = in[inpos++];
			}

			if (code == tableA[code]) {
				// This byte is itself, write this to the output
				out[outpos++] = code;
			} else {
				// This byte is actually a codeword, expand it into the expansion buffer
				if (expbufpos >= (signed)sizeof(expbuf) - 2) {
                    return -1;
					//throw filter_error("Codeword expanded to more than "
					//	TOSTRING(sizeof(expbuf)) " bytes");
				}
				expbuf[expbufpos++] = tableB[code];
				expbuf[expbufpos++] = tableA[code];
			}
		}
	}
	return outpos;
}

