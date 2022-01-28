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


/////////// Xpress Huffman Compression and Decompression Functions ////////////
// This is the Xpress compression used in WIM files - not the hibernation file.
//
// See mscomp.h for function descriptions.
//
// The algorithm is documented in the MSDN article [MS-XCA]:
// https://msdn.microsoft.com/library/hh554002.aspx
// which contains (although with mistakes):
//   Compression:   https://msdn.microsoft.com/library/hh554076.aspx
//   Decompression: https://msdn.microsoft.com/library/hh536379.aspx
//   Example:       https://msdn.microsoft.com/library/hh536484.aspx
//
// A mostly complete pseudo-code decompression implementation is given at: https://msdn.microsoft.com/library/dd644740.aspx
// The decompression code is similar to the given pseudo-code but has been optimized, can handle
// 64kb chunked data, and detects the end-of-stream instead of requiring the decompressed size to
// be known.
//
// The compression code is completely new and performs similar to the WIMGAPI compression ratio
// (time not tested).

#ifndef XPRESS_HUFF_H
#define XPRESS_HUFF_H
#include "mscomp/general.h"

#ifdef MSCOMP_WITH_XPRESS_HUFF

EXTERN_C_START

MSCOMPAPI MSCompStatus xpress_huff_compress(const_bytes in, size_t in_len, bytes out, size_t* out_len);
MSCOMPAPI size_t xpress_huff_max_compressed_size(size_t in_len);

MSCOMPAPI MSCompStatus xpress_huff_decompress(const_bytes in, size_t in_len, bytes out, size_t* out_len);

//MSCOMPAPI MSCompStatus xpress_huff_deflate_init(mscomp_stream* stream);
//MSCOMPAPI MSCompStatus xpress_huff_deflate(mscomp_stream* stream, MSCompFlush flush);
//MSCOMPAPI MSCompStatus xpress_huff_deflate_end(mscomp_stream* stream);

//MSCOMPAPI MSCompStatus xpress_huff_inflate_init(mscomp_stream* stream);
//MSCOMPAPI MSCompStatus xpress_huff_inflate(mscomp_stream* stream);
//MSCOMPAPI MSCompStatus xpress_huff_inflate_end(mscomp_stream* stream);

EXTERN_C_END

#endif // MSCOMP_WITH_XPRESS_HUFF

#endif
