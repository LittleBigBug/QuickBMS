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
#include "../include/mscomp.h"

#include "../include/lznt1.h"
#include "../include/xpress.h"
#include "../include/xpress_huff.h"

// First we give some simple no-compression 'compression' functions
MSCompStatus copy(const_bytes in, size_t in_len, bytes out, size_t* _out_len)
{
	if (in_len > *_out_len) { return MSCOMP_BUF_ERROR; }
	memcpy(out, in, in_len);
	*_out_len = in_len;
	return MSCOMP_OK;
}
size_t copy_max_size(size_t in_len) { return in_len; }
MSCompStatus copy_xxflate_init(mscomp_stream* stream) { INIT_STREAM(stream, true, MSCOMP_NONE); return MSCOMP_OK; }
MSCompStatus copy_deflate(mscomp_stream* stream, MSCompFlush flush)
{
	CHECK_STREAM(stream, true, MSCOMP_NONE);
	size_t n = stream->in_avail < stream->out_avail ? stream->in_avail : stream->out_avail;
	memcpy(stream->out, stream->in, n);
	stream->out       += n;
	stream->out_total += n;
	stream->out_avail -= n;
	stream->in        += n;
	stream->in_total  += n;
	stream->in_avail  -= n;
	return (flush == MSCOMP_FINISH && !stream->in_avail) ? MSCOMP_STREAM_END : MSCOMP_OK;
}
MSCompStatus copy_inflate(mscomp_stream* stream)
{
	CHECK_STREAM(stream, true, MSCOMP_NONE);
	size_t n = stream->in_avail < stream->out_avail ? stream->in_avail : stream->out_avail;
	memcpy(stream->out, stream->in, n);
	stream->out       += n;
	stream->out_total += n;
	stream->out_avail -= n;
	stream->in        += n;
	stream->in_total  += n;
	stream->in_avail  -= n;
	return MSCOMP_OK;
}
MSCompStatus copy_xxflate_end(mscomp_stream* stream) { CHECK_STREAM(stream, true, MSCOMP_NONE); return MSCOMP_OK; }



// Standard Compression and Decompression Functions

#ifdef MSCOMP_WITH_LZNT1
#define IF_WITH_LZNT1(x) x
#else
#define IF_WITH_LZNT1(x) NULL
#endif

#ifdef MSCOMP_WITH_XPRESS
#define IF_WITH_XPRESS(x) x
#else
#define IF_WITH_XPRESS(x) NULL
#endif

#ifdef MSCOMP_WITH_XPRESS_HUFF
#define IF_WITH_XPRESS_HUFF(x) x
#else
#define IF_WITH_XPRESS_HUFF(x) NULL
#endif

typedef size_t (*max_compressed_size_func)(size_t in_len);

static max_compressed_size_func max_compressed_sizers[] =
{
	copy_max_size,
	NULL,
	IF_WITH_LZNT1(lznt1_max_compressed_size),
	IF_WITH_XPRESS(xpress_max_compressed_size),
	IF_WITH_XPRESS_HUFF(xpress_huff_max_compressed_size),
};

MSCOMPAPI size_t ms_max_compressed_size(MSCompFormat format, size_t in_len)
{
	if ((unsigned)format >= ARRAYSIZE(max_compressed_sizers) || !max_compressed_sizers[format]) { return (size_t)-1; }
	return max_compressed_sizers[format](in_len);
}

typedef MSCompStatus (*compress_func)(const_bytes in, size_t in_len, bytes out, size_t* out_len);

static compress_func compressors[] =
{
	copy,
	NULL,
	IF_WITH_LZNT1(lznt1_compress),
	IF_WITH_XPRESS(xpress_compress),
	IF_WITH_XPRESS_HUFF(xpress_huff_compress),
};

MSCOMPAPI MSCompStatus ms_compress(MSCompFormat format, const_bytes in, size_t in_len, bytes out, size_t* out_len)
{
	if ((unsigned)format >= ARRAYSIZE(compressors) || !compressors[format]) { return MSCOMP_ARG_ERROR; }
	return compressors[format](in, in_len, out, out_len);
}

static compress_func decompressors[] =
{
	copy,
	NULL,
	IF_WITH_LZNT1(lznt1_decompress),
	IF_WITH_XPRESS(xpress_decompress),
	IF_WITH_XPRESS_HUFF(xpress_huff_decompress),
};

MSCOMPAPI MSCompStatus ms_decompress(MSCompFormat format, const_bytes in, size_t in_len, bytes out, size_t* out_len)
{
	if ((unsigned)format >= ARRAYSIZE(decompressors) || !decompressors[format]) { return MSCOMP_ARG_ERROR; }
	return decompressors[format](in, in_len, out, out_len);
}

// Streaming Compression and Decompression Functions

typedef MSCompStatus (*stream_func)(mscomp_stream* stream);
typedef MSCompStatus (*stream_flush_func)(mscomp_stream* stream, MSCompFlush flush);

static stream_func deflaters_init[] =
{
	copy_xxflate_init,
	NULL,
	IF_WITH_LZNT1(lznt1_deflate_init),
	IF_WITH_XPRESS(xpress_deflate_init),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_deflate_init),
};

static stream_flush_func deflaters[] =
{
	copy_deflate,
	NULL,
	IF_WITH_LZNT1(lznt1_deflate),
	IF_WITH_XPRESS(xpress_deflate),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_deflate),
};

static stream_func deflaters_end[] =
{
	copy_xxflate_end,
	NULL,
	IF_WITH_LZNT1(lznt1_deflate_end),
	IF_WITH_XPRESS(xpress_deflate_end),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_deflate_end),
};

MSCompStatus ms_deflate_init(MSCompFormat format, mscomp_stream* stream)
{
	if ((unsigned)format >= ARRAYSIZE(deflaters_init) || !deflaters_init[format]) { SET_ERROR(stream, "Error: Invalid format provided"); return MSCOMP_ARG_ERROR; }
	return deflaters_init[format](stream);
}
MSCompStatus ms_deflate(mscomp_stream* stream, MSCompFlush flush)
{
	if (stream == NULL || (unsigned)stream->format >= ARRAYSIZE(deflaters) || !deflaters[stream->format]) { SET_ERROR(stream, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }
	return deflaters[stream->format](stream, flush);
}
MSCompStatus ms_deflate_end(mscomp_stream* stream)
{
	if (stream == NULL || (unsigned)stream->format >= ARRAYSIZE(deflaters_end) || !deflaters_end[stream->format]) { SET_ERROR(stream, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }
	return deflaters_end[stream->format](stream);
}

static stream_func inflaters_init[] =
{
	copy_xxflate_init,
	NULL,
	IF_WITH_LZNT1(lznt1_inflate_init),
	IF_WITH_XPRESS(xpress_inflate_init),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_inflate_init),
};

static stream_func inflaters[] =
{
	copy_inflate,
	NULL,
	IF_WITH_LZNT1(lznt1_inflate),
	IF_WITH_XPRESS(xpress_inflate),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_inflate),
};

static stream_func inflaters_end[] =
{
	copy_xxflate_end,
	NULL,
	IF_WITH_LZNT1(lznt1_inflate_end),
	IF_WITH_XPRESS(xpress_inflate_end),
	// TODO: IF_WITH_XPRESS_HUFF(xpress_huff_inflate_end),
};

MSCompStatus ms_inflate_init(MSCompFormat format, mscomp_stream* stream)
{
	if ((unsigned)format >= ARRAYSIZE(inflaters_init) || !inflaters_init[format]) { SET_ERROR(stream, "Error: Invalid format provided"); return MSCOMP_ARG_ERROR; }
	return inflaters_init[format](stream);
}
MSCompStatus ms_inflate(mscomp_stream* stream)
{
	if (stream == NULL || (unsigned)stream->format >= ARRAYSIZE(inflaters) || !inflaters[stream->format]) { SET_ERROR(stream, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }
	return inflaters[stream->format](stream);
}
MSCompStatus ms_inflate_end(mscomp_stream* stream)
{
	if (stream == NULL || (unsigned)stream->format >= ARRAYSIZE(inflaters_end) || !inflaters_end[stream->format]) { SET_ERROR(stream, "Error: Invalid stream provided"); return MSCOMP_ARG_ERROR; }
	return inflaters_end[stream->format](stream);
}
