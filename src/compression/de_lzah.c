// from macutil 2.0b3 of dik t. winter: http://homepages.cwi.nl/~dik/english/ftp.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BYTEMASK        0x000000ff

/* Note: compare with LZSS decoding in lharc! */

#define N	314
#define T	(2*N-1)

/*	Huffman table used for first 6 bits of offset:
	#bits	codes
	3	0x000
	4	0x040-0x080
	5	0x100-0x2c0
	6	0x300-0x5c0
	7	0x600-0xbc0
	8	0xc00-0xfc0
*/

static const unsigned short HuffCode[] = {
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040,
    0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040,
    0x080, 0x080, 0x080, 0x080, 0x080, 0x080, 0x080, 0x080,
    0x080, 0x080, 0x080, 0x080, 0x080, 0x080, 0x080, 0x080,
    0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0,
    0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0, 0x0c0,
    0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100,
    0x140, 0x140, 0x140, 0x140, 0x140, 0x140, 0x140, 0x140,
    0x180, 0x180, 0x180, 0x180, 0x180, 0x180, 0x180, 0x180,
    0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0,
    0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
    0x240, 0x240, 0x240, 0x240, 0x240, 0x240, 0x240, 0x240,
    0x280, 0x280, 0x280, 0x280, 0x280, 0x280, 0x280, 0x280,
    0x2c0, 0x2c0, 0x2c0, 0x2c0, 0x2c0, 0x2c0, 0x2c0, 0x2c0,
    0x300, 0x300, 0x300, 0x300, 0x340, 0x340, 0x340, 0x340,
    0x380, 0x380, 0x380, 0x380, 0x3c0, 0x3c0, 0x3c0, 0x3c0,
    0x400, 0x400, 0x400, 0x400, 0x440, 0x440, 0x440, 0x440,
    0x480, 0x480, 0x480, 0x480, 0x4c0, 0x4c0, 0x4c0, 0x4c0,
    0x500, 0x500, 0x500, 0x500, 0x540, 0x540, 0x540, 0x540,
    0x580, 0x580, 0x580, 0x580, 0x5c0, 0x5c0, 0x5c0, 0x5c0,
    0x600, 0x600, 0x640, 0x640, 0x680, 0x680, 0x6c0, 0x6c0,
    0x700, 0x700, 0x740, 0x740, 0x780, 0x780, 0x7c0, 0x7c0,
    0x800, 0x800, 0x840, 0x840, 0x880, 0x880, 0x8c0, 0x8c0,
    0x900, 0x900, 0x940, 0x940, 0x980, 0x980, 0x9c0, 0x9c0,
    0xa00, 0xa00, 0xa40, 0xa40, 0xa80, 0xa80, 0xac0, 0xac0,
    0xb00, 0xb00, 0xb40, 0xb40, 0xb80, 0xb80, 0xbc0, 0xbc0,
    0xc00, 0xc40, 0xc80, 0xcc0, 0xd00, 0xd40, 0xd80, 0xdc0,
    0xe00, 0xe40, 0xe80, 0xec0, 0xf00, 0xf40, 0xf80, 0xfc0};

static const short HuffLength[] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

unsigned char lzah_getbyte();

static void lzah_inithuf();
static void lzah_reorder();
static void lzah_move();
static void lzah_getbit();
static void lzah_outchar();

static char lzah_buf[4096];
static int lzah_bufptr;
static int lzah_bitsavail;
static int lzah_bits;
static int Frequ[1000];
static int ForwTree[1000];
static int BackTree[1000];

unsigned char   *out_ptr = NULL,
                *in_ptr  = NULL,
                *in_ptrl = NULL;
unsigned char lzah_getbyte(void) {
    if(in_ptr >= in_ptrl) return(-1);
    return(*in_ptr++);
}

int de_lzah(unsigned char *in, int insz, unsigned char *out, int obytes) {
    int i, i1, j, ch, byte, offs, skip;

    out_ptr = out;
    in_ptr  = in;
    in_ptrl = in + insz;

    lzah_inithuf();
    lzah_bitsavail = 0;
    for(i = 0; i < 4036; i++) {
	lzah_buf[i] = ' ';
    }
    lzah_bufptr = 4036;
    while(obytes != 0) {
	ch = ForwTree[T - 1];
	while(ch < T) {
	    lzah_getbit();
	    if(lzah_bits & 0x80) {
		ch = ch + 1;
	    }
	    ch = ForwTree[ch];
	}
	ch -= T;
	if(Frequ[T - 1] >= 0x8000) {
	    lzah_reorder();
	}

	i = BackTree[ch + T];
	do {
	    j = ++Frequ[i];
	    i1 = i + 1;
	    if(Frequ[i1] < j) {
		while(Frequ[++i1] < j) ;
		i1--;
		Frequ[i] = Frequ[i1];
		Frequ[i1] = j;

		j = ForwTree[i];
		BackTree[j] = i1;
		if(j < T) {
		    BackTree[j + 1] = i1;
		}
		ForwTree[i] = ForwTree[i1];
		ForwTree[i1] = j;
		j = ForwTree[i];
		BackTree[j] = i;
		if(j < T) {
		    BackTree[j + 1] = i;
		}
		i = i1;
	    }
	    i = BackTree[i];
	} while(i != 0);

	if(ch < 256) {
	    lzah_outchar((char)ch);
	    obytes--;
	} else {
	    if(lzah_bitsavail != 0) {
		byte = (lzah_bits << 1) & BYTEMASK;
		lzah_bits = lzah_getbyte() & BYTEMASK;
		byte |= (lzah_bits >> lzah_bitsavail);
		lzah_bits = lzah_bits << (7 - lzah_bitsavail);
	    } else {
		byte = lzah_getbyte() & BYTEMASK;
	    }
	    offs = HuffCode[byte];
	    skip = HuffLength[byte] - 2;
	    while(skip-- != 0) {
		byte = byte + byte;
		lzah_getbit();
		if(lzah_bits & 0x80) {
		    byte++;
		}
	    }
	    offs |= (byte & 0x3f);
	    offs = ((lzah_bufptr - offs - 1) & 0xfff);
	    ch = ch - 253;
	    while(ch-- > 0) {
		lzah_outchar(lzah_buf[offs++ & 0xfff]);
		obytes--;
		if(obytes == 0) {
		    break;
		}
	    }
	}
    }
    return(out_ptr - out);
}

static void lzah_inithuf()
{
    int i, j;

    for(i = 0; i < N; i++) {
	Frequ[i] = 1;
	ForwTree[i] = i + T;
	BackTree[i + T] = i;
    }
    for(i = 0, j = N; j < T; i += 2, j++) {
	Frequ[j] = Frequ[i] + Frequ[i + 1];
	ForwTree[j] = i;
	BackTree[i] = j;
	BackTree[i + 1] = j;
    }
    Frequ[T] = 0xffff;
    BackTree[T - 1] = 0;
}

static void lzah_reorder()
{
    int i, j, k, l;

    j = 0;
    for(i = 0; i < T; i++) {
	if(ForwTree[i] >= T) {
	    Frequ[j] = ((Frequ[i] + 1) >> 1);
	    ForwTree[j] = ForwTree[i];
	    j++;
	}
    }
    for(i = 0, j = N; i < T; i += 2, j++) {
	k = i + 1;
	l = Frequ[i] + Frequ[k];
	Frequ[j] = l;
	k = j - 1;
	while(l < Frequ[k]) {
	    k--;
	}
	k = k + 1;
	lzah_move(Frequ + k, Frequ + k + 1, j - k);
	Frequ[k] = l;
	lzah_move(ForwTree + k, ForwTree + k + 1, j - k);
	ForwTree[k] = i;
    }
    for(i = 0; i < T; i++) {
	k = ForwTree[i];
	if(k >= T) {
	    BackTree[k] = i;
	} else {
	    BackTree[k] = i;
	    BackTree[k + 1] = i;
	}
    }
}

static void lzah_move(p, q, n)
int *p, *q, n;
{
    if(p > q) {
	while(n-- > 0) {
	    *q++ = *p++;
	}
    } else {
	p += n;
	q += n;
	while(n-- > 0) {
	    *--q = *--p;
	}
    }
}

static void lzah_getbit()
{
    if(lzah_bitsavail != 0) {
	lzah_bits = lzah_bits + lzah_bits;
	lzah_bitsavail--;
    } else {
	lzah_bits = lzah_getbyte() & BYTEMASK;
	lzah_bitsavail = 7;
    }
}

static void lzah_outchar(ch)
char ch;
{
    *out_ptr++ = ch;
    lzah_buf[lzah_bufptr++] = ch;
    lzah_bufptr &= 0xfff;
}

