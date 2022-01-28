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

#ifdef MSCOMP_WITH_XPRESS_HUFF

#include "../include/xpress_huff.h"
#include "../include/mscomp/Bitstream.h"
#include "../include/mscomp/HuffmanDecoder.h"

#define PRINT_ERROR(...) // TODO: remove

////////////////////////////// General Definitions and Functions ///////////////////////////////////
#define CHUNK_SIZE		0x10000
#define STREAM_END		0x100
#define SYMBOLS			0x200
#define HALF_SYMBOLS	0x100
#define HUFF_BITS_MAX	15
#define MIN_DATA		HALF_SYMBOLS + 4 // the 512 Huffman lens + 2 uint16s for minimal bitstream
typedef HuffmanDecoder<HUFF_BITS_MAX, SYMBOLS> Decoder;


////////////////////////////// Decompression Functions /////////////////////////////////////////////
static MSCompStatus xpress_huff_decompress_chunk(const_bytes* _in, const const_bytes in_end, bytes* _out, const const_bytes out_end, const const_bytes out_origin, Decoder *decoder)
{
	InputBitstream bstr(*_in, in_end);
	const const_bytes in_endx  = in_end - 13; // 6 bytes for the up-to 30 bits we may need (along with the 16-bit alignments the bitstream does) + 7 for an extra length bytes
	bytes out = *_out;
	const const_bytes out_endx = out_end - FAST_COPY_ROOM, out_end_chunk = out + CHUNK_SIZE, out_endx_chunk = MIN(out_end_chunk, out_endx);
	uint32_t len, off;
	uint_fast16_t sym;

	// Fast decompression - minimal bounds checking
	while (LIKELY(out < out_endx_chunk && bstr.RawStream() < in_endx))
	{
		sym = decoder->DecodeSymbolFast(&bstr);
		// TODO: figure out if the following line can ever happen, if not it gives up to a 5 MB/s speedup
		if (UNLIKELY(sym == INVALID_SYMBOL))	{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read enough bits for symbol\n"); return MSCOMP_DATA_ERROR; }
		if (sym < 0x100) { *out++ = (byte)sym; }
		else
		{
			const uint_fast8_t off_bits = (uint_fast8_t)((sym>>4) & 0xF);
			if ((len = sym & 0xF) == 0xF)
			{
				if ((len = bstr.ReadRawByte()) == 0xFF)
				{
					if (UNLIKELY((len = bstr.ReadRawUInt16()) == 0)) { len = bstr.ReadRawUInt32(); }
					if (UNLIKELY(len < 0xF))	{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Invalid length specified\n"); return MSCOMP_DATA_ERROR; }
					len -= 0xF;
				}
				len += 0xF;
			}
			len += 3;
			off = bstr.ReadBits_Fast(off_bits) | (1 << off_bits);
			const_bytes o = out-off;
			if (UNLIKELY(o < out_origin))		{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Invalid offset\n"); return MSCOMP_DATA_ERROR; }
			FAST_COPY(out, o, len, off, out_endx,
				if (UNLIKELY(out + len > out_end)) { return MSCOMP_BUF_ERROR; }
				goto CHECKED_COPY);
		}
	}

	// Slow decompression - full bounds checking
	while (out < out_end_chunk || !bstr.MaskIsZero()) /* end of chunk, not stream */
	{
		sym = decoder->DecodeSymbol(&bstr);
		if (UNLIKELY(sym == INVALID_SYMBOL))						{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read enough bits for symbol\n"); return MSCOMP_DATA_ERROR; }
		if (sym == STREAM_END && bstr.RemainingRawBytes() == 0 && bstr.MaskIsZero()) { *_in = bstr.RawStream(); *_out = out; return MSCOMP_STREAM_END; }
		if (sym < 0x100)
		{
			if (UNLIKELY(out == out_end))							{ PRINT_ERROR("XPRESS Huffman Decompression Error: Insufficient buffer\n"); return MSCOMP_BUF_ERROR; }
			*out++ = (byte)sym;
		}
		else
		{
			if ((len = sym & 0xF) == 0xF)
			{
				if (UNLIKELY(bstr.RemainingRawBytes() < 1))			{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read extra byte for length\n"); return MSCOMP_DATA_ERROR; }
				else if ((len = bstr.ReadRawByte()) == 0xFF)
				{
					if (UNLIKELY(bstr.RemainingRawBytes() < 2))		{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read two bytes for length\n"); return MSCOMP_DATA_ERROR; }
					if (UNLIKELY((len = bstr.ReadRawUInt16()) == 0))
					{
						if (UNLIKELY(bstr.RemainingRawBytes() < 4))	{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read four bytes for length\n"); return MSCOMP_DATA_ERROR; }
						len = bstr.ReadRawUInt32();
					}
					if (UNLIKELY(len < 0xF))						{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Invalid length specified\n"); return MSCOMP_DATA_ERROR; }
					len -= 0xF;
				}
				len += 0xF;
			}
			len += 3;
			{
				const uint_fast8_t off_bits = (uint_fast8_t)((sym>>4) & 0xF);
				if (UNLIKELY(off_bits > bstr.AvailableBits()))		{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Unable to read %u bits for offset\n", sym); return MSCOMP_DATA_ERROR; }
				off = bstr.ReadBits_Fast(off_bits) + (1 << off_bits);
			}
			if (UNLIKELY(out - off < out_origin))					{ PRINT_ERROR("XPRESS Huffman Decompression Error: Invalid data: Illegal offset (%p-%u < %p)\n", out, off, out_origin); return MSCOMP_DATA_ERROR; }
			if (UNLIKELY(out + len > out_end))						{ PRINT_ERROR("XPRESS Huffman Decompression Error: Insufficient buffer\n"); return MSCOMP_BUF_ERROR; }
			if (off == 1)
			{
				memset(out, out[-1], len);
				out += len;
			}
			else
			{
				const_bytes end;
CHECKED_COPY:	for (end = out + len; out < end; ++out) { *out = *(out-off); }
			}
		}
	}
	*_out = out;
	*_in = bstr.RawStream();
	if (decoder->DecodeSymbol(&bstr) == STREAM_END && bstr.RemainingRawBytes() == 0 && bstr.MaskIsZero())
	{
		*_in = bstr.RawStream();
		return MSCOMP_STREAM_END;
	}
	return MSCOMP_OK;
}
ENTRY_POINT MSCompStatus xpress_huff_decompress(const_bytes in, size_t in_len, bytes out, size_t* out_len)
{
	const const_bytes                  in_end  = in  + in_len;
	const const_bytes out_start = out, out_end = out + *out_len;
	MSCompStatus status;
	Decoder decoder;
	byte code_lengths[SYMBOLS];
	do
	{
		if (UNLIKELY(in_end - in < MIN_DATA))
		{
			if (in != in_end) { PRINT_ERROR("Xpress Huffman Decompression Error: Invalid Data: Less than %d input bytes\n", MIN_DATA); return MSCOMP_DATA_ERROR; }
			break;
		}
		for (uint_fast16_t i = 0, i2 = 0; i < HALF_SYMBOLS; ++i)
		{
			code_lengths[i2++] = (in[i] & 0xF);
			code_lengths[i2++] = (in[i] >>  4);
		}
		in += HALF_SYMBOLS;
		if (UNLIKELY(!decoder.SetCodeLengths(code_lengths))) { PRINT_ERROR("Xpress Huffman Decompression Error: Invalid Data: Unable to resolve Huffman codes\n"); return MSCOMP_DATA_ERROR; }
		status = xpress_huff_decompress_chunk(&in, in_end, &out, out_end, out_start, &decoder);
		if (UNLIKELY(status < MSCOMP_OK)) { return status; }
	} while (status != MSCOMP_STREAM_END);
	*out_len = out-out_start;
	return MSCOMP_OK;
}

#endif
