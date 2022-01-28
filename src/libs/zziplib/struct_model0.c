/*---------------------------------------------*/
/* Zzip/Zzlib compressor       struct_model0.c */
/* 0-1-2 encoding order-3 model                */
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

#define MULT 2

static uint32 freq_tab_SM0[NO_OF_GROUPS_0][NO_OF_SYMBOLS_0] ALIGN;
static uint32 threshold[NO_OF_GROUPS_0] ALIGN;
static uint32 groups[NO_OF_SYMBOLS_0][NO_OF_SYMBOLS_0][NO_OF_SYMBOLS_0] ALIGN;
static uint32 tab_cum[NO_OF_SYMBOLS_0 + 1] ALIGN;

/*---------------------------------------------*/

/* Start the model (init) main function */
static
void Start_Model()
{
	groups[0][0][0] = 0;
	threshold[0] = 128*MULT;

	groups[0][0][1] = 1;
	threshold[1] = 48*MULT;

	groups[0][1][0] = 2;
	groups[1][1][0] = 2;
	groups[1][0][0] = 2;
	threshold[2] = 64*MULT;

	groups[0][1][1] = 3;
	groups[1][1][1] = 3;
	threshold[3] = 64*MULT;

	groups[0][2][0] = 4;
	groups[2][0][0] = 4;
	groups[2][1][0] = 4;
	threshold[4] = 72*MULT;

	groups[1][2][1] = 5;
	groups[1][2][0] = 5;
	threshold[5] = 72*MULT;

	groups[2][2][1] = 6;
	groups[2][2][0] = 6;
	threshold[6] = 72*MULT;

	groups[2][0][1] = 7;
	groups[0][0][2] = 7;
	threshold[7] = 72*MULT;

	groups[0][1][2] = 8;
	groups[1][1][2] = 8;
	groups[2][0][2] = 8;
	threshold[8] = 72*MULT;

	groups[2][1][2] = 9;
	groups[0][2][2] = 9;
	groups[1][2][2] = 9;
	threshold[9] = 72*MULT;

	groups[0][2][1] = 10;
	groups[2][1][1] = 10;
	threshold[10] = 72*MULT;

	groups[1][0][2] = 11;
	threshold[11] = 64*MULT;

	groups[1][0][1] = 12;
	threshold[12] = 64*MULT;

	groups[2][2][2] = 13;
	threshold[13] = 128*MULT;

	memset(freq_tab_SM0, 0, sizeof(uint)*NO_OF_GROUPS_0*NO_OF_SYMBOLS_0);
}

/*---------------------------------------------*/

INLINE static
uint Get_Cum_Encode(uint *cum_freq, 
					uint freq1, 
					uint freq2, 
					uint freq3)
{
	freq1++; freq2++; freq3++;
	cum_freq[0] = freq1 + freq2 + freq3;
	cum_freq[3] = 0;

	if (freq1 >= freq2)
	{ /* freq1 >= freq2 */
		if (freq2 >= freq3)
		{ /* freq1 >= freq2 >= freq3 */
			cum_freq[1] = freq2 + freq3;
			cum_freq[2] = freq3;
			return (3<<4)|(2<<2)|1;
		}
		else
		{ /* freq3 > freq2 */
			cum_freq[2] = freq2;
			if (freq3 >= freq1)
			{ /* freq3 >= freq1 >= freq2 */
				cum_freq[1] = freq1 + freq2;
				return (1<<4)|(3<<2)|2;
			}
			else
			{ /* freq1 > freq3 >= freq2 */
				cum_freq[1] = freq3 + freq2;
				return (2<<4)|(3<<2)|1;
			}
		}
	}
	else
	{ /* freq2 > freq1 */
		if (freq1 >= freq3)
		{ /* freq2 > freq1 > freq3 */
			cum_freq[1] = freq1 + freq3;
			cum_freq[2] = freq3;
			return (3<<4)|(1<<2)|2;
		}
		else
		{ /* freq3 >= freq1 */
			cum_freq[2] = freq1;
			if (freq2 >= freq3)
			{ /* freq2 > freq3 > freq1 */
				cum_freq[1] = freq3 + freq1;
				return (2<<4)|(1<<2)|3;
			}
			else
			{ /* freq3 >= freq2 > freq1 */
				cum_freq[1] = freq2 + freq1;
				return (1<<4)|(2<<2)|3;
			}
		}
	}
}

/*---------------------------------------------*/

INLINE static
uint Get_Cum_Decode(uint *cum_freq, 
					uint freq1, 
					uint freq2, 
					uint freq3)
{
	freq1++; freq2++; freq3++;
	cum_freq[0] = freq1 + freq2 + freq3;
	cum_freq[3] = 0;

	if (freq1 >= freq2)
	{ /* freq1 >= freq2 */
		if (freq2 >= freq3)
		{ /* freq1 >= freq2 >= freq3 */
			cum_freq[1] = freq2 + freq3;
			cum_freq[2] = freq3;
			return (2<<4)|(1<<2)|0;
		}
		else
		{ /* freq3 > freq2 */
			cum_freq[2] = freq2;
			if (freq3 >= freq1)
			{ /* freq3 >= freq1 >= freq2 */
				cum_freq[1] = freq1 + freq2;
				return (1<<4)|(0<<2)|2;
			}
			else
			{ /* freq1 > freq3 >= freq2 */
				cum_freq[1] = freq3 + freq2;
				return (1<<4)|(2<<2)|0;
			}
		}
	}
	else
	{ /* freq2 > freq1 */
		if (freq1 >= freq3)
		{ /* freq2 > freq1 > freq3 */
			cum_freq[1] = freq1 + freq3;
			cum_freq[2] = freq3;
			return (2<<4)|(0<<2)|1;
		}
		else
		{ /* freq3 >= freq1 */
			cum_freq[2] = freq1;
			if (freq2 >= freq3)
			{ /* freq2 > freq3 > freq1 */
				cum_freq[1] = freq3 + freq1;
				return (0<<4)|(2<<2)|1;
			}
			else
			{ /* freq3 >= freq2 > freq1 */
				cum_freq[1] = freq2 + freq1;
				return (0<<4)|(1<<2)|2;
			}
		}
	}
}

/*---------------------------------------------*/

#ifndef SFX

/* Compress a block with an order-0 Structured Model */
uint32 Zip_SM0(uint32 len, 
			   uint8  *bufin)
{
	bool   in_run = true;
	uint8  *buf = NULL;
	uint32 ch1 = 0, ch2 = 0, ch3 = 0, i = 0;
	uint32 buffer = 0, low, high, bits_to_go;
	sint32 bits_to_follow;

	buf = block.buffer;

	Start_Model();
	START_OUTPUTING_BITS();
	START_ENCODING();

	while (i < len)
    {
		uint32 group, *freq_tab, symbol, ch;

		ch = bufin[i++];

		group = groups[ch3][ch2][ch1];
		freq_tab = freq_tab_SM0[group];

		symbol = Get_Cum_Encode(tab_cum, freq_tab[0], freq_tab[1], freq_tab[2]);
		symbol = (symbol >> (ch * 2)) & 3;
		ENCODE_SYMBOL_SM0(symbol, tab_cum, tab_cum[0]);

		ch3 = ch2;
		ch2 = ch1;
		ch1 = ch;

		if ((ch | in_run | group) == 0)
		{
			in_run = true;
			freq_tab[0] >>= 1;
			freq_tab[0] += MULT + 1;
			freq_tab[1] >>= 1;
			freq_tab[2] >>= 1;
		}
		else
		{
			in_run = (ch != 0) ? false : in_run;

			if (tab_cum[0] > ((in_run == 1) ? 8 * 1024 : threshold[group])) 
			{
				freq_tab[0] >>= 1;
				freq_tab[1] >>= 1;
				freq_tab[2] >>= 1;
			}

			freq_tab[ch] += MULT;
		}
	}

	DONE_ENCODING();
	DONE_OUTPUTING_BITS();

	return (buf - block.buffer);
}

#endif /* !SFX */

/*---------------------------------------------*/

/* Uncompress a block with an order-0 Structured Model */
void Unzip_SM0(uint32 len, 
			   uint8  *bufout)
{
	bool   in_run = true;
	uint8  *buf = NULL, *length_buf = NULL;
	uint32 ch1 = 0, ch2 = 0, ch3 = 0, i = 0;
	uint32 buffer = 0, low, high, value, bits_to_go;
	
	buf = block.buffer;
	length_buf = block.buffer_length;

	Start_Model();
	START_INPUTING_BITS();
	START_DECODING();

	while (i < len)
    {
		uint32 group, *freq_tab, symbol, ch, order;

		group = groups[ch3][ch2][ch1];
		freq_tab = freq_tab_SM0[group];

		order = Get_Cum_Decode(tab_cum, freq_tab[0], freq_tab[1], freq_tab[2]);

		DECODE_SYMBOL_SM0(symbol, tab_cum);
		ch = (order >> ((symbol - 1) * 2)) & 3;

		ch3 = ch2;
		ch2 = ch1;
		ch1 = ch;

		bufout[i++] = ch;

		if ((ch | in_run | group) == 0)
		{
			in_run = true;
			freq_tab[0] >>= 1;
			freq_tab[0] += MULT + 1;
			freq_tab[1] >>= 1;
			freq_tab[2] >>= 1;
		}
		else
		{
			in_run = (ch != 0) ? false : in_run;

			if (tab_cum[0] > ((in_run == 1) ? 8 * 1024 : threshold[group])) 
			{
				freq_tab[0] >>= 1;
				freq_tab[1] >>= 1;
				freq_tab[2] >>= 1;
			}

			freq_tab[ch] += MULT;
		}
	}
}

/*---------------------------------------------*/
/* end                         struct_model0.c */
/*---------------------------------------------*/
