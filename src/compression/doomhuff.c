// modified by Luigi Auriemma for getting HuffFreq externally
/*
	huffman.c
	huffman encoding/decoding for use in hexenworld networking

	$Id: huffman.c,v 1.27 2009/01/27 10:48:18 sezero Exp $
*/

#include <stdlib.h>
#include <string.h>
#define Hunk_HighAllocName(X, Y)    malloc(X)
void Sys_Error(char *fmt, ...) {
    // do nothing
}
//#include "q_stdinc.h"
//#include "compiler.h"
//#include "huffman.h"
/* h2w engine includes */
#undef	USE_INTEL_ASM
//#include "sys.h"
//#include "printsys.h"
//#include "zone.h"

#define HuffPrintf	Sys_Printf


//
// huffman types and vars
//

typedef struct huffnode_s
{
	struct huffnode_s *zero;
	struct huffnode_s *one;
	float		freq;
	unsigned char	val;
	unsigned char	pad[3];
} huffnode_t;

typedef struct
{
	unsigned int	bits;
	int		len;
} hufftab_t;

static void *HuffMemBase = NULL;
static huffnode_t *HuffTree = NULL;
static hufftab_t HuffLookup[256];

#ifdef _MSC_VER
#pragma warning(disable:4305)
/* double to float truncation */
#endif
static const float HuffFreq[256] =
{
#	include "hufffreq.h"
};


//=============================================================================

//
// huffman debugging
//
#if _DEBUG_HUFFMAN

static int HuffIn = 0;
static int HuffOut= 0;
static int freqs[256];

static void ZeroFreq (void)
{
	memset(freqs, 0, 256 * sizeof(int));
}

static void CalcFreq (const unsigned char *packet, int packetlen)
{
	int		ix;

	for (ix = 0; ix < packetlen; ix++)
	{
		freqs[packet[ix]]++;
	}
}

void PrintFreqs (void)
{
	int		ix;
	float	total = 0;

	for (ix = 0; ix < 256; ix++)
	{
		total += freqs[ix];
	}

	if (total > .01)
	{
		for (ix = 0; ix < 256; ix++)
		{
			HuffPrintf("\t%.8f,\n", ((float)freqs[ix])/total);
		}
	}

	ZeroFreq();
}
#endif	/* _DEBUG_HUFFMAN */


//=============================================================================

//
// huffman functions
//

static void FindTab (huffnode_t *tmp, int len, unsigned int bits)
{
	if (!tmp)
		Sys_Error("no huff node");

	if (tmp->zero)
	{
		if (!tmp->one)
			Sys_Error("no one in node");
		if (len >= 32)
			Sys_Error("compression screwd");
		FindTab (tmp->zero, len+1, bits<<1);
		FindTab (tmp->one, len+1, (bits<<1)|1);
		return;
	}

	HuffLookup[tmp->val].len = len;
	HuffLookup[tmp->val].bits = bits;
	return;
}

static unsigned char const Masks[8] =
{
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80
};

static void PutBit (unsigned char *buf, int pos, int bit)
{
	if (bit)
		buf[pos/8] |= Masks[pos%8];
	else
		buf[pos/8] &=~Masks[pos%8];
}

static int GetBit (const unsigned char *buf, int pos)
{
	if (buf[pos/8] & Masks[pos%8])
		return 1;
	else
		return 0;
}

static void BuildTree (const float *freq)
{
	float	min1, min2;
	int	i, j, minat1, minat2;
	huffnode_t	*work[256];
	huffnode_t	*tmp;

	if(!HuffMemBase) HuffMemBase = Hunk_HighAllocName(512 * sizeof(huffnode_t), "hufftree");
	tmp = (huffnode_t *) HuffMemBase;

	for (i = 0; i < 256; tmp++, i++)
	{
		tmp->val = (unsigned char)i;
		tmp->freq = freq[i];
		tmp->zero = NULL;
		tmp->one = NULL;
		HuffLookup[i].len = 0;
		work[i] = tmp;
	}

	for (i = 0; i < 255; tmp++, i++)
	{
		minat1 = -1;
		minat2 = -1;
		min1 = 1E30;
		min2 = 1E30;

		for (j = 0; j < 256; j++)
		{
			if (!work[j])
				continue;
			if (work[j]->freq < min1)
			{
				minat2 = minat1;
				min2 = min1;
				minat1 = j;
				min1 = work[j]->freq;
			}
			else if (work[j]->freq < min2)
			{
				minat2 = j;
				min2 = work[j]->freq;
			}
		}
		if (minat1 < 0)
			Sys_Error("minat1: %d", minat1);
		if (minat2 < 0)
			Sys_Error("minat2: %d", minat2);
		tmp->zero = work[minat2];
		tmp->one = work[minat1];
		tmp->freq = work[minat2]->freq + work[minat1]->freq;
		tmp->val = 0xff;
		work[minat1] = tmp;
		work[minat2] = NULL;
	}

	HuffTree = --tmp; // last incrementation in the loop above wasn't used
	FindTab (HuffTree, 0, 0);

#if _DEBUG_HUFFMAN
	for (i = 0; i < 256; i++)
	{
		if (!HuffLookup[i].len && HuffLookup[i].len <= 32)
		{
		//	HuffPrintf("%d %d %2X\n", HuffLookup[i].len, HuffLookup[i].bits, i);
			Sys_Error("bad frequency table");
		}
	}
#endif	/* _DEBUG_HUFFMAN */
}

void doom_HuffDecode (const unsigned char *in, unsigned char *out, int inlen, int *outlen, const int maxlen)
{
	int	bits, tbits;
	huffnode_t	*tmp;

	--inlen;
	if (inlen < 0)
	{
		*outlen = 0;
		return;
	}
	if (*in == 0xff)
	{
		if (inlen > maxlen)
			memcpy (out, in+1, maxlen);
		else if (inlen)
			memcpy (out, in+1, inlen);
		*outlen = inlen;
		return;
	}

	tbits = inlen*8 - *in;
	bits = 0;
	*outlen = 0;

	while (bits < tbits)
	{
		tmp = HuffTree;
		do
		{
			if ( GetBit(in+1, bits) )
				tmp = tmp->one;
			else
				tmp = tmp->zero;
			bits++;
		} while (tmp->zero);

		if ( ++(*outlen) > maxlen )
			return;	// out[maxlen - 1] is written already
		*out++ = tmp->val;
	}
}

void doom_HuffEncode (const unsigned char *in, unsigned char *out, int inlen, int *outlen)
{
	int	i, j, bitat;
	unsigned int	t;
#if _DEBUG_HUFFMAN
	unsigned char	*buf;
	int	tlen;
#endif	/* _DEBUG_HUFFMAN */

	bitat = 0;

	for (i = 0; i < inlen; i++)
	{
		t = HuffLookup[in[i]].bits;
		for (j = 0; j < HuffLookup[in[i]].len; j++)
		{
			PutBit (out+1, bitat + HuffLookup[in[i]].len-j-1, t&1);
			t >>= 1;
		}
		bitat += HuffLookup[in[i]].len;
	}

	*outlen = 1 + (bitat + 7)/8;
	*out = 8 * ((*outlen)-1) - bitat;

	if (*outlen >= inlen+1)
	{
		*out = 0xff;
		memcpy (out+1, in, inlen);
		*outlen = inlen+1;
	}

#if _DEBUG_HUFFMAN
	HuffIn += inlen;
	HuffOut += *outlen;
	HuffPrintf("in: %d  out: %d  ratio: %f\n", HuffIn, HuffOut, 1-(float)HuffOut/(float)HuffIn);
	CalcFreq(in, inlen);

	buf = (unsigned char *) Z_Malloc(inlen, Z_MAINZONE);
	HuffDecode (out, buf, *outlen, &tlen, inlen);
	if (tlen != inlen)
		Sys_Error("bogus compression");
	for (i = 0; i < inlen; i++)
	{
		if (in[i] != buf[i])
			Sys_Error("bogus compression");
	}
	Z_Free (buf);
#endif	/* _DEBUG_HUFFMAN */
}

void doom_HuffInit (float *myfreq)
{
#if _DEBUG_HUFFMAN
	ZeroFreq ();
#endif	/* _DEBUG_HUFFMAN */
    if(!myfreq) myfreq = (float *)HuffFreq;
	BuildTree(myfreq);
}

