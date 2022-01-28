// modified by Luigi Auriemma

/* xpkSQSH.c -- an LZ based cruncher; special algorithms for 8 bit sound data;
 * fast decrunching
 * Copyright (C) 1994 John Hendrikx
 * This file is part of the xpk package.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA.
 */

/* Written by John Hendrikx <?>
 * Converted by Bert Jahn <wepl@kagi.com>
 * XPK library version by Vesa Halttunen <vesuri@jormas.com>
 */

int xpkSQSH_bfextu(unsigned char *p,int bo,int bc) {
  int r;

  p += bo / 8;
  r = *(p++);
  r <<= 8;
  r |= *(p++);
  r <<= 8;
  r |= *p;
  r <<= bo % 8;
  r &= 0xffffff;
  r >>= 24 - bc;

  return r;
}

#define xpkSQSH_bfextu1 ((*(src + d0 / 8) >> (7 - (d0 % 8))) & 1)

int xpkSQSH_bfextu3(unsigned char *p,int bo) {
  int r;

  p += bo / 8;
  r = *(p++);
  r <<= 8;
  r |= *p;
  r >>= 13 - (bo % 8);
  r &= 7;

  return r;
}

int xpkSQSH_bfexts(unsigned char *p,int bo,int bc) {
  int r;

  p += bo / 8;
  r = *(p++);
  r <<= 8;
  r |= *(p++);
  r <<= 8;
  r |= *p;
  r <<= (bo % 8) + 8;
  r >>= 32 - bc;

  return r;
}

int xpkSQSH_unsqsh(unsigned char *src, unsigned char *dst) {
  int d0,d1,d2,d3,d4,d5,d6,a2,a5;
  unsigned char *a4,*a6;
  unsigned char a3[] = { 2,3,4,5,6,7,8,0,3,2,4,5,6,7,8,0,4,3,5,2,6,7,8,0,5,4,
	6,2,3,7,8,0,6,5,7,2,3,4,8,0,7,6,8,2,3,4,5,0,8,7,6,2,3,4,5,0 };

    unsigned char *ret_dst = dst;
	a6 = dst;
	a6 += *src++ << 8;
	a6 += *src++;
	d0 = d1 = d2 = d3 = a2 = 0;
 
	d3 = *(src++);
	*(dst++) = d3;

l6c6:	if (d1 >= 8) goto l6dc;
	if (xpkSQSH_bfextu1) goto l75a;
	d0 ++;
	d5 = 0;
	d6 = 8;
	goto l734;

l6dc:	if (xpkSQSH_bfextu1) goto l726;
	d0 ++;
	if (! xpkSQSH_bfextu1) goto l75a;
	d0 ++;
	if (xpkSQSH_bfextu1) goto l6f6;
	d6 = 2;
	goto l708;

l6f6:	d0 ++;
	if (! xpkSQSH_bfextu1) goto l706;
	d6 = xpkSQSH_bfextu3(src,d0);
	d0 += 3;
	goto l70a;

l706:	d6 = 3;
l708:	d0 ++;
l70a:	d6 = *(a3 + (8*a2) + d6 - 17);
	if (d6 != 8) goto l730;
l718:	if (d2 < 20) goto l722;
	d5 = 1;
	goto l732;

l722:	d5 = 0;
	goto l734;

l726:	d0 += 1;
	d6 = 8;
	if (d6 == a2) goto l718;
	d6 = a2;
l730:	d5 = 4;
l732:	d2 += 8;
l734:	d4 = xpkSQSH_bfexts(src,d0,d6);
	d0 += d6;
	d3 -= d4;
	*dst++ = d3;
	d5--;
	if (d5 != -1) goto l734;
	if (d1 == 31) goto l74a;
	d1 += 1;
l74a:	a2 = d6;
l74c:	d6 = d2;
	d6 >>= 3;
	d2 -= d6;
	if (dst < a6) goto l6c6;

	return ret_dst - dst;

l75a:	d0++;
	if (xpkSQSH_bfextu1) goto l766;
	d4 = 2;
	goto l79e;

l766:	d0++;
	if (xpkSQSH_bfextu1) goto l772;
	d4 = 4;
	goto l79e;

l772:	d0++;
	if (xpkSQSH_bfextu1) goto l77e;
	d4 = 6;
	goto l79e;

l77e:	d0++;
	if (xpkSQSH_bfextu1) goto l792;
	d0++;
	d6 = xpkSQSH_bfextu3(src,d0);
	d0 += 3;
	d6 += 8;
	goto l7a8;

l792:	d0++;
	d6 = xpkSQSH_bfextu(src,d0,5);
	d0 += 5;
	d4 = 16;
	goto l7a6;

l79e:	d0++;
	d6 = xpkSQSH_bfextu1;
	d0 ++;
l7a6:	d6 += d4;
l7a8:	if (xpkSQSH_bfextu1) goto l7c4;
	d0 ++;
	if (xpkSQSH_bfextu1) goto l7bc;
	d5 = 8;
	a5 = 0;
	goto l7ca;

l7bc:	d5 = 14;
	a5 = -0x1100;
	goto l7ca;

l7c4:	d5 = 12;
	a5 = -0x100;
l7ca:	d0++;
	d4 = xpkSQSH_bfextu(src,d0,d5);
	d0 += d5;
	d6 -= 3;
	if (d6 < 0) goto l7e0;
	if (d6 == 0) goto l7da;
	d1 -= 1;
l7da:	d1 -= 1;
	if (d1 >= 0) goto l7e0;
	d1 = 0;
l7e0:	d6 += 2;
	a4 = -1 + dst + a5 - d4;
l7ex:	*dst++ = *a4++;
	d6--;
	if (d6 != -1) goto l7ex;
	d3 = *(--a4);
	goto l74c;
    return -1;
}
