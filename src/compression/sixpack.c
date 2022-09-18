// modified by Luigi Auriemma
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define far
#define farmalloc   malloc
#define farfree     free

/********************************************/
/*  SIXPACK.C -- Data compression program   */
/*  Written by Philip G. Gage, April 1991   */
/********************************************/

#include <stdio.h>
//#include <alloc.h>        /* Use <malloc.c> for Power C */

#define TEXTSEARCH 1000   /* Max strings to search in text file */
#define BINSEARCH   200   /* Max strings to search in binary file */
#define TEXTNEXT     50   /* Max search at next character in text file */
#define BINNEXT      20   /* Max search at next character in binary file */
#define MAXFREQ    2000   /* Max frequency count before table reset */ 
#define MINCOPY       3   /* Shortest string copy length */
#define MAXCOPY      64   /* Longest string copy length */
#define SHORTRANGE    3   /* Max distance range for shortest length copy */
#define COPYRANGES    6   /* Number of string copy distance bit ranges */
static short copybits[COPYRANGES] = {4,6,8,10,12,14};   /* Distance bits */

#define CODESPERRANGE (MAXCOPY - MINCOPY + 1)
static int copymin[COPYRANGES], copymax[COPYRANGES];
static int maxdistance, maxsize;
static int distance, insert = MINCOPY, dictfile = 0, binary = 0;

#define NIL -1                    /* End of linked list marker */
#define HASHSIZE 16384            /* Number of entries in hash table */
#define HASHMASK (HASHSIZE - 1)   /* Mask for hash key wrap */
static short far *head, far *tail;       /* Hash table */
static short far *succ, far *pred;       /* Doubly linked lists */
static unsigned char *buffer;            /* Text buffer */

/* Define hash key function using MINCOPY characters of string prefix */
#define getkey(n) ((buffer[n] ^ (buffer[(n+1)%maxsize]<<4) ^ \
                   (buffer[(n+2)%maxsize]<<8)) & HASHMASK)

/* Adaptive Huffman variables */
#define TERMINATE 256             /* EOF code */
#define FIRSTCODE 257             /* First code for copy lengths */
#define MAXCHAR (FIRSTCODE+COPYRANGES*CODESPERRANGE-1)
#define SUCCMAX (MAXCHAR+1)
#define TWICEMAX (2*MAXCHAR+1)
#define ROOT 1
static short left[MAXCHAR+1], right[MAXCHAR+1];  /* Huffman tree */
static short up[TWICEMAX+1], freq[TWICEMAX+1];

/*** Bit packing routines ***/

static int input_bit_count = 0;           /* Input bits buffered */
static int input_bit_buffer = 0;          /* Input buffer */
static int output_bit_count = 0;          /* Output bits buffered */
static int output_bit_buffer = 0;         /* Output buffer */
static long bytes_in = 0, bytes_out = 0;  /* File size counters */

/* Write one bit to output file */
static void output_bit(output,bit)
  FILE *output;
  int bit;
{
  output_bit_buffer <<= 1;
  if (bit) output_bit_buffer |= 1;
  if (++output_bit_count == 8) {
    xputc(output_bit_buffer,output);
    output_bit_count = 0;
    ++bytes_out;
  }
}

/* Read a bit from input file */
static int input_bit(input)
  FILE *input;
{
  int bit;

  if (input_bit_count-- == 0) {
    input_bit_buffer = xgetc(input);
    if (input_bit_buffer == EOF) {
      //printf(" UNEXPECTED END OF FILE\n");
      return(-1);
    }
    ++bytes_in;
    input_bit_count = 7;
  }
  bit = (input_bit_buffer & 0x80) != 0;
  input_bit_buffer <<= 1;
  return(bit);
}

/* Write multibit code to output file */
static void output_code(output,code,bits)
  FILE *output;
  int code,bits;
{
  int i;

  for (i = 0; i<bits; i++) {
    output_bit(output,code & 0x01);
    code >>= 1;
  }
}

/* Read multibit code from input file */
static int input_code(input,bits)
  FILE *input;
  int bits;
{
  int i, bit = 1, code = 0;

  for (i = 0; i<bits; i++) {
    if (input_bit(input)) code |= bit;
    bit <<= 1;
  }
  return(code);
}

/* Flush any remaining bits to output file before closing file */
static void flush_bits(output)
  FILE *output;
{
  if (output_bit_count > 0) {
    xputc((output_bit_buffer << (8-output_bit_count)),output);
    ++bytes_out;
  }
}

/*** Adaptive Huffman frequency compression ***/

/* Data structure based partly on "Application of Splay Trees
   to Data Compression", Communications of the ACM 8/88 */

/* Initialize data for compression or decompression */
static void initialize()
{
  int i, j;

  /* Initialize Huffman frequency tree */
  for (i = 2; i<=TWICEMAX; i++) {
    up[i] = i/2;
    freq[i] = 1;
  }
  for (i = 1; i<=MAXCHAR; i++) {
    left[i] = 2*i;
    right[i] = 2*i+1;
  }

  /* Initialize copy distance ranges */
  j = 0;
  for (i = 0; i<COPYRANGES; i++) {
    copymin[i] = j;
    j += 1 << copybits[i];
    copymax[i] = j - 1;
  }
  maxdistance = j - 1;
  maxsize = maxdistance + MAXCOPY;
}

/* Update frequency counts from leaf to root */
static void update_freq(a,b)
  int a,b;
{
  do {
    freq[up[a]] = freq[a] + freq[b];
    a = up[a];
    if (a != ROOT) {
      if (left[up[a]] == a) b = right[up[a]];
      else b = left[up[a]];
    }
  } while (a != ROOT);

  /* Periodically scale frequencies down by half to avoid overflow */
  /* This also provides some local adaption and better compression */
  if (freq[ROOT] == MAXFREQ)
    for (a = 1; a<=TWICEMAX; a++) freq[a] >>= 1;
}

/* Update Huffman model for each character code */
static void update_model(code)
  int code;
{
  int a, b, c, ua, uua;

  a = code + SUCCMAX;
  ++freq[a];
  if (up[a] != ROOT) {
    ua = up[a];
    if (left[ua] == a) update_freq(a,right[ua]);
    else update_freq(a,left[ua]);
    do {
      uua = up[ua];
      if (left[uua] == ua) b = right[uua];
      else b = left[uua];

      /* If high freq lower in tree, swap nodes */
      if (freq[a] > freq[b]) {
        if (left[uua] == ua) right[uua] = a;
        else left[uua] = a;
        if (left[ua] == a) {
          left[ua] = b; c = right[ua];
        } else {
          right[ua] = b; c = left[ua];
        }
        up[b] = ua; up[a] = uua;
        update_freq(b,c); a = b;
      }
      a = up[a]; ua = up[a];
    } while (ua != ROOT);
  }
}

/* Compress a character code to output stream */
static void sixpack_compress(output,code)
  FILE *output;
  int code;
{
  int a, sp = 0;
  int stack[50];

  a = code + SUCCMAX;
  do {
    stack[sp++] = (right[up[a]] == a);
    a = up[a];
  } while (a != ROOT);
  do {
    output_bit(output,stack[--sp]);
  } while (sp);
  update_model(code);
}

/* Uncompress a character code from input stream */
static int sixpack_uncompress(input)
  FILE *input;
{
  int a = ROOT;

  do {
    if (input_bit(input)) a = right[a];
    else a = left[a];
  } while (a <= MAXCHAR);
  update_model(a-SUCCMAX);
  return(a-SUCCMAX);
}

/*** Hash table linked list string search routines ***/

/* Add node to head of list */
static void add_node(n)  
  int n;
{
  int key;

  key = getkey(n);
  if (head[key] == NIL) {
    tail[key] = n;
    succ[n] = NIL;
  } else {
    succ[n] = head[key];
    pred[head[key]] = n;
  }
  head[key] = n;
  pred[n] = NIL;
}

/* Delete node from tail of list */
static void delete_node(n)
  int n;
{
  int key;

  key = getkey(n);
  if (head[key] == tail[key])
    head[key] = NIL;
  else {
    succ[pred[tail[key]]] = NIL;
    tail[key] = pred[tail[key]];
  }
}

/* Find longest string matching lookahead buffer string */
static int match(n,depth)
  int n,depth;
{
  int i, j, index, key, dist, len, best = 0, count = 0;

  if (n == maxsize) n = 0;
  key = getkey(n);
  index = head[key];
  while (index != NIL) {
    if (++count > depth) break;     /* Quit if depth exceeded */
    if (buffer[(n+best)%maxsize] == buffer[(index+best)%maxsize]) {
      len = 0;  i = n;  j = index;
      while (buffer[i]==buffer[j] && len<MAXCOPY && j!=n && i!=insert) {
        ++len;
        if (++i == maxsize) i = 0;
        if (++j == maxsize) j = 0;
      }
      dist = n - index;
      if (dist < 0) dist += maxsize;
      dist -= len;
      /* If dict file, quit at shortest distance range */
      if (dictfile && dist > copymax[0]) break;
      if (len > best && dist <= maxdistance) {     /* Update best match */
        if (len > MINCOPY || dist <= copymax[SHORTRANGE+binary]) {
          best = len; distance = dist;
        }
      }
    }
    index = succ[index];
  }
  return(best);
}

/*** Finite Window compression routines ***/

#define IDLE 0    /* Not processing a copy */
#define COPY 1    /* Currently processing copy */

/* Check first buffer for ordered dictionary file */
/* Better compression using short distance copies */
static void dictionary()
{
  int i = 0, j = 0, k, count = 0;

  /* Count matching chars at start of adjacent lines */
  while (++j < MINCOPY+MAXCOPY) {
    if (buffer[j-1] == 10) {
      k = j;
      while (buffer[i++] == buffer[k++]) ++count;
      i = j;
    }
  }
  /* If matching line prefixes > 25% assume dictionary */
  if (count > (MINCOPY+MAXCOPY)/4) dictfile = 1;
}

/* Decode file from input to output */
static int decode(input,output)
  FILE *input,*output;
{
  int c, i, j, k, dist, len, n = 0, index;

  initialize();
  buffer = (unsigned char *) malloc(maxsize*sizeof(unsigned char));
  if (buffer == NULL) {
    //printf("Error allocating memory\n");
    return(-1);
  }
  while ((c = sixpack_uncompress(input)) != TERMINATE) {
    if (c < 256) {     /* Single literal character ? */
      xputc(c,output);
      ++bytes_out;
      buffer[n++] = c;
      if (n == maxsize) n = 0;
    } else {            /* Else string copy length/distance codes */
      index = (c - FIRSTCODE)/CODESPERRANGE;
      len = c - FIRSTCODE + MINCOPY - index*CODESPERRANGE;
      dist = input_code(input,copybits[index]) + len + copymin[index];
      j = n; k = n - dist;
      if (k < 0) k += maxsize;
      for (i = 0; i<len; i++) {
        xputc(buffer[k],output);  ++bytes_out;
        buffer[j++] = buffer[k++];
        if (j == maxsize) j = 0;
        if (k == maxsize) k = 0;
      }
      n += len;
      if (n >= maxsize) n -= maxsize;
    }
  }
  free(buffer);
  return(0);
}



int unsixpack(unsigned char *in, int insz, unsigned char *out, int outsz) {

    // necessary or the next runs will fail
    insert = MINCOPY;
    dictfile = 0;
    binary = 0;
    input_bit_count = 0;
    input_bit_buffer = 0;
    output_bit_count = 0;
    output_bit_buffer = 0;
    bytes_in = 0;
    bytes_out = 0;

    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;
    if(decode(NULL,NULL) < 0) return(-1);
    return(outfile - out);
}


