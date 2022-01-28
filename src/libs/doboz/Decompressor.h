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

namespace doboz {

struct CompressionInfo
{
	uint64_t uncompressedSize;
	uint64_t compressedSize;
	int version;
};

class Decompressor
{
public:
	// Decompresses a block of data
	// The source and destination buffers must not overlap
	// This operation is memory safe
	// On success, returns RESULT_OK
	Result decompress(const void* source, size_t sourceSize, void* destination, size_t destinationSize);

	// Retrieves information about a compressed block of data
	// This operation is memory safe
	// On success, returns RESULT_OK and outputs the compression information
	Result getCompressionInfo(const void* source, size_t sourceSize, CompressionInfo& compressionInfo);

private:
	int decodeMatch(detail::Match& match, const void* source);
	Result decodeHeader(detail::Header& header, const void* source, size_t sourceSize, int& headerSize);
};

} // namespace doboz