/******************************************************************************
**  LZW codec
**  --------------------------------------------------------------------------
**
**  Header file for LZW codec.
**
**  Author: V.Antonenko
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2 of the License,
** or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.
**
******************************************************************************/
#ifndef __LZW_H__

// do not set DICT_SIZE > 24bit (32-bit bit-buffer is too short)
#define DICT_SIZE	(1 << 20)
#define CODE_NULL	DICT_SIZE
#define HASH_SIZE	(DICT_SIZE)

#define LZW_ERR_DICT_IS_FULL	-1
#define LZW_ERR_INPUT_BUF		-2
#define LZW_ERR_WRONG_CODE		-3

// bit-buffer
typedef struct _bitbuffer
{
	unsigned long buf;		// bits
	unsigned n;				// number of bits
}
bitbuffer_t;

// LZW encoder node, represents a string
typedef struct _node_enc
{
	int           prev;		// prefix code
	int           next;		// next child code
	unsigned char ch;		// last symbol
}
node_enc_t;

// LZW decoder node, represents a string
typedef struct _node_dec
{
	int           prev;		// prefix code
	unsigned char ch;		// last symbol
}
node_dec_t;

// LZW encoder context
typedef struct _lzw_enc
{
	int           code;				// current code
	unsigned      max;				// maximal code
	unsigned      codesize;			// number of bits in code
	bitbuffer_t   bb;				// bit-buffer struct
	void          *stream;			// pointer to the stream object
	unsigned      lzwn;				// output code-buffer byte counter
	node_enc_t    dict[DICT_SIZE];	// code dictionary
	int           hash[HASH_SIZE];	// hast table
	unsigned char buff[256];		// output code-buffer
}
lzw_enc_t;

// LZW decoder context
typedef struct _lzw_dec
{
	int           code;				// current code
	unsigned      max;				// maximal code
	unsigned      codesize;			// number of bits in code
	bitbuffer_t   bb;				// bit-buffer struct
	void          *stream;			// pointer to the stream object
	unsigned      lzwn;				// input code-buffer byte counter
	unsigned      lzwm;				// input code-buffer size
	unsigned char *inbuff;		    // input code-buffer
	node_dec_t    dict[DICT_SIZE];	// code dictionary
	unsigned char c;				// first char of the code
	unsigned char buff[DICT_SIZE];	// output string buffer
}
lzw_dec_t;

void lzw_enc_init(lzw_enc_t *ctx, void *stream);
int  lzw_encode  (lzw_enc_t *ctx, char buf[], unsigned size);
void lzw_enc_end (lzw_enc_t *ctx);

void lzw_dec_init(lzw_dec_t *ctx, void *stream);
int  lzw_decode  (lzw_dec_t *ctx, char buf[], unsigned size);

// Application defined stream callbacks
void     lzw_writebuf(void *stream, char *buf, unsigned size);
unsigned lzw_readbuf (void *stream, char *buf, unsigned size);

#endif //__LZW_H__
