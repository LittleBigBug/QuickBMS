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


#include "../include/mscomp/internal.h"

#ifdef MSCOMP_WITH_XPRESS

#define MIN_DATA	5

#include "../include/xpress.h"
#include "../include/mscomp/CircularBuffer.h"
typedef CircularBuffer<0x2000> Buffer;

struct _mscomp_internal_state
{ // 38-42 bytes (+padding) + buffer
	uint32_t flagged, flags;
	byte half_byte;
	bool has_half_byte;
	byte in[10];
	size_t in_avail;
	Buffer buffer;
	uint_fast16_t copy_off;
	uint32_t copy_len;
};

// This function checks that a number is of the form 1..10..0 - basically all 1 bits are more significant than all 0 bits (also allowed are all 1s and all 0s)
static FORCE_INLINE bool set_bits_are_highest(uint32_t x) { x = ~x; return !((x+1) & x); }

////////////////////////////// Decompression Functions /////////////////////////////////////////////

MSCompStatus xpress_inflate_init(mscomp_stream* stream)
{
	INIT_STREAM(stream, false, MSCOMP_XPRESS);

	mscomp_internal_state* state = (mscomp_internal_state*)malloc(sizeof(mscomp_internal_state));
	if (UNLIKELY(state == NULL)) { SET_ERROR(stream, "XPRESS Decompression Error: Unable to allocate state memory"); return MSCOMP_MEM_ERROR; }

	state->flagged = 1;
	state->flags = 0;
	state->has_half_byte = false;
	state->in_avail  = 0;
	state->copy_len = 0;
	new (&state->buffer) Buffer();

	stream->state = state;
	return MSCOMP_OK;
}
#define _READ_SYMBOL(ERROR, LABEL) \
{ /*
	Reads a symbol from the input data.

	Arguments:
		ERROR  A macro that takes four arguments:
		        * an error string
		        * true/false if the problem is not enough bytes to read
				* number of bytes in has been advanced (not including a half-byte)
				* if the half-byte was updated (must do some work to determine if it was read or not)
		LABEL  A blank string or a label with a colon to be able to jump right before reading an extra full byte for length

	The code after the macro will execute when we are leaving fast mode at the end of a fragment.

	This makes use of the following local variables:
		in        in/out  input array of bytes, updated to the next byte to read (?)
		in_end    in      first invalid byte in in
		half_byte in      a pointer to a byte with the last half-byte length, or NULL if not available
		off       out     last offset read from a symbol
		len       out     last length read from a symbol
	*/ \
	if (UNLIKELY(in + 2 > in_end)) { ERROR("XPRESS Decompression Error: Invalid data: Unable to read 2 bytes for offset/length", true, 0, false); return MSCOMP_DATA_ERROR; } \
	{ \
		uint16_t sym = GET_UINT16(in); in += 2; \
		off = (sym >> 3) + 1; len = sym & 0x7; \
	} \
	if (len == 0x7) \
	{ \
		if (half_byte) { len = *half_byte >> 4; half_byte = NULL; } \
		else if (UNLIKELY(in == in_end)) { ERROR("XPRESS Decompression Error: Invalid data: Unable to read a half-byte for length", true, 2, false); return MSCOMP_DATA_ERROR; } \
		else { len = *(half_byte = in++) & 0xF; }; \
		if (len == 0xF) \
		{ \
			if (UNLIKELY(in == in_end)) { ERROR("XPRESS Decompression Error: Invalid data: Unable to read a byte for length", true, 2, true); return MSCOMP_DATA_ERROR; } \
			if ((len = *(in++)) == 0xFF) \
			{ \
LABEL			if (UNLIKELY(in + 2 > in_end)) { ERROR("XPRESS Decompression Error: Invalid data: Unable to read two bytes for length", true, 3, true); return MSCOMP_DATA_ERROR; } \
				len = GET_UINT16(in); in += 2; \
				if (UNLIKELY(len == 0)) \
				{ \
					if (UNLIKELY(in + 4 > in_end)) { ERROR("XPRESS Decompression Error: Invalid data: Unable to read four bytes for length", true, 5, true); return MSCOMP_DATA_ERROR; } \
					len = GET_UINT32(in); in += 4; \
				} \
				if (UNLIKELY(len < 0xF+0x7)) { ERROR("XPRESS Decompression Error: Invalid data: Invalid length", false, 9, true); return MSCOMP_DATA_ERROR; } \
				len -= 0xF+0x7; \
			} \
			len += 0xF; \
		} \
		len += 0x7; \
	} \
	len += 0x3; \
}
#define READ_SYMBOL(ERROR) _READ_SYMBOL(ERROR,)
#define READ_SYMBOL_WITH_LABEL(ERROR, LABEL) _READ_SYMBOL(ERROR,LABEL:)

#define IN_NEAR_END  0x074; // 4 + 32 * (2 + 0.5 + 1) from the end, or maybe 4 + 32 * (2 + 0.5 + 1 + 2 + 4) = 0x134
#define OUT_NEAR_END 32*FAST_COPY_ROOM;
#define INFLATE_FAST(ERROR, CHECKED_LENGTH, CHECKED_COPY) \
{ /*
	Fast decompression loop with many shortcuts and assumptions. Most of the decompression happens
	here with very few bounds checks but we can only get to within a few hundred bytes of the end.

	This can only be started at the beginning of a fragment/chunk (about to read the flags) and the
	output (from out_start to out) must contain enough bytes for the look-back (up to 0x2000).

	Arguments:
		ERROR          A macro that takes a single argument: an error string
		CHECKED_LENGTH Code to execute when we are leaving fast mode in the middle of getting a length
		CHECKED_COPY   Code to execute when we are leaving fast mode in the middle of a block copy

	The code after the macro will execute when we are leaving fast mode at the end of a fragment.

	This makes use of the following local variables:
		in        in/out  input array of bytes, updated to the next byte to read
		in_end    in      first invalid byte in in
		in_endx   in      IN_NEAR_END from in_end
		out       in/out  output array of bytes, updated to the next byte to write
		out_end   in      first invalid byte in out
		out_endx  in      OUT_NEAR_END from out_end
		out_start in      the start of the output bytes (may be <out if we know there is buffer data before out)
		half_byte in      a pointer to a byte with the last half-byte length, or NULL if not available
		flagged   out     current flag state (either 0 or non-zero)
		flags     out     remaining flags in current fragment w/ a sentinel (if 0, then fragment is complete)
		off       out     last offset read from a symbol
		len       out     last length read from a symbol, reduced if some were copied
	*/ \
	while (LIKELY(in < in_endx && out < out_endx)) \
	{ \
		/* Start a fragment */ \
		flags = GET_UINT32(in); \
		flagged = flags & 0x80000000; \
		flags = (flags << 1) | 1; \
		in += 4; \
		do \
		{ \
			if (flagged) /* Either: offset/length symbol, end of flags, or end of stream (only happens in non-fast versions) */ \
			{ \
				/* Offset/length symbol */ \
				const uint16_t sym = GET_UINT16(in); in += 2; \
				off = (sym >> 3) + 1; \
				if ((len = sym & 0x7) == 0x7) \
				{ \
					if (half_byte) { len = *half_byte >> 4; half_byte = NULL; } \
					else           { len = *(half_byte = in++) & 0xF; } \
					if (len == 0xF) \
					{ \
						if ((len = *(in++)) == 0xFF) \
						{ \
							if (UNLIKELY(in + 6 > in_endx)) { CHECKED_LENGTH; } \
							len = GET_UINT16(in); in += 2; \
							if (UNLIKELY(len == 0)) { len = GET_UINT32(in); in += 4; } \
							if (UNLIKELY(len < 0xF+0x7)) { ERROR("XPRESS Decompression Error: Invalid data: Invalid length"); return MSCOMP_DATA_ERROR; } \
							len -= 0xF+0x7; \
						} \
						len += 0xF; \
					} \
					len += 0x7; \
				} \
				len += 0x3; \
				const_bytes o = out-off; \
				if (UNLIKELY(o < out_start)) { ERROR("XPRESS Decompression Error: Invalid data: Invalid offset"); return MSCOMP_DATA_ERROR; } \
				FAST_COPY(out, o, len, off, out_endx, CHECKED_COPY); \
				flagged = flags & 0x80000000; \
				flags <<= 1; \
			} \
			else /* Copy up to 32 bytes directly */ \
			{ \
				int n = count_leading_zeros(flags) + 1; \
				ALWAYS(0 < n && n <= 32); \
				flagged = 1; \
				flags = (uint32_t)(((uint64_t)flags) << n); \
				COPY_128_FAST(out, in); if (n > 16) { COPY_128_FAST(out+16, in+16); } \
				out += n; in += n; \
			} \
		} while (LIKELY(flags)); \
	} \
}

#define INFLATE_SYNC_STATE() \
	state->has_half_byte = half_byte != NULL; \
	if (state->has_half_byte) { state->half_byte = *half_byte; } \
	state->flags = flags; \
	state->flagged = flagged
#define READ_ALL_IN_NO_SYNC() \
	size_t _out_len = out - stream->out; \
	stream->in_total  += in_len;   stream->in_avail   = 0;        stream->in  = in_end; \
	stream->out_total += _out_len; stream->out_avail -= _out_len; stream->out = out
#define READ_ALL_IN() INFLATE_SYNC_STATE(); READ_ALL_IN_NO_SYNC()
#define WROTE_ALL_OUT() \
	INFLATE_SYNC_STATE(); \
	size_t in_len = in - stream->in; \
	stream->in_total  += in_len;  stream->in_avail -= in_len; stream->in  = in; \
	stream->out_total += out_len; stream->out_avail = 0;      stream->out = out_end
#define SET_STREAM_ERROR(MSG) SET_ERROR(stream, MSG)
#define READ_SYMBOL_PART_ERROR(MSG, NOT_ENOUGH_BYTES, BYTES_ADVANCED, ...) \
	WARNINGS_PUSH() \
	WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT() \
	if (LIKELY(NOT_ENOUGH_BYTES)) { ADVANCE_IN(stream, to_copy); state->in_avail += to_copy; return !BYTES_ADVANCED && !stream->in_avail && !state->in_avail && set_bits_are_highest(state->flags) ? MSCOMP_POSSIBLE_STREAM_END : MSCOMP_OK; } \
	WARNINGS_POP() \
	SET_STREAM_ERROR(MSG)
#define READ_SYMBOL_ERROR(MSG, NOT_ENOUGH_BYTES, BYTES_ADVANCED, HALF_BYTE_UPDATED) \
	WARNINGS_PUSH() \
	WARNINGS_IGNORE_CONDITIONAL_EXPR_CONSTANT() \
	if (LIKELY(NOT_ENOUGH_BYTES)) \
	{ \
		in -= BYTES_ADVANCED; \
		if (!HALF_BYTE_UPDATED) \
		{ \
			state->has_half_byte = half_byte != NULL; \
			if (state->has_half_byte) { state->half_byte = *half_byte; } \
		} \
		else if (half_byte) { state->has_half_byte = false; --in; } \
		else { state->has_half_byte = true; state->half_byte = 0xF0; } \
		memcpy(state->in, in, state->in_avail=in_end-in); \
		state->flags = flags; \
		state->flagged = flagged; \
		READ_ALL_IN_NO_SYNC(); \
		return !BYTES_ADVANCED && !stream->in_avail && !state->in_avail && set_bits_are_highest(state->flags) ? MSCOMP_POSSIBLE_STREAM_END : MSCOMP_OK; \
	} \
	WARNINGS_POP() \
	SET_STREAM_ERROR(MSG);
#define DO_NOTHING(...)

WARNINGS_PUSH()
WARNINGS_IGNORE_POTENTIAL_UNINIT_VALRIABLE_USED()
ENTRY_POINT MSCompStatus xpress_inflate(mscomp_stream* stream)
{
	CHECK_STREAM_PLUS(stream, false, MSCOMP_XPRESS, stream->state == NULL);

	mscomp_internal_state *state = stream->state;
	Buffer* const buf = &state->buffer;
	const bytes out_start = stream->out;

	// Copy data from the buffer to the output
	if (state->copy_len)
	{
		if (state->copy_len > stream->out_avail)
		{
			buf->copy(state->copy_off, stream->out_avail, out_start);
			ADVANCE_OUT_TO_END(stream);
			state->copy_len -= (uint32_t)stream->out_avail;
			return MSCOMP_OK;
		}
		buf->copy(state->copy_off, state->copy_len, out_start);
		ADVANCE_OUT(stream, state->copy_len);
		state->copy_len = 0;
		state->flagged = state->flags & 0x80000000;
		state->flags <<= 1;
	}

	// Examine the state and restart from the last operation
	const_byte* half_byte = state->has_half_byte ? &state->half_byte : NULL;
	uint32_t flags = state->flags, flagged = state->flagged, len;
	uint_fast16_t off;
	int starting_place = 0;
	if (!flags)
	{
		// Start with a new flag
		if (state->in_avail != 0)
		{
			if (UNLIKELY(stream->in_avail + state->in_avail < 4))
			{
				memcpy(state->in+state->in_avail, stream->in, stream->in_avail);
				state->in_avail += stream->in_avail;
				ADVANCE_IN_TO_END(stream);
				return MSCOMP_OK;
			}
			memcpy(state->in+state->in_avail, stream->in, 4-state->in_avail);
			ADVANCE_IN(stream, 4-state->in_avail);
			const_bytes in = state->in;
			flagged = (flags = GET_UINT32(in)) & 0x80000000;
			flags = (flags << 1) | 1;
			state->in_avail = 0;
			starting_place = 1; // "MAIN_LOOP"
		}
	}
	else if (flagged)
	{
		// Start with a symbol + extra lengths
		size_t to_copy = MIN(10-state->in_avail, stream->in_avail);
		ALWAYS(to_copy <= 10);
		memcpy(state->in+state->in_avail, stream->in, to_copy);
		const_bytes in = state->in;
		const const_bytes in_end = in + state->in_avail + to_copy;
		READ_SYMBOL(READ_SYMBOL_PART_ERROR);
		size_t used = (in - state->in) - state->in_avail;
		ADVANCE_IN(stream, used);
		state->in_avail = 0;
		starting_place = 2; // "COPY_DATA"
	}
	else
	{
		// Start with copying a single byte
		if (UNLIKELY(!stream->in_avail)) { return MSCOMP_OK; }
		starting_place = 3; // "COPY_BYTE"
	}

	// Make local copies of these stream variables so we can use them quickly
	const size_t in_len = stream->in_avail, out_len = stream->out_avail;
	const_bytes in  = stream->in;  const const_bytes in_end  = in  + in_len,  in_endx  = in_end  - IN_NEAR_END;
	bytes       out = stream->out; const bytes       out_end = out + out_len, out_endx = out_end - OUT_NEAR_END;

	// Restart the loop from where we left off
	switch (starting_place)
	{
		case 0: break;
		case 1: goto MAIN_LOOP;
		case 2: goto COPY_DATA;
		case 3: goto COPY_BYTE;
		default: UNREACHABLE();
	}
	while (LIKELY(in + 4 <= in_end))
	{
		if (out-buf->size() >= out_start && in < in_endx && out < out_endx)
		{
			// Switch to fast decompression mode
			const const_bytes out_fast_start = out;
			INFLATE_FAST(SET_STREAM_ERROR,
				buf->push_back(out_fast_start, out-out_fast_start); goto CHECKED_LENGTH,
				buf->push_back(out_fast_start, out-out_fast_start); goto COPY_DATA);
			buf->push_back(out_fast_start, out-out_fast_start); continue;
		}

		// Start a fragment
		flagged = (flags = GET_UINT32(in)) & 0x80000000;
		flags = (flags << 1) | 1;
		in += 4;
MAIN_LOOP:
		do
		{
			if (in == in_end) { READ_ALL_IN(); return state->flagged && set_bits_are_highest(state->flags) ? MSCOMP_POSSIBLE_STREAM_END : MSCOMP_OK; } // We have read all the we can for now, save state and quit
			else if (flagged) // Either: offset/length symbol, end of flags, or end of stream (checked above)
			{
				READ_SYMBOL_WITH_LABEL(READ_SYMBOL_ERROR, CHECKED_LENGTH);
COPY_DATA:
				if (UNLIKELY(buf->size() < off)) { SET_ERROR(stream, "XPRESS Decompression Error: Invalid data: Illegal offset"); return MSCOMP_DATA_ERROR; }
				size_t out_rem = out_end-out;
				if (len > out_rem)
				{
					buf->copy(off, out_rem, out);
					// We have written all the we can for now, save state and quit
					state->copy_len = (uint32_t)(len - out_rem);
					state->copy_off = off;
					WROTE_ALL_OUT();
					return MSCOMP_OK;
				}
				buf->copy(off, len, out);
				out += len;
			}
			else
			{
COPY_BYTE:
				if (out == out_end) { WROTE_ALL_OUT(); return MSCOMP_OK; } // We have written all the we can for now, save state and quit
				else { buf->push_back(*out++ = *in++); } // Copy byte directly
			}
			flagged = flags & 0x80000000;
			flags <<= 1;
		} while (LIKELY(flags));
	}
	state->in_avail = in_end - in;
	ALWAYS(state->in_avail <= 3);
	memcpy(state->in, in, state->in_avail); // at most 3 bytes
	READ_ALL_IN();
	return MSCOMP_OK;
}
WARNINGS_POP()
MSCompStatus xpress_inflate_end(mscomp_stream* stream)
{
	CHECK_STREAM_PLUS(stream, false, MSCOMP_XPRESS, stream->state == NULL);

	mscomp_internal_state* state = stream->state;

	MSCompStatus status = MSCOMP_OK;
	if (UNLIKELY(stream->in_avail || state->in_avail || !state->flagged || !set_bits_are_highest(state->flags))) { SET_ERROR(stream, "XPRESS Decompression Error: End prematurely called"); status = MSCOMP_DATA_ERROR; }

	// Cleanup
	state->buffer.~Buffer();
	free(state);
	stream->state = NULL;

	return status;
}
#ifdef MSCOMP_WITH_OPT_DECOMPRESS
ENTRY_POINT MSCompStatus xpress_decompress(const_bytes in, size_t in_len, bytes out, size_t* _out_len)
{
	const size_t out_len = *_out_len;
	const const_bytes                  in_end  = in +in_len,  in_endx  = in_end -IN_NEAR_END;
	const const_bytes out_start = out, out_end = out+out_len, out_endx = out_end-OUT_NEAR_END;
	const_byte* half_byte = NULL;
	uint32_t flags, flagged, len;
	uint_fast16_t off;

	if (in_len < MIN_DATA)
	{
		if (LIKELY(in_len == 0 || (in_len == 4 && (*(uint32_t*)in) != 0xFFFFFFFF))) { *_out_len = 0; return MSCOMP_OK; }
		return MSCOMP_DATA_ERROR;
	}

	INFLATE_FAST(DO_NOTHING, goto CHECKED_LENGTH,
		if (UNLIKELY(out + len > out_end)) { return MSCOMP_BUF_ERROR; }
		goto CHECKED_COPY);

	// Slower decompression but with full bounds checking
	while (LIKELY(in + 4 <= in_end))
	{
		// Start a fragment
		flagged = (flags = GET_UINT32(in)) & 0x80000000;
		flags = (flags << 1) | 1;
		in += 4;
		do
		{
			if (in == in_end)
			{
				if (UNLIKELY(!flagged || !set_bits_are_highest(flags))) { return MSCOMP_DATA_ERROR; }
				*_out_len = out - out_start;
				return MSCOMP_OK;
			}
			else if (flagged) // Either: offset/length symbol, end of flags, or end of stream (checked above)
			{
				READ_SYMBOL_WITH_LABEL(DO_NOTHING, CHECKED_LENGTH);
				if (UNLIKELY(out - off < out_start)) { return MSCOMP_DATA_ERROR; }
				if (UNLIKELY(out + len > out_end))   { return MSCOMP_BUF_ERROR; }
				if (off == 1)
				{
					memset(out, out[-1], len);
					out += len;
				}
				else
				{
					const_bytes end;
CHECKED_COPY:		for (end = out + len; out < end; ++out) { *out = *(out-off); }
				}
			}
			else if (UNLIKELY(out == out_end)) { return MSCOMP_BUF_ERROR; }
			else { *out++ = *in++; } // Copy byte directly
			flagged = flags & 0x80000000;
			flags <<= 1;
		} while (LIKELY(flags));
	}
	return MSCOMP_DATA_ERROR;
}
#else
ALL_AT_ONCE_WRAPPER_DECOMPRESS(xpress)
#endif

#endif
