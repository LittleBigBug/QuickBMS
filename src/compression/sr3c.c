/* SR3C, a symbol ranking data compressor.
 *
 * This file implements a fast and effective data compressor in a form
 * easily embeddable into C programs.  The compression is slightly
 * faster to gzip -7, but results in ~11% smaller data.  bzip2 -2
 * compresses slightly better than SR3C, but takes almost three times
 * as long.  Furthermore, since bzip2 is based on Burrows-Wheeler
 * block sorting, it can't be used in on-line compression tasks.
 * Memory consumption of SR3C is currently around 4.5 MB per ongoing
 * compression and decompression.
 *
 * Author: Kenneth Oksanen <cessu@iki.fi>, 2008.
 * Copyright (C) Helsinki University of Technology.
 *
 * This code borrows many ideas and some paragraphs of comments from
 * Matt Mahoney's s symbol ranking compression program SR2 and Peter
 * Fenwicks SRANK, but otherwise all code has been implemented from
 * scratch.
 *
 * This file is distributed under the following license:

The MIT License
Copyright (c) 2008 Helsinki University of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */


/* This code is based on many of the ideas in Matt Mahoney's SR2
   symbol ranking compression program
       http://www.cs.fit.edu/~mmahoney/compression/#sr2
   which in turn is based on SRANK by P. M. Fenwick in 1997-98
       ftp://ftp.cs.auckland.ac.nz/pub/staff/peter-f/srank.c
   See also Fenwick's technical report "A fast, constant-order, symbol
   ranking text compressor", The University of Auckland, Department of
   Computer Science Report No 145, Apr. 1997.

   A symbol ranking compressor maintains a list (a move-to-front
   queue) of the most recently seen symbols (bytes) in the given
   context in order of time since last seen.  The original SRANK used
   a table of 2^10 to 2^18 hashed order-3 contexts, SR2 uses 2^20
   hashed order-4 contexts, and some versions of SR3 use 2^24
   contexts, which we considered excessively much in terms of implied
   memory consumption.  All the mentioned programs use a queue length
   of 3, SR2 furthermore uses a 6 bit count (n) of consecutive hits,
   SR3C uses only a 4 bit count but employs a 4-bit checksum to reduce
   hash collisions.

   SR2 as well as SR3C follow Fenwick's suggestion for a hardware
   implementation in sending 0 for literals, 110 and 111 for the
   second and third least recently seen, and 10xxxxxxxx's are reserved
   for literals.  These are compressed further with arithmetic coding
   using both an order-1 context (last coded byte) and the count as
   context, or order-0 and count if the count is greater or equal to
   four.

   Codes and updates are as follows:

      Input    Code        Next state
                           (c1  c2  c3  n)
      -----    ----        ---------------
      Initial              (0,  0,  0,  0)
        c1     0           (c1, c2, c3, min(n+1, 15))
        c2     110         (c2, c1, c3, 1)
        c3     111         (c3, c1, c2, 1)
      other c  10cccccccc  (c,  c1, c2, 0)
   
   As an exception, however, in SR3C if input is c2 and n has reached
   very high counts (12 or more), then the count is reduced to four
   but the queue is kept intact.

   After coding byte c, the hash index h is updated to h * 480 + c + 1
   (mod 2^20) which depends on only the last 4 bytes.  SR2 did not
   detect hash collisions, but SR3C uses a 4-bit checksum which is
   formed by the next four higher bits of the hash.  The values are
   packed into a 32 bit integer as follows: c1 in bits 0-7, c2 in
   8-15, c3 in 16-23, checksum in 24-27, n in 28-31.

   End of file is marked by coding c3 as a literal (SR2 used c1)
   followed by three 0xFF's.  The compressor library adds no header,
   but ensures the message doesn't begin with 0xFF so as to enable
   arbitrary catenation of messages.  Additional headers and checksums
   may be added by a stand-alone archiver using this library.

   Arithmetic coding is performed as in SR2, and we advise the reader
   to its documentation.  Let it only be noted that compared to SR2,
   SR3C uses far fewer arithmetic coding states than SR2 thus saving
   ~1.5 MB of RAM as well as incidentally also a slightly better
   compression ratio.  There are also several other differences in
   between SR2 and SR3C in how the predictions for next bits are
   updated.
 */


#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#include "sr3c.h"


#define SR_HMASK   0xFFFFF
#define SR_HMULT   480
#define SR_XSHFT   20

/* It is possible to reduce memory consumption from 4.5 MB to, say,
   1.5 MB with the following definitions, but this will also reduce
   the compression ratio.  In my tests on average this would result in
   an almost 4% increase in the size of the compressed data.
	#define SR_HMASK   0x3FFFF
	#define SR_HMULT   352
	#define SR_XSHFT   18
   Even further memory savings are possible, but again with worse
   compression.  The following definitions require some 0.75 MB memory
   and result 7-8% larger compressed data than with current default
   definitions.  Further memory savings would require changes in the
   secondary arithmetic compression, a.k.a. state maps.
	#define SR_HMASK   0xFFFF
	#define SR_HMULT   288
	#define SR_XSHFT   16
   Conversely, by allowing a loftier memory budget, say 64.5 MB,
   compression can be improved further.  The following will result in
   a little over 2% improvement in compression:
	#define SR_HMASK   0xFFFFFF
	#define SR_HMULT   704
	#define SR_XSHFT   24
 */


struct sr3c_context_t {
  /* Symbol ranking tables. */
  uint32_t rank[SR_HMASK + 1];

  /* Hash value of the current compression context. */
  uint32_t hash;
  
  /* Previous byte we encoded, 0 if none. */
  int prev_ch;

  /* Statemaps map secondary context to probability.  Each statemap
     entry contains the prediction in the upper 25 bits and a count of
     how many times this state has been reached in the lower 7 bits. */
  /* States for encoding the first three bits in each context.  Some
     of the entries in sm_huff_hi are unused. */
#define SM_HUFF_LO_N  (4 * 256 * 3)
  uint32_t sm_huff_lo[SM_HUFF_LO_N];
#define SM_HUFF_HI_N1  (16)
#define SM_HUFF_HI_N2  (3)
  uint32_t sm_huff_hi[SM_HUFF_HI_N1][SM_HUFF_HI_N2];
  /* States for encoding a byte not predicted by symbol ranker using a
     context-1 arithmetic encoding. */
#define SM_BYTES_N  (256 * 256)
  uint32_t sm_bytes[SM_BYTES_N];

  /* Arithmetic coder range, initially [0, 1), scaled by 2^32.  The
     field x is use only during uncompression. */
  uint32_t x1, x2, x;

  /* Output function and its context, returns non-zero on error. */
  sr3c_output_f_t output_f;
  void *output_ctx;

  enum {
    SR_FLUSHED, 
    SR_COMPRESSING, 
    /* The following states are for the uncompressor. */
    SR_FILLING_1, SR_FILLING_2, SR_FILLING_3, 
    SR_UNCOMPRESSING_1, SR_UNCOMPRESSING_2, SR_UNCOMPRESSING_3,
    SR_UNCOMPRESSING_BYTE, SR_FLUSHING,
  } status;

  /* The following field is used by the uncompressor while
     uncompressing literal bytes. */
  int lit_ix;
};


/* Some precomputed tables on how the secondary arithmetical
   compressor adjusts to actual data.  See comments later. */
static unsigned char sr_wt_bytes[512];
static unsigned char sr_wt_ranks[512];
static uint16_t sr_ct_bytes[65][512];
static uint16_t sr_ct_ranks[65][512];


void sr3c_reset(sr3c_context_t *context,
		sr3c_output_f_t output_f, void *output_ctx)
{
  int i/*, j*/;

  assert(context != NULL);

#define SM_P_HALF  (1 << 31)
  /* Initialize statemaps.  The initial values were chosen based on a
     large number of test runs.  The cumulative statistic match quite
     well those of Fenwick:
       - Least recently used matches with 45% probability (over all counts),
       - Second least recently used matches with 15% probability,
       - Third least recently used matches with 7% probability,
       - Literals match with 32% probability.
     Initializing to anything else but SM_P_HALF produces
     proportionally modest benefits for large inputs, but we wanted
     SR3C to be effective also for small inputs. */
  for (i = 0; i < 3 * 256; i += 3) {
    context->sm_huff_lo[i] = (3400 << 20) | 0;
    context->sm_huff_lo[i + 1] = (150 << 20) | 12;
    context->sm_huff_lo[i + 2] = (1000 << 20) | 12;
  }
  for (; i < 2 * 3 * 256; i += 3) {
    context->sm_huff_lo[i] = (1500 << 20) | 0;
    context->sm_huff_lo[i + 1] = (1840 << 20) | 12;
    context->sm_huff_lo[i + 2] = (780 << 20) | 12;
  }
  for (; i < 3 * 3 * 256; i += 3) {
    context->sm_huff_lo[i] = (880 << 20) | 0;
    context->sm_huff_lo[i + 1] = (1840 << 20) | 12;
    context->sm_huff_lo[i + 2] = (760 << 20) | 12;
  }
  for (; i < 4 * 3 * 256; i += 3) {
    context->sm_huff_lo[i] = (780 << 20) | 0;
    context->sm_huff_lo[i + 1] = (1840 << 20) | 12;
    context->sm_huff_lo[i + 2] = (1120 << 20) | 12;
  }
  for (i = 0; i < SM_HUFF_HI_N1; i++) {
    context->sm_huff_hi[i][0] = (400 << 20) | 10;
    context->sm_huff_hi[i][1] = (1840 << 20) | 12;
    context->sm_huff_hi[i][2] = (1180 << 20) | 12;
  }
  for (i = 0; i < SM_BYTES_N; i++)
    context->sm_bytes[i] = SM_P_HALF;

  for (i = 0; i < SR_HMASK + 1; i++)
    context->rank[i] = 0;

  context->prev_ch = 0;
  context->hash = 0;

  context->x1 = 0;
  context->x2 = 0xFEFFFFFF;
  if (output_f != NULL) {
    context->output_f = output_f;
    context->output_ctx = output_ctx;
  }
  context->status = SR_FLUSHED;

  if (sr_ct_ranks[0][0] == 0) {
    /* Initialize the various precomputed tables. */

    int d, count, c;

    for (count = 0; count < 512; count++) {
      /* A table indexed by current state map count and returning the
	 corresponding responsiveness to unpredicted input.  Found
	 with numeric optimization for the order-0 arithmetic compressor. */
      sr_wt_bytes[count] =
	2.814086 + (1.107489 * count + 639.588922)
  	  / (0.006940 * count * count + count + 6.318012);
      /* As above, but for rank encoding data. */
      sr_wt_ranks[count] =
	-1.311630 + (0.616477 * count + 640.391038)
	  / (0.001946 * count * count + count + 5.632143);
      for (d = 0; d <= 64; d++) {
	/* A table for updating the count-part in statemaps for bytes. */
	if (d > 1.1898325 && count > 19.782085)
	  c = -2 + 0.021466 * (d - 1.1898325) * (count - 19.782085);
	else
	  c = -2;
	if (c > count)
	  c = 0;
	else if (count - c > 0x1FF)
	  c = 0x1FF;
	else
	  c = count - c;
	sr_ct_bytes[d][count] = c ^ count;
	/* A table for updating the count-part in statemaps for ranks. */
	if (count > 33.861341)
	  c = -2 + 0.005355 * (d + 0.981405) * (count - 33.861341);
	else
	  c = -2;
	if (c > count)
	  c = 0;
	else if (count - c > 0x1FF)
	  c = 0x1FF;
	else
	  c = count - c;
	sr_ct_ranks[d][count] = c ^ count;
      }
    }
  }
}


sr3c_context_t *sr3c_alloc(sr3c_output_f_t output_f, void *output_ctx)
{
  sr3c_context_t *ctx = (sr3c_context_t *) malloc(sizeof(sr3c_context_t));

  if (ctx == NULL)
    return NULL;

  sr3c_reset(ctx, NULL, NULL);
  ctx->output_f = output_f;
  ctx->output_ctx = output_ctx;

  return ctx;
}


void sr3c_free(sr3c_context_t *ctx)
{
  assert(ctx != NULL);
  free(ctx);
}


/* What is the prediction, as a fractional probability from 0 to 4095, of
   the next bit being one. */
#define SM_PREDICT(sm)   ((sm) >> 20)
#define SM_COUNT(sm)     ((sm) & 0x1FF)

#define SM_UPDATE_RANKS(sm, bit)			\
  do {							\
    assert((bit) == 0 || (bit) == 1);			\
							\
    int count = (sm) & 0x1FF, prediction = (sm) >> 9;	\
    int d = ((bit) << 23) - prediction;			\
    (sm) += (d * sr_wt_ranks[count]) & 0xFFFFFE00;	\
    if (d < 0)						\
      d = -d;						\
    d >>= 17;						\
    /* (sm) += sr_ct_ranks[d][count]; */		\
    (sm) ^= sr_ct_ranks[d][count];			\
  } while (0)

#define SM_UPDATE_BYTES(sm, bit)			\
  do {							\
    assert((bit) == 0 || (bit) == 1);			\
							\
    int count = (sm) & 0x1FF, prediction = (sm) >> 9;	\
    int d = ((bit) << 23) - prediction;			\
    (sm) += (d * sr_wt_bytes[count]) & 0xFFFFFE00;	\
    if (d < 0)						\
      d = -d;						\
    d >>= 17;						\
    (sm) ^= sr_ct_bytes[d][count]; 			\
  } while (0)


/* Compress bit in the given state map, possibly outputting bytes in
   process. */

#define SR_ENCODE_RANK_BIT(sm, bit, context)				\
  do {									\
    uint32_t xmid;							\
    int prediction = SM_PREDICT(sm);					\
									\
    assert((bit) == 0 || (bit) == 1);					\
    assert(prediction >= 0 && prediction < 4096);			\
    xmid = (context)->x1						\
      + (((context)->x2 - (context)->x1) >> 12) * prediction;		\
    assert(xmid >= (context)->x1 && xmid < (context)->x2);		\
    if (bit)								\
      (context)->x2 = xmid;						\
    else								\
      (context)->x1 = xmid + 1;						\
    SM_UPDATE_RANKS(sm, (bit));						\
    /* Pass equal leading bytes of range. */				\
    while (((context)->x1 >> 24) == ((context)->x2 >> 24)) {		\
      *outbuf_p++ = (context)->x2 >> 24;				\
      (context)->x1 <<= 8;						\
      (context)->x2 = ((context)->x2 << 8) + 255;			\
    }									\
  } while (0)


#define SR_ENCODE_BYTE_BIT(sm, bit, context)				\
  do {									\
    uint32_t xmid;							\
    int prediction = SM_PREDICT(sm);					\
									\
    assert((bit) == 0 || (bit) == 1);					\
    assert(prediction >= 0 && prediction < 4096);			\
    xmid = (context)->x1						\
      + (((context)->x2 - (context)->x1) >> 12) * prediction;		\
    assert(xmid >= (context)->x1 && xmid < (context)->x2);		\
    if (bit)								\
      (context)->x2 = xmid;						\
    else								\
      (context)->x1 = xmid + 1;						\
    SM_UPDATE_BYTES(sm, (bit));						\
    /* Pass equal leading bytes of range. */				\
    while (((context)->x1 >> 24) == ((context)->x2 >> 24)) {		\
      *outbuf_p++ = (context)->x2 >> 24;				\
      (context)->x1 <<= 8;						\
      (context)->x2 = ((context)->x2 << 8) + 255;			\
    }									\
  } while (0)


#define SR_ENCODE_BYTE(ch, context)					\
  do {									\
    uint32_t *sm = &(context)->sm_bytes[256 * (context)->prev_ch];	\
    unsigned int ix = 1, x = (ch), bit;					\
    do {								\
      bit = (x >> 7) & 0x1;						\
      SR_ENCODE_BYTE_BIT(sm[ix], bit, context);				\
      ix = (ix << 1) | bit;						\
      x <<= 1;								\
    } while (ix < 256);							\
  } while (0)


int sr3c_compress(const unsigned char *bytes, size_t n_bytes, 
		  sr3c_context_t *context)
{
  uint32_t index, xsum, r, *sm;
  int ch;
#define OUTBUF_SIZE 1024
  unsigned char outbuf[OUTBUF_SIZE], *outbuf_p;
  /* The theoretical worst case is that each bit is encoded into 12
     bits, and there can be 10 bits of output per bit sent to the
     arithmetic coder, or 15 bytes.  Additionally there may be some
     bytes of flushing from the arithmetic coder itself, thus a margin
     of slightly cautious 30 bytes. */
  unsigned char *outbuf_end = outbuf + OUTBUF_SIZE - 30;

  assert(context->status == SR_FLUSHED || context->status == SR_COMPRESSING);

  context->status = SR_COMPRESSING;

  while (n_bytes > 0) {
    outbuf_p = outbuf;

    while (outbuf_p < outbuf_end && n_bytes > 0) {
      n_bytes--;
      index = context->hash & SR_HMASK;
      xsum = (context->hash >> SR_XSHFT) & 0xF;
      r = context->rank[index];
      if (((r >> 24) & 0xF) != xsum) {
	/* Hash collision.  Pick another index, use it if checksums
	   match or it has a lower first-hit counter value. */
	int alt_index = (index + 0x3A77) & SR_HMASK;
	uint32_t alt_r = context->rank[alt_index];
	if (((alt_r >> 24) & 0xF) == xsum || alt_r < r) {
	  index = alt_index;
	  r = alt_r;
	}
      }
      if (r >= 0x40000000)
	sm = &context->sm_huff_hi[r >> 28][0];
      else
	sm = &context->sm_huff_lo[3 * (context->prev_ch | ((r >> 20) & 0x300))];
      
      ch = *bytes++;
      xsum = (xsum << 24) | ch;
      if (ch == (r & 0xFF)) {
	/* Is ch the least recently seen? */
	SR_ENCODE_RANK_BIT(sm[0], 0, context);
	if (r < 0xF0000000)	/* Increment hit count. */
	  r += 0x10000000;
      } else {
	SR_ENCODE_RANK_BIT(sm[0], 1, context);
	if (ch == ((r >> 8) & 0xFF)) {
	  /* Is ch the second least recent? */
	  SR_ENCODE_RANK_BIT(sm[1], 1, context);
	  SR_ENCODE_RANK_BIT(sm[2], 0, context);
	  if ((r >> 28) >= 0xC)
	    r &= 0x4FFFFFFF;
	  else
	    r = (r & 0xFF0000) | ((r & 0xFF) << 8) | xsum | 0x10000000;
	} else if (ch == ((r >> 16) & 0xFF)) {
	  /* Is ch the third least recent? */
	  SR_ENCODE_RANK_BIT(sm[1], 1, context);
	  SR_ENCODE_RANK_BIT(sm[2], 1, context);
	  r = ((r & 0xFFFF) << 8) | xsum | 0x10000000;
	} else {
	  SR_ENCODE_RANK_BIT(sm[1], 0, context);
	  SR_ENCODE_BYTE(ch, context);
	  r = ((r & 0xFFFF) << 8) | xsum;
	}
      }
      context->rank[index] = r;
      context->prev_ch = ch;
      context->hash = SR_HMULT * context->hash + ch + 1;
    }

    if (outbuf_p > outbuf) {
      int err = context->output_f(outbuf, outbuf_p - outbuf,
				  0, context->output_ctx);
      if (err)
	return err;
    }
  }

  return 0;
}


int sr3c_flush(sr3c_context_t *context)
{
  uint32_t r, *sm;
  int index, xsum, /*i,*/ err;
  unsigned char outbuf[128], *outbuf_p = &outbuf[0];

  assert(context->status == SR_COMPRESSING
	 || context->status == SR_FLUSHED);

  /* Pick state map entry as if compressing a normal data byte. */
  index = context->hash & SR_HMASK;
  xsum = (context->hash >> SR_XSHFT) & 0xF;
  r = context->rank[index];
  if (((r >> 24) & 0xF) != xsum) {
    int alt_index = (index + 0x3A77) & SR_HMASK;
    uint32_t alt_r = context->rank[alt_index];
    if (((alt_r >> 24) & 0xF) == xsum || alt_r < r) {
      index = alt_index;
      r = alt_r;
    }
  }
  if (r >= 0x40000000)
    sm = &context->sm_huff_hi[r >> 28][0];
  else
    sm = &context->sm_huff_lo[3 * (context->prev_ch | ((r >> 20) & 0x300))];
  
  /* Mark end of data by coding third least recently used byte as
     a literal. */
  SR_ENCODE_RANK_BIT(sm[0], 1, context);
  SR_ENCODE_RANK_BIT(sm[1], 0, context);
  SR_ENCODE_BYTE((r >> 16) & 0xFF, context);
  
  /* Flush also the arithmetic encoder, first by the first unequal
     byte in the range and thereafter three maximum bytes. */
  *outbuf_p++ = context->x1 >> 24;
  *outbuf_p++ = 0xFF;
  *outbuf_p++ = 0xFF;
  *outbuf_p++ = 0xFF;
  
  /* Finally send this all out. */
  err = context->output_f(outbuf, outbuf_p - outbuf, 1, context->output_ctx);
  if (err)
    return err;

  /* Reset internal values in the context, not however the statemaps
     or ranks. */
  context->prev_ch = 0;
  context->hash = 0;
  context->x1 = 0;
  context->x2 = 0xFEFFFFFF;
  context->status = SR_FLUSHED;

  return 0;
}


int sr3c_is_flushed(sr3c_context_t *context)
{
  return context->status == SR_FLUSHED;
}


#define SR_INPUT(context, state_name)				\
  do {								\
  state_name:							\
    while (((context)->x1 >> 24) == ((context)->x2 >> 24)) {	\
      if (n_bytes == 0) {					\
        (context)->status = state_name;				\
	goto out_of_input;					\
      }								\
      (context)->x1 <<= 8;					\
      (context)->x2 = ((context)->x2 << 8) + 255;		\
      (context)->x = ((context)->x << 8) + *bytes++;		\
      n_bytes--;						\
    }								\
  } while (0)

#define SR_DECODE_BIT(sm, bit, context, state_name)			\
  do {									\
    uint32_t xmid;							\
    int prediction;							\
									\
    SR_INPUT(context, state_name);					\
    prediction = SM_PREDICT(sm);					\
    assert(prediction >= 0 && prediction < 4096);			\
    xmid = (context)->x1						\
      + (((context)->x2 - (context)->x1) >> 12) * prediction;		\
    assert(xmid >= (context)->x1 && xmid < (context)->x2);		\
    if ((context)->x <= xmid) {						\
      bit = 1;								\
      (context)->x2 = xmid;						\
    } else {								\
      bit = 0;								\
      (context)->x1 = xmid + 1;						\
    }									\
  } while (0)


int sr3c_uncompress(const unsigned char *bytes, size_t n_bytes,
		    sr3c_context_t *context)
{
  uint32_t index, xsum, r, *sm;
  int bit, ch, err;
  unsigned char outbuf[OUTBUF_SIZE], *outbuf_p = outbuf;
  unsigned char *outbuf_end = outbuf + OUTBUF_SIZE;

  assert(context->status != SR_COMPRESSING);

  switch (context->status) {
  case SR_FLUSHED:
    while (n_bytes > 0 && *bytes == 0xFF) {
      bytes++;
      n_bytes--;
    }
    if (n_bytes-- == 0)
      return 0;
  restart:
    context->x = (context->x << 8) | *bytes++;
    /*FALLTHROUGH*/
  case SR_FILLING_1:
    if (n_bytes-- == 0) {
      context->status = SR_FILLING_1;
      return 0;
    }
    context->x = (context->x << 8) | *bytes++;
    /*FALLTHROUGH*/
  case SR_FILLING_2:
    if (n_bytes-- == 0) {
      context->status = SR_FILLING_2;
      return 0;
    }
    context->x = (context->x << 8) | *bytes++;
    /*FALLTHROUGH*/
  case SR_FILLING_3:
    if (n_bytes-- == 0) {
      context->status = SR_FILLING_3;
      return 0;
    }
    context->x = (context->x << 8) | *bytes++;
    context->status = SR_UNCOMPRESSING_1;
    break;
  case SR_FLUSHING:
    goto SR_FLUSHING;
  default:
    /* The default branch is here to only to keep the compiler happy. */
    break;
  }

  index = context->hash & SR_HMASK;
  xsum = (context->hash >> SR_XSHFT) & 0xF;
  r = context->rank[index];
  if (((r >> 24) & 0xF) != xsum) {
    /* Hash collision.  Pick another index, use it if checksums
       match or it has a lower first-hit counter value. */
    int alt_index = (index + 0x3A77) & SR_HMASK;
    uint32_t alt_r = context->rank[alt_index];
    if (((alt_r >> 24) & 0xF) == xsum || alt_r < r) {
      index = alt_index;
      r = alt_r;
    }
  }
  if (r >= 0x40000000)
    sm = &context->sm_huff_hi[r >> 28][0];
  else
    sm = &context->sm_huff_lo[3 * (context->prev_ch | ((r >> 20) & 0x300))];
  xsum <<= 24;

  switch (context->status) {
  case SR_UNCOMPRESSING_1:
    goto SR_UNCOMPRESSING_1;
  case SR_UNCOMPRESSING_2:
    goto SR_UNCOMPRESSING_2;
  case SR_UNCOMPRESSING_3:
    goto SR_UNCOMPRESSING_3;
  case SR_UNCOMPRESSING_BYTE:
    sm = &context->sm_bytes[256 * context->prev_ch];
    goto SR_UNCOMPRESSING_BYTE;
  default:
    break;
  }

  /*NOTREACHED*/
  abort();

  while (1) {
    index = context->hash & SR_HMASK;
    xsum = (context->hash >> SR_XSHFT) & 0xF;
    r = context->rank[index];
    if (((r >> 24) & 0xF) != xsum) {
      /* Hash collision.  Pick another index, use it if checksums
	 match or it has a lower first-hit counter value. */
      int alt_index = (index + 0x3A77) & SR_HMASK;
      uint32_t alt_r = context->rank[alt_index];
      if (((alt_r >> 24) & 0xF) == xsum || alt_r < r) {
	index = alt_index;
	r = alt_r;
      }
    }
    if (r >= 0x40000000)
      sm = &context->sm_huff_hi[r >> 28][0];
    else
      sm = &context->sm_huff_lo[3 * (context->prev_ch | ((r >> 20) & 0x300))];
    xsum <<= 24;
    
    SR_DECODE_BIT(sm[0], bit, context, SR_UNCOMPRESSING_1);
    SM_UPDATE_RANKS(sm[0], bit);
    if (bit) {
      SR_DECODE_BIT(sm[1], bit, context, SR_UNCOMPRESSING_2);
      SM_UPDATE_RANKS(sm[1], bit);
      if (bit) {
	SR_DECODE_BIT(sm[2], bit, context, SR_UNCOMPRESSING_3);
	SM_UPDATE_RANKS(sm[2], bit);
	if (bit) {
	  /* Third least recent byte. */
	  ch = (r >> 16) & 0xFF;
	  r = ((r & 0xFFFF) << 8) | ch | xsum | 0x10000000;
	} else {
	  /* Second least recent byte. */
	  ch = (r >> 8) & 0xFF;
	  if ((r >> 28) >= 0xC)
	    r &= 0x4FFFFFFF;
	  else
	    r = (r & 0xFF0000) | ((r & 0xFF) << 8) | ch | xsum | 0x10000000;
	}
      } else {
	sm = &context->sm_bytes[256 * context->prev_ch];
	context->lit_ix = 1;
	do {
	  SR_DECODE_BIT(sm[context->lit_ix], bit, context, 
			SR_UNCOMPRESSING_BYTE);
	  SM_UPDATE_BYTES(sm[context->lit_ix], bit);
	  context->lit_ix = (context->lit_ix << 1) | bit;
	} while (context->lit_ix < 256);
	ch = context->lit_ix & 0xFF;
	if (ch == ((r >> 16) & 0xFF))
	  goto flush;
	r = ((r & 0xFFFF) << 8) | ch | xsum;
      }
    } else {
      /* Least recent byte. */
      ch = r & 0xFF;
      if (r < 0xF0000000)
	r += 0x10000000;
    }
    
    *outbuf_p++ = ch;
    context->rank[index] = r;
    context->prev_ch = ch;
    context->hash = SR_HMULT * context->hash + ch + 1;
    if (outbuf_p == outbuf_end) {
      err = context->output_f(outbuf, OUTBUF_SIZE, 0, context->output_ctx);
      if (err)
	return err;
      outbuf_p = outbuf;
    }
  }

 flush:
  /* We come here when we have received a flush.  Pass the
     flush-induced bytes in the data stream and reset internal values
     in the context, not however the statemaps or ranks. */
  SR_INPUT(context, SR_FLUSHING);
  context->prev_ch = 0;
  context->hash = 0;
  context->x1 = 0;
  context->x2 = 0xFEFFFFFF;
  context->status = SR_FLUSHED;
  /* Skip 0xFF-bytes. */
  while (n_bytes > 0 && *bytes == 0xFF) {
    bytes++;
    n_bytes--;
  }
  err = context->output_f(outbuf, outbuf_p - outbuf, 1, context->output_ctx);
  if (err)
    return err;
  outbuf_p = outbuf;
  if (n_bytes-- > 0) {
    outbuf_p = outbuf;
    goto restart;
  }
  return 0;

 out_of_input:
  if (outbuf_p != outbuf)
    return context->output_f(outbuf, outbuf_p - outbuf, 0, context->output_ctx);
  return 0;
}
