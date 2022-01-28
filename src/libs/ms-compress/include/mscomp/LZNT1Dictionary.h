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


/////////////////// LZNT1 Dictionary //////////////////////////////////////////////////////////////
// The dictionary system used for LZNT1 compression that favors speed over memory usage.
// Most of the compression time is spent in the dictionary, particularly Find (72%) and Fill (19%).
//
// The base memory usage is 512 KB (or 768 KB on 64-bit systems). More memory is always allocated
// but only as much as needed. For a series of 4 KB chunks (what LZNT1 uses), the extra consumed
// memory averages about 715 KB (1430 KB on 64-bit) in test, but could theoretically grow to 1 GB
// (2 GB on 64-bit)!
//
// This implementation is about twice as fast as the SA version but consumes about 17-35x as much
// memory on average (and could be tons more) and requires dynamic allocations.

#include "internal.h"
#ifdef MSCOMP_WITH_LZNT1_SA_DICT
#include "LZNT1Dictionary_SA.h"
#endif

#ifndef MSCOMP_LZNT1_DICTIONARY_H
#define MSCOMP_LZNT1_DICTIONARY_H

class LZNT1Dictionary // 512+ KB (768+ KB on 64-bit systems)
{
private:
	// An entry within the dictionary, using a dynamically resized array of positions
	struct Entry // 6+ bytes (10+ bytes on 64-bit systems)
	{
		const_bytes* pos;
		int16_t cap;
		INLINE bool add(const const_rest_bytes data, const int16_t size)
		{
			if (size >= this->cap)
			{
				const_bytes *temp = (const_bytes*)realloc((bytes*)this->pos, (this->cap=(this->cap?(this->cap<<1):4))*sizeof(const_bytes));
				if (UNLIKELY(temp == NULL)) { return false; }
				this->pos = temp;
			}
			this->pos[size] = data;
			return true;
		}
	};

	// The dictionary
	const_bytes data;
	Entry entries[0x100*0x100]; // 384/640 KB
	int16_t sizes[0x100*0x100]; // 128 KB

public:
	INLINE LZNT1Dictionary()
	{
		// need to set pos to NULL and cap to 0
		memset(this->entries, 0, 0x100*0x100*sizeof(Entry));
	}

	INLINE ~LZNT1Dictionary()
	{
		for (uint32_t idx = 0; idx < 0x100*0x100; ++idx)
		{
			free(this->entries[idx].pos);
		}
	}

	// Fills the dictionary, ready to start a new chunk
	// This should also be called before any Find
	bool Fill(const_rest_bytes data, const int_fast16_t len)
	{
		this->data = data;
		Entry* const RESTRICT entries = this->entries;
		int16_t* const RESTRICT sizes = this->sizes;
		memset(sizes, 0, 0x100*0x100*sizeof(uint16_t));
		uint16_t idx = data[0];
		for (const_bytes end = data + len - 2; data < end; ++data)
		{
			idx = idx << 8 | data[1];
			if (UNLIKELY(!entries[idx].add(data, sizes[idx]++))) { return false; }
		}
		return true;
	}
	
WARNINGS_PUSH()
WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()

	// Finds the best symbol in the dictionary for the data
	// Returns the length of the string found, or 0 if nothing of length >= 3 was found
	// offset is set to the offset from the current position to the string
	int_fast16_t Find(const_rest_bytes data, const int_fast16_t max_len, int_fast16_t* RESTRICT offset) const
	{
		if (LIKELY(max_len >= 3 && data > this->data))
		{
			const uint_fast16_t idx = data[0] << 8 | data[1];
			const byte z = data[2];
			const int_fast16_t size = this->sizes[idx] - 1;
			const const_rest_bytes* RESTRICT pos = this->entries[idx].pos;
			int_fast16_t len = 0;
			const_rest_bytes found;

			// Do an exhaustive search (with the possible positions)
			for (int_fast16_t j = 0; j < size && pos[j] < data; ++j)
			{
				const const_rest_bytes ss = pos[j];
				if (ss[2] == z)
				{
					int_fast16_t i = 3;
					for (const_rest_bytes s = ss+3; i < max_len && data[i] == *s; ++i, ++s);
					if (i > len) { found = ss; len = i; if (len == max_len) { break; } }
				}
			}

			// Found a match, return it
			if (len >= 3) { *offset = (int_fast16_t)(data-found); return len; }
		}

		// No match found, return 0
		return 0;
	}
};

WARNINGS_POP()

#endif
