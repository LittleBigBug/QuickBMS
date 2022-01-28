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

#include "../include/xpress.h"
#include "../include/mscomp/XpressDictionary.h"


#define MIN_DATA	5

typedef XpressDictionary<0x2000> Dictionary;

size_t xpress_max_compressed_size(size_t in_len) { return in_len + 4 + 4 * (in_len / 32); }

struct _mscomp_internal_state
{ // ?-? bytes
	bool finished;
	
	uint32_t flags, *out_flags;
	byte flag_count;
	byte* half_byte;
	
	//byte in[10];
	//size_t in_avail;

	byte out[10];
	size_t out_avail;
};

////////////////////////////// Compression Functions ///////////////////////////////////////////////
#define PRINT_ERROR(...) // TODO: remove
// TODO: streaming-compression not made yet
MSCompStatus xpress_deflate_init(mscomp_stream* stream)
{
#ifdef _DEBUG
	INIT_STREAM(stream, true, MSCOMP_XPRESS);

	mscomp_internal_state* state = (mscomp_internal_state*)malloc(sizeof(mscomp_internal_state));
	if (UNLIKELY(state == NULL)) { SET_ERROR(stream, "XPRESS Compression Error: Unable to allocate buffer memory"); return MSCOMP_MEM_ERROR; }
	state->finished  = false;
	state->flags = 0;
	state->out_flags = NULL;
	state->flag_count = 0;
	state->half_byte = NULL;
	state->out_avail = 0;

	//DictionaryStatic* d = (DictionaryStatic*)malloc(sizeof(DictionaryStatic));
	//if (UNLIKELY(d == NULL)) { free(state); SET_ERROR(stream, "XPRESS Compression Error: Unable to allocate dictionary memory"); return MSCOMP_MEM_ERROR; }
	//if (!(d = new (d) DictionaryStatic())->Initialized()) { free(state); d->~DictionaryStatic(); free(d); SET_ERROR(stream, "XPRESS Compression Error: Unable to allocate dictionary memory"); return MSCOMP_MEM_ERROR; }
	//state->d = d;

	stream->state = state;
	return MSCOMP_OK;
#else
	return MSCOMP_MEM_ERROR;
#endif
}
ENTRY_POINT MSCompStatus xpress_deflate(mscomp_stream* stream, MSCompFlush flush)
{
	// There will be one conceptual difference between the streaming and non-streaming versions.
	// The streaming version has to deal with the fact that the two partnered half-bytes might be
	// very far apart. To not take up too much memory, after 16kb (or more - up to 128kb?) of not
	// finding a partner for a half-byte the partner will be assumed to be 0x0 (forcing a length
	// of 10 the next time a length 10+ match is found). This adds at most 2 bytes to the output
	// for data that is already not compressing well (each time it occurs).
#ifdef _XDEBUG
	CHECK_STREAM_PLUS(stream, true, MSCOMP_XPRESS, stream->state == NULL || stream->state->finished);

	mscomp_internal_state *state = stream->state;

	const size_t out_avail = stream->out_avail;
	const_bytes in  = stream->in;  const const_bytes in_end  = in +stream->in_avail;
	      bytes out = stream->out; const       bytes out_end = out+stream->out_avail;
	// TODO: const_bytes filled_to = in;

	uint32_t flags = state->flags, *out_flags = (uint32_t*)state->out_flags;
	byte flag_count = state->flag_count;
	byte* half_byte = state->half_byte;

	// TODO:
	//if (finish && stream->in_avail == 0 && state->in_avail == 0)
	//{
	//	if (UNLIKELY(out_len < 4)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
	//	SET_UINT32(out, 0xFFFFFFFF);
	//	*_out_len = 4;
	//	return MSCOMP_OK;
	//}
	//if (!d.Initialized()) { return MSCOMP_MEM_ERROR; }

	if (out_flags == NULL)
	{
		*out_flags = out;
	}

	out += 4;		// skip four for flags
	*out++ = *in++;	// copy the first byte
	flag_count = 1;

	while (in < in_end-1 && out < out_end)
	{
		uint32_t len, off;
		if (filled_to <= in) { filled_to = d.Fill(filled_to); }
		len = d.Find(in, &off);

		if (in + len >= in_end)
		{
		}

		flags <<= 1;
		if (len < 3) { *out++ = *in++; } // Copy byte
		else // Match found
		{
			flags |= 1;
			in += len;
			len -= 3;
			SET_UINT16(out, ((off-1) << 3) | MIN(len, 7));
			out += 2;
			if (len >= 0x7)
			{
				len -= 0x7;
				if (half_byte)
				{
					*half_byte |= MIN(len, 0xF) << 4;
					half_byte = NULL;
				}
				else
				{
					if (out >= out_end) { TODO; }
					*(half_byte=out++) = (byte)(MIN(len, 0xF));
				}
				if (len >= 0xF)
				{
					len -= 0xF;
					if (out >= out_end) { state->out[state->out_avail++] = (byte)MIN(len, 0xFF); }
					else { *out++ = (byte)MIN(len, 0xFF); }
					if (len >= 0xFF)
					{
						len += 0xF+0x7;
						if (len <= 0xFFFF)
						{
							if (out + 2 > out_end)
							{
								ptrdiff_t rem = out_end - out;
								ALWAYS(0 <= rem && rem < 2);
								if (rem == 1) { byte buf[2]; SET_UINT16(buf, len); *out++ = buf[0]; state->out[0] = buf[1]; ++state->out_avail; }
								else /*if (rem == 0)*/ { SET_UINT16(state->out+state->out_avail, len); state->out_avail += 2; }
							}
							else { SET_UINT16(out, len); out += 2; }
						}
						else
						{
							if (out + 6 > out_end)
							{
								// TODO: this really could be improved...
								ptrdiff_t rem = out_end - out;
								ALWAYS(0 <= rem && rem < 6);
								byte buf[6];
								SET_UINT16(buf, 0);
								SET_UINT32(buf+2, len);
								memcpy(out, buf, rem);
								memcpy(state->out+state->out_avail, buf+rem, 6-rem);
								state->out_avail += 6-rem;
								out = out_end;
							}
							else { SET_UINT16(out, 0); SET_UINT32(out+2, len); out += 6; }
						}
					}
				}
			}
		}
		if (++flag_count == 32)
		{
			SET_UINT32(out_flags, flags);
			flag_count = 0;
			if (out + 4 > out_end) { TODO; }
			out_flags = (uint32_t*)out;
			out += 4;
		}
	}
	while (in < in_end && out < out_end)
	{
		*out++ = *in++;
		flags <<= 1;
		if (++flag_count == 32)
		{
			SET_UINT32(out_flags, flags);
			flag_count = 0;
			if (out + 4 > out_end) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
			out_flags = (uint32_t*)out;
			out += 4;
		}
	}
	// Finish shifting over flags and set all unused bytes to 1
	// Note: the shifting math does not effect flags at all when flag_count == 0, resulting in a copy of the previous flags so the proper value must be set manually
	// RTL produces improper output in this case as well, so the decompressor still must tolerate bad flags at the very end
	if (UNLIKELY(in != in_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
	flags = flag_count ? (flags << (32 - flag_count)) | ((1 << (32 - flag_count)) - 1) : 0xFFFFFFFF;
	SET_UINT32(out_flags, flags);
	*_out_len = out - out_start;
	return MSCOMP_OK;

#else
	return MSCOMP_ARG_ERROR;
#endif
}
MSCompStatus xpress_deflate_end(mscomp_stream* stream)
{
	CHECK_STREAM_PLUS(stream, true, MSCOMP_XPRESS, stream->state == NULL);

	MSCompStatus status = MSCOMP_OK;
	//if (UNLIKELY(!stream->state->finished || stream->in_avail || stream->state->in_avail || stream->state->out_avail)) { SET_ERROR(stream, "XPRESS Compression Error: End prematurely called"); status = MSCOMP_DATA_ERROR; }

	//// Cleanup
	//stream->state->d->~DictionaryStatic();
	//free(stream->state->d);
	//free(stream->state);
	//stream->state = NULL;

	return status;
}

#ifdef MSCOMP_WITH_OPT_COMPRESS
ENTRY_POINT MSCompStatus xpress_compress(const_bytes in, size_t in_len, bytes out, size_t* _out_len)
{
	const size_t out_len = *_out_len;
	const const_bytes                  in_end  = in +in_len,  in_end2  = in_end  - 2;
	const const_bytes out_start = out, out_end = out+out_len, out_end1 = out_end - 1;
	const_bytes filled_to = in;

	uint32_t flags = 0, *out_flags = (uint32_t*)out;
	byte flag_count;
	byte* half_byte = NULL;
	
	Dictionary d(in, in_end);

	if (in_len == 0)
	{
		if (UNLIKELY(out_len < 4)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
		SET_UINT32(out, 0xFFFFFFFF);
		*_out_len = 4;
		return MSCOMP_OK;
	}
	if (out_len < MIN_DATA) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }

	out += 4;		// skip four for flags
	*out++ = *in++;	// copy the first byte
	flag_count = 1;

	while (in < in_end2 && out < out_end1)
	{
		uint32_t len, off;
		if (filled_to <= in) { filled_to = d.Fill(filled_to); }
		flags <<= 1;
		if ((len = d.Find(in, &off)) < 3) { *out++ = *in++; } // Copy byte
		else // Match found
		{
			in += len;
			len -= 3;
			SET_UINT16(out, ((off-1) << 3) | MIN(len, 7));
			out += 2;
			if (len >= 0x7)
			{
				len -= 0x7;
				if (half_byte)
				{
					*half_byte |= MIN(len, 0xF) << 4;
					half_byte = NULL;
				}
				else
				{
					if (out >= out_end) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
					*(half_byte=out++) = (byte)(MIN(len, 0xF));
				}
				if (len >= 0xF)
				{
					len -= 0xF;
					if (UNLIKELY(out >= out_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
					*out++ = (byte)MIN(len, 0xFF);
					if (len >= 0xFF)
					{
						len += 0xF+0x7;
						if (len <= 0xFFFF)
						{
							if (UNLIKELY(out + 2 > out_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
							SET_UINT16(out, len);
							out += 2;
						}
						else
						{
							if (UNLIKELY(out + 6 > out_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
							SET_UINT16(out, 0);
							SET_UINT32(out+2, len);
							out += 6;
						}
					}
				}
			}
			flags |= 1;
		}
		if (++flag_count == 32)
		{
			SET_UINT32(out_flags, flags);
			flag_count = 0;
			if (UNLIKELY(out + 4 > out_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
			out_flags = (uint32_t*)out;
			out += 4;
		}
	}
	while (in < in_end && out < out_end)
	{
		*out++ = *in++;
		flags <<= 1;
		if (++flag_count == 32)
		{
			SET_UINT32(out_flags, flags);
			flag_count = 0;
			if (out + 4 > out_end) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
			out_flags = (uint32_t*)out;
			out += 4;
		}
	}
	// Finish shifting over flags and set all unused bytes to 1
	// Note: the shifting math does not effect flags at all when flag_count == 0, resulting in a copy of the previous flags so the proper value must be set manually
	// RTL produces improper output in this case as well, so the decompressor still must tolerate bad flags at the very end
	if (UNLIKELY(in != in_end)) { PRINT_ERROR("Xpress Compression Error: Insufficient buffer"); return MSCOMP_BUF_ERROR; }
	flags = flag_count ? (flags << (32 - flag_count)) | ((1 << (32 - flag_count)) - 1) : 0xFFFFFFFF;
	SET_UINT32(out_flags, flags);
	*_out_len = out - out_start;
	return MSCOMP_OK;
}
#else
ALL_AT_ONCE_WRAPPER_COMPRESS(xpress)
#endif

#endif
