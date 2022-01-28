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


/////////////////// Configuration include file ////////////////////////////////////////////////////
// There are various configurations you can make. This is accomplished by defining preprocessor
// values like MSCOMP_WITH_x or MSCOMP_WITHOUT_x where x is one of the names below. By default, all
// are enabled.

// UNALIGNED_ACCESS - Allow unaligned access
// Without this option everything possibly unaligned is done with byte-level operations
#if !defined(MSCOMP_WITH_UNALIGNED_ACCESS) && !defined(MSCOMP_WITHOUT_UNALIGNED_ACCESS)
#define MSCOMP_WITH_UNALIGNED_ACCESS
#endif

// OPT_COMPRESS - Optimize compress(...)
// Without this option the single-call compressors are simple wrappers around the stream functions,
// otherwise they are specialized for single-calls.
#if !defined(MSCOMP_WITH_OPT_COMPRESS) && !defined(MSCOMP_WITHOUT_OPT_COMPRESS)
#define MSCOMP_WITH_OPT_COMPRESS
#endif

// OPT_DECOMPRESS - Optimize decompress(...)
// Without this option the single-call decompressors are simple wrappers around the stream
// functions, otherwise they are specialized for single-calls. Note: some decompressors may never
// provide a specialized version as it provides no benefit.
#if !defined(MSCOMP_WITH_OPT_DECOMPRESS) && !defined(MSCOMP_WITHOUT_OPT_DECOMPRESS)
#define MSCOMP_WITH_OPT_DECOMPRESS
#endif

// ERROR_MESSAGES - Include textual error messages
// Includes detailed error messages in the stream object (mainly for debugging, error codes are
// always given). This makes the stream object and compiled code larger.
#if !defined(MSCOMP_WITH_ERROR_MESSAGES) && !defined(MSCOMP_WITHOUT_ERROR_MESSAGES)
#define MSCOMP_WITH_ERROR_MESSAGES
#endif

// WARNING_MESSAGES - Include textual warning messages
// Includes detailed warning messages in the stream object (mainly for debugging). This makes the
// stream object and compiled code larger. Warning information is never available without this.
#if !defined(MSCOMP_WITH_WARNING_MESSAGES) && !defined(MSCOMP_WITHOUT_WARNING_MESSAGES)
#define MSCOMP_WITH_WARNING_MESSAGES
#endif

// LZNT1, XPRESS, XPRESS_HUFF, LZX
// Enable/disable support for a specific algorithm.
#if !defined(MSCOMP_WITH_LZNT1) && !defined(MSCOMP_WITHOUT_LZNT1)
#define MSCOMP_WITH_LZNT1
#endif
#if !defined(MSCOMP_WITH_XPRESS) && !defined(MSCOMP_WITHOUT_XPRESS)
#define MSCOMP_WITH_XPRESS
#endif
#if !defined(MSCOMP_WITH_XPRESS_HUFF) && !defined(MSCOMP_WITHOUT_XPRESS_HUFF)
#define MSCOMP_WITH_XPRESS_HUFF
#endif
#if !defined(MSCOMP_WITH_LZX) && !defined(MSCOMP_WITHOUT_LZX)
//#define MSCOMP_WITH_LZX // not working yet - never enable!
#endif


////////// Compressor-specific options //////////

// LZNT1_SA_DICT - Use the suffix array dictionary for LZNT1 compression
// The LZNT1 SA is about half as fast as the default dictionary but uses much less memory and does
// not dynamically allocate memory (so no memory errors, but needs at least 41 KB of stack space).
#if !defined(MSCOMP_WITH_LZNT1_SA_DICT) && !defined(MSCOMP_WITHOUT_LZNT1_SA_DICT)
#define MSCOMP_WITHOUT_LZNT1_SA_DICT
#endif
