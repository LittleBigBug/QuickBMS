// ms-compress: implements Microsoft compression algorithms
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


////////////////////////////// Bitstreams //////////////////////////////////////////////////////////
// A bitstream that allows either reading or writing, but not both at the same time.
// It reads uint16s for bits and 16 bits can be reliably read at a time.
// These are designed for speed and perform few checks. The burden of checking is on the caller.
// See the functions for assumptions they make that should be checked by the caller (asserts check
// these in the functions as well). Note that this->bits is >= 16 unless near the very end of the
// stream.

#ifndef MSCOMP_BITSTREAM_H
#define MSCOMP_BITSTREAM_H
#include "internal.h"

WARNINGS_PUSH()
WARNINGS_IGNORE_ASSIGNMENT_OPERATOR_NOT_GENERATED()

////////// Input Bitstream ////////////////////////////////////////////////////
class InputBitstream 
{
private:
	const_bytes in;
	const const_bytes in_end;
	uint32_t mask;		// The next bits to be read/written in the bitstream
	uint_fast8_t bits;	// The number of bits in mask that are valid
public:
	// Create an input bitstream
	//   Assumption: in != NULL && in_end - in >= 4
	INLINE InputBitstream(const_bytes in, const const_bytes in_end) : in(in+4), in_end(in_end), mask((GET_UINT16(in) << 16) | GET_UINT16(in+2)), bits(32) { assert(in); assert(in_end - in >= 4); }
	
	///// Basic Properties /////
	FORCE_INLINE const_bytes RawStream() { return this->in; }
	FORCE_INLINE uint_fast8_t AvailableBits() const { return this->bits; }
	// Get the remaining number of raw bytes (disregards pre-read bits)
	FORCE_INLINE size_t RemainingRawBytes() const { return this->in_end - this->in; }

	///// Peeking Functions /////
	// Peek at the next n bits of the stream
	//   Assumption: n <= 16 && n <= this->bits
	FORCE_INLINE uint32_t Peek(const uint_fast8_t n) const { ASSERT_ALWAYS(n <= 16); assert(n <= this->bits); return (this->mask >> 16) >> (16 - n); } // we can't do a single shift because if n is 0 then a shift by 32 is undefined
	// Check if all pre-read bits are 0, essentially Peek(AvailableBits()) == 0 (except that AvailableBits can be larger than 16, which Peek does not allow)
	// If there are 0 pre-read bits, returns true
	FORCE_INLINE bool MaskIsZero() const { return this->bits == 0 || (this->mask>>(32-this->bits)) == 0; }
	
	///// Skipping Functions /////
	// Skip the next n bits of the stream
	//   Assumption: n <= 16 && n <= this->bits
	INLINE void Skip(const uint_fast8_t n)
	{
		ASSERT_ALWAYS(n <= 16); ASSERT_ALWAYS(n <= this->bits);
		this->mask <<= n;
		this->bits -= n;
		if (this->bits < 16 && this->in + 2 <= this->in_end)
		{
			this->mask |= GET_UINT16(this->in) << (16 - this->bits);
			this->bits |= 0x10; //this->bits += 16;
			this->in += 2;
		}
	}
	// Skip the next n bits of the stream without bounds checks
	//   Assumption: n <= 16
	INLINE void Skip_Fast(const uint_fast8_t n)
	{
		ASSERT_ALWAYS(n <= 16);
		this->mask <<= n;
		this->bits -= n;
		if (this->bits < 16)
		{
			this->mask |= GET_UINT16(this->in) << (16 - this->bits);
			this->bits |= 0x10; //this->bits += 16;
			this->in += 2;
		}
	}
	
	///// Reading Functions /////
	// Read the next n bits of the stream, where n <= 16 (essentially Peek(n); Skip(n))
	//   Assumption: n <= 16 && n <= this->bits
	FORCE_INLINE uint32_t ReadBits(const uint_fast8_t n) { const uint32_t x = this->Peek(n); this->Skip(n); return x; }
	
	///// Fast Reading Functions /////
	// Equivalent to the reading functions but do not do bounds checks
	// Read the next n bits of the stream, where n <= 16 (essentially Peek(n); Skip_Fast(n))
	//   Assumption: n <= 16
	FORCE_INLINE uint32_t ReadBits_Fast(const uint_fast8_t n) { const uint32_t x = this->Peek(n); this->Skip_Fast(n); return x; }
	
	///// Raw Reading Functions /////
	// Get the next integer from the underlying stream, not the pre-read bits.
	// These assume that this->RemainingRawBytes() >= sizeof(type)
	FORCE_INLINE byte     ReadRawByte()   { assert(this->in + 1 <= this->in_end); return *this->in++; }
	FORCE_INLINE uint16_t ReadRawUInt16() { assert(this->in + 2 <= this->in_end); const uint16_t x = GET_UINT16(this->in); this->in += 2; return x; }
	FORCE_INLINE uint32_t ReadRawUInt32() { assert(this->in + 4 <= this->in_end); const uint32_t x = GET_UINT32(this->in); this->in += 4; return x; }
};


////////// Output Bitstream ///////////////////////////////////////////////////
class OutputBitstream
{
private:
	bytes out;
	uint16_t* pntr[2];	// the uint16's to write the data in mask to when there are enough bits
	uint32_t mask;		// The next bits to be read/written in the bitstream
	uint_fast8_t bits;	// The number of bits in mask that are valid
public:
	INLINE OutputBitstream(bytes out) : out(out+4), mask(0), bits(0)
	{
		assert(out);
		this->pntr[0] = (uint16_t*)(out);
		this->pntr[1] = (uint16_t*)(out+2);
	}
	FORCE_INLINE bytes RawStream() { return this->out; }
	INLINE void WriteBits(uint32_t b, uint_fast8_t n)
	{
		assert(n <= 16);
		this->mask |= b << (32 - (this->bits += n));
		if (this->bits > 16)
		{
			SET_UINT16(this->pntr[0], this->mask >> 16);
			this->mask <<= 16;
			this->bits &= 0xF; //this->bits -= 16;
			this->pntr[0] = this->pntr[1];
			this->pntr[1] = (uint16_t*)(this->out);
			this->out += 2;
		}
	}
	FORCE_INLINE void WriteRawByte(byte x)       { *this->out++ = x; }
	FORCE_INLINE void WriteRawUInt16(uint16_t x) { SET_UINT16(this->out, x); this->out += 2; }
	FORCE_INLINE void WriteRawUInt32(uint32_t x) { SET_UINT32(this->out, x); this->out += 4; }
	FORCE_INLINE void Finish()
	{
		SET_UINT16(this->pntr[0], this->mask >> 16); // if !bits then mask is 0 anyways
		SET_UINT16_RAW(this->pntr[1], 0);
	}
};

WARNINGS_POP()

#endif
