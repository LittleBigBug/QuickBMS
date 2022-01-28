/***************************************************************************
*                Common Huffman Encoding and Decoding Header
*
*   File    : huflocal.h
*   Purpose : Common constants, types, and prototypes not used by caller of
*             public Huffman library functions.
*   Author  : Michael Dipperstein
*   Date    : May 21, 2005
*
****************************************************************************
*
* Huffman: An ANSI C Huffman Encoding/Decoding Routine
* Copyright (C) 2005, 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Huffman library.
*
* The Huffman library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The Huffman library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/
#ifndef _HUFFMAN_LOCAL_H
#define _HUFFMAN_LOCAL_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <limits.h>

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
/* use preprocessor to verify type lengths */
#if (UCHAR_MAX != 0xFF)
#error This program expects unsigned char to be 1 byte
#endif

#if (UINT_MAX != 0xFFFFFFFF)
#error This program expects unsigned int to be 4 bytes
#endif

/* system dependent types */
typedef unsigned char byte_t;       /* unsigned 8 bit */
typedef unsigned int count_t;       /* unsigned 32 bit for character counts */

typedef struct huffman_node_t
{
    int value;          /* character(s) represented by this entry */
    count_t count;      /* number of occurrences of value (probability) */

    char ignore;        /* TRUE -> already handled or no need to handle */
    int level;          /* depth in tree (root is 0) */

    /***********************************************************************
    *  pointer to children and parent.
    *  NOTE: parent is only useful if non-recursive methods are used to
    *        search the huffman tree.
    ***********************************************************************/
    struct huffman_node_t *left, *right, *parent;
} huffman_node_t;

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define NONE    -1

#define COUNT_T_MAX     UINT_MAX    /* based on count_t being unsigned int */

#define COMPOSITE_NODE      -1      /* node represents multiple characters */
#define NUM_CHARS   (UCHAR_MAX + 2) /* Add an extra char for EOF */
#define EOF_CHAR    (NUM_CHARS - 1) /* index used for EOF */

/***************************************************************************
*                                 MACROS
***************************************************************************/
#define max(a, b) ((a)>(b)?(a):(b))

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* create/destroy tree */
huffman_node_t *GenerateTreeFromFile(FILE *inFile);
huffman_node_t *BuildHuffmanTree(huffman_node_t **ht, int elements);
huffman_node_t *AllocHuffmanNode(int value);
void FreeHuffmanTree(huffman_node_t *ht);

#endif  /* define _HUFFMAN_LOCAL_H */
