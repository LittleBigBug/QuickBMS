// modified by Luigi Auriemma (not tested)

/*********************************************************************/
/* This file contains two versions of the lzd() decompression routine.
The default is to use a fast version coded by Ray Gardner.  If the
symbol SLOW_LZD is defined, the older slower one is used.  I have tested
Ray's code and it seems to be portable and reliable.  But if you
suspect any problems you can define SLOW_LZD for your system in
options.h and cause the older code to be used.  --R.D. */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define  IN_BUF_SIZE       8192
#define  OUT_BUF_SIZE      8192
#define  INBUFSIZ    (IN_BUF_SIZE - 10)   /* avoid obo errors */
#define  OUTBUFSIZ   (OUT_BUF_SIZE - 10)
#define  MEMERR      2
#define  IOERR       1
#define  MAXBITS     13
#define  CLEAR       256         /* clear code */
#define  Z_EOF       257         /* end of file marker */
#define  FIRST_FREE  258         /* first free code */
#define  MAXMAX      8192        /* max code + 1 */
#define UNBUF_IO
typedef void *  VOIDPTR;
#define assert(X)   if(!(X)) exit(1)
#define debug(X)    {}
#define prterror(X,Y)   exit(1)
#define ealloc(X)   calloc(X,1)

/*********************************************************************/
/* Original slower lzd().                                            */
/*********************************************************************/

/*
Lempel-Ziv decompression.  Mostly based on Tom Pfau's assembly language
code.  The contents of this file are hereby released to the public domain.
                                 -- Rahul Dhesi 1986/11/14
*/

#define  STACKSIZE   4000

struct tabentry {
   unsigned next;
   char z_ch;
};

void init_dtab ();
unsigned rd_dcode ();
void wr_dchar (int);
void ad_dcode ();

#ifdef FILTER
/* to send data back to zoofilt */
/*extern*/ unsigned int filt_lzd_word;
#endif /* FILTER */


static unsigned stack_pointer = 0;
static unsigned *stack;

#define  push(x)  {  \
                     stack[stack_pointer++] = (x);                   \
                     if (stack_pointer >= STACKSIZE)                 \
                        prterror ('f', "Stack overflow in lzd().\n");\
                  }
#define  pop()    (stack[--stack_pointer])

/*extern*/ char out_buf_adr[OUTBUFSIZ];        /* output buffer */
/*extern*/ char in_buf_adr[IN_BUF_SIZE];         /* input buffer */

char memflag = 0;                /* memory allocated? flag */
/*extern*/ struct tabentry *table;   /* hash table from lzc.c */
static unsigned cur_code;
static unsigned old_code;
static unsigned in_code;

static unsigned free_code;
static int nbits;
static unsigned max_code;

static char fin_char;
static char k;
static unsigned masks[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff };
static unsigned bit_offset;
static unsigned output_offset;

#ifdef UNBUF_IO
#define		BLOCKFILE		unsigned char *
int BLOCKREAD(BLOCKFILE *p, unsigned char *data, int size) {
    memcpy(data, *p, size);
    (*p) += size;
    return size;
}
int BLOCKWRITE(BLOCKFILE *p, unsigned char *data, int size) {
    memcpy(*p, data, size);
    (*p) += size;
    return size;
}
/*
#define		BLOCKFILE		int
#define		BLOCKREAD		read
#define		BLOCKWRITE		blockwrite
*/
#else
#define		BLOCKFILE		ZOOFILE
#define		BLOCKREAD		zooread
#define		BLOCKWRITE		zoowrite
#endif /* UNBUF_IO */

static BLOCKFILE in_f;
static BLOCKFILE out_f; 

int lzd(BLOCKFILE input_f, BLOCKFILE output_f)
//BLOCKFILE input_f, output_f;          /* input & output file handles */
{
   in_f = input_f;                 /* make it avail to other fns */
   out_f = output_f;               /* ditto */
   nbits = 9;
   max_code = 512;
   free_code = FIRST_FREE;
   stack_pointer = 0;
   bit_offset = 0;
   output_offset = 0;

   if (BLOCKREAD (&in_f, in_buf_adr, INBUFSIZ) == -1)
      return(IOERR);
   if (memflag == 0) {
     table = (struct tabentry *) ealloc((MAXMAX+10) * sizeof(struct tabentry));
     stack = (unsigned *) ealloc (sizeof (unsigned) * STACKSIZE + 20);
     memflag++;
   }

   init_dtab();             /* initialize table */

loop:
   cur_code = rd_dcode();
goteof: /* special case for CLEAR then Z_EOF, for 0-length files */
   if (cur_code == Z_EOF) {
      debug((printf ("lzd: Z_EOF\n")))
      if (output_offset != 0) {
         if (BLOCKWRITE (&out_f, out_buf_adr, output_offset) != output_offset)
            prterror ('f', "Output error in lzd().\n");
         //addbfcrc(out_buf_adr, output_offset);
      }
#ifdef FILTER
		/* get next two bytes and put them where zoofilt can find them */
		/* nbits known to be in range 9..13 */
		bit_offset = ((bit_offset + 7) / 8) * 8; /* round up to next byte */
		filt_lzd_word = rd_dcode();
		filt_lzd_word |= (rd_dcode() << nbits);
		filt_lzd_word &= 0xffff;
#endif
      return (0);
   }

   assert(nbits >= 9 && nbits <= 13);

   if (cur_code == CLEAR) {
      debug((printf ("lzd: CLEAR\n")))
      init_dtab();
      fin_char = k = old_code = cur_code = rd_dcode();
		if (cur_code == Z_EOF)		/* special case for 0-length files */
			goto goteof;
      wr_dchar(k);
      goto loop;
   }

   in_code = cur_code;
   if (cur_code >= free_code) {        /* if code not in table (k<w>k<w>k) */
      cur_code = old_code;             /* previous code becomes current */
      push(fin_char);
   }

   while (cur_code > 255) {               /* if code, not character */
      push(table[cur_code].z_ch);         /* push suffix char */
      cur_code = table[cur_code].next;    /* <w> := <w>.code */
   }

   assert(nbits >= 9 && nbits <= 13);

   k = fin_char = cur_code;
   push(k);
   while (stack_pointer != 0) {
      wr_dchar(pop());
   }
   assert(nbits >= 9 && nbits <= 13);
   ad_dcode();
   old_code = in_code;

   assert(nbits >= 9 && nbits <= 13);

   goto loop;
} /* lzd() */

/* rd_dcode() reads a code from the input (compressed) file and returns
its value. */
unsigned rd_dcode()
{
   register char *ptra, *ptrb;    /* miscellaneous pointers */
   unsigned word;                     /* first 16 bits in buffer */
   unsigned byte_offset;
   char nextch;                           /* next 8 bits in buffer */
   unsigned ofs_inbyte;               /* offset within byte */

   ofs_inbyte = bit_offset % 8;
   byte_offset = bit_offset / 8;
   bit_offset = bit_offset + nbits;

   assert(nbits >= 9 && nbits <= 13);

   if (byte_offset >= INBUFSIZ - 5) {
      int space_left;

#ifdef CHECK_BREAK
	check_break();
#endif

      assert(byte_offset >= INBUFSIZ - 5);
      debug((printf ("lzd: byte_offset near end of buffer\n")))

      bit_offset = ofs_inbyte + nbits;
      space_left = INBUFSIZ - byte_offset;
      ptrb = byte_offset + in_buf_adr;          /* point to char */
      ptra = in_buf_adr;
      /* we now move the remaining characters down buffer beginning */
      debug((printf ("rd_dcode: space_left = %d\n", space_left)))
      while (space_left > 0) {
         *ptra++ = *ptrb++;
         space_left--;
      }
      assert(ptra - in_buf_adr == ptrb - (in_buf_adr + byte_offset));
      assert(space_left == 0);
      if (BLOCKREAD (&in_f, ptra, byte_offset) == -1)
         prterror ('f', "I/O error in lzd:rd_dcode.\n");
      byte_offset = 0;
   }
   ptra = byte_offset + in_buf_adr;
   /* NOTE:  "word = *((int *) ptra)" would not be independent of byte order. */
   word = (unsigned char) *ptra; ptra++;
   word = word | ( ((unsigned char) *ptra) << 8 ); ptra++;

   nextch = *ptra;
   if (ofs_inbyte != 0) {
      /* shift nextch right by ofs_inbyte bits */
      /* and shift those bits right into word; */
      word = (word >> ofs_inbyte) | (((unsigned)nextch) << (16-ofs_inbyte));
   }
   return (word & masks[nbits]); 
} /* rd_dcode() */

void init_dtab()
{
   nbits = 9;
   max_code = 512;
   free_code = FIRST_FREE;
}

void wr_dchar (ch)
int ch;
{
   if (output_offset >= OUTBUFSIZ) {      /* if buffer full */
#ifdef CHECK_BREAK
	check_break();
#endif
      if (BLOCKWRITE (&out_f, out_buf_adr, output_offset) != output_offset)
         prterror ('f', "Write error in lzd:wr_dchar.\n");
      //addbfcrc(out_buf_adr, output_offset);     /* update CRC */
      output_offset = 0;                  /* restore empty buffer */
   }
   assert(output_offset < OUTBUFSIZ);
   out_buf_adr[output_offset++] = ch;        /* store character */
} /* wr_dchar() */

/* adds a code to table */
void ad_dcode()
{
   assert(nbits >= 9 && nbits <= 13);
   assert(free_code <= MAXMAX+1);
   table[free_code].z_ch = k;                /* save suffix char */
   table[free_code].next = old_code;         /* save prefix code */
   free_code++;
   assert(nbits >= 9 && nbits <= 13);
   if (free_code >= max_code) {
      if (nbits < MAXBITS) {
         debug((printf("lzd: nbits was %d\n", nbits)))
         nbits++;
         assert(nbits >= 9 && nbits <= 13);
         debug((printf("lzd: nbits now %d\n", nbits)))
         max_code = max_code << 1;        /* double max_code */
         debug((printf("lzd: max_code now %d\n", max_code)))
      }
   }
}
