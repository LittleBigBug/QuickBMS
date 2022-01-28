// modified by Luigi Auriemma
/*
	Copyright (C) 2005  Brendan G Bohannon (aka: cr88192)
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
	
	Email: cr88192@hotmail.com


	Rough description:
	A bytewise LZ77 based compression format (LZ2).

	Aims to be generally simple and offer fast decompression, with ratios
	that hopefully don't suck.

	The first 64 bytes of the compressed data represent values with low
	probabilities in the input data, conceptually mapping to the range
	0xC0-0xFF (this is what these values are likely to be in plaintext
	given the encoder).
	Values in this table are to be conceptually remapped (all other values
	map "somewhere else", but where is not given by the table).

	This range gives values that are special and treated like escapes, all
	others are sent to output unchanged.

	Offsets are the count of the bytes backwards since the current end of
	the window. The offset/length pair may overlap the end of the window
	in which case, what can be copied is, and what remains is taken from
	the just written values, and so forth.

	Eg:
	ABABABAB

	May be treated as:
	AB[-2, 6]

	Escape value mappings:
	0xC0: <byte length> <byte offset>
	0xC1..0xDE: (length=v-0xC0) <byte offset>
	0xE0: <byte length> <byte offset_low> <byte offset_high>
	0xE1..FE: (length=v-0xE0) <byte offset_low> <byte offset_high>

	0xDF: special value, repeat last run
	0xFF: <value>, encode literal value in conflict with an escape code
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <sys/time.h>

//typedef unsigned char byte;

int PDRLELZ_DecompLZ2(byte *in, byte *out, int sz)
{
	byte num[256];

	int i, k=0, l, ll=0;
	byte *cs, *ct, *ce;

	if(!sz)return(0);

	cs=in;
	ce=in+sz;
	ct=out;

	for(i=0; i<256; i++)
		num[i]=0;

	for(i=0; i<64; i++)num[*cs++]=i+0xC0;

	while(cs<ce)
	{
		i=num[*cs];
		if(i)
		{
			if(i==0xFF)
			{
				cs++;
				*ct++=*cs++;
				continue;
			}else if(i>0xE0)
			{
				l=i-0xE0;
				k=cs[1]+(cs[2]<<8);
				cs+=3;
			}else if(i==0xE0)
			{
				l=cs[1];
				k=cs[2]+(cs[3]<<8);
				cs+=4;
			}else if(i==0xDF)
			{
				l=ll;
				cs++;
			}else if(i>0xC0)
			{
				l=i-0xC0;
				k=cs[1];
				cs+=2;
			}else
			{
				l=cs[1];
				k=cs[2];
				cs+=3;
			}

			ll=l;
			while(k<l)
			{
				memcpy(ct, ct-k, k);
				ct+=k;
				l-=k;
			}

			memcpy(ct, ct-k, l);
			ct+=l;

			continue;
		}

		*ct++=*cs++;
		continue;

	}

	return(ct-out);
}

