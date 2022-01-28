/*
 * Doboz Data Compression Library
 * Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>
 * 
 * This software is provided 'as-is', without any express or implied warranty. In no event will
 * the authors be held liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including commercial
 * applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 *    original software. If you use this software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 *    being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <climits>
#include <cassert>

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */
#endif

#if defined(_MSC_VER)
#define DOBOZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define DOBOZ_FORCEINLINE inline __attribute__ ((always_inline))
#else
#define DOBOZ_FORCEINLINE inline
#endif

namespace doboz {

const int VERSION = 0; // encoding format

enum Result
{
	RESULT_OK,
	RESULT_ERROR_BUFFER_TOO_SMALL,
	RESULT_ERROR_CORRUPTED_DATA,
	RESULT_ERROR_UNSUPPORTED_VERSION,
};


namespace detail {

struct Match
{
	int length;
	int offset;
};

struct Header
{
	uint64_t uncompressedSize;
	uint64_t compressedSize;
	int version;
	bool isStored;
};


const int WORD_SIZE = 4; // uint32_t

const int MIN_MATCH_LENGTH = 3;
const int MAX_MATCH_LENGTH = 255 + MIN_MATCH_LENGTH;
const int MAX_MATCH_CANDIDATE_COUNT = 128;
const int DICTIONARY_SIZE = 1 << 21; // 2 MB, must be a power of 2!

const int TAIL_LENGTH = 2 * WORD_SIZE; // prevents fast write operations from writing beyond the end of the buffer during decoding
const int TRAILING_DUMMY_SIZE = WORD_SIZE; // safety trailing bytes which decrease the number of necessary buffer checks


// Reads up to 4 bytes and returns them in a word
// WARNING: May read more bytes than requested!
DOBOZ_FORCEINLINE uint32_t fastRead(const void* source, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		return *reinterpret_cast<const uint32_t*>(source);

	case 3:
		return *reinterpret_cast<const uint32_t*>(source);

	case 2:
		return *reinterpret_cast<const uint16_t*>(source);

	case 1:
		return *reinterpret_cast<const uint8_t*>(source);

	default:
		return 0; // dummy
	}
}

// Writes up to 4 bytes specified in a word
// WARNING: May write more bytes than requested!
DOBOZ_FORCEINLINE void fastWrite(void* destination, uint32_t word, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		*reinterpret_cast<uint32_t*>(destination) = word;
		break;

	case 3:
		*reinterpret_cast<uint32_t*>(destination) = word;
		break;

	case 2:
		*reinterpret_cast<uint16_t*>(destination) = static_cast<uint16_t>(word);
		break;

	case 1:
		*reinterpret_cast<uint8_t*>(destination) = static_cast<uint8_t>(word);
		break;
	}
}

} // namespace detail

} // namespace doboz