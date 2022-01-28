// modified by Luigi Auriemma

// http://www.rkeene.org/oss/dact/
/*
 * Copyright (C) 2001, 2002, and 2003  Roy Keene
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *      email: dact@rkeene.org
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>



uint32_t dact_bit_buffer_data=0;
int dact_bit_buffer_location=0;
int dact_bit_buffer_read(int bits) {
        int retval;
        if (bits>dact_bit_buffer_location) { 
		bits=dact_bit_buffer_location;
	}

        retval=(dact_bit_buffer_data>>((sizeof(dact_bit_buffer_data)*8)-bits));
        dact_bit_buffer_data=(dact_bit_buffer_data<<bits);
	dact_bit_buffer_location-=bits;
        return(retval);
}
void dact_bit_buffer_write(unsigned int val, unsigned int bits) {
        while ((val>>bits)!=0) {
		bits++;
        }

        if ((bits+dact_bit_buffer_location)>(sizeof(dact_bit_buffer_data)*8)) {
                return;
        }
        dact_bit_buffer_location+=bits;
        dact_bit_buffer_data+=(val<<((sizeof(dact_bit_buffer_data)*8)-dact_bit_buffer_location));
        return;
}
int dact_bit_buffer_size(void) {
        return(dact_bit_buffer_location);
}
void dact_bit_buffer_purge(void) {
        dact_bit_buffer_location=0;
        dact_bit_buffer_data=0;
        return;
}


int dact_delta_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, char *out_block, int blk_size, int bufsize) {
	int i=0,x=0;
	unsigned char CurrByte;
	unsigned char DeltaByte;
	char CompBit;
	char Neg;
	char Val;

	CurrByte=curr_block[0];
	dact_bit_buffer_purge();
	while (1) {
		if ((dact_bit_buffer_size()<=8) && i==blk_size) break;
		if ((dact_bit_buffer_size()<=8) && i!=blk_size)
			dact_bit_buffer_write((unsigned char) curr_block[++i],8);
		CompBit=dact_bit_buffer_read(1);
		if ((dact_bit_buffer_size()<=8) && i!=blk_size)
			dact_bit_buffer_write((unsigned char) curr_block[++i],8);

		if (CompBit==1) {
			Neg=dact_bit_buffer_read(1);
			DeltaByte=dact_bit_buffer_read(5);
			Val=CurrByte+(((-2*Neg)+1)*DeltaByte);
		} else {
			Val=dact_bit_buffer_read(8);
		}
		out_block[x++]=CurrByte;
		CurrByte=Val;
	}
	return(x);
}



int dact_mzlib2_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, unsigned char *out_block, int blk_size, int bufsize) {
	static  uint16_t lookup_table[65536 /*SQRD_256*/];  // avoid to kill the stack
    uint16_t curr_val, repl_val;
	unsigned long dest_size=(blk_size*2);
	unsigned char *tmp_block;
	unsigned int m=0, hdrsize;
	int retval, i, low_byte=0;

	tmp_block=malloc(dest_size);
	retval=uncompress(tmp_block,&dest_size,curr_block,blk_size);

	if (retval!=Z_OK) return(0);

	hdrsize=((unsigned int) (((tmp_block[0]<<8)|tmp_block[1])))+2;

	if ((hdrsize-2)<0x200) low_byte=1;

	for (i=2;i<hdrsize;i+=2) {
		lookup_table[(i-2)/2]=((tmp_block[i]<<8)|tmp_block[i+1]);
	}

	for (i=hdrsize;i<(dest_size);i+=(2-low_byte)) {
		if (low_byte) {
			curr_val=tmp_block[i];
		} else {
			curr_val=((tmp_block[i]<<8)|tmp_block[i+1]);
		}
		repl_val=lookup_table[curr_val];
		out_block[m++]=repl_val>>8;
		out_block[m++]=repl_val&0xff;
	}

/*
	for (i=0;i<m;i++) printf("%02x ", out_block[i]);
	printf("\n");
*/

	free(tmp_block);
	return(m);
}



int dact_mzlib_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, unsigned char *out_block, int blk_size, int bufsize) {
	unsigned long dest_size=(blk_size*2);
	int retval;
	int i;

	retval=uncompress(out_block,&dest_size,curr_block,blk_size);

	if (retval!=Z_OK) {
		return(0);
	}

	for (i=0;i<dest_size;i++) {
		out_block[i]=( ( (out_block[i]&0xf0)>>4) + ((out_block[i]&0x0f)<<4) );
	}

	return(dest_size);
}



int dact_rle_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, char *out_block, int blk_size, int bufsize) {
	int i,x=0,m;
	unsigned char sentinel=curr_block[0];
	unsigned char currchar;
	unsigned char charcnt;

	for (i=1;i<blk_size;i++) {
		currchar=curr_block[i];
		if (currchar==sentinel) {
			currchar=curr_block[++i];
			charcnt=curr_block[++i];
			if ((x+charcnt)>bufsize) {
				printf("Error in RLE compression!\n");
				return(0);
			}
			for (m=0;m<charcnt;m++) out_block[x++]=currchar;
		} else {
			out_block[x++]=currchar;
		}
	}
	return(x);
}



int dact_snibble_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, char *out_block, int blk_size, int bufsize) {
	const unsigned char lookup_table[8]={0, 0, 1, 0, 0, 0, 2, 3};
	unsigned int freq[4];
	unsigned int x, m=0;
	unsigned int i, cnt=0, f=0, j=0;
	int32_t g=0;

	freq[0]=(curr_block[0]&0xc0)>>6; /* 0 */
	freq[1]=(curr_block[0]&0x30)>>4; /* 2 */
	freq[2]=(curr_block[0]&0x0c)>>2; /* 6 */
	for (i=0;i<4;i++) {
		if (freq[0]!=i && freq[1]!=i && freq[2]!=i) freq[3]=i;
	}
	out_block[0]=0;

	dact_bit_buffer_purge();

	dact_bit_buffer_write(curr_block[g++]&0x03,2);

	while (1) {
		while (((dact_bit_buffer_size()+8)<=16) && g<=blk_size) dact_bit_buffer_write(curr_block[g++],8);
		x=dact_bit_buffer_read(1);
		m=(m<<1)+x;
		cnt++;
		if (x==0 || cnt==3) {
			out_block[f]|=(freq[lookup_table[m]]<<( j )) ;
			j+=2;
/* Obsficated comes to mind ... */
			if (j==8) out_block[++f]=(j=0);
			m=0;
			cnt=0;
		}
		if (dact_bit_buffer_size()==0 || f==bufsize) break;
	}

	return(f);
}



int dact_text_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, char *out_block, int blk_size, int bufsize) {
	unsigned char high_byte, low_byte;
	unsigned int range;
	int i=0,x=0,y;
	
	low_byte=curr_block[x++];
	high_byte=curr_block[x++];
	range=(unsigned int) (high_byte-low_byte);

	if (range==0) {
		memset(out_block, low_byte, bufsize);
		return(bufsize);
	}
        for (y=1;y<=8;y++)
                if ((range>>y)==0) break;

	dact_bit_buffer_purge();

	while (1) {
		if (dact_bit_buffer_size()<y) dact_bit_buffer_write((unsigned char) curr_block[x++],8);
		out_block[i++]=(dact_bit_buffer_read(y)+low_byte);
		if ((x==blk_size) && (dact_bit_buffer_size()<y)) break;
		if (i>=(bufsize)) break;
	}

	return(i);
}



int dact_textrle_decompress(/*unsigned char *prev_block,*/ unsigned char *curr_block, char *out_block, int blk_size, int bufsize) {
	int i,x=0,m;
	unsigned char sentinel=curr_block[0];
	unsigned char currchar;
	unsigned char charcnt;

	for (i=1;i<blk_size;i++) {
		currchar=curr_block[i];
		if (currchar==sentinel) {
			currchar=curr_block[++i];
			charcnt=curr_block[++i];
			if ((x+charcnt)>bufsize) {
				printf("Error in RLE compression!\n");
				return(0);
			}
			for (m=0;m<charcnt;m++) out_block[x++]=currchar;
		} else {
			out_block[x++]=currchar;
		}
	}
	return(x);
}

