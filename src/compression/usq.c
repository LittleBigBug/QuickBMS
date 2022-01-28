// modified by Luigi Auriemma
/* Program to unsqueeze files formed by sq.com
 *
 * Useage:
 *
 *	usq [-count] [-fcount] [file1] [file2] ... [filen]
 *
 * where file1 through filen represent one or more files to be compressed,
 * and the following options may be specified:
 *
 *	-count		Previewing feature: redirects output
 * 			files to standard output with parity stripped
 *			and unprintables except CR, LF, TAB and  FF
 *			converted to periods. Limits each file
 *			to first count lines.
 *			Defaults to console, but see below how
 *			to capture all in one file for further
 *			processing, such as by PIP.
 *			Count defaults to a very high value.
 *			No CRC check is performed when previewing.
 *			Use drive: to cancel this.
 *
 *	-fcount		Same as -count except formfeed
 *			appended to preview of each file.
 *			Example: -f10.
 *
 * If no such items are given on the command line you will be
 * prompted for commands (one at a time). An empty command
 * terminates the program.
 *
 * The unsqueezed file name is recorded in the squeezed file.
 * 
 */
/* CHANGE HISTORY:
 * 1.3	Close inbuff to avoid exceeding maximum number of
 *	open files. Includes rearranging error exits.
 * 1.4	Add -count option to allow quick inspection of files.
 * 1.5  Break up long lines of introductory text
 * 1.5  -count no longer appends formfeed to preview of each file.
 *	-fcount (-f10, -F10) does append formfeed.
 * 1.6  Modified to work correctly under MP/M II (DIO.C change) and
 *      signon message shortened.
 * 2.0	Modified to work with CI-C86 compiler (CP/M-86 and MS-DOS)
 * 2.1  Modified for use in MLINK
 * 2.2  Modified for use with optimizing CI-C86 compiler (MS-DOS)
 * 3.0  Generalized for use under UNIX
 * 3.3  Modified to work with c/70 (#define C70) as per Mike Barker.
 *      Modified to work with ULTRIX (#define ULTRIX) as per Tom Reid.
 *	Fixed non-ASCII name problem, as per Ted Medin.
 */

#define SQMAIN

/*#define	ULTRIX		Comment out for non-ULTRIX systems.*/
/*#define	C70		Comment out for non-c/70 systems.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SQMAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#undef EXTERN   // aluigi //
#define EXTERN  static

/* Definitions and external declarations */

#define RECOGNIZE 0xFF76	/* unlikely pattern */

/* *** Stuff for first translation module *** */

#define DLE 0x90

EXTERN unsigned int crc;	/* error check code */

/* *** Stuff for second translation module *** */

#define SPEOF 256	/* special endfile token */
#define NUMVALS 257	/* 256 data values plus SPEOF*/

#define LARGE 30000

/* Decoding tree */
EXTERN struct {
	int children[2];	/* left, right */
} dnode[NUMVALS - 1];

EXTERN int bpos;	/* last bit position read */
EXTERN int curin;	/* last byte value read */

/* Variables associated with repetition decoding */
EXTERN int repct;	/*Number of times to retirn value*/
EXTERN int value;	/*current byte value or EOF */

#define VERSION "3.3   10/29/86"


#define ERROR   -1
#define inbuff  infile
#define outbuff outfile
static unsigned char    *infile   = NULL;
static unsigned char    *outfile  = NULL;
static unsigned char    *infilel  = NULL;
static unsigned char    *outfilel = NULL;
static int xgetc(void *X) {
    if(infile >= infilel) return(EOF);
    return(*infile++);
}



static int getw16(iob)			/* get 16-bit word from file */
FILE *iob;
{
int temp;

temp = xgetc(iob);		/* get low order byte */
temp |= xgetc(iob) << 8;
if (temp & 0x8000) temp |= (~0) << 15;	/* propogate sign for big ints */
return (temp);

}


static int getx16(iob)			/* get 16-bit (unsigned) word from file */
FILE *iob;
{
int temp;

temp = xgetc(iob);		/* get low order byte */
temp |= xgetc(iob) << 8;
return(temp);
}


/* initialize decoding functions */

static void init_cr()
{
	repct = 0;
}

static void init_huff()
{
	bpos = 99;	/* force initial read */
}


/* Decode file stream into a byte level code with only
 * repetition encoding remaining.
 */

static int
getuhuff(ib)
FILE *ib;
{
	int i;

	/* Follow bit stream in tree to a leaf*/
	i = 0;	/* Start at root of tree */
	do {
		if(++bpos > 7) {
			if((curin = xgetc(ib)) == ERROR)
				return (ERROR);
			bpos = 0;
			/* move a level deeper in tree */
			i = dnode[i].children[1 & curin];
		} else
			i = dnode[i].children[1 & (curin >>= 1)];
	} while(i >= 0);

	/* Decode fake node index to original data value */
	i = -(i + 1);
	/* Decode special endfile token to normal EOF */
	i = (i == SPEOF) ? EOF : i;
	return (i);
}

/* Get bytes with decoding - this decodes repetition,
 * calls getuhuff to decode file stream into byte
 * level code with only repetition encoding.
 *
 * The code is simple passing through of bytes except
 * that DLE is encoded as DLE-zero and other values
 * repeated more than twice are encoded as value-DLE-count.
 */

static int
getcr(ib)
FILE *ib;
{
	int c;

	if(repct > 0) {
		/* Expanding a repeated char */
		--repct;
		return (value);
	} else {
		/* Nothing unusual */
		if((c = getuhuff(ib)) != DLE) {
			/* It's not the special delimiter */
			value = c;
			if(value == EOF)
				repct = LARGE;
			return (value);
		} else {
			/* Special token */
			if((repct = getuhuff(ib)) == 0)
				/* DLE, zero represents DLE */
				return (DLE);
			else {
				/* Begin expanding repetition */
				repct -= 2;	/* 2nd time */
				return (value);
			}
		}
	}
}


int unsqueeze(unsigned char *in, int insz, unsigned char *out, int outsz) {
    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

	int i, c;
	int numnodes;		/* size of decoding tree */
	//unsigned int linect;	/* count of number of lines previewed */
	//char obuf[128];		/* output buffer */
	//int oblen;		/* length of output buffer */

	/* Initialization */
	//linect = 0;
	//crc = 0;
	init_cr();
	init_huff();

	/* Process header */
    // removed header

#ifdef C70
	numnodes = getx16(inbuff);
#else
	numnodes = getw16(inbuff);
#endif

	if(numnodes < 0 || numnodes >= NUMVALS) {
        return(-1);
	}

	/* Initialize for possible empty tree (SPEOF only) */
	dnode[0].children[0] = -(SPEOF + 1);
	dnode[0].children[1] = -(SPEOF + 1);

	/* Get decoding tree from file */
	for(i = 0; i < numnodes; ++i) {
		dnode[i].children[0] = getw16(inbuff);
		dnode[i].children[1] = getw16(inbuff);
	}

		while((c = getcr(inbuff)) != EOF) {
            if(outfile >= outfilel) return(-1);
			*outfile++ = c;
		}

    return(outfile - out);
}

