/***************************************************************************
*          Lempel, Ziv, Storer, and Szymanski Encoding and Decoding
*
*   File    : kmp.c
*   Purpose : Implements the Knuth-Morris-Pratt string matching algorithm
*             to match uncoded strings to the sliding window buffer for
*             the LZSS algorithm.  A description of the KMP algorithm may
*             be found here:
*             <http://en.wikipedia.org/wiki/Knuth-Morris-Pratt_algorithm>
*   Author  : Michael Dipperstein
*   Date    : February 10, 2010
*
****************************************************************************
*
* KMP: Knuth–Morris–Prat matching routines used by LZSS Encoding/Decoding
*        Routine
* Copyright (C) 2010, 2014 by
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

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "lzlocal.h"

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
/* cyclic buffer sliding window of already read characters */
extern unsigned char slidingWindow[];
extern unsigned char uncodedLookahead[];

/****************************************************************************
*   Function   : InitializeSearchStructures
*   Description: This function initializes structures used to speed up the
*                process of matching uncoded strings to strings in the
*                sliding window.  The KMP search doesn't use any special
*                structures that remain between searches, so this function
*                doesn't do anything.
*   Parameters : None
*   Effects    : None
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
int InitializeSearchStructures(void)
{
    return 0;
}

/****************************************************************************
*   Function   : FillTable
*   Description: This function populates an array with the fallback positions
*                used by the KMP algorithm to determine where to continue
*                searching from in the event of a mismatch.
*   Parameters : uncoded - a pointer to an unrolled copy of the uncoded
*                          lookahead buffer
*                kmpTable - a pointer to an array to be used as the KMP
*                           partial match table.
*   Effects    : kmpTable will be populated with the fallback values such
*                that a mismatch at uncoded[i] requires a fallback of
*                kmpTable[i].
*   Returned   : None
****************************************************************************/
static void FillTable(unsigned char *uncoded, int* kmpTable)
{
    int i;  /* current position in the kmpTable */
    int j;  /* next position for the current candidate substring in uncoded */

    /* initialize the start of the kmpTable with */
    kmpTable[0] = -1;
    kmpTable[1] = 0;
    i = 2;
    j = 0;

    while (i < MAX_CODED)
    {
        if (uncoded[i - 1] == uncoded[j])
        {
            /* the candidate substring continues to match */
            j++;
            kmpTable[i] = j;
            i++;
        }
        else if (j > 0)
        {
            /* no more matches, but we had at least one for fallback */
            j = kmpTable[j];
        }
        else
        {
            /* we never did find a candidate substring (j == 0) */
            kmpTable[i] = 0;
            i++;
        }
    }
}

/****************************************************************************
*   Function   : FindMatch
*   Description: This function will search through the slidingWindow
*                dictionary for the longest sequence matching the MAX_CODED
*                long string stored in uncodedLookahed.
*   Parameters : windowHead - head of sliding window
*                uncodedHead - head of uncoded lookahead buffer
*   Effects    : None
*   Returned   : The sliding window index where the match starts and the
*                length of the match.  If there is no match a length of
*                zero will be returned.
****************************************************************************/
encoded_string_t FindMatch(const unsigned int windowHead,
    const unsigned int uncodedHead)
{
    encoded_string_t matchData;
    unsigned int m;             /* starting position in string being searched */
    unsigned int i;             /* offset from m and uncoded data */
    int kmpTable[MAX_CODED];    /* kmp partial match table */
    unsigned char localUncoded[MAX_CODED];  /* non-circular copy of uncoded */

    /* build non-circular copy of the uncoded lookahead buffer */
    for (i = 0; i < MAX_CODED; i++)
    {
        localUncoded[i] =
            uncodedLookahead[Wrap((uncodedHead + i), MAX_CODED)];
    }

    FillTable(localUncoded, kmpTable);      /* build kmp partial match table */

    matchData.length = 0;
    matchData.offset = 0;
    m = 0;
    i = 0;

    while (m < WINDOW_SIZE)
    {
        if (localUncoded[i] ==
            slidingWindow[Wrap((m + i + windowHead), WINDOW_SIZE)])
        {
            /* one more character matches */
            i++;

            if (MAX_CODED == i)
            {
                /* entire string is matched */
                matchData.length = MAX_CODED;
                matchData.offset = Wrap((m + windowHead), WINDOW_SIZE);
                break;
            }
        }
        else
        {
            if (i > matchData.length)
            {
                /* partial match is longest yet */
                matchData.length = i;
                matchData.offset = Wrap((m + windowHead), WINDOW_SIZE);
            }

            /* compute next position to search from */
            m = m + i - kmpTable[i];

            if (kmpTable[i] > 0)
            {
                i = kmpTable[i];
            }
            else
            {
                i = 0;
            }
        }
    }

    /* we either got a complete match or searched the whole dictionary */
    return matchData;
}

/****************************************************************************
*   Function   : ReplaceChar
*   Description: This function replaces the character stored in
*                slidingWindow[charIndex] with the one specified by
*                replacement.
*   Parameters : charIndex - sliding window index of the character to be
*                            removed from the linked list.
*   Effects    : slidingWindow[charIndex] is replaced by replacement.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
int ReplaceChar(const unsigned int charIndex, const unsigned char replacement)
{
    slidingWindow[charIndex] = replacement;
    return 0;
}
