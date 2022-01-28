/*---------------------------------------------*/
/* Zzip/Zzlib compressor       struct_model1.c */
/* struct model compress/uncompress order-0    */
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
#include "ac-common.h"

/*---------------------------------------------*/

typedef struct
{
	uint32 char_to_index;
	uint32 index_to_char;
} s_SM1_i;

typedef struct
{
	uint32 freq;
	uint32 cum;
} s_SM1_f;

static s_SM1_i     SM1_i[NO_LEVEL_1 + 1]  ALIGN;
static s_SM1_i tab_SM1_i[NO_LEVEL_1][256] ALIGN;
static s_SM1_f     SM1_f[NO_LEVEL_1 + 1]  ALIGN;
static s_SM1_f tab_SM1_f[NO_LEVEL_1][256] ALIGN;

/*---------------------------------------------*/

/* Start the model (init) sub-function */
static
void Start_Model_2(s_SM1_i *t_i, 
					   s_SM1_f *t_f)
{
	uint i;

	for (i = 0; i < t_f[0].freq; ++i)
    {
		t_i[i].char_to_index = i + 1;
		t_i[i + 1].index_to_char = i;
    }

	t_f[0].cum = t_f[0].freq;
	for (i = 1; i <= t_f[0].freq; ++i)
	{
		t_f[i].freq = 1;
		t_f[i].cum = t_f[0].freq - i;
	}
}

/*---------------------------------------------*/

/* Start the model (init) main function */
static
void Start_Model()
{
	uint i, cum = 0, max, l;

	for (i = 0; i < NO_LEVEL_1; ++i)
    {
		SM1_i[i].char_to_index = i + 1;
		SM1_i[i + 1].index_to_char = i;
    }

	SM1_f[0 + 1].freq = 1;
	i = 1;

	max = block.mtf_max_char - 1;
	while (max > 0)
	{
		l = 1 << i;
		if (max < l)
		{
			i--;
			tab_SM1_f[i][0].freq += max;
			tab_SM1_f[i + 1][0].freq = 0;
			Start_Model_2(tab_SM1_i[i], tab_SM1_f[i]);
			max = 0;
		}
		else
		{
			max -= l;
			SM1_f[i + 1].freq = 1;
			tab_SM1_f[i][0].freq = l;
			Start_Model_2(tab_SM1_i[i], tab_SM1_f[i]);
		}
		i++;
	}

	SM1_f[0].freq = i;
	for (; i > 0; --i)
    {
		SM1_f[i].cum = cum;
		cum += SM1_f[i].freq;
    }
	SM1_f[0].cum = cum;
}

/*---------------------------------------------*/

/* Update of the model for the specified symbol (structured model level 2) */
INLINE static
void Update_Model_2(uint    symbol, 
					s_SM1_i *t_i, 
					s_SM1_f *t_f)
{
	uint k, fs, cum;
	uint max = MIN(t_f[0].freq * MAX_FREQUENCY2, MAX_FREQ_MODEL);

	if (t_f[0].cum >= max)
	{
		cum = 0;
		for (k = t_f[0].freq; k > 0; --k)
		{
			t_f[k].cum = cum;
			cum += (t_f[k].freq = (t_f[k].freq + 1) >> 1);
		}
		t_f[0].cum = cum;
	}

	k = symbol - 1;
	fs = t_f[symbol].freq;
	while ((k > 0) && (fs == t_f[k].freq)) k--;

	if (++k < symbol)
    {
		uint ch_k, ch_symbol;
		ch_k = t_i[k].index_to_char;
		ch_symbol = t_i[symbol].index_to_char;
		t_i[k].index_to_char         = ch_symbol;
		t_i[symbol].index_to_char    = ch_k;
		t_i[ch_k].char_to_index      = symbol;
		t_i[ch_symbol].char_to_index = k;
    }

	t_f[k].freq++;
	while (k > 0) t_f[--k].cum++;
}

/*---------------------------------------------*/

/* Update of the model (twice) for the specified symbol (structured model level 1) */
INLINE static
void Update_Model_1(const uint smbl)
{
	uint k, fs, cum, symbol = smbl;

	if (SM1_f[0].cum >= MAX_FREQUENCY1) 
	{	
		cum = 0;
		for (k = SM1_f[0].freq; k > 0; --k)
		{	
			SM1_f[k].cum = cum;
			cum += (SM1_f[k].freq = (SM1_f[k].freq + 1) >> 1); 
		}
		SM1_f[0].cum = cum; }

	k = symbol - 1; fs = SM1_f[symbol].freq;
	while ((k > 0) && (fs == SM1_f[k].freq)) k--;

	if (++k < symbol)
    {	
		uint ch_k, ch_symbol;
		ch_k = SM1_i[k].index_to_char;
		ch_symbol = SM1_i[symbol].index_to_char;
		SM1_i[k].index_to_char         = ch_symbol;
		SM1_i[symbol].index_to_char    = ch_k;
		SM1_i[ch_k].char_to_index      = symbol;
		SM1_i[ch_symbol].char_to_index = k; 
	}

	symbol = k;	k--; fs++;
	while ((k > 0) && (fs == SM1_f[k].freq)) k--;

	if (++k < symbol)
    {	
		uint ch_k, ch_symbol;
		ch_k = SM1_i[k].index_to_char;
		ch_symbol = SM1_i[symbol].index_to_char;
		SM1_i[k].index_to_char         = ch_symbol;
		SM1_i[symbol].index_to_char    = ch_k;
		SM1_i[ch_k].char_to_index      = symbol;
		SM1_i[ch_symbol].char_to_index = k; 
	}

	SM1_f[k].freq++;
	SM1_f[symbol].freq++;
	while (symbol > k) SM1_f[--symbol].cum++;
	while (symbol > 0) SM1_f[--symbol].cum += 2;
}

/*---------------------------------------------*/

#ifndef SFX

/* Compress a block with an order-0 Structured Model */
uint32 Zip_SM1(uint32 len, 
			   uint8  *bufin)
{
	uint32 i = 0;
	uint32 buffer = 0, low, high, bits_to_go;
	sint32 bits_to_follow;
	uint8  *buf = NULL;
	
	buf = block.buffer;

	Start_Model();
	START_OUTPUTING_BITS();
	START_ENCODING();

	while (i < len)
    {
		uint j, ch, symbol;

		ch = bufin[i++] - 2;

		j = 0;
		if (ch != 0)
		{
			ch++;
			while ((ch >> ++j) != 1);
			if (tab_SM1_f[j][0].freq == 0) j--;
			ch -= 1 << j;
		}

		symbol = SM1_i[j].char_to_index;
		ENCODE_SYMBOL_SM1(symbol, SM1_f, SM1_f[0].cum);
		Update_Model_1(symbol);

		if (j != 0)
		{
			symbol = tab_SM1_i[j][ch].char_to_index;
			ENCODE_SYMBOL_SM1(symbol, tab_SM1_f[j], tab_SM1_f[j][0].cum);
			Update_Model_2(symbol, tab_SM1_i[j], tab_SM1_f[j]);
		}
	}

	DONE_ENCODING();
	DONE_OUTPUTING_BITS();

	return (buf - block.buffer);
}

#endif /* !SFX */

/*---------------------------------------------*/

/* Uncompress a block with an order-0 Structured Model */
void Unzip_SM1(uint32 len, 
			   uint8  *bufout)
{
	uint32 i = 0, buffer = 0, low, high, value, bits_to_go;
	uint8  *buf = NULL, *length_buf = NULL;
	
	buf = block.buffer;
	length_buf = block.buffer_length;

	Start_Model();
	START_INPUTING_BITS();
	START_DECODING();

	while (i < len)
    {
		uint ch, ch2, symbol;

		DECODE_SYMBOL_SM1(symbol, SM1_f);
		ch = SM1_i[symbol].index_to_char;
		Update_Model_1(symbol);

		if (ch > 0)
		{
			ch2 = ch;
			DECODE_SYMBOL_SM1(symbol, tab_SM1_f[ch2]);
			ch = tab_SM1_i[ch2][symbol].index_to_char;
			Update_Model_2(symbol, tab_SM1_i[ch2], tab_SM1_f[ch2]);
			ch += 1 << ch2;
			ch--;
		}

		bufout[i++] = ch + 2;
    }
}

/*---------------------------------------------*/
/* end                         struct_model1.c */
/*---------------------------------------------*/
