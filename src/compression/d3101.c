/******************************************************************************
*                                                                             *
*          ######          ######         Copyright (C) 1991                  *
*         ##########    ##########        Advanced Hardware Architectures     *
*        ###########    ###########       P.O. 9669 Moscow, Idaho 83843       *
*       ##### ################ #####      208-883-8000                        *
*      #####   ##############   #####                                         *
*     ################################    Copyright (C) 1991                  *
*    ###############    ###############   Hewlett-Packard Co.                 *
*   #####       ####    ####       #####                                      *
*  #####        ####    ####        #####                                     *
*                                                                             *
*******************************************************************************

*******************************************************************************
*  This code is the property of Advanced Hardware Architectures and may       *
*  not be copied in whole or in part without prior written permission of      *
*  Advanced Hardware Architectures.                                           *
*                                                                             *
******************************************************************************/

/*  Compile with define STAND_ALONE to make d3101.exe as follows:      
        for MS C6.0 use:  cl /AL /D STAND_ALONE d3101.c           */

/* Speed Hacks */

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE			/* Not available for non-GNU C Compilers */
#endif

/* Header Files */

#include <stdio.h>
#if defined(unix) || defined(__osf__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#include <dos.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#endif

/* Macros */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static unsigned short dict[3832][2];

/* mask right most bits according to index value  */
static unsigned short cdmsk[17] =
{0x0000, 0x0001, 0x0003, 0x0007,
 0x000f, 0x001f, 0x003f, 0x007f,
 0x00ff, 0x01ff, 0x03ff, 0x07ff,
 0x0fff, 0x1fff, 0x3fff, 0x7fff,
 0xffff
};

static int      inp_code_len = 9;
static int      inp_len = 0;
static long     inp_code_remainder = 0;
long            inp_byte_cnt = 0;
long            out_byte_cnt = 0;
static int      EOR_flag = 0;
static int      EOF_flag = 0;
static int      ERROR_flag = 0;
static int      FREEZE_flag = 0;

//extern FILE *infile, *outfile;
static unsigned char    *infile   = NULL;
static unsigned char    *outfile  = NULL;
static unsigned char    *infilel  = NULL;
static unsigned char    *outfilel = NULL;
static int xgetc(void *X) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
static int xputc(int chr, void *X) {
    if(outfile >= outfilel) {
        outfile = NULL;
        return(-1);
    }
    *outfile++ = chr;
    return(chr);
}

/****************************************************************************
*                                                                           *
*  Routine:  get_ccode  :  get compressed code from stdin                   *
*                                                                           *
****************************************************************************/

static INLINE int
get_ccode()
{
    int             inp_byte;
    int             cur_code;

    while (inp_len < inp_code_len) {
        if ((inp_byte = xgetc(infile)) < 0) return (EOF);
        inp_byte_cnt++;
        inp_code_remainder |= ((long) inp_byte) << inp_len;
	inp_len += 8;
    }

    inp_len -= inp_code_len;
    cur_code = inp_code_remainder & cdmsk[inp_code_len];
    inp_code_remainder >>= inp_code_len;

    switch (cur_code) {
	  case 0:			/* Freeze dictionary */
		FREEZE_flag = 1;
		break;

      case 1:			/* reset dictionary */
        inp_code_len = 9;
        inp_len = 0;
        inp_code_remainder = 0;
        break;

      case 2:			/* increment codeword size  */
        inp_code_len++;
        if (inp_code_len > 12 ) {  
            fprintf(stderr,"Error: codeword length > 12 at count %d\n", 
                (int)inp_byte_cnt);
            ERROR_flag = 1;
            return 0;
        }
		break;

      case 3:			/* next code is last one */
        inp_len = 0;
        inp_code_remainder = 0;
        break;

      case 4:
      case 5:
      case 6:
      case 7:
        fprintf(stderr,"Error: undefined control code %d at count %d\n",
            cur_code,(int)inp_byte_cnt);
        ERROR_flag = 1;
        break;
      }
      return (cur_code);
}

static unsigned char fifo[128];	/* fifo */

/*****************************************************************************
*                                                                            *
*  Routine:  dcmpr   : to decompress data input from stdin                   *
*                      and output out to stdout                              *
*                                                                            *
*****************************************************************************/
extern void
dcmpr()
{
    register short code;	/* working code */
    register unsigned char *fifop;	/* fifo pointer */
    register unsigned short lcode;	/* last code */
    register unsigned short ncode;	/* next avail code value */
    register unsigned short scode;	/* saved code */
    register unsigned short slen;	/* string length */
    register unsigned short lchr;	/* last char */

    inp_len = 0;
    inp_code_remainder = 0;
    inp_code_len = 9;

	code = get_ccode();		/* get first code  */		
        if (code != 1) {
            fprintf(stderr,"Error: first code is not RESET. code= %d\n",code);
            ERROR_flag = 1;
            return;
        }
        if ( code == EOF || ERROR_flag == 1) return;

  dcmpr0:                       /* RESET   */
    ncode = 0x108;		/* init next avail code = 264 */

    code = get_ccode();		/* get next code */
    if (ERROR_flag == 1) return;
    if (code < 3) goto dcmpr0;	/* skip past control codes */
    if (code > 263) {
        ERROR_flag = 1;
        fprintf(stderr,"Error: illegal codeword %d after EOR or RESET\n",code);
        return;
    }
    if (code == 3) {		/* check for terminator */
        lchr = get_ccode();	/* get last character   */
        if (ERROR_flag == 1) return;
        if (lchr < 8 || lchr > 263) {
            fprintf(stderr,"Error: Last character is invalid %d\n",lchr);
            ERROR_flag = 1;
            return;
        }
        if(xputc(lchr - 8,outfile) < 0) return;
        out_byte_cnt++;
        return;
    }
    lchr = lcode = code;	/* set up for 2nd char */
    if(xputc(lchr - 8,outfile) < 0) return;
    out_byte_cnt++;
    slen = 1;

    do {
        code = get_ccode();	/* get next code */
        if (ERROR_flag == 1) return;
        if (code < 8) {		/* handle control code */
            if (EOR_flag == 1) { 
                fprintf(stderr,"Error: control code %d after EOR\n",code);
                ERROR_flag = 1;
                return;
            }
        switch (code) {
          case 0:		/* Dictionary Frozen (don't care) */
            break;
          case 1:		/* Dictionary Reset  */
            goto dcmpr0;
            break;
          case 2:		/* Increment codeword size : 9,10,11,12  */
            break;
          case 3:		/* End of Record     */
            EOR_flag = 1;
            break;
          case 4:             /* reserved */
          case 5:             /* reserved */
          case 6:             /* reserved */
          case 7:             /* reserved */
            break;
        }
        continue;
    }
    EOF_flag = EOR_flag;	/* if EOF codeword seen, then EOF */
    scode = code;		/* save code */

    fifop = &fifo[128];         /* initialize fifo pointer */

    if (code > ncode) {
        if (EOF_flag) 
            fprintf(stderr,"Error: illegal codeword %d after EOR\n",code);
        else
            fprintf(stderr,"Error: illegal codeword %d\n",code);
        ERROR_flag = 1;
        return;
    }
    if (code == ncode) {	/* check for new code */
        *--fifop = lchr - 8;
        code = lcode;
    }
    while (code >= 0x108) {	/* "un-reverse" string */
        *--fifop = dict[code - 0x108][0] - 8;
        code = dict[code - 0x108][1];
    }
    *--fifop = code - 8;
    lchr = code;

    code = (unsigned short) (&fifo[128] - fifop);	/* save string len */

    while (fifop != &fifo[128]) {	/* output string */
        if(xputc(*fifop++,outfile) < 0) return;
        out_byte_cnt++;
    }

    if (slen < 128 && ncode < 0x1000 && !FREEZE_flag) {	/* add new entry */
        dict[ncode - 0x108][0] = lchr;
        dict[ncode - 0x108][1] = lcode;
        ncode += 1;
    }
    lcode = scode;		/* "remember" last code */
    slen = code;		/*   and length         */
    if (slen > 128){
        fprintf(stderr,"Error: string length greater than 128\n");
        ERROR_flag=1;
        return;
    }
    } while (!EOF_flag);        /*   end of do          */
}

/****************************************************************************
*                                                                           *
*   Main program   ( calls dcmpr )                                          *
*                                                                           *
****************************************************************************/

#ifdef STAND_ALONE
main()
{
    double          ratio;

#ifdef MSDOS
    setmode(0, O_BINARY);
    setmode(1, O_BINARY);
#endif

    /* call dcmpr to decompress stdin to stdout */
    fprintf(stderr, "d3101 - AHA 3101 DCLZ Decompression Demonstration, Copyright 1991.\n");
    dcmpr();
    if (ERROR_flag != 1) {
        ratio = out_byte_cnt ? (double) inp_byte_cnt / out_byte_cnt : 0.0;
        fprintf(stderr, "In, Out, Ratio: %10lu%10lu%12.3f\n",
	    inp_byte_cnt, out_byte_cnt, ratio);
    }
    exit(0);
}
#endif



// some modifications by Luigi Auriemma
// note that the "license" at the beginning of the file is not much clear
// anyway this code was got from BrainVISA / Anatomist
int d3101(unsigned char *in, int insz, unsigned char *out, int outsz) {
    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

    memset(dict, 0, sizeof(dict));

    inp_code_len = 9;
    inp_len = 0;
    inp_code_remainder = 0;
    inp_byte_cnt = 0;
    out_byte_cnt = 0;
    EOR_flag = 0;
    EOF_flag = 0;
    ERROR_flag = 0;
    FREEZE_flag = 0;
    dcmpr();

    if(!outfile || ERROR_flag || FREEZE_flag) return(-1);
    return(outfile - out);
}

