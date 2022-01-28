// modified by Luigi Auriemma
/*

                        -------------------------------
                         Document for the Source Codes
                        -------------------------------

                                   W. Jiang


                    I.   About Jam/Unjam and the source codes
                    II.  Algorithms
                    III. What about other versions of Jam/Unjam
                    IV.  What about LZ77
                    V.   About the author



-------------------------------------------------------------------------------

                    I.   About Jam/Unjam and the source codes

This is the source code of Jam/Unjam May/96, without the header and encryption.

The processing of jamming is as following:



                  |---------------|                  |---------|
  source stream   | enhanced LZ78 |   code stream    |  code   |
 ---------------> |  compression  | ---------------> | refiner | ------|
                  |---------------|                  |---------|       |
                                                                       |
                                                                       |
                                                                       |
              output stream     |----------|     refined code stream   |
          <------------------- | encryptor | <-------------------------|
                                |----------|


Unjam just do the reverse.

The source codes given here omit the encryptor in Jam and decryptor in Unjam.

The first 3 bytes of the output of JAM is the header.  The rest are compressed
data.  The meaning of the header is:

    first byte, the flag byte:
       bit 0 (LSB): set if the file is copied (not compressed) *
       bit 1 : set if file is encrypted
    second and third byte:
       16 bit CRC of the password, if the file is encrypted.

*note:  The Jam will keep an eye on the refined code stream.  When it becomes
longer than the input stream, Jam will stop packing immediately and read in the
input stream again as refined code stream.

The source codes given here omit the header.


-------------------------------------------------------------------------------

                    II.  Algorithms

A.  The enhanced LZ78

A dictionary of 8k (8192) entries is used.  Code length varies according to the
number of entries filled.  When the dictionary is full, all the leaves are
purged and all the holes are moved to the high indexed end of the dictionary.
This gives the remaining nodes smaller indexes and thus shorter code lengths.
The hole entries can be reused to note down new words in the next round.

Although the dictionary is of tree (or forest) structure, recursive procedures
are not used in Jam/Unjam because that take a lot of stack memory.  A property
is introduced to ensure that the algorithm is correct:

We say that the dictionary is "well-formed", if it satisfies the following
terms:

    1.  for any two nodes A and B in the dictionary, if A is the first
        child of B, A has a greater index than B.

    2.  for any two nodes A and B in the dictionary, if A is the younger
        sibling of B, A has a greater index than B.

After the initialization, the dictionary is well-formed.  Notice that after a
new node is added, the dictionary remains well-formed.  So when the dictionary
is full, it keeps the attribute.  In the process of purging leaves and
shrinking dictionary, the dictionary remains well-formed after each loop and
each step as well.  This property helps to form non-recursive algorithms and
lower the time cost of shrinking a dictionary to O(n) where n is the size of
the dictionary.  The result is small, constant memory consumption and fast
speed.


B. The code refiner

Notice that the code output by the enhanced LZ78 doesn't make use of the number
of bits efficiently.  Suppose that 1200 entries are filled in this directory.
The next code output will have 11 bits.  But the possible code value is in
range [0, 1200), not [0, 2048).  That is, less than 11 bits is needed to
represent the code.  Some scheme can be used to take advantage of the wasted
fraction of a bit.

Think of the 'mirror code', for example.  We define that when code range is [0,
1200), 1200 is the mirror code of 1199, 1201 is that of 1198, 1202 that of
1197, ..., etc.  Not all codes have mirror code, usually.  But if a code has a
mirror code, we can reduce the bits of the next code by 1.  For example, if the
MSB (most significant bit) of the next code is 0, the current code is output,
otherwise the mirror code of the current code is output.  In either case, the
next code is output without its MSB.  By this, the code stream output from the
LZ78 is "refined" to yield a higher compression rate.

The code refiner in Jam takes a more complex scheme.  It uses "shift code"
instead of mirror code.  Suppose for the current code c the code range is [0,
next_code), c will have a shift code of next_code+c, if the shift code drops in
the range of [next_code, 1<<code_len).  If the next code is a character (in
range of [0, 256)), the shift code is output and the next code is output in 8
bits.  After that each code is output in a tagged form, that is, a bit of 0
followed by a 8-bit character code or a bit of 1 followed by a normal length
non-character code.  Some other minor skills are implemented to help increasing
the chance of a code to have a shift code.

-------------------------------------------------------------------------------

                    III. What about other versions of Jam/Unjam

It can be observed that the smaller the codes from LZ78 are, the better the
code refiner works.  In Jam/Unjam marked "Aug/96" version, code stream from
LZ78 is slightly modified before entering the code refiner.  If a code c is
within the range of [0, x], we define x-c as its complement.  Two accumulator
is maintained, one for code stream, the other for the complements.  When a code
is to be put into the code refiner, the accumulators are checked: if the one
for codes is no greater than the one for complements, the code is sent to the
refiner.  Otherwise the complement is sent.  After that the code and its
complement is added to the accumulators, respectively.  When one of the
accumulators exceeds some predefined value, they are reduced by half so they
can adapt to the nature of recent code stream.

The outcome of this scheme is much weaker than I had expected.  It gives a mere
0.3% average improvement in compression rate, while the refiner often
contributes nearly 10 percent.

The "Feb/96" version uses a "lazy code length increasing" method.  For example,
when code range is expanded to [0, 2048], code length doesn't increase
automatically to 13.  Instead, the codes are output in 12 bit until a code no
less than 2048 is encountered.  In that case a "expand-length" code is output
before writing the 13-bit code out.  The decoder depends on the "expand-length"
code to increase its read-in code length.

-------------------------------------------------------------------------------

                    IV.  What about LZ77

LZ77 is more straight forward than LZ78.  The decoder of LZ77 can be made very
fast, thought the coder of LZ78 seems faster than that of LZ77.  LZ78 requires
highly "synchronization" between its coder and decoder while LZ77 doesn't.  I
think there must be some algorithms that combine them together.

-------------------------------------------------------------------------------

                    V.   About the author

B.S. software, Fudan University, Shanghai, 1988.
M.S. software engineering, Fudan University, Shanghai, 1991.
Now works for a small local company.
Personal interest: algorithms, network computing, distributes systems, parallel
computing, OSs, compilers or other system software, etc.
I wrote Jam/Unjam in my spare time.
I can be reached via email: wjiang@xmu.edu.cn.

                                --- The End ---
*/

#include <stdio.h>
//#include <alloc.h>
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

#define dic_size     8192
#define base          256
#define nil           256
#define unused          0
#define fst_code      257

typedef struct
{
   unsigned char name;
   int  son;
   int  dad;
}
dic_item;

static dic_item *dic;
static FILE *source, *target;
static long bitbuf=0, bits=0;
static int code_len=9, code, available=fst_code-1;
static int mapped=0, tagged=0, pure_char=0;

static int read_code(void);
static int read_bits(int len);
static unsigned char write_string(int code);
static int shrink();

int unjam(unsigned char *in, int insz, unsigned char *out, int outsz) {
  int  old_code=nil, i;
  unsigned char c;

    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

  dic=(dic_item*)malloc(dic_size*sizeof(dic_item));
  if (dic==NULL) return(-1);

  /* clear the dictionary */
  for (i=0; i<fst_code; i++)
  {
    dic[i].name=i;
    dic[i].son=nil;
    dic[i].dad=nil;
  }

  while (1)
  {
     code=read_code();
     if (code==EOF || code==nil || code>available)
        break;
     if (code<available || old_code==nil)
        c=write_string(code);
     else
     {
        c=write_string(old_code);
        xputc(c, target);
     }
     if (old_code!=nil)
     {
        dic[available].name=c;
        dic[available].son=nil;
        dic[available].dad=old_code;
     }
     available++;
     if (available==(1<<code_len))
        code_len++;
     old_code=code;
     if (available==dic_size)
     {
        shrink();
        while (available<=(1<<(code_len-1))) code_len--;
        available--;
        old_code=nil;
     }
  }
  free(dic);
  //fclose(target);
  //fclose(source);
  return(outfile - out);
}

/* ----------------------- routines ------------------------ */


static int read_code(void)
{ int val;
  if (!tagged)
  { val=read_bits(code_len); if (val==EOF) return EOF;
    if (!mapped)
    { if (val<=available)  /* unshifted */
      { if (val+available+1<1<<code_len)  /* but shiftable */ mapped=1;
        return val;
      }
      val-=available+1; tagged=1; pure_char=1; return val;
    }
    else  /* mapped */
    { mapped=0;
      if (val<=available-base)  /* unshifted */
      { if (val+available+1-base<1<<code_len) mapped=1;
        return val+base;
      }
      val-=available+1-base; tagged=1; pure_char=1; return val+base;
    }
  }
  else  /* tagged */
    if (pure_char)
    { val=read_bits(8); pure_char=0; return val; }
    else
    { int tag=read_bits(1); if (tag==EOF) return EOF;
      if (tag==0)  /* char */
      { val=read_bits(8); return val; }
      else /* tag==1 */
      { tagged=0; val=read_bits(code_len); if (val==EOF) return EOF;
        if (val>available-base)
        { val-=available-base+1; tagged=1; pure_char=1; }
        else
          if (val+available+1-base<1<<code_len) mapped=1;
        return val+base;
      }
    }
}

static int read_bits(int len)
{
   int c;
   while (bits<len)
   {
      c=xgetc(source);
      if (c==EOF)
         return EOF;
      bitbuf=(bitbuf<<8)|c; bits+=8;
   }
   bits-=len;
   return (bitbuf>>bits)&((1<<len)-1);
}

static unsigned char write_string(int codex)
{
   unsigned char c;
   int i=codex;
   while (dic[i].dad!=nil)
   {
      int j=dic[i].dad;
      dic[j].son=i;
      i=j;
   }
   c=dic[i].name;
   /* write out the string */
   do
   {
      int j=dic[i].son; dic[i].son=nil;
      xputc(dic[i].name, target);
      i=j;
   }
   while (i!=nil);
   return c;
}

static int shrink()
{ int i; /* 'dad' will be used as 'next' */

  /* step one: mark leaves, reconstruct the tree structure, and
     transform the tree to well-formed */
  for (i=available-1; i>=fst_code; i--)
  { int j=dic[i].dad;
    if (dic[j].son==nil) dic[j].son=unused;
    if (dic[i].son==nil)
    { dic[i].son=unused; if (i<available) available=i; continue; }
    if (dic[i].son==unused) dic[i].son=nil;
    if (dic[j].son==unused) dic[j].son=nil;
    dic[i].dad/*next*/=dic[j].son; dic[j].son=i;
  }
  for (i=0; i<fst_code; i++) if (dic[i].son==unused) dic[i].son=nil;

  /* step two: move the unused nodes to the end of the array.
  notice that the tree remains well-formed after each loop! */
  i=0;
  while (i<available)
  { int j;
    /* if i has a son behind available, move it to available */
    j=dic[i].son;
    if (j!=nil && j>available)
    { dic[available]=dic[j];
      dic[i].son=available;
      dic[j].son=unused;
      do available++; while (dic[available].son!=unused);
    }

    /* do the same to the brother of i */
    j=dic[i].dad/*next*/;
    if (j!=nil && j>available)
    { dic[available]=dic[j];
      dic[i].dad/*next*/=available;
      dic[j].son=unused;
      do available++; while (dic[available].son!=unused);
    }

    i++;
  }

  /* step three: transform the tree back to its original form */
  for (i=0; i<available; i++)
  { int j=dic[i].son;
    dic[i].son=nil;
    while (j!=nil)
    { int k=dic[j].dad;/*next*/
      dic[j].dad=i;
      j=k;
    }
  }

  return(0);
}
