/***************************************************************************
*                     Rice Encoding/Decoding Functions
*
*   File    : rice.c
*   Purpose : Provide a functions for Rice encoding and decoding of file
*             streams.
*   Author  : Michael Dipperstein
*   Date    : January 23, 2008S
*
****************************************************************************
*   UPDATES
*
*   $Id: rice.c,v 1.2 2008/01/25 07:26:13 michael Exp $
*   $Log: rice.c,v $
*   Revision 1.2  2008/01/25 07:26:13  michael
*   Added CVS log.
*
****************************************************************************
*
* Rice: ANSI C Rice Encoding/Decoding Routines
* Copyright (C) 2008 by Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the rice library.
*
* The rice library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The rice library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "rice.h"
#include "bitfile.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                                  MACROS
***************************************************************************/

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : RiceEncodeFile
*   Description: This routine reads an input file 1 character at a time and
*                writes out a Rice encoded version of that file.
*   Parameters : inFile - Name of file to encode
*                outFile - Name of file to write encoded output to
*                k - length of binary portion of encoded word
*   Effects    : File is encoded using the Rice algorithm with a k bit
*                binary portion.
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int RiceEncodeFile(char *inFile, char *outFile, unsigned char k)
{
    FILE *fpIn;                         /* uncoded input */
    bit_file_t *bfpOut;                 /* encoded output */
    unsigned char unary, binary;        /* unary & binary portions */
    unsigned char mask;                 /* mask for binary portion */
    int c;

    /* open input and output files */
    if (NULL == (fpIn = fopen(inFile, "rb")))
    {
        perror(inFile);
        return FALSE;
    }

    if (NULL == outFile)
    {
        bfpOut = MakeBitFile(stdout, BF_WRITE);
    }
    else
    {
        if (NULL == (bfpOut = BitFileOpen(outFile, BF_WRITE)))
        {
            fclose(fpIn);
            perror(outFile);
            return FALSE;
        }
    }

    mask =  0xFF >> (CHAR_BIT - k);

    /* encode input file one byte at a time */
    while ((c = fgetc(fpIn)) != EOF)
    {
        /* compute the unary portion */
        unary = (unsigned char)c;
        unary >>= k;

        while (unary > 0)
        {
            /* write out unary worth of 1s */
            unary--;
            BitFilePutBit(1, bfpOut);
        }

        /* write an ending 0 */
        BitFilePutBit(0, bfpOut);

        /* binary portion */
        binary = (unsigned char)c & mask;
        binary <<= (CHAR_BIT - k);      /* right justify bits */
        BitFilePutBits(bfpOut, &binary, k);
    }

    /* pad fill with 1s so decode will run into EOF */
    BitFileFlushOutput(bfpOut, TRUE);

    fclose(fpIn);
    BitFileClose(bfpOut);
    return TRUE;
}

/***************************************************************************
*   Function   : RiceDecodeFile
*   Description: This routine reads a rice encoded input file and writes
*                the decoded output one byte at a time.
*   Parameters : inFile - Name of file to decode
*                outFile - Name of file to write decoded output to
*                k - length of binary portion of encoded word
*   Effects    : File is decoded using the Rice algorithm for codes with a
*                k bit binary portion.
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int RiceDecodeFile(char *inFile, char *outFile, unsigned char k)
{
    bit_file_t *bfpIn;                  /* encoded input */
    FILE *fpOut;                        /* decoded output */
    int bit;
    unsigned char tmp, byte;

    /* open input and output files */
    if (NULL == (bfpIn = BitFileOpen(inFile, BF_READ)))
    {
        perror(inFile);
        return FALSE;
    }

    if (NULL == outFile)
    {
        fpOut = stdout;
    }
    else
    {
        if (NULL == (fpOut = fopen(outFile, "wb")))
        {
            BitFileClose(bfpIn);
            perror(outFile);
            return FALSE;
        }
    }

    byte = 0;

    /* decode input file */
    while ((bit = BitFileGetBit(bfpIn)) != EOF)
    {
        if (1 == bit)
        {
            byte++;
        }
        else
        {
            /* finished unary portion */
            tmp = byte << k;

            if (EOF == BitFileGetBits(bfpIn, &byte, k))
            {
                /* unary was actually spare bits */
                break;
            }

            byte >>= (CHAR_BIT - k);        /* leftt justify bits */
            byte |= tmp;
            fputc(byte, fpOut);

            byte = 0;
        }
    }

    fclose(fpOut);
    BitFileClose(bfpIn);
    return TRUE;
}
