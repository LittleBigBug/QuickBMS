/***************************************************************************
*          Lempel, Ziv, Storer, and Szymanski Encoding and Decoding
*
*   File    : lzss.h
*   Purpose : Header for LZSS encode and decode routines.  Contains the
*             prototypes to be used by programs linking to the LZSS
*             library.
*   Author  : Michael Dipperstein
*   Date    : February 21, 2004
*
****************************************************************************
*
* LZSS: An ANSI C LZSS Encoding/Decoding Routine
* Copyright (C) 2004, 2006, 2007, 2014 by
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
#ifndef _LZSS_H
#define _LZSS_H

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
* LZSS encoding and decoding prototypes for functions with file pointer
* parameters.  Provide these functions with a pointer to the open binary
* file to be encoded/decoded (fpIn) and pointer to the open binary target
* file (fpOut).  It is the job of the function caller to open the files
* prior to callings these functions and to close the file after these
* functions have been called.
*
* These functions return 0 for success and -1 for failure.  errno will be
* set in the event of a failure. 
***************************************************************************/
int EncodeLZSS(FILE *fpIn, FILE *fpOut);
int DecodeLZSS(FILE *fpIn, FILE *fpOut);

#endif      /* ndef _LZSS_H */
