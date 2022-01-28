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


////////////////////////////// Circular Buffer /////////////////////////////////////////////////////
// A circular buffer is one that stores a limited number of elements, and as you try to store more
// that the oldest elements are erased. These are optimized for the compression algorithms used
// here.

#ifndef MSCOMP_BITSTREAM_H
#define MSCOMP_BITSTREAM_H
#include "internal.h"

template<uint32_t Size>
class CircularBuffer
{
	byte buffer[Size];
	const bytes end;
	bytes tail;
	bool full;
public:
	INLINE CircularBuffer() : end(buffer+Size), tail(buffer), full(false) { }
	INLINE CircularBuffer(const CircularBuffer<Size>& rhs) : end(buffer+Size), tail(buffer+(rhs.tail-rhs.buffer)), full(rhs.full) { memcpy(this->buffer, rhs.buffer, Size); }
	INLINE CircularBuffer<Size>& operator=(const CircularBuffer<Size>& rhs)
	{
		memcpy(this->buffer, rhs.buffer, Size);
		this->tail = this->buffer + (rhs.tail - rhs.buffer);
		this->full = rhs.full;
	}
	INLINE bool empty() const { return UNLIKELY(!this->full && this->tail == this->buffer); }
	INLINE uint32_t size() const { return LIKELY(this->full) ? Size : (uint32_t)(this->tail - this->buffer); }
	INLINE uint32_t max_size() const { return Size; }
	INLINE uint32_t capacity() const { return Size; }

	INLINE void copy(const uint32_t off, const size_t len, bytes dest)
	{
		// TODO: this could be made more efficient, particularly with off == 1 and the copying loop near the end

		// Copies a region from the buffer into the destination buffer along with writing it to the end 
		// of the buffer itself.
		// Assumptions: off < MIN(# of bytes pushed, Size) and dest has room for len bytes
		// This does support len > off with repeated data
		const byte* start = this->tail - off;
		const size_t n = MIN(len, off);
		if (start >= this->buffer) { memcpy(dest, start, n); }
		else
		{
			size_t space = this->buffer - start;
			if (len <= space) { memcpy(dest, start+Size, len); }
			else
			{
				memcpy(dest, start+Size, space);
				memcpy(dest+space, this->buffer, n-space);
			}
		}
		// At this point we have n bytes copied to dest from the buffer
		// There are rem bytes to go, but they are repeats of what is already in dest
		for (size_t i = n; i < len; ++i) { dest[i] = dest[i-off]; }

		// Update the buffer itself
		this->push_back(dest, len);
	}
	INLINE void push_back(const byte x)
	{
		*this->tail = x;
		if (++this->tail == this->end) { this->tail = this->buffer; this->full = true; }
	}
	INLINE void push_back(const const_bytes x, const size_t n)
	{
		if (n >= Size)
		{
			memcpy(this->buffer, x+n-Size, Size);
			this->tail = this->buffer;
			this->full = true;
		}
		else
		{
			const size_t space = this->end - this->tail;
			if (n <= space)
			{
				memcpy(this->tail, x, n);
				if ((this->tail += n) == this->end) { this->tail = this->buffer; this->full = true; }
			}
			else
			{
				memcpy(this->tail, x, space);
				memcpy(this->buffer, x+space, n-space);
				this->tail = this->buffer + n-space;
				this->full = true;
			}
		}
	}
	INLINE void clear()
	{
		this->tail = this->buffer;
		this->full = false;
	}
};

#endif
