/* cabextract 0.2 - a program to extract Microsoft Cabinet files
 * (C) 2000-2001 Stuart Caie <kyzer@4u.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lzx.h"
#include "lzx_int.h"

/* LZX decruncher */

/* This LZX decruncher was pulled out of the program cabextract 0.2 by 
   Stuart Caie <kyzer@4u.net> and modified to be useful as an LZX decruncher 
   outside the context of CAB files.  I do not claim any copyright on the
   (minor) modifications.
      -- Matthew T. Russotto
*/

/* Microsoft's LZX document and their implementation of the
 * com.ms.util.cab Java package do not concur.
 * 
 * Correlation between window size and number of position slots: In
 * the LZX document, 1MB window = 40 slots, 2MB window = 42 slots. In
 * the implementation, 1MB = 42 slots, 2MB = 50 slots. (The actual
 * calculation is 'find the first slot whose position base is equal to
 * or more than the required window size'). This would explain why
 * other tables in the document refer to 50 slots rather than 42.
 *
 * The constant NUM_PRIMARY_LENGTHS used in the decompression
 * pseudocode is not defined in the specification, although it could
 * be derived from the section on encoding match lengths.
 *
 * The LZX document does not state the uncompressed block has an
 * uncompressed length. Where does this length field come from, so we
 * can know how large the block is? The implementation suggests that
 * it's in the 24 bits proceeding the 3 blocktype bits, before the
 * alignment padding.
 *
 * The LZX document states that aligned offset blocks have their
 * aligned offset huffman tree AFTER the main and length tree. The
 * implementation suggests that the aligned offset tree is BEFORE the
 * main and length trees.
 *
 * The LZX document decoding algorithim states that, in an aligned
 * offset block, if an extra_bits value is 1, 2 or 3, then that number
 * of bits should be read and the result added to the match
 * offset. This is correct for 1 and 2, but not 3 bits, where only an
 * aligned symbol should be read.
 *
 * Regarding the E8 preprocessing, the LZX document states 'No
 * translation may be performed on the last 6 bytes of the input
 * block'. This is correct. However, the pseudocode provided checks
 * for the *E8 leader* up to the last 6 bytes. If the leader appears
 * between -10 and -7 bytes from the end, this would cause the next
 * four bytes to be modified, at least one of which would be in the
 * last 6 bytes, which is not allowed according to the spec.
 *
 * The specification states that the huffman trees must always contain
 * at least one element. However, many CAB files badly compressed
 * sections where the length tree is completely empty (because there
 * are no matches), and this is expected to succeed.
 */


/* LZX uses what it calls 'position slots' to represent match offsets.
 * What this means is that a small 'position slot' number and a small
 * offset from that slot are encoded instead of one large offset for
 * every match.
 * - position_base is an index to the position slot bases
 * - extra_bits states how many bits of offset-from-base data is needed.
 */
static ULONG position_base[51];
static UBYTE extra_bits[52];


int LZXinit(int window) {
  unsigned int wndsize = 1 << window;
  int i, j, posn_slots;

  /* LZX supports window sizes of 2^15 (32Kb) through 2^21 (2Mb) */
  /* if a previously allocated window is big enough, keep it     */
  if (window < 15 || window > 21) return DECR_DATAFORMAT;
  if (LZX(actual_size) < wndsize) {
    if (LZX(window)) free(LZX(window));
    LZX(window) = NULL;
  }
  if (!LZX(window)) {
    if (!(LZX(window) = malloc(wndsize))) return DECR_NOMEMORY;
    LZX(actual_size) = wndsize;
  }
  LZX(window_size) = wndsize;

  /* initialise static tables */
  for (i=0, j=0; i <= 50; i += 2) {
    extra_bits[i] = extra_bits[i+1] = j; /* 0,0,0,0,1,1,2,2,3,3... */
    if ((i != 0) && (j < 17)) j++; /* 0,0,1,2,3,4...15,16,17,17,17,17... */
  }
  for (i=0, j=0; i <= 50; i++) {
    position_base[i] = j; /* 0,1,2,3,4,6,8,12,16,24,32,... */
    j += 1 << extra_bits[i]; /* 1,1,1,1,2,2,4,4,8,8,16,16,32,32,... */
  }

  /* calculate required position slots */
       if (window == 20) posn_slots = 42;
  else if (window == 21) posn_slots = 50;
  else posn_slots = window << 1;

  /*posn_slots=i=0; while (i < wndsize) i += 1 << extra_bits[posn_slots++]; */
  

  LZX(R0)  =  LZX(R1)  = LZX(R2) = 1;
  LZX(main_elements)   = LZX_NUM_CHARS + (posn_slots << 3);
  LZX(header_read)     = 0;
  LZX(frames_read)     = 0;
  LZX(block_remaining) = 0;
  LZX(block_type)      = LZX_BLOCKTYPE_INVALID;
  LZX(intel_curpos)    = 0;
  LZX(intel_started)   = 0;
  LZX(window_posn)     = 0;

  /* initialise tables to 0 (because deltas will be applied to them) */
  for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) LZX(MAINTREE_len)[i] = 0;
  for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   LZX(LENGTH_len)[i]   = 0;

  return DECR_OK;
}


/* Bitstream reading macros:
 *
 * INIT_BITSTREAM    should be used first to set up the system
 * READ_BITS(var,n)  takes N bits from the buffer and puts them in var
 *
 * ENSURE_BITS(n)    ensures there are at least N bits in the bit buffer
 * PEEK_BITS(n)      extracts (without removing) N bits from the bit buffer
 * REMOVE_BITS(n)    removes N bits from the bit buffer
 *
 * These bit access routines work by using the area beyond the MSB and the
 * LSB as a free source of zeroes. This avoids having to mask any bits.
 * So we have to know the bit width of the bitbuffer variable. This is
 * sizeof(ULONG) * 8, also defined as ULONG_BITS
 */

/* number of bits in ULONG. Note: This must be at multiple of 16, and at
 * least 32 for the bitbuffer code to work (ie, it must be able to ensure
 * up to 17 bits - that's adding 16 bits when there's one bit left, or
 * adding 32 bits when there are no bits left. The code should work fine
 * for machines where ULONG >= 32 bits.
 */
#define ULONG_BITS (sizeof(ULONG)<<3)

#define INIT_BITSTREAM do { bitsleft = 0; bitbuf = 0; } while (0)

#define ENSURE_BITS(n)							\
  while (bitsleft < (n)) {						\
    bitbuf |= ((inpos[1]<<8)|inpos[0]) << (ULONG_BITS-16 - bitsleft);	\
    bitsleft += 16; inpos+=2;						\
  }

#define PEEK_BITS(n)   (bitbuf >> (ULONG_BITS - (n)))
#define REMOVE_BITS(n) ((bitbuf <<= (n)), (bitsleft -= (n)))

#define READ_BITS(v,n) do {						\
  ENSURE_BITS(n);							\
  (v) = PEEK_BITS(n);							\
  REMOVE_BITS(n);							\
} while (0)


/* Huffman macros */

#define TABLEBITS(tbl)   (LZX_##tbl##_TABLEBITS)
#define MAXSYMBOLS(tbl)  (LZX_##tbl##_MAXSYMBOLS)
#define SYMTABLE(tbl)    (LZX(tbl##_table))
#define LENTABLE(tbl)    (LZX(tbl##_len))

/* BUILD_TABLE(tablename) builds a huffman lookup table from code lengths.
 * In reality, it just calls make_decode_table() with the appropriate
 * values - they're all fixed by some #defines anyway, so there's no point
 * writing each call out in full by hand.
 */
#define BUILD_TABLE(tbl)						\
  if (make_decode_table(						\
    MAXSYMBOLS(tbl), TABLEBITS(tbl), LENTABLE(tbl), SYMTABLE(tbl)	\
  )) { return 200+DECR_ILLEGALDATA; }


/* READ_HUFFSYM(tablename, var) decodes one huffman symbol from the
 * bitstream using the stated table and puts it in var.
 */
#define READ_HUFFSYM(tbl,var) do {					\
  ENSURE_BITS(16);							\
  hufftbl = SYMTABLE(tbl);						\
  if ((i = hufftbl[PEEK_BITS(TABLEBITS(tbl))]) >= MAXSYMBOLS(tbl)) {	\
    j = 1 << (ULONG_BITS - TABLEBITS(tbl));				\
    do {								\
      j >>= 1; i <<= 1; i |= (bitbuf & j) ? 1 : 0;			\
      if (!j) { return 300+DECR_ILLEGALDATA; }	                        \
    } while ((i = hufftbl[i]) >= MAXSYMBOLS(tbl));			\
  }									\
  j = LENTABLE(tbl)[(var) = i];						\
  REMOVE_BITS(j);							\
} while (0)


/* READ_LENGTHS(tablename, first, last) reads in code lengths for symbols
 * first to last in the given table. The code lengths are stored in their
 * own special LZX way.
 */
#define READ_LENGTHS(tbl,first,last) do { \
  lb.bb = bitbuf; lb.bl = bitsleft; lb.ip = inpos; \
  if (lzx_read_lens(LENTABLE(tbl),(first),(last),&lb)) { \
    return 400+DECR_ILLEGALDATA; \
  } \
  bitbuf = lb.bb; bitsleft = lb.bl; inpos = lb.ip; \
} while (0)


/* make_decode_table(nsyms, nbits, length[], table[])
 *
 * This function was coded by David Tritscher. It builds a fast huffman
 * decoding table out of just a canonical huffman code lengths table.
 *
 * nsyms  = total number of symbols in this huffman tree.
 * nbits  = any symbols with a code length of nbits or less can be decoded
 *          in one lookup of the table.
 * length = A table to get code lengths from [0 to syms-1]
 * table  = The table to fill up with decoded symbols and pointers.
 *
 * Returns 0 for OK or 1 for error
 */

int make_decode_table(int nsyms, int nbits, UBYTE *length, UWORD *table) {
  register UWORD sym;
  register ULONG leaf;
  register UBYTE bit_num = 1;
  ULONG fill;
  ULONG pos         = 0; /* the current position in the decode table */
  ULONG table_mask  = 1 << nbits;
  ULONG bit_mask    = table_mask >> 1; /* don't do 0 length codes */
  ULONG next_symbol = bit_mask; /* base of allocation for long codes */

  /* fill entries for codes short enough for a direct mapping */
  while (bit_num <= nbits) {
    for (sym = 0; sym < nsyms; sym++) {
      if (length[sym] == bit_num) {
        leaf = pos;

        if((pos += bit_mask) > table_mask) return 1; /* table overrun */

        /* fill all possible lookups of this symbol with the symbol itself */
        fill = bit_mask;
        while (fill-- > 0) table[leaf++] = sym;
      }
    }
    bit_mask >>= 1;
    bit_num++;
  }

  /* if there are any codes longer than nbits */
  if (pos != table_mask) {
    /* clear the remainder of the table */
    for (sym = pos; sym < table_mask; sym++) table[sym] = (unsigned short)0;

    /* give ourselves room for codes to grow by up to 16 more bits */
    pos <<= 16;
    table_mask <<= 16;
    bit_mask = 1 << 15;

    while (bit_num <= 16) {
      for (sym = 0; sym < nsyms; sym++) {
        if (length[sym] == bit_num) {
          leaf = pos >> 16;
          for (fill = 0; fill < bit_num - nbits; fill++) {
            /* if this path hasn't been taken yet, 'allocate' two entries */
            if (table[leaf] == 0) {
              table[(next_symbol << 1)] = 0;
              table[(next_symbol << 1) + 1] = 0;
              table[leaf] = next_symbol++;
            }
            /* follow the path and select either left or right for next bit */
            leaf = table[leaf] << 1;
            if ((pos >> (15-fill)) & 1) leaf++;
          }
          table[leaf] = sym;

          if ((pos += bit_mask) > table_mask) return 1; /* table overflow */
        }
      }
      bit_mask >>= 1;
      bit_num++;
    }
  }

  /* full table? */
  if (pos == table_mask) return 0;

  /* either erroneous table, or all elements are 0 - let's find out. */
  for (sym = 0; sym < nsyms; sym++) if (length[sym]) return 1;
  return 0;
}

struct lzx_bits {
  ULONG bb;
  int bl;
  UBYTE *ip;
};

int lzx_read_lens(UBYTE *lens, int first, int last, struct lzx_bits *lb) {
  ULONG i,j, x,y;
  int z;

  register ULONG bitbuf = lb->bb;
  register int bitsleft = lb->bl;
  UBYTE *inpos = lb->ip;
  UWORD *hufftbl;
  
  for (x = 0; x < 20; x++) {
    READ_BITS(y, 4);
    LENTABLE(PRETREE)[x] = y;
  }
  BUILD_TABLE(PRETREE);

  for (x = first; x < last; ) {
    READ_HUFFSYM(PRETREE, z);
    if (z == 17) {
      READ_BITS(y, 4); y += 4;
      while (y--) lens[x++] = 0;
    }
    else if (z == 18) {
      READ_BITS(y, 5); y += 20;
      while (y--) lens[x++] = 0;
    }
    else if (z == 19) {
      READ_BITS(y, 1); y += 4;
      READ_HUFFSYM(PRETREE, z);
      z = lens[x] - z; if (z < 0) z += 17;
      while (y--) lens[x++] = z;
    }
    else {
      z = lens[x] - z; if (z < 0) z += 17;
      lens[x++] = z;
    }
  }

  lb->bb = bitbuf;
  lb->bl = bitsleft;
  lb->ip = inpos;
  return 0;
}

void LZXreset(void) {
    int i;

    LZX(block_type) = 0;
    LZX(block_length) = 0;
    LZX(header_read) = 0;
    for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) LZX(MAINTREE_len)[i] = 0;
    for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   LZX(LENGTH_len)[i]   = 0;
    LZX(R0) = LZX(R1) = LZX(R2) = 1;
}



int LZXdecompress(UBYTE *inbuf, UBYTE *outbuf, ULONG inlen, ULONG outlen) {
  UBYTE *inpos  = inbuf;
  UBYTE *outpos  = outbuf;
  UBYTE *endinp = inpos + inlen;
  UBYTE *window = LZX(window);
  UBYTE *runsrc, *rundest;
  UWORD *hufftbl; /* used in READ_HUFFSYM macro as chosen decoding table */

  ULONG window_posn = LZX(window_posn);
  ULONG last_window_posn = 0;
  ULONG window_size = LZX(window_size);
  ULONG R0 = LZX(R0);
  ULONG R1 = LZX(R1);
  ULONG R2 = LZX(R2);

  register ULONG bitbuf;
  register int bitsleft;
  ULONG match_offset, i,j,k; /* ijk used in READ_HUFFSYM macro */
  struct lzx_bits lb; /* used in READ_LENGTHS macro */

  int togo = outlen, this_run, main_element, aligned_bits;
  int match_length, length_footer, extra, verbatim_bits;
  int space_remaining = outlen, bytes_to_write;

  INIT_BITSTREAM;

  /* main decoding loop */
  while (togo > 0) {
    /* last block finished, new block expected */
    if (LZX(block_remaining) == 0) {
      if (LZX(block_type) == LZX_BLOCKTYPE_UNCOMPRESSED) {
        if (LZX(block_length) & 1) inpos++; /* realign bitstream to word */
        INIT_BITSTREAM;
      }

      /* Moved header read check inside loop and added the code to reset
	 the huffman tables and re-read the Intel preprocessing info
	 every time a window-size boundary is reached. This is actually not 
	 correct, there's a separate parameter for when the tables get reset.
	 But all existing CHM files I've seen have that parameter the same
	 as the window size.  Consider this a FIXME -- MTR */

      if (window_posn == window_size) {
	LZX(header_read) = 0;
	for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) LZX(MAINTREE_len)[i] = 0;
	for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   LZX(LENGTH_len)[i]   = 0;
	R0 = R1 = R2 = 1;
      }
      
      /* read header if necessary */
      if (!LZX(header_read)) {
	i = j = 0;
	READ_BITS(k, 1); if (k) { READ_BITS(i,16); READ_BITS(j,16); }
	LZX(intel_filesize) = (i << 16) | j; /* or 0 if not encoded */
	LZX(header_read) = 1;
      }
      

      READ_BITS(LZX(block_type), 3);
      READ_BITS(i, 16);
      READ_BITS(j, 8);
      LZX(block_remaining) = LZX(block_length) = (i << 8) | j;

      switch (LZX(block_type)) {
      case LZX_BLOCKTYPE_ALIGNED:
        for (i = 0; i < 8; i++) { READ_BITS(j, 3); LENTABLE(ALIGNED)[i] = j; }
        BUILD_TABLE(ALIGNED);
        /* rest of aligned header is same as verbatim */

      case LZX_BLOCKTYPE_VERBATIM:
        READ_LENGTHS(MAINTREE, 0, 256);
        READ_LENGTHS(MAINTREE, 256, LZX(main_elements));
        BUILD_TABLE(MAINTREE);
        if (LENTABLE(MAINTREE)[0xE8] != 0) LZX(intel_started) = 1;

        READ_LENGTHS(LENGTH, 0, LZX_NUM_SECONDARY_LENGTHS);
        BUILD_TABLE(LENGTH);
        break;

      case LZX_BLOCKTYPE_UNCOMPRESSED:
        LZX(intel_started) = 1; /* because we can't assume otherwise */
        ENSURE_BITS(16); /* get up to 16 pad bits into the buffer */
        if (bitsleft > 16) inpos -= 2; /* and align the bitstream! */
        R0=inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        R1=inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        R2=inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        break;

      default:
        return 500+DECR_ILLEGALDATA;
      }
    }

    /* buffer exhaustion check */
    if (inpos > endinp) {
      /* it's possible to have a file where the next run is less than
       * 16 bits in size. In this case, the READ_HUFFSYM() macro used
       * in building the tables will exhaust the buffer, so we should
       * allow for this, but not allow those accidentally read bits to
       * be used (so we check that there are at least 16 bits
       * remaining - in this boundary case they aren't really part of
       * the compressed data)
       */
      if (inpos > (endinp+2) || bitsleft < 16) return 600+DECR_ILLEGALDATA;
    }

    while ((this_run = LZX(block_remaining)) > 0 && togo > 0) {
      if (this_run > togo) this_run = togo;
      togo -= this_run;
      LZX(block_remaining) -= this_run;

      /* apply 2^x-1 mask */
      last_window_posn = window_posn;
      window_posn &= window_size - 1;
      /* runs can't straddle the window wraparound */
      if ((window_posn + this_run) > window_size)
        return DECR_DATAFORMAT;

      switch (LZX(block_type)) {

      case LZX_BLOCKTYPE_VERBATIM:
        while (this_run > 0) {
          READ_HUFFSYM(MAINTREE, main_element);
          if (main_element < LZX_NUM_CHARS) {
            /* literal: 0 to LZX_NUM_CHARS-1 */
            window[window_posn++] = main_element;
            this_run--;
          }
          else {
            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
            main_element -= LZX_NUM_CHARS;
  
            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
              READ_HUFFSYM(LENGTH, length_footer);
              match_length += length_footer;
            }
            match_length += LZX_MIN_MATCH;
  
            match_offset = main_element >> 3;
  
            if (match_offset > 2) {
              /* not repeated offset */
              if (match_offset != 3) {
                extra = extra_bits[match_offset];
                READ_BITS(verbatim_bits, extra);
                match_offset = position_base[match_offset] - 2 + verbatim_bits;
              }
              else {
                match_offset = 1;
              }
  
              /* update repeated offset LRU queue */
              R2 = R1; R1 = R0; R0 = match_offset;
            }
            else if (match_offset == 0) {
              match_offset = R0;
            }
            else if (match_offset == 1) {
              match_offset = R1;
              R1 = R0; R0 = match_offset;
            }
            else /* match_offset == 2 */ {
              match_offset = R2;
              R2 = R0; R0 = match_offset;
            }

            rundest = window + window_posn;
            runsrc  = rundest - match_offset;
            window_posn += match_length;
            this_run -= match_length;

            /* copy any wrapped around source data */
            while ((runsrc < window) && (match_length-- > 0)) {
             *rundest++ = *(runsrc + window_size); runsrc++;
            }
            /* copy match data - no worries about destination wraps */
            while (match_length-- > 0) *rundest++ = *runsrc++;

          }
	  if ((window_posn % 32768 == 0) && (window_posn != 0)) {
	    int b = (bitsleft<16)?bitsleft:(bitsleft-16);
	    REMOVE_BITS(b);
	    }
        }
        break;

      case LZX_BLOCKTYPE_ALIGNED:
        while (this_run > 0) {
          READ_HUFFSYM(MAINTREE, main_element);
  
          if (main_element < LZX_NUM_CHARS) {
            /* literal: 0 to LZX_NUM_CHARS-1 */
            window[window_posn++] = main_element;
            this_run--;
          }
          else {
            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
            main_element -= LZX_NUM_CHARS;
  
            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
              READ_HUFFSYM(LENGTH, length_footer);
              match_length += length_footer;
            }
            match_length += LZX_MIN_MATCH;
  
            match_offset = main_element >> 3;
  
            if (match_offset > 2) {
              /* not repeated offset */
              extra = extra_bits[match_offset];
              match_offset = position_base[match_offset] - 2;
              if (extra > 3) {
                /* verbatim and aligned bits */
                extra -= 3;
                READ_BITS(verbatim_bits, extra);
                match_offset += (verbatim_bits << 3);
                READ_HUFFSYM(ALIGNED, aligned_bits);
                match_offset += aligned_bits;
              }
              else if (extra == 3) {
                /* aligned bits only */
                READ_HUFFSYM(ALIGNED, aligned_bits);
                match_offset += aligned_bits;
              }
              else if (extra > 0) { /* extra==1, extra==2 */
                /* verbatim bits only */
                READ_BITS(verbatim_bits, extra);
                match_offset += verbatim_bits;
              }
              else /* extra == 0 */ {
                /* ??? */
                match_offset = 1;
              }
  
              /* update repeated offset LRU queue */
              R2 = R1; R1 = R0; R0 = match_offset;
            }
            else if (match_offset == 0) {
              match_offset = R0;
            }
            else if (match_offset == 1) {
              match_offset = R1;
              R1 = R0; R0 = match_offset;
            }
            else /* match_offset == 2 */ {
              match_offset = R2;
              R2 = R0; R0 = match_offset;
            }

            rundest = window + window_posn;
            runsrc  = rundest - match_offset;
            window_posn += match_length;
            this_run -= match_length;

            /* copy any wrapped around source data */
            while ((runsrc < window) && (match_length-- > 0)) {
             *rundest++ = *(runsrc + window_size); runsrc++;
            }
            /* copy match data - no worries about destination wraps */
            while (match_length-- > 0) *rundest++ = *runsrc++;

          }
	  if ((window_posn % 32768 == 0) && (window_posn != 0)) {
	    int b = (bitsleft<16)?bitsleft:(bitsleft-16);
	    REMOVE_BITS(b);
	    /* -- MTR -- adjust for 32768 block boundaries */
	  }
        }
        break;

      case LZX_BLOCKTYPE_UNCOMPRESSED:
        if ((inpos + this_run) > endinp) return 700+DECR_ILLEGALDATA;
        memcpy(window + window_posn, inpos, this_run);
        inpos += this_run; window_posn += this_run;
        break;

      default:
        return 800+DECR_ILLEGALDATA; /* might as well */
      }

    }
    if (window_posn > last_window_posn) {
      bytes_to_write = window_posn - last_window_posn;
      if (bytes_to_write > space_remaining) bytes_to_write = space_remaining;
      memcpy(outpos, window + last_window_posn,  bytes_to_write);
      outpos += window_posn - last_window_posn;
      space_remaining -= bytes_to_write;
    }
    else {
      bytes_to_write = window_size - last_window_posn;
      if (bytes_to_write > space_remaining) bytes_to_write = space_remaining;
      memcpy(outpos, window + last_window_posn, bytes_to_write);
      outpos += window_size - last_window_posn;
      space_remaining -= bytes_to_write;
      
      bytes_to_write = window_posn;
      if (bytes_to_write > space_remaining) bytes_to_write = space_remaining;
      
      memcpy(outpos, window,  bytes_to_write);
	fflush(stdout);
      outpos += window_posn;
      space_remaining -= bytes_to_write;
    }
  }

  if (togo != 0) return 900+DECR_ILLEGALDATA;

  LZX(window_posn) = window_posn;
  LZX(R0) = R0;
  LZX(R1) = R1;
  LZX(R2) = R2;

  /* intel E8 decoding */
  if ((LZX(frames_read)++ < 32768) && LZX(intel_filesize) != 0) {
    if (outlen <= 6 || !LZX(intel_started)) {
      LZX(intel_curpos) += outlen;
    }
    else {
      UBYTE *data    = outbuf;
      UBYTE *dataend = data + outlen - 10;
      LONG curpos    = LZX(intel_curpos);
      LONG filesize  = LZX(intel_filesize);
      LONG abs_off, rel_off;

      LZX(intel_curpos) = curpos + outlen;

      while (data < dataend) {
        if (*data++ != 0xE8) { curpos++; continue; }
        abs_off = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
        if ((abs_off >= -curpos) && (abs_off < filesize)) {
          rel_off = (abs_off >= 0) ? abs_off - curpos : abs_off + filesize;
          data[0] = (UBYTE) rel_off;
          data[1] = (UBYTE) (rel_off >> 8);
          data[2] = (UBYTE) (rel_off >> 16);
          data[3] = (UBYTE) (rel_off >> 24);
        }
        data += 4;
        curpos += 5;
      }
    }
  }
  return DECR_OK;
}
