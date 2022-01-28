// modified by Luigi Auriemma
// the compressor is a dummy function so it can't be used in reimport mode
// additionally the last bits may be incomplete due to the bitstream replacement I used
// USE ONLY THE DECOMPRESSOR!!!

/**
 * @file  filter-lzw.cpp
 * @brief Filter for compressing and decompressing data using LZW.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
 *
 * LZW algorithm based on GPL code by Juha Nieminen
 *   http://warp.povusers.org/EfficientLZW/
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
//#include <cassert>
//#include <functional>
//#include <iostream>
#include "filter-lzw.hpp"

/// How many bytes should be left in reserve
/**
 * This is required to avoid trying to read or write the next codeword, and
 * running out of space in the middle of it, truncating the codeword.  Setting
 * this to 2 means there will always be enough room for a 16-bit codeword.
 * Since all the LZW code so far only goes up to 12-bit codewords this should
 * cover everything.
 */
//#define LZW_LEFTOVER_BYTES 2
#define LZW_LEFTOVER_BYTES 0    // necessary

namespace camoto {

CodeString::CodeString(byte newByte, unsigned pI)
	:	prefixIndex(pI), first(~0U),
		nextLeft(~0U), nextRight(~0U),
		k(newByte)
{
}

void Dictionary::fillDecodedString(unsigned code)
{
	decodedString.clear();
	unsigned int safety = 0;
	unsigned int tableSize = table.size();
	while (code != ~0U) {
		if (code >= tableSize) return; /*throw filter_error("LZW data is corrupted - "
			"codeword was larger than the number of entries in the dictionary!");*/
		const CodeString& cs = table[code];
		decodedString.push_back(cs.k);

		// Make sure this codeword's prefix is not itself, otherwise we'll get
		// stuck in an infinite loop!
		if (cs.prefixIndex == code) return; /*throw filter_error("LZW data is corrupted - "
			"codeword's prefix is itself, cannot continue as this would cause an "
			"infinite loop!");*/
		code = cs.prefixIndex;

		if (++safety > tableSize * 2) {
			return; /*throw filter_error("LZW data is corrupted - searched through the "
				"dictionary twice but the code never mapped to a byte value!");*/
		}
	}
}

Dictionary::Dictionary(unsigned maxBits, unsigned codeStart)
	:	table(1<<maxBits),
		codeStart(codeStart), newCodeStringIndex(codeStart)
{
	for(unsigned i = 0; i < codeStart; ++i)
		table[i].k = i;
}

void Dictionary::decode(unsigned oldCode, unsigned code,
	std::deque<byte>& outStream)
{
	const bool exists = code < newCodeStringIndex;

	if (exists) fillDecodedString(code);
	else fillDecodedString(oldCode);

	for (size_t i = decodedString.size(); i > 0;) {
		outStream.push_back(decodedString[--i]);
	}
	if (!exists) outStream.push_back(decodedString.back());

	if (newCodeStringIndex < table.size()) {
		if (newCodeStringIndex == oldCode) {
            return;
            /* std::cerr << "LZW Warning: Tried to set prefix for codeword 0x"
				<< std::hex << newCodeStringIndex << " to itself!  This "
				"will cause an error if this codeword is ever used."
				<< std::endl; */
		}
		table[newCodeStringIndex].prefixIndex = oldCode;
		table[newCodeStringIndex++].k = decodedString.back();
	} // else dictionary is full, don't add anything to it
}

unsigned Dictionary::size() const
{
	return newCodeStringIndex;
}

void Dictionary::reset()
{
	newCodeStringIndex = codeStart;
	for(unsigned i = 0; i < codeStart; ++i)
		table[i] = CodeString(i);
}


filter_lzw_decompress::filter_lzw_decompress(int initialBits, int maxBits,
	int firstCode, int eofCode, int resetCode, int flags)
	:	maxBits(maxBits),
		flags(flags),
		eofCode(eofCode),
		resetCode(resetCode),
		initialBits(initialBits),
		dictionary(maxBits, firstCode),
		//data(((flags & LZW_BIG_ENDIAN) != LZW_BIG_ENDIAN) ? bitstream::littleEndian : bitstream::bigEndian),
		code(0)
{
}

void filter_lzw_decompress::reset(int lenInput)
{
	mybits_flush(&this->data); //this->data.flushByte();
	this->currentBits = this->initialBits;
	this->recalcCodes();
	this->resetDictionary();
	return;
}

void filter_lzw_decompress::transform(uint8_t *out, int *lenOut,
	const uint8_t *in, int *lenIn)
{
	int r = 0, w = 0;

    mybits_init(&data, (uint8_t *)in, *lenIn, NULL);
    data.force_endian = ((flags & LZW_BIG_ENDIAN) == LZW_BIG_ENDIAN);
	//fn_getnextchar cbNext = std::bind(bitstreamFilterNextChar, &in, lenIn, &r,
	//	std::placeholders::_1);
	while (
		(w < *lenOut) && (
			(
				(r + LZW_LEFTOVER_BYTES < *lenIn) || // Make sure there's at least some leftover bytes (for the longest codeword)
				((r < *lenIn) && (*lenIn <= LZW_LEFTOVER_BYTES)) // unless it's the last incoming byte
			) || (!this->buffer.empty())
		)
	) {
		if (!this->buffer.empty()) {
			*out++ = this->buffer.front();
			this->buffer.pop_front();
			w++;
		} else {
			if ((this->flags & LZW_EOF_PARAM_VALID) && (this->code == this->curEOFCode)) break;

			//unsigned int bitsRead = this->data.read(cbNext, this->currentBits, &this->code);
            this->code = mybits_read(&this->data, this->currentBits, 0);
            r = this->data.data - this->data.data_start;
            unsigned int bitsRead = this->currentBits;

			//if (this->currentBits > LZW_LEFTOVER_BYTES * 8) {
			//	std::cerr << "WARNING: libgamecommon/lzw.cpp needs bit width increased "
			//		"to avoid data corruption." << std::endl;
			//}
			if (bitsRead < this->currentBits) {
                /* std::cerr << "Couldn't read all bits this iteration (tried to read "
					<< this->currentBits << " but only got " << bitsRead << ".  "
					<< *lenIn - r << " bytes left to read, " << *lenOut - w
					<< " bytes left to write into.)" << std::endl; */
				// TODO: save bits and append next time?  How to test this?
				// EOF
				break;
			}
			//assert(bitsRead > 0);

			if ((this->flags & LZW_EOF_PARAM_VALID) && (this->code == this->curEOFCode)) continue;

			if (this->isDictReset) {
				// When the dictionary is empty, the next "codeword" is always the first
				// byte for the first dictionary entry, which also means it's the first
				// output byte too (once it's truncated to eight bits.)
				this->buffer.push_back(this->code);
				this->oldCode = this->code;
				this->isDictReset = false;
				continue;
			}

			// See if the code we just got is a special one that will reset the dictionary
			if ((this->flags & LZW_RESET_PARAM_VALID) && (this->code == this->curResetCode)) {
				this->resetDictionary();
				if (this->flags & LZW_FLUSH_ON_RESET) {
					mybits_flush(&this->data); //this->data.flushByte();
					// We can't use seek here because cbNext might not be from the stream
					//this->data.read(cbNext, num*8, &dummy);
				}
				continue;
			}

			this->dictionary.decode(this->oldCode, this->code, this->buffer);


			if (this->dictionary.size() > this->maxCode) {
				if (this->currentBits == this->maxBits) {
					if (this->flags & LZW_RESET_FULL_DICT) this->resetDictionary();
				} else {
					++this->currentBits;
					this->recalcCodes();
				}
			}

			this->oldCode = this->code;


			if (this->buffer.empty()) {
				//if (r == 0) r = -1; // EOF
				break;
			}
		}
	}
	*lenIn = r;
	*lenOut = w;
	return;
}

void filter_lzw_decompress::resetDictionary()
{
	this->dictionary.reset();
	if (!(this->flags & LZW_NO_BITSIZE_RESET)) {
		// Only reset the bit length with the dictionary if wanted
		this->currentBits = this->initialBits;
		this->recalcCodes();
	}
	this->isDictReset = true;
	return;
}

void filter_lzw_decompress::recalcCodes()
{
	// Work out the maximum codeword value that will be possible at the current
	// bit length.
	int actualMaxCode = (1 << this->currentBits) - 1;
	this->maxCode = actualMaxCode;

	// Take any reserved codewords into account so we increase the bit depth
	// at the correct time.
	if (this->flags & LZW_EOF_PARAM_VALID) {
		if (this->eofCode < 1) {
			// If the eofCode is zero or negative, take it as an index back from
			// the last codeword, with 0 being the largest possible codeword.
			this->curEOFCode = actualMaxCode + this->eofCode;
			this->maxCode--;
		} else this->curEOFCode = this->eofCode;
	}

	if (this->flags & LZW_RESET_PARAM_VALID) {
		if (this->resetCode < 1) {
			// If the resetCode is zero or negative, take it as an index back from
			// the last codeword, with 0 being the largest possible codeword.
			this->curResetCode = actualMaxCode + this->resetCode;
			this->maxCode--;
		} else this->curResetCode = this->resetCode;
	}
	return;
}

filter_lzw_compress::filter_lzw_compress(int initialBits, int maxBits,
	int firstCode, int eofCode, int resetCode, int flags)
	:	maxBits(maxBits),
		flags(flags),
		eofCode(eofCode),
		resetCode(resetCode),
		firstCode(firstCode),
		initialBits(initialBits),
		dictSize(256),
		currentBits(initialBits)
		//data(((flags & LZW_BIG_ENDIAN) != LZW_BIG_ENDIAN) ? bitstream::littleEndian : bitstream::bigEndian)
{
	//assert(initialBits > 0);
}

void filter_lzw_compress::reset(int lenInput)
{
	mybits_flush(&this->data); //this->data.flushByte();
	this->currentBits = this->initialBits;
	this->recalcCodes();
	this->resetDictionary();
	return;
}

void filter_lzw_compress::transform(uint8_t *out, int *lenOut,
	const uint8_t *in, int *lenIn)
{
	int r = 0, w = 0;

    mybits_init(&data, (uint8_t *)out, *lenOut, NULL);
    data.force_endian = ((flags & LZW_BIG_ENDIAN) == LZW_BIG_ENDIAN);
	//fn_putnextchar cbNext = std::bind(bitstreamFilterPutChar, &out, lenOut, &w,
	//	std::placeholders::_1);
	while (
		(w + LZW_LEFTOVER_BYTES < *lenOut) && ( // leave some leftover bytes to guarantee the codeword will be written
			(
				(r + 1 < *lenIn) || // Make sure there's at least one leftover byte
				((r < *lenIn) && (*lenIn < 2)) // unless it's the last incoming byte
			)
		)
	) {
		//if (this->currentBits > LZW_LEFTOVER_BYTES * 8) {
		//	std::cerr << "WARNING: libgamecommon/lzw.cpp needs bit width increased "
		//		"to avoid data corruption." << std::endl;
		//}
		//unsigned int bitsWritten = this->data.write(cbNext, this->currentBits, *in);
        unsigned int bitsWritten = mybits_write(&this->data, *in, this->currentBits, 0);
        w = this->data.data - this->data.data_start;

		in++;
		r++;
		if (bitsWritten < this->currentBits) {
			/* std::cerr << "Couldn't write all bits this iteration (tried to write "
				<< this->currentBits << " but only wrote " << bitsWritten << ".  "
				<< *lenIn - r << " bytes left to read, " << *lenOut - w
				<< " bytes left to write into.)" << std::endl; */
			// EOF
			break;
		}

		if (this->dictSize > this->maxCode) {
			if (this->currentBits == this->maxBits) {
				if (this->flags & LZW_RESET_FULL_DICT) this->resetDictionary();
			} else {
				++this->currentBits;
				this->recalcCodes();
			}
		} else {
			this->dictSize++;
		}
		//assert(bitsWritten > 0);
	}
	if ((w < *lenOut) && (*lenIn == 0)) {
		// No more data to read, but more space to write
		if ((this->flags & LZW_EOF_PARAM_VALID) && (this->curEOFCode > 0)) {
			// Add the EOF parameter
			//this->data.write(cbNext, this->currentBits, this->curEOFCode);
            mybits_write(&this->data, this->curEOFCode, this->currentBits, 0);
            w = this->data.data - this->data.data_start;

			// Don't write this again
			this->curEOFCode = 0;
		}
	}
	// Do the flush separately, in case we just ran out of output space
	if ((w < *lenOut) && (*lenIn == 0)) {
		mybits_flush(&this->data); //this->data.flushByte(cbNext);
	}
	*lenIn = r;
	*lenOut = w;
	return;
}

void filter_lzw_compress::resetDictionary()
{
	this->dictSize = 256;
	if (!(this->flags & LZW_NO_BITSIZE_RESET)) {
		// Only reset the bit length with the dictionary if wanted
		this->currentBits = this->initialBits;
		this->recalcCodes();
	}
	return;
}

void filter_lzw_compress::recalcCodes()
{
	// Work out the maximum codeword value that will be possible at the current
	// bit length.
	int actualMaxCode = (1 << this->currentBits) - 1;
	this->maxCode = actualMaxCode;

	// Take any reserved codewords into account so we increase the bit depth
	// at the correct time.
	if (this->flags & LZW_EOF_PARAM_VALID) {
		if (this->eofCode < 1) {
			// If the eofCode is zero or negative, take it as an index back from
			// the last codeword, with 0 being the largest possible codeword.
			this->curEOFCode = actualMaxCode + this->eofCode;
			this->maxCode--;
		} else {
			this->curEOFCode = this->eofCode;
			this->dictSize++; // this assumes the code is at the start
		}
	}

	if (this->flags & LZW_RESET_PARAM_VALID) {
		if (this->resetCode < 1) {
			// If the resetCode is zero or negative, take it as an index back from
			// the last codeword, with 0 being the largest possible codeword.
			this->curResetCode = actualMaxCode + this->resetCode;
			this->maxCode--;
		} else {
			this->curResetCode = this->resetCode;
			if (this->curEOFCode != this->curResetCode) {
				this->dictSize++; // this assumes the code is at the start
			}
		}
	}
	return;
}

} // namespace camoto





extern "C"
int camoto_lzw_decompress(
    int initialBits, int maxBits, int firstCode, int eofCode, int resetCode, int flags,
    uint8_t *in, int insz, uint8_t *out, int outsz
) {
    int     lenIn = insz,
            lenOut = outsz;
    camoto::filter_lzw_decompress   ctx(initialBits, maxBits, firstCode, eofCode, resetCode, flags);
    ctx.reset(0);   // necessary!
    ctx.transform(out, &lenOut, in, &lenIn);
    return lenOut;
}



/*
extern "C"
int camoto_lzw_compress(
    int initialBits, int maxBits, int firstCode, int eofCode, int resetCode, int flags,
    uint8_t *in, int insz, uint8_t *out, int outsz
) {
    int     lenIn = insz,
            lenOut = outsz;
    camoto::filter_lzw_compress     ctx(initialBits, maxBits, firstCode, eofCode, resetCode, flags);
    ctx.reset(0);   // necessary!
    ctx.transform(out, &lenOut, in, &lenIn);
    return lenOut;
}
*/
