/***************************************************************************
*          Lempel, Ziv, Storer, and Szymanski Encoding and Decoding
*
*   File    : lzlocal.h
*   Purpose : Internal headers for LZSS encode and decode routines.
*             Contains the prototypes to be used by the different match
*             finding algorithms.
*   Author  : Michael Dipperstein
*   Date    : February 18, 2004
*
****************************************************************************
*
* LZSS: An ANSI C LZSS Encoding/Decoding Routine
* Copyright (C) 2004 - 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the lzss library.
*
* The lzss library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The lzss library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/
#ifndef _LZSS_LOCAL_H
#define _LZSS_LOCAL_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <limits.h>

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

#define OFFSET_BITS     12
#define LENGTH_BITS     4

#if (((1 << (OFFSET_BITS + LENGTH_BITS)) - 1) > UINT_MAX)
#error "Size of encoded data must not exceed the size of an unsigned int"
#endif

/* We want a sliding window*/
#define WINDOW_SIZE     (1 << OFFSET_BITS)

/* maximum match length not encoded and maximum length encoded (4 bits) */
#define MAX_UNCODED     2
#define MAX_CODED       ((1 << LENGTH_BITS) + MAX_UNCODED)

#define ENCODED     0       /* encoded string */
#define UNCODED     1       /* unencoded character */

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
* This data structure stores an encoded string in (offset, length) format.
* The actual encoded string is stored using OFFSET_BITS for the offset and
* LENGTH_BITS for the length.
***************************************************************************/
typedef struct encoded_string_t
{
    unsigned int offset;    /* offset to start of longest match */
    unsigned int length;    /* length of longest match */
} encoded_string_t;

/***************************************************************************
*                                 MACROS
***************************************************************************/
/* wraps array index within array bounds (assumes value < 2 * limit) */
#define Wrap(value, limit) \
    (((value) < (limit)) ? (value) : ((value) - (limit)))

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
* These are the prototypes for functions that must be provided by any
* methods for maintaining and searching the sliding window dictionary.
*
* InitializeSearchStructures and ReplaceChar return 0 for success and -1
* for a failure.  errno will be set in the event of a failure.
*
* FindMatch will return the encoded_string_t value referencing the match
* in the sliding window dictionary.  the length field will be 0 if no
* match is found.
***************************************************************************/
int InitializeSearchStructures(void);
int ReplaceChar(const unsigned int charIndex, const unsigned char replacement);

encoded_string_t FindMatch(const unsigned int windowHead,
    const unsigned int uncodedHead);

#endif      /* ndef _LZSS_LOCAL_H */
