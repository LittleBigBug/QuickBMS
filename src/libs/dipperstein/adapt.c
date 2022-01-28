/***************************************************************************
*                  Adaptive Delta Code Size Computations
*
*   File    : adapt.c
*   Purpose : Module containing calculations used to adapt the code word
*             size used in adaptive delta encoding/decoding.  This is where
*             tweaks may be made to add intelligence to the adjustment of
*             the code word size.
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

#define _ADAPT_C_

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdlib.h>
#include "adapt.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
/* maximum overflows and underflows before code size change */
#define MAX_OVF 3
#define MAX_UNF 3

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

typedef struct adaptive_data_t
{
    unsigned char codeSize;
    unsigned char overflowCount;
    unsigned char underflowCount;
} adaptive_data_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : CreateAdaptiveData
*   Description: This function creates the data structure used to track
*                encoding/decoding statistics and determine how the code
*                word size should be adapted.
*   Parameters : codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : The data structure used to track encoding/decoding
*                statistics is created on the heap.
*   Returned   : A pointer to created data structure on success, otherwise
*                NULL.
***************************************************************************/
adaptive_data_t* CreateAdaptiveData(const unsigned char codeSize)
{
    adaptive_data_t *data;

    data = malloc(sizeof(adaptive_data_t));

    if (NULL != data)
    {
        data->codeSize = codeSize;
        data->overflowCount = 0;
        data->underflowCount = 0;
    }

    return data;
}

/***************************************************************************
*   Function   : FreeAdaptiveData
*   Description: This function frees the data structure used to track
*                encoding/decoding statistics and determine how the code
*                word size should be adapted.
*   Parameters : data - a pointer to the data structure to be freed.
*   Effects    : The data structure used to track encoding/decoding
*                statistics is created on the heap.
*   Returned   : None
***************************************************************************/
void FreeAdaptiveData(adaptive_data_t *data)
{
    if (NULL != data)
    {
        free(data);
    }
}

/***************************************************************************
*   Function   : UpdateAdaptiveStatistics
*   Description: This function updates the data structure used to track
*                encoding/decoding statistics and determines the code
*                word size to be used.
*   Parameters : data - pointer to the data structure that is used to
*                       determine the code word size.
*                stat - an indication of overflow, underflow, or neither
*                       used to determine the size of the next code word.
*   Effects    : Statistical counters are updated and a new code word
*                length may be determined.
*   Returned   : The number of bits to be used for the next code word.
***************************************************************************/
unsigned char UpdateAdaptiveStatistics(adaptive_data_t *data,
    const code_word_stat_t stat)
{
    switch(stat)
    {
        case CS_OKAY:
            if (data->overflowCount > 0)
            {
                data->overflowCount--;
            }
            
            if (data->underflowCount > 0)
            {
                data->underflowCount--;
            }
            break;

        case CS_OVERFLOW:
            if (data->underflowCount > 0)
            {
                data->underflowCount--;
            }

            data->overflowCount++;

            if (MAX_OVF < data->overflowCount)
            {
                if (data->codeSize < 8)
                {
                    data->codeSize++;
                }

                data->underflowCount = 0;
                data->overflowCount = 0;
            }
            break;

        case CS_UNDERFLOW:
            if (data->overflowCount > 0)
            {
                data->overflowCount--;
            }

            data->underflowCount++;

            if (MAX_UNF < data->underflowCount)
            {
                if (data->codeSize > 2)
                {
                    data->codeSize--;
                }

                data->underflowCount = 0;
                data->overflowCount = 0;
            }
            break;
            
        default:
            break;
    }

    return data->codeSize;
}
