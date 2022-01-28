#define DECOMPRESS
//#include "macunpack.h"
#ifdef SIT
#define DECOMPRESS
#endif /* SIT */
#ifdef LZC
#define DECOMPRESS
#endif /* LZC */
#ifdef DECOMPRESS
//#include "globals.h"
//#include "../fileio/wrfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Written to allow for bits to be upto 16, MacCompress can use 16 bits */

#define	BITS	16
#define HSIZE	69001		/* 95% occupancy */

#define INIT_BITS 9			/* initial number of bits/code */

static int n_bits;				/* number of bits/code */
static int maxbits;			/* user settable max # bits/code */
static long maxcode;			/* maximum code, given n_bits */
static long maxmaxcode;			/* should NEVER generate this code */
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

static long htab [HSIZE];
static unsigned short codetab [HSIZE];

#define tab_prefixof(i) codetab[i]
#define tab_suffixof(i)	((unsigned char *)(htab))[i]
#define de_stack	((unsigned char *)&tab_suffixof(1<<BITS))

static long free_ent = 0;			/* first unused entry */

static long getcode();

static int clear_flg = 0;

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

static int toread;

static unsigned char    *de_compress_in,
                        *de_compress_inl;

int de_compress(ibytes, mb, infp, out)
unsigned long ibytes;
int mb;
unsigned char *infp;
unsigned char *out;
{
    de_compress_in  = infp;
    de_compress_inl = infp + ibytes;
    unsigned char *out_ptr = out;

    register unsigned char *stackp;
    register int finchar;
    register long code, oldcode, incode;

    toread = ibytes;
    maxbits = mb;
    maxmaxcode = 1 << maxbits;
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for(code = 255; code >= 0; code--) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (unsigned char)code;
    }
    free_ent = FIRST;
    finchar = oldcode = getcode();
    if(oldcode == -1) {	/* EOF already? */
	return out_ptr - out;			/* Get out of here */
    }
    /* first code must be 8 bits = char */
    *out_ptr++ = (char)finchar;
    stackp = de_stack;
    while((code = getcode()) > -1) {
	if(code == CLEAR) {
	    for(code = 255; code >= 0; code--) {
		tab_prefixof(code) = 0;
	    }
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if((code = getcode()) == -1) {	/* O, untimely death! */
		break;
	    }
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if(code >= free_ent) {
	    *stackp++ = finchar;
	    code = oldcode;
	}
	/*
	 * Generate output characters in reverse order
	 */
	while(code >= 256) {
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);
	/*
	 * And put them out in forward order
	 */
	do {
	    *out_ptr++ = (char)*--stackp;
	} while(stackp > de_stack);
	/*
	 * Generate the new entry.
	 */
	if((code=free_ent) < maxmaxcode) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	}
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    return out_ptr - out;
}

static unsigned char rmask[9] =
    {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

static int get_core_bytes;
static char *core_ptr;
static int file_bytes();
static int core_bytes();

static long getcode()
{
    register long code;
    static int offset = 0, size = 0;
    static unsigned char buf[BITS];
    register int r_off, bits;
    register unsigned char *bp = buf;

    if(clear_flg > 0 || offset >= size || free_ent > maxcode) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if(free_ent > maxcode) {
	    n_bits++;
	    if(n_bits == maxbits) {
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    } else {
		maxcode = MAXCODE(n_bits);
	    }
	}
	if(clear_flg > 0) {
	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	if(toread == 0) {
	    return -1;
	}
	if(get_core_bytes) {
	    size = core_bytes((char *)buf, (n_bits < toread ? n_bits : toread));
	} else {
	    size = file_bytes((char *)buf, (n_bits < toread ? n_bits : toread));
	}
	toread -= size;
	if(size <= 0) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("macunpack: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
    /*
     * Get to the first byte.
     */
    bp += (r_off >> 3);
    r_off &= 7;
    /* Get first part (low order bits) */
    code = (*bp++ >> r_off);
    bits -= (8 - r_off);
    r_off = 8 - r_off;		/* now, offset into code word */
    /* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
    if(bits >= 8) {
	code |= *bp++ << r_off;
	r_off += 8;
	bits -= 8;
    }
    /* high order bits. */
    code |= (*bp & rmask[bits]) << r_off;
    offset += n_bits;
    return code;
}

static int file_bytes(buf, length)
char *buf;
int length;
{
    //return fread(buf, 1, length, infp);
    if(length > (de_compress_inl - de_compress_in)) length = de_compress_inl - de_compress_in;
    memcpy(buf, de_compress_in, length);
    de_compress_in += length;
    return length;
}

static int core_bytes(buf, length)
char *buf;
int length;
{
    int i;

    for(i = 0; i < length; i++) {
	*buf++ = *core_ptr++;
    }
    return length;
}

void core_compress(ptr)
char *ptr;
{
    core_ptr = ptr;
    get_core_bytes = ptr != NULL;
}
#else /* DECOMPRESS */
int decompress; /* keep lint and some compilers happy */
#endif /* DECOMPRESS */

