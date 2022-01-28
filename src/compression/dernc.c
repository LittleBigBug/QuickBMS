/*
 * dernc.c   decompress RNC data
 *
 * Compiled normally, this file is a well-behaved, re-entrant code
 * module exporting only `rnc_ulen', `rnc_unpack' and `rnc_error'.
 * Compiled with MAIN defined, it's a standalone program which will
 * decompress argv[1] into argv[2].
 */

#ifdef MAIN
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

#define INTERNAL
#include "dernc.h"

#ifdef MAIN
int main_unpack (char *pname, char *iname, char *oname);
int copy_file (char *iname, char *oname);

int main(int argc, char **argv) {
    int mode=0;
    int i;

    if (argc==1)
    {
	fprintf(stderr, "usage: %s <files> or %s -o <infile> <outfile>\n", 
		*argv, *argv);
	return 0;
    }
    for (i=1; i < argc; i++)
	if (!strcmp (argv[i], "-o"))
	    mode=i;
    if (mode && argc != 4)
    {
	fprintf(stderr, "usage: %s <files> or %s -o <infile> <outfile>\n", 
		*argv, *argv);
	return 1;
    }
    switch (mode)
    {
      case 0 :
	for (i=1; i < argc; i++)
	    if (main_unpack (*argv, argv[i], argv[i]))
		return 1;
	return 0;
      case 1 :
	return main_unpack (*argv, argv[2], argv[3]);
      case 2 :
	return main_unpack (*argv, argv[1], argv[3]);
      case 3 :
	return main_unpack (*argv, argv[1], argv[2]);
      default :
	fprintf (stderr, "Internal fault.\n");
    }
    return 1;
}

int main_unpack (char *pname, char *iname, char *oname)
{
    FILE *ifp, *ofp;
    long plen, ulen;
    void *packed, *unpacked;
    char buffer[4];
    
    ifp = fopen(iname, "rb");
    if (!ifp) {
	perror(iname);
	return 1;
    }

    fseek (ifp, 0L, SEEK_END);
    plen = ftell (ifp);
    rewind (ifp);
    if (plen < 4) /* Can't be an RNC file */
    {
	if (strcmp (iname, oname))
	    return copy_file (iname, oname);
	return 0;
    }
    fread (buffer, 1, 4, ifp);
    if (strncmp (buffer, "RNC", 3))
    {
	fclose (ifp);
	if (strcmp (iname, oname))
	    return copy_file (iname, oname);
	return 0;
    }
    rewind (ifp);
    packed = malloc(plen);
    if (!packed) {
	perror(pname);
	return 1;
    }
    fread (packed, 1, plen, ifp);
    fclose (ifp);

    ulen = rnc_ulen (packed);
    if (ulen < 0) 
    {
	free (packed);
	if (ulen == -1) /* File wasn't RNC to start with */
	    return 0;
	fprintf(stderr, "%s\n", rnc_error (ulen));
	return 1;
    }

    unpacked = malloc(ulen);
    if (!unpacked) {
	perror(pname);
	return 1;
    }

    ulen = rnc_unpack (packed, unpacked);
    if (ulen < 0) {
	fprintf(stderr, "%s\n", rnc_error (ulen));
	return 1;
    }

    ofp = fopen(oname, "wb");
    if (!ofp) {
	perror(oname);
	return 1;
    }

    fwrite (unpacked, 1, ulen, ofp);
    fclose (ofp);

    free (unpacked);
    free (packed);

    return 0;
}

int copy_file (char *iname, char *oname)
{
    char *sysbuf;
    
    sysbuf = malloc (strlen (iname)+strlen(oname)+6);
    if (!sysbuf)
    {
	fprintf (stderr, "Out of memory.\n");
	return 1;
    }
    strcpy (sysbuf, "cp ");
    strcat (sysbuf, iname);
    strcat (sysbuf, " ");
    strcat (sysbuf, oname);
    system (sysbuf);
    free (sysbuf);
    return 0;
}

#endif
    
typedef struct {
    unsigned long bitbuf;	       /* holds between 16 and 32 bits */
    int bitcount;		       /* how many bits does bitbuf hold? */
} bit_stream;

typedef struct {
    int num;			       /* number of nodes in the tree */
    struct {
	unsigned long code;
	int codelen;
	int value;
    } table[32];
} huf_table;

static void read_huftable (huf_table *h, bit_stream *bs, unsigned char **p);
static unsigned long huf_read (huf_table *h, bit_stream *bs,
			       unsigned char **p);

static void bitread_init (bit_stream *bs, unsigned char **p);
static void bitread_fix (bit_stream *bs, unsigned char **p);
static unsigned long bit_peek (bit_stream *bs, unsigned long mask);
static void bit_advance (bit_stream *bs, int n, unsigned char **p);
static unsigned long bit_read (bit_stream *bs, unsigned long mask,
			       int n, unsigned char **p);

static unsigned long blong (unsigned char *p);
//static unsigned long llong (unsigned char *p);
static unsigned long bword (unsigned char *p);
static unsigned long lword (unsigned char *p);

static unsigned long mirror (unsigned long x, int n);

/*
 * Return an error string corresponding to an error return code.
 */
char *rnc_error (long errcode) {
    static char *const errors[] = {
	"No error",
	"File is not RNC-1 format",
	"Huffman decode error",
	"File size mismatch",
	"CRC error in packed data",
	"CRC error in unpacked data",
	"Unknown error"
    };

    errcode = -errcode;
    if (errcode < 0)
	errcode = 0;
    if (errcode > sizeof(errors)/sizeof(*errors) - 1)
	errcode = sizeof(errors)/sizeof(*errors) - 1;
    return errors[errcode];
}

/*
 * Return the uncompressed length of a packed data block, or a
 * negative error code.
 */
long rnc_ulen (void *packed) {
    unsigned char *p = packed;
    if (blong (p) != RNC_SIGNATURE)
	return RNC_FILE_IS_NOT_RNC;
    return blong (p+4);
}

/*
 * Decompress a packed data block. Returns the unpacked length if
 * successful, or negative error codes if not.
 *
 * If COMPRESSOR is defined, it also returns the leeway number
 * (which gets stored at offset 16 into the compressed-file header)
 * in `*leeway', if `leeway' isn't NULL.
 */
long rnc_unpack (void *packed, void *unpacked
#ifdef COMPRESSOR
		 , long *leeway
#endif
		, long packed_len, long unpacked_len) {
    unsigned char *input = packed;
    unsigned char *output = unpacked;
    unsigned char *inputend, *outputend;
    bit_stream bs;
    huf_table raw, dist, len;
    unsigned long ch_count;
    unsigned long ret_len;
    unsigned out_crc;
#ifdef COMPRESSOR
    long lee = 0;
#endif

    if(unpacked_len >= 0) {
        ret_len = unpacked_len;
        outputend = output + ret_len;
        inputend = input + packed_len;
        out_crc = 0;
        goto rnc_unpack_go;
    }
    if (blong(input) != RNC_SIGNATURE)
    return RNC_FILE_IS_NOT_RNC;
    ret_len = blong (input+4);
    outputend = output + ret_len;
    inputend = input + 18 + blong(input+8);

    input += 18;		       /* skip header */

    /*
     * Check the packed-data CRC. Also save the unpacked-data CRC
     * for later.
     */
    if (rnc_crc(input, inputend-input) != bword(input-4))
	return RNC_PACKED_CRC_ERROR;
    out_crc = bword(input-6);

rnc_unpack_go:
    bitread_init (&bs, &input);
    bit_advance (&bs, 2, &input);      /* discard first two bits */

    /*
     * Process chunks.
     */
    while (output < outputend) {
#ifdef COMPRESSOR
	long this_lee;
#endif
	read_huftable (&raw, &bs, &input);
	read_huftable (&dist, &bs, &input);
	read_huftable (&len, &bs, &input);
	ch_count = bit_read (&bs, 0xFFFF, 16, &input);

	while (1) {
	    long length, posn;

	    length = huf_read (&raw, &bs, &input);
	    if (length == -1)
		return RNC_HUF_DECODE_ERROR;
	    if (length) {
		while (length--)
		    *output++ = *input++;
		bitread_fix (&bs, &input);
	    }
	    if (--ch_count <= 0)
		break;

	    posn = huf_read (&dist, &bs, &input);
	    if (posn == -1)
		return RNC_HUF_DECODE_ERROR;
	    length = huf_read (&len, &bs, &input);
	    if (length == -1)
		return RNC_HUF_DECODE_ERROR;
	    posn += 1;
	    length += 2;
	    while (length--) {
		*output = output[-posn];
		output++;
	    }
#ifdef COMPRESSOR
	    this_lee = (inputend - input) - (outputend - output);
	    if (lee < this_lee)
		lee = this_lee;
#endif
	}
    }

    if (outputend != output)
	return RNC_FILE_SIZE_MISMATCH;

#ifdef COMPRESSOR
    if (leeway)
	*leeway = lee;
#endif

    if(unpacked_len >= 0) {
        // do nothing
    } else {
    /*
     * Check the unpacked-data CRC.
     */
    if (rnc_crc(outputend-ret_len, ret_len) != out_crc)
	return RNC_UNPACKED_CRC_ERROR;
    }
    return ret_len;
}

/*
 * Read a Huffman table out of the bit stream and data stream given.
 */
static void read_huftable (huf_table *h, bit_stream *bs, unsigned char **p) {
    int i, j, k, num;
    int leaflen[32];
    int leafmax;
    unsigned long codeb;	       /* big-endian form of code */

    num = bit_read (bs, 0x1F, 5, p);
    if (!num)
	return;

    leafmax = 1;
    for (i=0; i<num; i++) {
	leaflen[i] = bit_read (bs, 0x0F, 4, p);
	if (leafmax < leaflen[i])
	    leafmax = leaflen[i];
    }

    codeb = 0L;
    k = 0;
    for (i=1; i<=leafmax; i++) {
	for (j=0; j<num; j++)
	    if (leaflen[j] == i) {
		h->table[k].code = mirror (codeb, i);
		h->table[k].codelen = i;
		h->table[k].value = j;
		codeb++;
		k++;
	    }
	codeb <<= 1;
    }

    h->num = k;
}

/*
 * Read a value out of the bit stream using the given Huffman table.
 */
static unsigned long huf_read (huf_table *h, bit_stream *bs,
			       unsigned char **p) {
    int i;
    unsigned long val;

    for (i=0; i<h->num; i++) {
	unsigned long mask = (1 << h->table[i].codelen) - 1;
	if (bit_peek(bs, mask) == h->table[i].code)
	    break;
    }
    if (i == h->num)
	return -1;
    bit_advance (bs, h->table[i].codelen, p);

    val = h->table[i].value;

    if (val >= 2) {
	val = 1 << (val-1);
	val |= bit_read (bs, val-1, h->table[i].value - 1, p);
    }
    return val;
}

/*
 * Initialises a bit stream with the first two bytes of the packed
 * data.
 */
static void bitread_init (bit_stream *bs, unsigned char **p) {
    bs->bitbuf = lword (*p);
    bs->bitcount = 16;
}

/*
 * Fixes up a bit stream after literals have been read out of the
 * data stream.
 */
static void bitread_fix (bit_stream *bs, unsigned char **p) {
    bs->bitcount -= 16;
    bs->bitbuf &= (1<<bs->bitcount)-1; /* remove the top 16 bits */
    bs->bitbuf |= (lword(*p)<<bs->bitcount);/* replace with what's at *p */
    bs->bitcount += 16;
}

/*
 * Returns some bits.
 */
static unsigned long bit_peek (bit_stream *bs, unsigned long mask) {
    return bs->bitbuf & mask;
}

/*
 * Advances the bit stream.
 */
static void bit_advance (bit_stream *bs, int n, unsigned char **p) {
    bs->bitbuf >>= n;
    bs->bitcount -= n;
    if (bs->bitcount < 16) {
	(*p) += 2;
	bs->bitbuf |= (lword(*p)<<bs->bitcount);
	bs->bitcount += 16;
    }
}

/*
 * Reads some bits in one go (ie the above two routines combined).
 */
static unsigned long bit_read (bit_stream *bs, unsigned long mask,
			       int n, unsigned char **p) {
    unsigned long result = bit_peek (bs, mask);
    bit_advance (bs, n, p);
    return result;
}

/*
 * Return the big-endian longword at p.
 */
static unsigned long blong (unsigned char *p) {
    unsigned long n;
    n = p[0];
    n = (n << 8) + p[1];
    n = (n << 8) + p[2];
    n = (n << 8) + p[3];
    return n;
}

/*
 * Return the little-endian longword at p.
 */
/*
static unsigned long llong (unsigned char *p) {
    unsigned long n;
    n = p[3];
    n = (n << 8) + p[2];
    n = (n << 8) + p[1];
    n = (n << 8) + p[0];
    return n;
}
*/

/*
 * Return the big-endian word at p.
 */
static unsigned long bword (unsigned char *p) {
    unsigned long n;
    n = p[0];
    n = (n << 8) + p[1];
    return n;
}

/*
 * Return the little-endian word at p.
 */
static unsigned long lword (unsigned char *p) {
    unsigned long n;
    n = p[1];
    n = (n << 8) + p[0];
    return n;
}

/*
 * Mirror the bottom n bits of x.
 */
static unsigned long mirror (unsigned long x, int n) {
    unsigned long top = 1 << (n-1), bottom = 1;
    while (top > bottom) {
	unsigned long mask = top | bottom;
	unsigned long masked = x & mask;
	if (masked != 0 && masked != mask)
	    x ^= mask;
	top >>= 1;
	bottom <<= 1;
    }
    return x;
}

/*
 * Calculate a CRC, the RNC way. It re-computes its CRC table every
 * time it's run, but who cares? ;-)
 */
long rnc_crc (void *data, long len) {
    unsigned short crctab[256];
    unsigned short val;
    int i, j;
    unsigned char *p = data;

    for (i=0; i<256; i++) {
	val = i;

	for (j=0; j<8; j++) {
	    if (val & 1)
		val = (val >> 1) ^ 0xA001;
	    else
		val = (val >> 1);
	}
	crctab[i] = val;
    }

    val = 0;
    while (len--) {
	val ^= *p++;
	val = (val >> 8) ^ crctab[val & 0xFF];
    }

    return val;
}
