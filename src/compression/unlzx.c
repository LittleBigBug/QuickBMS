/* $VER: unlzx.c 1.0 (22.2.98) */
/* Created: 11.2.98 */
/* Added Pipe support to read from stdin (03.4.01, Erik Meusel)           */

/* LZX Extract in (supposedly) portable C.                                */

/* ********************* WINDOWS PORTING NOTES STARTS *********************/

/* Ported to Windows Platform by Stefano Coletta on 22.5.02               */
/* creator@mindcreations.com or visit http://creator.mindcreations.com    */

/* Changes to original code include:                                      */
/* - Using #include <direct.h> to substitute the mkdir() function call    */
/* - Added #include "getopt.h" to support getopt() function               */

/* Compile with:                                                          */
/* Microsoft Visual Studio 6.0 SP5 using supplied project file            */

/* ********************** WINDOWS PORTING NOTES END ***********************/


/* Thanks to Dan Fraser for decoding the coredumps and helping me track   */
/* down some HIDEOUSLY ANNOYING bugs.                                     */

/* Everything is accessed as unsigned char's to try and avoid problems    */
/* with byte order and alignment. Most of the decrunch functions          */
/* encourage overruns in the buffers to make things as fast as possible.  */
/* All the time is taken up in crc_calc() and decrunch() so they are      */
/* pretty damn optimized. Don't try to understand this program.           */

/* ---------------------------------------------------------------------- */

/* - minimal UAE specific version 13-14.06.2007 by Toni Wilen */

#include <stdlib.h>
#include <stdio.h>

//#include "sysconfig.h"
//#include "sysdeps.h"

//#include "zfile.h"
//#include "zarchive.h"

/* ---------------------------------------------------------------------- */

static unsigned char *source;
static unsigned char *destination;
static unsigned char *source_end;
static unsigned char *destination_end;

static unsigned int decrunch_method;
static unsigned int decrunch_length;
static unsigned int last_offset;
static unsigned int global_control;
static int global_shift;

static unsigned char offset_len[8];
static unsigned short offset_table[128];
static unsigned char huffman20_len[20];
static unsigned short huffman20_table[96];
static unsigned char literal_len[768];
static unsigned short literal_table[5120];

/* ---------------------------------------------------------------------- */

//static unsigned int sum;

/* ---------------------------------------------------------------------- */

static const unsigned char table_one[32]=
{
 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14
};

static const unsigned int table_two[32]=
{
 0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,
 1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152
};

static const unsigned int table_three[16]=
{
 0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767
};

static const unsigned char table_four[34]=
{
 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
};

/* ---------------------------------------------------------------------- */

/* Possible problems with 64 bit machines here. It kept giving warnings   */
/* for people so I changed back to ~.                                     */

/* ---------------------------------------------------------------------- */

/* Build a fast huffman decode table from the symbol bit lengths.         */
/* There is an alternate algorithm which is faster but also more complex. */

static int make_decode_table(int number_symbols, int table_size,
		      unsigned char *length, unsigned short *table)
{
 register unsigned char bit_num = 0;
 register int symbol;
 unsigned int leaf; /* could be a register */
 unsigned int table_mask, bit_mask, pos, fill, next_symbol, reverse;
 int myabort = 0;

 pos = 0; /* consistantly used as the current position in the decode table */

 bit_mask = table_mask = 1 << table_size;

 bit_mask >>= 1; /* don't do the first number */
 bit_num++;

 while((!myabort) && (bit_num <= table_size))
 {
  for(symbol = 0; symbol < number_symbols; symbol++)
  {
   if(length[symbol] == bit_num)
   {
    reverse = pos; /* reverse the order of the position's bits */
    leaf = 0;
    fill = table_size;
    do /* reverse the position */
    {
     leaf = (leaf << 1) + (reverse & 1);
     reverse >>= 1;
    } while(--fill);
    if((pos += bit_mask) > table_mask)
    {
     myabort = 1;
     break; /* we will overrun the table! myabort! */
    }
    fill = bit_mask;
    next_symbol = 1 << bit_num;
    do
    {
     table[leaf] = symbol;
     leaf += next_symbol;
    } while(--fill);
   }
  }
  bit_mask >>= 1;
  bit_num++;
 }

 if((!myabort) && (pos != table_mask))
 {
  for(symbol = pos; symbol < table_mask; symbol++) /* clear the rest of the table */
  {
   reverse = symbol; /* reverse the order of the position's bits */
   leaf = 0;
   fill = table_size;
   do /* reverse the position */
   {
    leaf = (leaf << 1) + (reverse & 1);
    reverse >>= 1;
   } while(--fill);
   table[leaf] = 0;
  }
  next_symbol = table_mask >> 1;
  pos <<= 16;
  table_mask <<= 16;
  bit_mask = 32768;

  while((!myabort) && (bit_num <= 16))
  {
   for(symbol = 0; symbol < number_symbols; symbol++)
   {
    if(length[symbol] == bit_num)
    {
     reverse = pos >> 16; /* reverse the order of the position's bits */
     leaf = 0;
     fill = table_size;
     do /* reverse the position */
     {
      leaf = (leaf << 1) + (reverse & 1);
      reverse >>= 1;
     } while(--fill);
     for(fill = 0; fill < bit_num - table_size; fill++)
     {
      if(table[leaf] == 0)
      {
       table[(next_symbol << 1)] = 0;
       table[(next_symbol << 1) + 1] = 0;
       table[leaf] = next_symbol++;
      }
      leaf = table[leaf] << 1;
      leaf += (pos >> (15 - fill)) & 1;
     }
     table[leaf] = symbol;
     if((pos += bit_mask) > table_mask)
     {
      myabort = 1;
      break; /* we will overrun the table! myabort! */
     }
    }
   }
   bit_mask >>= 1;
   bit_num++;
  }
 }
 if(pos != table_mask) myabort = 1; /* the table is incomplete! */

 return(myabort);
}

/* ---------------------------------------------------------------------- */

/* Read and build the decrunch tables. There better be enough data in the */
/* source buffer or it's stuffed. */

static int read_literal_table()
{
 register unsigned int control;
 register int shift;
 unsigned int temp; /* could be a register */
 unsigned int symbol, pos, count, fix, max_symbol;
 int myabort = 0;

 control = global_control;
 shift = global_shift;

 if(shift < 0) /* fix the control word if necessary */
 {
  shift += 16;
  control += *source++ << (8 + shift);
  control += *source++ << shift;
 }

/* read the decrunch method */

 decrunch_method = control & 7;
 control >>= 3;
 if((shift -= 3) < 0)
 {
  shift += 16;
  control += *source++ << (8 + shift);
  control += *source++ << shift;
 }

/* Read and build the offset huffman table */

 if((!myabort) && (decrunch_method == 3))
 {
  for(temp = 0; temp < 8; temp++)
  {
   offset_len[temp] = control & 7;
   control >>= 3;
   if((shift -= 3) < 0)
   {
    shift += 16;
    control += *source++ << (8 + shift);
    control += *source++ << shift;
   }
  }
  myabort = make_decode_table(8, 7, offset_len, offset_table);
 }

/* read decrunch length */

 if(!myabort)
 {
  decrunch_length = (control & 255) << 16;
  control >>= 8;
  if((shift -= 8) < 0)
  {
   shift += 16;
   control += *source++ << (8 + shift);
   control += *source++ << shift;
  }
  decrunch_length += (control & 255) << 8;
  control >>= 8;
  if((shift -= 8) < 0)
  {
   shift += 16;
   control += *source++ << (8 + shift);
   control += *source++ << shift;
  }
  decrunch_length += (control & 255);
  control >>= 8;
  if((shift -= 8) < 0)
  {
   shift += 16;
   control += *source++ << (8 + shift);
   control += *source++ << shift;
  }
 }

/* read and build the huffman literal table */

 if((!myabort) && (decrunch_method != 1))
 {
  pos = 0;
  fix = 1;
  max_symbol = 256;

  do
  {
   for(temp = 0; temp < 20; temp++)
   {
    huffman20_len[temp] = control & 15;
    control >>= 4;
    if((shift -= 4) < 0)
    {
     shift += 16;
     control += *source++ << (8 + shift);
     control += *source++ << shift;
    }
   }
   myabort = make_decode_table(20, 6, huffman20_len, huffman20_table);

   if(myabort) break; /* argh! table is corrupt! */

   do
   {
    if((symbol = huffman20_table[control & 63]) >= 20)
    {
     do /* symbol is longer than 6 bits */
     {
      symbol = huffman20_table[((control >> 6) & 1) + (symbol << 1)];
      if(!shift--)
      {
       shift += 16;
       control += *source++ << 24;
       control += *source++ << 16;
      }
      control >>= 1;
     } while(symbol >= 20);
     temp = 6;
    }
    else
    {
     temp = huffman20_len[symbol];
    }
    control >>= temp;
    if((shift -= temp) < 0)
    {
     shift += 16;
     control += *source++ << (8 + shift);
     control += *source++ << shift;
    }
    switch(symbol)
    {
     case 17:
     case 18:
     {
      if(symbol == 17)
      {
       temp = 4;
       count = 3;
      }
      else /* symbol == 18 */
      {
       temp = 6 - fix;
       count = 19;
      }
      count += (control & table_three[temp]) + fix;
      control >>= temp;
      if((shift -= temp) < 0)
      {
       shift += 16;
       control += *source++ << (8 + shift);
       control += *source++ << shift;
      }
      while((pos < max_symbol) && (count--))
       literal_len[pos++] = 0;
      break;
     }
     case 19:
     {
      count = (control & 1) + 3 + fix;
      if(!shift--)
      {
       shift += 16;
       control += *source++ << 24;
       control += *source++ << 16;
      }
      control >>= 1;
      if((symbol = huffman20_table[control & 63]) >= 20)
      {
       do /* symbol is longer than 6 bits */
       {
	symbol = huffman20_table[((control >> 6) & 1) + (symbol << 1)];
	if(!shift--)
	{
	 shift += 16;
	 control += *source++ << 24;
	 control += *source++ << 16;
	}
	control >>= 1;
       } while(symbol >= 20);
       temp = 6;
      }
      else
      {
       temp = huffman20_len[symbol];
      }
      control >>= temp;
      if((shift -= temp) < 0)
      {
       shift += 16;
       control += *source++ << (8 + shift);
       control += *source++ << shift;
      }
      symbol = table_four[literal_len[pos] + 17 - symbol];
      while((pos < max_symbol) && (count--))
       literal_len[pos++] = symbol;
      break;
     }
     default:
     {
      symbol = table_four[literal_len[pos] + 17 - symbol];
      literal_len[pos++] = symbol;
      break;
     }
    }
   } while(pos < max_symbol);
   fix--;
   max_symbol += 512;
  } while(max_symbol == 768);

  if(!myabort)
   myabort = make_decode_table(768, 12, literal_len, literal_table);
 }

 global_control = control;
 global_shift = shift;

 return(myabort);
}

/* ---------------------------------------------------------------------- */

/* Fill up the decrunch buffer. Needs lots of overrun for both destination */
/* and source buffers. Most of the time is spent in this routine so it's  */
/* pretty damn optimized. */

static void decrunch(void)
{
 register unsigned int control;
 register int shift;
 unsigned int temp; /* could be a register */
 unsigned int symbol, count;
 unsigned char *string;

 control = global_control;
 shift = global_shift;

 do
 {
  if((symbol = literal_table[control & 4095]) >= 768)
  {
   control >>= 12;
   if((shift -= 12) < 0)
   {
    shift += 16;
    control += *source++ << (8 + shift);
    control += *source++ << shift;
   }
   do /* literal is longer than 12 bits */
   {
    symbol = literal_table[(control & 1) + (symbol << 1)];
    if(!shift--)
    {
     shift += 16;
     control += *source++ << 24;
     control += *source++ << 16;
    }
    control >>= 1;
   } while(symbol >= 768);
  }
  else
  {
   temp = literal_len[symbol];
   control >>= temp;
   if((shift -= temp) < 0)
   {
    shift += 16;
    control += *source++ << (8 + shift);
    control += *source++ << shift;
   }
  }
  if(symbol < 256)
  {
   *destination++ = symbol;
  }
  else
  {
   symbol -= 256;
   count = table_two[temp = symbol & 31];
   temp = table_one[temp];
   if((temp >= 3) && (decrunch_method == 3))
   {
    temp -= 3;
    count += ((control & table_three[temp]) << 3);
    control >>= temp;
    if((shift -= temp) < 0)
    {
     shift += 16;
     control += *source++ << (8 + shift);
     control += *source++ << shift;
    }
    count += (temp = offset_table[control & 127]);
    temp = offset_len[temp];
   }
   else
   {
    count += control & table_three[temp];
    if(!count) count = last_offset;
   }
   control >>= temp;
   if((shift -= temp) < 0)
   {
    shift += 16;
    control += *source++ << (8 + shift);
    control += *source++ << shift;
   }
   last_offset = count;

   count = table_two[temp = (symbol >> 5) & 15] + 3;
   temp = table_one[temp];
   count += (control & table_three[temp]);
   control >>= temp;
   if((shift -= temp) < 0)
   {
    shift += 16;
    control += *source++ << (8 + shift);
    control += *source++ << shift;
   }
   string = destination - last_offset;
   do
   {
    *destination++ = *string++;
   } while(--count);
  }
 } while((destination < destination_end) && (source < source_end));

 global_control = control;
 global_shift = shift;
}



int unlzx(unsigned char *in, int insz, unsigned char *out, int outsz) {
    source = in;
    source_end = in + insz;
    global_control = 0;
    global_shift = -16;
    last_offset = 1;
    destination = out;
    if(read_literal_table()) return(-1);
    destination_end = out + outsz;
    decrunch();
    return(destination - out);
}


