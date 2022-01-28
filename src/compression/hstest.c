// modified by Luigi Auriemma

//
// HSTEST.C
// this is a little demo & test programm for the kombination
// sliding directory & huffman compression
// the real kompression is done using a sliding directory mechanism pack(),
// which will eventually be farther compressed using a modified huffman
// method which takes some time as well, is currently not very efficient
// implemented, but may be used if space is really important,
// the first method should be chosen if the unpack time is of importance
// unpack(), written in assembler for speed reasons roughly decompresses
// some 25 KByte / Landmark or 1 MB on a 25 MHz 386 maschine
//
// warning : the code is not yet clean for byte ordering, it will work
// only on one type of maschine, the compressed files are probable not
// (yet) portable
//
/*
    program name        :   HSTEST
    program author      :   tom ehlert
    post address        :   SIG m.b.H
                            tom ehlert
                            bachstrasse 22
                            D-5100 aachen       / germany
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ONLY_ON_80x86
#define far
#define _loadds
#ifndef _fastcall
    #define _fastcall
#endif
#define STATIC

#define MIN_LENGTH		3		/* minimum to be compressed */
#define MAXCODELENGTH 0x0f
#define SENDLEN  	  0xfff		/* MUST BE 00001111111 */

typedef unsigned char uchar;

void *ck_alloc(unsigned int len)
{
	void *calloc(),*s;
	if ((s = calloc(len,1)) == NULL)
		//panic("can't allocate memory");
        return(NULL);
    return(s);
}

typedef struct huff_tab {
	int count;
	int  bitcnt;
	struct huff_tab *lpt,*rpt;
	} huff_tab;

typedef struct huff_code {
		unsigned bitcode;
		uchar	 bitcnt;
		} huff_code;

typedef struct huff_icode {
		uchar bitcode;
		uchar bitcnt;
		} huff_icode;


STATIC void _fastcall addbit(huff_tab *cpt);
STATIC void _fastcall bit_write(register unsigned value,register int length);
STATIC unsigned short _fastcall bit_read(register int length);
STATIC unsigned short _fastcall huff_read(huff_icode *icode,int maxbit);
/**
*** generate Huffman codes for every character 
**/

//**********************************************************************
// generate from huff_tab a field bitlength which holds the best bitlength
// for each character. allthough it would be possible to generate codes
// at the same time this has been avoided to make it easier usable when
// the data are read back
//**********************************************************************


STATIC void gen_code(huff_code *hufftab,uchar *bitlength,int charset)
{
	register int loop,bitmask,first,lastcode,length;
	for (loop = 0; loop < charset;loop++)
		{
		hufftab[loop].bitcnt  = bitlength[loop];
		hufftab[loop].bitcode = 0xffff;
		}

	lastcode = 0;
	bitmask  = 1;
	first = 1;

	for (length = 16; length >= 1; length--,bitmask <<= 1)
		for (loop = 0; loop < charset;loop++)
			{
			if (hufftab[loop].bitcnt  == length)
				{
				if (!first)
					{
					lastcode += bitmask;
					}
				first = 0;
				hufftab[loop].bitcode = lastcode & ~(0xffff >> length);
				}
			}
}
	



STATIC void gen_invtab(huff_icode *inv,huff_code *tab,int charset,int max_bit)
{
	register int loop,ival,ianz,bitcnt;

	for (loop = 0; loop < charset; loop++)
		{
		ival = tab[loop].bitcode >> (16-max_bit);
		if ((bitcnt = tab[loop].bitcnt) == 0)
			continue;
		ianz = 1 << (max_bit-tab[loop].bitcnt);
		for ( ; --ianz >= 0;ival++)
			{
			inv[ival].bitcode = loop;
			inv[ival].bitcnt  = bitcnt;
			}
		}
}

#define HIGH_BIT 16
unsigned short bit_value = 0;
unsigned short far *bit_ptr = NULL;
unsigned int   bit_drin= 0;

#define dbrange(x) 0			// ((x) >= 0x1b0 && (x) <= 0x1d0)

STATIC void bit_flush(void)
{
	if (bit_drin)
		*bit_ptr++ = bit_value;
}

STATIC void bit_init(void far *ptr)
{
	bit_ptr  = ptr;
	bit_drin = 0;
	bit_value = 0;
}


STATIC void _fastcall bit_write(register unsigned value,register int length)		
{
	int fits = HIGH_BIT - bit_drin;

	if (length <= fits)
		{
		*bit_ptr |= value >> bit_drin;
		if (length == fits)
			{
			*bit_ptr++ = bit_value | (value >> bit_drin);
			bit_drin  = 0;
			bit_value = 0;
			}
		else 
			{
			bit_value |= value >> bit_drin;
			bit_drin+=length;
			}
		}
	else
		{
		*bit_ptr++ = bit_value | (value >> (HIGH_BIT-fits));
		bit_value  = value <<= fits;
		bit_drin = length-fits;
		}
}
STATIC unsigned short _fastcall bit_read(register int maxbit)
{
	register unsigned value;

	if (maxbit <= HIGH_BIT-bit_drin)
		{
		value = (*bit_ptr << bit_drin) >> (HIGH_BIT-maxbit);
		}
	else {
		value = ((bit_ptr[0] << bit_drin) | (bit_ptr[1] >> (HIGH_BIT-bit_drin))) 
				>> (HIGH_BIT-maxbit);
		}		

	if ((bit_drin += maxbit) >= HIGH_BIT)
		bit_drin -= HIGH_BIT,bit_ptr++;

	return value;
}	

STATIC unsigned short _fastcall huff_read(huff_icode *icode,int maxbit)
{
	//register int loop,value;unsigned int lmask,*lptr;
    register int value;

	if (maxbit <= HIGH_BIT-bit_drin)
		{
		value = (*bit_ptr << bit_drin) >> (HIGH_BIT-maxbit);
		}
	else {
		value = ((bit_ptr[0] << bit_drin) | (bit_ptr[1] >> (HIGH_BIT-bit_drin))) 
				>> (HIGH_BIT-maxbit);
		}		

	if ((bit_drin += icode[value].bitcnt) >= HIGH_BIT)
		bit_drin -= HIGH_BIT,bit_ptr++;

	return icode[value].bitcode;
}	

//****************************************************************************
// reading / writing characterset bitcounts
//  0) write no characterset at all
//  1) write number of characters (4 Bits) with number of bits (4 Bits) PKZIP
//  2) write for each character number of bits (4 Bits)
//  3) set bitval to 0;
//			while (bitval < bitcnt) write (0x10,2)
//			while (bitval > bitcnt) write (0x11,2)
//			write (0,1)
//  4) if (bitval OK) write (0,1)
//	   else write (1,1),write (number of Bits,4)
//****************************************************************************

unsigned bit_write_method = 0;


int write_cnt0(register uchar *bitlength,int charset,unsigned short test)
{
	return 0;
}

void read_cnt0(register uchar *bitlength,int charset)
{
	int mask,bits;
	for (bits = 0,mask = 1;mask < charset ; mask <<= 1)
		bits++;
	memset(bitlength,bits,charset);
}
//****************************************************************************

int write_cnt1(register uchar *bitlength,int charset,unsigned short test)
{
	register int needed,loop,id;

	for (id = 0,loop=0,needed=0; loop < charset;loop++,bitlength++)
		{
		if (bitlength[0] == bitlength[1] && id < 15 && loop < charset-1)
			id++;
		else
			{
			if (test) bit_write(((bitlength[0] << 4) | id) << 8,8);
			id = 0;
			needed++;
			}
		}
	return needed;
}

void read_cnt1(uchar *bitlength,int charset)
{
	register int id,val;

	do 	{
		id  = bit_read(8);
		val = id >> 4;
		id  &= 0x0f;
		do {
			*bitlength++ = val;
			charset--;
			} while (--id >= 0);
		} while (charset > 0);
}


//****************************************************************************

int write_cnt2(register uchar *bitlength,int charset,unsigned short test)
{
	if (!test)
		return charset/2;

	do {
		bit_write(*bitlength<<(HIGH_BIT-4),4);	// 4 Bit
		bitlength++;
		} while (--charset);
    return(1); //???
}
void read_cnt2(register uchar *bitlength,int charset)
{
	do {
		*bitlength++ = bit_read(4);		// 4 Bit
		} while (--charset);
}

//****************************************************************************

int write_cnt3(register uchar *bitlength,int charset,unsigned short test)
{
	register int needed,loop,bitval;

	for (bitval = 0,loop=0,needed=0; loop < charset;loop++,bitlength++)
		{
		while (bitval > bitlength[0])
			{
			bitval--,needed+=2;
			if (test)
				bit_write(0x01 << (HIGH_BIT-2),2);
			}
		while (bitval < bitlength[0])
			{
			bitval++,needed+=2;
			if (test)
				bit_write(0x00 << (HIGH_BIT-2),2);
			}
		if (test)
			bit_write(0x1 << (HIGH_BIT-1),1);
		needed++;
		}
	return (needed+7)/8;
}
void read_cnt3(register uchar *bitlength,int charset)
{
	register int bitval;

	for (bitval = 0; --charset >= 0;)
		{
		while (bit_read(1) == 0)
			if (bit_read(1))
				bitval--;
			else
				bitval++;
		*bitlength++ = bitval;
		}
}
//****************************************************************************

int write_cnt4(register uchar *bitlength,int charset,unsigned short test)
{
	register int needed,bitval;

	for (bitval = 0,needed=0; --charset >= 0;bitlength++)
		{
		if (bitval != bitlength[0])
			{
			bitval = bitlength[0],needed+=5;
			if (test)
				{
				bit_write(1 << (HIGH_BIT-1),1);
				bit_write( bitlength[0] << (HIGH_BIT-4),4);
				}
			}
		else
			{
			if (test)
				bit_write(0 << (HIGH_BIT-1),1);
			needed++;
			}
		}
	return (needed+7)/8;
}

void read_cnt4(register uchar *bitlength,int charset)
{
	register int bitval;

	for (bitval = 0; --charset >= 0;bitlength++)
		{
		if (bit_read(1))
			bitval = bit_read(4);
		*bitlength = bitval;
		}
}

//****************************************************************************

typedef int (*wrf)(register uchar *,int,unsigned short);
typedef void (*rdf)(register uchar *,int);

wrf write_fun[] = { write_cnt0,write_cnt1,write_cnt2,write_cnt3,write_cnt4};
rdf read_fun[]  = {  read_cnt0, read_cnt1, read_cnt2, read_cnt3, read_cnt4};


#define LITLEN 256		// characterset for literal characters
#define LENLEN 16		// characterset for length 0..15
#define INDLEN (4096 >> INDSHIFT)	// characterset for index = (0..4096) >> 4
#define MASLEN 256		// characterset for packmask
#define EXTLEN 256		// characterset for extended length
#define INDSHIFT 4
#define INDMASK  0x0f

#define LIT_MAX 12		// MAXIMUM needed bits for each charset
#define IND_MAX 11
#define LEN_MAX 7
#define MAS_MAX 11
#define EXT_MAX 11

#define LIT_FLAG 0
#define IND_FLAG 3
#define LEN_FLAG 6
#define MAS_FLAG 9
#define EXT_FLAG 12


unsigned short do_huff    = 0;


huff_icode *hs_rd_code(int charset,int maxbit,unsigned short flatlen,int do_flag)
{
	uchar *bitlength;
	huff_code *code;
	huff_icode *icode;
	//register int loop;

	bitlength = ck_alloc(charset);
    if(!bitlength) return(NULL);

	(*read_fun[(do_huff >> do_flag) & 0x07])(bitlength,charset);

	code = ck_alloc(charset*sizeof(huff_code));
    if(!code) return(NULL);
	gen_code(code,bitlength,charset);
	free(bitlength);

	icode = ck_alloc((1<<maxbit)*sizeof(huff_icode));
    if(!icode) return(NULL);
	gen_invtab(icode,code,charset,maxbit);
	free(code);
	return icode;
}

int hstest_hs_unpack(uchar far *ubuffer,uchar far *hbuffer,unsigned short packlen)
{
	huff_icode *lit_icode,*len_icode,*ind_icode,*mas_icode,*ext_icode;
	register int uloop;
	register uchar packmask=0;
	int length;unsigned index;
	uchar far *sbuffer = ubuffer;

	do_huff = ((short far *)hbuffer)[0];
	packlen = ((short far *)hbuffer)[1];

	bit_init(hbuffer+4);

	lit_icode = hs_rd_code(LITLEN,LIT_MAX,8,LIT_FLAG);
    if(!lit_icode) return(-1);
	ind_icode = hs_rd_code(INDLEN,IND_MAX,8,IND_FLAG);
    if(!ind_icode) return(-1);
	len_icode = hs_rd_code(LENLEN,LEN_MAX,4,LEN_FLAG);
    if(!len_icode) return(-1);
	mas_icode = hs_rd_code(MASLEN,MAS_MAX,8,MAS_FLAG);
    if(!mas_icode) return(-1);
	ext_icode = hs_rd_code(EXTLEN,EXT_MAX,8,EXT_FLAG);
    if(!ext_icode) return(-1);

	for (uloop = 1;packlen > 0;packmask <<= 1)
		{
		if (--uloop == 0)
			{
			packmask    = huff_read(mas_icode,MAS_MAX),packlen--;
			uloop = 8;
			}
		if ((packmask & 0x80) == 0)				/* this byte is literally */
			{
			*ubuffer++ = huff_read(lit_icode,LIT_MAX),packlen--;
			}
		else
			{									/* next 2 bytes icoded     */
			length = huff_read(len_icode,LEN_MAX)+MIN_LENGTH;
			index  = huff_read(ind_icode,IND_MAX) << INDSHIFT;
			index += bit_read(INDSHIFT);

			if (length == MAXCODELENGTH+MIN_LENGTH)	/* length > 18			  */
				{								/* use next byte for length*/
				length  = huff_read(ext_icode,EXT_MAX);		/* icode is 3 bytes		  */
				packlen -= 3;
				}
			else {
				packlen -= 2;
				}
									/* copy BYTEWISE with OVERWRITE */
			if (index == 1)			// memcpy will not work as it copies WORDS
				memset(ubuffer,*(ubuffer-1),length);
			else
				memcpy(ubuffer,ubuffer-index,length);

			ubuffer += length;
			}			
		}

	free(lit_icode);
	free(ind_icode);
	free(len_icode);
	free(mas_icode);
	free(ext_icode);

	return ubuffer-sbuffer;
}



int far _loadds hstest_unpackc(outbuffer,pbuffer,packlen)
uchar far *outbuffer;
uchar far *pbuffer;register int packlen;
{
	register int uloop;
	register uchar packmask=0;
	int length;unsigned index;
	uchar far *sbuffer = outbuffer;

	for (uloop = 1;packlen > 0;packmask <<= 1)
		{
		if (--uloop == 0)
			{
			packmask    = *pbuffer++,packlen--;
			uloop = 8;
			}
		if ((packmask & 0x80) == 0)				/* this byte is literally */
			*outbuffer++ = *pbuffer++,packlen--;
		else
			{									/* next 2 bytes coded     */
			ONLY_ON_80x86;
			index = *(int far *)pbuffer;		/* 4 bit length			  */
			length = (index >> 12) + MIN_LENGTH;/* 12 bit offset		  */
			index &= SENDLEN;
			if (length == MAXCODELENGTH+MIN_LENGTH)	/* length > 18			  */
				{								/* use next byte for length*/
				length  = pbuffer[2];			/* code is 3 bytes		  */
				pbuffer += 3;
				packlen -= 3;
				}
			else {
				packlen -= 2;
				pbuffer += 2;
				}
									/* copy BYTEWISE with OVERWRITE */
			if (index == 1)			// memcpy will not work as it copies WORDS
				memset(outbuffer,*(outbuffer-1),length);
			else
				memcpy(outbuffer,outbuffer-index,length);

			outbuffer += length;
			}			
		}
	return outbuffer-sbuffer;
}


