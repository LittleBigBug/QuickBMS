/*---------------------------------------------*/
/* Zzip/Zzlib compressor              coding.c */
/* (de)coding functions (RLE,WIN32,MM,unBWT...)*/
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

#include "zzip.h"

/*---------------------------------------------*/

static uint32 cc[256] ALIGN;

/*
 * unBWT for a given block (from bufinout to bufinout)
 * uses 4*len bytes + bufinout
 * len must be < (1<<24) (16Mb)
 */
void BWT_Decoding(uint32 len, 
				  uint32 first, 
				  uint8  *bufinout, 
				  uint32 *offset)
{
	memset(cc, 0, sizeof(uint32) * 256);

	/* bufinout is spread over the high bytes (0xFF000000) of offset */
	{
		uint32 i = 0;

		for (; i < len; ++i)
			offset[i] = ((uint32)bufinout[ i ]) << 24;
	}

	/* we use only the lower bytes (0x00FFFFFF) of offset to store the counts */
	{
		uint32 *olen, *o = offset;

		/* we skip 'first' */
		olen = offset + first;
		for (; o < olen; ++o)
			*o |= cc[*o >> 24]++;

		o++;
		olen = offset + len;
		for (; o < olen; ++o)
			*o |= cc[*o >> 24]++;

		offset[first] |= cc[offset[first] >> 24]++;
	}

	
	{
		uint32 k = 255, j = len;

		do cc[k] = (j -= cc[k]);
		while (--k != 0);
		cc[0] = 0;
	}
	

	{
		uint8  *b = bufinout + len - 1; 
		uint32 j = first;

		while (b >= bufinout) 
			j = cc[*(b--) = (offset[j] >> 24)] + (offset[j] & 0x00FFFFFFUL);
	}
}

/*---------------------------------------------*/

/* reverse a block, improve compression with some binary files */
void Reverse_Block(uint8 *bufin, 
				   uint8 *bufin_end)
{

	bufin_end--;
	for (; bufin < bufin_end; ++bufin, --bufin_end)
	{	
		uint t = *bufin;
		*bufin = *bufin_end;
		*bufin_end = t;
	}
}

/*---------------------------------------------*/

#ifndef SFX

/* 
 * 32 bits asm call trick : We transform relative adresses (following the CALL 
 * opcode) into absolute ones to improve compression. With most of files, it 
 * seems to be useless to do the same with JMP opcode.
 */
void Win32_Coding(uint8 *bufin, 
				  uint8 *bufin_end)
{
	uint8 *bufin_start = bufin;

	bufin_end -= 6;
	for (; bufin < bufin_end; ++bufin)
	{
		if (*bufin == WIN32_ASM_CALL)
		{
#ifdef WIN32
			bufin++;
			*(uint32*)bufin += (uint32)(bufin - bufin_start);
			bufin += 3;
#else  /* WIN32 */ 
			/* the piece of code above doesn't seem to work under UNIX ! */
			uint32 offset = (uint32)(bufin - bufin_start);
			bufin++;
			offset += ((uint32)*(bufin+0)) << 0;
			offset += ((uint32)*(bufin+1)) << 8;
			offset += ((uint32)*(bufin+2)) << 16;
			offset += ((uint32)*(bufin+3)) << 24;
			*(bufin+0) = (uint8)(offset >> 0);
			*(bufin+1) = (uint8)(offset >> 8);
			*(bufin+2) = (uint8)(offset >> 16);
			*(bufin+3) = (uint8)(offset >> 24);
			bufin += 3;
#endif /* WIN32 */
		}
	}
}

#endif /* !SFX */

/*---------------------------------------------*/

/* The reverse operation */
void Win32_Decoding(uint8 *bufin, 
					uint8 *bufin_end)
{
	uint8 *bufin_start = bufin;

	bufin_end -= 6;
	for (; bufin < bufin_end; ++bufin)
	{
		if (*bufin == WIN32_ASM_CALL)
		{
#ifdef WIN32
			bufin++;
			*(uint32*)bufin -= (uint32)(bufin - bufin_start);
			bufin += 3;
#else  /* WIN32 */
			uint32 offset = 0;
			bufin++;
			offset += ((uint32)*(bufin+0)) << 0;
			offset += ((uint32)*(bufin+1)) << 8;
			offset += ((uint32)*(bufin+2)) << 16;
			offset += ((uint32)*(bufin+3)) << 24;
			offset -=  (uint32)(bufin - 1 - bufin_start);
			*(bufin+0) = (uint8)(offset >> 0);
			*(bufin+1) = (uint8)(offset >> 8);
			*(bufin+2) = (uint8)(offset >> 16);
			*(bufin+3) = (uint8)(offset >> 24);
			bufin += 3;
#endif /* WIN32 */
		}
	}
}

/*---------------------------------------------*/

#ifndef SFX

/* trick to compute an absolute value without any test/jump */
INLINE static
uint32 MyAbs(sint32 a)
{
	ssint64 s;
	s.s64 = a;
	return (s.d.l^s.d.h)-s.d.h;
}

/* 
 * We try to find out if it's a "multimedia" (image, audio) block and what is 
 * its interlacing, because raw "multimedia" files (24 bits images, 8/16 bits 
 * mono/stereo audio files) are usually interlaced we try different interlacing
 * and calculate the mean distance.
 */
uint MM_Test(uint8 *bufin, 
			 uint8 *bufin_end)
{
	union
	{
		uint16 *b16;
		uint8  *b8;
	} b;
	uint32 len32 = bufin_end - bufin;
	uint8  *b8_end = bufin_end - 1;
	uint32 s1 = 0, s2 = 0;
	uint32 t1 = 0, t2 = 0, t3 = 0;

	for (b.b8 = bufin + 4; b.b8 < b8_end; b.b8 += 2)
	{
		s1 += MyAbs(*(b.b16) - *(b.b16-1)) >> 8;
		s2 += MyAbs(*(b.b16) - *(b.b16-2)) >> 8;
		t1 += MyAbs(*(b.b8+1) - *(b.b8-0));
		t1 += MyAbs(*(b.b8+0) - *(b.b8-1));
		t2 += MyAbs(*(b.b8+1) - *(b.b8-1));
		t2 += MyAbs(*(b.b8+0) - *(b.b8-2));
		t3 += MyAbs(*(b.b8+1) - *(b.b8-2));
		t3 += MyAbs(*(b.b8+0) - *(b.b8-3));
	}

/*	printf("\n|Wav16:%.2f,%.2f|", (double)s1/len32, (double)s2/len32);
	printf("Wav8:%.2f,%.2f,%.2f|", (double)t1/len32, (double)t2/len32, (double)t3/len32);*/

	/* these thresholds seem to work ;) */
	if (t1 < 13*len32) return 1;
	else if (t2 < 13*len32) return 2;
	else if (t3 < 14*len32) return 3;
	else if (s1 < 13*len32) return 4;
	else if (s2 < 17*len32) return 5;
	else return 0;
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifndef SFX

/*
 * We do a delta-encoding on the block (according to the interlacing), it 
 * improves compression with "multimedia" files.
 */
void MM_Coding(uint8 *bufin, 
			   uint8 *bufin_end)
{
	switch (block.mm_type)
	{
	case 1: /* interlacing 1:1 (byte) */
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u1.b.ll = u2.b.ll - u1.b.hh;
				u1.b.lh = u2.b.lh - u2.b.ll;
				u1.b.hl = u2.b.hl - u2.b.lh;
				u1.b.hh = u2.b.hh - u2.b.hl;
				*b32 = u1.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 2: /* interlacing 1:2 (byte) */
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u1.b.ll = u2.b.ll - u1.b.hl;
				u1.b.lh = u2.b.lh - u1.b.hh;
				u1.b.hl = u2.b.hl - u2.b.ll;
				u1.b.hh = u2.b.hh - u2.b.lh;
				*b32 = u1.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 3: /* interlacing 1:3 (byte) */
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u1.b.ll = u2.b.ll - u1.b.lh;
				u1.b.lh = u2.b.lh - u1.b.hl;
				u1.b.hl = u2.b.hl - u1.b.hh;
				u1.b.hh = u2.b.hh - u2.b.ll;
				*b32 = u1.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 4: /* interlacing 1:1 (word) */
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u1.w.l = u2.w.l - u1.w.h;
				u1.w.h = u2.w.h - u2.w.l;
				*b32 = u1.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 5: /* interlacing 1:2 (word) */
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u1.w.l = u2.w.l - u1.w.l;
				u1.w.h = u2.w.h - u1.w.h;
				*b32 = u1.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	}
}

#endif

/*---------------------------------------------*/

/* The reverse operation */
void MM_Decoding(uint8 *bufin, 
				 uint8 *bufin_end)
{
	switch (block.mm_type)
	{
	case 1:
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u2.b.ll += u1.b.hh;
				u2.b.lh += u2.b.ll;
				u2.b.hl += u2.b.lh;
				u2.b.hh += u2.b.hl;
				*b32 = u2.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 2:
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u2.b.ll += u1.b.hl;
				u2.b.lh += u1.b.hh;
				u2.b.hl += u2.b.ll;
				u2.b.hh += u2.b.lh;
				*b32 = u2.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 3:
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u2.b.ll += u1.b.lh;
				u2.b.lh += u1.b.hl;
				u2.b.hl += u1.b.hh;
				u2.b.hh += u2.b.ll;
				*b32 = u2.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 4:
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u2.w.l += u1.w.h;
				u2.w.h += u2.w.l;
				*b32 = u2.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	case 5:
		{
			uint32  *b32 = (uint32*)bufin + 1;
			uint32  *b32_end = (uint32*)bufin_end + 1;
			uuint32 u1, u2;
			u1.u32 = *(uint32*)bufin;
			for (; b32 < b32_end; ++b32)
			{
				u2.u32 = *b32;
				u2.w.l += u1.w.l;
				u2.w.h += u1.w.h;
				*b32 = u2.u32;
				u1.u32 = u2.u32;
			}
		}
		break;
	}
}

/*---------------------------------------------*/

#ifndef SFX

static const uint move[256] ALIGN = 
{ 0,1,2,3,4,5,6,7,8,9,31,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
96,32,10,33,34,44,59,58,63,39,40,41,42,43,45,35,46,47,48,49,50,51,52,53,54,55,56,
57,37,36,60,62,61,38,64,65,70,71,72,66,74,73,75,67,83,84,77,79,80,68,81,82,76,78,
85,69,87,86,88,89,90,91,92,93,94,95,30,97,102,103,104,98,106,105,107,99,115,116,
109,111,112,100,113,114,108,110,117,101,119,118,120,121,122,
130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,
150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,
171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,
213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,
234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,
255,123,124,125,126,127,128,129 };

/*
 * some ASCII transformations: alphabet reordering, carriage-return tagging,
 * upper-case letter tagging
 */
uint32 Filter1(uint8  *bufin, 
			   uint8  *bufout, 
			   uint32 len)
{
	uint  b0, b1;
	uint8 *bout = bufout, *blen = bufin + len;

	b1 = *bufin++;
	while (bufin < blen)
	{
		b0 = b1;
		b1 = *bufin++;
		/* upper-case letter tagging */
		if (((b0 - 65) < 26) | (b0 == TAG_CAPS))
		{
			*bout++ = 96;/* move[TAG_CAPS]; */
			b0 += 32;
		}

		*bout++ = move[b0];

		/* carriage-return tagging */
		if ((b0 == ASCII_CR) && ((b1 == ASCII_SP) | ((b1 - 97) < 26)))
		{
			*bout++ = 10;/* move[32]; */
		}
	}
	*bout++ = move[b1];

	return (bout - bufout);
}

#endif /* !SFX */

/*---------------------------------------------*/

static const uint unmove[256] ALIGN = 
{ 0,1,2,3,4,5,6,7,8,9,32,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,
27,28,29,96,10,31,33,34,45,59,58,63,39,40,41,42,43,35,44,46,47,48,49,50,51,52,53,
54,55,56,57,37,36,60,62,61,38,64,65,69,73,79,85,66,67,68,71,70,72,82,76,83,77,78,
80,81,74,75,84,87,86,88,89,90,91,92,93,94,95,30,97,101,105,111,117,98,99,100,103,
102,104,114,108,115,109,110,112,113,106,107,116,119,118,120,121,122,249,250,251,
252,253,254,255,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,
139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,
159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,
179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,
199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
239,240,241,242,243,244,245,246,247,248 };

/* The reverse operation */
uint32 UnFilter1(uint8  *bufin, 
				 uint32 len)
{
	uint  b0, b1;
	uint8 *bin = bufin, *blen = bufin + len, *bout = bufin;

/*	int i, j;
	printf("\n");
	for(i = 0; i < 256+1; ++i)
	{
		j = 0;
		while(move[j] != i) j++;
		printf("%u,", j);
	}
	printf("\n");*/

	b1 = unmove[*bin++];
	while (bin < blen)
	{
		b0 = b1;
		b1 = unmove[*bin++];
		
		if (b0 != TAG_CAPS)
		{
			*bout++ = b0;
			if ((b0 == ASCII_CR) && (b1 == ASCII_SP)) b1 = unmove[*bin++];
		}
		else
		{
			*bout++ = b1 - 32;
			b1 = unmove[*bin++];
		}
	}
	*bout++ = b1;

	return (bout - bufin);
}

/*---------------------------------------------*/

#ifndef SFX

/* 
 * Some phrase replacements.
 * It's far from being optimal, but I can't find any better substitution.
 */
uint32 Filter2(uint8 *bufin, 
			   uint8 *bufin_end)
{
	uint8  *buf, *bufout = bufin;

	for (buf = bufin; buf < (bufin_end - 3); ++buf)
	{
		uint32 b = *(uint32*)buf; 
		switch (b & 0x00FFFFFF)
		{
		case 6450037 /* the */: *bufout++ = 129; buf+=2; break;
		case 6644857 /* you */: *bufout++ = 128; buf+=2; break;
		case 7103594 /* for */: *bufout++ = 127; buf+=2; break;
		case 6844513 /* and */: *bufout++ = 126; buf+=2; break;
		case 7823723 /* hav */: *bufout++ = 125; buf+=2; break;
		default:
			switch (b & 0x0000FFFF)
			{
			case 28259 /* he */: *bufout++ = 124; buf++; break;
			case 24950 /* nd */: *bufout++ = 123; buf++; break;
			default : *bufout++ = *buf; break;
			}
			break;
		}
	}

	for (; buf < bufin_end; ++buf)
		*bufout++ = *buf;

	return (bufout-bufin);
}

#endif /* !SFX */

/*---------------------------------------------*/

/* The reverse operation */
uint32 UnFilter2(uint8 *bufin, 
				 uint8 *bufout, 
				 uint8 *bufin_end)
{
	uint8  *bufout_sav = bufout;

	for (; bufin < bufin_end; ++bufin)
	{
		switch (*bufin)
		{
		case 129 : *(uint32*)bufout = 6450037; bufout += 3; break;
		case 128 : *(uint32*)bufout = 6644857; bufout += 3; break;
		case 127 : *(uint32*)bufout = 7103594; bufout += 3; break;
		case 126 : *(uint32*)bufout = 6844513; bufout += 3; break;
		case 125 : *(uint32*)bufout = 7823723; bufout += 3; break;
		case 124 : *(uint16*)bufout = 28259;   bufout += 2; break;
		case 123 : *(uint16*)bufout = 24950;   bufout += 2; break;
		default  : *bufout++ = *bufin;
		}
	}

	return (bufout - bufout_sav);
}

/*---------------------------------------------*/

#ifndef SFX

static uint32 tab[257] ALIGN;

/* Block analysis */
void Analysis(uint8 *bufin, 
			  uint8 *bufin_end)
{
	uint32 buf_len = (uint32)(bufin_end - bufin);
	uint32 nb_win32 = 0, above = 0, below = 0, nb_he = 0, max;
	uint32 count = 0, max_count = 0;
	uint8  *buf;
	sint32 i;

	memset(tab, 0, sizeof(uint32) * 257);

	for (buf = bufin; buf < bufin_end; ++buf)
	{
		uint b = *(uint16*)buf;

		nb_he += (b == ASCII_H_E); 

		if ((b & 0xFF) == (b >> 8)) count++;
		else 
		{
			if (count > max_count) max_count = count;
			count = 0;
		}

		b &= 0xFF;

		if (b == WIN32_ASM_CALL && *(buf + 4) == 0) nb_win32++;

		tab[b]++;
	}
	max_count = MAX(count, max_count);

	if (max_count > RUN_LENGTH_MAX) block.rle_encoding = true;
	else block.rle_encoding = false;

	/* look for a free char */
	i = 0;
	while (tab[i] != 0) i++;
	
	/* decide if it is a binary block (BIN type) */
	if ((i > 255) 
		&& (block.type == NO_TYPE) 
		&& (block.compression_mode != 0)) 
	{
		block.type = BIN;
	}

	/* find max char value */
	i = 255;
	while (tab[i] == 0) i--;
	max = i;

	/* count the chars out of the ASCII [10-127] set */
	for (i = 0; i < 10; ++i)
		below += tab[i];
	for (i = 128; i < 256; ++i)
		above += tab[i];

	/* decide if it is a text block (TEXT type) */
	if (((block.compression_mode & 1) == 1) 
		&& (block.type == NO_TYPE) 
		&& (max > 32) 
		&& ((above + below) < (buf_len >> 4))) /* no more than 6% out of the ASCII set */
	{
		 block.type = TEXT;
	}

	if (((nb_he*1000) > (4*buf_len)) 
		&& (buf_len > 128 * 1024) 
		&& (block.type == TEXT) 
		&& (max < 256-7))
	{
		block.english_encoding = true;
	}
	else
	{
		block.english_encoding = false;
	}

	if ((block.compression_mode != 0) 
		&& ((block.type == NO_TYPE) | (block.type == BIN)) 
		&& ((tab[0xE8] * 128) > (uint32)(bufin_end - bufin)) 
		&& ((nb_win32 * 6) > tab[0xE8])) 
		/* if we have more than 1/6 of 0xE8,xx,xx,00 -> it's a win32 file (WIN_EXE type) */
	{
		 block.type = WIN_EXE;
	}
}

#endif /* !SFX */

/*---------------------------------------------*/

#ifndef SFX

/*
 * split the block into two, one with the charset [0..2], and the other with
 * the charset [2..255]. By the way, it finds the max char value.
 */
uint32 Split(uint8 *bufin, 
			 uint8 *bufin_end, 
			 uint8 *bufout2)
{
	uint8 *sav_bufout2 = bufout2;

	block.mtf_max_char = 1;

	for (; bufin < bufin_end; ++bufin)
		if (*bufin >= 2)
		{
			block.mtf_max_char = MAX(*bufin, block.mtf_max_char);
			*bufout2++ = *bufin;
			*bufin = 2;
		}

	return (bufout2 - sav_bufout2);
}

#endif /* !SFX */

/*---------------------------------------------*/

/* The reverse operation */
void UnSplit(uint8 *bufin, 
			 uint8 *bufin_end, 
			 uint8 *bufin2)
{
	for (; bufin < bufin_end; ++bufin)
		if (*bufin == 2) *bufin = *bufin2++;
}

/*---------------------------------------------*/

static const uint32 crc32_table[] ALIGN = {
0UL, 16777216UL, 33554432UL, 50331648UL, 67108864UL, 83886080UL, 100663296UL, 
117440512UL, 134217728UL, 150994944UL, 167772160UL, 184549376UL, 201326592UL, 
218103808UL, 234881024UL, 251658240UL, 268435456UL, 285212672UL, 301989888UL, 
318767104UL, 335544320UL, 352321536UL, 369098752UL, 385875968UL, 402653184UL, 
419430400UL, 436207616UL, 452984832UL, 469762048UL, 486539264UL, 503316480UL, 
520093696UL, 536870912UL, 553648128UL, 570425344UL, 587202560UL, 603979776UL, 
620756992UL, 637534208UL, 654311424UL, 671088640UL, 687865856UL, 704643072UL, 
721420288UL, 738197504UL, 754974720UL, 771751936UL, 788529152UL, 805306368UL, 
822083584UL, 838860800UL, 855638016UL, 872415232UL, 889192448UL, 905969664UL, 
922746880UL, 939524096UL, 956301312UL, 973078528UL, 989855744UL, 1006632960UL, 
1023410176UL, 1040187392UL, 1056964608UL, 1073741824UL, 1090519040UL, 1107296256UL, 
1124073472UL, 1140850688UL, 1157627904UL, 1174405120UL, 1191182336UL, 1207959552UL, 
1224736768UL, 1241513984UL, 1258291200UL, 1275068416UL, 1291845632UL, 1308622848UL, 
1325400064UL, 1342177280UL, 1358954496UL, 1375731712UL, 1392508928UL, 1409286144UL, 
1426063360UL, 1442840576UL, 1459617792UL, 1476395008UL, 1493172224UL, 1509949440UL, 
1526726656UL, 1543503872UL, 1560281088UL, 1577058304UL, 1593835520UL, 1610612736UL, 
1627389952UL, 1644167168UL, 1660944384UL, 1677721600UL, 1694498816UL, 1711276032UL, 
1728053248UL, 1744830464UL, 1761607680UL, 1778384896UL, 1795162112UL, 1811939328UL, 
1828716544UL, 1845493760UL, 1862270976UL, 1879048192UL, 1895825408UL, 1912602624UL, 
1929379840UL, 1946157056UL, 1962934272UL, 1979711488UL, 1996488704UL, 2013265920UL, 
2030043136UL, 2046820352UL, 2063597568UL, 2080374784UL, 2097152000UL, 2113929216UL, 
2130706432UL, 2147483648UL, 2164260864UL, 2181038080UL, 2197815296UL, 2214592512UL, 
2231369728UL, 2248146944UL, 2264924160UL, 2281701376UL, 2298478592UL, 2315255808UL, 
2332033024UL, 2348810240UL, 2365587456UL, 2382364672UL, 2399141888UL, 2415919104UL, 
2432696320UL, 2449473536UL, 2466250752UL, 2483027968UL, 2499805184UL, 2516582400UL, 
2533359616UL, 2550136832UL, 2566914048UL, 2583691264UL, 2600468480UL, 2617245696UL, 
2634022912UL, 2650800128UL, 2667577344UL, 2684354560UL, 2701131776UL, 2717908992UL, 
2734686208UL, 2751463424UL, 2768240640UL, 2785017856UL, 2801795072UL, 2818572288UL, 
2835349504UL, 2852126720UL, 2868903936UL, 2885681152UL, 2902458368UL, 2919235584UL, 
2936012800UL, 2952790016UL, 2969567232UL, 2986344448UL, 3003121664UL, 3019898880UL, 
3036676096UL, 3053453312UL, 3070230528UL, 3087007744UL, 3103784960UL, 3120562176UL, 
3137339392UL, 3154116608UL, 3170893824UL, 3187671040UL, 3204448256UL, 3221225472UL, 
3238002688UL, 3254779904UL, 3271557120UL, 3288334336UL, 3305111552UL, 3321888768UL, 
3338665984UL, 3355443200UL, 3372220416UL, 3388997632UL, 3405774848UL, 3422552064UL, 
3439329280UL, 3456106496UL, 3472883712UL, 3489660928UL, 3506438144UL, 3523215360UL, 
3539992576UL, 3556769792UL, 3573547008UL, 3590324224UL, 3607101440UL, 3623878656UL, 
3640655872UL, 3657433088UL, 3674210304UL, 3690987520UL, 3707764736UL, 3724541952UL, 
3741319168UL, 3758096384UL, 3774873600UL, 3791650816UL, 3808428032UL, 3825205248UL, 
3841982464UL, 3858759680UL, 3875536896UL, 3892314112UL, 3909091328UL, 3925868544UL, 
3942645760UL, 3959422976UL, 3976200192UL, 3992977408UL, 4009754624UL, 4026531840UL, 
4043309056UL, 4060086272UL, 4076863488UL, 4093640704UL, 4110417920UL, 4127195136UL, 
4143972352UL, 4160749568UL, 4177526784UL, 4194304000UL, 4211081216UL, 4227858432UL, 
4244635648UL, 4261412864UL, 4278190080UL };

/* Compute the CRC32 for a block. */
uint32 Crc32(uint8  *buffer, 
			 uint8  *buffer_end, 
			 uint32 crc)
{
	while (buffer < buffer_end)
		crc = (crc >> 8) ^ crc32_table[(crc ^ *buffer++) & 0xFF];

	return crc;
}

/*---------------------------------------------*/

#ifndef SFX

/* RLE (variant) compression, useful for removing runs */
uint32 RLE_Coding(uint8 *bufin, 
				  uint8 *bufout, 
				  uint8 *bufend)
{
	uint32 c, last = 0, count = 0;
	uint8  *bufsav = bufout;

	while (bufin < bufend)
    {
		*bufout++ = (c = *bufin++);
		if (c != last) 
		{ 
			count = 0; 
			last = c; 
		}
		else if (++count == 7)
		{
			count = 0;
			while ((*bufin == c) & (bufin < bufend)) 
			{ 
				count++; 
				bufin++;
			}

			while (count != 0)
			{
				*bufout++ = c + (count & 1);
				count >>= 1;
			}
			if ((bufin == bufend)
				| (*bufin == ((c + 1) & 0xFF)) 
				| (*bufin == ((c + 2) & 0xFF)))
			{
				*bufout++ = (c + 2) & 0xFF;
			}
        }
    }

	return (bufout - bufsav);
}

#endif /* !SFX */

/*---------------------------------------------*/

/* The reverse operation */
uint32 RLE_Decoding(uint8 *bufin, 
					uint8 *bufout, 
					uint8 *bufend)
{
	uint32 c, last = 0, count = 0;
	uint8  *bufsav = bufout;

	while (bufin < bufend)
    {
		*bufout++ = (c = *bufin++);
		if (c != last) 
		{ 
			count = 0; 
			last = c; 
		}
		else if (++count == 7)
		{
			uint i = 0;
			count = 0;

			while ((*bufin == ((c + 1)&0xFF)) | (*bufin == c))
				count |= (uint32)((*bufin++ - ((c + 1)&0xFF) + 1)&0xFF) << (i++);

			if (*bufin == ((c + 2)&0xFF)) bufin++;

			for (; count > 0; --count) *bufout++ = c;
			count = 0;
        }
    }

	return (bufout - bufsav);
}

/*---------------------------------------------*/

static uint order[256] ALIGN;

#ifndef SFX

/* MTF (in fact a "move one from front") encoding function */
void M1FF2_Coding(uint8 *bufinout, 
				  uint8 *bufend)
{
	uint i, c, l = 0, flag = 0;

	for (i = 0; i < 256; ++i) order[i] = i;
	
	for (; bufinout < bufend; ++bufinout)
    {
		c = *bufinout; 
		l = 0;
		while (order[l] != c) l++;
		*bufinout = l;

		if (l > 1)
		{
			for (; l > 1; --l) order[l] = order[l - 1];
			order[1] = c;
			l = 1;
		}
		else if ((l == 1) & flag)
		{ 
			order[1] = order[0]; 
			order[0] = c;
		}
		flag = l;
    }
}

#endif /* !SFX */

/*---------------------------------------------*/

/* MTF decoding function */
void M1FF2_Decoding(uint8 *bufinout, 
					uint8 *bufend)
{
	uint i, c, l = 0, flag = 0;

	for (i = 0; i < 256; ++i) order[i] = i;
	
	for (; bufinout < bufend; ++bufinout)
    {
		l = *bufinout;
		c = order[l];
		*bufinout = c;

		if (l > 1)
		{
			for (; l > 1; --l) order[l] = order[l - 1];
			order[1] = c;
			l = 1;
		}
		else if ((l == 1) & flag)
		{ 
			order[1] = order[0]; 
			order[0] = c;
		}
		flag = l;
    }
}

/*---------------------------------------------*/
/* end                                coding.c */
/*---------------------------------------------*/
