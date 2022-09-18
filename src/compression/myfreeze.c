// modified by Luigi Auriemma

/*
 * Freeze - data freezing program
 * Version 1.0:
 * This program is made from GNU compress.c and Yoshizaki/Tagawa's
 * lzhuf.c. (Thanks to all of them.)
 * The algorithm is modified for using in pipe
 * (added ENDOF symbol in Huffman table).
 * Version 1.1:
 * Check for lack of bytes in frozen file when melting.
 * Put the GetBit routine into DecodeChar for reduce function-
 * call overhead when melting.
 * Version 1.2:
 * Added delayed coding a la COMIC.
 * Now freeze works on Intels (*NIX, Microsoft, Turbo),
 * Sun (SunOS).
 * Version 2.0:
 * Buffer size is now 8192 bytes, maximum match length - 256 bytes.
 * Improved hash function (with tuning of hash-table)
 * Version 2.1: Noticeable speedup: Insert_Node and Get_Next_Match
 * are now separated. (Boyer-Moore string matching)
 * Version 2.2: Tunable static Huffman table for position information,
 * this info may be given in the command string now.
 * Version 2.2.3: Bug fixes, 10% freezing speedup.
 * Version 2.3: Minor bug fixes (DOS filenames handling, backward
 * compatibility feature improved, "bits" compression ratio display,
 * preventive check for special files), speedups, more comments added.
 * Version 2.3.1: Typedefs cleaned, utime bug on the m88k corrected
 * (pa@verano.sba.ca.us, clewis@ferret.ocunix.on.ca (Chris Lewis)),
 * "chain threshold" heuristic used for speedup (in "greedy" mode) -
 * a la ZIP (Jean-Loup Gailly). Max. hash bits reduced to 16.
 * Version 2.3.2: Adaptation to TOS 1.04 (fifi@hiss.han.de), UTIMES
 * handling (jik@athena.mit.edu).
 * Version 2.3.3: More accurate adaptation for XENIX 286.
 * Version 2.3.4: Minor bug fixes, HP-UX (longnames w/o BSD) handling,
 * greedy_threshold added.
 * Version 2.3.5: Noticeable speedup (on RISCs, don't know for sure
 * about CISCs).
 * Version 2.4: Yet another speedup, many general changes...
 * Version 2.4.1 (unofficial): A bug is corrected.
 * Version 2.5: Speedup of melting, more thorough adaptation to
 * 16-bits (and 64-bits?) processors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMPAT  1   // required for melt1
static int greedy = 0;
static unsigned char   *freeze_in  = NULL;
static unsigned char   *freeze_inl = NULL;


#define F1              60
#define N1              4096
#define N_CHAR1         (256 - THRESHOLD + F1 + 1)


//

typedef unsigned short us_t;
typedef unsigned char uc_t;
typedef unsigned long ul_t;

#define LOOKAHEAD       256     /* pre-sence buffer size */
#define MAXDIST         7936
#define WINSIZE         (MAXDIST + LOOKAHEAD)   /* must be a power of 2 */
#define WINMASK         (WINSIZE - 1)

#define THRESHOLD	2

#define N_CHAR2         (256 - THRESHOLD + LOOKAHEAD + 1) /* code: 0 .. N_CHARi - 1 */
#define T2              (N_CHAR2 * 2 - 1)       /* size of table */

#define ENDOF           256                     /* pseudo-literal */



// bitio.c
static unsigned bitbuf = 0;    /* use native word size, it's faster */
static int     bitlen = 0, overrun = 0;
static long    bytes_out;      /* counter of bytes processed by PutBits */


// bitio.h

/* Some definitions for faster bit-level I/O */
/* Assumptions: local variables "loclen" and "locbuf" are defined
 * via "DefBits";
 * AvailBits() is called before all bit input operations, the
 * maximum allowed argument for AvailBits() is bits(bitbuf) -7;
 * FlushBits() is called often enough while bit output operations;
 * KeepBits() is called as opposite to DefBits.
 */

#define bits(x) ((int)sizeof(x)*8)
#define BYSH  (bits(bitbuf)-8)
#define BISH  (bits(bitbuf)-1)

#define InitIO()        { overrun = bitlen = 0; bitbuf = 0; }

#define DefBits         register unsigned locbuf = bitbuf;\
register int loclen = bitlen

#define FillBits()   if (loclen <= bits(bitbuf) - 8) {\
	do {\
		locbuf |= (unsigned)((*freeze_in++) & 0xFF) << (BYSH - loclen);\
		loclen += 8;\
	} while (loclen <= bits(bitbuf) - 8);\
if (freeze_in >= freeze_inl) overrun++;\
}

#define KeepBits()      bitbuf = locbuf, bitlen = loclen

/* GetX() macros may be used only in "var op= GetX();" statements !! */

#define GetBit()  /* var op= */locbuf >> BISH, locbuf <<= 1, loclen--

#define GetByte() /* var op= */locbuf >> BYSH, locbuf <<= 8, loclen -= 8

/* NB! `n' is used more than once here! */
#define GetNBits(n) /* var op= */ locbuf >> (bits(bitbuf) - (n)),\
	locbuf <<= (n), loclen -= (n)


// lz.h

#ifndef SEGMENTED
# define MAXBITS 16
#else
# ifdef INT_16_BITS
#  define MAXBITS 15
# else
#  define MAXBITS 14
# endif
#endif

#ifdef SEGMENTED
# ifdef TINY
#  undef MAXBITS
#  define MAXBITS 13
# endif
#endif

#ifndef BITS
# define BITS    MAXBITS
#endif

#if BITS < 13
# undef BITS
# define BITS    13      /* 1:1 hash */
#endif

#if BITS > MAXBITS
# undef BITS
# define BITS    MAXBITS
#endif

/* The following hash-function isn't optimal but it is very fast:

	HASH =      ((first + (second << LEN0) +
		    (third << LEN1)) & ((1L << BITS) - 1);

  The difference of LENs is no more than one bit.
*/

#define LEN0    ((BITS-8)/2)
#define LEN1    (BITS-8)

#define hash_size      (1L << BITS)

/* Use native size hash unless required otherwise */
#if defined(SMALL) || defined(TINY)
typedef us_t hash_t;
#else
typedef unsigned hash_t;
#endif  /* SMALL || TINY */

/* Some defines to eliminate function-call overhead */

/* Hash function (no more than 16 bits, so we don't need longs */

#define hash(p)\
	((unsigned)(p)[0] + ((unsigned)(p)[1] << LEN0) +\
	((unsigned)(p)[2] << LEN1))

#ifdef FASTHASH
#define hashof(p)\
	(((p)[0] != (p)[1] ? hash(p) : hash(p) + hash((p) + 3)) &\
	((1L << BITS) - 1))
#else
#define hashof(p)\
	(hash(p) & ((1L << BITS) - 1))
#endif

/* Inserting of a node `r' into hashed linked list: `r' becomes
	the head of list.
*/

#define InsertNode()\
{\
	register uc_t  *key = &text_buf[r & WINMASK];\
	register unsigned p = hashof(key);\
	if (r < MAXDIST) /* wraparound occured */\
		r = rehash(r);\
	next[r & WINMASK] = hashtab[p];\
	hashtab[p] = r;\
}

#if defined(__GNUC__)
#if defined(__i386__)
/* Optimizer cannot allocate these registers correctly :( (v1.39) */
#define FIX_SI  asm("si")
#define FIX_DI  asm("di")
#else

/* GNU-style register allocations for other processors are welcome! */

#define FIX_SI
#define FIX_DI
#endif
#else

/* Dummy defines for non-GNU compilers */

#define FIX_SI
#define FIX_DI
#endif

/* some heuristic to avoid necessity of "-ggg..." */
#define CHAIN_THRESHOLD (LOOKAHEAD / 2)

#ifdef ALLOW_MISALIGN
#define FETCH(array,index) *(us_t*)(&array[index]-1)
#else
#define FETCH(array,index) array[index]
#endif


// lz.c

/*----------------------------------------------------------------------*/
/*                                                                      */
/*                          LZSS ENCODING                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static uc_t    text_buf[WINSIZE + LOOKAHEAD - 1];/* cyclic buffer with an overlay */
static int     match_position;                 /* current position of
						matched pattern */
static int     chain_length;                   /* max_chain_length ==
						CHAIN_THRESHOLD >> greedy */

/*      next[N+1..] is used as hash table,
	the rest of next is a link down,
*/

static hash_t hashtab[hash_size];      /* a VERY large array :-) */
static hash_t next[WINSIZE];

#ifdef GATHER_STAT
long node_matches, node_compares, node_prolongations;
#endif  /* GATHER_STAT */

/* Initialize the data structures and allocate memory, if needed.
	Although there is no more trees in the LZ algorithm
	implementation, routine name is kept intact :-)
*/

static void InitTree ()
{

	unsigned i = 0;
#ifdef GATHER_STAT
	node_matches = node_compares = node_prolongations = 0;
#endif  /* GATHER_STAT */

	do {
		hashtab[i] = 0;
	} while (i++ != hash_size - 1);

	if (greedy >= 0)
		chain_length = ((CHAIN_THRESHOLD - 1) >> greedy) + 1;
	else
		chain_length = LOOKAHEAD * 2;
}

/* Get the longest (longer than `match_length' when entering in function)
	nearest match of the string beginning in text_buf[r]
	to the cyclic buffer. Result (length & position) is returned
	as the result and in global variable
	`match_position'). Unchanged `match_length' denotes failure and
	`match_position' contains garbage !!
	In order to achieve faster operation, `match_length' is shifted
	down to LOOKAHEAD. Ideas of Andrew Cadach <kadach@isi.itfs.nsk.su>
	have been used (lastbyte).
*/

static int get_next_match (match_length, r)
	register hash_t r; int match_length;
{
	register hash_t p = r & WINMASK;
	register int m;
#ifdef ALLOW_MISALIGN
	register us_t lastbyte;
#else
	register uc_t lastbyte;
#endif
	register uc_t  *key FIX_SI, *pattern FIX_DI;
	int     chain_count = chain_length;

#ifdef GATHER_STAT
	node_matches++;
#endif
	key = text_buf + (r & WINMASK) + LOOKAHEAD;
	r -= MAXDIST;           /* `r' is now a "barrier value" */

	for(;;) {
		lastbyte = FETCH(key, match_length);
		do {
			if(chain_count <= 0)
				/* chain length exceeded, simple return */
				return match_length;

			pattern = text_buf + match_length + LOOKAHEAD;

			do {
				if ((p = next[p]) < r)
					return match_length;
			} while (FETCH(pattern, p &= WINMASK) != lastbyte);

			chain_count--;  /* successful lastbyte match, cost = 1 */
			pattern = text_buf + p + LOOKAHEAD;

#ifdef GATHER_STAT
		node_compares++;
#endif

#ifdef ALLOW_MISALIGN
			for (m = -LOOKAHEAD;
			*(unsigned*)&key[m] == *(unsigned*)&pattern[m] &&
				(m += sizeof(unsigned)) < 0;);
#ifndef INT_16_BITS
		/* Hope that sizeof(int) > 2 ==> sizeof(int) > sizeof(short) */
			if (m < 0 && *(us_t*)&key[m] == *(us_t*)&pattern[m])
				m += sizeof(us_t);
#endif
#ifdef BIGSHORTS
			while
#else
			if
#endif
				(m < 0 && key[m] == pattern[m])
					++m;
#else
			for (m = -LOOKAHEAD; key[m] == pattern[m] && ++m < 0;);
#endif
		} while (m < match_length);

		match_position = p;     /* remember new results */
		if (m == 0)
			return 0;
		match_length = m;

#ifdef GATHER_STAT
		node_prolongations++;
#endif
		chain_count -= 2;       /* yet another match found, cost = 2 */
	}
}

hash_t
rehash(r)
hash_t r;
{
  unsigned i = 0;
  r += WINSIZE;
  do {
    /* process links; zero must remain zero */
    if (next[i] && (next[i] += WINSIZE) > r) {
	    next[i] = 0;
    }
  } while(i++ != WINSIZE - 1);
  i = 0;
  do {
    /* process the hash table itself; zero must remain zero */
    if (hashtab[i] && (hashtab[i] += WINSIZE) > r)
	    hashtab[i] = 0;
  } while (i++ != hash_size - 1);
  return r;
}



// huf.c

#define MAX_FREQ        (us_t)0x8000 /* Tree update timing */

/*----------------------------------------------------------------------*/
/*									*/
/*		HUFFMAN ENCODING					*/
/*									*/
/*----------------------------------------------------------------------*/

/* TABLES OF ENCODE/DECODE for upper 6 bits position information */

/* The contents of `Table' are used for freezing only, so we use
 * it freely when melting.
 */

#ifndef HUFVALUES
#define HUFVALUES 0,0,1,2,6,19,34,0
#endif

static uc_t Table2[9] = { 0, HUFVALUES };

/* d_len is really the number of bits to read to complete literal
 * part of position information.
 */
static uc_t p_len[64];        /* These arrays are built accordingly to values */
static uc_t d_len[256];       /* of `Table' above which are default, from the */
		      /* command line or from the header of frozen file */
static uc_t code[256];

/* use arrays of native word-size elements to improve speed */

static unsigned freq[T2 + 1];          /* frequency table */
static int     son[T2];                /* points to son node (son[i],son[i+1]) */
static int     prnt[T2 + N_CHAR2];     /* points to parent node */

static  int t, r, chars;

/* notes :
   prnt[Tx .. Tx + N_CHARx - 1] used by
   indicates leaf position that corresponding to code.
*/

/* Initializes Huffman tree, bit I/O variables, etc.
   Static array is initialized with `table', dynamic Huffman tree
   has `n_char' leaves.
*/

static void StartHuff (n_char)
	int n_char;
{
	register int i, j;
	t = n_char * 2 - 1;
	r = t - 1;
	chars = n_char;

/* A priori frequences are 1 */

	for (i = 0; i < n_char; i++) {
		freq[i] = 1;
		son[i] = i + t;
		prnt[i + t] = i;
	}
	i = 0; j = n_char;

/* Building the balanced tree */

	while (j <= r) {
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[t] = 0xffff;
	prnt[r] = 0;
	bytes_out = 5;
#if defined(DEBUG) || defined (GATHER_STAT)
	symbols_out = refers_out = 0;
#endif
}

/* Reconstructs tree with `chars' leaves */

static void reconst ()
{
	register int i, j, k;
	register unsigned f;

#ifdef DEBUG
	if (quiet < 0)
	  fprintf(stderr,
	    "Reconstructing Huffman tree: symbols: %ld, references: %ld\n",
	    symbols_out, refers_out);
#endif

/* correct leaf node into of first half,
   and set these freqency to (freq+1)/2
*/
	j = 0;
	for (i = 0; i < t; i++) {
		if (son[i] >= t) {
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}
/* Build tree.  Link sons first */

	for (i = 0, j = chars; j < t; i += 2, j++) {
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for (k = j - 1; f < freq[k]; k--);
		k++;
		{       register unsigned *p, *e;
			for (p = &freq[j], e = &freq[k]; p > e; p--)
				p[0] = p[-1];
			freq[k] = f;
		}
		{       register int *p, *e;
			for (p = &son[j], e = &son[k]; p > e; p--)
				p[0] = p[-1];
			son[k] = i;
		}
	}

/* Link parents */
	for (i = 0; i < t; i++) {
		if ((k = son[i]) >= t) {
			prnt[k] = i;
		} else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


/* Updates given code's frequency, and updates tree */

static void update (c)
	register int c;
{
	register unsigned k, *p;
	register int    i, j, l;

	if (freq[r] == MAX_FREQ) {
		reconst();
	}
	c = prnt[c + t];
	do {
		k = ++freq[c];

		/* swap nodes when become wrong frequency order. */
		if (k > freq[l = c + 1]) {
			for (p = freq+l+1; k > *p++; ) ;
			l = p - freq - 2;
			freq[c] = p[-2];
			p[-2] = k;

			i = son[c];
			prnt[i] = l;
			if (i < t) prnt[i + 1] = l;

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if (j < t) prnt[j + 1] = c;
			son[c] = j;

			c = l;
		}
	} while ((c = prnt[c]) != 0);	/* loop until reach to root */
}

/* Decodes the literal or length info and returns its value.
	Returns ENDOF, if the file is corrupt.
*/

static int DecodeChar ()
{
	register int c = r;
	DefBits;

	if (overrun >= sizeof(bitbuf)) {
		return ENDOF;
	}
	/* As far as MAX_FREQ == 32768, maximum length of a Huffman
	 * code cannot exceed 23 (consider Fibonacci numbers),
	 * so we don't need additional FillBits while decoding
	 * if sizeof(int) == 4.
	 */
	FillBits();
	/* trace from root to leaf,
	   got bit is 0 to small(son[]), 1 to large (son[]+1) son node */

	while ((c = son[c]) < t) {
		c += GetBit();
#ifdef INT_16_BITS
		if (loclen == 0)
			FillBits();
#endif
	}
	KeepBits();
	update(c -= t);
	return c;
}

/* Decodes the position info and returns it */

static int DecodePosition ()
{
	int             i, j;
	DefBits;

	/* Upper 6 bits can be coded by a byte (8 bits) or less,
	 * plus 7 bits literally ...
	 */
	FillBits();
	/* decode upper 6 bits from the table */
	i = GetByte();
	j = (code[i] << 7) | ((i << d_len[i]) & 0x7F);

	/* get lower 7 bits literally */
#ifdef INT_16_BITS
	FillBits();
#endif
	j |= GetNBits(d_len[i]);
	KeepBits();
	return j;
}

#ifdef COMPAT

uc_t Table1[9] = { 0, 0, 0, 1, 3, 8, 12, 24, 16 };

/* Old version of a routine above for handling files made by
	the 1st version of Freeze.
*/

short DecodePOld ()
{
	int             i, j;
	DefBits;

	FillBits();
	i = GetByte();
	j = (code[i] << 6) | ((i << d_len[i]) & 0x3F);
#ifdef INT_16_BITS
	FillBits();
#endif
	j |= GetNBits(d_len[i]);
	KeepBits();
	return j;
}
#endif

/* Initializes static Huffman arrays */

static void init(table) uc_t * table; {
	short i, j, k, num;
	num = 0;

/* There are `table[i]' `i'-bits Huffman codes */

	for(i = 1, j = 0; i <= 8; i++) {
		num += table[i] << (8 - i);
		for(k = table[i]; k; j++, k--)
			p_len[j] = i;
	}
	if (num != 256) {
        return;
		//fprintf(stderr, "Invalid position table\n");
		//exit(1);
	}
	num = j;
	//if (do_melt == 0)

/* Freezing: building the table for encoding */
    /*
		for(i = j = 0;;) {
			code[j] = i;
			i++;
			j++;
			if (j == num) break;
			i <<= p_len[j] - p_len[j-1];
		}
	else*/ {

/* Melting: building the table for decoding */

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				code[k++] = j;

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				d_len[k++] =  p_len[j] - 1
#ifdef COMPAT
				- (table == Table1)
#endif
							;
	}
}

/* Reconstructs `Table' from the header of the frozen file and checks
	its correctness. Returns 0 if OK, EOF otherwise.
*/

static int read_header() {
	int i, j;
	i = (*freeze_in++) & 0xFF;
	i |= ((*freeze_in++) & 0xFF) << 8;
	Table2[1] = i & 1; i >>= 1;
	Table2[2] = i & 3; i >>= 2;
	Table2[3] = i & 7; i >>= 3;
	Table2[4] = i & 0xF; i >>= 4;
	Table2[5] = i & 0x1F; i >>= 5;

	if (i & 1 || (i = (*freeze_in++)) & 0xC0) {
		fprintf(stderr, "Unknown header format.\n");
		return -1;
	}

	Table2[6] = i & 0x3F;

	i = Table2[1] + Table2[2] + Table2[3] + Table2[4] +
	Table2[5] + Table2[6];

	i = 62 - i;     /* free variable length codes for 7 & 8 bits */

	j = 128 * Table2[1] + 64 * Table2[2] + 32 * Table2[3] +
	16 * Table2[4] + 8 * Table2[5] + 4 * Table2[6];

	j = 256 - j;    /* free byte images for these codes */

/*      Equation:
	    Table[7] + Table[8] = i
	2 * Table[7] + Table[8] = j
*/
	j -= i;
	if (j < 0 || i < j) {
		return -1;
	}
	Table2[7] = j;
	Table2[8] = i - j;

#ifdef DEBUG
	fprintf(stderr, "Codes: %d %d %d %d %d %d %d %d\n",
		Table2[1], Table2[2], Table2[3], Table2[4],
		Table2[5], Table2[6], Table2[7], Table2[8]);
#endif
	return 0;
}



static void melt_out(unsigned char **ret_out, int n) {
    int     x;
    unsigned char *o = *ret_out;
    for(x = 0; x < n; x++) *o++ = text_buf[x];
    *ret_out = o;
}



int melt2 (unsigned char *in, int insz, unsigned char *out)
{
    unsigned char *o = out;
    freeze_in  = in;
    freeze_inl = in + insz;

	register short    i, j, k, r, c;

/* Huffman-dependent part */
	if(read_header() == -1)
		return -1;
	StartHuff(N_CHAR2);
	init(Table2);
/* end of Huffman-dependent part */

	InitIO();
	for (i = WINSIZE - LOOKAHEAD; i < WINSIZE; i++)
		text_buf[i] = ' ';
	r = 0;
	for (;; ) {
		c = DecodeChar();

		if (c == ENDOF)
			break;
		if (c < 256) {
			text_buf[r++] = c;
			r &= WINMASK;
			if (r == 0) {
				melt_out(&o, WINSIZE);
			}
		} else {
			i = (r - DecodePosition() - 1) & WINMASK;
			j = c - 256 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & WINMASK];
				text_buf[r++] = c;
				r &= WINMASK;
				if (r == 0) {
					melt_out(&o, WINSIZE);
				}
			}
		}
	}
	if (r) {
		melt_out(&o, r);
	}

    return o - out;
}



int melt1 (unsigned char *in, int insz, unsigned char *out)
{
    unsigned char *o = out;
    freeze_in  = in;
    freeze_inl = in + insz;

	register short    i, j, k, r, c;

	StartHuff(N_CHAR1);
	init(Table1);
	InitIO();
	for (i = N1 - F1; i < N1; i++)
		text_buf[i] = ' ';
	r = 0;
	for (;; ) {
		c =  DecodeChar();

		if (c == ENDOF)
			break;

		if (c < 256) {
			text_buf[r++] = c;
			r &= (N1 - 1);
			if (r == 0) {
				melt_out(&o, N1);
			}
		} else {
			i = (r - DecodePOld() - 1) & (N1 - 1);
			j = c - 256 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & (N1 - 1)];
				text_buf[r++] = c;
				r &= (N1 - 1);
				if (r == 0) {
					melt_out(&o, N1);
				}
			}
		}
	}
	if (r) {
		melt_out(&o, r);
	}

    return o - out;
}


