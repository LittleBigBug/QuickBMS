// from macutil 2.0b3 of dik t. winter: http://homepages.cwi.nl/~dik/english/ftp.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BYTEMASK        0x000000ff
#define WORDMASK        0x0000ffff
#define  BITBUFSIZ       16


// bits_be.c
unsigned int bit_be_bitbuf;
char *bit_be_filestart;
int bit_be_inbytes;

static unsigned int bit_be_subbitbuf;
static int bit_be_bitcount;

void bit_be_fillbuf(n)  /* Shift bit_be_bitbuf n bits left, read n bits */
int n;
{
    bit_be_bitbuf <<= n;
    while (n > bit_be_bitcount) {
	bit_be_bitbuf |= bit_be_subbitbuf << (n -= bit_be_bitcount);
	if(bit_be_inbytes == 0) {
	    bit_be_subbitbuf = 0;
	} else {
	    bit_be_subbitbuf = *bit_be_filestart++ & BYTEMASK;
	    bit_be_inbytes--;
	}
	bit_be_bitcount = 8;
    }
    bit_be_bitbuf |= bit_be_subbitbuf >> (bit_be_bitcount -= n);
    bit_be_bitbuf &= WORDMASK;
}

unsigned int bit_be_getbits(n)
int n;
{
    unsigned int x;

    x = bit_be_bitbuf >> (BITBUFSIZ - n);
    bit_be_fillbuf(n);
    return x;
}

void bit_be_init_getbits()
{
    bit_be_bitbuf = 0;
    bit_be_subbitbuf = 0;
    bit_be_bitcount = 0;
    bit_be_fillbuf(BITBUFSIZ);
}
// bits_be.c


/* This code is valid for bitsused upto 15. */
#define DICBIT    13    /* 12(-lh4-) or 13(-lh5-) */
#ifndef UCHAR_MAX
#define UCHAR_MAX    255
#endif
#define THRESHOLD    3

static int decoded;
static int bitsused;
static unsigned int blocksize;
static unsigned int decode_c();
static unsigned int decode_p();
static int make_table();

/* lzh compression */
int de_lzh(unsigned char *in, int ibytes, unsigned char *out, int obytes, int bits) {
    unsigned int i, r, c;
    int remains;

    bit_be_inbytes = ibytes;
    bit_be_filestart = in;
    bitsused = bits;
    bit_be_init_getbits();
    blocksize = 0;
    decoded = 0;
    r = 0;
    for(;;) {
	c = decode_c();
	if(decoded) {
	    //*data = bit_be_filestart;
	    //return;
        break;
	}
	if(c <= UCHAR_MAX) {
        if(r >= obytes) return(-1);
	    out[r++] = c;
	} else {
	    remains = c - (UCHAR_MAX + 1 - THRESHOLD);
	    i = (r - decode_p() - 1);
        if(i < 0) return(-1);
	    while(--remains >= 0) {
            if(r >= obytes) return(-1);
            out[r++] = out[i++];
	    }
	}
    }
    return(r);
}

#define MAXMATCH 256
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT 9
#define CODE_BIT 16
#define NP (DICBIT + 1)
#define NT (CODE_BIT + 3)
#define PBIT 4  /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5  /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
# define NPT NT
#else
# define NPT NP
#endif

static unsigned int left[2 * NC - 1], right[2 * NC - 1];
static unsigned char c_len[NC], pt_len[NPT];
static unsigned int c_table[4096], pt_table[256];

static void read_pt_len(nn, nbit, i_special)
int nn;
int nbit;
int i_special;
{
    int i, c, n;
    unsigned int mask;

    n = bit_be_getbits(nbit);
    if (n == 0) {
	c = bit_be_getbits(nbit);
	for (i = 0; i < nn; i++) {
	    pt_len[i] = 0;
	}
	for (i = 0; i < 256; i++) {
	    pt_table[i] = c;
	}
    } else {
	i = 0;
	while (i < n) {
	    c = bit_be_bitbuf >> (BITBUFSIZ - 3);
	    if (c == 7) {
		mask = (unsigned) 1 << (BITBUFSIZ - 1 - 3);
		while (mask & bit_be_bitbuf) {
		    mask >>= 1;
		    c++;
		}
	    }
	    bit_be_fillbuf((c < 7) ? 3 : c - 3);
	    pt_len[i++] = c;
	    if (i == i_special) {
		c = bit_be_getbits(2);
		while (--c >= 0) {
		    pt_len[i++] = 0;
		}
	    }
	}
	while (i < nn) {
	    pt_len[i++] = 0;
	}
	make_table(nn, pt_len, 8, pt_table, 256);
    }
}

static void read_c_len()
{
    int i, c, n;
    unsigned int mask;

    n = bit_be_getbits(CBIT);
    if (n == 0) {
	c = bit_be_getbits(CBIT);
	for (i = 0; i < NC; i++) {
	    c_len[i] = 0;
	}
	for (i = 0; i < 4096; i++) {
	    c_table[i] = c;
	}
    } else {
	i = 0;
	while (i < n) {
	    c = pt_table[bit_be_bitbuf >> (BITBUFSIZ - 8)];
	    if (c >= NT) {
		mask = (unsigned) 1 << (BITBUFSIZ - 1 - 8);
		do {
		    if (bit_be_bitbuf & mask) {
			c = right[c];
		    } else {
			c = left [c];
		    }
		    mask >>= 1;
		} while (c >= NT);
	    }
	    bit_be_fillbuf((int)pt_len[c]);
	    if (c <= 2) {
		if (c == 0) {
		    c = 1;
		} else if (c == 1) {
		    c = bit_be_getbits(4) + 3;
		} else {
		    c = bit_be_getbits(CBIT) + 20;
		}
		while (--c >= 0) {
		    c_len[i++] = 0;
		}
	    } else {
		c_len[i++] = c - 2;
	    }
	}
	while (i < NC) {
	    c_len[i++] = 0;
	}
	make_table(NC, c_len, 12, c_table, 4096);
    }
}

static unsigned int decode_c()
{
    unsigned int j, mask;

    if (blocksize == 0) {
	blocksize = bit_be_getbits(16);
	if (blocksize == 0) {
	    decoded = 1;
	    return 0;
	}
	read_pt_len(NT, TBIT, 3);
	read_c_len();
	read_pt_len(bitsused + 1, PBIT, -1);
    }
    blocksize--;
    j = c_table[bit_be_bitbuf >> (BITBUFSIZ - 12)];
    if (j >= NC) {
	mask = (unsigned) 1 << (BITBUFSIZ - 1 - 12);
	do {
	    if (bit_be_bitbuf & mask) {
		j = right[j];
	    } else {
		j = left [j];
	    }
	    mask >>= 1;
	} while (j >= NC);
    }
    bit_be_fillbuf((int)c_len[j]);
    return j;
}

static unsigned int decode_p()
{
    unsigned int j, mask;

    j = pt_table[bit_be_bitbuf >> (BITBUFSIZ - 8)];
    if ((int)j > bitsused) {
	mask = (unsigned) 1 << (BITBUFSIZ - 1 - 8);
	do {
	    if (bit_be_bitbuf & mask) {
		j = right[j];
	    } else {
		j = left [j];
	    }
	    mask >>= 1;
	} while ((int)j > bitsused);
    }
    bit_be_fillbuf((int)pt_len[j]);
    if (j != 0) {
	j = ((unsigned) 1 << (j - 1)) + bit_be_getbits((int) (j - 1));
    }
    return j;
}

static int make_table(nchar, bitlen, tablebits, table, tablesz)
int nchar;
unsigned char bitlen[];
int tablebits;
unsigned int table[];
int tablesz;
{
    unsigned int count[17], weight[17], start[18], *p;
    unsigned int i, k, len, ch, jutbits, avail, nextcode, mask;

    for (i = 1; i <= 16; i++) {
	count[i] = 0;
    }
    for (i = 0; (int)i < nchar; i++) {
	count[bitlen[i]]++;
    }

    start[1] = 0;
    for (i = 1; i <= 16; i++) {
	start[i + 1] = start[i] + (count[i] << (16 - i));
    }

    jutbits = 16 - tablebits;
    for (i = 1; (int)i <= tablebits; i++) {
	start[i] >>= jutbits;
	weight[i] = (unsigned) 1 << (tablebits - i);
    }
    while (i <= 16) {
       weight[i] = (unsigned) 1 << (16 - i);
       i++;
    }

    i = start[tablebits + 1] >> jutbits;
    if (i != (unsigned int)((unsigned) 1 << 16)) {
	k = 1 << tablebits;
	while (i != k) {
        if(i >= tablesz) return(-1);
	    table[i++] = 0;
	}
    }

    avail = nchar;
    mask = (unsigned) 1 << (15 - tablebits);
    for (ch = 0; (int)ch < nchar; ch++) {
	if ((len = bitlen[ch]) == 0) {
	    continue;
	}
	nextcode = start[len] + weight[len];
	if ((int)len <= tablebits) {
	    for (i = start[len]; i < nextcode; i++) {
        if(i >= tablesz) return(-1);
		table[i] = ch;
	    }
	} else {
	    k = start[len];
	    p = &table[k >> jutbits];
	    i = len - tablebits;
	    while (i != 0) {
		if (*p == 0) {
		    right[avail] = left[avail] = 0;
		    *p = avail++;
		}
		if (k & mask) {
		    p = &right[*p];
		} else {
		    p = &left[*p];
		}
		k <<= 1;
		i--;
	    }
	    *p = ch;
	}
	start[len] = nextcode;
    }
    return(0);
}
