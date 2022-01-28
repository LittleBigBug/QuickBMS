/***************************************************************************
*          Header for Lempel-Ziv-Welch Encoding and Decoding Library
*
*   File    : lzw.h
*   Purpose : Provides prototypes for functions that use Lempel-Ziv-Welch
*             coding to encode/decode files.
*   Author  : Michael Dipperstein
*   Date    : January 30, 2004
*
****************************************************************************
*
* LZW: An ANSI C Lempel-Ziv-Welch Encoding/Decoding Routines
* Copyright (C) 2005, 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the lzw library.
*
* The lzw library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The lzw library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

#ifndef _LZW_H_
#define _LZW_H_

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
 /* encode inFile */
int LZWEncodeFile(FILE *fpIn, FILE *fpOut);

/* decode inFile*/
int LZWDecodeFile(FILE *fpIn, FILE *fpOut);

#endif  /* ndef _LZW_H_ */
