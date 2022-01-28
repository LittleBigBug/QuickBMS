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

#include "Common.h"
#include "Dictionary.h"

namespace doboz {

class Compressor
{
public:
	// Returns the maximum compressed size of any block of data with the specified size
	// This function should be used to determine the size of the compression destination buffer
	static uint64_t getMaxCompressedSize(uint64_t size);

	// Compresses a block of data
	// The source and destination buffers must not overlap and their size must be greater than 0
	// This operation is memory safe
	// On success, returns RESULT_OK and outputs the compressed size
	Result compress(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize);

private:
	detail::Dictionary dictionary_;

	static int getSizeCodedSize(uint64_t size);
	static int getHeaderSize(uint64_t maxCompressedSize);

	Result store(const void* source, size_t sourceSize, void* destination, size_t& compressedSize);
	detail::Match getBestMatch(detail::Match* matchCandidates, int matchCandidateCount);
	int encodeMatch(const detail::Match& match, void* destination);
	int getMatchCodedSize(const detail::Match& match);
	void encodeHeader(const detail::Header& header, uint64_t maxCompressedSize, void* destination);
};

} // namespace doboz