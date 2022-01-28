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
namespace detail {

class Dictionary
{
public:
	Dictionary();
	~Dictionary();

	void setBuffer(const uint8_t* buffer, size_t bufferLength);

	int findMatches(Match* matchCandidates);
	void skip();

	size_t position() const
	{
		return absolutePosition_;
	}

private:
	static const int HASH_TABLE_SIZE = 1 << 20;
	static const int CHILD_COUNT = DICTIONARY_SIZE * 2;
	static const int INVALID_POSITION = -1;
	static const int REBASE_THRESHOLD = (INT_MAX - DICTIONARY_SIZE + 1) / DICTIONARY_SIZE * DICTIONARY_SIZE; // must be a multiple of DICTIONARY_SIZE!

	// Buffer
	const uint8_t* buffer_; // pointer to the beginning of the buffer inside which we look for matches
	const uint8_t* bufferBase_; // bufferBase_ > buffer_, relative positions are necessary to support > 2 GB buffers
	size_t bufferLength_;
	size_t matchableBufferLength_;
	size_t absolutePosition_; // position from the beginning of buffer_

	// Cyclic dictionary
	int* hashTable_; // relative match positions to bufferBase_
	int* children_; // children of the binary tree nodes (relative match positions to bufferBase_)

	void initialize();

	int computeRelativePosition();
	uint32_t hash(const uint8_t* data);
};

} // namespace detail
} // namespace doboz