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


// General include file which includes necessary files, defines, and typedefs.

#ifndef MSCOMP_GENERAL_H
#define MSCOMP_GENERAL_H
#include "config.h"

#include <stdint.h> // for uint8_t
#include <stddef.h> // for size_t

// There are 2 defines which effect the linkage of the public functions
// Define MSCOMP_API_DLL if you are either creating or using a dynamic library
// Define MSCOMP_API_EXPORT if you are creating a library (either static or dynamic)
// If you are bundling the code in your own code (and don't want/need the functions exported) or using a static library, define neither.
// If you are creating a dynamic library, define both.
#ifdef MSCOMP_API_DLL
	#ifdef MSCOMP_API_EXPORT
		#ifdef _WIN32
			#define MSCOMPAPI __declspec(dllexport)
		#else
			#define MSCOMPAPI extern // unnecessary but whatever
		#endif
	#elif defined(_WIN32)
		#define MSCOMPAPI __declspec(dllimport)
	#else
		#define MSCOMPAPI
	#endif
#else
	#define MSCOMPAPI
#endif

#ifndef __cplusplus
	#define EXTERN_C_START
	#define EXTERN_C_END
#else
	#define EXTERN_C_START extern "C" {
	#define EXTERN_C_END   }
#endif

// Define types used
typedef uint8_t byte; // should always be unsigned char (there is a check for CHAR_BIT == 8 in internal)
typedef byte* bytes;
typedef const byte const_byte;
typedef const_byte* const_bytes;

typedef struct _mscomp_internal_state mscomp_internal_state;

// Formats supported
typedef enum _MSCompFormat {
	MSCOMP_NONE			= 0, // COMPRESSION_FORMAT_NONE
	MSCOMP_RESERVED		= 1, // Called COMPRESSION_FORMAT_DEFAULT in MSDN but can never be used
	MSCOMP_LZNT1		= 2, // COMPRESSION_FORMAT_LZNT1
	MSCOMP_XPRESS		= 3, // COMPRESSION_FORMAT_XPRESS
	MSCOMP_XPRESS_HUFF	= 4  // COMPRESSION_FORMAT_XPRESS_HUFF
} MSCompFormat;

// Error Codes
typedef enum _MSCompStatus
{
	MSCOMP_OK					= 0,
	MSCOMP_STREAM_END			= 1,
	MSCOMP_POSSIBLE_STREAM_END	= 2,

	MSCOMP_ERRNO				= -1,
	MSCOMP_ARG_ERROR			= -2,
	MSCOMP_DATA_ERROR			= -3,
	MSCOMP_MEM_ERROR			= -4,
	MSCOMP_BUF_ERROR			= -5
} MSCompStatus;

// Flush Codes
typedef enum _MSCompFlush
{
	MSCOMP_NO_FLUSH  = 0,
	MSCOMP_FLUSH     = 2,
	MSCOMP_FINISH    = 4
} MSCompFlush;

// Compression Stream Object
typedef struct _mscomp_stream {
	MSCompFormat	format;
#ifdef __cplusplus
	bool		compressing;
#else
	int		compressing;
#endif

	const_bytes	in;			// next input byte
	size_t		in_avail;	// number of bytes available at next_in
	size_t		in_total;	// total number of input bytes read so far

	bytes		out;		// next output byte should be put there
	size_t		out_avail;	// remaining free space at next_out
	size_t		out_total;	// total number of bytes output so far

#ifdef MSCOMP_WITH_ERROR_MESSAGES
	char error[256];
#endif
#ifdef MSCOMP_WITH_WARNING_MESSAGES
	char warning[256];
#endif

	mscomp_internal_state* state;
} mscomp_stream;

#endif
