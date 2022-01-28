/***************************************************************************
*            Header for Run Length Encoding and Decoding Library
*
*   File    : rle.h
*   Purpose : Provides prototypes for functions that use run length coding
*             to encode/decode files.
*   Author  : Michael Dipperstein
*   Date    : April 30, 2004
*
****************************************************************************
*   UPDATES
*
*   $Id: rle.h,v 1.3 2007/09/08 17:10:24 michael Exp $
*   $Log: rle.h,v $
*   Revision 1.3  2007/09/08 17:10:24  michael
*   Changes required for LGPL v3.
*
*   Revision 1.2  2006/09/10 05:08:36  michael
*   Add prototypes for variant packbits algorithm.
*
*   Revision 1.1.1.1  2004/05/03 03:56:49  michael
*   Initial version
*
*
****************************************************************************
*
* RLE: An ANSI C Run Length Encoding/Decoding Routines
* Copyright (C) 2004, 2006-2007 by
*       Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the RLE library.
*
* The RLE library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation; either version 3 of the License, or (at
* your option) any later version.
*
* The RLE library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

#ifndef _RLE_H_
#define _RLE_H_

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* traditional RLE encodeing/decoding */
int RleEncodeFile(char *inFile, char *outFile);
int RleDecodeFile(char *inFile, char *outFile);

/* variant of packbits RLE encodeing/decoding */
int VPackBitsEncodeFile(char *inFile, char *outFile);
int VPackBitsDecodeFile(char *inFile, char *outFile);

#endif  /* ndef _RLE_H_ */
