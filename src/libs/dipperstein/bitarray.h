/***************************************************************************
*                         Arrays of Arbitrary Bit Length
*
*   File    : bitarray.h
*   Purpose : Header file for library supporting the creation and
*             manipulation of arbitrary length arrays of bits.
*   Author  : Michael Dipperstein
*   Date    : January 30, 2004
*
****************************************************************************
*   HISTORY
*
*   $Id: bitarray.h,v 1.2 2007/08/26 21:20:20 michael Exp $
*   $Log: bitarray.h,v $
*   Revision 1.2  2007/08/26 21:20:20  michael
*   Changes required for LGPL v3.
*
*   Revision 1.1.1.1  2004/02/09 04:15:45  michael
*   Initial release
*
*
****************************************************************************
*
* Bitarray: An ANSI C library for manipulating arbitrary length bit arrays
* Copyright (C) 2004, 2006-2007 by Michael Dipperstein (mdipper@cs.ucsb.edu)
*
* This file is part of the bit array library.
*
* The bit array library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The bit array library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/
#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
/* incomplete type to hide implementation */
struct bit_array_t;
typedef struct bit_array_t bit_array_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* create/destroy functions */
bit_array_t *BitArrayCreate(unsigned int bits);
void BitArrayDestroy(bit_array_t *ba);

/* debug functions */
void BitArrayDump(bit_array_t *ba, FILE *outFile);

/* set/clear functions */
void BitArraySetAll(bit_array_t *ba);
void BitArrayClearAll(bit_array_t *ba);
void BitArraySetBit(bit_array_t *ba, unsigned int bit);
void BitArrayClearBit(bit_array_t *ba, unsigned int bit);

/* raw bit access */
void *BitArrayGetBits(bit_array_t *ba);

/* bit test function */
int BitArrayTestBit(bit_array_t *ba, unsigned int bit);

/* copy functions */
void BitArrayCopy(bit_array_t *dest, const bit_array_t *src);
bit_array_t *BitArrayDuplicate(const bit_array_t *src);

/* logical operations */
void BitArrayAnd(bit_array_t *dest,
                 const bit_array_t *src1,
                 const bit_array_t *src2);

void BitArrayOr(bit_array_t *dest,
           const bit_array_t *src1,
           const bit_array_t *src2);

void BitArrayXor(bit_array_t *dest,
            const bit_array_t *src1,
            const bit_array_t *src2);

void BitArrayNot(bit_array_t *dest,
            const bit_array_t *src);

/* bit shift functions */
void BitArrayShiftLeft(bit_array_t *ba, unsigned int shifts);
void BitArrayShiftRight(bit_array_t *ba, unsigned int shifts);

/* increment/decrement */
void BitArrayIncrement(bit_array_t *ba);
void BitArrayDecrement(bit_array_t *ba);

/* comparison */
int BitArrayCompare(const bit_array_t *ba1, const bit_array_t *ba2);

#endif  /* ndef BIT_ARRAY_H */
