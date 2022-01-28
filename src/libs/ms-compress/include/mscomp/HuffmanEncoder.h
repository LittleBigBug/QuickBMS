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

#ifndef MSCOMP_HUFFMAN_ENCODER
#define MSCOMP_HUFFMAN_ENCODER

#include "internal.h"
#include "Bitstream.h"
#include "sorting.h"

template <byte NumBitsMax, uint16_t NumSymbols>
class HuffmanEncoder
{
private:
	uint16_t codes[NumSymbols];
	byte lens[NumSymbols];

#define HEAP_PUSH(x)                         \
{                                            \
	heap[++heap_len] = x;                    \
	uint_fast16_t j = heap_len;              \
	while (weights[x] < weights[heap[j>>1]]) \
	{                                        \
		heap[j] = heap[j>>1]; j >>= 1;       \
	}                                        \
	heap[j] = x;                             \
}

#define HEAP_POP()                                  \
{                                                   \
	uint_fast16_t i = 1, t = heap[1] = heap[heap_len--]; \
	for (;;)                                        \
	{                                               \
		uint_fast16_t j = i << 1;                   \
		if (j > heap_len) { break; }                \
		if (j < heap_len && weights[heap[j+1]] < weights[heap[j]]) { ++j; } \
		if (weights[t] < weights[heap[j]]) { break; } \
		heap[i] = heap[j];                          \
		i = j;                                      \
	}                                               \
	heap[i] = t;                                    \
}

public:
	const_bytes CreateCodes(uint32_t symbol_counts[NumSymbols]) // 17 kb stack (for NumSymbols == 0x200)
	{
		// Creates Length-Limited Huffman Codes using an optimized version of the original Huffman algorithm
		// Does not always produce optimal codes
		// Algorithm from "In-Place Calculation of Minimum-Redundancy Codes" by A Moffat and J Katajainen
		// Code adapted from bzip2. See http://www.bzip.org/.
		memset(this->codes, 0, sizeof(this->codes));

		// Compute the initial weights (the weight is in the upper 24 bits, the depth (initially 0) is in the lower 8 bits
		uint32_t weights[NumSymbols * 2]; // weights of nodes
		weights[0] = 0;
		for (uint_fast16_t i = 0; i < NumSymbols; ++i) { weights[i+1] = (symbol_counts[i] == 0 ? 1 : symbol_counts[i]) << 8; }

		for (;;)
		{
			// Build the initial heap
			uint_fast16_t heap_len = 0, heap[NumSymbols + 2] = { 0 }; // heap of symbols, 1 to heap_len
			for (uint_fast16_t i = 1; i <= NumSymbols; ++i) { HEAP_PUSH(i); }

			// Build the tree (its a bottom-up tree)
			uint_fast16_t n_nodes = NumSymbols, parents[NumSymbols * 2]; // parents of nodes, 1 to n_nodes
			memset(parents, 0, sizeof(parents));
			while (heap_len > 1)
			{
				uint_fast16_t n1 = heap[1]; HEAP_POP();
				uint_fast16_t n2 = heap[1]; HEAP_POP();
				parents[n1] = parents[n2] = ++n_nodes;
				weights[n_nodes] = ((weights[n1]&0xffffff00)+(weights[n2]&0xffffff00)) | (1 + MAX((weights[n1]&0x000000ff),(weights[n2]&0x000000ff)));
				HEAP_PUSH(n_nodes);
			}

			// Create the actual length codes
			bool too_long = false;
			for (uint_fast16_t i = 1; i <= NumSymbols; ++i)
			{
				byte j = 0;
				uint_fast16_t k = i;
				while (parents[k] > 0) { k = parents[k]; ++j; }
				this->lens[i-1] = j;
				if (j > NumBitsMax) { too_long = true; }
			}

			// If we had codes that were too long then we need to make all the weights smaller
			if (!too_long) { break; }
			for (uint_fast16_t i = 1; i <= NumSymbols; ++i)
			{
				weights[i] = (1 + (weights[i] >> 9)) << 8;
			}
		}

		// Compute the values of the codes
		uint_fast16_t min = this->lens[0], max = min;
		for (uint_fast16_t i = 1; i < NumSymbols; ++i)
		{
			if (this->lens[i] > max) { max = this->lens[i]; }
			else if (this->lens[i] < min) { min = this->lens[i]; }
		}
		uint16_t code = 0;
		for (uint_fast16_t n = min; n <= max; ++n)
		{
			for (uint_fast16_t i = 0; i < NumSymbols; ++i)
			{
				if (this->lens[i] == n) { this->codes[i] = code++; }
			}
			code <<= 1;
		}

		// Done!
		return this->lens;
	}

	const_bytes CreateCodesSlow(uint32_t symbol_counts[NumSymbols]) // 519 kb stack (for NumSymbols == 0x200)
	{
		// Creates Length-Limited Huffman Codes using the package-merge algorithm
		// Always produces optimal codes but is significantly slower than the Huffman algorithm
		memset(this->codes, 0, sizeof(this->codes));
		memset(this->lens,  0, sizeof(this->lens));

		// Fill the syms_by_count and syms_by_length with the symbols that were found
		uint16_t syms_by_count[NumSymbols], syms_by_len[NumSymbols], temp[NumSymbols]; // 3*2*512 = 3 kb
		uint_fast16_t len = 0;
		for (uint_fast16_t i = 0; i < NumSymbols; ++i) { if (symbol_counts[i]) { syms_by_count[len] = (uint16_t)i; syms_by_len[len++] = (uint16_t)i; this->lens[i] = NumBitsMax; } }

		////////// Get the Huffman lengths //////////
		merge_sort(syms_by_count, temp, symbol_counts, len); // sort by the counts
		if (UNLIKELY(len == 1))
		{
			this->lens[syms_by_count[0]] = 1; // never going to happen, but the code below would probably assign a length of 0 which is not right
		}
		else
		{
			///// Package-Merge Algorithm /////
			typedef struct _collection // 516 bytes each
			{
				byte symbols[NumSymbols];
				uint_fast16_t count;
			} collection;
			collection _cols[NumSymbols], *cols = _cols, _next_cols[NumSymbols], *next_cols = _next_cols; // 2*516*512 = 516 kb
			uint_fast16_t cols_len = 0, next_cols_len = 0;

			// Start at the lowest value row, adding new collection
			for (uint_fast16_t j = 0; j < NumBitsMax; ++j)
			{
				uint_fast16_t cols_pos = 0, pos = 0;

				// All but the last one/none get added to collections
				while ((cols_len-cols_pos + len-pos) > 1)
				{
					memset(next_cols+next_cols_len, 0, sizeof(collection));
					for (uint_fast16_t i = 0; i < 2; ++i) // hopefully unrolled...
					{
						if (pos >= len || (cols_pos < cols_len && cols[cols_pos].count < symbol_counts[syms_by_count[pos]]))
						{
							// Add cols[cols_pos]
							next_cols[next_cols_len].count += cols[cols_pos].count;
							for (uint_fast16_t s = 0; s < NumSymbols; ++s)
							{
								next_cols[next_cols_len].symbols[s] += cols[cols_pos].symbols[s];
							}
							++cols_pos;
						}
						else
						{
							// Add syms[pos]
							next_cols[next_cols_len].count += symbol_counts[syms_by_count[pos]];
							++next_cols[next_cols_len].symbols[syms_by_count[pos]];
							++pos;
						}
					}
					++next_cols_len;
				}
			
				// Leftover gets dropped
				if (cols_pos < cols_len)
				{
					const byte* const syms = cols[cols_pos].symbols;
					for (uint_fast16_t i = 0; i < NumSymbols; ++i) { this->lens[i] -= syms[i]; }
				}
				else if (pos < len)
				{
					--this->lens[syms_by_count[pos]];
				}

				// Move the next_collections to the current collections
				collection* temp = cols; cols = next_cols; next_cols = temp;
				cols_len = next_cols_len;
				next_cols_len = 0;
			}


			////////// Create Huffman codes from lengths //////////
			merge_sort(syms_by_len, temp, this->lens, len); // Sort by the code lengths
			for (uint_fast16_t i = 1; i < len; ++i)
			{
				// Code is previous code +1 with added zeroes for increased code length
				this->codes[syms_by_len[i]] = (this->codes[syms_by_len[i-1]] + 1) << (this->lens[syms_by_len[i]] - this->lens[syms_by_len[i-1]]);
			}
		}

		return this->lens;
	}

	FORCE_INLINE void EncodeSymbol(uint_fast16_t sym, OutputBitstream *bits) const { bits->WriteBits(this->codes[sym], this->lens[sym]); }
};

#undef HEAP_PUSH
#undef HEAP_POP

#endif
