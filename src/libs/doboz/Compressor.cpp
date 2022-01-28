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

#include <cstring>
#include <algorithm>
#include "Compressor.h"

namespace doboz {

using namespace detail;

Result Compressor::compress(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize)
{
	assert(source != 0);
	assert(destination != 0);

	if (sourceSize == 0)
	{
		return RESULT_ERROR_BUFFER_TOO_SMALL;
	}

	uint64_t maxCompressedSize = getMaxCompressedSize(sourceSize);
	if (destinationSize < maxCompressedSize)
	{
		return RESULT_ERROR_BUFFER_TOO_SMALL;
	}

	const uint8_t* inputBuffer = static_cast<const uint8_t*>(source);
	uint8_t* outputBuffer = static_cast<uint8_t*>(destination);
	uint8_t* outputEnd = outputBuffer + destinationSize;
	assert((inputBuffer + sourceSize <= outputBuffer || inputBuffer >= outputEnd) && "The source and destination buffers must not overlap.");

	// Compute the maximum output end pointer
	// We use this to determine whether we should store the data instead of compressing it
	uint8_t* maxOutputEnd = outputBuffer + static_cast<size_t>(maxCompressedSize);

	// Allocate the header
	uint8_t* outputIterator = outputBuffer;
	outputIterator += getHeaderSize(maxCompressedSize);

	// Initialize the dictionary
	dictionary_.setBuffer(inputBuffer, sourceSize);

	// Initialize the control word which contains the literal/match bits
	// The highest bit of a control word is a guard bit, which marks the end of the bit list
	// The guard bit simplifies and speeds up the decoding process, and it 
	const int controlWordBitCount = WORD_SIZE * 8 - 1;
	const uint32_t controlWordGuardBit = 1u << controlWordBitCount;
	uint32_t controlWord = controlWordGuardBit;
	int controlWordBit = 0;

	// Since we do not know the contents of the control words in advance, we allocate space for them and subsequently fill them with data as soon as we can
	// This is necessary because the decoder must encounter a control word *before* the literals and matches it refers to
	// We begin the compressed data with a control word
	uint8_t* controlWordPointer = outputIterator;
	outputIterator += WORD_SIZE;

	// The match located at the current inputIterator position
	Match match;

	// The match located at the next inputIterator position
	// Initialize it to 'no match', because we are at the beginning of the inputIterator buffer
	// A match with a length of 0 means that there is no match
	Match nextMatch;
	nextMatch.length = 0;

	// The dictionary matching look-ahead is 1 character, so set the dictionary position to 1
	// We don't have to worry about getting matches beyond the inputIterator, because the dictionary ignores such requests
	dictionary_.skip();

	// At each position, we select the best match to encode from a list of match candidates provided by the match finder
	Match matchCandidates[MAX_MATCH_CANDIDATE_COUNT];
	int matchCandidateCount;

	// Iterate while there is still data left
	while (dictionary_.position() - 1 < sourceSize)
	{
		// Check whether the output is too large
		// During each iteration, we may output up to 8 bytes (2 words), and the compressed stream ends with 4 dummy bytes
		if (outputIterator + 2 * WORD_SIZE + TRAILING_DUMMY_SIZE > maxOutputEnd)
		{
			// Stop the compression and instead store
			return store(source, sourceSize, destination, compressedSize);
		}

		// Check whether the control word must be flushed
		if (controlWordBit == controlWordBitCount)
		{
			// Flush current control word
			fastWrite(controlWordPointer, controlWord, WORD_SIZE);

			// New control word
			controlWord = controlWordGuardBit;
			controlWordBit = 0;

			controlWordPointer = outputIterator;
			outputIterator += WORD_SIZE;
		}

		// The current match is the previous 'next' match
		match = nextMatch;

		// Find the best match at the next position
		// The dictionary position is automatically incremented
		matchCandidateCount = dictionary_.findMatches(matchCandidates);
		nextMatch = getBestMatch(matchCandidates, matchCandidateCount);

		// If we have a match, do not immediately use it, because we may miss an even better match (lazy evaluation)
		// If encoding a literal and the next match has a higher compression ratio than encoding the current match, discard the current match
		if (match.length > 0 && (1 + nextMatch.length) * getMatchCodedSize(match) > match.length * (1 + getMatchCodedSize(nextMatch)))
		{
			match.length = 0;
		}
		
		// Check whether we must encode a literal or a match
		if (match.length == 0)
		{
			// Encode a literal (0 control word flag)
			// In order to efficiently decode literals in runs, the literal bit (0) must differ from the guard bit (1)

			// The current dictionary position is now two characters ahead of the literal to encode
			assert(outputIterator + 1 <= outputEnd);
			fastWrite(outputIterator, inputBuffer[dictionary_.position() - 2], 1);
			++outputIterator;
		}
		else
		{
			// Encode a match (1 control word flag)
			controlWord |= 1 << controlWordBit;

			assert(outputIterator + WORD_SIZE <= outputEnd);
			outputIterator += encodeMatch(match, outputIterator);
			
			// Skip the matched characters
			for (int i = 0; i < match.length - 2; ++i)
			{
				dictionary_.skip();
			}

			matchCandidateCount = dictionary_.findMatches(matchCandidates);
			nextMatch = getBestMatch(matchCandidates, matchCandidateCount);
		}

		// Next control word bit
		++controlWordBit;
	}

	// Flush the control word
	fastWrite(controlWordPointer, controlWord, WORD_SIZE);

	// Output trailing safety dummy bytes
	// This reduces the number of necessary buffer checks during decoding
	assert(outputIterator + TRAILING_DUMMY_SIZE <= outputEnd);
	fastWrite(outputIterator, 0, TRAILING_DUMMY_SIZE);
	outputIterator += TRAILING_DUMMY_SIZE;

	// Done, compute the compressed size
	compressedSize = outputIterator - outputBuffer;

	// Encode the header
	Header header;
	header.version = VERSION;
	header.isStored = false;
	header.uncompressedSize = sourceSize;
	header.compressedSize = compressedSize;

	encodeHeader(header, maxCompressedSize, outputBuffer);

	// Return the compressed size
	return RESULT_OK;
}

// Store the source
Result Compressor::store(const void* source, size_t sourceSize, void* destination, size_t& compressedSize)
{
	uint8_t* outputBuffer = static_cast<uint8_t*>(destination);
	uint8_t* outputIterator = outputBuffer;

	// Encode the header
	uint64_t maxCompressedSize = getMaxCompressedSize(sourceSize);
	int headerSize = getHeaderSize(maxCompressedSize);

	compressedSize = headerSize + sourceSize;

	Header header;

	header.version = VERSION;
	header.isStored = true;
	header.uncompressedSize = sourceSize;
	header.compressedSize = compressedSize;

	encodeHeader(header, maxCompressedSize, destination);
	outputIterator += headerSize;

	// Store the data
	memcpy(outputIterator, source, sourceSize);

	return RESULT_OK;
}

// Selects the best match from a list of match candidates provided by the match finder
Match Compressor::getBestMatch(Match* matchCandidates, int matchCandidateCount)
{
	Match bestMatch;
	bestMatch.length = 0;

	// Select the longest match which can be coded efficiently (coded size is less than the length)
	for (int i = matchCandidateCount - 1; i >= 0; --i)
	{
		if (matchCandidates[i].length > getMatchCodedSize(matchCandidates[i]))
		{
			bestMatch = matchCandidates[i];
			break;
		}
	}
	
	return bestMatch;
}

int Compressor::encodeMatch(const Match& match, void* destination)
{
	assert(match.length <= MAX_MATCH_LENGTH);
	assert(match.length == 0 || match.offset < DICTIONARY_SIZE);

	uint32_t word;
	int size;

	uint32_t lengthCode = static_cast<uint32_t>(match.length - MIN_MATCH_LENGTH);
	uint32_t offsetCode = static_cast<uint32_t>(match.offset);

	if (lengthCode == 0 && offsetCode < 64)
	{
		word = offsetCode << 2; // 00
		size = 1;
	}
	else if (lengthCode == 0 && offsetCode < 16384)
	{
		word = (offsetCode << 2) | 1; // 01
		size = 2;
	}
	else if (lengthCode < 16 && offsetCode < 1024)
	{
		word = (offsetCode << 6) | (lengthCode << 2) | 2; // 10
		size = 2;
	}
	else if (lengthCode < 32 && offsetCode < 65536)
	{
		word = (offsetCode << 8) | (lengthCode << 3) | 3; // 11
		size = 3;
	}
	else
	{
		word = (offsetCode << 11) | (lengthCode << 3) | 7; // 111
		size = 4;
	}

	if (destination != 0)
	{
		fastWrite(destination, word, size);
	}

	return size;
}

int Compressor::getMatchCodedSize(const Match& match)
{
	return encodeMatch(match, 0);
}

int Compressor::getSizeCodedSize(uint64_t size)
{
	if (size <= UCHAR_MAX)
	{
		return 1;
	}

	if (size <= USHRT_MAX)
	{
		return 2;
	}

	if (size <= UINT_MAX)
	{
		return 4;
	}
	
	return 8;
}

int Compressor::getHeaderSize(uint64_t maxCompressedSize)
{
	return 1 + 2 * getSizeCodedSize(maxCompressedSize);
}

void Compressor::encodeHeader(const Header& header, uint64_t maxCompressedSize, void* destination)
{
	assert(header.version < 8);

	uint8_t* outputIterator = static_cast<uint8_t*>(destination);

	// Encode the attribute byte
	uint32_t attributes = header.version;

	uint32_t sizeCodedSize = getSizeCodedSize(maxCompressedSize);
	attributes |= (sizeCodedSize - 1) << 3;

	if (header.isStored)
	{
		attributes |= 128;
	}

	*outputIterator++ = static_cast<uint8_t>(attributes);

	// Encode the uncompressed and compressed sizes
	switch (sizeCodedSize)
	{
	case 1:
		*reinterpret_cast<uint8_t*>(outputIterator) = static_cast<uint8_t>(header.uncompressedSize);
		*reinterpret_cast<uint8_t*>(outputIterator + sizeCodedSize) = static_cast<uint8_t>(header.compressedSize);
		break;

	case 2:
		*reinterpret_cast<uint16_t*>(outputIterator) = static_cast<uint16_t>(header.uncompressedSize);
		*reinterpret_cast<uint16_t*>(outputIterator + sizeCodedSize) = static_cast<uint16_t>(header.compressedSize);
		break;

	case 4:
		*reinterpret_cast<uint32_t*>(outputIterator) = static_cast<uint32_t>(header.uncompressedSize);
		*reinterpret_cast<uint32_t*>(outputIterator + sizeCodedSize) = static_cast<uint32_t>(header.compressedSize);
		break;

	case 8:
		*reinterpret_cast<uint64_t*>(outputIterator) = header.uncompressedSize;;
		*reinterpret_cast<uint64_t*>(outputIterator + sizeCodedSize) = header.compressedSize;
		break;
	}
}

uint64_t Compressor::getMaxCompressedSize(uint64_t size)
{
	// The header + the original uncompressed data
	return getHeaderSize(UINT64_MAX) + size;
}

} // namespace doboz