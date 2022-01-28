/***************************************************************************
*               Variant Packbits Encoding and Decoding Library
*
*   File    : vpackbits.c
*   Purpose : Use a variant of packbits run length coding to compress and
*             decompress files.  This packbits variant begins each block of
*             data with the a byte header that is decoded as follows.
*
*             Byte (n)   | Meaning
*             -----------+-------------------------------------
*             0 - 127    | Copy the next n + 1 bytes
*             -128 - -1  | Make -n + 2 copies of the next byte
*
*   Author  : Michael Dipperstein
*   Date    : September 7, 2006
*
****************************************************************************
*   UPDATES
*
*   $Id: vpackbits.c,v 1.4 2007/09/08 17:12:27 michael Exp $
*   $Log: vpackbits.c,v $
*   Revision 1.4  2007/09/08 17:12:27  michael
*   Corrected error decoding maximum length runs.
*   Replace getopt with optlist.
*   Changes required for LGPL v3.
*
*   Revision 1.3  2007/02/13 05:29:43  michael
*   trimmed spaces.
*
*   Revision 1.2  2006/09/20 13:13:30  michael
*   Minor modifications to look more like description on my web page.
*
*   Revision 1.1  2006/09/10 05:12:57  michael
*   Initial release
*
****************************************************************************
*
* VPackBits: ANSI C PackBits Style Run Length Encoding/Decoding Routines
* Copyright (C) 2006-2007 by
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

#define MIN_RUN     3                   /* minimum run length to encode */
#define MAX_RUN     (128 + MIN_RUN - 1) /* maximum run length to encode */
#define MAX_COPY    128                 /* maximum characters to copy */

/* maximum that can be read before copy block is written */
#define MAX_READ    (MAX_COPY + MIN_RUN - 1)

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : VPackBitsEncodeFile
*   Description: This routine reads an input file and writes out a run
*                length encoded version of that file.  The technique used
*                is a variation of the packbits technique.
*   Parameters : inFile - Name of file to encode
*                outFile - Name of file to write encoded output to
*   Effects    : File is encoded using RLE
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int VPackBitsEncodeFile(char *inFile, char *outFile)
{
    FILE *fpIn;                         /* uncoded input */
    FILE *fpOut;                        /* encoded output */
    int currChar;                       /* current character */
    unsigned char charBuf[MAX_READ];    /* buffer of already read characters */
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

    /* prime the read loop */
    currChar = fgetc(fpIn);
    count = 0;

    /* read input until there's nothing left */
    while (currChar != EOF)
    {
        charBuf[count] = (unsigned char)currChar;

        count++;

        if (count >= MIN_RUN)
        {
            int i;

            /* check for run  charBuf[count - 1] .. charBuf[count - MIN_RUN]*/
            for (i = 2; i <= MIN_RUN; i++)
            {
                if (currChar != charBuf[count - i])
                {
                    /* no run */
                    i = 0;
                    break;
                }
            }

            if (i != 0)
            {
                /* we have a run write out buffer before run*/
                int nextChar;

                if (count > MIN_RUN)
                {
                    /* block size - 1 followed by contents */
                    fputc(count - MIN_RUN - 1, fpOut);
                    fwrite(charBuf, sizeof(unsigned char), count - MIN_RUN,
                        fpOut);
                }


                /* determine run length (MIN_RUN so far) */
                count = MIN_RUN;

                while ((nextChar = fgetc(fpIn)) == currChar)
                {
                    count++;
                    if (MAX_RUN == count)
                    {
                        /* run is at max length */
                        break;
                    }
                }

                /* write out encoded run length and run symbol */
                fputc((char)((int)(MIN_RUN - 1) - (int)(count)), fpOut);
                fputc(currChar, fpOut);

                if ((nextChar != EOF) && (count != MAX_RUN))
                {
                    /* make run breaker start of next buffer */
                    charBuf[0] = nextChar;
                    count = 1;
                }
                else
                {
                    /* file or max run ends in a run */
                    count = 0;
                }
            }
        }

        if (MAX_READ == count)
        {
            int i;

            /* write out buffer */
            fputc(MAX_COPY - 1, fpOut);
            fwrite(charBuf, sizeof(unsigned char), MAX_COPY, fpOut);

            /* start a new buffer */
            count = MAX_READ - MAX_COPY;

            /* copy excess to front of buffer */
            for (i = 0; i < count; i++)
            {
                charBuf[i] = charBuf[MAX_COPY + i];
            }
        }

        currChar = fgetc(fpIn);
    }

    /* write out last buffer */
    if (0 != count)
    {
        if (count <= MAX_COPY)
        {
            /* write out entire copy buffer */
            fputc(count - 1, fpOut);
            fwrite(charBuf, sizeof(unsigned char), count, fpOut);
        }
        else
        {
            /* we read more than the maximum for a single copy buffer */
            fputc(MAX_COPY - 1, fpOut);
            fwrite(charBuf, sizeof(unsigned char), MAX_COPY, fpOut);

            /* write out remainder */
            count -= MAX_COPY;
            fputc(count - 1, fpOut);
            fwrite(&charBuf[MAX_COPY], sizeof(unsigned char), count, fpOut);
        }
    }

    /* close all open files */
    fclose(fpOut);
    fclose(fpIn);

    return TRUE;
}

/***************************************************************************
*   Function   : VPackBitsDecodeFile
*   Description: This routine opens a file encoded by a variant of the
*                packbits run length encoding, and decodes it to an output
*                file.
*   Parameters : inFile - Name of file to decode
*                outFile - Name of file to write decoded output to
*   Effects    : Encoded file is decoded
*   Returned   : TRUE for success, otherwise FALSE.
***************************************************************************/
int VPackBitsDecodeFile(char *inFile, char *outFile)
{
    FILE *fpIn;                         /* encoded input */
    FILE *fpOut;                        /* uncoded output */
    int countChar;                      /* run/copy count */
    int currChar;                       /* current character */

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

    /* read input until there's nothing left */
    while ((countChar = fgetc(fpIn)) != EOF)
    {
        countChar = (char)countChar;    /* force sign extension */

        if (countChar < 0)
        {
            /* we have a run write out  2 - countChar copies */
            countChar = (MIN_RUN - 1) - countChar;

            if (EOF == (currChar = fgetc(fpIn)))
            {
                fprintf(stderr, "Run block is too short!\n");
                countChar = 0;
            }

            while (countChar > 0)
            {
                fputc(currChar, fpOut);
                countChar--;
            }
        }
        else
        {
            /* we have a block of countChar + 1 symbols to copy */
            for (countChar++; countChar > 0; countChar--)
            {
                if ((currChar = fgetc(fpIn)) != EOF)
                {
                    fputc(currChar, fpOut);
                }
                else
                {
                    fprintf(stderr, "Copy block is too short!\n");
                    break;
                }
            }
        }
    }

    /* close all open files */
    fclose(fpOut);
    fclose(fpIn);

    return TRUE;
}
