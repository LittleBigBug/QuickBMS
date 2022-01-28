/*---------------------------------------------*/
/* Zzip/Zzlib compressor               block.c */
/* (un)compress/archive managing functions     */
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

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
# include <io.h>
# include <sys/utime.h>
# include <direct.h>
#else  /* WIN32 */
# include <utime.h>
# include <unistd.h>
# include <dirent.h>
#endif /* WIN32 */

#include "zzip.h"

#ifndef S_IREAD
#define	S_IREAD		S_IRUSR
#endif

#ifdef SFX
# include "sfx_code.h"
#endif

/*---------------------------------------------*/

#ifdef WIN32
# define SEP_PATH '\\'
# define MKDIR_OPTIONS
#else  /* WIN32 */
# define SEP_PATH '/'
# define MKDIR_OPTIONS ,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif /* WIN32 */

#ifdef SFX
# define START_OFFSET SFX_CODE_SIZE
#else  /* SFX */
# define START_OFFSET 0L
#endif /* SFX */

#define WRITE_F(a,b)	fwrite((a),1,(b),session->output_file);
#define READ_F(a,b)		fread((a),1,(b),session->input_file);
#define WRITE_M(a,b)	{ memcpy(buffer_in, (a), (b)); buffer_in += (b); }
#define READ_M(a,b)		{ memcpy((a), buffer_in, (b)); buffer_in += (b); }
#define IO_ERROR()		{ last_error = errno; return; }
#define CHECK_IO_R()	{ if (ferror(session->input_file) != 0) last_error = errno; if (feof(session->input_file) != 0) last_error = UNEXPECTED_EOF; }
#define CHECK_IO_W()	{ if (ferror(session->output_file) != 0) last_error = errno; }
#define FTELL_I(a)		{ if ((a = ftell(session->input_file )) == -1) last_error = errno; }
#define FTELL_O(a)		{ if ((a = ftell(session->output_file)) == -1) last_error = errno; }
#define FSEEK_I(a,b)	{ if (fseek(session->input_file, (a), (b)) != 0)  last_error = FSEEK_INPUT_FILE; }
#define FSEEK_O(a,b)	{ if (fseek(session->output_file, (a), (b)) != 0) last_error = FSEEK_OUTPUT_FILE; }

int last_error = OK;

block_param_s block = { 0, false, false, false, 0, 0, 0, 0, NO_TYPE, NULL, NULL };

static session_param_s *session = NULL;
static union
{
	uint8  *buffer8;
	uint16 *buffer16;
	uint32 *buffer32;
} mem = { NULL };

#ifdef GET_STAT
 time_stat_s time_stat = { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif /* GET_STAT */

#ifdef ZZLIB
# define NB_MAX_SESSION 8
  static session_param_s *session_tab[NB_MAX_SESSION + 1] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
#endif /* ZZLIB */

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT int Get_last_error() { return last_error; }
#endif /* ZZLIB */

/*---------------------------------------------*/

INLINE static
void *MyFree(void *pp)
{
	free(pp);
	return NULL;
}

INLINE static
void *MyMalloc(size_t size)
{
	void *mm = malloc(size);
	if (mm == NULL) last_error = NOT_ENOUGH_MEMORY;
	return mm;
}

INLINE static
void *MyRealloc(void   *pp, 
				size_t size)
{
	void *mm = realloc(pp, size);
	if (mm == NULL) last_error = NOT_ENOUGH_MEMORY;
	return mm;
}

/*---------------------------------------------*/

#ifndef SFX

void CleanMemory()
{
	mem.buffer8 = MyFree(mem.buffer8);
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifndef SFX

/* Write header for an "archive" file */
static
void Write_Header_Arc()
{
	WRITE_F(session->head_arc.magic, sizeof(char) * 2);
	WRITE_F(&session->head_arc.version_number, sizeof(session->head_arc.version_number));
	WRITE_F(&session->head_arc.nb_of_file, sizeof(session->head_arc.nb_of_file));

	CHECK_IO_W();
}

/* Write header for a compressed file inside an "archive" file */
static
void Write_Header_File()
{
	uint lg = strlen(session->head_file.name);
	WRITE_F(&lg, sizeof(lg));
	WRITE_F(session->head_file.name, sizeof(char) * lg);
	WRITE_F(&session->head_file.time, sizeof(session->head_file.time));
	WRITE_F(&session->head_file.attributes, sizeof(session->head_file.attributes));
	WRITE_F(&session->head_file.packed_size, sizeof(session->head_file.packed_size));
	WRITE_F(&session->head_file.original_size, sizeof(session->head_file.original_size));
	WRITE_F(&session->head_file.nb_of_block, sizeof(session->head_file.nb_of_block));

	CHECK_IO_W();
}

#endif /* !SFX */

/*---------------------------------------------*/

/* Read header for an "archive" file */
static
void Read_Header_Arc()
{
	READ_F(session->head_arc.magic, sizeof(char)*2);
	READ_F(&session->head_arc.version_number, sizeof(session->head_arc.version_number));
	READ_F(&session->head_arc.nb_of_file, sizeof(session->head_arc.nb_of_file));

	CHECK_IO_R();

#ifndef SFX
	if (session->head_arc.magic[0] != 'Z' || session->head_arc.magic[1] != 'Z')
	{ 
		last_error = NOT_A_ZZIP_FILE; 
		return; 
	}
	if (session->head_arc.version_number != VERSION_NUMBER)
	{ 
		last_error = UNSUPPORTED_VERSION; 
		return; 
	}
#endif /* !SFX */
}

/* Read header for a compressed file inside an "archive" file */
static
void Read_Header_File()
{
	uint lg;
	READ_F(&lg, sizeof(lg));
	MyFree(session->head_file.name);
	session->head_file.name = (char*)MyMalloc(sizeof(char) * (lg + 1));
	READ_F(session->head_file.name, sizeof(char) * lg);
	session->head_file.name[lg] = '\0';
	READ_F(&session->head_file.time, sizeof(session->head_file.time));
	READ_F(&session->head_file.attributes, sizeof(session->head_file.attributes));
	READ_F(&session->head_file.packed_size, sizeof(session->head_file.packed_size));
	READ_F(&session->head_file.original_size, sizeof(session->head_file.original_size));
	READ_F(&session->head_file.nb_of_block, sizeof(session->head_file.nb_of_block));

	CHECK_IO_R();
}

/*---------------------------------------------*/

#ifdef SFX

#define BUFFER_SIZE (512*1024)

static
void Crc32_File(FILE *fin)
{
	uint8  *buffer;
	uint32 nb, crc = 0xFFFFFFFFUL, *crc_in_file;
	sint   end_of_file = 0;

	buffer = MyMalloc(BUFFER_SIZE * sizeof(uint8));
	if (last_error != OK) return;

	while (end_of_file == 0)
	{
		nb = fread(buffer, 1, BUFFER_SIZE, fin);
		if (ferror(fin) != 0)
		{
			last_error = errno;
			return;
		}
		end_of_file = feof(fin);
		if (end_of_file != 0) nb -= 4; /* 4 last bytes of file = CRC */
		crc = Crc32(buffer, buffer + nb, crc);
	} 

	crc_in_file = (uint32*)(buffer + nb);

	if (*crc_in_file != crc)
	{
		last_error = CRC_ERROR;
		return;
	}

	MyFree(buffer);
}

#endif /* SFX */

/*---------------------------------------------*/

#ifndef SFX

#define STAT_SIZE			(8*1024)
#define NB_S				64
#define BLOCKSTAT_THRESHOLD	5400

static sint32 bstat[NB_S] ALIGN;
static sint32 cstat[NB_S] ALIGN;

/* trick to compute an absolute value without any test/jump */
INLINE static
uint32 MyAbs(sint32 a)
{
	ssint64 s;
	s.s64 = a;
	return (s.d.l ^ s.d.h) - s.d.h;
}

static
uint32 BlockStat(uint8  *input_buffer, 
				 uint32 input_len)
{
	uint32 *buffer = (uint32*)input_buffer;
	uint32 len = input_len;
	sint32 *b1 = bstat, *b2 = cstat;

	b2[0] = -1;

	while (len > STAT_SIZE * 2)
	{
		uint32 i;

		len -= STAT_SIZE;

		for (i = 0; i < NB_S; ++i)
			b1[i] = 0;
		
		for (i = STAT_SIZE >> 2; i > 0; --i)
		{
			uuint32 u;
			u.u32 = *buffer++;
			b1[u.b.ll >> 2]++;
			b1[u.b.lh >> 2]++;
			b1[u.b.hl >> 2]++;
			b1[u.b.hh >> 2]++;
		}

		/* skip the first time */
		if (b2[0] != -1)
		{
			sint32 s = 0;

			for (i = 0; i < NB_S; ++i)
				s += MyAbs(b1[i] - b2[i]);
			
			if (s > BLOCKSTAT_THRESHOLD) 
				return ((uint8*)buffer - input_buffer) - STAT_SIZE;
		}

		/* swap buffers bstat and cstat */
		{
			sint32 *t = b1;
			b1 = b2;
			b2 = t;
		}
	}

	return input_len;
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifndef SFX

static
#ifdef ZZLIB
sint32 CompressBlock(bool   from_file, 
					 uint8  *buffer_in, 
					 uint32 len_in)
#else  /* ZZLIB */
sint32 CompressBlock()
#endif /* ZZLIB */
{
	bool   ff_bug = false;
	uint16 status;
	uint8  *buffer1 = NULL, *buffer2 = NULL, *buf_out1 = NULL, *buf_out2 = NULL;
	uint32 len, len2, len_max, first, tot1 = 0, tot2, block_len;
	sint   err = 0;
	slong  pos = 0, deb = 0, fin = 0;
#ifdef GET_STAT
	uint64 p1, p2;
#endif /* GET_STAT */

#ifdef ZZLIB
	uint8 *sav_buffer_in = buffer_in;

	if (from_file == false)
	{
		mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * (len_in + RUN_LENGTH_MAX + 32) * 6);
		if (last_error != OK) return -1;
		buffer1 = mem.buffer8;
		buffer2 = (uint8*)ROUND32(buffer1 + len_in * 2);

		memcpy(buffer1, buffer_in, sizeof(uint8) * len_in);
		len = len_in;
	}
	else
#endif /* ZZLIB */
	{
		uint32 taille_max = MIN(session->head_file.original_size + 1, session->block_size);

		mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * (taille_max + RUN_LENGTH_MAX + 32) * 6);
		if (last_error != OK) return -1;

		buffer1 = mem.buffer8;
		buffer2 = (uint8*)ROUND32(buffer1 + taille_max * 2);

		FTELL_I(pos);
		FTELL_O(deb);
		if (last_error != OK) return -1;
		
		len = READ_F(buffer1, sizeof(uint8) * taille_max);
		CHECK_IO_R();
		if (last_error == UNEXPECTED_EOF) last_error = OK;
		if (last_error != OK) return -1;

		if ((session->compression_mode & 2) == 2)
		{
			uint32 blockstat_len = BlockStat(buffer1, len);
			if (blockstat_len != len)
			{
				FSEEK_I((sint32)blockstat_len - (sint32)len, SEEK_CUR);
				if (last_error != OK) return -1;
				len = blockstat_len;
			}
		}
	}

	block_len = len;
	len_max = len;
	block.crc = Crc32_2(buffer1, buffer1 + len);
	block.mm_type = 0;

	STAT_ADD_SIZE(kb_tot, len);

	if (len > 64)
	{
		if (len < 3 * 1024) block.compression_mode = 0;
		
		/*- Beginning ---- Analyze ------------*/
		
		GET_TSC(p1);
		Analysis(buffer1, buffer1 + len);
		GET_TSC(p2);

		STAT_ADD_TIME(time_ana, p2, p1);
		STAT_ADD_SIZE(kb_ana, len);

		/*- End ---------- Analyze ------------*/
		
		/* trick for 'Canterbury Corpus: kennedy.xls' !, delta-encoding with record size of 13 */
		if (((uint32*)buffer1)[10] == 161480704 && ((uint32*)buffer1)[20] == 60818693)
		{
			uint  i;
			uint8 *b8_out = buffer2, *b8;

			for (i = 0; i < 13; ++i)
				for (b8 = buffer1 + 2320 + i; b8 < buffer1 + len; b8 += 13)
					*b8_out++ = *b8 - *(b8-13);
			memcpy(buffer1 + 2320, buffer2, len - 2320);
			block.rle_encoding = true;
			block.type = NO_TYPE;
			block.mm_type = 6;
		}

		/*- Beginning ---- RLE Coding ---------*/

		if (block.rle_encoding == true)
		{
			uint32 rle_len;

			GET_TSC(p1);
			rle_len = RLE_Coding(buffer1, buffer2, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_rle, p2, p1);
			STAT_ADD_SIZE(kb_rle, len);

			len = rle_len;
			memcpy(buffer1, buffer2, len);
		}

		/*- End ---------- RLE Coding ---------*/

		/*- Beginning ---- MM Coding ----------*/

		if ((len > 128 * 1024) 
			& ((block.type == BIN) | (block.type == NO_TYPE)) 
			& (block.multimedia_test == true))
		{
			uint res;

			GET_TSC(p1);
			res = MM_Test(buffer1 + 512, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_ana, p2, p1);

			if (res != 0)
			{
				block.mm_type = res;
				block.type = MULTIMEDIA;
			}
		}

		if (block.mm_type != 0)
		{
			GET_TSC(p1);
			MM_Coding(buffer1, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_mm, p2, p1);
			STAT_ADD_SIZE(kb_mm, len);
		}

		/*- End ---------- MM Coding ----------*/

		/*- Beginning ---- W32/BIN Coding -----*/

		if (block.type == WIN_EXE)	
		{
			GET_TSC(p1)
			Win32_Coding(buffer1, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
		}

		if (block.type == BIN || block.type == WIN_EXE)
		{
			GET_TSC(p1);
			Reverse_Block(buffer1, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
			STAT_ADD_SIZE(kb_txt, len);
		}

		/*- End ---------- W32/BIN Coding -----*/
		
		/*- Beginning ---- alpha/txt Cod. -----*/

		if (block.type == TEXT)
		{
			GET_TSC(p1);

			len = Filter1(buffer1, buffer2, len);

			memcpy(buffer1, buffer2, len);

			mem.buffer8 = (uint8*)MyRealloc(mem.buffer8, sizeof(uint8) * (len + RUN_LENGTH_MAX + 32) * 6);
			if (last_error != OK) return -1;

			buffer1 = mem.buffer8;

			len_max = MAX(len, len_max);

			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
			STAT_ADD_SIZE(kb_txt, len);
		}

		/*- End ---------- alpha/txt Cod. -----*/

		/*- Beginning ---- Phr. replacement ---*/

		if (block.english_encoding == true)
		{
			GET_TSC(p1);
			len = Filter2(buffer1, buffer1 + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
		}

		/*- End ---------- Phr. replacement ---*/
	
		/*- Beginning ---- BWT ----------------*/

		/* to avoid a bug if the block ends with a run of 0xFF */
		if (buffer1[len - 1] == 0xFF)
		{
			buffer1[len - 1] -= buffer1[len - 2];
			ff_bug = true;
		}

		BWT_Coding(len, &first, buffer1);

		if (last_error != OK) return -1;

		/*- End ---------- BWT ----------------*/

		/*- Beginning ---- MTF Coding ---------*/
		
		GET_TSC(p1);

		M1FF2_Coding(buffer1, buffer1 + len);
		
		/*- End ---------- MTF Coding ---------*/

		/*- Beginning ---- Split --------------*/

		buf_out1 = (uint8*)ROUND32(buffer1 + len);
		buffer2  = (uint8*)ROUND32(buf_out1 + len);
		buf_out2 = (uint8*)ROUND32(buffer2 + len);

		len2 = Split(buffer1, buffer1 + len, buffer2);

		GET_TSC(p2);

		STAT_ADD_TIME(time_mtf, p2, p1);
		STAT_ADD_SIZE(kb_mtf, len);

		/*- End ---------- Split --------------*/
		
		/*- Beginning ---- Arith Compression --*/

		block.buffer = buf_out1;

		GET_TSC(p1);
		tot1 = Zip_SM0(len, buffer1);
		GET_TSC(p2);

		STAT_ADD_TIME(time_st0, p2, p1);
		STAT_ADD_SIZE(kb_st0, len);

		if (last_error != OK) return -1;
		
		block.buffer = buf_out2;

		GET_TSC(p1);
		tot2 = Zip_SM1(len2, buffer2);
		GET_TSC(p2);

		STAT_ADD_TIME(time_st1, p2, p1);
		STAT_ADD_SIZE(kb_st1, len2);

		if (last_error != OK) return -1;
		
		/*- End ---------- Arith Compression --*/
		
		/* if we can't compress the block, we get the original block */
		if ((tot1+tot2) > len_max)
		{
			err = -1;
			
#ifdef ZZLIB
			if (from_file == false)
			{
				mem.buffer8 = MyFree(mem.buffer8);
				mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * len_in);
				if (last_error != OK) return -1;
				buf_out1 = mem.buffer8;
				
				memcpy(buf_out1, buffer_in, sizeof(uint8) * len_in);
				tot1 = len_in;
			}
			else
#endif /* ZZLIB */
			{
				mem.buffer8 = MyFree(mem.buffer8);
				mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * block_len);
				if (last_error != OK) return -1;
				buf_out1 = mem.buffer8;
				
				FSEEK_I(pos, SEEK_SET);
				if (last_error != OK) return -1;
				
				tot1 = READ_F(buf_out1, block_len);
				if (last_error == UNEXPECTED_EOF) last_error = OK;
				if (last_error != OK) return -1;
			}
			
			len_max = tot1;
		}
	} /* (len > 50) */
	else 
	{
		buf_out1 = buffer1;
		err = -1;
		tot1 = len;
	}

	/*- Beginning ---- Writings -----------*/

	/*  status
     *   1   1   1   1   1   1   1
	 *   6   5   4   3   2   1   0   9   8   7   6   5   4   3   2   1
	 * |err|                       |ffb|eng|   mm_type |      type |rle|
	 */
	status = 0;
	status |= ff_bug;
	status <<= 1;
	status |= block.english_encoding;
	status <<= 3;
	status |= block.mm_type;
	status <<= 3;
	status |= block.type;
	status <<= 1;
	status |= block.rle_encoding;

	if (err == -1) status |= 32768;

	/* we add a small margin, in case... */
	len_max += 32;

#ifdef ZZLIB
	if (from_file == false)
	{
		WRITE_M(&len_max, sizeof(len_max));
		WRITE_M(&block.crc, sizeof(block.crc));
		if ((status & 32768) == 0)
		{
			WRITE_M(&status, sizeof(status));
			WRITE_M(&len, sizeof(len));
			WRITE_M(&len2, sizeof(len2));
			WRITE_M(&first, sizeof(first));
			WRITE_M(&block.mtf_max_char, sizeof(block.mtf_max_char));
			WRITE_M(&tot2, sizeof(tot2));
			WRITE_M(buf_out2, sizeof(uint8) * tot2);
		}
		else 
		{ 
			WRITE_M(&status, sizeof(status)); 
		}
		WRITE_M(&tot1, sizeof(tot1));
		WRITE_M(buf_out1, sizeof(uint8) * tot1);
	}
	else
#endif /* ZZLIB */
	{
		WRITE_F(&len_max, sizeof(len_max));
		WRITE_F(&block.crc, sizeof(block.crc));
		if ((status & 32768) == 0)
		{
			WRITE_F(&status, sizeof(status));
			WRITE_F(&len, sizeof(len));
			WRITE_F(&len2, sizeof(len2));
			WRITE_F(&first, sizeof(first));
			WRITE_F(&block.mtf_max_char, sizeof(block.mtf_max_char));
			WRITE_F(&tot2, sizeof(tot2));
			WRITE_F(buf_out2, sizeof(uint8) * tot2);
		}
		else 
		{ 
			WRITE_F(&status, sizeof(status)); 
		}
		WRITE_F(&tot1, sizeof(tot1));
		WRITE_F(buf_out1, sizeof(uint8) * tot1);
		
		CHECK_IO_W();
		if (last_error != OK) return -1;

		FTELL_O(fin);
		if (last_error != OK) return -1;
	}
	
#ifdef _DEBUG
	printf("1 (compression) : status=%u len=%u len2=%u first=%u tot2=%u tot1=%u\n", status, len, len2, first, tot2, tot1);
	printf("2 (compression) : mtf_max_char=%u english=%u mm_type=%u type=%u rle=%u \n", block.mtf_max_char, block.english_encoding, block.mm_type, block.type, block.rle_encoding);
#endif /* _DEBUG */
	
	/*- End ---------- Writings -----------*/

	mem.buffer8 = MyFree(mem.buffer8);

#ifdef ZZLIB
	if (from_file == false) return (uint32)(buffer_in - sav_buffer_in);
	else
#endif /* ZZLIB */
	return (uint32)(fin - deb);
}

#endif /* !SFX */

/*---------------------------------------------*/

static
#ifdef ZZLIB
sint32 UncompressBlock(modes mode, 
					   bool  to_file, 
					   uint8 *buffer_in)
#else  /* ZZLIB */
sint32 UncompressBlock(modes mode)
#endif /* ZZLIB */
{
	bool   ff_bug = false;
	uint16 status;
	uint32 len = 0, len2 = 0, len_max = 0, first = 0, tot1 = 0, tot2 = 0;
	uint8  *bufferin1 = NULL, *bufferin2 = NULL, *bufferout = NULL;
#ifdef GET_STAT
	uint64 p1, p2;
#endif /* GET_STAT */

#ifdef ZZLIB
	uint8 *sav_buffer_in = buffer_in;

	if (to_file == false)
	{
		READ_M(&len_max, sizeof(len_max));
		READ_M(&block.crc, sizeof(block.crc));
		READ_M(&status, sizeof(status));

		if ((status & 32768) == 0)
		{
			ff_bug					= (status >> 8) & (1);
			block.english_encoding	= (status >> 7) & (1);
			block.mm_type			= (status >> 4) & (1 + 2 + 4);
			block.type				= (status >> 1) & (1 + 2 + 4);
			block.rle_encoding		= (status >> 0) & (1);
			READ_M(&len, sizeof(len));
			READ_M(&len2, sizeof(len2));
			READ_M(&first, sizeof(first));
			READ_M(&block.mtf_max_char, sizeof(block.mtf_max_char));
			
			mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * len_max * 5 + 32 * 2);
			if (last_error != OK) return -1;
			bufferout = mem.buffer8;
			bufferin1 = (uint8*)ROUND32(bufferout + len_max);
			bufferin2 = (uint8*)ROUND32(bufferin1 + len_max);
			
			READ_M(&tot2, sizeof(tot2));
			READ_M(bufferin2, sizeof(uint8) * tot2);
		}		
		else 
		{ 
			mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * len_max * 1);
			if (last_error != OK) return -1;
			bufferin1 = mem.buffer8;
		}
		READ_M(&tot1, sizeof(tot1));	
		READ_M(bufferin1, sizeof(uint8) * tot1);
	
		buffer_in = sav_buffer_in;
	}
	else
#endif /* ZZLIB */
	{
		READ_F(&len_max, sizeof(len_max));
		READ_F(&block.crc, sizeof(block.crc));
		READ_F(&status, sizeof(status));
		
		CHECK_IO_R();
		if (last_error != OK) return -1;

		if ((status & 32768) == 0)
		{
			ff_bug					= (status >> 8) & (1);
			block.english_encoding	= (status >> 7) & (1);
			block.mm_type			= (status >> 4) & (1 + 2 + 4);
			block.type				= (status >> 1) & (1 + 2 + 4);
			block.rle_encoding		= (status >> 0) & (1);
			READ_F(&len, sizeof(len));
			READ_F(&len2, sizeof(len2));
			READ_F(&first, sizeof(first));
			READ_F(&block.mtf_max_char, sizeof(block.mtf_max_char));
			
			mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * len_max * 5 + 32 * 2);
			if (last_error != OK) return -1;
			bufferout = mem.buffer8;
			bufferin1 = (uint8*)ROUND32(bufferout + len_max);
			bufferin2 = (uint8*)ROUND32(bufferin1 + len_max);
			
			READ_F(&tot2, sizeof(tot2));
			READ_F(bufferin2, sizeof(uint8) * tot2);
		}		
		else 
		{ 
			mem.buffer8 = (uint8*)MyMalloc(sizeof(uint8) * len_max * 1);
			if (last_error != OK) return -1;
			bufferin1 = mem.buffer8;
		}
		READ_F(&tot1, sizeof(tot1));	
		READ_F(bufferin1, sizeof(uint8) * tot1);
		
		CHECK_IO_R();
		if (last_error != OK) return -1;
	}
	
	
#ifdef _DEBUG
	printf("1 (decompression) : status=%u len=%u len2=%u first=%u tot2=%u tot1=%u\n", status, len, len2, first, tot2, tot1);
	printf("2 (decompression) : mtf_max_char=%u english=%u mm_type=%u type=%u rle=%u \n", block.mtf_max_char, block.english_encoding, block.mm_type, block.type, block.rle_encoding);
#endif /* _DEBUG */

	if ((status & 32768) == 0)
	{
		block.buffer        = bufferin1;
		block.buffer_length = bufferin1 + tot1;

		GET_TSC(p1);
		Unzip_SM0(len, bufferout);
		GET_TSC(p2);

		STAT_ADD_TIME(time_st0, p2, p1);
		STAT_ADD_SIZE(kb_st0, len);

		if (last_error != OK) return -1;

		block.buffer        = bufferin2;
		block.buffer_length = bufferin2 + tot2;
		
		GET_TSC(p1);
		Unzip_SM1(len2, bufferin1);
		GET_TSC(p2);

		STAT_ADD_TIME(time_st1, p2, p1);
		STAT_ADD_SIZE(kb_st1, len2);

		if (last_error != OK) return -1;
		
		GET_TSC(p1);

		UnSplit(bufferout, bufferout + len, bufferin1);

		M1FF2_Decoding(bufferout, bufferout + len);

		GET_TSC(p2);

		STAT_ADD_TIME(time_mtf, p2, p1);
		STAT_ADD_SIZE(kb_mtf, len);

		GET_TSC(p1);
		BWT_Decoding(len, first, bufferout, (uint32*)bufferin1);
		GET_TSC(p2);

		STAT_ADD_TIME(time_bwt1, p2, p1);
		STAT_ADD_SIZE(kb_bwt, len);

		if (last_error != OK) return -1;

		if (ff_bug == true)
			bufferout[len - 1] += bufferout[len - 2];

		if (block.english_encoding == true)
		{
			GET_TSC(p1);
			len = UnFilter2(bufferout, bufferin1, bufferout + len);
			memcpy(bufferout, bufferin1, len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
		}

		if (block.type == TEXT)
		{
			GET_TSC(p1);
			len = UnFilter1(bufferout, len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
			STAT_ADD_SIZE(kb_txt, len);
		}

		if (block.type == BIN || block.type == WIN_EXE)
		{
			GET_TSC(p1);
			Reverse_Block(bufferout, bufferout + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
			STAT_ADD_SIZE(kb_txt, len);
		}
		
		if (block.type == WIN_EXE)	
		{
			GET_TSC(p1);
			Win32_Decoding(bufferout, bufferout + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_txt, p2, p1);
		}

		if (block.mm_type != 0)
		{
			GET_TSC(p1);
			MM_Decoding(bufferout, bufferout + len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_mm, p2, p1);
			STAT_ADD_SIZE(kb_mm, len);
		}

		if (block.rle_encoding == true) 
		{
			GET_TSC(p1);
			len = RLE_Decoding(bufferout, bufferin1, bufferout + len);
			memcpy(bufferout, bufferin1, len);
			GET_TSC(p2);

			STAT_ADD_TIME(time_rle, p2, p1);
			STAT_ADD_SIZE(kb_rle, len);
		}

		/* trick for 'Canterbury Corpus: kennedy.xls' !*/
		if (block.mm_type == 6)
		{
			uint  i;
			uint8 *b8_out, *b8, *bufferout2 = bufferin1;

			memcpy(bufferout2, bufferout, 2320);

			b8 = bufferout + 2320;
			for (i = 0; i < 13; ++i)
				for (b8_out = bufferout2 + 2320 + i; b8_out < bufferout2 + len; b8_out += 13)
					*b8_out = *b8++ + *(b8_out-13);

			memcpy(bufferout + 2320, bufferout2 + 2320, len - 2320);
		}

		STAT_ADD_SIZE(kb_tot, len);

		if (block.crc != Crc32_2(bufferout, bufferout + len))
		{
			last_error = CRC_ERROR;
			return -1;
		}

		if (mode == UNCOMPRESS)
		{
#ifdef ZZLIB
			if (to_file == false) { WRITE_M(bufferout, sizeof(uint8) * len); }
			else
#endif /* ZZLIB */
			WRITE_F(bufferout, sizeof(uint8) * len);
		}
	}
	else 
	{
		STAT_ADD_SIZE(kb_tot, tot1);

		if (block.crc != Crc32_2(bufferin1, bufferin1 + tot1))
		{
			last_error = CRC_ERROR;
			return -1;
		}

		len = tot1;
		if (mode == UNCOMPRESS)
		{
#ifdef ZZLIB
			if (to_file == false) { WRITE_M(bufferin1, sizeof(uint8) * len); }
			else
#endif /* ZZLIB */
			WRITE_F(bufferin1, sizeof(uint8) * len);
		}
	}

	mem.buffer8 = MyFree(mem.buffer8);

	if (mode == UNCOMPRESS)
	{
#ifdef ZZLIB
		if (to_file == true)
#endif /* ZZLIB */
		{
			CHECK_IO_W();
			if (last_error != OK) return -1;
		}
	}

	return len;
}

/*---------------------------------------------*/

#ifndef SFX

static
void Compress()
{
	uint		i;
	block_types	block_type = NO_TYPE;
    struct stat	buf_stat;
	slong		p1, p3;
	uint32		out_size;
#ifndef ZZLIB
	sint32		l = 0;
#endif /* ZZLIB */

	if (stat(session->input_filename, &buf_stat) == -1) IO_ERROR();
	session->head_file.name          = session->input_filename;
	session->head_file.time          = buf_stat.st_mtime;
	session->head_file.attributes    = buf_stat.st_mode;
	session->head_file.original_size = buf_stat.st_size;

	if ((session->input_file = fopen(session->input_filename, "rb")) == NULL) 
	{ 
		last_error = CANNOT_OPEN_INPUT_FILE;
		return;
	}
				
	FTELL_O(p1);
	Write_Header_File();

	if (last_error != OK) return;

	block.type  = session->type;
	block.multimedia_test = session->multimedia_test;

	out_size = 0;
	for (i = 0; feof(session->input_file) == 0; ++i)
	{
		if (block.type != WIN_EXE) block.type = session->type;
		block.compression_mode = session->compression_mode;

#ifdef ZZLIB
		out_size += CompressBlock(1, NULL, 0);
		if (last_error != OK) return;
		if (i == 0) block_type = block.type;
	}
#else  /* ZZLIB*/
		out_size += CompressBlock();
		if (last_error != OK) return;
		if (i == 0) block_type = block.type;
		FTELL_I(l);
		VERBOSE printf("%#04.1f%%\b\b\b\b\b", (((float)l) * 100) / session->head_file.original_size);
	}
	VERBOSE printf("      \b\b\b\b\b\b\b");
#endif /* ZZLIB */
	block.type = block_type;
	session->head_file.nb_of_block = i;

	FTELL_O(p3);
	if (last_error != OK) return;

	if (fclose(session->input_file)) { last_error = CANNOT_CLOSE_INPUT_FILE; return; }
	session->input_file = NULL;

	FSEEK_O(p1, SEEK_SET);
	if (last_error != OK) return;

	session->head_file.packed_size = out_size;
	Write_Header_File();
	if (last_error != OK) return;

	FSEEK_O(p3, SEEK_SET);
	if (last_error != OK) return;

	session->input_total += session->head_file.original_size;
	session->output_total += out_size;
}

#endif /* !SFX */

/*---------------------------------------------*/

static
void Uncompress(modes mode)
{
	struct utimbuf filetime;
	slong  t1 = 0, t2 = 0;
	uint   i = 0;
	uint32 l = 0;

	FTELL_I(t1);
	if (last_error != OK) return;

	Read_Header_File();
	if (last_error != OK) return;
	
	if (mode == UNCOMPRESS)
	{
		/* we strip the path before the file name */
		session->output_filename = strrchr(session->head_file.name, SEP_PATH);
		if (session->output_filename != NULL)
		{
			if (session->with_path == 0) session->output_filename++;
			else
			{
				struct stat st_buf;
				char *p, *path = session->head_file.name;
				
				for (p = path; *p; ++p)
				{
					if (*p == SEP_PATH)
					{
						*p = 0;
						if (stat(path, &st_buf) != 0 
							|| (st_buf.st_mode & (S_IFDIR|S_IREAD)) != (S_IFDIR|S_IREAD))
						{
							if (mkdir(path MKDIR_OPTIONS) == -1) IO_ERROR();
						}
						*p = SEP_PATH;
					}
				}
				
				session->output_filename = session->head_file.name;
			}
		}
		else session->output_filename = session->head_file.name;
		
		if ((session->output_file = fopen(session->output_filename, "wb")) == NULL)
		{ 
			last_error = CANNOT_OPEN_OUTPUT_FILE; 
			return;
		}
	}
	else session->output_filename = session->head_file.name;

	for (i = 0; i < session->head_file.nb_of_block; ++i)
	{
#ifdef ZZLIB
		l += UncompressBlock(mode, 1, NULL);
		if (last_error != OK) return;
	}
#else  /* ZZLIB */
		l += UncompressBlock(mode);
		if (last_error != OK) return;
		VERBOSE printf(" %#04.1f%%\b\b\b\b\b\b", (((float)l) * 100) / session->head_file.original_size);
	}
	VERBOSE printf("      \b\b\b\b\b\b\b");
#endif /* ZZLIB */

	session->output_total += l;
	
	FTELL_I(t2);
	session->input_total += t2 - t1;

	if (mode == UNCOMPRESS)
	{
		if (fclose(session->output_file)) 
		{ 
			last_error = CANNOT_CLOSE_OUTPUT_FILE; 
			return; 
		}
		session->output_file = NULL;
		
		filetime.actime = session->head_file.time;
		filetime.modtime = session->head_file.time;
		if (utime(session->output_filename, &filetime) == -1) IO_ERROR();
		if (chmod(session->output_filename, session->head_file.attributes) == -1) IO_ERROR();
	}
}

/*---------------------------------------------*/

#ifdef ZZLIB

DLL_EXPORT int ZzCompressBlock(unsigned char *buffer, 
							   unsigned int  size, 
							   unsigned int  compression_mode, 
							   unsigned int  multimedia_test)
{
	int len;

	last_error = OK;

	if (buffer == NULL
		|| size > 20*1024*1024)
	{
		last_error = BAD_PARAMETER;
		return -1;
	}

	session = (session_param_s*)MyMalloc(sizeof(session_param_s));
	if (last_error != OK) return -1;

	block.type = NO_TYPE;
	session->multimedia_test = multimedia_test;
	session->compression_mode = compression_mode;

	len = CompressBlock(0, buffer, size);

	session = MyFree(session);

	return len;
}

DLL_EXPORT int ZzUncompressBlock(unsigned char *buffer)
{
	int len;

	last_error = OK;

	if (buffer == NULL)
	{
		last_error = BAD_PARAMETER;
		return -1;
	}

	session = (session_param_s*)MyMalloc(sizeof(session_param_s));
	if (last_error != OK) return -1;

	len = UncompressBlock(UNCOMPRESS, 0, buffer);

	session = MyFree(session);

	return len;
}

#endif /* ZZLIB */

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT
#endif /* ZZLIB */
int OpenArchive(actions action, 
				 char    *filename, 
				 info_s  *info)
{
	sint r = 0;

	last_error = OK;

	session = (session_param_s*)MyMalloc(sizeof(session_param_s));
	if (last_error != OK) return -1;

#ifdef ZZLIB
	if (filename == NULL)
	{
		last_error = BAD_PARAMETER;
		return -1;
	}

	while (session_tab[r] != NULL) r++;
	if (r == NB_MAX_SESSION)
	{ 
		last_error = TOO_MANY_SESSION; 
		return -1; 
	}
	session_tab[r] = session;
#endif /* ZZLIB */

    session->action = action;
	session->input_total = 0;
	session->output_total = 0;
	session->output_filename = NULL;
	session->output_file = NULL;
	session->input_filename = NULL;
	session->input_file = NULL;
	session->head_file.name = NULL;

	switch (action)
	{

#ifndef SFX
	case DELETE:
	case UPDATE:
		session->output_filename = strdup(filename);
		if ((session->output_file = fopen(session->output_filename, "r+b")) == NULL) 
		{ 
			last_error = CANNOT_OPEN_OUTPUT_FILE; 
			return -1;
		}
		session->input_file = session->output_file;
		
		FSEEK_O(0, SEEK_SET);

		Read_Header_Arc();

		FSEEK_I(0, SEEK_END);

		if (last_error != OK) return -1;

		break;
	case CREATE:
		session->head_arc.nb_of_file = 0;
		session->head_arc.magic[0] = 'Z';
		session->head_arc.magic[1] = 'Z';
		session->head_arc.version_number = VERSION_NUMBER;

		session->output_filename = strdup(filename);
		if ((session->output_file = fopen(session->output_filename, "wb")) == NULL) 
		{ 
			last_error = CANNOT_OPEN_OUTPUT_FILE; 
			return -1;
		}

		Write_Header_Arc();
		if (last_error != OK) return -1;

		break;
#endif /* !SFX */
	case TEST:
	case LIST:
	case EXTRACT:
		session->input_filename = strdup(filename);

		if ((session->input_file = fopen(session->input_filename, "rb")) == NULL)
		{ 
			last_error = CANNOT_OPEN_INPUT_FILE; 
			return -1;
		}

#ifdef SFX
		VERBOSE printf("\n  Checking archive CRC...");

		Crc32_File(session->input_file);
		if (last_error != OK) return -1;

		FSEEK_I(START_OFFSET, SEEK_SET);
		if (last_error != OK) return -1;

		VERBOSE printf(" OK\n");
#endif /* SFX */

		Read_Header_Arc();
		if (last_error != OK) return -1;

		break;
	default:
		last_error = BAD_PARAMETER;
		return -1;
	}

	if (info != NULL)
	{
		info->nb_of_file = session->head_arc.nb_of_file;
	}

	return r;
}

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void CloseArchive(sint   handle, 
							 info_s *info)
#else  /* ZZLIB */
void CloseArchive(info_s *info)
#endif /* ZZLIB */
{
#ifdef ZZLIB
	if (session_tab[handle] == NULL)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#else  /* ZZLIB */
	if (session == NULL) return;
#endif /* ZZLIB */


#ifndef SFX
	if (session->action == CREATE || session->action == UPDATE || session->action == DELETE)
	{
		FSEEK_O(0, SEEK_SET);
		Write_Header_Arc();
	}

	if (last_error != OK) return;

	if (session->action == DELETE)
	{
		slong filesize;
		ListAllFile(
#ifdef ZZLIB
			handle,
#endif /* ZZLIB */
			NULL);
		FTELL_I(filesize);

#ifdef WIN32
		if (chsize(fileno(session->input_file), filesize) != 0) IO_ERROR();
#else
		if (ftruncate(fileno(session->input_file), filesize) != 0) IO_ERROR();
#endif /* WIN32 */
	}
#endif /* !SFX */

	if (session->input_file != NULL) 
		if (fclose(session->input_file) != 0) last_error = CANNOT_CLOSE_INPUT_FILE;

	if (session->output_file != NULL && session->output_file != session->input_file) 
		if (fclose(session->output_file) != 0) last_error = CANNOT_CLOSE_OUTPUT_FILE;

	session->input_file = NULL;
	session->output_file = NULL;

	if (last_error != OK && session->output_filename != NULL) remove(session->output_filename); 

	free(session->input_filename);
	free(session->output_filename);

	if (info != NULL)
	{
		info->input_size = session->input_total;
		info->output_size = session->output_total;
		info->nb_of_file = session->head_arc.nb_of_file;
	}

	session = MyFree(session);
#ifdef ZZLIB
	session_tab[handle] = NULL;
#endif /* ZZLIB */
}

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void ListNextFile(sint   handle, 
							 info_s *info)
#else  /* ZZLIB */
void ListNextFile(info_s *info)
#endif /* ZZLIB */
{
	slong pos1, pos2;

	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	FTELL_I(pos1);

	Read_Header_File();

	FSEEK_I((sint32)session->head_file.packed_size, SEEK_CUR);

	FTELL_I(pos2);

	if (last_error != OK) return;

	if (info != NULL)
	{
		info->filename = session->head_file.name;
		info->input_size = session->head_file.original_size;
		info->output_size = (uint32)(pos2 - pos1);
		info->filetime = session->head_file.time;
	}
}

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void ListAllFile(sint   handle, 
							info_s **info)
#else  /* ZZLIB */
void ListAllFile(info_s **info)
#endif /* ZZLIB */
{
	uint  i, nb;

	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	FSEEK_I(START_OFFSET, SEEK_SET);
	if (last_error != OK) return;

	nb = session->head_arc.nb_of_file;
	Read_Header_Arc();
	if (last_error != OK) return;
	session->head_arc.nb_of_file = nb;

	for (i = 0; i < session->head_arc.nb_of_file; ++i)
	{
		if (info != NULL && info[i] != NULL) 
		{ 
			ListNextFile(
#ifdef ZZLIB
				handle,
#endif /* ZZLIB */
				info[i]);
			if (last_error != OK) return;
			info[i]->filename = strdup(info[i]->filename);
			session->input_total += info[i]->input_size;
			session->output_total += info[i]->output_size;
		}
		else
		{
			ListNextFile(
#ifdef ZZLIB
				handle,
#endif /* ZZLIB */
				NULL);
			if (last_error != OK) return;
		}
	}
}

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void SetArchivePointer(sint   handle, 
								  char   *filename, 
								  info_s *info)
#else  /* ZZLIB */
void SetArchivePointer(char   *filename, 
					   info_s *info)
#endif /* ZZLIB */
{
	uint  i, nb;
	bool  ok = false;

	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	FSEEK_I(START_OFFSET, SEEK_SET);
	if (last_error != OK) return;

	nb = session->head_arc.nb_of_file;
	Read_Header_Arc();
	if (last_error != OK) return;
	session->head_arc.nb_of_file = nb;

	if (filename == NULL) 
	{
		session->input_total = 0;
		session->output_total = 0;
		return;
	}

	if (info == NULL)
	{
		info = (info_s*)MyMalloc(sizeof(info_s));
		if (last_error != OK) return;
	}

	for (i = 0; i < session->head_arc.nb_of_file; ++i)
	{
		ListNextFile(
#ifdef ZZLIB
			handle,
#endif
			info);
		if (last_error != OK) return;
		if (strcmp(info->filename, filename) == 0)
		{
			ok = true;
			break;
		}
	}

	if (ok == false)
	{
		last_error = FILE_NOT_FOUND;
		return;
	}

	FSEEK_I(-((slong)(info->output_size)), SEEK_CUR);
	if (last_error != OK) return;
}

/*---------------------------------------------*/

#ifndef SFX

#ifdef ZZLIB
DLL_EXPORT void AddFile(sint   handle, 
						char   *filename, 
						uint   compression_mode, 
						uint   multimedia_test, 
						uint   block_size, 
						info_s *info)
#else  /* ZZLIB */
void AddFile(char   *filename, 
			 uint   compression_mode, 
			 uint   multimedia_test, 
			 uint   block_size, 
			 info_s *info)
#endif /* ZZLIB */
{
	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL
		|| filename == NULL
		|| (session->action != CREATE && session->action != UPDATE))
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	block_size = MIN(block_size, 15*1024*1024);	/* max block size of 15Mb (1<<24 plus margin, cf. bwt.c) */
	block_size = MAX(block_size, 64*1024);		/* min block size of 64kb */

	session->compression_mode = compression_mode;
	session->type = NO_TYPE;
	session->block_size = block_size;
	session->head_arc.nb_of_file++;
	session->multimedia_test = multimedia_test;
	session->input_filename = filename;

	Compress();
	if (last_error != OK) return;

	if (info != NULL)
	{
		info->filename = session->output_filename;
		info->filetime = session->head_file.time;
		info->input_size = session->head_file.original_size;
		info->output_size = session->head_file.packed_size;
	}

	session->input_filename = NULL;
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void TestNextFile(sint   handle, 
							 info_s *info)
#else  /* ZZLIB */
void TestNextFile(info_s *info)
#endif /* ZZLIB*/
{
	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL	|| session->action != TEST)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	session->with_path = false;
	session->output_filename = NULL;

	Uncompress(TEST_CRC);
	if (last_error != OK) return;

	if (info != NULL)
	{
		info->filename = session->output_filename;
		info->filetime = session->head_file.time;
		info->input_size = session->head_file.packed_size;
		info->output_size = session->head_file.original_size;
	}

	session->output_filename = NULL;
}

/*---------------------------------------------*/

#ifndef SFX

#define BSIZE (2 * 1024 * 1024)

#ifdef ZZLIB
DLL_EXPORT void DeleteNextFile(sint handle)
#else  /* ZZLIB */
void DeleteNextFile()
#endif /* ZZLIB */
{
	uint32 len;
	slong  rpointer, wpointer;
	uint8  *fileblock = NULL;

	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL || session->action != DELETE)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	fileblock = (uint8*)MyMalloc(sizeof(uint8) * BSIZE);
	if (last_error != OK) return;
				
	FTELL_I(wpointer);
	if (last_error != OK) return;

	ListNextFile(
#ifdef ZZLIB
		handle,
#endif /* ZZLIB */
		NULL);
	if (last_error != OK) return;

	FTELL_I(rpointer);
	if (last_error != OK) return;
				
	do
	{
		FSEEK_I(rpointer, SEEK_SET);
		if (last_error != OK) return;
		len = READ_F(fileblock, sizeof(uint8) * BSIZE);
		if (last_error == UNEXPECTED_EOF) last_error = OK;
		if (last_error != OK) return;
		rpointer += len;
		
		FSEEK_I(wpointer, SEEK_SET);
		if (last_error != OK) return;
		WRITE_F(fileblock, sizeof(uint8) * len);
		CHECK_IO_W();
		if (last_error != OK) return;
		wpointer += len;
	}
	while (len == BSIZE);

	session->head_arc.nb_of_file--;
				
	fileblock = MyFree(fileblock);
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifdef ZZLIB
DLL_EXPORT void ExtractNextFile(sint   handle, 
								char   *filename, 
								uint   with_path, 
								info_s *info)
#else  /* ZZLIB */
void ExtractNextFile(char   *filename, 
					 uint   with_path, 
					 info_s *info)
#endif /* ZZLIB */
{
	last_error = OK;

#ifdef ZZLIB
	if (session_tab[handle] == NULL	|| session->action != EXTRACT)
	{
		last_error = BAD_PARAMETER;
		return;
	}
	session = session_tab[handle];
#endif /* ZZLIB */

	session->with_path = with_path;
	session->output_filename = filename;

	Uncompress(UNCOMPRESS);
	if (last_error != OK) return;

	if (info != NULL)
	{
		info->filename = session->output_filename;
		info->filetime = session->head_file.time;
		info->input_size = session->head_file.packed_size;
		info->output_size = session->head_file.original_size;
	}
	session->output_filename = NULL;
}

/*---------------------------------------------*/
/* end                                 block.c */
/*---------------------------------------------*/
