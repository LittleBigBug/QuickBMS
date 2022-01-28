/***************************************************************************
*             Frequency Substitution Encoding/Decoding Functions
*
*   File    : freqsub.h
*   Purpose : Provides prototypes for functions that perform Frequency
*             Substitution encoding and decoding of file streams.
*
*             Two forms are provided a normal form and a "sparse" form.
*             The sparse form is intended for files that use less than
*             half of the available symbol set.  It encodes the
*             substitution key as a (symbol offset, code) pair.  The
*             normal from encodes the key as a position based table of the
*             entire symbol set (even the unused symbols).
*
*   Author  : Michael Dipperstein
*   Date    : December 21, 2008
*
****************************************************************************
*
* freqsub: An ANSI C Frequency Substitution Encoding/Decoding Library
*          Example
* Copyright (C) 2008, 2014 by
*   Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Frequency Substitution library.
*
* The Frequency Substitution library is free software; you can redistribute
* it and/or modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either version 3 of
* the License, or (at your option) any later version.
*
* The Frequency Substitution library is distributed in the hope that it will
* be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/
#ifndef _FREQSUB_H_
#define _FREQSUB_H_


/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* encode/decode for files using most symbols */
int FreqEncodeFile(FILE *inFile, FILE *outFile);
int FreqDecodeFile(FILE *inFile, FILE *outFile);

#endif  /* ndef _FREQSUB_H_ */
