/***************************************************************************
*            Header for Arithmetic Encoding and Decoding Library
*
*   File    : arcode.h
*   Purpose : Provides prototypes for functions that use arithmetic coding
*             to encode/decode files.
*   Author  : Michael Dipperstein
*   Date    : April 2, 2004
*
****************************************************************************
*
* Arcode: An ANSI C Arithmetic Encoding/Decoding Routines
* Copyright (C) 2004, 2006-2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the arcode library.
*
* The arcode library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The arcode library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

#ifndef _ARCODE_H_
#define _ARCODE_H_

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef enum
{
    MODEL_ADAPTIVE = 0,
    MODEL_STATIC = 1
} model_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
 /* encode/decode routines from inFile to outFile.  returns 0 on success */
int ArEncodeFile(FILE *inFile, FILE *outFile, const model_t model);
int ArDecodeFile(FILE *inFile, FILE *outFile, const model_t model);

#endif  /* ndef _ARCODE_H_ */
