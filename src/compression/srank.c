// modified by Luigi Auriemma
/*..:....|....:....|....:....|....:....|....:....|....:....|....:....|...*/

/*
  Symbol ranking text compressor P M Fenwick 5 September 1996.
  
  V 0.2  9 Sep 1996
         * uses 32-bit words as context,
         * move most globals into code/decode procedures to aid 
            register allocation
         * adds variable coding order (with transmission to decoder)
         * run without parameters displays help messages.
         
         -S reports symbol-rank counts and percentages
         -F reports symbol-index counts from MTF 
         -D specifies number of MTF dither bits
         
  V 0.5  uses 6-bit ASCII subsets to generate contexts.
         Unary coding for short runs
         
  V 1.0  Removes rank=3 symbol to allow shorter codes - negligible change.
    1.1  Changes initialisation of Contexts array (10 Apr 97)
  
  The basis of symbol-ranking compression is that we have a list of symbols
  ordered according to their likelihood in the current context. 
  On compression, each input symbol is translated into its index in that
  list, with the most likely symbol 'Rank-0', the next 'Rank-1', and so on.
  On expansion, the received index is used to get the symbol from the list.
  
  This version is intended for high speed rather than good compression.
  
  It is based on what is really a 4-way set associative cache, with LRU
  update.  The preceding 3-symbol context is hashed and used to select
  a context, which has 3 symbols in LRU order. Each context has
  its own "cache line" -- usually 16--64K contexts are held.
  
  Each word in the "Contexts" array holds the three ranked symbols, from
    Rank-2 (Most Sig) to Rank-0 (least Sig)
  
  For a cache "hit" we encode the LRU position -- rank=0 for most recent 
  through to rank=2 for least recent.
  
  For a miss we could encode the actual symbol, but rather do an approximate
  MTF to favour more frequent symbols.
    
  After coding, each symbol is moved to the front of the LRU list for its context
  
  The complete coding is --
  
      0              rank-0
      10 xxxxx       literal, MTF index < 32
      110            rank-1 
      1110           rank-2
      1111 xxxxxxxx  literal, index >= 32 (also EOF, with code = 0)
      
Short runs of rank-0 are emitted "as is".
Longer runs are emitted as a run-length bit count emitted as a long literal
followed by the length. (Such values cannot occur in a proper literal.)
      
The "pseudo MTF" mechanism has a table of symbols in approximate MTF order.
When one is referenced it is exchanged with one half-way to the front of
the table. A mapping table is included to accelerate the operation, giving 
a constant 6 memory references for any symbol.
A small dither is added to the exchange-position to improve performance.

"Stats" should be 0 for normal use, but 1 to enable extended statistics reports.
Enabling statistics slows execution slightly, but no other effect 
*/

#define Stats 0         /* ########### 0 or 1 to control extended statistics */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <signal.h>


static
unsigned char   *infile   = NULL,
                *infilel  = NULL,
                *outfile  = NULL,
                *outfilel = NULL;
static int xgetc(void *skip) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
static int xputc(int chr, void *skip) {
    if(outfile >= outfilel) return(-1);
    *outfile++ = chr;
    return(chr);
}

#define srName      "srank"

#define srVersion   "1"
#define srLevel     "0"
#define srSuffix    ".sr"

#define outMode "wb"            /* may have to be "w" on some computers */

#define usChar  unsigned char

#if (INT_MAX > 2000000L)        /* an integer of 32 bits */
  #define int32 int
  #define us32  unsigned int
#else
  #define int32 long
  #define us32  unsigned long
#endif

#define runThresh    16         /* encode 0 runs longer than this */

FILE    *inFile,                /* the main input file */
        *outFile;               /* the main output file */

time_t sttTime, endTime;        /* to time the operation */

us32    *Contexts,              /* the table of contexts */
        mask1, mask23;
int     ChToIx[260],            /* for pseudo MTF */
        IxToCh[260];            /*  "   "  "   "  */ 
                
int     Order = 3,		        /* encoding order */
        error = 0;              /* error indicator */
#if Stats
   int  dithMask =  7;          /* mask to dither the MTF index */
#else
  #define dithMask 7
#endif
#define errFinish  1            /* just quietly finish */
#define errRemove  2            /* must remove output file */

#if Stats
  #define logSize    300
  int   Trigger,                /* log from this position */
        logCnt;                 /* symbols left to log */
#endif
        
int32   ctxParam = 15,          /* initially 2^15 contexts */
        currContexts = -1,      /* number of contexts */
        ctxMask,                /* get context index from hash */
        grpBits = 3,            /* sets contexts in group */
        grpMask,
        compBytes, dataBytes;   /* data counters */
        //runLength;              /* rank-0 length */
        
char    Header[20],             /* the file header */
        outSuffix[8],           /* output suffix */
        inName[80], outName[80];        /* file names */

/* ---------- statistics counters ---------- */

#if Stats
  long R0, R1, R2, shortLit, longLit, Runs,
       Ranks[260],
       Lengths[20];
#endif

/* ---------- prototype declarations --------- */

void startoutputtingbits(void);
void doneoutputtingbits(void);
void startinputtingbits(void);

int  checkHeader();
int Initialize();
int decodeFile();
char convByte(int byte);
     
/* ===== Convert a symbol to a printable code (Macintosh specific) ===== */

#if Stats
  char convByte(int byte)
    {
    byte &= 0xff;
    if (byte >= ' ') return byte;      /* normal character */
    if (byte == '\n') return '©';      /* new-line   LF    */
    if (byte ==   0 ) return '¡';      /* zero       NUL   */
    if (byte == '\r') return '¨';      /* return     CR    */
    if (byte == '\t') return 'Æ';      /* hor tab    HT    */
    return '¥';                        /* something < space */
    }
#endif

/* ========== input output routines ========== */

us32    BitBuffer,   /* the bit output buffer */
        BitsInBuf,   /* bits in the buffer */
        theBits,     /* return value from getBits and lookBits */
        Masks[17] = {0, 0x1,    0x3,    0x7,    0xF, 
                        0x1F,   0x3F,   0x7F,   0xFF, 
                        0x1FF,  0x3FF,  0x7FF,  0xFFF, 
                        0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

void startoutputtingbits(void)
    {
    BitBuffer = 0;      /* clear the buffer */ 
    BitsInBuf = 0;      /* and the bit count */
    }

#define putBits(Bits, N)                                        \
    {                                                           \
    BitBuffer = (BitBuffer << N) | Bits;                        \
    BitsInBuf += N;                                             \
    while (BitsInBuf >= 8)      /* write buffer to file */      \
      {                                                         \
      xputc((BitBuffer >> (BitsInBuf-8)), outFile);             \
      BitsInBuf -= 8;                                           \
      BitBuffer &= Masks[BitsInBuf];    /* remaining bits */    \
      compBytes ++;                                             \
      }                                                                                                 \
    }

void doneoutputtingbits(void)
    {
    while (BitsInBuf > 8)       /* process high-order bytes */
      {
      xputc(BitBuffer >> (BitsInBuf - 8), outFile);
      BitsInBuf -= 8;
      compBytes ++;
      }
    if (BitsInBuf > 0)  /* and the low-order bits */
      {
      xputc(BitBuffer << (8-BitsInBuf), outFile);
      compBytes ++;
      }
    }
    
void startinputtingbits(void)
    {
    BitBuffer = 0;      /* clear the buffer */ 
    BitsInBuf = 0;      /* and the bit count */
    }
    
/* ========== read N bits from the input buffer ========== */
/* the value is returned in the global variable 'theBits' */

#define getBits(N)                                              \
    {                                                           \
    while (BitsInBuf < (N))     /* ensure enough bits */        \
      {                                                         \
      BitBuffer = (BitBuffer << 8) | xgetc(inFile);             \
      BitsInBuf += 8;           /* another byte in buff */      \
      compBytes ++;                                             \
      }                                                         \
    theBits = (BitBuffer >> (BitsInBuf - N));  /* align */      \
    BitsInBuf -= N;                     /* N bits taken */      \
    BitBuffer &= Masks[BitsInBuf];      /* select bits */       \
    }                                                                                                           
    
/* ========== inspect N bits from the input buffer ========== */

#define lookBits(N)                                             \
    {                                                           \
    while (BitsInBuf < (N))                                     \
      {                                                         \
      BitBuffer = (BitBuffer << 8) | xgetc(inFile);             \
      BitsInBuf += 8;                                           \
      compBytes ++;                                             \
      }                                                         \
    theBits = BitBuffer >> (BitsInBuf - N);                     \
    }

/* ========== process the file header ========== */
/*
   It has the form "srzp#VLnm", where "srzp" are those letters,
   V is a version number, L is a level number, 
   n is a value indicating the context size, and
   m is the compression order (1, 2 or 3)
*/

/* ====== initialise the coding models etc, and emit values ====== */

int Initialize(int log2Ctx)
    {
    int i, ctxSize;
    
    ctxSize = 1 << log2Ctx;
    if (ctxSize != currContexts)        /* check for changed context size */
      {
      if (Contexts != NULL)
        free(Contexts);                 /* release old space */
      Contexts = (us32 *) calloc(ctxSize+8, sizeof(us32));
      if (Contexts == NULL)             /* failure */
        {
        //fprintf(stderr, "Contexts allocation failure - %d contexts\n",
                       //ctxSize);
        return(-1);
        }
      currContexts = ctxSize;           /* remember number of contexts */
      }
    
    for (i = 0; i < ctxSize; i++)
      Contexts[i] = 0x00652000;         /* initialise to  {NUL, ' ', 'e'} */
      
    ctxMask = (ctxSize-1);    /* get valid context index */
    
    compBytes = dataBytes = 0;

    for (i = 0; i < 256; i++)
      ChToIx[i] = IxToCh[i] = i;        /* set up pseudo MTF table */
            
#if Stats
    R0 = R1 = R2 = shortLit = longLit = Runs = 0;
    for (i = 0; i < 260; i++)
      Ranks[1] = 0;
    for (i = 0; i < 20; i++)
      Lengths[i] = 0;
#endif
    return(0);
    }
    
/* ========== ========== */

/* maintain a sort of MTF list by exchanging a character with one
   half-way to the start of the list.
*/

#define pseudoMTF(currCh, Dither)                                                                       \
    {                                                                                                                           \
    int currIx, exchCh, exchIx;                                                                         \
                                                                                                                                \
    currIx = ChToIx[currCh];    /* position of current character */     \
    exchIx = currIx >> 1;       /* new posn of current character */     \
    exchIx ^= ((Dither) & dithMask);                                    \
    exchCh = IxToCh[exchIx];    /* character to be exchanged */         \
                                                                        \
    ChToIx[currCh] = exchIx;    /* new position of current char */      \
    IxToCh[exchIx] = currCh;                                            \
    ChToIx[exchCh] = currIx;    /* new posn of exchange char */         \
    IxToCh[currIx] = exchCh;                                            \
    }
    
/* ===== code for ISO Fletcher checksum ===== */
    
#define addCheckSum(C)                                          \
    {                                                           \
    check1 += C;                /* add character into check1 */ \
    if (check1 >= 255)          /* ... modulo 255 */            \
      check1 -= 255;                                            \
    check2 += check1;           /* add check1 into check2 */    \
    if (check2 >= 255)          /* ... modulo 255 */            \
      check2 -= 255;                                            \
    }
    
/* ========== expand a file ========== */
    
int decodeFile()
    {    
    int headVal,                /* value from header record */
        rxCheck1=0, rxCheck2=0; /* checksums from trailer */

    int SymIx,                  /* index in pseudo MTF table */
        symbol,                 /* THE decoded symbol */
        check1, check2;         /* ISO checksums */
#if Stats
   int  code;
   char rank;
#endif

   us32 W,                      /* the selected context */
        prev1, prev2, prev3,    /* hash codes of prev symbols */
        runLength,
        Hx;                     /* and its index */
    
    prev1 = prev2 = prev3 = runLength = 0;
    check1 = check2 = 0;
    
    sttTime = clock();
    startinputtingbits();       /* start input */
    //headVal = checkHeader();    /* check header */
    headVal = 15;
    Initialize(headVal);

    for (;;)                    /* start of main decoding loop */
      {
#if Stats
     if (dataBytes == Trigger)
       logCnt = logSize;                /* start logging */
     SymIx = -1;
#endif

      Hx = (prev1 | prev2 | prev3) & mask23 & ctxMask;
      W = Contexts[Hx];         /* select context  */

      if (runLength > 0)
        {
        symbol = W & 0xFF;       /* rank-0 symbol, in run */
        runLength --;
#if Stats
        R0++;
        rank = '0';
        code = 0;
#endif
        }
      else
        {
        lookBits(4);                    /* get code into 'theBits' */
        if ((theBits & 8) == 0)         /* 0xxx = Rank-0 */
          {
          getBits(1);
          symbol = W & 0xFF;            /* rank-0 symbol */
#if Stats
          R0++;
          rank = '0';
          code = 0;
#endif
          }
        else if ((theBits & 12) == 8)   /* 10xx = Short literal */
          {
          getBits(7);                   /* 2 prefix & 5 data */
          SymIx  = theBits & 0x1F;      /* remove prefix */
          symbol = IxToCh[SymIx];       /* get symbol from index */
          Contexts[Hx] = (W << 8) | symbol; /* force as most recent symbol */
#if Stats
          shortLit++;
          Ranks[SymIx] ++;
          code = 10;
          rank = 'S';
#endif
          }
        else if ((theBits & 14) == 12)  /* 110x = Rank-1 */
          {
          getBits(3);
          symbol = (W >> 8) & 0xFF;     /* fetch the rank-1 symbol */
          Contexts[Hx] =  (W & 0xFFFF0000) |
                         ((W & 0xFF) << 8) |
                         symbol;
#if Stats
          R1++;
          rank = '1';
          code = 110;
#endif
          }
        else if ((theBits & 15) == 14)  /* 1110 = Rank-2 */
          {
          getBits(4);
          symbol = (W >> 16) & 0xFF;     /* fetch the rank-2 symbol */
          Contexts[Hx] =  (W & 0xFF000000) |
                         ((W & 0xFFFF) << 8) |
                         symbol;
#if Stats
          R2++;
          rank = '2';
          code = 1110;
#endif
          }
        else if (theBits == 15)         /* 1111 = long Literal */
          {
          getBits(12);                  /* get code & literal */
          SymIx = theBits & 0xFF;       /* remove prefix code */
          if (SymIx == 0)               /* end of file */
            {
            getBits(8);                 /* get the checksums */
            rxCheck1 = theBits;
            getBits(8);
            rxCheck2 = theBits;
            if ((check1 != rxCheck1) || (check2 != rxCheck2))   /* final check */
              {
              fprintf(stderr, "File '%s' - invalid checksum\n", inName);
              error = errRemove;
              }
            break;
            }
          else if (SymIx < 32)          /* encoded run */
            {
            getBits(SymIx);             /* SymIx has bit-length of run-length */
            runLength = theBits + runThresh - 1; /* get length & adjust */
            symbol = W & 0xff;          /* first symbol of run */
#if Stats
            Runs++;
            if (logCnt > 0)
              fprintf(stderr, "    Run %d\n", runLength + 1);
#endif
            }
          else                          /* long literal */
            {
            symbol = IxToCh[SymIx];
            Contexts[Hx] = (W << 8) | symbol;
#if Stats
            longLit++;
            rank = 'L';
            Ranks[SymIx] ++;
            code = 11111;
#endif
            }
          }
        else
          {
          fprintf(stderr, "File '%s' -- decode failure - code = %ld, dataBytes = %ld\n",
                inName, theBits, dataBytes);
          error = errRemove;
          break;
          }
        } /* end of main decode */
     
#if Stats
      if (logCnt-- > 0)
        {
        fprintf(stderr, "%7ld < %-7ld '%c' %02X  %c\"%c%c%c\"  rank %c, code %-6d", 
                        dataBytes, compBytes, convByte(symbol), symbol,
                        convByte(W>>24),
                        convByte(W), convByte(W>>8), convByte(W>>16),
                        rank, code);
        if (SymIx >= 0)
          fprintf(stderr, " %3d", SymIx);
        fprintf(stderr, "\n");
        }
#endif

      dataBytes++;
      pseudoMTF(symbol, dataBytes);     /* update pseudo MTF table */
      prev3 = (prev2 << 6);             /* move along hash codes */
      prev2 = ((prev1 & 0x3F) << 6);
      prev1 = symbol & mask1;           /* and get new one */
      addCheckSum(symbol);              /* build check sum */
      xputc(symbol,outFile);            /* write symbol to file */
      } /* end of decompressing a symbol */
      
    endTime = clock();
    //fclose(outFile);
    if ((error & errRemove) != 0)
      remove(outName);
    return(0);
    }
   
/* =================================================== */

int unsrank(unsigned char *in, int insz, unsigned char *out, int outsz) {
    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

    error = 0;
    Order = 3;
    mask1  = 0x3F;                      /* order 3 -- 18 bits */
    mask23 = 0x3FFFF;
#if Stats
    Trigger = -1;
    dithMask = 7;                       /* 3 bits */
#endif    
          if(decodeFile() < 0) return(-1);
#if Stats
        logCnt = 0;
        Trigger = -1;                   /* reset the logging threshold */
#endif

    return(outfile - out);
}
