/***************************************************************************
*              Header for Delta Encoding and Decoding Library
*
*   File    : delta.h
*   Purpose : Provides prototypes for functions that ues adaptive delta
*             coding to encode/decode files.
*   Author  : Michael Dipperstein
*   Date    : April 16, 2009
*
****************************************************************************
*
* Delta: An adaptive delta encoding/decoding library
* Copyright (C) 2009, 2014 by
*       Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Delta library.
*
* Delta is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the
* Free Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* Delta is distributed in the hope that it will be useful, but WITHOUT ANY
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

#ifndef _DELTA_H_
#define _DELTA_H_

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
 /* encode inFile */
int DeltaEncodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize);

/* decode inFile*/
int DeltaDecodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize);

#endif  /* ndef _DELTA_H_ */
