// by Luigi Auriemma
// partially derived from the original code of Okumura

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int EI = 12, EJ = 4, P = 2, rless = 2/*P*/;

static int N/* = 1<<EI*/;      /* size of ring buffer */
static int F/* = (1<<EJ)+P*/;  /* upper limit for match_length */
static int THRESHOLD = 2/*P*/; /* encode string into position and length if match_length is greater than this */
static int NIL;// N;           /* index for root of binary search trees */
static int init_chr = ' ';

static void lzss_init(void) {
    EI = 12;    /* typically 10..13 */
    EJ = 4;     /* typically 4..5 */
    P  = 2;     /* If match length <= P then output one character */
    rless = P;  // in some rare implementations it could be 0
    init_chr = ' ';
}

static void lzss_set_window(u8 *window, int window_size, int init_chr) {
    int     i,
            n;
    switch(init_chr) {
        case -1:    // Tales of Vesperia (thanks to delguoqing)
            memset(window, 0, window_size);
            for(i = 0;; i++) {
                n = (i * 8) + 6;
                if(n >= window_size) break;
                window[n] = i;
            }
            break;
        case -2:    // invented
            for(i = 0; i < window_size; i++) window[i] = i;
            break;
        case -3:    // invented
            for(i = window_size - 1; i >= 0; i--) window[i] = i;
            break;
        default:
            memset(window, init_chr, window_size);
            break;
    }
}

int unlzss(unsigned char *src, int srclen, unsigned char *dst, int dstlen, u8 *parameters) {
    lzss_init();

    static int slide_winsz = 0;
    static unsigned char *slide_win = NULL;
    unsigned char *dststart = dst;
    unsigned char *srcend = src + srclen;
    unsigned char *dstend = dst + dstlen;
    int  i, j, k, r, c;
    unsigned flags;

    if(parameters) {
        //sscanf(parameters, "%d %d %d %d %d",
        get_parameter_numbers(parameters,
            &EI, &EJ, &P, &rless, &init_chr, NULL);
    }
    N = 1 << EI;
    F = 1 << EJ;

    if(N > slide_winsz) {
        slide_win   = realloc(slide_win, N);
        if(!slide_win) return -1;
        slide_winsz = N;
    }
    lzss_set_window(slide_win, N, init_chr);

    dst = dststart;
    srcend = src + srclen;
    r = (N - F) - rless;
    N--;    // AND
    F--;    // AND

    for(flags = 0;; flags >>= 1) {
        if(!(flags & 0x100)) {
            if(src >= srcend) break;
            flags = *src++;
            flags |= 0xff00;
        }
        if(flags & 1) {
            if(src >= srcend) break;
            c = *src++;
            if(dst >= dstend) goto quit; //return -1; better?
            *dst++ = c;
            slide_win[r] = c;
            r = (r + 1) & N;
        } else {
            if(src >= srcend) break;
            i = *src++;
            if(src >= srcend) break;
            j = *src++;
            i |= ((j >> EJ) << 8);
            j  = (j & F) + P;
            for(k = 0; k <= j; k++) {
                c = slide_win[(i + k) & N];
                if(dst >= dstend) goto quit; //return -1; better?
                *dst++ = c;
                slide_win[r] = c;
                r = (r + 1) & N;
            }
        }
    }
quit:
    return(dst - dststart);
}



// modified by Luigi Auriemma
/**************************************************************
	LZSS.C -- A Data Compression Program
	(tab = 4 spaces)
***************************************************************
	4/6/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN		SCIENCE
		NIFTY-Serve	PAF01022
		CompuServe	74050,1022
**************************************************************/

static unsigned int
		textsize = 0,	/* text size counter */
		codesize = 0;	/* code size counter */
static unsigned char *text_buf = NULL;	/* ring buffer of size N,
			with extra F-1 bytes to facilitate string comparison */
static int		match_position, match_length;  /* of longest match.  These are
			set by the InsertNode() procedure. */
static int  *lson = NULL,
            *rson = NULL,
            *dad  = NULL;
            /* left & right children &
			parents -- These constitute binary search trees. */

static
unsigned char   *infile   = NULL,
                *infilel  = NULL,
                *outfile  = NULL,
                *outfilel = NULL;
static int lzss_xgetc(void *skip) {
    if(infile >= infilel) return -1;
    return(*infile++);
}
static int lzss_xputc(int chr, void *skip) {
    if(outfile >= outfilel) return -1;
    *outfile++ = chr;
    return(chr);
}

static void InitTree(void)  /* initialize trees */
{
	int  i;

	/* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
	   left children of node i.  These nodes need not be initialized.
	   Also, dad[i] is the parent of node i.  These are initialized to
	   NIL (= N), which stands for 'not used.'
	   For i = 0 to 255, rson[N + i + 1] is the root of the tree
	   for strings that begin with character i.  These are initialized
	   to NIL.  Note there are 256 trees. */

	for (i = N + 1; i <= N + 256; i++) rson[i] = NIL;
	for (i = 0; i < N; i++) dad[i] = NIL;
}

static void InsertNode(int r)
	/* Inserts string of length F, text_buf[r..r+F-1], into one of the
	   trees (text_buf[r]'th tree) and returns the longest-match position
	   and length via the global variables match_position and match_length.
	   If match_length = F, then removes the old node in favor of the new
	   one, because the old one will be deleted sooner.
	   Note r plays double role, as tree node and position in buffer. */
{
	int  i, p, cmp;
	unsigned char  *key;

	cmp = 1;  key = &text_buf[r];  p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;  match_length = 0;
	for ( ; ; ) {
		if (cmp >= 0) {
			if (rson[p] != NIL) p = rson[p];
			else {  rson[p] = r;  dad[r] = p;  return;  }
		} else {
			if (lson[p] != NIL) p = lson[p];
			else {  lson[p] = r;  dad[r] = p;  return;  }
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)  break;
		if (i > match_length) {
			match_position = p;
			if ((match_length = i) >= F)  break;
		}
	}
	dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
	dad[lson[p]] = r;  dad[rson[p]] = r;
	if (rson[dad[p]] == p) rson[dad[p]] = r;
	else                   lson[dad[p]] = r;
	dad[p] = NIL;  /* remove p */
}

static void DeleteNode(int p)  /* deletes node p from tree */
{
	int  q;
	
	if (dad[p] == NIL) return;  /* not in tree */
	if (rson[p] == NIL) q = lson[p];
	else if (lson[p] == NIL) q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {  q = rson[q];  } while (rson[q] != NIL);
			rson[dad[q]] = lson[q];  dad[lson[q]] = dad[q];
			lson[q] = lson[p];  dad[lson[p]] = q;
		}
		rson[q] = rson[p];  dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p) rson[dad[p]] = q;  else lson[dad[p]] = q;
	dad[p] = NIL;
}

static void Encode(void)
{
	int  i, c, len, r, s, last_match_length, code_buf_ptr;
	unsigned char  code_buf[/*17*/33/*???*/], mask;
	
	InitTree();  /* initialize trees */
	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;  r = N - F;
	//for (i = s; i < r; i++) text_buf[i] = init_chr;  /* Clear the buffer with
	//	any character that will appear often. */
    lzss_set_window(text_buf, r, init_chr);

	for (len = 0; len < F && (c = lzss_xgetc(infile)) != EOF; len++)
		text_buf[r + len] = c;  /* Read F bytes into the last F bytes of
			the buffer */
	if ((textsize = len) == 0) return;  /* text of size zero */
	for (i = 1; i <= F; i++) InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */
	do {
		if (match_length > len) match_length = len;  /* match_length
			may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD) {
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		} else {
			code_buf[code_buf_ptr++] = (unsigned char) match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
              (((match_position >> (8-EJ)) & ~((1<<EJ)-1))
			  | (match_length - (THRESHOLD + 1)));  /* Send position and
					length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
				lzss_xputc(code_buf[i], outfile);     /* code together */
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length &&
				(c = lzss_xgetc(infile)) != EOF; i++) {
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = c;	/* read new bytes */
			if (s < F - 1) text_buf[s + N] = c;  /* If the position is
				near the end of buffer, extend the buffer to make
				string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
				/* Since this is a ring buffer, increment the position
				   modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		while (i++ < last_match_length) {	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len) InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */
	if (code_buf_ptr > 1) {		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++) lzss_xputc(code_buf[i], outfile);
		codesize += code_buf_ptr;
	}
}

int lzss_compress(u8 *in, int insz, u8 *out, int outsz, u8 *parameters) {
    lzss_init();

    N = 1<<EI;
    F = (1<<EJ)+P;
    THRESHOLD =	P;

		textsize = 0;	/* text size counter */
		codesize = 0;	/* code size counter */

    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

    if(parameters) {
        get_parameter_numbers(parameters,
            &EI, &EJ, &P, &rless, &init_chr, NULL);

        if((EI < 0) || (EI >= 0xffff)) return -1;
        if((EJ < 0) || (EJ >= 31))     return -1;
        if((P  < 0) || (P  >= 0xffff)) return -1;

        if(EI >= 16) N = EI;
        else         N = 1 << EI;
        THRESHOLD = P;
        F = (1 << EJ) + THRESHOLD;
        // rless unused
    }

    NIL = N;
    text_buf = realloc(text_buf, N + F - 1);
    lson     = realloc(lson, sizeof(int) * (N + 1));
    rson     = realloc(rson, sizeof(int) * (N + 257));
    dad      = realloc(dad,  sizeof(int) * (N + 1));

    Encode();
    return(outfile - out);
}

