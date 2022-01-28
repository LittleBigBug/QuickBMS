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


/////////////////// LZNT1 Dictionary - Suffix Array Version ///////////////////////////////////////
// A dictionary system used for LZNT1 compression that balances speed and memory usage.
// Most of the compression time is spent in the dictionary, particularly Find (12%) and Fill (82%).
//
// This dictionary is based on a suffix+LCP array.
//
// The memory usage is at most ~41 KB, all on the stack and not dynamically allocated. While not
// filling, it only takes ~24 KB.
//
// This implementation is about half the speed of the default LZNT1 dictionary while using much less
// memory and no dynamic allocations.
//
// One additional note is that while both this dictionary and the default dictionary will produce
// "optimal" compression ratios, the output will not be identical since this algorithm will not
// find the closest match of the best length, but only a match of the best length.

#include "internal.h"
#ifdef MSCOMP_WITHOUT_LZNT1_SA_DICT
#include "LZNT1Dictionary.h"
#endif

#ifndef MSCOMP_LZNT1_DICTIONARY_H
#define MSCOMP_LZNT1_DICTIONARY_H

WARNINGS_PUSH()
WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT()

class LZNT1Dictionary // ~24kb
{
private:
	///// Suffix Array Construction /////
	// Uses the on SA-IS algorithm of Nong, Zhang & Chan (2009) - http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=4976463
	// Runs in O(n) time. Uses a max working space of ~17kb, all on the stack.
	// The implementation is based on the code by Yuta Mori - https://sites.google.com/site/yuta256/sais (MIT license).
	// It has been heavily modified to eliminate dynamic memory allocation and be optimized for n<=0x1000.
	#define get_bucket_starts(C, B, k) do { int16_t sum = 0; for (int_fast16_t i = 0; i < k; ++i) { sum += C[i]; B[i] = sum - C[i]; } } while (0)
	#define get_bucket_ends(C, B, k)   do { int16_t sum = 0; for (int_fast16_t i = 0; i < k; ++i) { sum += C[i]; B[i] = sum;        } } while (0)
	template<typename char_t>
	static INLINE void LMSsort(const char_t* RESTRICT const T, int16_t* RESTRICT const SA, const int16_t* RESTRICT const C, int16_t* RESTRICT const B, const int_fast16_t n, const int_fast16_t k)
	{
		// TODO: can SA/b be restricted?
		ALWAYS(T); ALWAYS(SA); ALWAYS(C); ALWAYS(B);
		if (sizeof(char_t) == sizeof(byte)) { ALWAYS(1 < n && n <= 0x1000 && k == 0x100); }
		else { ALWAYS(1 < n && n <= 0x800 && 0 < k && k < n); }

		/* compute SAl */
		get_bucket_starts(C, B, k); /* find starts of buckets */
		{
			int_fast16_t j = n - 1;
			char_t c1 = T[j];
			int16_t* RESTRICT b = SA + B[c1];
			*b++ = (int16_t)((T[--j] < c1) ? ~j : j);
			for (int_fast16_t i = 0; i < n; ++i)
			{
				int16_t j = SA[i];
				if (j < 0) { SA[i] = ~j; }
				else if (j > 0)
				{
					ASSERT_ALWAYS(T[j] >= T[j + 1]);
					const char_t c0 = T[j];
					if (c0 != c1) { B[c1] = (int16_t)(b - SA); b = SA + B[c1 = c0]; }
					ASSERT_ALWAYS(i < (b - SA));
					*b++ = (T[--j] < c1) ? ~j : j;
					SA[i] = 0;
				}
			}
		}

		/* compute SAs */
		get_bucket_ends(C, B, k); /* find ends of buckets */
		{
			char_t c1 = 0;
			int16_t* RESTRICT b = SA + B[c1];
			for (int_fast16_t i = n - 1; i >= 0; --i)
			{
				int16_t j = SA[i];
				if (j > 0)
				{
					ASSERT_ALWAYS(T[j] <= T[j + 1]);
					const char_t c0 = T[j];
					if (c0 != c1) { B[c1] = (int16_t)(b - SA); b = SA + B[c1 = c0]; }
					ASSERT_ALWAYS((b - SA) <= i);
					*--b = (T[--j] > c1) ? ~(j + 1) : j;
					SA[i] = 0;
				}
			}
		}
	}
	template<typename char_t>
	static INLINE int_fast16_t LMSpostproc(const char_t* RESTRICT const T, int16_t* RESTRICT const SA, int_fast16_t n, int_fast16_t m)
	{
		ALWAYS(T); ALWAYS(SA); ALWAYS(1 < m && 2*m <= n);
		if (sizeof(char_t) == sizeof(byte)) { ALWAYS(1 < n && n <= 0x1000); } else { ALWAYS(1 < n && n <= 0x800); }

		/* compact all the sorted substrings into the first m items of SA 2*m must be not larger than n (proveable) */
		{
			int_fast16_t i;
			int16_t p;
			for (i = 0; (p = SA[i]) < 0; ++i) { SA[i] = ~p; ASSERT_ALWAYS((i + 1) < n); }
			if (i < m)
			{
				int_fast16_t j = i;
				for (++i; ; ++i)
				{
					ASSERT_ALWAYS(i < n);
					if ((p = SA[i]) < 0)
					{
						SA[j++] = ~p; SA[i] = 0;
						if (j == m) { break; }
					}
				}
			}
		}

		/* store the length of all substrings */
		{
			int_fast16_t i = n - 1, j = n - 1;
			char_t c0 = T[n - 1], c1;
			do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
			while (i >= 0)
			{
				do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) <= c1));
				if (i >= 0)
				{
					SA[m + ((i + 1) >> 1)] = (int16_t)(j - i); j = i + 1;
					do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
				}
			}
		}

		/* find the lexicographic names of all substrings */
		int_fast16_t name = 0;
		{
			for (int_fast16_t i = 0, qlen = 0, q = n; i < m; ++i)
			{
				int_fast16_t p = SA[i], plen = SA[m + (p >> 1)];
				bool diff = true;
				if ((plen == qlen) && ((q + plen) < n))
				{
					int_fast16_t j;
					for (j = 0; (j < plen) && (T[p + j] == T[q + j]); ++j);
					if (j == plen) { diff = false; }
				}
				if (diff) { ++name, q = p, qlen = plen; }
				SA[m + (p >> 1)] = (int16_t)name;
			}
		}

		return name;
	}
	template<typename char_t>
	static INLINE int_fast16_t stage1sort(const char_t* RESTRICT const T, int16_t* RESTRICT const SA, const int16_t* RESTRICT const C, int16_t* RESTRICT const B, const int_fast16_t n, const int_fast16_t k)
	{
		// TODO: can SA/b be restricted?
		ALWAYS(T); ALWAYS(SA); ALWAYS(C); ALWAYS(B);
		if (sizeof(char_t) == sizeof(byte)) { ALWAYS(1 < n && n <= 0x1000 && k == 0x100); }
		else { ALWAYS(1 < n && n <= 0x800 && 0 < k && k < n); }

		get_bucket_ends(C, B, k); /* find ends of buckets */
		memset(SA, 0, n*sizeof(int16_t));
		int16_t* RESTRICT b = SA + n - 1;
		int_fast16_t m = 0, j = n, i = n - 1;
		char_t c0 = T[i], c1;
		do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
		while (i >= 0)
		{
			do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) <= c1));
			if (i >= 0)
			{
				*b = (int16_t)j; b = SA + --B[c1]; j = i; ++m; assert(B[c1] != (n - 1)); ALWAYS(B[c1] != (n - 1));
				do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
			}
		}
		SA[n - 1] = 0;
		if (m > 1) { LMSsort(T, SA, C, B, n, k); }
		else if (m == 1) { *b = (int16_t)(j + 1); }
		return m;
	}
	template<typename char_t>
	static INLINE void induceSA(const char_t* RESTRICT const T, int16_t* RESTRICT const SA, const int16_t* RESTRICT const C, int16_t* RESTRICT const B, const int_fast16_t n, const int_fast16_t k)
	{
		// TODO: can SA/b be restricted?
		ALWAYS(T); ALWAYS(SA); ALWAYS(C); ALWAYS(B); ALWAYS(1 < n && n <= 0x1000);
		if (sizeof(char_t) == sizeof(byte)) { ALWAYS(k == 0x100); } else { ALWAYS(0 < k && k < n); }

		/* compute SAl */
		get_bucket_starts(C, B, k); /* find starts of buckets */
		{
			const int_fast16_t j = n - 1;
			char_t c1 = T[j];
			int16_t* RESTRICT b = SA + B[c1];
			*b++ = (int16_t)(((j > 0) && (T[j - 1] < c1)) ? ~j : j);
			for (int_fast16_t i = 0; i < n; ++i)
			{
				int16_t j = SA[i];
				SA[i] = ~j;
				if (j > 0)
				{
					const char_t c0 = T[--j];
					if (c0 != c1) { B[c1] = (int16_t)(b - SA); b = SA + B[c1 = c0]; }
					*b++ = (int16_t)(((j > 0) && (T[j - 1] < c1)) ? ~j : j);
				}
			}
		}

		/* compute SAs */
		get_bucket_ends(C, B, k); /* find ends of buckets */
		{
			char_t c1 = 0;
			int16_t* RESTRICT b = SA + B[0];
			for (int_fast16_t i = n - 1; 0 <= i; --i)
			{
				int16_t j = SA[i];
				if (j > 0)
				{
					const char_t c0 = T[--j];
					if (c0 != c1) { B[c1] = (int16_t)(b - SA); b = SA + B[c1 = c0]; }
					*--b = (int16_t)(((j == 0) || (T[j - 1] > c1)) ? ~j : j);
				}
				else { SA[i] = ~j; }
			}
		}
	}
	static INLINE void suffixsort(const int16_t* RESTRICT const T, int16_t* RESTRICT const SA, const int_fast16_t n, const int_fast16_t k)
	{
		// TODO: can SA/RA be restricted?
		ALWAYS(T); ALWAYS(SA); ALWAYS(1 < n && n <= 0x800); ALWAYS(0 < k && k < n);

		/* stage 1: reduce the problem by at least 1/2 sort all the S-substrings */
		int16_t C[0x800], B[0x800];
		memset(C, 0, k * sizeof(int16_t));
		for (int_fast16_t i = 0; i < n; ++i) { ++C[T[i]]; }
		const int_fast16_t m = stage1sort(T, SA, C, B, n, k);
		ALWAYS(0 <= m && m <= n);
		if (m > 1)
		{
			ASSERT_ALWAYS((n >> 1) <= (n - m));
			const int_fast16_t name = LMSpostproc(T, SA, n, m);
			ALWAYS(0 <= name && name <= n);
			if (m > name)
			{
				/* stage 2: solve the reduced problem recurse if names are not yet unique */
				int16_t* RESTRICT const RA = SA + n - m;
				for (int_fast16_t i = m + (n >> 1) - 1, j = m - 1; i >= m; --i) { if (SA[i]) { RA[j--] = SA[i] - 1; } }
				suffixsort(RA, SA, m, name);

				int_fast16_t i = n - 1, j = m - 1;
				int16_t c0 = T[n - 1], c1;
				do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
				for(; 0 <= i;)
				{
					do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) <= c1));
					if (0 <= i)
					{
						RA[j--] = (int16_t)(i + 1);
						do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
					}
				}
				for (i = 0; i < m; ++i) { SA[i] = RA[SA[i]]; }
			}

			/* put all left-most S characters into their buckets */
			get_bucket_ends(C, B, k); /* find ends of buckets */
			int_fast16_t i = m - 1, j = n;
			int16_t p = SA[m - 1], c1 = T[p];
			do
			{
				const int16_t c0 = c1, q = B[c1];
				while (j > q) { SA[--j] = 0; }
				do { SA[--j] = p; if (--i < 0) { break; } p = SA[i]; } while ((c1 = T[p]) == c0);
			} while (i >= 0);
			while (j > 0) { SA[--j] = 0; }
		}

		/* stage 3: induce the result for the original problem */
		induceSA(T, SA, C, B, n, k);
	}
	static INLINE void sais(const const_rest_bytes T, int16_t* RESTRICT const SA, const int_fast16_t n)
	{
		// TODO: can SA/RA be restricted?
		ALWAYS(T); ALWAYS(SA); ALWAYS(0 < n && n <= 0x1000);
		if (UNLIKELY(n == 1)) { SA[0] = 0; return; }

		/* stage 1: reduce the problem by at least 1/2 sort all the S-substrings */
		int16_t C[0x100], B[0x100];
		memset(C, 0, sizeof(C));
		for (int_fast16_t i = 0; i < n; ++i) { ++C[T[i]]; }
		const int_fast16_t m = stage1sort(T, SA, C, B, n, 0x100);
		ALWAYS(0 <= m && m <= n);

		if (m > 1)
		{
			ASSERT_ALWAYS((n >> 1) <= (n - m));
			const int_fast16_t name = LMSpostproc(T, SA, n, m);
			ALWAYS(0 <= name && name <= n);
			if (m > name)
			{
				/* stage 2: solve the reduced problem recurse if names are not yet unique */
				int16_t* RESTRICT const RA = SA + n - m;
				for (int_fast16_t i = m + (n>>1) - 1, j = m - 1; i >= m; --i) { if (SA[i]) { RA[j--] = SA[i] - 1; } }
				suffixsort(RA, SA, m, name);

				int_fast16_t i = n - 1, j = m - 1;
				byte c0 = T[i], c1;
				do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
				while (i >= 0)
				{
					do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) <= c1));
					if (i >= 0)
					{
						RA[j--] = (int16_t)(i + 1);
						do { c1 = c0; } while ((--i >= 0) && ((c0 = T[i]) >= c1));
					}
				}

				for (i = 0; i < m; ++i) { SA[i] = RA[SA[i]]; }
			}

			/* put all left-most S characters into their buckets */
			get_bucket_ends(C, B, 0x100); /* find ends of buckets */
			int_fast16_t i = m - 1, j = n;
			int16_t p = SA[m - 1];
			byte c1 = T[p];
			do
			{
				const byte c0 = c1;
				const int16_t q = B[c1];
				while (j > q) { SA[--j] = 0; }
				do { SA[--j] = p; if (--i < 0) { break; } p = SA[i]; } while ((c1 = T[p]) == c0);
			} while (i >= 0);
			while (j > 0) { SA[--j] = 0; }
		}

		/* stage 3: induce the result for the original problem */
		induceSA(T, SA, C, B, n, 0x100);
	}
	#undef get_bucket_starts
	#undef get_bucket_ends

	///// LCP Array Construction /////
	// Creates the LCP (longest common prefix) array from the suffix array in O(n) time with 4n memory (~16kb).
	// Uses the Phi algorithm of Karkkainen, Manzini & Puglisi (2009). It calculates the PLCP first and converts it to the LCP.
	// Another choice would be to use Fischer (2011) which actually calculates the LCP as part of the SA-IS algorithm.
	INLINE static void calc_lcp(const const_bytes T, const int16_t* RESTRICT const SA, int16_t* RESTRICT const lcp, const int_fast16_t n)
	{
		// Compute Phi
		int16_t phi[0x1000];
		for (int_fast16_t i = 1; i < n; ++i) { phi[SA[i]] = SA[i-1]; }
		phi[SA[0]] = (int16_t)n; // in a theoretical suffix array, the terminating symbol would be an extra entry in the SA table right before our SA[0]

		// Compute PLCP
		int16_t plcp[0x1000];
		for (int_fast16_t i = 0, l = 0; i < n; ++i)
		{
			const int16_t ip = phi[i];
			const_rest_bytes Ti = T + i, Tphi = T + ip;
			const int_fast16_t max_l = n - MAX(i, ip);
			while (l < max_l && Ti[l] == Tphi[l]) { ++l; }
			plcp[i] = (int16_t)l;
			if (l > 0) { --l; }
		}

		// Compute LCP
		for (int_fast16_t i = 1; i < n; ++i) { lcp[i] = plcp[SA[i]]; }
	}
	//INLINE static void calc_lcp_bf(const const_rest_bytes T, const int16_t* RESTRICT const SA, int16_t* RESTRICT const lcp, const int_fast16_t n)
	//{
	//	// The naive way to calculate the LCP array, runs in O(n^2) time with essentially no extra memory.
	//	const_bytes end = T+n;
	//	for (int_fast16_t i = 1; i < n; ++i) { lcp[i] = (int16_t)match_length(T+SA[i], T+SA[i-1], end); }
	//}

	///// Dictionary Data /////
	const_rest_bytes data;
	int_fast16_t len;
	int16_t sa[0x1000], lcp[0x1000], sa_inv[0x1000];
	//INLINE const_bytes suf(const int_fast16_t i) const { return this->data+this->sa[i]; }
	//INLINE static int_fast16_t match_length(const_rest_bytes a, const_rest_bytes b, const_bytes end)
	//{
	//	// The length of the match between a and b, up until end.
	//	// a, b, and end must be from the same memory chunk.
	//	// TODO: can optimize this like in XpressDictionary
	//	const const_bytes b_start = b;
	//	while (a < end && b < end) { if (*a++ != *b++) { return (int_fast16_t)(b - b_start - 1); } }
	//	return (int_fast16_t)(b - b_start);
	//}

public:
	INLINE LZNT1Dictionary() { this->lcp[0] = 0; }
	INLINE void Fill(const_rest_bytes data, const int_fast16_t len)
	{
		this->data = data;
		this->len = len;
		if (LIKELY(len > 3))
		{
			int16_t *const sa = this->sa, *const sa_inv = this->sa_inv;
			sais(data, sa, len);
			for (int16_t i = 0; i < len; ++i) { sa_inv[sa[i]] = i; }
			calc_lcp(data, sa, this->lcp, len);

			//int16_t lcp_bf[0xFFD];
			//calc_lcp_bf(data, sa, lcp_bf, len);
			//for (int i = 0; i < len3; ++i) { assert(lcp[i] == lcp_bf[i]); }
		}
	}

	// Finds the best symbol in the dictionary for the data
	// Returns the length of the string found, or 0 if nothing of length >= 3 was found
	// offset is set to the offset from the current position to the string
WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()
	INLINE int_fast16_t Find(const_rest_bytes data, const int_fast16_t max_len, int_fast16_t* RESTRICT offset) const
	{
		const int_fast16_t pos = (int_fast16_t)(data - this->data);
		if (LIKELY(max_len >= 3 && pos))
		{
			const int_fast16_t sa_inv = this->sa_inv[pos], n = this->len;
			const int16_t* RESTRICT const sa = this->sa, *RESTRICT lcp = this->lcp;
			
			// The found match (if len>2)
			int_fast16_t len = 2, found;

			// Search lexicographically before
			int_fast16_t min_lcp = lcp[sa_inv];
			if (min_lcp > 2) // if sa_inv == 0 then lcp[sa_inv] == 0
			{
				const int_fast16_t sa_i = sa[sa_inv-1];
				if (sa_i < pos) { len = min_lcp; found = sa_i; }
				else
				{
					for (int_fast16_t i = sa_inv-2; i >= 0; --i)
					{
						const int_fast16_t lcp_i = lcp[i+1];
						if (lcp_i < min_lcp) { if (lcp_i <= 2) { break; } min_lcp = lcp_i; }
						const int_fast16_t sa_i = sa[i];
						if (sa_i < pos) { len = min_lcp; found = sa_i; break; }
					}
				}
			}

			// Search lexicographically after
			if (sa_inv != n-1)
			{
				int_fast16_t min_lcp = lcp[sa_inv+1];
				if (min_lcp > len)
				{
					const int_fast16_t sa_i = sa[sa_inv+1];
					if (sa_i < pos) { len = min_lcp; found = sa_i; }
					else
					{
						for (int_fast16_t i = sa_inv+2; i < n; ++i)
						{
							const int_fast16_t lcp_i = lcp[i];
							if (lcp_i < min_lcp) { if (lcp_i <= len) { break; } min_lcp = lcp_i; }
							const int_fast16_t sa_i = sa[i];
							if (sa_i < pos) { len = min_lcp; found = sa_i; break; }
						}
					}
				}
			}

			// Report match
			if (len > 2) { *offset = pos - found; return len > max_len ? max_len : len; }
		}

		// No match found, return 0
		return 0;
	}
};

WARNINGS_POP()

#endif
