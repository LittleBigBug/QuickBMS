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


/////////////////// Dictionary /////////////////////////////////////////////////
// The dictionary system used for Xpress compression.
//
// TODO: ? Most of the compression time is spent in the dictionary - particularly Find and Add.

#ifndef MSCOMP_XPRESS_DICTIONARY_H
#define MSCOMP_XPRESS_DICTIONARY_H
#include "internal.h"

template<unsigned> class XpressDictionaryLevel { private: XpressDictionaryLevel(); };
template<> struct XpressDictionaryLevel<1> { const static uint32_t NiceLength =  16, MaxChain =   4; };
template<> struct XpressDictionaryLevel<2> { const static uint32_t NiceLength =  32, MaxChain =   8; };
template<> struct XpressDictionaryLevel<3> { const static uint32_t NiceLength =  48, MaxChain =  11; };
template<> struct XpressDictionaryLevel<4> { const static uint32_t NiceLength =  64, MaxChain =  16; };
template<> struct XpressDictionaryLevel<5> { const static uint32_t NiceLength = 128, MaxChain =  32; };
template<> struct XpressDictionaryLevel<6> { const static uint32_t NiceLength = 256, MaxChain =  64; };
template<> struct XpressDictionaryLevel<7> { const static uint32_t NiceLength = 512, MaxChain = 128; };
template<> struct XpressDictionaryLevel<8> { const static uint32_t NiceLength = UINT32_MAX, MaxChain = UINT32_MAX; };

WARNINGS_PUSH()
WARNINGS_IGNORE_ASSIGNMENT_OPERATOR_NOT_GENERATED()

template<uint32_t MaxOffset, uint32_t ChunkSize = MaxOffset, unsigned HashBits = 15, unsigned Level = 3>
class XpressDictionary
	// when ChunkSize is 0x02000: 192 kb (or  384 kb on 64-bit) [Xpress]
	// when ChunkSize is 0x10000: 640 kb (or 1280 kb on 64-bit) [Xpress Huffman]
{
	//TODO: CASSERT(IS_POW2(ChunkSize));
	CASSERT(MaxOffset <= ChunkSize);
	CASSERT(HashBits >= 8 && HashBits <= 16);

private:
	// Window properties
	static const uint32_t WindowSize = ChunkSize << 1;
	static const uint32_t WindowMask = WindowSize-1;
	FORCE_INLINE uint32_t WindowPos(const_bytes x) const { return (uint32_t)((x - this->start) & WindowMask); } // { return (uint32_t)((x - this->start) % WindowSize); }

	// The hashing function, which works progressively
	static const uint32_t HashSize = 1 << HashBits;
	static const uint32_t HashMask = HashSize - 1;
	static const unsigned HashShift = (HashBits+2)/3;
	FORCE_INLINE static uint_fast16_t HashUpdate(const uint_fast16_t h, const byte c) { return ((h<<HashShift) ^ c) & HashMask; }

	const const_bytes start, end, end2;
	const_bytes table[HashSize];
	const_bytes window[WindowSize];
	
#ifdef MSCOMP_WITH_UNALIGNED_ACCESS
	INLINE static uint32_t GetMatchLength(const_bytes a, const_bytes b, const const_bytes end, const const_bytes end4)
#else
	INLINE static uint32_t GetMatchLength(const_bytes a, const_bytes b, const const_bytes end)
#endif
	{
		// like memcmp but tells you the length of the match and optimized
		// assumptions: a < b < end, end4 = end - 4
		const const_bytes b_start = b;
		byte a0, b0;
#ifdef MSCOMP_WITH_UNALIGNED_ACCESS
		while (b < end4 && *((uint32_t*)a) == *((uint32_t*)b))
		{
			a += sizeof(uint32_t);
			b += sizeof(uint32_t);
		}
#endif
		do
		{
			a0 = *a++;
			b0 = *b++;
		} while (b < end && a0 == b0);
		return (uint32_t)(b - b_start - 1);
	}

public:
	typedef XpressDictionaryLevel<Level> LevelConfig;

	INLINE XpressDictionary(const const_bytes start, const const_bytes end) : start(start), end(end), end2(end - 2)
	{
		memset(this->table, 0, HashSize*sizeof(const_bytes));
	}

	INLINE const_bytes Fill(const_bytes data)
	{
		// equivalent to Add(data, ChunkSize)
		if (UNLIKELY(data >= this->end2)) { return this->end2; }
		uint32_t pos = WindowPos(data); // either 0x00000 or ChunkSize
		const const_bytes end = ((data + ChunkSize) < this->end2) ? data + ChunkSize : this->end2;
		uint_fast16_t hash = HashUpdate(data[0], data[1]);
		while (data < end)
		{
			hash = HashUpdate(hash, data[2]);
			this->window[pos++] = this->table[hash];
			this->table[hash] = data++;
		}
		return end;
	}

	INLINE void Add(const_bytes data)
	{
		if (data < this->end2)
		{
			// TODO: could make this more efficient by keeping track of the last hash
			uint_fast16_t hash = HashUpdate(HashUpdate(data[0], data[1]), data[2]);
			this->window[WindowPos(data)] = this->table[hash];
			this->table[hash] = data++;
		}
	}
	
	INLINE void Add(const_bytes data, size_t len)
	{
		if (UNLIKELY(data >= this->end2)) { return; }
		uint32_t pos = WindowPos(data);
		const const_bytes end = ((data + len) < this->end2) ? data + len : this->end2;
		uint_fast16_t hash = HashUpdate(data[0], data[1]);
		while (data < end)
		{
			hash = HashUpdate(hash, data[2]);
			this->window[pos++] = this->table[hash];
			this->table[hash] = data++;
		}
	}

	INLINE uint32_t Find(const const_bytes data, uint32_t* offset) const
	{
#if PNTR_BITS <= 32
		const const_bytes end = this->end; // on 32-bit, + UINT32_MAX will always overflow
#else
		const const_bytes end = ((data + UINT32_MAX) < data || (data + UINT32_MAX) >= this->end) ? this->end : data + UINT32_MAX; // if overflow or past end use the end
#endif
#ifdef MSCOMP_WITH_UNALIGNED_ACCESS
		const const_bytes xend = data - MaxOffset, end4 = end - 4;
		const uint16_t prefix = *(uint16_t*)data;
#else
		const const_bytes xend = data - MaxOffset;
		const byte prefix0 = data[0], prefix1 = data[1];
#endif
		const_bytes x;
		uint32_t len = 2, chain_length = LevelConfig::MaxChain;
		for (x = this->window[WindowPos(data)]; chain_length && x >= xend; x = this->window[WindowPos(x)], --chain_length)
		{
#ifdef MSCOMP_WITH_UNALIGNED_ACCESS
			if (*(uint16_t*)x == prefix)
			{
				// at this point the at least 3 bytes are matched (due to the hashing function forcing byte 3 to the same)
				const uint32_t l = GetMatchLength(x, data, end, end4);
#else
			if (x[0] == prefix0 && x[1] == prefix1)
			{
				// at this point the at least 3 bytes are matched (due to the hashing function forcing byte 3 to the same)
				const uint32_t l = GetMatchLength(x, data, end);
#endif
				if (l > len)
				{
					*offset = (uint32_t)(data - x);
					len = l;
					if (len >= LevelConfig::NiceLength) { break; }
				}
			}
		}
		return len;
	}
};

WARNINGS_POP()

#endif
