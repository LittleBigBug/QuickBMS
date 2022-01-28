/******************************************************************************
**  LZW decoder
**  --------------------------------------------------------------------------
**  
**  Decompresses data using LZW algorithm.
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
#include "lzw.h"


/******************************************************************************
**  lzw_dec_readbits
**  --------------------------------------------------------------------------
**  Read bits from bit-buffer.
**  The number of bits should not exceed 24.
**  
**  Arguments:
**      ctx     - pointer to LZW context;
**      nbits   - number of bits to read, 0-24;
**
**  Return: bits or -1 if there is no data
******************************************************************************/
static int lzw_dec_readbits(lzw_dec_t *const ctx, unsigned nbits)
{
	// read bytes
	while (ctx->bb.n < nbits)
	{
		if (ctx->lzwn == ctx->lzwm)
			return -1;

		// shift old bits to the left, add new to the right
		ctx->bb.buf = (ctx->bb.buf << 8) | ctx->inbuff[ctx->lzwn++];
		ctx->bb.n += 8;
	}

	ctx->bb.n -= nbits;

	return (ctx->bb.buf >> ctx->bb.n) & ((1 << nbits)-1);
}

/******************************************************************************
**  lzw_dec_init
**  --------------------------------------------------------------------------
**  Initializes LZW decoder context.
**  
**  Arguments:
**      ctx     - LZW decoder context;
**      stream  - Pointer to application defined Input/Output stream object;
**
**  Return: -
******************************************************************************/
void lzw_dec_init(lzw_dec_t *ctx, void *stream)
{
	unsigned i;

	ctx->code     = CODE_NULL;
	ctx->max      = 255;
	ctx->codesize = 8;
	ctx->bb.n     = 0; // bitbuffer init
	ctx->stream   = stream;

	for (i = 0; i < 256; i++)
	{
		ctx->dict[i].prev = CODE_NULL;
		ctx->dict[i].ch   = i;
	}
}

/******************************************************************************
**  lzw_dec_reset
**  --------------------------------------------------------------------------
**  Reset LZW decoder context. Used when the dictionary overflows.
**  Code size set to 8 bit. Code and output str are equal in this situation.
**  
**  Arguments:
**      ctx     - LZW decoder context;
**
**  Return: -
******************************************************************************/
static void lzw_dec_reset(lzw_dec_t *const ctx)
{
	ctx->code     = CODE_NULL;
	ctx->max      = 255;
	ctx->codesize = 8;
#if DEBUG
	printf("reset\n");
#endif
}


/******************************************************************************
**  lzw_dec_getstr
**  --------------------------------------------------------------------------
**  Reads string from the LZW dictionaly. Because of particular dictionaty
**  structure the buffer is filled from the end so the offset from the 
**  beginning of the buffer will be <buffer size> - <string size>.
**  
**  Arguments:
**      ctx  - LZW context;
**      code - code of the string (already in dictionary);
**
**  Return: the number of bytes in the string
******************************************************************************/
static unsigned lzw_dec_getstr(lzw_dec_t *const ctx, int code)
{
	unsigned i = sizeof(ctx->buff);

	while (code != CODE_NULL && i)
	{
		ctx->buff[--i] = ctx->dict[code].ch;
		code = ctx->dict[code].prev;
	}

	return sizeof(ctx->buff) - i;
}

/******************************************************************************
**  lzw_dec_addstr
**  --------------------------------------------------------------------------
**  Adds string to the LZW dictionaly.
**  
**  Arguments:
**      ctx  - LZW context;
**      code - code for the string beginning (already in dictionary);
**      c    - last symbol;
**
**  Return: code representing the string or CODE_NULL if dictionary is full.
******************************************************************************/
static int lzw_dec_addstr(lzw_dec_t *const ctx, int code, unsigned char c)
{
	if (code == CODE_NULL)
		return c;
		
	if (++ctx->max == CODE_NULL)
		return CODE_NULL;

	ctx->dict[ctx->max].prev = code;
	ctx->dict[ctx->max].ch   = c;
#if DEBUG
	printf("add code %x = %x + %c\n", ctx->max, code, c);
#endif

	return ctx->max;
}

/******************************************************************************
**  lzw_dec_writestr
**  --------------------------------------------------------------------------
**  Writes a string represented by the code into output stream.
**  The code should always be in the dictionary.
**  It is important that codesize is increased after code is sent into
**  output stream.
**  
**  Arguments:
**      ctx  - LZW context;
**      code - LZW code;
**
**  Return: The first symbol of the output string.
******************************************************************************/
static unsigned char lzw_dec_writestr(lzw_dec_t *const ctx, int code)
{
	// get string for the new code from dictionary
	unsigned strlen = lzw_dec_getstr(ctx, code);

	// write the string into the output stream
	lzw_writebuf(ctx->stream, ctx->buff+(sizeof(ctx->buff) - strlen), strlen);

	// to remember the first sybmol of this string
	return ctx->buff[sizeof(ctx->buff) - strlen];
}

/******************************************************************************
**  lzw_decode
**  --------------------------------------------------------------------------
**  Decodes buffer of LZW codes and writes strings into output stream.
**  The output data is written by application specific callback to
**  the application defined stream inside this function.
**  
**  Arguments:
**      ctx  - LZW context;
**      buf  - input code buffer;
**      size - size of the buffer;
**
**  Return: Number of processed bytes or error code if the value is negative.
******************************************************************************/
int lzw_decode(lzw_dec_t *ctx, char buf[], unsigned size)
{
	if (!size) return 0;

	ctx->inbuff = buf;	// save ptr to code-buffer
	ctx->lzwn   = 0;	// current position in code-buffer
	ctx->lzwm   = size;	// code-buffer data size

	for (;;)
	{
		int ncode;

		// read a code from the input buffer (ctx->inbuff[])
		ncode = lzw_dec_readbits(ctx, ctx->codesize);
#if DEBUG
		printf("code %x (%d)\n", ncode, ctx->codesize);
#endif

		// check the input for EOF
		if (ncode < 0)
		{
#if DEBUG
			if (ctx->lzwn != ctx->lzwm)
				return LZW_ERR_INPUT_BUF;
#endif
			break;
		}
		else if (ncode <= ctx->max) // known code
		{
			// output string for the new code from dictionary
			ctx->c = lzw_dec_writestr(ctx, ncode);

			// add <prev code str>+<first str symbol> to the dictionary
			if (lzw_dec_addstr(ctx, ctx->code, ctx->c) == CODE_NULL)
				return LZW_ERR_DICT_IS_FULL;
		}
		else // unknown code
		{
			// try to guess the code
			if (ncode != ctx->max+1)
				return LZW_ERR_WRONG_CODE;

			// create code: <nc> = <code> + <c> wich is equal to ncode
			if (lzw_dec_addstr(ctx, ctx->code, ctx->c) == CODE_NULL)
				return LZW_ERR_DICT_IS_FULL;

			// output string for the new code from dictionary
			ctx->c = lzw_dec_writestr(ctx, ncode);
		}

		ctx->code = ncode;

		// increase the code size (number of bits) if needed
		if (ctx->max+1 == (1 << ctx->codesize))
			ctx->codesize++;

		// check the dictionary overflow
		if (ctx->max+1 == DICT_SIZE)
			lzw_dec_reset(ctx);
	}

	return ctx->lzwn;
}
