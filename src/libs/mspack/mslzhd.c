// kwajd.c -> mslzh.c
/* This file is part of libmspack.
 * (C) 2003-2011 Stuart Caie.
 *
 * KWAJ is a format very similar to SZDD. KWAJ method 3 (LZH) was
 * written by Jeff Johnson.
 *
 * libmspack is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (LGPL) version 2.1
 *
 * For further details, see the file COPYING.LIB distributed with libmspack
 */

/* KWAJ decompression implementation */

#include "system.h"
#include "mslzh.h"

/* prototypes */

struct mslzhd_stream *mslzh_init(
    struct mspack_system *sys, struct mspack_file *in, struct mspack_file *out);
int mslzh_decompress(
    struct mslzhd_stream *kwaj);
void mslzh_free(
    struct mslzhd_stream *kwaj);
static int mslzh_read_lens(
    struct mslzhd_stream *kwaj,
    unsigned int type, unsigned int numsyms,
    unsigned char *lens, unsigned short *table);
static int mslzh_read_input(
    struct mslzhd_stream *kwaj);

/***************************************
 * LZH_INIT, LZH_DECOMPRESS, LZH_FREE
 ***************************************
 * unpacks KWAJ method 3 files
 */

/* import bit-reading macros and code */
#define BITS_TYPE struct mslzhd_stream
#define BITS_VAR lzh
#define BITS_ORDER_MSB
#define BITS_NO_READ_INPUT
#define READ_BYTES do {					\
    if (i_ptr >= i_end) {				\
	if ((err = mslzh_read_input(lzh))) return err;	\
	i_ptr = lzh->i_ptr;				\
	i_end = lzh->i_end;				\
    }							\
    INJECT_BITS(*i_ptr++, 8);				\
} while (0)
#include "readbits.h"

/* import huffman-reading macros and code */
#define TABLEBITS(tbl)      KWAJ_TABLEBITS
#define MAXSYMBOLS(tbl)     KWAJ_##tbl##_SYMS
#define HUFF_TABLE(tbl,idx) lzh->tbl##_table[idx]
#define HUFF_LEN(tbl,idx)   lzh->tbl##_len[idx]
#define HUFF_ERROR          return MSPACK_ERR_DATAFORMAT
#include "readhuff.h"

/* In the KWAJ LZH format, there is no special 'eof' marker, it just
 * ends. Depending on how many bits are left in the final byte when
 * the stream ends, that might be enough to start another literal or
 * match. The only easy way to detect that we've come to an end is to
 * guard all bit-reading. We allow fake bits to be read once we reach
 * the end of the stream, but we check if we then consumed any of
 * those fake bits, after doing the READ_BITS / READ_HUFFSYM. This
 * isn't how the default readbits.h read_input() works (it simply lets
 * 2 fake bytes in then stops), so we implement our own.
 */
#define READ_BITS_SAFE(val, n) do {			\
    READ_BITS(val, n);					\
    if (lzh->input_end && bits_left < lzh->input_end)	\
	return MSPACK_ERR_OK;				\
} while (0)

#define READ_HUFFSYM_SAFE(tbl, val) do {		\
    READ_HUFFSYM(tbl, val);				\
    if (lzh->input_end && bits_left < lzh->input_end)	\
	return MSPACK_ERR_OK;				\
} while (0)

#define BUILD_TREE(tbl, type)					\
    STORE_BITS;							\
    err = mslzh_read_lens(lzh, type, MAXSYMBOLS(tbl),		\
			 &HUFF_LEN(tbl,0), &HUFF_TABLE(tbl,0));	\
    if (err) return err;					\
    RESTORE_BITS;						\
    if (make_decode_table(MAXSYMBOLS(tbl), TABLEBITS(tbl),	\
	&HUFF_LEN(tbl,0), &HUFF_TABLE(tbl,0)))			\
	return MSPACK_ERR_DATAFORMAT;

#define WRITE_BYTE do {							\
    if (lzh->sys->write(lzh->output, &lzh->window[pos], 1) != 1)	\
        return MSPACK_ERR_WRITE;					\
} while (0)

struct mslzhd_stream *mslzh_init(struct mspack_system *sys,
    struct mspack_file *in, struct mspack_file *out)
{
    struct mslzhd_stream *lzh;

    if (!sys || !in || !out) return NULL;
    if (!(lzh = (struct mslzhd_stream *) sys->alloc(sys, sizeof(struct mslzhd_stream)))) return NULL;

    lzh->sys    = sys;
    lzh->input  = in;
    lzh->output = out;
    return lzh;
}

int mslzh_decompress(struct mslzhd_stream *lzh)
{
    register unsigned int bit_buffer;
    register int bits_left, i;
    register unsigned short sym;
    unsigned char *i_ptr, *i_end, lit_run = 0;
    int j, pos = 0, len, offset, err;
    unsigned int types[6];

    /* reset global state */
    INIT_BITS;
    RESTORE_BITS;
    memset(&lzh->window[0], LZSS_WINDOW_FILL, (size_t) LZSS_WINDOW_SIZE);

    /* read 6 encoding types (for byte alignment) but only 5 are needed */
    for (i = 0; i < 6; i++) READ_BITS_SAFE(types[i], 4);

    /* read huffman table symbol lengths and build huffman trees */
    BUILD_TREE(MATCHLEN1, types[0]);
    BUILD_TREE(MATCHLEN2, types[1]);
    BUILD_TREE(LITLEN,    types[2]);
    BUILD_TREE(OFFSET,    types[3]);
    BUILD_TREE(LITERAL,   types[4]);

    while (!lzh->input_end) {
	if (lit_run) READ_HUFFSYM_SAFE(MATCHLEN2, len);
	else         READ_HUFFSYM_SAFE(MATCHLEN1, len);

	if (len > 0) {
	    len += 2;
	    lit_run = 0; /* not the end of a literal run */
	    READ_HUFFSYM_SAFE(OFFSET, j); offset = j << 6;
	    READ_BITS_SAFE(j, 6);         offset |= j;

	    /* copy match as output and into the ring buffer */
	    while (len-- > 0) {
		lzh->window[pos] = lzh->window[(pos+4096-offset) & 4095];
		WRITE_BYTE;
		pos++; pos &= 4095;
	    }
	}
	else {
	    READ_HUFFSYM_SAFE(LITLEN, len); len++;
	    lit_run = (len == 32) ? 0 : 1; /* end of a literal run? */
	    while (len-- > 0) {
		READ_HUFFSYM_SAFE(LITERAL, j);
		/* copy as output and into the ring buffer */
		lzh->window[pos] = j;
		WRITE_BYTE;
		pos++; pos &= 4095;
	    }
	}
    }
    return MSPACK_ERR_OK;
}

void mslzh_free(struct mslzhd_stream *lzh)
{
    struct mspack_system *sys;
    if (!lzh || !lzh->sys) return;
    sys = lzh->sys;
    sys->free(lzh);
}

static int mslzh_read_lens(struct mslzhd_stream *lzh,
			 unsigned int type, unsigned int numsyms,
			 unsigned char *lens, unsigned short *table)
{
    register unsigned int bit_buffer;
    register int bits_left;
    unsigned char *i_ptr, *i_end;
    unsigned int i, c, sel;
    int err;

    RESTORE_BITS;
    switch (type) {
    case 0:
	i = numsyms; c = (i==16)?4: (i==32)?5: (i==64)?6: (i==256)?8 :0;
	for (i = 0; i < numsyms; i++) lens[i] = c;
	break;

    case 1:
	READ_BITS_SAFE(c, 4); lens[0] = c;
	for (i = 1; i < numsyms; i++) {
    	           READ_BITS_SAFE(sel, 1); if (sel == 0)  lens[i] = c;
	    else { READ_BITS_SAFE(sel, 1); if (sel == 0)  lens[i] = ++c;
	    else { READ_BITS_SAFE(c, 4);                  lens[i] = c; }}
	}
	break;

    case 2:
	READ_BITS_SAFE(c, 4); lens[0] = c;
	for (i = 1; i < numsyms; i++) {
	    READ_BITS_SAFE(sel, 2);
	    if (sel == 3) READ_BITS_SAFE(c, 4); else c += (char) sel-1;
	    lens[i] = c;
	}
	break;

    case 3:
	for (i = 0; i < numsyms; i++) {
	    READ_BITS_SAFE(c, 4); lens[i] = c;
	}
	break;
    }
    STORE_BITS;
    return MSPACK_ERR_OK;
}

static int mslzh_read_input(struct mslzhd_stream *lzh) {
    int read;
    if (lzh->input_end) {
	lzh->input_end += 8;
	lzh->inbuf[0] = 0;
	read = 1;
    }
    else {
	read = lzh->sys->read(lzh->input, &lzh->inbuf[0], KWAJ_INPUT_SIZE);
	if (read < 0) return MSPACK_ERR_READ;
	if (read == 0) {
	    lzh->input_end = 8;
	    lzh->inbuf[0] = 0;
	    read = 1;
	}
    }

    /* update i_ptr and i_end */
    lzh->i_ptr = &lzh->inbuf[0];
    lzh->i_end = &lzh->inbuf[read];
    return MSPACK_ERR_OK;
}
