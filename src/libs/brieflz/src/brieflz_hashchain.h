//
// BriefLZ - small fast Lempel-Ziv
//
// Lazy parsing with chains of previous positions
//
// Copyright (c) 2016-2020 Joergen Ibsen
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must
//      not claim that you wrote the original software. If you use this
//      software in a product, an acknowledgment in the product
//      documentation would be appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must
//      not be misrepresented as being the original software.
//
//   3. This notice may not be removed or altered from any source
//      distribution.
//

#ifndef BRIEFLZ_HASHCHAIN_H_INCLUDED
#define BRIEFLZ_HASHCHAIN_H_INCLUDED

static size_t
blz_hashchain_workmem_size(size_t src_size)
{
	return (LOOKUP_SIZE + src_size) * sizeof(blz_word);
}

// Lazy parsing with chains of previous positions.
//
// Use the lookup table to create chains of previous positions for a given
// hash. This version uses an array of previous positions the size of the
// input.
//
// Since the same four bytes may occur many times, it is often required to
// limit the number of positions checked. The search is limited to following
// max_depth positions down each chain.
//
static unsigned long
blz_pack_hashchain(const void *src, void *dst, unsigned long src_size, void *workmem,
                   const unsigned long max_depth, const unsigned long accept_len)
{
	struct blz_state bs;
	blz_word *const lookup = (blz_word *) workmem;
	blz_word *const prev = lookup + LOOKUP_SIZE;
	const unsigned char *const in = (const unsigned char *) src;
	const unsigned long last_match_pos = src_size > 4 ? src_size - 4 : 0;
	unsigned long cur = 0;

	assert(src_size < BLZ_WORD_MAX);

	// Check for empty input
	if (src_size == 0) {
		return 0;
	}

	bs.next_out = (unsigned char *) dst;

	// First byte verbatim
	*bs.next_out++ = in[0];

	// Check for 1 byte input
	if (src_size == 1) {
		return 1;
	}

	// Initialize first tag
	bs.tag_out = bs.next_out;
	bs.next_out += 2;
	bs.tag = 0;
	bs.bits_left = 16;

	// Initialize lookup
    unsigned long i;
	for (i = 0; i < LOOKUP_SIZE; ++i) {
		lookup[i] = NO_MATCH_POS;
	}

	// Build hash chains in prev
	if (last_match_pos > 0) {
		for (i = 0; i <= last_match_pos; ++i) {
			const unsigned long hash = blz_hash4(&in[i]);
			prev[i] = lookup[hash];
			lookup[hash] = i;
		}
	}

	// Main compression loop
	for (cur = 1; cur <= last_match_pos; ) {
		unsigned long best_pos = NO_MATCH_POS;
		unsigned long best_len = 0;

		// Look up first match for current position
		unsigned long pos = prev[cur];

		const unsigned long len_limit = src_size - cur;
		unsigned long num_chain = max_depth;

		// Check matches
		while (pos != NO_MATCH_POS && num_chain--) {
			unsigned long len = 0;

			// If next byte matches, so this has a chance to be a longer match
			if (best_len < len_limit
			 && in[pos + best_len] == in[cur + best_len]) {
				// Find match len
				while (len < len_limit && in[pos + len] == in[cur + len]) {
					++len;
				}
			}

			// Update best match
			if (blz_match_better(cur, pos, len, best_pos, best_len)) {
				best_pos = pos;
				best_len = len;
				if (best_len >= accept_len) {
					break;
				}
			}

			// Go to previous match
			pos = prev[pos];
		}

		// Check if match at next position is better
		if (best_len > 3 && best_len < accept_len && cur < last_match_pos) {
			// Look up first match for next position
			unsigned long next_pos = prev[cur + 1];

			const unsigned long next_len_limit = src_size - (cur + 1);
			num_chain = max_depth;

			// Check matches
			while (next_pos != NO_MATCH_POS && num_chain--) {
				unsigned long next_len = 0;

				// Check match
				if (best_len - 1 < next_len_limit
				 && in[next_pos + best_len - 1] == in[cur + 1 + best_len - 1]) {
					while (next_len < next_len_limit
					    && in[next_pos + next_len] == in[cur + 1 + next_len]) {
						++next_len;
					}
				}

				if (next_len >= best_len) {
					// Replace with next match if it extends backwards
					if (next_pos > 0 && in[next_pos - 1] == in[cur]) {
						if (blz_match_better(cur, next_pos - 1, next_len + 1, best_pos, best_len)) {
							best_pos = next_pos - 1;
							best_len = next_len + 1;
						}
					}
					else {
						// Drop current match if next match is better
						if (blz_next_match_better(cur, next_pos, next_len, best_pos, best_len)) {
							best_len = 0;
							break;
						}
					}
				}

				// Go to previous match
				next_pos = prev[next_pos];
			}
		}

		// Output match or literal
		if (best_len > 4 || (best_len == 4 && cur - best_pos - 1 < 0x3FE00UL)) {
			const unsigned long offs = cur - best_pos - 1;

			// Output match tag
			blz_putbit(&bs, 1);

			// Output match length
			blz_putgamma(&bs, best_len - 2);

			// Output match offset
			blz_putgamma(&bs, (offs >> 8) + 2);
			*bs.next_out++ = offs & 0x00FF;

			cur += best_len;
		}
		else {
			// Output literal tag
			blz_putbit(&bs, 0);

			// Copy literal
			*bs.next_out++ = in[cur++];
		}
	}

	// Output any remaining literals
	while (cur < src_size) {
		// Output literal tag
		blz_putbit(&bs, 0);

		// Copy literal
		*bs.next_out++ = in[cur++];
	}

	// Trailing one bit to delimit any literal tags
	blz_putbit(&bs, 1);

	// Shift last tag into position and store
	bs.tag <<= bs.bits_left;
	bs.tag_out[0] = bs.tag & 0x00FF;
	bs.tag_out[1] = (bs.tag >> 8) & 0x00FF;

	// Return compressed size
	return (unsigned long) (bs.next_out - (unsigned char *) dst);
}

#endif /* BRIEFLZ_HASHCHAIN_H_INCLUDED */
