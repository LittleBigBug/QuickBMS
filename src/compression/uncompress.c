// from the original compress utility ("The Regents of the University of California")
// modified by Luigi Auriemma

#define de_stack_size HSIZE

#ifndef SACREDMEM
#define SACREDMEM	0
#endif

#ifndef USERMEM
# define USERMEM 	450000	/* default user memory */
#endif

#ifdef interdata		/* (Perkin-Elmer) */
#define SIGNED_COMPARE_SLOW	/* signed compare is slower than unsigned */
#endif

#ifdef pdp11
# define BITS 	12	/* max bits/code for 16-bit machine */
# define NO_UCHAR	/* also if "unsigned char" functions as signed char */
# undef USERMEM 
#endif /* pdp11 */	/* don't forget to compile with -i */

#ifdef z8000
# define BITS 	12
# undef vax		/* weird preprocessor */
# undef USERMEM 
#endif /* z8000 */

#ifdef pcxt
# define BITS   12
# undef USERMEM
#endif /* pcxt */

#ifdef USERMEM
# if USERMEM >= (433484+SACREDMEM)
#  define PBITS	16
# else
#  if USERMEM >= (229600+SACREDMEM)
#   define PBITS	15
#  else
#   if USERMEM >= (127536+SACREDMEM)
#    define PBITS	14
#   else
#    if USERMEM >= (73464+SACREDMEM)
#     define PBITS	13
#    else
#     define PBITS	12
#    endif
#   endif
#  endif
# endif
# undef USERMEM
#endif /* USERMEM */

#ifdef PBITS		/* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
# endif /* BITS */
#endif /* PBITS */

#if BITS == 16
# define HSIZE	69001		/* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE	35023		/* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE	18013		/* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE	9001		/* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE	5003		/* 80% occupancy */
#endif

#ifdef M_XENIX			/* Stupid compiler can't handle arrays with */
# if BITS == 16			/* more than 65535 bytes - so we fake it */
#  define XENIX_16
# else
#  if BITS > 13			/* Code only handles BITS = 12, 13, or 16 */
#   define BITS	13
#  endif
# endif
#endif

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
#if BITS > 15
typedef long int	code_int;
#else
typedef int		code_int;
#endif

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else
typedef long int	  count_int;
#endif

#ifdef NO_UCHAR
 typedef char	char_type;
#else
 typedef	unsigned char	char_type;
#endif /* UCHAR */
static char_type magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/
#define INIT_BITS 9			/* initial number of bits/code */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef notdef
#include <sys/ioctl.h>
#endif

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static int n_bits;				/* number of bits/code */
static int maxbits = BITS;			/* user settable max # bits/code */
static code_int maxcode;			/* maximum code, given n_bits */
static code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
#ifdef COMPATIBLE		/* But wrong! */
# define MAXCODE(n_bits)	(1 << (n_bits) - 1)
#else
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)
#endif /* COMPATIBLE */

#ifdef XENIX_16
#undef de_stack_size
#define de_stack_size 8192
static count_int htab0[8192];
static count_int htab1[8192];
static count_int htab2[8192];
static count_int htab3[8192];
static count_int htab4[8192];
static count_int htab5[8192];
static count_int htab6[8192];
static count_int htab7[8192];
static count_int htab8[HSIZE-65536];
static count_int * htab[9] = {
	htab0, htab1, htab2, htab3, htab4, htab5, htab6, htab7, htab8 };

#define htabof(i)	(htab[(i) >> 13][(i) & 0x1fff])
static unsigned short code0tab[16384];
static unsigned short code1tab[16384];
static unsigned short code2tab[16384];
static unsigned short code3tab[16384];
static unsigned short code4tab[16384];
static unsigned short * codetab[5] = {
	code0tab, code1tab, code2tab, code3tab, code4tab };

#define codetabof(i)	(codetab[(i) >> 14][(i) & 0x3fff])

#else	/* Normal machine */

#ifdef sel	/* gould base register braindamage */
/*NOBASE*/
static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];
/*NOBASE*/
#else
static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];
#endif /* sel */

#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]
#endif	/* XENIX_16 */
static code_int hsize = HSIZE;			/* for dynamic table sizing */
static count_int fsize;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i)	codetabof(i)
#ifdef XENIX_16
# define tab_suffixof(i)	((char_type *)htab[(i)>>15])[(i) & 0x7fff]
# define de_stack		((char_type *)(htab2))
#else	/* Normal machine */
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))
#endif	/* XENIX_16 */

static code_int free_ent = 0;			/* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int block_compress = BLOCK_MASK;
static int clear_flg = 0;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

#ifndef vax
static char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
static char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
#endif /* vax */

#define xputc(X)  { \
                        if((o + 1) > outl) return(-1); \
                        *o++ = X; \
                    }

static
unsigned char   *uncompress_input  = NULL,
                *uncompress_inputl = NULL;

#include <string.h>
static
int xread(unsigned char *buf, int skip1, int len, FILE *skip2) {
    if((uncompress_input + len) > uncompress_inputl) {
        len = uncompress_inputl - uncompress_input;
    }
    memcpy(buf, uncompress_input, len);
    uncompress_input += len;
    return(len);
}

static 
code_int
getcode() {
    /*
     * On the VAX, it is important to have the declarations
     * in exactly the order given, or the asm will break.
     */
    code_int code;
    static int offset = 0, size = 0;
    static char_type buf[BITS];
    int r_off, bits;
    char_type *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	size = xread( buf, 1, n_bits, stdin );
	if ( size <= 0 )
	    return -1;			/* end of file */
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
#ifdef vax
    asm( "extzv   r10,r9,(r8),r11" );
#else /* not a vax */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
#ifdef NO_UCHAR
	code = ((*bp++ >> r_off) & rmask[8 - r_off]) & 0xff;
#else
	code = (*bp++ >> r_off);
#endif /* NO_UCHAR */
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
#ifdef NO_UCHAR
	    code |= (*bp++ & 0xff) << r_off;
#else
	    code |= *bp++ << r_off;
#endif /* NO_UCHAR */
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
#endif /* vax */
    offset += n_bits;

    return code;
}

int uncompress_lzw(unsigned char *in, int insz, unsigned char *out, int outsz, int /*don't use u8*/ init_byte) {
    char_type *stackp, *stackl;
    int finchar;
    code_int code, oldcode, incode;
    unsigned char   *o,
                    *outl;

    clear_flg = 0;
    o = out;
    outl = out + outsz;
    uncompress_input  = in;
    uncompress_inputl = in + insz;

    if(init_byte < 0) {
        maxbits = BITS;
        maxmaxcode = 1 << BITS;
    } else {
		maxbits = init_byte & BIT_MASK;
		//block_mode = init_byte & BLOCK_MODE;
		maxmaxcode = MAXCODE(maxbits);
    }

    /*
     * As above, initialize the first 256 entries in the table.
     */
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (char_type)code;
    }
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode();
    if(oldcode == -1)	/* EOF already? */
	return(-1);			/* Get out of here */
    xputc( (char)finchar )		/* first code must be 8 bits = char */
    //if(ferror(stdout))		/* Crash if can't write */
	//writeerr();
    stackp = de_stack;
    stackl = de_stack + de_stack_size;

    while ( (code = getcode()) > -1 ) {

	if ( (code == CLEAR) && block_compress ) {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ( (code = getcode ()) == -1 )	/* O, untimely death! */
		break;
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if ( code >= free_ent ) {
        if(stackp >= stackl) return(-1);
            *stackp++ = finchar;
	    code = oldcode;
	}

	/*
	 * Generate output characters in reverse order
	 */
#ifdef SIGNED_COMPARE_SLOW
	while ( ((unsigned long)code) >= ((unsigned long)256) ) {
#else
	while ( code >= 256 ) {
#endif
        if(stackp >= stackl) return(-1);
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);

	/*
	 * And put them out in forward order
	 */
	do
	    xputc ( *--stackp )
	while ( stackp > de_stack );

	/*
	 * Generate the new entry.
	 */
	if ( (code=free_ent) < maxmaxcode ) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	} 
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    //fflush( stdout );
    //if(ferror(stdout))
	//writeerr();
    return(o - out);
}

