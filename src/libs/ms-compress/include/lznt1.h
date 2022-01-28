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


//////////////// LZNT1 Compression and Decompression Functions ////////////////
// This algorithm is used for NTFS file compression, Windows 2000 hibernation file, Active
// Directory, File Replication Service, Windows Vista SuperFetch Files, and Windows Vista/7
// bootmgr.
//
// See mscomp.h for function descriptions.
//
// The algorithm is documented in the MSDN article [MS-XCA]:
// https://msdn.microsoft.com/library/hh554002.aspx
// which contains:
//   Algorithm: https://msdn.microsoft.com/library/jj665697.aspx
//   Example:   https://msdn.microsoft.com/library/jj711990.aspx
//
// Assumptions based on RtlCompressBuffer output on NT 3.51, NT 4 SP1, XP SP2, Win 7 SP1:
//   All flags besides the compressed flag are always 011 (binary)
//   Actual chunk size is 4096 bytes (regardless of requested chunk size)
//   All chunks represent 4096 bytes uncompressed bytes except the last one (tested using RtlDecompressBuffer)
//
// Differences between these and RtlDecompressBuffer and RtlCompressBuffer:
//   Higher memory usage for compression (variable, from 512 KB to several megabytes)
//   Decompression gets faster with better compression ratios
//   Compressed size has a nicer worst-case upper limit

#ifndef LZNT1_H
#define LZNT1_H
#include "mscomp/general.h"

#ifdef MSCOMP_WITH_LZNT1

EXTERN_C_START

MSCOMPAPI MSCompStatus lznt1_compress(const_bytes in, size_t in_len, bytes out, size_t* out_len);
MSCOMPAPI size_t lznt1_max_compressed_size(size_t in_len);

MSCOMPAPI MSCompStatus lznt1_decompress(const_bytes in, size_t in_len, bytes out, size_t* out_len);


MSCOMPAPI MSCompStatus lznt1_deflate_init(mscomp_stream* stream);
MSCOMPAPI MSCompStatus lznt1_deflate(mscomp_stream* stream, MSCompFlush flush);
MSCOMPAPI MSCompStatus lznt1_deflate_end(mscomp_stream* stream);

MSCOMPAPI MSCompStatus lznt1_inflate_init(mscomp_stream* stream);
MSCOMPAPI MSCompStatus lznt1_inflate(mscomp_stream* stream);
MSCOMPAPI MSCompStatus lznt1_inflate_end(mscomp_stream* stream);

EXTERN_C_END

#endif // MSCOMP_WITH_LZNT1

#endif
