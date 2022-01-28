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


////////////// Microsoft Compression and Decompression Functions //////////////
// These mimic RtlCompressBuffer and RtlDecompressBuffer from NTDLL.DLL. They provide access to
// LZNT1, Xpress (LZ), and Xpress Huffman algorithms. They attempt to provide results similar to
// the RTL functions in terms of speed and compression rate. The provided functions always use the
// 'maximum engine'. For differences in the specific algorithms see their header files.
//
// Additionally, there are streaming versions of the compressors and decompressors.
//
// These functions are simple wrappers around the functions available in the other header files.


#ifndef MSCOMPRESSION_H
#define MSCOMPRESSION_H
#include "mscomp/general.h"

EXTERN_C_START

///////////////////////// Compression /////////////////////////////////////////
///// MSCompStatus ms_compress(             /////
/////        MSCompFormat format,           /////
/////        const_bytes in, size_t in_len, /////
/////        bytes out, size_t* out_len)    /////
//
// Compress the input buffer into the output buffer all in one go using the given format.
//
// This is essentially equivalent to the NTDLL function RtlCompressBuffer. To convert:
//     err = RtlCompressBuffer(FORMAT|MAX, in, in_len, out, out_len, *, &out_len, *); // chunk size and temporary buffer arguments are dropped
//     err = ms_compress(FORMAT, in, in_len, out, &out_len); // out_len is both in and out
//
// <format> is one of MSCOMP_NONE (0), MSCOMP_LZNT1 (2), MSCOMP_XPRESS (3), or
// MSCOMP_XPRESS_HUFF (4) which are numerically equivalent to the various COMPRESSION_FORMAT_*
// formats used by RtlCompressBuffer. The COMPRESSION_ENGINE_* are not supported (always MAXIMUM).
//
// <in> and <in_len> are the input buffer of bytes to be compressed and the length of that buffer.
//
// <out> is the output buffer, where to compress bytes to. <out_len> initially points to the length
// of the output buffer, and upon success contains the number of bytes used in the output buffer.
// If the function does not return successfully, the value pointed to by <out_len> is undefined.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), MSCOMP_MEM_ERROR (-4), or MSCOMP_BUF_ERROR (-5)).
MSCOMPAPI MSCompStatus ms_compress(MSCompFormat format, const_bytes in, size_t in_len, bytes out, size_t* out_len);

///////////////////////// Decompression ///////////////////////////////////////
///// MSCompStatus ms_decompress(           /////
/////        MSCompFormat format,           /////
/////        const_bytes in, size_t in_len, /////
/////        bytes out, size_t* out_len)    /////
//
// Decompress the input buffer into the output buffer all in one go using the given format.
//
// This is essentially equivalent to the NTDLL function RtlDecompressBuffer. To convert:
//     err = RtlDecompressBuffer(FORMAT, out, out_len, in, in_len, &out_len); // switched order of in and out!
//     err = ms_decompress(FORMAT, in, in_len, out, &out_len); // out_len is both in and out
//
// <format> is one of MSCOMP_NONE (0), MSCOMP_LZNT1 (2), MSCOMP_XPRESS (3), or
// MSCOMP_XPRESS_HUFF (4) which are numerically equivalent to the various COMPRESSION_FORMAT_*
// formats used by RtlDecompressBuffer.
//
// <in> and <in_len> are the input buffer of bytes to be decompressed and the length of that
// buffer.
//
// <out> is the output buffer, where to decompress bytes to. <out_len> initially points to the
// length of the output buffer, and upon success contains the number of bytes used in the output
// buffer. If the function does not return successfully, the value pointed to by <out_len> is
// undefined.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), MSCOMP_DATA_ERROR (-3), MSCOMP_MEM_ERROR (-4),
// or MSCOMP_BUF_ERROR (-5)).
MSCOMPAPI MSCompStatus ms_decompress(MSCompFormat format, const_bytes in, size_t in_len, bytes out, size_t* out_len);

///////////////////////// Max Compressed Size /////////////////////////////////
///// size_t ms_max_compressed_size(MSCompFormat format, size_t in_len) /////
//
// Calculate the upper bound of the length of compressed data based on the format.
//
// <format> is one of MSCOMP_NONE (0), MSCOMP_LZNT1 (2), MSCOMP_XPRESS (3), or
// MSCOMP_XPRESS_HUFF (4). <in_len> is the number of bytes that will be compressed.
//
// The return value is some value >=in_len.
MSCOMPAPI size_t ms_max_compressed_size(MSCompFormat format, size_t in_len);

///////////////////////// Deflate [Compress] - Streaming //////////////////////
///// MSCompStatus ms_deflate_init(MSCompFormat format, mscomp_stream* stream) /////
//
// Initialize a stream for compressing data of the given format. This initializes all values of the
// stream, however it your responsibility to set in, in_avail, out, and out_avail to usable values
// before calling ms_deflate.
//
// <format> is one of MSCOMP_NONE (0), MSCOMP_LZNT1 (2), MSCOMP_XPRESS (3), or
// MSCOMP_XPRESS_HUFF (4). <stream> is a pointer to a mscomp_stream structure with fields
// uninitialized.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), or MSCOMP_MEM_ERROR (-4)).
MSCOMPAPI MSCompStatus ms_deflate_init(MSCompFormat format, mscomp_stream* stream);

///// MSCompStatus ms_deflate(mscomp_stream* stream, MSCompFlush flush) /////
//
// Deflate as much as possible from a stream's input to its output.
//
// <stream> is a pointer to a mscomp_stream structure that is initialized for compression. The
// in, in_avail, out, and out_avail need to be set appropriately. If this returns successfully,
// either in_avail or out_avail will be 0. If this returns a failure, the stream will be in an
// indeterminate state and must be closed. The fields in, in_avail, in_total, out, out_avail, and
// out_total are updated, and if an error or warning is generated and support for messages is
// enabled then those fields are updated as well.
//
// <flush> is one of MSCOMP_NO_FLUSH (0), MSCOMP_FLUSH (2), or MSCOMP_FINISH (4). Some formats
// might not support MSCOMP_FLUSH and will generate a MSCOMP_ARG_ERROR if that is given.
// MSCOMP_NO_FLUSH is the value that should usually be given. MSCOMP_FLUSH forces data to not be
// buffered by such that if the call causes all input data to be consumed (in_avail==0) then the
// output contains all the data given to in and can be decompressed, while possibly sacrificing
// the compression ratio. MSCOMP_FINISH causes the compressor to assume what data is in the input
// buffer is the last to be compressed.
//
// If either MSCOMP_FLUSH or MSCOMP_FINISH is given, ms_deflate must be called repeatedly with the
// same flush value without modifying in or in_avail until in_avail is 0. If MSCOMP_FINISH is
// given, when in_avail reaches 0 the return value will be MSCOMP_STREAM_END. At this point no
// further calls to ms_deflate are allowed.
//
// The return value is MSCOMP_OK (0) if successful, MSCOMP_STREAM_END (1) if flush == MSCOMP_FINISH
// and all data in the input has been compressed. Or it is negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), or MSCOMP_MEM_ERROR (-4)).
MSCOMPAPI MSCompStatus ms_deflate(mscomp_stream* stream, MSCompFlush flush);

///// MSCompStatus ms_deflate_end(mscomp_stream* stream) /////
//
// Closes a compression stream. This does not compress any more data, but frees any resources used
// by the stream object. If this returns MSCOMP_ARG_ERROR, then the stream is not closed. In all
// other cases, the stream is closed and the stream is left in an uninitialized state (the values
// of in_total and out_total will be unchanged). If there was more data to compress (the last call
// to ms_inflate did not have flush set to MSCOMP_FINISH and did not return MSCOMP_STREAM_END),
// this will return MSCOMP_DATA_ERROR.
//
// <stream> is a pointer to a mscomp_stream structure that is initialized for compression.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (either
// MSCOMP_ARG_ERROR (-2) or MSCOMP_DATA_ERROR (-3)).
MSCOMPAPI MSCompStatus ms_deflate_end(mscomp_stream* stream);


///////////////////////// Inflate [Decompress] - Streaming ////////////////////
///// MSCompStatus ms_inflate_init(MSCompFormat format, mscomp_stream* stream) /////
//
// Initialize a stream for decompressing data of the given format. This initializes all values of
// the stream, however it your responsibility to set in, in_avail, out, and out_avail to usable
// values before calling ms_inflate.
//
// <format> is one of MSCOMP_NONE (0), MSCOMP_LZNT1 (2), MSCOMP_XPRESS (3), or
// MSCOMP_XPRESS_HUFF (4). <stream> is a pointer to a mscomp_stream structure with fields
// uninitialized.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), or MSCOMP_MEM_ERROR (-4)).
MSCOMPAPI MSCompStatus ms_inflate_init(MSCompFormat format, mscomp_stream* stream);

///// MSCompStatus ms_inflate(mscomp_stream* stream) /////
//
// Inflate as much as possible from a stream's input to its output.
//
// <stream> is a pointer to a mscomp_stream structure that is initialized for decompression. The
// in, in_avail, out, and out_avail need to be set appropriately. If this returns successfully,
// either in_avail or out_avail will be 0. If this returns a failure, the stream will be in an
// indeterminate state and must be closed. The fields in, in_avail, in_total, out, out_avail, and
// out_total are updated, and if an error or warning is generated and support for messages is
// enabled then those fields are updated as well.
//
// Most of the formats support do not have definite end-of-stream indications (special symbol,
// embedded decompressed length, or some form of CRC/hash) so there are many cases in which it may
// seem that the stream could be at its end, thus MSCOMP_POSSIBLE_STREAM_END is returned whenever
// it is possible that we are at the end of the stream, and if the caller knows there is no more
// input data, then it is the end of the stream, otherwise you should treat the return value as
// MSCOMP_OK. Some formats under some situations may return MSCOMP_STREAM_END. In these cases the
// stream has reached the end and ms_inflate can not be called with any more input data.
//
// The return value is MSCOMP_OK (0), MSCOMP_STREAM_END (1), or MSCOMP_POSSIBLE_STREAM_END (2)
// if successful (see above for explanation) or it is negative if an error occurred (one of
// MSCOMP_ERRNO (-1), MSCOMP_ARG_ERROR (-2), or MSCOMP_MEM_ERROR (-4)).
MSCOMPAPI MSCompStatus ms_inflate(mscomp_stream* stream);

///// MSCompStatus ms_inflate_end(mscomp_stream* stream) /////
//
// Closes a decompression stream. This does not decompress any more data, but frees any resources
// used by the stream object. If this returns MSCOMP_ARG_ERROR, then the stream is not closed.
// In all other cases, the stream is closed and the stream is left in an uninitialized state (the
// values of in_total and out_total will be unchanged). If there was more data to decompress
// (the last call to ms_inflate did not return MSCOMP_STREAM_END or MSCOMP_POSSIBLE_STREAM_END),
// this will return MSCOMP_DATA_ERROR.
//
// <stream> is a pointer to a mscomp_stream structure that is initialized for decompression.
//
// The return value is MSCOMP_OK (0) if successful or negative if an error occurred (either
// MSCOMP_ARG_ERROR (-2) or MSCOMP_DATA_ERROR (-3)).
MSCOMPAPI MSCompStatus ms_inflate_end(mscomp_stream* stream);

EXTERN_C_END

#endif
