// modified by Luigi Auriemma
/*
Written 2005 By Brendan G Bohannon

I release this under the public domain. 
However, I do requst mention in any derived source if applicable.

This is intended, where possible, to be used as an example of an encoder.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//#define WHUFF_NOLONGLONG	//'long long' type not present
//#define WHUFF_BITIO		//a few extra functions, may cost speed

#define WHUFF_LOWER	0x00000000
#define WHUFF_UPPER	0xFFFFFFFF
#define WHUFF_NEUTRAL	0x8000
#define WHUFF_WBITS	16
#define WHUFF_WMASK	((1<<WHUFF_WBITS)-1)

#define WARITH2_CTXBITS		16
#define WARITH2_BITS		8

//#define INLINE	inline
//#define INLINE

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ull;


//globals

static unsigned short *warith2_divtab = NULL;	//division table
static unsigned int *warith2_divtab2 = NULL;	//division table (32 bit)
static int warith2_ctx;
static unsigned char *warith2_model = NULL;	//pairs of counts

static int warith2_ctxbits=WARITH2_CTXBITS;
static int warith2_ctxmask=(1<<WARITH2_CTXBITS)-1;

static byte *huff_winbuf;	//window buffer
//int huff_winofs;	//bit offset in byte
//int huff_outbits;	//dummy output bits

static unsigned int whuff_rmin;		//window lower range
static unsigned int whuff_rmax;		//window upper range
static unsigned int whuff_rval;		//window decode value


//bit io

int HUFF_InputByte()
{
	return(*huff_winbuf++);
}


//arithmetic coder core

int WHUFF_InputModelBit(unsigned char *model, int ctx)
{
	unsigned int r, v, i, w;

//	w=warith2_divtab2[((unsigned short *)model)[ctx]];
	r=whuff_rmax-whuff_rmin;
//	v=whuff_rmin+(((ull)r*(ull)w)>>32);

#ifdef WHUFF_NOLONGLONG
	w=warith2_divtab[((unsigned short *)model)[ctx]];
	v=whuff_rmin+(r>>WHUFF_WBITS)*w;
	v+=((r&WHUFF_WMASK)*w)>>WHUFF_WBITS;
#else
	w=warith2_divtab2[((unsigned short *)model)[ctx]];
	v=whuff_rmin+(((ull)r*(ull)w)>>32);
#endif

	i=(whuff_rval>v);
	if(i)
	{
		whuff_rmin=v+1;
		model[(ctx<<1)|1]++;

		if(!model[(ctx<<1)|1])
		{
			model[(ctx<<1)|0]>>=1;
			model[(ctx<<1)|1]=0x80;
		}
	}else
	{
		whuff_rmax=v;
		model[ctx<<1]++;

		if(!model[ctx<<1])
		{
			model[(ctx<<1)|0]=0x80;
			model[(ctx<<1)|1]>>=1;
		}
	}

	//while bits
	while(!((whuff_rmin^whuff_rmax)&0xFF000000))
	{
		whuff_rmin<<=8;
		whuff_rmax<<=8;
		whuff_rmax|=255;

		whuff_rval<<=8;
		whuff_rval|=HUFF_InputByte();
	}

	return(i);
}

void WHUFF_SetupDecode(byte *in)
{
	int i;

	//huff_outbits=0;
	huff_winbuf=in;
	//huff_winofs=0;

	whuff_rmin=WHUFF_LOWER;
	whuff_rmax=WHUFF_UPPER;

	whuff_rval=WHUFF_LOWER;
	for(i=0; i<4; i++)
		whuff_rval=(whuff_rval<<8)|HUFF_InputByte();
}


//arithmatic coder model/init

int WArith2_DecodeSymbol()
{
	int i, j;

	i=WARITH2_BITS;
	while(i--)
	{
		j=WHUFF_InputModelBit(warith2_model, warith2_ctx);
		warith2_ctx=((warith2_ctx<<1)|j)&warith2_ctxmask;
	}

	return(warith2_ctx&((1<<WARITH2_BITS)-1));
}

void WArith2_SetupDecode(byte *in)
{
	int i, j, k;

    if(!warith2_divtab)  warith2_divtab  = calloc(65536, sizeof(short));
    if(!warith2_divtab2) warith2_divtab2 = calloc(65536, sizeof(int));

	for(i=0; i<256; i++)
		for(j=0; j<256; j++)
	{
		k=((i+1)<<16)/(i+j+2);
		warith2_divtab[i+(j<<8)]=k;
		warith2_divtab2[i+(j<<8)]=k<<16;
	}

	j=1<<warith2_ctxbits;
	if(!warith2_model) warith2_model=malloc(j*2*sizeof(unsigned char));
	for(i=0; i<j*2; i++)warith2_model[i]=0;
	warith2_ctx=0;

	WHUFF_SetupDecode(in);
}

int BPAQ0_DecodeData(byte *in, byte *out)
{
	byte *cs, *ce;
	int j, sz/*, crc, crc2*/;

	if((in[0]!='B') || (in[1]!='P') || (in[2]!='A') || (in[3]!='Q'))
		return(-1);

	if(in[4]!=0)return(-2);
	if(in[6]!=8)return(-3);
	warith2_ctxbits=in[7];

	sz=in[8]+(in[9]<<8)+(in[10]<<16)+(in[11]<<24);
	//crc=in[12]+(in[13]<<8)+(in[14]<<16)+(in[15]<<24);

	WArith2_SetupDecode(in+in[5]);


	cs=out;
	ce=out+sz;
	while(cs<ce)
	{
		j=WArith2_DecodeSymbol();
		*cs++=j;
	}

	//crc2=PDRLELZ_DataAdler32(out, sz, 1);
	//if(crc!=crc2)return(-4);
	return(sz);
}
