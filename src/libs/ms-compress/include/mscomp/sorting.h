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

#ifndef MSCOMP_SORTING
#define MSCOMP_SORTING

#include "internal.h"

#define SORT_SWITCH_TO_INSERT_LIMIT 90

// Insertion-sorts syms[l, r) using conditions[syms[x]]
// Use insertion-sort so that it is stable, keeping symbols in increasing order
// This is only used at the tail end of merge_sort.
template<typename T> // T is either uint32_t or byte
void insertion_sort(uint16_t* syms, const T* const conditions, const uint_fast16_t len)
{
	for (uint_fast16_t i = 1; i < len; ++i)
	{
		const uint16_t x = syms[i];
		const T cond = conditions[x];
		uint_fast16_t j = i;
		while (j > 0 && conditions[syms[j-1]] > cond) { syms[j] = syms[j-1]; --j; }
		syms[j] = x;
	}
}

// Merge-sorts syms[l, r) using conditions[syms[x]]
// Use merge-sort so that it is stable, keeping symbols in increasing order
template<typename T> // T is either uint32_t or byte
void merge_sort(uint16_t* syms, uint16_t* temp, const T* const conditions, const uint_fast16_t len)
{
	if (len < SORT_SWITCH_TO_INSERT_LIMIT)
	{
		// Insertion-Sort for the last "few"
		insertion_sort(syms, conditions, len);
	}
	else
	{
		// Merge-Sort
		const uint_fast16_t m = len >> 1;
		uint_fast16_t i = 0, j = 0, k = m;
		
		// Divide and Conquer
		merge_sort(syms,   temp,   conditions, m      );
		merge_sort(syms+m, temp+m, conditions, len - m);
		memcpy(temp, syms, len*sizeof(uint16_t));
		
		// Merge
		while (j < m && k < len) syms[i++] = (conditions[temp[k]] < conditions[temp[j]]) ? temp[k++] : temp[j++]; // if == then does j which is from the lower half, keeping stable
		if (j < m)        { memcpy(syms+i, temp+j, (m  -j)*sizeof(uint16_t)); }
		else if (k < len) { memcpy(syms+i, temp+k, (len-k)*sizeof(uint16_t)); }
	}
}

#endif
