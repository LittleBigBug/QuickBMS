/***************************************************************************
*          Header for Adaptive Delta Encoding and Decoding Library
*
*   File    : adapt.h
*   Purpose : Provides prototypes for functions that compute the code word
*             size for adaptive delta encoding/decoding.
*   Author  : Michael Dipperstein
*   Date    : April 29, 2009
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

#ifndef _ADAPT_H_
#define _ADAPT_H_

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef enum
{
    CS_OKAY,
    CS_OVERFLOW,
    CS_UNDERFLOW
} code_word_stat_t;

#ifndef _ADAPT_C_
typedef struct adaptive_data_t adaptive_data_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* create and free data structures used for adaptive code size computations */
adaptive_data_t* CreateAdaptiveData(const unsigned char codeSize);
void FreeAdaptiveData(adaptive_data_t *data);

/* returns code size for next code word based on fit of current code word */
unsigned char UpdateAdaptiveStatistics(adaptive_data_t *data,
    const code_word_stat_t stat);
#endif  /* ndef _ADAPT_C_ */
#endif  /* ndef _ADAPT_H_ */
