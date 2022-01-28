/*---------------------------------------------*/
/* Zzip/Zzlib compressor              global.h */
/*---------------------------------------------*/

/*
  This file is a part of zzip and/or zzlib, a program and
  library for lossless, block-sorting data compression.
  Copyright (C) 1999-2001 Damien Debin. All Rights Reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the 
  Free Software Foundation, Inc., 
  59 Temple Place, Suite 330, 
  Boston, MA 02111-1307 USA

  Damien Debin
  <damien@debin.net>

  This program is based on (at least) the work of: Mike Burrows, 
  David Wheeler, Peter Fenwick, Alistair Moffat, Ian H. Witten, 
  Robert Sedgewick, Jon Bentley, Brenton Chapin, Stephen R. Tate, 
  Szymon Grabowski, Bernhard Balkenhol, Stefan Kurtz
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>

#ifdef __GNUC__
# define INLINE   __inline__
# define ALIGN    __attribute__ ((aligned(16)))
# define NORETURN __attribute__ ((noreturn))
#elif  _VISUALC
# define INLINE   __inline
# define ALIGN
# define NORETURN
#else
# define INLINE
# define ALIGN
# define NORETURN
#endif

#define VERSION_STRING		"v0.36c (04-Jun-2001)"
#define VERSION_NUMBER		49
#define MAX_FILES			65535
#define RUN_LENGTH_MAX		512
#define FILENAME_LENGTH_MAX (256+4+1)
#define DEFAULT_BLOCK_SIZE	(1024*1024)

#define WIN32_ASM_CALL		0xE8
#define ASCII_H_E			0x6568
#define ASCII_CR			10
#define ASCII_SP			32
#define TAG_CAPS			30

#define FEATURE_CPUID		0x00000001
#define FEATURE_TSC			0x00000010
#define FEATURE_MMX			0x00000020
#define FEATURE_CMOV		0x00000040

#define ROUND32(a)		(((uint32)(a+32))&(~31UL))

#define VERBOSE			if (verbose == true)

#define MEDIAN(a,b,c)	((a)<(b))?(((b)<(c))?(b):(((a)<(c))?(c):(a))):(((a)<(c))?(a):(((b)<(c))?(c):(b)))

#ifndef MAX
# define MAX(a,b)		(((a)>(b))?(a):(b))
#endif /* !MAX */

#ifndef MIN
# define MIN(a,b)		(((a)<(b))?(a):(b))
#endif /* !MIN */

#if defined(ZZIP) && defined(__GNUC__) && (defined(i386) || defined(_i386) || defined(_I386) || defined(__i386))
# define GET_STAT
# define STAT_ADD_TIME(a,b,c)	time_stat.a += (uint32)((b - c) >> 16)
# define STAT_ADD_SIZE(a,b)		time_stat.a += b;
# define GET_TSC(b) {uuint64 a;		\
   if (time_stat.t_stamp == true)	\
   { asm volatile ("rdtsc"			\
   :"=d"(a.d.h),"=a"(a.d.l):); }	\
   else a.u64 = 0;					\
   b=a.u64;}
#else  /* ZZIP && __GNUC__ && I386 */
# define GET_TSC(b)
# define STAT_ADD_TIME(a,b,c)
# define STAT_ADD_SIZE(a,b)
#endif /* ZZIP && __GNUC__ && I386 */

#ifndef bool
typedef enum
{
	false = 0,
	true  = 1
} bool;
#endif /* !bool */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef   signed char  sint8;
typedef   signed short sint16;
typedef   signed int   sint32;
typedef   signed int   sint;
typedef   signed long  slong;

#ifndef UNIX
typedef unsigned int   uint;
typedef unsigned long  ulong;
#endif /* !UNIX */

#ifdef _VISUALC
typedef unsigned __int64 uint64;
typedef   signed __int64 sint64;
#else  /* _VISUALC */
typedef unsigned long long uint64;
typedef   signed long long sint64;
#endif /* _VISUALC */

typedef union
{
	uint64 u64;
	struct
	{
		uint32 l;
		uint32 h;
	} d;
} uuint64;

typedef union
{
	sint64 s64;
	struct
	{
		uint32 l;
		uint32 h;
	} d;
} ssint64;

typedef union
{
	uint32 u32;
	struct
	{
		uint16 l;
		uint16 h;
	} w;
	struct
	{
		uint8 ll;
		uint8 lh;
		uint8 hl;
		uint8 hh;
	} b;
} uuint32;

typedef enum
{
	NO_TYPE,
	TEXT,
	MULTIMEDIA,
	BIN,
	WIN_EXE
} block_types;

typedef enum
{
	NONE,
	CREATE,
	EXTRACT,
	TEST,
	LIST,
	UPDATE,
	DELETE
} actions;

typedef enum
{
	UNCOMPRESS,
	TEST_CRC
} modes;

typedef enum
{
	OK                       =  0,
	NOT_ENOUGH_MEMORY        = -1,
    CRC_ERROR                = -2,
    TOO_MANY_SESSION         = -3,
	BAD_PARAMETER            = -4,
	UNEXPECTED_EOF           = -5,
	NOT_A_ZZIP_FILE          = -6,
	UNSUPPORTED_VERSION      = -7,
	CANNOT_CLOSE_INPUT_FILE  = -8,
	CANNOT_CLOSE_OUTPUT_FILE = -9,
	CANNOT_OPEN_INPUT_FILE   = -10,
	CANNOT_OPEN_OUTPUT_FILE  = -11,
	FSEEK_INPUT_FILE         = -12,
	FSEEK_OUTPUT_FILE        = -13,
	FILE_NOT_FOUND           = -14
} errors;

/*
 * Archive header
 * 2 bytes : "ZZ"
 * 1 byte  : Zzip version number used for compression
 * 4 bytes : number of files in the archive
 */

typedef struct
{
	char   magic[2];
	uint8  version_number;
	uint32 nb_of_file;
} header_arc_s;

/*
 * File header
 * 2 bytes : attributes
 * 2 bytes : number of blocks
 * 4 bytes : pointer to name (without trailing '\0')
 * 4 bytes : date/time of modification
 * 4 bytes : file size = size of all blocks
 * 4 bytes : original file size
 */

typedef struct
{
	uint16 attributes;
	uint16 nb_of_block;
	char   *name;
	time_t time;
	uint32 packed_size;
	uint32 original_size;
} header_file_s;

typedef struct
{
	uint8		mtf_max_char;
	bool		multimedia_test;
	bool		rle_encoding;
	bool		english_encoding;
	uint		compression_mode;
	uint		mm_type;
	sint		nblock;
	uint32		crc;
	block_types	type;
	uint8		*buffer;
	uint8		*buffer_length;
} block_param_s;

typedef struct
{
	bool			multimedia_test;
	bool			with_path;
    uint			compression_mode;
    uint32			block_size;
	uint32			input_total;
	uint32			output_total;

	char			*input_filename;
	FILE			*input_file;
	char			*output_filename;
	FILE			*output_file;

	block_types		type;
    actions			action;

	header_arc_s	head_arc;
	header_file_s	head_file;
} session_param_s;

typedef struct
{
	char         *filename;
	time_t       filetime;
	unsigned int input_size;
	unsigned int output_size;
	unsigned int nb_of_file;
} info_s;

typedef struct
{
	bool   t_stamp;
	uint32 cpuspeed;
	uint32 time_ana;
	uint32 time_rle;
	uint32 time_txt;
	uint32 time_mm;
	uint32 time_bwt1;
	uint32 time_bwt2;
	uint32 time_bwt3;
	uint32 time_bwt4;
	uint32 time_mtf;
	uint32 time_st0;
	uint32 time_st1;
	uint32 time_tot;
	uint32 kb_ana;
	uint32 kb_rle;
	uint32 kb_txt;
	uint32 kb_mm;
	uint32 kb_bwt;
	uint32 kb_mtf;
	uint32 kb_st0;
	uint32 kb_st1;
	uint32 kb_tot;
} time_stat_s;

#endif /* !GLOBAL_H */

/*---------------------------------------------*/
/* end                                global.h */
/*---------------------------------------------*/
