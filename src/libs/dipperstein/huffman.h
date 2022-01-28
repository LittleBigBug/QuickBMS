/***************************************************************************
*                        Huffman Library Header File
*
*   File    : huffman.h
*   Purpose : Provide header file for programs linking to Huffman library
*             functions.
*   Author  : Michael Dipperstein
*   Date    : February 25, 2004
*
****************************************************************************
*
* Huffman: An ANSI C Huffman Encoding/Decoding Routine
* Copyright (C) 2004, 2007, 2014 by
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

#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* traditional codes */
int HuffmanShowTree(FILE *inFile, FILE *outFile);       /* dump codes */
int HuffmanEncodeFile(FILE *inFile, FILE *outFile);     /* encode file */
int HuffmanDecodeFile(FILE *inFile, FILE *outFile);     /* decode file */

/* canonical code */
int CanonicalShowTree(FILE *inFile, FILE *outFile);     /* dump codes */
int CanonicalEncodeFile(FILE *inFile, FILE *outFile);   /* encode file */
int CanonicalDecodeFile(FILE *inFile, FILE *outFile);   /* decode file */

#endif /* _HUFFMAN_H_ */
