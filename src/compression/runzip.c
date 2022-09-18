// modified by Luigi Auriemma
/* 
   Copyright (C) Andrew Tridgell 1998-2003
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* rzip decompression algorithm */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "rzip.h"

#define fatal(...)  return -1

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef int32
#if (SIZEOF_INT == 4)
#define int32 int
#elif (SIZEOF_LONG == 4)
#define int32 long
#elif (SIZEOF_SHORT == 4)
#define int32 short
#endif
#endif

#ifndef uint32
#define uint32 unsigned int32
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif



static inline uchar read_u8(unsigned char **ss, int stream)
{
	uchar b;
    unsigned char *in = *ss;
    b = *in++;
    *ss = in;
	return b;
}

static inline unsigned read_u16(unsigned char **ss, int stream)
{
	unsigned ret;
	ret = read_u8(ss, stream);
	ret |= read_u8(ss, stream)<<8;
	return ret;
}

static inline unsigned read_u32(unsigned char **ss, int stream)
{
	unsigned ret;
	ret = read_u8(ss, stream);
	ret |= read_u8(ss, stream)<<8;
	ret |= read_u8(ss, stream)<<16;
	ret |= read_u8(ss, stream)<<24;
	return ret;
}

static inline unsigned read_u24(unsigned char **ss, int stream)
{
	unsigned ret;
	ret = read_u8(ss, stream);
	ret |= read_u8(ss, stream)<<8;
	ret |= read_u8(ss, stream)<<16;
	return ret;
}


static int read_header(unsigned char **ss, uchar *head)
{
	*head = read_u8(ss, 0);
	return read_u16(ss, 0);
}


static int unzip_literal(unsigned char **ss, int len, unsigned char **fd_out)
{
    unsigned char *in = *ss;
    unsigned char *o  = *fd_out;

    int     i;
    for(i = 0; i < len; i++) {
        *o++ = *in++;
    }

    *ss = in;
    *fd_out = o;
	return len;
}

static int unzip_match(unsigned char **ss, int len, unsigned char **fd_out)
{
    unsigned char *in = *ss;
    unsigned char *o  = *fd_out;

	unsigned offset;
	offset = read_u32(ss, 0);

    int     i;
    for(i = 0; i < len; i++) {
        *o = o[-offset];
        o++;
    }

    *ss = in;
    *fd_out = o;
	return len;
}


/* decompress a section of an open file. Call fatal() on error
   return the number of bytes that have been retrieved
 */
int runzip_chunk(unsigned char *in, unsigned char *out)
{
	uchar head;
	int len;
	unsigned char **ss = &in;
    unsigned char *o = out;
	int total = 0;

	while ((len = read_header(ss, &head)) || head) {
		switch (head) {
		case 0:
			total += unzip_literal(ss, len, &o);
			break;

		default:
			total += unzip_match(ss, len, &o);
			break;
		}
	}

	return o - out;
}
