/*---------------------------------------------*/
/* Zzip/Zzlib compressor                 bwt.c */
/* BWT sorting functions                       */
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

#define MAX_INSERTSORT 20

#define VECSWAP(lb,ub,n)	\
{	uint16 **lb_v = (lb);	\
	uint16 **ub_v = (ub);	\
	uint32 n_v = (n);		\
	for(; n_v > 0; --n_v)	\
	{	uint16 *t;			\
		    t = *lb_v;		\
		*lb_v = *ub_v;		\
		*ub_v = t;			\
		lb_v++; ub_v--; } }

#define KEY(a) *(*(a)+depth)

/*---------------------------------------------*/

INLINE static
void ShellSort_depth_2(uint16 **lowerb, 
					   uint16 **upperb)
{
	uint16 **i;

	if (upperb - lowerb > 10)
	{
		for (i = lowerb + 4; i <= upperb; ++i) 
		{ 
			uint16 **j, *t;
			t = *i;
			for (j = i - 4; j >= lowerb; j -= 4)
			{
				uint16 *ba = *j + 2;
				uint16 *bb = t + 2;
				while (*bb == *ba) { ba++; bb++; } 
				if (*bb > *ba) break;
				*(j + 4) = *j;
			}       
			*(j + 4) = t; 
		}
	}

	for (i = lowerb + 1; i <= upperb; ++i) 
	{ 
		uint16 **j, *t;
		t = *i;
		for (j = i - 1; j >= lowerb; --j)
		{ 
			uint16 *ba = *j + 2;
			uint16 *bb = t + 2;
			while (*bb == *ba) { ba++; bb++; }
			if (*bb > *ba) break;
			*(j + 1) = *j; 
		}       
		*(j + 1) = t; 
	} 
}

/*---------------------------------------------*/

INLINE static
void ShellSort(uint16 **lowerb, 
			   uint16 **upperb, 
			   uint32 depth)
{
	uint16 **i;

	if (upperb - lowerb > 10)
	{
		for (i = lowerb + 4; i <= upperb; ++i) 
		{ 
			uint16 **j =  i - 4;
			uint16  *t = *i;
			for (; j >= lowerb; j -= 4)
			{ 
				uint16 *ba = *j + depth;
				uint16 *bb = t + depth;
				while (*bb == *ba) { ba++; bb++; }
				if (*bb > *ba) break;
				*(j + 4) = *j;
			}       
			*(j + 4) = t; 
		}
	}

	for (i = lowerb + 1; i <= upperb; ++i) 
	{ 
		uint16 **j =  i - 1;
		uint16  *t = *i;
		for (; j >= lowerb; --j)
		{
			uint16 *ba = *j + depth;
			uint16 *bb = t + depth;
			while (*bb == *ba) { ba++; bb++; }
			if (*bb > *ba) break;
			*(j + 1) = *j;
		}       
		*(j + 1) = t;
	} 
}

/*---------------------------------------------*/

#define MAX_STACK   1024
#define PUSH(a,b,c) { stack[sp].lb=(a);stack[sp].ub=(b);stack[sp].d=(c);sp++; }
#define  POP(a,b,c) { --sp;  a=stack[sp].lb;  b=stack[sp].ub;  c=stack[sp].d; }

typedef struct
{
	uint16 **lb;
	uint16 **ub;
	uint32 d;
} stack_s;

static stack_s stack[MAX_STACK] ALIGN;

/* classic Sedgewick ternary sort, tweaked a bit for speed */
INLINE static
void TernarySort(uint16 **lowerb_t, 
				 uint16 **upperb_t, 
				 uint32 depth_t)
{
	sint32 sp = 1;
	uint32 v, depth;
	uint16 **lb, **ub, **i, **j, **lowerb, **upperb;
	
	lowerb = lowerb_t;
	upperb = upperb_t;
	depth  = depth_t;
	
	while (sp > 0)
    {
		{
			sint32 r = upperb - lowerb + 1;
			
			if (r <= MAX_INSERTSORT)
			{
				if (r >= 2) ShellSort(lowerb, upperb, depth);
				POP(lowerb, upperb, depth);
				continue;
			}
			
			if (r > 64) 
			{ /* median-of-3 median-of-3 */
				uint v1, v2, v3;
				r >>= 3;
				lb = lowerb;
				{
					uint v_1, v_2, v_3;
					v_1  = KEY(lb);    v_2 = KEY(lb+=r); v_3 = KEY(lb+=r);
					v1 = MEDIAN(v_1, v_2, v_3);
				}
				{
					uint v_1, v_2, v_3;
					v_1  = KEY(lb+=r); v_2 = KEY(lb+=r); v_3 = KEY(lb+=r);
					v2 = MEDIAN(v_1, v_2, v_3);
				}
				{
					uint v_1, v_2, v_3;
					v_1  = KEY(lb+=r); v_2 = KEY(lb+=r); v_3 = KEY(upperb);
					v3 = MEDIAN(v_1, v_2, v_3);
				}
				v = MEDIAN(v1, v2, v3);
			}
			else
			{ /* median-of-3 */
				uint v1, v2, v3;
				v1 = KEY(lowerb);
				v2 = KEY(lowerb + (r >> 1));
				v3 = KEY(upperb);
				v  = MEDIAN(v1, v2, v3);
			}
		}

		i = lb = lowerb;
		j = ub = upperb;
		
		for(;;)
		{
			sint32 r;

			while (i <= j && (r = KEY(i) - v) <= 0) 
			{
				if (r == 0)
				{ 
					uint16 *t = *i;
					       *i = *lb;
					  *(lb++) = t;
				}
				i++;
			}
			while (i <= j && (r = KEY(j) - v) >= 0) 
			{
				if (r == 0)
				{ 
					uint16 *t = *j;
					       *j = *ub;
					  *(ub--) = t;
				}
				j--;
			}

			if (i > j) break;

			{
				uint16 *t = *i;
     		       *(i++) = *j;
				   *(j--) = t;
			}
		}
		
		if (ub < lb) { depth += 2; continue; }

		{
			uint32 r;
			r=MIN((uint32)(lb-lowerb), (uint32)(i-lb));
			VECSWAP(lowerb, i-1, r);
		}

		{
			uint32 r;
			r=MIN((uint32)(ub-j), (uint32)(upperb-ub));
			VECSWAP(i, upperb, r);
		}
		
		/* sort the smallest partition    */
		/* to minimize stack requirements */
		{
			sint32 r1, r2, r3;
				
			r1 = (i - lb) - 1;
			r2 = (ub - i);
			r3 = (upperb - lowerb) + (lb - ub) - 1;
			
			if (r1 < r2)
			{	/* r1 < r2 */
				if (r3 <= r1)
				{	/* r3 <= r1 < r2 */
					PUSH(upperb - r2, upperb, depth);
					PUSH(lowerb, lowerb + r1, depth);
					lowerb += r1 + 1; upperb -= r2 + 1; depth += 2;
				} 
				else if (r3 <= r2)
				{	/* r1 < r3 <= r2 */
					PUSH(upperb - r2, upperb, depth);
					PUSH(lowerb + r1 + 1, upperb - r2 - 1, depth + 2);
					upperb = lowerb + r1;
				} 
				else
				{	/* r1 < r2 < r3 */
					PUSH(lowerb + r1 + 1, upperb - r2 - 1, depth + 2);
					PUSH(upperb - r2, upperb, depth);
					upperb = lowerb + r1;
				}
			} 
			else
			{   /* r1 > r2 */
				if (r3 >= r1)
				{	/* r3 >= r1 > r2 */
					PUSH(lowerb + r1 + 1, upperb - r2 - 1, depth + 2);
					PUSH(lowerb, lowerb + r1, depth);
					lowerb = upperb - r2;
				} 
				else if (r3 >= r2)
				{	/* r1 > r3 >= r2 */
					PUSH(lowerb, lowerb + r1, depth);
					PUSH(lowerb + r1 + 1, upperb - r2 - 1, depth + 2);
					lowerb = upperb - r2;
				} 
				else
				{	/* r1 > r2 > r3 */
					PUSH(lowerb, lowerb + r1, depth);
					PUSH(upperb - r2, upperb, depth);
					lowerb += r1 + 1; upperb -= r2 + 1; depth += 2;
				}
			}
		}
	}
}

/*---------------------------------------------*/

/* we use only one table instead of two */
static union
{
	uint16 **start[65536];
	uint32    size[65536];
} m ALIGN;

/* BWT memory need : 6*len + 256ko */
void BWT_Coding(uint32 len, 
				uint32 *first, 
				uint8  *buffer8)
{
	/* some aliases */
	union
	{
		uint16 **pointer;
		uint8  *bufinout;
	} p;
	union
	{
		uint8  *buffer;
		uint16 *buffer16;
	} b;
#ifdef GET_STAT
	uint64 p1, p2;
#endif /* GET_STAT */

	p.bufinout = buffer8;
	b.buffer   = (uint8*)ROUND32(buffer8 + (len + RUN_LENGTH_MAX) * sizeof(uint16*));

	GET_TSC(p1);

	memset(m.size, 0, sizeof(uint32) * 65536);
	{
		uint32 i, val = buffer8[0];
		buffer8[len] = 255;

		for (i = 0; i < len; ++i)
		{
			val = ((val & 0xFF) << 8) | buffer8[i + 1]; /* strip buffer over words */
			m.size[val]++;								/* update bucket sizes */
			b.buffer16[i] = val;						/* store word */
		}
	}
	memset(b.buffer16 + len, 0xFF, sizeof(uint16) * (RUN_LENGTH_MAX + 32));

	/* compute start offset of each bucket */
	{
		sint32 j;
		uint32 l = m.size[0], k;
		m.start[0] = p.pointer;

		for (j = 1; j < 65536; ++j)
		{
			k = m.size[j];
			m.start[j] = m.start[j - 1] + l;
			l = k;
		}
	}

	GET_TSC(p2);
	STAT_ADD_TIME(time_bwt1, p2, p1);
	STAT_ADD_SIZE(kb_bwt, len);

	/* finalize bucket sorting */
	{
		uint16 *bu = b.buffer16, *bulen = b.buffer16 + len;
		for (; bu < bulen; ++bu)
			*(m.start[*bu]++) = bu;
	}

	GET_TSC(p1);
	STAT_ADD_TIME(time_bwt2, p1, p2);

	/* finalize BWT sort by sorting each bucket */
	{
		sint32 j;
		uint16 **deb = p.pointer;
		uint32 l = m.start[0] - p.pointer;
		for (j = 0; j < 65536; ++j) 
		{
			if (l > 1)
			{
				if (l <= (MAX_INSERTSORT+2)) ShellSort_depth_2(deb, deb + l - 1);
				else TernarySort(deb, deb + l - 1, 2);
			}
			deb += l;
			l = m.start[j + 1] - m.start[j]; /* width of next bucket */
		}
	}

	GET_TSC(p2);
	STAT_ADD_TIME(time_bwt3, p2, p1);

	{
		uint32 i = 0;
		uint16 *pp = p.pointer[0];

		while (pp != b.buffer16)
		{
			p.bufinout[i] = *(pp - 1) >> 8;
			pp = p.pointer[++i];
		}

		p.bufinout[i] = *(b.buffer16 + len - 1) >> 8;
		*first = i;
		i++;

		for (; i < len; ++i)
			p.bufinout[i] = *(p.pointer[i] - 1) >> 8;
	}

	GET_TSC(p1);
	STAT_ADD_TIME(time_bwt4, p1, p2);
}

/*---------------------------------------------*/
/* end                                   bwt.c */
/*---------------------------------------------*/
