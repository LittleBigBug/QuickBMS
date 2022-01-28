/*---------------------------------------------*/
/* Zzip/Zzlib compressor           ac-common.h */
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

#ifndef AC_COMMON_H
#define AC_COMMON_H

#define CODE_VALUE_BITS     16
#define TOP_VALUE           ((1UL << CODE_VALUE_BITS) - 1)
#define FIRST_QTR           ((TOP_VALUE >> 2) + 1)
#define HALF                ((TOP_VALUE >> 1) + 1)
#define THIRD_QTR           (((3*TOP_VALUE) >> 2) + 1)

#define MAX_FREQ_MODEL      16384
#define MAX_FREQUENCY2      128 /* 16384 */
#define MAX_FREQUENCY1      256

#define NO_OF_SYMBOLS_0     3
#define NO_OF_GROUPS_0      16

#define NO_OF_SYMBOLS_1     (256 - 3)
#define NO_LEVEL_1          (7 + 1)

/*
 * ARI function
 */
#define OUTPUT_1()							\
{   buffer >>= 1;							\
    buffer |= 0x80;							\
	if (--bits_to_go == 0)					\
    {   *buf++ = buffer;					\
        bits_to_go = 8; }					\
}                                   

#define OUTPUT_0()							\
{   buffer >>= 1;							\
	if (--bits_to_go == 0)					\
    {   *buf++ = buffer;					\
        bits_to_go = 8; }					\
}                                   

#define BIT_PLUS_FOLLOW_1()					\
{											\
	OUTPUT_1()								\
	for (;bits_to_follow>0;--bits_to_follow)\
	  OUTPUT_0()							\
}

#define BIT_PLUS_FOLLOW_0()					\
{											\
	OUTPUT_0()								\
	for (;bits_to_follow>0;--bits_to_follow)\
	  OUTPUT_1()							\
}

#define INPUT_BIT(V)						\
{   if (bits_to_go == 0)					\
	{ buffer = *buf;						\
	buffer &= (buf >= length_buf) - 1;		\
	buf++;									\
    bits_to_go = 8; }						\
    bits_to_go--;							\
	V(buffer & 1);							\
	buffer >>= 1;							\
}

/*
 * ARI function
 */
#define ENCODE_SYMBOL_SM0(symbol, cumfreq, cumfreq0)					\
{	{uint32 range = high - low + 1;										\
	high = low + ((cumfreq[symbol - 1] * range) / cumfreq0) - 1;		\
	low += (cumfreq[symbol] * range) / cumfreq0; }						\
	for (;;)															\
    {	if (high < HALF)												\
			BIT_PLUS_FOLLOW_0()											\
		else if (low >= HALF)											\
{ BIT_PLUS_FOLLOW_1()  low -= HALF; high -= HALF; }						\
			 else if ((low >= FIRST_QTR) & (high < THIRD_QTR))			\
{ bits_to_follow++; low -= FIRST_QTR; high -= FIRST_QTR; }				\
				else break;												\
				low <<= 1;												\
				high <<= 1;												\
				high++; } } 

#define ENCODE_SYMBOL_SM1(symbol, t, cumfreq0)							\
{	{uint32 range = high - low + 1;										\
	high = low + ((t[symbol - 1].cum * range) / cumfreq0) - 1;			\
	low += (t[symbol].cum * range) / cumfreq0; }						\
	for (;;)															\
    {	if (high < HALF)												\
			BIT_PLUS_FOLLOW_0()											\
		else if (low >= HALF)											\
{ BIT_PLUS_FOLLOW_1()  low -= HALF; high -= HALF; }						\
			 else if ((low >= FIRST_QTR) & (high < THIRD_QTR))			\
{ bits_to_follow++; low -= FIRST_QTR; high -= FIRST_QTR; }				\
				else break;												\
				low <<= 1;												\
				high <<= 1;												\
				high++; } } 

/*
 * ARI function
 */
#define DECODE_SYMBOL_SM0(retour, cumfreq)								\
{	{ uint32 range = high - low + 1;									\
	uint32 cum = ((value - low + 1) * cumfreq[0] - 1) / range;			\
	retour = 0; while (cumfreq[++retour] > cum);						\
	high = low + ((cumfreq[retour - 1] * range) / cumfreq[0]) - 1;		\
	low += (cumfreq[retour] * range) / cumfreq[0]; }					\
	for (;;)															\
    {	if (high >= HALF)												\
		{ if (low >= HALF)												\
		  { value -= HALF; low -= HALF; high -= HALF; }					\
		      else if ((low >= FIRST_QTR) & (high < THIRD_QTR))			\
			  {value -= FIRST_QTR; low -= FIRST_QTR; high -= FIRST_QTR;}\
			    else break; }											\
		low <<= 1; high <<= 1; high++; value <<= 1;						\
		INPUT_BIT(value +=); } }

#define DECODE_SYMBOL_SM1(retour, t)									\
{	{ uint32 range = high - low + 1;									\
	uint32 cum = ((value - low + 1) * t[0].cum - 1) / range;			\
	retour = 0; while (t[++retour].cum > cum);							\
	high = low + ((t[retour - 1].cum * range) / t[0].cum) - 1;			\
	low += (t[retour].cum * range) / t[0].cum; }						\
	for (;;)															\
    {	if (high >= HALF)												\
		{ if (low >= HALF)												\
		  { value -= HALF; low -= HALF; high -= HALF; }					\
		      else if ((low >= FIRST_QTR) & (high < THIRD_QTR))			\
			  {value -= FIRST_QTR; low -= FIRST_QTR; high -= FIRST_QTR;}\
			    else break; }											\
		low <<= 1; high <<= 1; high++; value <<= 1;						\
		INPUT_BIT(value +=); } }


/*
 * ARI function
 */
#define START_OUTPUTING_BITS() { buffer = 0; bits_to_go = 8; }

#define DONE_OUTPUTING_BITS() {	*buf++ = buffer >> bits_to_go; }

#define START_ENCODING() { low = 0; high = TOP_VALUE; bits_to_follow = 0; }

#define DONE_ENCODING() { bits_to_follow++; if (low < FIRST_QTR) BIT_PLUS_FOLLOW_0() else BIT_PLUS_FOLLOW_1() }

#define START_INPUTING_BITS() { bits_to_go = 0; }

#define START_DECODING()						\
{   uint ii;	value = 0;						\
	for (ii = 1; ii <= CODE_VALUE_BITS; ++ii)	\
       INPUT_BIT(value = 2 * value +);			\
	low = 0; high = TOP_VALUE; }

#endif /* !AC_COMMON_H */

/*---------------------------------------------*/
/* end                             ac-common.h */
/*---------------------------------------------*/
