// modified by Luigi Auriemma for memory2memory decompression and removing encoding
/**************************************************************
	LZARI.C -- A Data Compression Program
	(tab = 4 spaces)
***************************************************************
	4/7/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN		SCIENCE
		NIFTY-Serve	PAF01022
		CompuServe	74050,1022
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/********** Bit I/O **********/

static
unsigned char   *infile   = NULL,
                *infilel  = NULL,
                *outfile  = NULL,
                *outfilel = NULL;
static int xgetc(void *skip) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
static int xputc(int chr, void *skip) {
    if(outfile >= outfilel) return(-1);
    *outfile++ = chr;
    return(chr);
}
static
unsigned long int  textsize = 0;

static
int GetBit(void)  /* Get one bit (0 or 1) */
{
	static unsigned int  buffer, mask = 0;
	
	if ((mask >>= 1) == 0) {
		buffer = xgetc(infile);  mask = 128;
	}
	return ((buffer & mask) != 0);
}

/********** LZSS with multiple binary trees **********/

#define N		 4096	/* size of ring buffer */
#define F		   60	/* upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length
						   if match_length is greater than this */
#define NIL			N	/* index for root of binary search trees */

unsigned char  text_buf[N + F - 1];	/* ring buffer of size N,
			with extra F-1 bytes to facilitate string comparison */
int		match_position, match_length,  /* of longest match.  These are
			set by the InsertNode() procedure. */
		lson[N + 1], rson[N + 257], dad[N + 1];  /* left & right children &
			parents -- These constitute binary search trees. */

/********** Arithmetic Compression **********/

/*  If you are not familiar with arithmetic compression, you should read
		I. E. Witten, R. M. Neal, and J. G. Cleary,
			Communications of the ACM, Vol. 30, pp. 520-540 (1987),
	from which much have been borrowed.  */

#define M   15

/*	Q1 (= 2 to the M) must be sufficiently large, but not so
	large as the unsigned long 4 * Q1 * (Q1 - 1) overflows.  */

#define Q1  (1UL << M)
#define Q2  (2 * Q1)
#define Q3  (3 * Q1)
#define Q4  (4 * Q1)
#define MAX_CUM (Q1 - 1)

#define N_CHAR  (256 - THRESHOLD + F)
	/* character code = 0, 1, ..., N_CHAR - 1 */

unsigned long int  low = 0, high = Q4, value = 0;
int  shifts = 0;  /* counts for magnifying low and high around Q2 */
int  char_to_sym[N_CHAR], sym_to_char[N_CHAR + 1];
unsigned int
	sym_freq[N_CHAR + 1],  /* frequency for symbols */
	sym_cum[N_CHAR + 1],   /* cumulative freq for symbols */
	position_cum[N + 1];   /* cumulative freq for positions */

static
void StartModel(void)  /* Initialize model */
{
	int ch, sym, i;
	
	sym_cum[N_CHAR] = 0;
	for (sym = N_CHAR; sym >= 1; sym--) {
		ch = sym - 1;
		char_to_sym[ch] = sym;  sym_to_char[sym] = ch;
		sym_freq[sym] = 1;
		sym_cum[sym - 1] = sym_cum[sym] + sym_freq[sym];
	}
	sym_freq[0] = 0;  /* sentinel (!= sym_freq[1]) */
	position_cum[N] = 0;
	for (i = N; i >= 1; i--)
		position_cum[i - 1] = position_cum[i] + 10000 / (i + 200);
			/* empirical distribution function (quite tentative) */
			/* Please devise a better mechanism! */
}

static
void UpdateModel(int sym)
{
	int i, c, ch_i, ch_sym;
	
	if (sym_cum[0] >= MAX_CUM) {
		c = 0;
		for (i = N_CHAR; i > 0; i--) {
			sym_cum[i] = c;
			c += (sym_freq[i] = (sym_freq[i] + 1) >> 1);
		}
		sym_cum[0] = c;
	}
	for (i = sym; sym_freq[i] == sym_freq[i - 1]; i--) ;
	if (i < sym) {
		ch_i = sym_to_char[i];    ch_sym = sym_to_char[sym];
		sym_to_char[i] = ch_sym;  sym_to_char[sym] = ch_i;
		char_to_sym[ch_i] = sym;  char_to_sym[ch_sym] = i;
	}
	sym_freq[i]++;
	while (--i >= 0) sym_cum[i]++;
}

static
int BinarySearchSym(unsigned int x)
	/* 1      if x >= sym_cum[1],
	   N_CHAR if sym_cum[N_CHAR] > x,
	   i such that sym_cum[i - 1] > x >= sym_cum[i] otherwise */
{
	int i, j, k;
	
	i = 1;  j = N_CHAR;
	while (i < j) {
		k = (i + j) / 2;
		if (sym_cum[k] > x) i = k + 1;  else j = k;
	}
	return i;
}

static
int BinarySearchPos(unsigned int x)
	/* 0 if x >= position_cum[1],
	   N - 1 if position_cum[N] > x,
	   i such that position_cum[i] > x >= position_cum[i + 1] otherwise */
{
	int i, j, k;
	
	i = 1;  j = N;
	while (i < j) {
		k = (i + j) / 2;
		if (position_cum[k] > x) i = k + 1;  else j = k;
	}
	return i - 1;
}

static
void StartDecode(void)
{
	int i;

	for (i = 0; i < M + 2; i++)
		value = 2 * value + GetBit();
}

static
int DecodeChar(void)
{
	int	 sym, ch;
	unsigned long int  range;
	
	range = high - low;
	sym = BinarySearchSym((unsigned int)
		(((value - low + 1) * sym_cum[0] - 1) / range));
	high = low + (range * sym_cum[sym - 1]) / sym_cum[0];
	low +=       (range * sym_cum[sym    ]) / sym_cum[0];
	for ( ; ; ) {
		if (low >= Q2) {
			value -= Q2;  low -= Q2;  high -= Q2;
		} else if (low >= Q1 && high <= Q3) {
			value -= Q1;  low -= Q1;  high -= Q1;
		} else if (high > Q2) break;
		low += low;  high += high;
		value = 2 * value + GetBit();
	}
	ch = sym_to_char[sym];
	UpdateModel(sym);
	return ch;
}

static
int DecodePosition(void)
{
	int position;
	unsigned long int  range;
	
	range = high - low;
	position = BinarySearchPos((unsigned int)
		(((value - low + 1) * position_cum[0] - 1) / range));
	high = low + (range * position_cum[position    ]) / position_cum[0];
	low +=       (range * position_cum[position + 1]) / position_cum[0];
	for ( ; ; ) {
		if (low >= Q2) {
			value -= Q2;  low -= Q2;  high -= Q2;
		} else if (low >= Q1 && high <= Q3) {
			value -= Q1;  low -= Q1;  high -= Q1;
		} else if (high > Q2) break;
		low += low;  high += high;
		value = 2 * value + GetBit();
	}
	return position;
}

/********** Encode and Decode **********/

int unlzari(unsigned char *in, int insz, unsigned char *out, int outsz) {
	int  i, j, k, r, c;
	unsigned long int  count;

    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

    textsize = (xgetc(infile));
    textsize |= (xgetc(infile) << 8);
    textsize |= (xgetc(infile) << 16);
    textsize |= (xgetc(infile) << 24);
	//if (textsize == 0) return(-1);
    if (textsize == 0) return(0);
    if (textsize < 0) return(-1);

	StartDecode();  StartModel();
	for (i = 0; i < N - F; i++) text_buf[i] = ' ';
	r = N - F;
	for (count = 0; count < textsize; ) {
        if(infile >= infilel) break;
		c = DecodeChar();
		if (c < 256) {
			xputc(c, outfile);  text_buf[r++] = c;
			r &= (N - 1);  count++;
		} else {
			i = (r - DecodePosition() - 1) & (N - 1);
			j = c - 255 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				xputc(c, outfile);  text_buf[r++] = c;
				r &= (N - 1);  count++;
			}
		}
	}
    return(outfile - out);
}
