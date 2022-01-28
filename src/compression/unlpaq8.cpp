// modified by Luigi Auriemma

// http://www.cs.fit.edu/~mmahoney/compression/

/*	lpaq8.cpp file compressor, September 29, 2007.
(C) 2007, Matt Mahoney, matmahoney@yahoo.com
	  Alexander Ratushnyak, artest@inbox.ru

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.

To compress:   N input output  (N=0..9, uses 3 + 3*2^N MB memory)
To decompress: d input output  (requires same memory)

For example:

  lpaq8 9 foo foo.lpq

compresses foo to foo.lpq using 1.5 GB memory.

  lpaq8 d foo.lpq foo

decompresses to foo, also using 1.5 GB memory.  Option 0 uses
6 MB memory.  The maximum file size is 2 GB.

DESCRIPTION OF LPAQ1:

lpaq1 is a "lite" version of PAQ, about 35 times faster than paq8l
at the cost of some compression (but similar to high end BWT and
PPM compressors).  It is a context-mixing compressor combining 7
contexts: orders 1, 2, 3, 4, 6, a lowercase unigram word context
(for ASCII text), and a "match" order, which predicts the next
bit in the last matching context.  The independent bit predictions of
the 7 models are combined using one of 80 neural networks (selected by
a small context), then adjusted using 2 SSE stages (order 0 and 1)
and arithmetic coded.

Prediction is bitwise.  This means that an order-n context consists
of the last n whole bytes plus any of the 0 to 7 previously coded
bits of the current byte starting with the most significant bit.
The unigram word context consists of a hash of the last (at most) 11
consecutive letters (A-Z, a-z) folded to lower case.  The context
does not include any nonalphabetic characters nor any characters
preceding the last nonalphabetic character.

The first 6 contexts (orders 1..4, 6, word) are used to index a
hash table to look up a bit-history represented by an 8-bit state.
The representable states are the same as in paq8l.  A state can
either represent all histories up to 4 bits long, or a pair of
0,1 counts plus a flag to indicate the most recent bit.  The counts
are bounded by (41,0), (40,1), (12,2), (5,3), (4,4) and likewise
for 1,0.  When a count is exceeded, the opposite count is reduced to
approximately preserve the count ratio.  The last bit flag is present
only for states whose total count is less than 16.  There are 253
possible states.

A bit history is mapped to a probability using an adaptive table
(StateMap).  This differs from paq8l in that each table entry includes
a count so that adaptation is rapid at first.  Each table entry
has a 22-bit probability (initially p = 0.5) and 10-bit count (initially
n = 0) packed into 32 bits.  After bit y is predicted, n is incremented
up to the limit (1023) and the probability is adjusted by
p := p + (y - p)/(n + 0.5).  This model is stationary:
p = (n1 + 0.5)/(n + 1), where n1 is the number of times y = 1 out of n.

The "match" model (MatchModel) looks up the current context in a
hash table, first using a longer context, then a shorter one.  If
a match is found, then the following bits are predicted until there is
a misprediction.  The prediction is computed by mapping the predicted
bit, the length of the match (1..15 or quantized by 4 in 16..62, max 62),
and the last whole byte as context into a StateMap.  If no match is found,
then the order 0 context (last 0..7 bits of the current byte) is used
as context to the StateMap.

The 7 predictions are combined using a neural network (Mixer) as in
paq8l, except it is a single level network without MMX code.  The
inputs p_i, i=0..6 are first stretched: t_i = log(p_i/(1 - p_i)),
then the output is computed: p = squash(SUM_i t_i * w_i), where
squash(x) = 1/(1 + exp(-x)) is the inverse of stretch().  The weights
are adjusted to reduce the error: w_i := w_i + L * t_i * (y - p) where
(y - p) is the prediction error and L ~ 0.002 is the learning rate.
This is a standard single layer backpropagation network modified to
minimize coding cost rather than RMS prediction error (thus dropping
the factors p * (1 - p) from learning).

One of 80 neural networks are selected by a context that depends on
the 3 high order bits of the last whole byte plus the context order
(quantized to 0, 1, 2, 3, 4, 6, 8, 12, 16, 32).  The order is
determined by the number of nonzero bit histories and the length of
the match from MatchModel.

The Mixer output is adjusted by 2 SSE stages (called APM for adaptive
probability map).  An APM is a StateMap that accepts both a discrte
context and an input probability, pr.  pr is stetched and quantized
to 24 levels.  The output is interpolated between the 2 nearest
table entries, and then only the nearest entry is updated.  The entries
are initialized to p = pr and n = 6 (to slow down initial adaptation)
with a limit n <= 255.  The APM differs from paq8l in that it uses
the new StateMap rapid initial adaptation, does not update both
adjacent table entries, and uses 24 levels instead of 33.  The two
stages use a discrete order 0 context (last 0..7 bits) and a hashed order-1
context (14 bits).  Each output is averaged with its input weighted
by 1/4.

The output is arithmetic coded.  The code for a string s with probability
p(s) is a number between Q and Q+p(x) where Q is the total probability
of all strings lexicographically preceding s.  The number is coded as
a big-endian base-256 fraction.  A header is prepended as follows:

- "pQ" 2 bytes must be present or decompression gives an error.
- 1 (0x01) version number (other values give an error).
- memory option N as one byte '0'..'9' (0x30..0x39).
- file size as a 4 byte big-endian number.
- arithmetic coded data.

Two thirds of the memory (2 * 2^N MB) is used for a hash table mapping
the 6 regular contexts (orders 1-4, 6, word) to bit histories.  A lookup
occurs every 4 bits.  The input is a byte-oriented context plus possibly
the first nibble of the next byte.  The output is an array of 15 bit
histories (1 byte each) for all possible contexts formed by appending
0..3 more bits.  The table entries have the format:

 {checksum, "", 0, 1, 00, 10, 01, 11, 000, 100, 010, 110, 001, 101, 011, 111}

The second byte is the bit history for the context ending on a nibble
boundary.  It also serves as a priority for replacement.  The states
are ordered by increasing total count, where state 0 represents the
initial state (no history).  When a context is looked up, the 8 bit
checksum (part of the hash) is compared with 3 adjacent entries, and
if there is no match, the entry with the lowest priority is cleared
and the new checksum is stored.

The hash table is aligned on 64 byte cache lines.  A table lookup never
crosses a 64 byte address boundary.  Given a 32-bit hash h of the context,
8 bits are used for the checksum and 17 + N bits are used for the
index i.  Then the entries i, i XOR 1, and i XOR 2 are tried.  The hash h
is actually a collision-free permuation, consisting of multiplying the
input by a large odd number mod 2^32, a 16-bit rotate, and another multiply.

The order-1 context is mapped to a bit history using a 64K direct
lookup table, not a hash table.

One third of memory is used by MatchModel, divided equally between
a rotating input buffer of 2^(N+19) bytes and an index (hash table)
with 2^(N+17) entries.  Two context hashes are maintained, a long one,
h1, of length ceil((N+17)/3) bytes and a shorter one, h2, of length
ceil((N+17)/5) bytes, where ceil() is the ceiling function.  The index
does not use collision detection.  At each byte boundary, if there is
not currently a match, then the bytes before the current byte are
compared with the location indexed by h1.  If less than 2 bytes match,
then h2 is tried.  If a match of length 1 or more is found, the
match is maintained until the next bit mismatches the predicted bit.
The table is updated at h1 and h2 after every byte.

To compile (g++ 3.4.5, upx 3.00w):
  g++ -Wall lpaq1.cpp -O2 -Os -march=pentiumpro -fomit-frame-pointer
      -s -o lpaq1.exe
  upx -qqq lpaq1.exe

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>
#define NDEBUG  // remove for debugging
#include <assert.h>

// 8, 16, 32 bit unsigned types (adjust as appropriate)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

#ifdef WIKI
#define FB_SIZE 8192
#define TOLIMIT_10 400
#define TOLIMIT_11 576
#define TOLIMIT_12 896
#define TOLIMIT_13 1023
#define TOLIMIT_14 1023
#define TOLIMIT_15 1023
#define TOLIMIT_16 256
#define TOLIMIT_2a 1023
#define TOLIMIT_2b 1023
#define SQUARD 0
#define MIN_LEN 3
#define MXR_MUL 1
#define MXR_UPD 11
#define MXR_INIT 0x0c00
#define PW_MUL 241
#define P5_MUL 47
#define add2order0 (c>>4)
#define add2order4 add2order+MI*10
#define upd3_cp\
	cp[2]=t1a.get0(hash7(c0*23-(c4&0xc0ffffff)*16381 ));	/*  3.25 */\
	cp[3]=t2a.get0(hash8(c0   -c4*509+(c8&0xc0ff)*19991));	/*  5.25 */\
	cp[4]=t3b.get0(hash9(c0*31-c4*59-c8*2039));		/*  8.00 */
#define upd7_cp\
	cp[2]=t1b.get1(hash2(c4&0xffffff));			/* order 3   */\
	cp[3]=t2b.get1(hash3(c4*127+((c8>>4)&15) ));		/* order 4.5 */\
	cp[4]=t3a.get1(hash4(c4*197-(c8&0xfcffff)*63331));	/* order 6.75*/
#define upd7_h123\
    int d=c;		\
    if (d>127) d=127;	\
    if (h[5]) d+=128;	\
    h[0]=d<<8;		/* order 1 */\
    h2=hash1a(c4*1021+c8*59999)&HN;\
    h1=hash3a(c4*71-c8*127+cc*79)&HN;

// 79 19991 251 223 2039 -- vsego 12, plus 1 - xchg in h2

#else
#define FB_SIZE 32768
U32	TOLIMIT_10;
#define TOLIMIT_11 320
#define TOLIMIT_12 240
#define TOLIMIT_13 140
#define TOLIMIT_14 124
#define TOLIMIT_15 124
#define TOLIMIT_16 112
#define TOLIMIT_2a 384
#define TOLIMIT_2b 680
#define SQUARD 2
#define MIN_LEN 2
#define MXR_MUL 341/1024
#define MXR_UPD 8
#define MXR_INIT 0x0c00
#define PW_MUL 509
#define P5_MUL 37
#define add2order0 ( (c>>6) + ((c4>>4)&12) )
#define add2order4 mxr_wx4 + MI*10*8*(((c4>>6)&3)+(c0&12))
#define get0 get
#define get1 get
	int calcprevfail[256];
#define upd3_cp\
	cp[2]=t1a.get(hash7(c0*23-(c4&0xffffff)*251));\
	cp[3]=t2a.get(hash8(c0   -c4*59));\
	cp[4]=t3b.get(hash9(c0*31-c4*197+(c8& 0xffff)*63331 ));
#define upd7_cp\
	cp[2]=t1b.get(hash2(c4&0xffffff));\
	cp[3]=t2b.get(hash3(c4*127));\
	cp[4]=t3a.get(hash4(c4*197-(c8& 0xffff)*59999));
#define upd7_h123\
    h[0]=c<<8;		/* order 1 */\
    t0c1=t0+( (c4&240) + ((c4>>12)&12) + ((c4>>22)&3)  )*256;\
    prevfail=calcprevfail[fails];\
    fails=1;\
    h1=h1*(7 <<1)-c-1&HN;\
    h2=(c4*1021-c8*2039+cc*421)&HN;\
    h3=hash3a(c4*59+c8*383)&HN;
#endif


/*typedef*/ enum {DEFAULT, TEXT, EXE };

U32 method=DEFAULT, mem_usage=0;

U32 fb_done=0;
U8 file_buf[FB_SIZE+4], *fb_pos=&file_buf[0], *fb_stop=&file_buf[FB_SIZE+4], *fb_len;
unsigned char *uncompressed;

int DP_SHIFT=14;
///int E8E9_codesize=0;


#ifdef WIKI
#define E8E9_encode(a,b) ;
#define E8E9_decode(a,b) ;
#else
inline void E8E9_encode(U8 *p, int i) {
    int f;
    for (; i>0; ++p, --i) {
	if ( (*p & 0xfe)==0xe8 ) {
		f=*(U32*)(p+1);
		if (  (f&0xffc00000)==0 || (f&0xffc00000)==0xffc00000 ) {
		    int a=(f+(fb_done+p-&file_buf[0])) << 9;
		    *(U32*)(p+1)=a >> 9;  // This is NOT big-endian-compatible
		}
	}
    }
    fb_done+=FB_SIZE;
}
inline void E8E9_decode(U8 *p, int i) {
    int f;
    for (; i>0; --p, --i) {
	if ( (*p & 0xfe)==0xe8 ) {
		f=*(U32*)(p+1);
		if (  (f&0xffc00000)==0 || (f&0xffc00000)==0xffc00000 ) {
		    int a=(f-(fb_done+p-&file_buf[0])) << 9;
		    *(U32*)(p+1)=a >> 9;  // This is NOT big-endian-compatible
		}
	}
    }
    fb_done+=FB_SIZE;
}
#endif
/*
inline int do_getc() {
  if (fb_pos<fb_stop) return *fb_pos++;
  if (fb_stop!=&file_buf[FB_SIZE] || fb_stop==fb_len) return EOF;

  fb_pos=&file_buf[0];
  *(U32*)fb_pos = *(U32*)fb_stop;

  fb_stop=fb_len-FB_SIZE;
  if (fb_stop==&file_buf[4]) {
	fb_len=fb_stop + fread(&file_buf[4], 1, FB_SIZE, uncompressed);
	if (method==EXE) E8E9_encode(fb_pos, fb_len-8-fb_pos);
	fb_stop=fb_len;	if (fb_stop>&file_buf[FB_SIZE]) fb_stop=&file_buf[FB_SIZE];
  }
  return *fb_pos++;
}
*/
inline void do_putc(U8 c) {
  if (fb_pos>=fb_stop) {
	if (method==EXE) E8E9_decode(&file_buf[FB_SIZE-1-4], FB_SIZE-4);
	//fwrite(&file_buf[0], 1, FB_SIZE, uncompressed);
    memcpy(uncompressed, &file_buf[0], FB_SIZE);
    uncompressed += FB_SIZE;

	fb_pos=&file_buf[4];
	*(U32*)(fb_pos-4) = *(U32*)(fb_stop-4);
  }
  *fb_pos++=c;
}

inline void do_putc_end() {
  int LEN=fb_pos-&file_buf[0];
  if (method==EXE) E8E9_decode(&file_buf[LEN-1-4], LEN-4);
  //fwrite(&file_buf[0], 1, LEN, uncompressed);
  memcpy(uncompressed, &file_buf[0], LEN);
  uncompressed += LEN;
}



// Error handler: print message if any, and exit
void quit(const char* message=0) {
  if (message) printf("%s\n", message);
  exit(1);
}

// Create an array p of n elements of type T
template <class T> void alloc(T*&p, int n) {
  p=(T*)calloc(n, sizeof(T));
  mem_usage+=n*sizeof(T);
  if (!p) quit("out of memory");
}

///////////////////////////// Squash //////////////////////////////

int squash_t[4096];	//initialized when Encoder is created

#define squash(x)  squash_t[(x)+2047]

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
int squash_init(int d) {
  if (d==2047) return 4095;
  if (d==-2047) return 0;
  double k=4096/(double(1+exp(-(double(d)/256))));
  return int(k);
}

//////////////////////////// Stretch ///////////////////////////////

// Inverse of squash. stretch(d) returns ln(p/(1-p)), d scaled by 8 bits,
// p by 12 bits.  d has range -2047 to 2047 representing -8 to 8.
// p has range 0 to 4095 representing 0 to 1.

  static int stretch_t[4096];	//initialized when Encoder is created
  static int stretch_t2[4096];
  static int dt[1024];	// i -> 16K/(i+3)
  static int dta[1024]; // i ->  8K/(i+3)
  static int calcfails[8192];   //as above, initialized when Encoder is created

  static int y20;   // y<<20
  static int c0=1;  // last 0-7 bits with leading 1
  static int bcount=7;  // bit count
  static U8* buf;	// input buffer
  static int pos=0;	// number of bytes in buf
  static int c4=0;  // last 4 bytes
  static int c1=0;  // last two higher 4-bit nibbles

///////////////////////// state table ////////////////////////

// State table:
//   nex(state, 0) = next state if bit y is 0, 0 <= state < 256
//   nex(state, 1) = next state if bit y is 1
//
// States represent a bit history within some context.
// State 0 is the starting state (no bits seen).
// States 1-30 represent all possible sequences of 1-4 bits.
// States 31-252 represent a pair of counts, (n0,n1), the number
//   of 0 and 1 bits respectively.  If n0+n1 < 16 then there are
//   two states for each pair, depending on if a 0 or 1 was the last
//   bit seen.
// If n0 and n1 are too large, then there is no state to represent this
// pair, so another state with about the same ratio of n0/n1 is substituted.
// Also, when a bit is observed and the count of the opposite bit is large,
// then part of this count is discarded to favor newer data over old.

static const U8 State_table[512*5]={
  1,  3,  4,  7,  8,  9, 11, 15, 16, 17, 18, 20, 21, 22, 26, 31, 32, 32, 32, 32,
 34, 34, 34, 34, 34, 34, 36, 36, 36, 36, 38, 41, 42, 42, 44, 44, 46, 46, 48, 48,
 50, 53, 54, 54, 56, 56, 58, 58, 60, 60, 62, 62, 50, 67, 68, 68, 70, 70, 72, 72,
 74, 74, 76, 76, 62, 62, 64, 83, 84, 84, 86, 86, 44, 44, 58, 58, 60, 60, 76, 76,
 78, 78, 80, 93, 94, 94, 96, 96, 48, 48, 88, 88, 80,103,104,104,106,106, 62, 62,
 88, 88, 80,113,114,114,116,116, 62, 62, 88, 88, 90,123,124,124,126,126, 62, 62,
 98, 98, 90,133,134,134,136,136, 62, 62, 98, 98, 90,143,144,144, 68, 68, 62, 62,
 98, 98,100,149,150,150,108,108,100,153,154,108,100,157,158,108,100,161,162,108,
110,165,166,118,110,169,170,118,110,173,174,118,110,177,178,118,110,181,182,118,
120,185,186,128,120,189,190,128,120,193,194,128,120,197,198,128,120,201,202,128,
120,205,206,128,120,209,210,128,130,213,214,138,130,217,218,138,130,221,222,138,
130,225,226,138,130,229,230,138,130,233,234,138,130,237,238,138,130,241,242,138,
130,245,246,138,140,249,250, 80,140,253,254, 80,140,253,254, 80,

  2,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
 80, 82, 84, 86, 88, 90, 92, 94, 96, 98,100,102,104,106,108,110,112,114,116,118,
120,122,124,126,128, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,
 96, 98,100,102,104,106,108,110,112,114,116,118,120,122,124, 94,130, 66, 68, 70,
 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98,100,102,104,106,108,110,
112,114,116,118,120,122,124,126,132,124,134, 92,136,124,138, 92,140,124,142, 92,
144,124,146, 92,148,124,150, 92,152,124,154, 92,156,124,158, 92,160,124,162, 92,
164,124,166, 92,168,120,170, 92,172,120,174, 92,176,120,178, 92,180,120,182, 92,
184,120,186, 92,188,120,190, 92,192,120,194, 88,196,120,198, 88,200,120,202, 88,
204,120,206, 88,208,120,210, 88,212,120,214, 88,216,120,218, 88,220,120,222, 88,
224,120,226, 88,228,120,230, 88,232,120,234, 88,236,120,238, 88,240,120,242, 88,
244,120,246, 88,248,120,250, 88,252,120,254, 88,252,120,254, 88,

  2,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
 80, 82, 84, 86, 88, 90, 92, 94, 96, 98,100,102,104,106,108,110,112,114,116,118,
120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,
160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,
200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,
240,242,244,246,248,250,252,254,128,130,132,134,136,138,140,142,144,146,148,150,
152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,
192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,
232,234,236,238,240,242,244,246,248,250,252,190,192,130,132,134,136,138,140,142,
144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,
184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,
224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254,

  1, 16, 17, 18, 38, 40, 42, 70, 46, 63, 95,195,159,191,223,240,  2, 32, 33, 34,
 39, 41, 43, 45, 47, 79,111,143,175,207,225,241,  3, 48, 49, 50, 36, 52, 53, 54,
 55, 53, 57, 58, 59, 60, 61, 62, 19, 35, 36, 66, 67, 64, 54, 70, 71, 72, 73, 74,
 75, 76, 77, 78, 68, 69, 37, 82, 83, 84, 80, 81, 87, 88, 89,  6, 91, 92, 93, 94,
 85, 86,  4, 98, 99,100,101, 96, 97,104,105,106,107,108,109,110,102,103, 20,114,
115,116,117,118,112,113,  6,122,123,124,125,126,119,120,  5,130,131, 70,133,134,
135,128, 97,138,139,140,141,157,136,152, 21,146,147,148,149,150,151,152, 97,145,
155,156,157,158,168,154,  6,162,163,164,165,166,167,168,149,160,161,172,173,174,
170,171, 22,178,179,180,181,182,183,199,185,186,176,177,189,190,187,188,  7,194,
195,196,197,198,199,200,201,202,195,192,226,206,204,220, 23,210,211,212,213,214,
149,216,217,233,219,220,208,209,221,222,  8,226,227,228,229,245,231,232,233,234,
235,236,226,224,238,239, 24,242,243,244,245,246,247,248,249,250,251, 14,253,254,
255,242,  9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31,

#ifdef WIKI
  2,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 34, 36, 38, 40, 42, 44, 46,
 48, 50, 52, 54, 56, 58, 60, 46, 66, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54,
 56, 58, 60, 62, 68, 60, 70, 44, 72, 60, 74, 44, 76, 60, 78, 44, 80, 60, 82, 44,
 84, 60, 86, 44, 88, 60, 90, 44, 92, 60, 94, 44, 96, 60, 98, 44,100, 60,102, 44,
104, 56,106, 44,108, 56,110, 44,112, 56,114, 44,116, 56,118, 44,120, 56,122, 44,
124, 56,126, 40,128, 56,130, 40,132, 56,134, 40,136, 56,138, 40,140, 56,142, 40,
144, 56,146, 40,148, 56,150, 40,152, 56,154, 40,156, 56,158, 40,160, 56,162, 40,
164, 56,166, 40,168, 56,170, 40,172, 56,174, 40,176, 56,178, 40,180, 56,182, 40,
184, 56,186, 40,188, 56,190, 40,192, 56,194, 40,196, 56,198, 40,200, 56,202, 40,
204, 56,206, 40,208, 56,210, 40,212, 56,214, 40,216, 56,218, 40,220, 56,222, 40,
224, 56,226, 40,228, 56,230, 40,232, 56,234, 40,236, 56,238, 40,240, 56,242, 40,
244, 56,246, 40,248, 56,250, 40,252, 56,254, 40,252, 56,254, 40,

#else
  2, 16,  4, 18,  6, 20,  8, 22, 10, 24, 12, 26, 14, 28,128, 30, 18, 32, 20, 34,
 22, 36, 24, 38, 26, 40, 28, 42, 30, 44,130, 46, 34, 48, 36, 50, 38, 52, 40, 54,
 42, 56, 44, 58, 46, 60, 24, 62, 50, 64, 52, 66, 54, 68, 56, 70, 58, 72, 60, 74,
 62, 76, 40, 78, 66, 80, 68, 82, 70, 84, 72, 86, 74, 88, 76, 90, 78, 92, 40, 94,
 82, 96, 84, 98, 86,100, 88,102, 90,104, 92,106, 94,108, 56,110, 98,112,100,114,
102,116,104,118,106,120,108,122,110,124, 56,126,114,112,116,114,118, 66,120, 66,
122, 68,124, 68,126, 70, 72, 70,132,112,134,114,136,112,138,114,140,112,142,114,
144,112,146,114,148,112,150,114,152,112,154,114,156,112,158,114,160,112,162,114,
164,112,166,114,168,112,170,114,172,112,174,114,176,112,178,114,180,112,182,114,
184,112,186,114,188,112,190,114,192,112,194,114,196,112,198,114,200,112,202,114,
204,112,206,114,208,112,210,114,212,112,214,114,216,112,218,114,220,112,222,114,
224,112,226,114,228,112,230,114,232,112,234,114,236,112,238,114,240,112,242,114,
244,112,246,114,248,112,250,114,252,112,254,114,252,112,254,114,
#endif


  2,  5,  6, 10, 12, 13, 14, 19, 23, 24, 25, 27, 28, 29, 30, 33, 35, 35, 35, 35,
 37, 37, 37, 37, 37, 37, 39, 39, 39, 39, 40, 43, 45, 45, 47, 47, 49, 49, 51, 51,
 52, 43, 57, 57, 59, 59, 61, 61, 63, 63, 65, 65, 66, 55, 57, 57, 73, 73, 75, 75,
 77, 77, 79, 79, 81, 81, 82, 69, 71, 71, 73, 73, 59, 59, 61, 61, 49, 49, 89, 89,
 91, 91, 92, 69, 87, 87, 45, 45, 99, 99,101,101,102, 69, 87, 87, 57, 57,109,109,
111,111,112, 85, 87, 87, 57, 57,119,119,121,121,122, 85, 97, 97, 57, 57,129,129,
131,131,132, 85, 97, 97, 57, 57,139,139,141,141,142, 95, 97, 97, 57, 57, 81, 81,
147,147,148, 95,107,107,151,151,152, 95,107,155,156, 95,107,159,160,105,107,163,
164,105,117,167,168,105,117,171,172,105,117,175,176,105,117,179,180,115,117,183,
184,115,127,187,188,115,127,191,192,115,127,195,196,115,127,199,200,115,127,203,
204,115,127,207,208,125,127,211,212,125,137,215,216,125,137,219,220,125,137,223,
224,125,137,227,228,125,137,231,232,125,137,235,236,125,137,239,240,125,137,243,
244,135,137,247,248,135, 69,251,252,135, 69,255,252,135, 69,255,

  3,  3,  5,  7,  9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39,
 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79,
 81, 83, 85, 87, 89, 91, 93, 95, 97, 99,101,103,105,107,109,111,113,115,117,119,
121,123,125,127, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95,
 97, 99,101,103,105,107,109,111,113,115,117,119,121,123,125,131, 97, 67, 69, 71,
 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95, 97, 99,101,103,105,107,109,111,
113,115,117,119,121,123,125,129, 67,133, 99,135, 67,137, 99,139, 67,141, 99,143,
 67,145, 99,147, 67,149, 99,151, 67,153, 99,155, 67,157, 99,159, 67,161, 99,163,
 67,165, 99,167, 71,169, 99,171, 71,173, 99,175, 71,177, 99,179, 71,181, 99,183,
 71,185, 99,187, 71,189, 99,191, 71,193,103,195, 71,197,103,199, 71,201,103,203,
 71,205,103,207, 71,209,103,211, 71,213,103,215, 71,217,103,219, 71,221,103,223,
 71,225,103,227, 71,229,103,231, 71,233,103,235, 71,237,103,239, 71,241,103,243,
 71,245,103,247, 71,249,103,251, 71,253,103,255, 71,253,103,255,

  3,  3,  5,  7,  9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39,
 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79,
 81, 83, 85, 87, 89, 91, 93, 95, 97, 99,101,103,105,107,109,111,113,115,117,119,
121,123,125,127,129,131,133,135,137,139,141,143,145,147,149,151,153,155,157,159,
161,163,165,167,169,171,173,175,177,179,181,183,185,187,189,191,193,195,197,199,
201,203,205,207,209,211,213,215,217,219,221,223,225,227,229,231,233,235,237,239,
241,243,245,247,249,251,253,255,129,131,133,135,137,139,141,143,145,147,149,151,
153,155,157,159,161,163,165,167,169,171,173,175,177,179,181,183,185,187,189,191,
193,195,197,199,201,203,205,207,209,211,213,215,217,219,221,223,225,227,229,231,
233,235,237,239,241,243,245,247,249,251,253,191,193,131,133,135,137,139,141,143,
145,147,149,151,153,155,157,159,161,163,165,167,169,171,173,175,177,179,181,183,
185,187,189,191,193,195,197,199,201,203,205,207,209,211,213,215,217,219,221,223,
225,227,229,231,233,235,237,239,241,243,245,247,249,251,253,255,

 51, 66, 52,  4, 40,116, 97, 75, 47, 79,141,188,250,238,137,212, 37, 67, 38, 85,
130,102, 89, 61,  9,110,172,219,207,240,182,227, 82, 70,100,130, 41, 84, 55, 21,
131,117,103,104, 90, 76, 62, 63, 80,115, 21, 65, 99, 70, 41,146,132,118,112,105,
 91, 77, 78, 94, 80, 56, 69,114, 85, 71,  6,147,133,119,113,106, 92, 93,109,125,
 42,162,  5,100, 81, 57, 22,148,134,120,121,107,108,124,140,156,163,149,115, 86,
 72, 43,178,164,135,128,122,123,139,155,161,176,150,136,101, 87, 58,  7,179,165,
151,129,138,145,160,171,187,203,137,144, 96, 73, 44,194,180,166,152,153,154,170,
186,202,218,234,169,185, 88, 59, 23,195,181,167,168,184,200,201,217,233,249, 13,
216,232, 74, 45,210,196,182,183,199,215,231,247,248, 28,191,209, 12,175, 60,  8,
211,197,198,214,230,246, 27,159,190,206,222,253,208,237, 46,226,212,213,229,245,
 11,143,174,193,221,252, 30,255,225, 31, 24,227,228,244, 26,127,158,189,205,236,
 14,239,241,152,152,182,242,243, 10,111,142,173,192,220,251,223,254,237,167,197,
212,242, 25, 95,126,157,177,204,235, 29,224, 15,252,167,197,227,

#ifdef WIKI
  3,  3,  5,  7,  9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39,
 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 33, 35, 37, 39, 41, 43, 45, 47,
 49, 51, 53, 55, 57, 59, 61, 67, 49, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55,
 57, 59, 61, 65, 35, 69, 51, 71, 35, 73, 51, 75, 35, 77, 51, 79, 35, 81, 51, 83,
 35, 85, 51, 87, 35, 89, 51, 91, 35, 93, 51, 95, 35, 97, 51, 99, 35,101, 51,103,
 39,105, 51,107, 39,109, 51,111, 39,113, 51,115, 39,117, 51,119, 39,121, 51,123,
 39,125, 55,127, 39,129, 55,131, 39,133, 55,135, 39,137, 55,139, 39,141, 55,143,
 39,145, 55,147, 39,149, 55,151, 39,153, 55,155, 39,157, 55,159, 39,161, 55,163,
 39,165, 55,167, 39,169, 55,171, 39,173, 55,175, 39,177, 55,179, 39,181, 55,183,
 39,185, 55,187, 39,189, 55,191, 39,193, 55,195, 39,197, 55,199, 39,201, 55,203,
 39,205, 55,207, 39,209, 55,211, 39,213, 55,215, 39,217, 55,219, 39,221, 55,223,
 39,225, 55,227, 39,229, 55,231, 39,233, 55,235, 39,237, 55,239, 39,241, 55,243,
 39,245, 55,247, 39,249, 55,251, 39,253, 55,255, 39,253, 55,255,

#else
  1, 17,  3, 19,  5, 21,  7, 23,  9, 25, 11, 27, 13, 29, 15, 31, 19, 33, 21, 35,
 23, 37, 25, 39, 27, 41, 29, 43, 31, 45, 31, 47, 35, 49, 37, 51, 39, 53, 41, 55,
 43, 57, 45, 59, 47, 61, 25, 63, 51, 65, 53, 67, 55, 69, 57, 71, 59, 73, 61, 75,
 63, 77, 25, 79, 67, 81, 69, 83, 71, 85, 73, 87, 75, 89, 77, 91, 79, 93, 41, 95,
 83, 97, 85, 99, 87,101, 89,103, 91,105, 93,107, 95,109, 41,111, 99,113,101,115,
103,117,105,119,107,121,109,123,111,125, 57,127,115,129,117,131,119, 67,121, 69,
123, 69,125, 71,127, 71, 57, 73, 15,133, 31,135, 15,137, 31,139, 15,141, 31,143,
 15,145, 31,147, 15,149, 31,151, 15,153, 31,155, 15,157, 31,159, 15,161, 31,163,
 15,165, 31,167, 15,169, 31,171, 15,173, 31,175, 15,177, 31,179, 15,181, 31,183,
 15,185, 31,187, 15,189, 31,191, 15,193, 31,195, 15,197, 31,199, 15,201, 31,203,
 15,205, 31,207, 15,209, 31,211, 15,213, 31,215, 15,217, 31,219, 15,221, 31,223,
 15,225, 31,227, 15,229, 31,231, 15,233, 31,235, 15,237, 31,239, 15,241, 31,243,
 15,245, 31,247, 15,249, 31,251, 15,253, 31,255, 15,253, 31,255,
#endif
};
#define nex(state,sel) *(p+state+sel*256)


//////////////////////////// StateMap, APM //////////////////////////

// A StateMap maps a context to a probability.  Methods:
//
// Statemap sm(n) creates a StateMap with n contexts using 4*n bytes memory.
// sm.p(y, cx, limit) converts state cx (0..n-1) to a probability (0..4095).
//	that the next y=1, updating the previous prediction with y (0..1).
//	limit (1..1023, default 1023) is the maximum count for computing a
//	prediction.  Larger values are better for stationary sources.

class StateMap {
public:
  U32 *t_cxt;	// Context of last prediction
  U32 *t;	// cxt -> prediction in high 22 bits, count in low 10 bits
  StateMap(int n=768);

  // update bit y (0..1), predict next bit in context cx
#define smp(x) {\
  assert(y>>1==0);\
  assert(cx>=0 && cx<N);\
  assert(cxt>=0 && cxt<N);\
    U32 p0=*t_cxt;\
    U32 i=p0&1023, pr=p0>>12; /* count, prediction */ \
    p0+=(i<x);\
    p0+=((y20-(int)pr)*dt[i])&0xfffffc00;\
    *t_cxt=p0;\
    t_cxt=t+cx;\
    return (*t_cxt) >>20;\
  }

  inline int p0(int cx) smp(TOLIMIT_10)
  inline int p1(int cx) smp(TOLIMIT_11)
  inline int p2(int cx) smp(TOLIMIT_12)
  inline int p3(int cx) smp(TOLIMIT_13)
  inline int p4(int cx) smp(TOLIMIT_14)
  inline int p5(int cx) smp(TOLIMIT_15)
  inline int p6(int cx) smp(TOLIMIT_16)

};

StateMap::StateMap(int n) {
  alloc(t, n);
  t_cxt=t;
  for (int i=0; i<n; ++i)
    t[i]=0x80000000;
}

// An APM maps a probability and a context to a new probability.  Methods:
//
// APM a(n) creates with n contexts using 96*n bytes memory.
// a.pp(y, pr, cx, limit) updates and returns a new probability (0..4095)
//     like with StateMap.  pr (0..4095) is considered part of the context.
//     The output is computed by interpolating pr into 24 ranges nonlinearly
//     with smaller ranges near the ends.  The initial output is pr.
//     y=(0..1) is the last bit.  cx=(0..n-1) is the other context.
//     limit=(0..1023) defaults to 255.

class APM {
protected:
  int cxt;	// Context of last prediction
  U32 *t;	// cxt -> prediction in high 22 bits, count in low 10 bits
public:
  APM(int n);

  inline int p1(int pr, int cx) {	//, int limit=1023)
    assert(y>>1==0);
    assert(cx>=0 && cx<N/24);
    assert(cxt>=0 && cxt<N);
  {
    U32 *p=&t[cxt], p0=p[0];
    U32 i=p0&1023, pr=p0>>12; // count, prediction
    p0+=(i<TOLIMIT_2a);
    p0+=((y20-(int)pr)*dta[i]+0x200)&0xfffffc00;
    p[0]=p0;
  }
    int wt=pr&0xfff;  // interpolation weight of next element
    cx=cx*24+(pr>>12);
    cxt=cx+(wt>>11);
    pr=(t[cx]>>13)*(0x1000-wt)+(t[cx+1]>>13)*wt>>19;
    return pr;
  }

  inline int p2(int pr, int cx) {	//, int limit=1023)
    assert(y>>1==0);
    assert(cx>=0 && cx<N/24);
    assert(cxt>=0 && cxt<N);
  {
    U32 *p=&t[cxt], p0=p[0];
    U32 i=p0&1023, pr=p0>>12; // count, prediction
    p0+=(i<TOLIMIT_2b);
    p0+=((y20-(int)pr)*dta[i]+0x200)&0xfffffc00;
    p[0]=p0;
  }
    int wt=pr&0xfff;  // interpolation weight of next element
    cx=cx*24+(pr>>12);
    cxt=cx+(wt>>11);
    pr=(t[cx]>>13)*(0x1000-wt)+(t[cx+1]>>13)*wt>>19;
    return pr;
  }
};

APM::APM(int n): cxt(0) {
  alloc(t, n);
  for (int i=0; i<n; ++i) {
    int p=((i%24*2+1)*4096)/48-2048;
    t[i]=(U32(squash_init(p))<<20)+12;
  }
}

//////////////////////////// Mixer /////////////////////////////

// Mixer m(MI, MC) combines models using MC neural networks with
//	MI inputs each using 4*MC*MI bytes of memory.  It is used as follows:
// m.update(y) trains the network where the expected output is the
//	last bit, y.
// m.add(stretch(p)) inputs prediction from one of MI models.  The
//	prediction should be positive to predict a 1 bit, negative for 0,
//	nominally -2K to 2K.
// m.set(cxt) selects cxt (0..MC-1) as one of MC neural networks to use.
// m.p() returns the output prediction that the next bit is 1 as a
//	12 bit number (0 to 4095).  The normal sequence per prediction is:
//
// - m.add(x) called MI times with input x=(-2047..2047)
// - m.set(cxt) called once with cxt=(0..MC-1)
// - m.p() called once to predict the next bit, returns 0..4095
// - m.update(y) called once for actual bit y=(0..1).

#define MI 8
#define MC 1280
  int mxr_tx[MI];	// MI inputs
  int* mxr_wx;		// MI*MC weights
  int* mxr_wx4;		// mxr_wx + MI*40
  int* mxr_cxt;		// context
  int mxr_pr=2048;	// last result (scaled 12 bits)

#if 0			// ATTENTION !  CHANGE this to 1 if you start to use
			//		<mixer max inputs>!=8 in your versions.
inline void train(int err) {
  int *w=mxr_cxt;
  assert(err>=-32768 && err<32768);
  for (int i=0; i<MI; ++i)
    w[i]+=mxr_tx[i]*err+0x4000>>15;
}

inline int dot_product() {
  int *w=mxr_cxt;
  int sum=0;
  for (int i=0; i<MI; ++i)
    sum+=mxr_tx[i]*w[i];
  sum>>=DP_SHIFT;
  if (sum<-2047) sum=-2047;
  if (sum> 2047) sum= 2047;
  return sum;
}

#else
inline void train(int err) {
    int *w=mxr_cxt;
    assert(err>=-32768 && err<32768);
#ifdef WIKI
    w[0]+=mxr_tx[0]*err+0x2000>>14;
    w[1]+=mxr_tx[1]*err+0x2000>>14;
    w[2]+=mxr_tx[2]*err+0x2000>>14;
    w[3]+=mxr_tx[3]*err+0x2000>>14;
    w[4]+=mxr_tx[4]*err+0x2000>>14;
    w[5]+=mxr_tx[5]*err+0x2000>>14;
    w[6]+=mxr_tx[6]*err+0x2000>>14;
    w[7]+=	    err+0x20  >>6;
#else
    w[0]+=mxr_tx[0]*err+0x0800>>12;
    w[1]+=mxr_tx[1]*err+0x0800>>12;
    w[2]+=mxr_tx[2]*err+0x1000>>13;
    w[3]+=mxr_tx[3]*err+0x0800>>12;
    w[4]+=mxr_tx[4]*err+0x0800>>12;
    w[5]+=mxr_tx[5]*err+0x0800>>12;
    w[6]+=mxr_tx[6]*err+0x0800>>12;
    w[7]+=	    err+4 >>3;
#endif
}
inline int dot_product() {
    int *w=mxr_cxt;
    int sum =mxr_tx[0]*w[0];
	sum+=mxr_tx[1]*w[1];
	sum+=mxr_tx[2]*w[2];
	sum+=mxr_tx[3]*w[3];
	sum+=mxr_tx[4]*w[4];
	sum+=mxr_tx[5]*w[5];
	sum+=mxr_tx[6]*w[6];
	sum+=		w[7]<<8;
  sum>>=DP_SHIFT;
  if (sum<-2047) sum=-2047;
  if (sum> 2047) sum= 2047;
  return sum;
}
#endif


///class Mixer {
///public:
///  Mixer(int m);

  // Adjust weights to minimize coding cost of last prediction
#define m_update(y) {			\
    int err=y*0xfff-mxr_pr;		\
    fails<<=1;				\
    if (err<MXR_UPD && err>-MXR_UPD) {} else {	\
      fails|=calcfails[err+4096];	\
      train(err*MXR_MUL);		\
    }\
  }

  // Input x (call up to MI times)

#define m_add(a,b) { assert((a)<MI); mxr_tx[a]=stretch_t[b]; }

  // predict next bit
#define m_p	dot_product()

///};

//////////////////////////// HashTable /////////////////////////

// A HashTable maps a 32-bit index to an array of B bytes.
// The first byte is a checksum using the upper 8 bits of the
// index.  The second byte is a priority (0 = empty) for hash
// replacement.  The index need not be a hash.

// HashTable<B> h(n) - create using n bytes  n and B must be
//     powers of 2 with n >= B*4, and B >= 2.
// h[i] returns array [1..B-1] of bytes indexed by i, creating and
//     replacing another element if needed.  Element 0 is the
//     checksum and should not be modified.

template <int B>
class HashTable {
  U8* t;	// table: 1 element = B bytes: checksum,priority,data,data,...
  const int NB;	// size in bytes
public:
  HashTable(int n);
  U8* get(U32 i);
#ifdef WIKI
  U8* get0(U32 i);
  U8* get1(U32 i);
#endif
};

template <int B>
HashTable<B>::HashTable(int n): NB(n-B) {
  assert(B>=2 && (B&B-1)==0);
  assert(n>=B*4 && (n&n-1)==0);
  alloc(t, n+512+B*2);
  t=(U8*)(((intptr_t)t+511)&(-200))+1;	// align on cache line boundary
}

#define RORi(x,y) x<<(32-y)|x>>y

#ifdef WIKI
inline U32 hash1(U32 i) {  i*=765432197;  i=RORi(i, 9); return (i*0xfffefdfb);}
inline U32 hash2(U32 i) {  i*=0xfffefdfb; i=RORi(i,12); return (i*654321893); }
inline U32 hash3(U32 i) {  i*=0xfffefdfb; i=RORi(i,10); return (i*543210973); }
inline U32 hash4(U32 i) {  i*=432109879;  i=RORi(i,10); return (i*0xfffefdfb);}
inline U32 hash5(U32 i) {  i*=0xfffefdfb; i=RORi(i,12); return (i*987654323); }
inline U32 hash6(U32 i) {  i*=876543211;  i=RORi(i,14); return (i*345678941); }
inline U32 hash7(U32 i) {  i*=765432197;  i=RORi(i,14); return (i*345678941); }
inline U32 hash8(U32 i) {  i*=345678941;  i=RORi(i,13); return (i*654321893); }
inline U32 hash9(U32 i) {  i*=345678941;  i=RORi(i,11); return (i*543210973); }
inline U32 hash0(U32 i) {  i*=345678941;  i=RORi(i,15); return (i*432109879); }
inline U32 hash1a(U32 i){		  i=RORi(i, 9); return (i	   ); }
inline U32 hash3a(U32 i){  i*=234567891;  i=RORi(i,11); return (i	   ); }
#else
inline U32 hash1(U32 i) {  i*=0xfffefdfb; i=RORi(i,10); return (i*765432197); }
inline U32 hash2(U32 i) {  i*=0xfffefdfb; i=RORi(i,13); return (i*654321893); }
inline U32 hash3(U32 i) {  i*=0xfffefdfb; i=RORi(i,11); return (i*543210973); }
inline U32 hash4(U32 i) {  i*=0xfffefdfb; i=RORi(i,12); return (i*432109879); }
inline U32 hash5(U32 i) {  i*=987654323;  i=RORi(i,11); return (i*234567891); }
inline U32 hash6(U32 i) {  i*=0xfeff77ff; i=RORi(i,13); return (i*876543211); }
inline U32 hash7(U32 i) {  i*=0xfeff77ff; i=RORi(i,12); return (i	   ); }
inline U32 hash8(U32 i) {		  i=RORi(i,14); return (i*654321893); }
inline U32 hash9(U32 i) {  i*=0xfeff77ff; i=RORi(i,10); return (i*543210973); }
inline U32 hash0(U32 i) {		  i=RORi(i,14); return (i*432109879); }
inline U32 hash3a(U32 i){		  i=RORi(i,10); return (i*543210973); }
#endif

template <int B>
inline U8* HashTable<B>::get(U32 i) {
  U8 *p=t+(i*B&NB), *q, *r, f;
  i>>=24;
  f=*(p-1);
  if (f==U8(i)) return p;
  q=p+B;
  f=*(q-1);
  if (f==U8(i)) return q;
  r=p+B*2;
  f=*(r-1);
  if (f==U8(i)) return r;
  if (*p>*q) p=q;
  if (*p>*r) p=r;
#if 0			// ATTENTION !	CHANGE this to 1 if you start to use
			//		HashTable with B!=16 in your versions.
  memset(p-1, 0, B);
  *(p-1)=i;		// This is big-endian-compatible
#else
		*(U32*)(p -1)=i;	// This is NOT big-endian-compatible
		*(U32*)(p+ 3)=0;
		*(U32*)(p+ 7)=0;
		*(U32*)(p+11)=0;
#endif
  return p;
}

#ifdef WIKI
template <int B>
inline U8* HashTable<B>::get0(U32 i) {
  U8 *p=t+(i*B&NB), *q, *r, f;
  i>>=25;
  f=*(p-1);
  if (f==U8(i)) return p;
  q=(U8*)((U32)p^B);
  f=*(q-1);
  if (f==U8(i)) return q;
  r=(U8*)((U32)p^B*2);
  f=*(r-1);
  if (f==U8(i)) return r;
  if (*p>*q) p=q;
  if (*p>*r) p=r;
	*(U32*)(p -1)=i;	// This is NOT big-endian-compatible
	*(U32*)(p+ 3)=0;
	*(U32*)(p+ 7)=0;
	*(U32*)(p+11)=0;
  return p;
}
template <int B>
inline U8* HashTable<B>::get1(U32 i) {
  U8 *p=t+(i*B&NB), *q, *r, f;
  i>>=25; i|=128;
  f=*(p-1);
  if (f==U8(i)) return p;
  q=(U8*)((U32)p^B);
  f=*(q-1);
  if (f==U8(i)) return q;
  r=(U8*)((U32)p^B*2);
  f=*(r-1);
  if (f==U8(i)) return r;
  if (*p>*q) p=q;
  if (*p>*r) p=r;
	*(U32*)(p -1)=i;	// This is NOT big-endian-compatible
	*(U32*)(p+ 3)=0;
	*(U32*)(p+ 7)=0;
	*(U32*)(p+11)=0;
  return p;
}
#endif

//////////////////////////// MatchModel ////////////////////////

// MatchModel(n) predicts next bit using most recent context match.
//     using n bytes of memory.  n must be a power of 2 at least 8.
// MatchModel::p(y, m) updates the model with bit y (0..1) and writes
//     a prediction of the next bit to Mixer m.  It returns the length of
//     context matched (0..62).

  U32 h1, h2, h3; // context hashes
  int N, HN; // last hash table index, n/8-1

  enum {MAXLEN=62};   // maximum match length, at most 62
  U32 len2cxt[MAXLEN*2+1], len2order[MAXLEN+1];

#ifdef WIKI
  U8 len2cxt0[]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,
	20,20,20,21,21,21,21,22,22,22,22,22,23,23,23,23,23,23,24,24,24,
	24,24,24,24,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,27 };
#else
  U8 len2cxt0[]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,16,16,
	17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,
	22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,25,26,26,26,26,26,27 };
#endif

class MatchModel {
  int* ht;    // context hash -> next byte in buf
  int match;  // pointer to current byte in matched context in buf
  int buf_match;  // buf[match]+256
  int len;    // length of match
  StateMap sm;  // len, bit, last byte -> prediction
public:
  MatchModel(int n);  // n must be a power of 2 at least 8.
  inline int p();	// predict next bit to m
  inline void upd();	// update bit y (0..1)
};

MatchModel::MatchModel(int n): sm(55<<8) {
  N=n/2-1;
  HN=n/8-1;
  h1=h2=h3=match=len=0;
  assert(n>=8 && (n&n-1)==0);
  alloc(buf, N+1);
  alloc(ht, HN+1);
}

#define SEARCH(hsh) {	\
	len=1;		\
	match=ht[hsh];	\
	if (match!=pos) {\
	  while (len<MAXLEN+1 && buf[match-len&N]==buf[pos-len&N])	++len; \
	}		\
}
#define SEARCH2(hsh) {	\
	len=1;		\
	match=ht[hsh];	\
	if (match!=pos) {\
	  p=p1;		\
	  while (len<MAXLEN+1 && buf[match-len&N]==*p)		--p, ++len; \
	}		\
}

inline void MatchModel::upd() {
    // find or extend match
    if (len>MIN_LEN) {
      ++match;
      match&=N;
      if (len<MAXLEN)	++len;
    }
    else {
	if (pos>=MAXLEN) {
		U8 *p1=buf+pos-1, *p;
			     SEARCH2(h1)
		if (len<4) { SEARCH2(h2)
#ifndef WIKI
		if (len<4)   SEARCH2(h3)
#endif
		}
	}
	else {
			     SEARCH(h1)
		if (len<4) { SEARCH(h2)
#ifndef WIKI
		if (len<4) SEARCH(h3)
#endif
		}
	}

	--len;
    }
    buf_match=buf[match]+256;

    // update index
#ifdef WIKI
  if (pos&1) {
    ht[h1]=pos;
    if (len>11)
    ht[h2]=pos;
  }
  else
  {
    ht[h2]=pos;
    if (len>12)
    ht[h1]=pos;
  }

#else
    ht[h1]=pos;
    ht[h2]=pos;
    ht[h3]=pos;
#endif
}

inline int MatchModel::p() {

  int cxt=c0;
  if (len>0) {
    int b=buf_match>>bcount;
    if ((b>>1)==cxt)
      b=b&1,	// next bit
      cxt=len2cxt[len*2-b] + c1;
    else
	len=0;
  }

  m_add(0, sm.p0(cxt));
  return len;
}

//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// Predictor(n) creates with 3*n bytes of memory.
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

//int MEM=0;	// Global memory usage = 3*MEM bytes (1<<20 .. 1<<29)

  U8 t0[0x10000];  // order 1 cxt -> state
  U8 *t0c1=t0, *cp[6]={t0, t0, t0, t0, t0, t0}; // pointer to bit history
  U32 h[6], pw=0, c8=0, cc=0, prevfail=0;
  U8 fails=0;
  StateMap sm[6];
  APM a1(24 * 0x10000), a2(24 * 0x800);

//////////////////////////// Encoder ////////////////////////////

// An Encoder does arithmetic encoding.  Methods:
// Encoder(COMPRESS, f) creates encoder for compression to archive f, which
//     must be open past any header for writing in binary mode.
// Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
//     which must be open past any header for reading in binary mode.
// code(i) in COMPRESS mode compresses bit i (0 or 1) to file f.
// code() in DECOMPRESS mode returns the next decompressed bit from file f.
// compress(c) in COMPRESS mode compresses one byte.
// decompress() in DECOMPRESS mode decompresses and returns one byte.
// flush() should be called exactly once after compression is done and
//     before closing f.  It does nothing in DECOMPRESS mode.

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
public:
#ifdef WIKI
  HashTable<16> t4a;  // cxt -> state
  HashTable<16> t4b;  // cxt -> state
#define t1a t4a
#define t2a t4a
#define t3a t4a
#define t1b t4b
#define t2b t4b
#define t3b t4b
#else
  HashTable<16> t1;  // cxt -> state
  HashTable<16> t2;  // cxt -> state
  HashTable<16> t3;  // cxt -> state
#define t1a t1
#define t2a t2
#define t3a t3
#define t1b t1
#define t2b t2
#define t3b t3
#endif
  MatchModel mm;	// predicts next bit by matching context
  unsigned char *archive;	// Compressed data file
  U32 x1, x2;		// Range, initially [0, 1), scaled by 2^32
  U32 x;		// Decompress mode: last 4 input bytes of archive
  U32 p;
  int *add2order;

#define cp_update(m0,m1,m2,m3,m4,m5)\
  assert(y==0 || y==1);	\
  y20=y<<20;		\
  {			\
  const U8 *p=&State_table[0]; if(y) p+=256*5;	\
  *cp[0]=nex(*cp[0], m0);  \
  *cp[1]=nex(*cp[1], m1);  \
  *cp[2]=nex(*cp[2], m2);  \
  *cp[3]=nex(*cp[3], m3);  \
  *cp[4]=nex(*cp[4], m4);  \
  *cp[5]=nex(*cp[5], m5);  \
  }			   \
  c0+=c0+y;

#define upd0(m0,m1,m2,m3,m4,m5,bc) \
  assert(y==0 || y==1);	\
  y20=y<<20;		\
	bcount=bc;		\
	add2order+=MI*10;	\
	{			\
	  const U8 *p=&State_table[0]; if(y) p+=256*5; \
	  int j=y+1 << (2-(bc&3));	\
	  *cp[1]=nex(*cp[1], m1); cp[1]+=j; \
	  *cp[2]=nex(*cp[2], m2); cp[2]+=j; \
	  *cp[3]=nex(*cp[3], m3); cp[3]+=j; \
	  *cp[4]=nex(*cp[4], m4); cp[4]+=j; \
	  *cp[5]=nex(*cp[5], m5); cp[5]+=j; \
	  *cp[0]=nex(*cp[0], m0);\
	}			\
  c0+=c0+y;

#define upd3(m0,m1,m2,m3,m4,m5,bc) \
  cp_update(m0,m1,m2,m3,m4,m5)	\
	bcount=bc;		\
	add2order=add2order4;	\
	  cp[1]=t1b.get0(hash6(c0   -h[1]));\
	  upd3_cp			    \
	  cp[5]=t2b.get0(hash0(c0*P5_MUL-h[5]));\

#define upd7(m0,m1,m2,m3,m4,m5) \
  cp_update(m0,m1,m2,m3,m4,m5)		\
    {					\
    int c=c0-256;			\
    add2order=mxr_wx+MI*10*8*add2order0;\
    c1=(c1>>4)+((c*2)&240);		\
    buf[pos]=c;				\
    pos=(pos+1)&N;			\
    cc=(cc<<8)+(c8>>24);		\
    c8=(c8<<8)+(c4>>24);		\
    c4=c4<<8|c;				\
    upd7_h123				\
    h[1]=(c4&0xffff)*8191;	/* order 2 */\
    cp[1]=t1b.get1(hash1(h[1]));	\
    upd7_cp				\
    if (c>=97 && c<=122) c-=32;	/* lowercase unigram word order */ \
    if (c>=65 && c<=90) h[5]=(h[5]*2+c)*191;\
    else pw=h[5]*PW_MUL, h[5]=0;	\
    cp[5]=t2a.get1(hash5(h[5]-pw));	\
    c0=1;		\
    bcount=7;		\
    mm.upd();		\
    }

U32 Predictor_upd(int y) {
  m_update(y);

  // predict
  int len=mm.p(), pr;
  if (len==0) {
#ifdef WIKI
	if (*cp[1]!=0) { len=(1+(*cp[2]!=0)+(*cp[3]!=0)+(*cp[4]!=0))*MI; }
#else
	if (*cp[1]!=0) { len =MI;
	if (*cp[2]!=0) { len+=MI;
	if (*cp[3]!=0) { len+=MI;
	if (*cp[4]!=0)   len+=MI;
	}}}
#endif
   }
   else len=len2order[len];
  mxr_cxt=add2order+len;
  m_add(1, sm[1].p1(*cp[1]));
  m_add(2, sm[2].p2(*cp[2]));
  m_add(3, sm[3].p3(*cp[3]));
  m_add(4, sm[4].p4(*cp[4]));
  m_add(5, sm[5].p5(*cp[5]));

#ifdef WIKI
  int h0c0=h[0]+c0, xx;
  cp[0]=t0+h0c0;
  m_add(6, sm[0].p6(*cp[0]));
  pr=m_p+2047;
  xx=a1.p1(pr*23, h0c0);
  mxr_pr=squash(pr-2047) + 7*xx >>3;
  pr=mxr_pr	+7*a2.p2(stretch_t2[xx], fails*8+bcount)+4 >>3;
#else
  cp[0]=t0c1+c0;
  m_add(6, sm[0].p6(*cp[0]));
  pr=m_p+2047;
  pr=squash(pr-2047)	+3*a1.p1(pr*23, h[0]+c0) >>2;
  mxr_pr=pr;
  pr=pr*5	+11*a2.p2(stretch_t2[pr], fails+prevfail)+8 >>4;
#endif
  return pr+(pr<2048);
}

  // Return decompressed bit
#define dec1(k) \
    U32 xmid=x1 + (x2-x1>>12)*p + ((x2-x1&0xfff)*p>>12);\
    assert(xmid>=x1 && xmid<x2);			\
    int y=x<=xmid;					\
    y ? (x2=xmid) : (x1=xmid+1);

#define dec2 \
    p=Predictor_upd(y);\
    while (((x1^x2)&0xff000000)==0) {  /* pass equal leading bytes of range */ \
      x=(x<<8)+(*archive++);	/* EOF is OK */ \
      x1<<=8;						\
      x2=(x2<<8)+255;					\
    }

  Encoder(Mode m, unsigned char* f, int MEM);
  void flush();  // call this when compression is finished

#ifdef WIKI
#define eight_bits(part1,part2) \
    sm[0].t+=256;\
    sm[1].t+=256;\
    sm[2].t+=256;\
    sm[4].t+=256;\
    { part1(7); upd0(2,1,0,0,0,0, 6); part2; }	sm[1].t+=256;\
    { part1(6); upd0(2,2,1,0,0,0, 5); part2; }\
    { part1(5); upd0(2,1,1,0,0,0, 4); part2; }	sm[5].t+=256;\
    { part1(4); upd3(2,1,1,0,0,0, 3); part2; }	sm[4].t+=256;\
    { part1(3); upd0(2,1,1,0,0,4, 2); part2; }\
    { part1(2); upd0(2,1,1,0,3,4, 1); part2; }  sm[2].t+=256;\
    { part1(1); upd0(2,1,1,0,3,4, 0); part2; }	sm[0].t-=256; sm[1].t-=512; sm[2].t-=512; sm[4].t-=512; sm[5].t-=256;\
    { part1(0); upd7(2,1,0,0,3,4   ); part2; }
#else
#define eight_bits(part1,part2) \
    sm[4].t+=256;\
    sm[0].t+=256;\
    sm[5].t+=256;\
    { part1(7); upd0(1,0,0,3,4,2, 6); part2; }\
    { part1(6); upd0(1,0,0,3,0,1, 5); part2; }	sm[2].t+=256; sm[5].t+=256; if (method==TEXT) sm[4].t+=256;\
    { part1(5); upd0(1,0,0,3,0,1, 4); part2; }	sm[1].t+=256;\
    { part1(4); upd3(1,0,1,3,0,1, 3); part2; }	sm[0].t+=256;\
    { part1(3); upd0(1,0,1,3,0,1, 2); part2; }	sm[3].t+=256;\
    { part1(2); upd0(1,0,1,3,0,1, 1); part2; }\
    { part1(1); upd0(1,0,1,2,0,1, 0); part2; }	sm[0].t-=512; sm[1].t-=256; sm[2].t-=256; sm[3].t-=256; sm[5].t-=512;\
						sm[4].t-=(method==TEXT)? 512:256;\
    { part1(0); upd7(1,0,1,2,0,1   ); part2; }
#endif

  // Decompress and return one byte
  inline int decompress() {
    eight_bits(dec1,dec2)
    int c=c4&255;
    if (method==TEXT)
	if (c==0x20||c==0x1f) c^=0x3f;
    return c;
  }
};

Encoder::Encoder(Mode m, unsigned char* f, int MEM):
#ifdef WIKI
    t4a(MEM), t4b(MEM),
#else
    t1(MEM/2), t2(MEM), t3(MEM/2),
#endif
    mm(MEM), x1(0), x2(0xffffffff), x(0), p(2048) {

  archive=f;
  if (m==DECOMPRESS) {  // x = first 4 bytes of archive
    for (int i=0; i<4; ++i)
      x=(x<<8)+(*archive++);
  }

  int i, pi=0;
  for (int x=-2047; x<=2047; ++x) {  // invert squash()
    int i=squash_init(x);
    squash(x)=i+SQUARD;	//rounding,  needed at the end of Predictor::update()
    for (int j=pi; j<=i; ++j)
      stretch_t[j]=x, stretch_t2[j]=(x+2047)*23;
    pi=i+1;
  }
  stretch_t[4095]=2047;
  stretch_t2[4095]=4094*23;

  for (i=0; i<1024; ++i)
#ifdef WIKI
    dt[i]=512*36/(i+i+9),
    dta[i]=8192/(i+i+8);
#else
    dt[i]=512*17/(i+i+3),
    dta[i]=512*34/(i+i+18);

  for (i=0; i<256; ++i) {
    pi=0;
    if (i&  3) pi+=1024;
    if (i& 28) pi+=512;
    if (i&224) pi+=256;
    calcprevfail[i]=pi;
  }

  int	cf1=1216, cf3=2592;
  if (method==TEXT)
	cf1=640, cf3=2272;
#endif

  for (i=-4096; i<4096; ++i) {
    int e=i, v=0;
    if (e<0) e=-e;
#ifdef WIKI
    if (e > 1184) v=1;
    if (e > 2592) v=3;
#else
    if (e > cf1) v=1;
    if (e > cf3) v=3;
#endif
    calcfails[i+4096]=v;
  }

  for (i=2; i<=MAXLEN*2; i+=2) {
      int c = len2cxt0[i>>1]*512;
	 len2cxt[i]=c;
	 len2cxt[i-1]=c-256;
	c=i>>1;
#ifdef WIKI
	len2order[c]=(5+(c>=10)+(c>=14)+(c>=18)+(c>26))*MI;
#else
    if (method==TEXT)
	len2order[c]=(5+(c>=10)+(c>=14)+(c>=19)+(c>28))*MI;
    else
	len2order[c]=(5+(c>=8) +(c>=12)+(c>=18)+(c==MAXLEN))*MI;
#endif
  }

  alloc(mxr_wx, MI*MC);
  for (i=0; i<MI*MC; ++i)	mxr_wx[i] = MXR_INIT;
  mxr_wx4=mxr_wx + MI*40;
  mxr_cxt=mxr_wx;
  add2order=mxr_wx;
}

void Encoder::flush() {
  ///if (mode==COMPRESS)
    //putc(x1>>24, archive);  // Flush first unequal byte of range
}


//////////////////////////// User Interface ////////////////////////////

extern "C" int unlpaq8(unsigned char *in, int insz, unsigned char *out, int size, int mem, int meth) {
    if(mem    <= 0) mem = 5;            // 9 requires tons of memory
    mem=1<<(mem+20);

    method = meth;
    if(method <= 0) method = DEFAULT;   // DEFAULT, EXE, TEXT

#ifndef WIKI
			  TOLIMIT_10=64;
	if (method==TEXT) TOLIMIT_10=320;
	if (method==EXE)  TOLIMIT_10=7;
#endif

    // Decompress
    Encoder e(DECOMPRESS, in, mem);
      uncompressed=out;
      {
      U8 *p=&file_buf[0], c;
      long s=FB_SIZE+4, k=0;
      if (s>size) s=size;
      size-=s;
      //ss=s;
      while (s-->0)
	{ c=e.decompress(); *p++=c; if(c>0x7f) ++k; }
      if (k==0)	{ if(method!=TEXT) return(-2); }
      else
      if (file_buf[0]=='M' && file_buf[1]=='Z')
	{ if(method!=EXE) return(-3); }
      fb_pos=p;
      }

    while (size-->0) {
	int c;
	do_putc(e.decompress());
	if ( (pos&(256*1024-1))==0 )
#ifndef WIKI
	if (method==TEXT)
	if ( (pos== 9*512*1024 && DP_SHIFT==16) || (pos==1024*1024 && DP_SHIFT==15) || DP_SHIFT==14 )
#else
	if ( (pos==25*256*1024 && DP_SHIFT==16) || (pos==1024*1024 && DP_SHIFT==15) || DP_SHIFT==14 )
#endif
		for (DP_SHIFT++, c=0; c<MI*MC; ++c)	mxr_wx[c] *= 2;
    }
	do_putc_end();

  return uncompressed - out;
}
