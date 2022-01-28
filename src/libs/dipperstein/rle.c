/***************************************************************************
*                 Run Length Encoding and Decoding Library
*
*   File    : rle.c
*   Purpose : Use run length coding to compress/decompress files.  The
*             version of encoding used by this library only provides a run
*             length if the last two symbols are matching.  This method
*             avoids the need to include run lengths for runs of only 1
*             symbol.  It also avoids the need for escape characters.
*   Author  : Michael Dipperstein
*   Date    : April 30, 2004
*
****************************************************************************
*   UPDATES
*
*   $Id: rle.c,v 1.2 2007/09/08 17:10:24 michael Exp $
*   $Log: rle.c,v $
*   Revision 1.2  2007/09/08 17:10:24  michael
*   Changes required for LGPL v3.
*
*   Revision 1.1.1.1  2004/05/03 03:56:49  michael
*   Initial version
*
****************************************************************************
*
* RLE: An ANSI C Run Length Encoding/Decoding Routines
* Copyright (C) 2004, 2007 by
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

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <limits.h>

#include "../../extra/mem2mem.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define FALSE       0
#define TRUE        1

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : RleEncodeFile
*   Description: This routine reads an input file and writes out a run
*                length encoded version of that file.
*   Parameters : inFile - Name of file to encode
*                outFile - Name of file to write encoded output to
*   Effects    : File is encoded using RLE
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int RleEncodeFile(char *inFile, char *outFile)
{
    FILE *fpIn;                         /* uncoded input */
    FILE *fpOut;                        /* encoded output */
    int currChar, prevChar;             /* current and previous characters */
    unsigned char count;                /* number of characters in a run */

    /* open input and output files */
    if ((fpIn = fopen(inFile, "rb")) == NULL)
    {
        perror(inFile);
        return FALSE;
    }

    if (outFile == NULL)
    {
        fpOut = stdout;     /* default to stdout */
    }
    else
    {
        if ((fpOut = fopen(outFile, "wb")) == NULL)
        {
            fclose(fpIn);
            perror(outFile);
            return FALSE;
        }
    }

    /* encode inFile */
    prevChar = EOF;     /* force next char to be different */
    count = 0;

    /* read input until there's nothing left */
    while ((currChar = fgetc(fpIn)) != EOF)
    {
        fputc(currChar, fpOut);

        /* check for run */
        if (currChar == prevChar)
        {
            /* we have a run.  count run length */
            count = 0;

            while ((currChar = fgetc(fpIn)) != EOF)
            {
                if (currChar == prevChar)
                {
                    count++;

                    if (count == UCHAR_MAX)
                    {
                        /* count is as long as it can get */
                        fputc(count, fpOut);

                        /* force next char to be different */
                        prevChar = EOF;
                        break;
                    }
                }
                else
                {
                    /* run ended */
                    fputc(count, fpOut);
                    fputc(currChar, fpOut);
                    prevChar = currChar;
                    break;
                }
            }
        }
        else
        {
            /* no run */
            prevChar = currChar;
        }

        if (currChar == EOF)
        {
            /* run ended because of EOF */
            fputc(count, fpOut);
            break;
        }
    }

    /* close all open files */
    fclose(fpOut);
    fclose(fpIn);

    return TRUE;
}

/***************************************************************************
*   Function   : RleDecodeFile
*   Description: This routine opens a run length encoded file, and decodes
*                it to an output file.
*   Parameters : inFile - Name of file to decode
*                outFile - Name of file to write decoded output to
*   Effects    : Encoded file is decoded
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int RleDecodeFile(char *inFile, char *outFile)
{
    FILE *fpIn;                         /* encoded input */
    FILE *fpOut;                        /* uncoded output */
    int currChar, prevChar;             /* current and previous characters */
    unsigned char count;                /* number of characters in a run */

    /* open input and output files */
    if ((fpIn = fopen(inFile, "rb")) == NULL)
    {
        perror(inFile);
        return FALSE;
    }

    if (outFile == NULL)
    {
        fpOut = stdout;     /* default to stdout */
    }
    else
    {
        if ((fpOut = fopen(outFile, "wb")) == NULL)
        {
            fclose(fpIn);
            perror(outFile);
            return FALSE;
        }
    }

    /* decode inFile */
    prevChar = EOF;     /* force next char to be different */

    /* read input until there's nothing left */
    while ((currChar = fgetc(fpIn)) != EOF)
    {
        fputc(currChar, fpOut);

        /* check for run */
        if (currChar == prevChar)
        {
            /* we have a run.  write it out. */
            count = fgetc(fpIn);
            while (count > 0)
            {
                fputc(currChar, fpOut);
                count--;
            }

            prevChar = EOF;     /* force next char to be different */
        }
        else
        {
            /* no run */
            prevChar = currChar;
        }
    }

    /* close all open files */
    fclose(fpOut);
    fclose(fpIn);

    return TRUE;
}
