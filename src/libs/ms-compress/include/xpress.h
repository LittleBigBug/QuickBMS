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


/////////////// Xpress Compression and Decompression Functions ////////////////
// This is the LZ version of the Xpress algorithm, used for Windows XP and newer hibernation file,
// Directory Replication Service (LDAP/RPC/AD), Windows Update Services, and Windows CE.
//
// See mscomp.h for function descriptions.
//
// The algorithm is documented in the MSDN article [MS-XCA]:
// https://msdn.microsoft.com/library/hh554002.aspx
// which contains (although with mistakes):
//   Compression Psuedo-Code:   https://msdn.microsoft.com/library/hh554053.aspx
//   Decompression Psuedo-Code: https://msdn.microsoft.com/library/hh536411.aspx
//   Example:                   https://msdn.microsoft.com/library/hh553843.aspx
//
// Differences from those documents found by testing the RTL functions are that:
//   supports 32-bit match lengths when the 16-bit match length is 0
//   There is always an end-of-stream flag even if we ended right at the end of the previous flags
// The RTL version also has some bugs:
//   cannot compress inputs of 7 bytes or less
//   requires at least 24 extra bytes in the compression output buffer


#ifndef XPRESS_H
#define XPRESS_H
#include "mscomp/general.h"

#ifdef MSCOMP_WITH_XPRESS

EXTERN_C_START

MSCOMPAPI MSCompStatus xpress_compress(const_bytes in, size_t in_len, bytes out, size_t* out_len);
MSCOMPAPI size_t xpress_max_compressed_size(size_t in_len);

MSCOMPAPI MSCompStatus xpress_decompress(const_bytes in, size_t in_len, bytes out, size_t* out_len);

MSCOMPAPI MSCompStatus xpress_deflate_init(mscomp_stream* stream);
MSCOMPAPI MSCompStatus xpress_deflate(mscomp_stream* stream, MSCompFlush flush);
MSCOMPAPI MSCompStatus xpress_deflate_end(mscomp_stream* stream);

MSCOMPAPI MSCompStatus xpress_inflate_init(mscomp_stream* stream);
MSCOMPAPI MSCompStatus xpress_inflate(mscomp_stream* stream);
MSCOMPAPI MSCompStatus xpress_inflate_end(mscomp_stream* stream);

EXTERN_C_END

#endif // MSCOMP_WITH_XPRESS

#endif
