/***************************************************************************
*                  Canonical Huffman Encoding and Decoding
*
*   File    : canonical.c
*   Purpose : Use canonical huffman coding to compress/decompress files
*   Author  : Michael Dipperstein
*   Date    : November 20, 2002
*
****************************************************************************
*
* Huffman: An ANSI C Canonical Huffman Encoding/Decoding Routine
* Copyright (C) 2002-2005, 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Huffman library.
*
* The Huffman library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The Huffman library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include <errno.h>
#include "huflocal.h"
#include "huffman.h"
#include "bitarray.h"
#include "bitfile.h"

#include "../../extra/mem2mem.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct canonical_list_t
{
    short value;        /* characacter represented */
    byte_t codeLen;     /* number of bits used in code (1 - 255) */
    bit_array_t *code;  /* code used for symbol (left justified) */
} canonical_list_t;

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                                 MACROS
***************************************************************************/

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* creating canonical codes */
static int BuildCanonicalCode(huffman_node_t *ht, canonical_list_t *cl);
static int AssignCanonicalCodes(canonical_list_t *cl);
static int CompareByCodeLen(const void *item1, const void *item2);

/* reading/writing code to file */
static void WriteHeader(canonical_list_t *cl, bit_file_t *bfp);
static int ReadHeader(canonical_list_t *cl,  bit_file_t *bfp);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : CanonicalEncodeFile
*   Description: This routine genrates a huffman tree optimized for a file
*                and writes out an encoded version of that file.
*   Parameters : inFile - Open file pointer for file to encode (must be
*                         rewindable).
*                outFile - Open file pointer for file receiving encoded data
*   Effects    : File is Huffman encoded
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.  Either way, inFile and outFile will
*                be left open.
****************************************************************************/
int CanonicalEncodeFile(FILE *inFile, FILE *outFile)
{
    bit_file_t *bOutFile;
    huffman_node_t *huffmanTree;        /* root of huffman tree */
    int c;
    canonical_list_t canonicalList[NUM_CHARS];  /* list of canonical codes */


    /* validate input and output files */
    if ((NULL == inFile) || (NULL == outFile))
    {
        errno = ENOENT;
        return -1;
    }

    bOutFile = MakeBitFile(outFile, BF_WRITE);

    if (NULL == bOutFile)
    {
        perror("Making Output File a BitFile");
        return -1;
    }

    /* build tree */
    if ((huffmanTree = GenerateTreeFromFile(inFile)) == NULL)
    {
        outFile = BitFileToFILE(bOutFile);
        return -1;
    }

    /* use tree to generate a canonical code */
    if (-1 == BuildCanonicalCode(huffmanTree, canonicalList))
    {
        outFile = BitFileToFILE(bOutFile);
        FreeHuffmanTree(huffmanTree);     /* free allocated memory */
        return -1;
    }

    /* write out encoded file */

    /* write header for rebuilding of code */
    WriteHeader(canonicalList, bOutFile);

    /* read characters from file and write them to encoded file */
    rewind(inFile);               /* start another pass on the input file */

    while((c = fgetc(inFile)) != EOF)
    {
        /* write encoded symbols */
        BitFilePutBits(bOutFile,
            BitArrayGetBits(canonicalList[c].code),
            canonicalList[c].codeLen);
    }

    /* now write EOF */
    BitFilePutBits(bOutFile,
        BitArrayGetBits(canonicalList[EOF_CHAR].code),
        canonicalList[EOF_CHAR].codeLen);

    /* clean up */
    outFile = BitFileToFILE(bOutFile);          /* make file normal again */

    return 0;
}

/****************************************************************************
*   Function   : CanonicalDecodeFile
*   Description: This routine reads a Huffman coded file and writes out a
*                decoded version of that file.
*   Parameters : inFile - Open file pointer for file to decode
*                outFile - Open file pointer for file receiving decoded data
*   Effects    : Huffman encoded file is decoded
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.  Either way, inFile and outFile will
*                be left open.
****************************************************************************/
int CanonicalDecodeFile(FILE *inFile, FILE *outFile)
{
    bit_file_t *bInFile;
    bit_array_t *code;
    byte_t length;
    char decodedEOF;
    int i, newBit;
    int lenIndex[NUM_CHARS];
    canonical_list_t canonicalList[NUM_CHARS];  /* list of canonical codes */

    /* validate input and output files */
    if ((NULL == inFile) || (NULL == outFile))
    {
        errno = ENOENT;
        return -1;
    }

    bInFile = MakeBitFile(inFile, BF_READ);

    if (NULL == bInFile)
    {
        perror("Making Input File a BitFile");
        return -1;
    }

    /* allocate canonical code list */
    code = BitArrayCreate(EOF_CHAR);
    if (code == NULL)
    {
        perror("Bit array allocation");
        inFile = BitFileToFILE(bInFile);
        return -1;
    }

    /* initialize canonical list */
    for (i = 0; i < NUM_CHARS; i++)
    {
        canonicalList[i].codeLen = 0;
        canonicalList[i].code = NULL;
    }

    /* populate list with code length from file header */
    if (0 != ReadHeader(canonicalList, bInFile))
    {
        BitArrayDestroy(code);
        inFile = BitFileToFILE(bInFile);
        return -1;
    }

    /* sort the header by code length */
    qsort(canonicalList, NUM_CHARS, sizeof(canonical_list_t),
        CompareByCodeLen);

    /* assign the codes using same rule as encode */
    if (AssignCanonicalCodes(canonicalList) != 0)
    {
        /* failed to assign codes */
        inFile = BitFileToFILE(bInFile);

        for (i = 0; i < NUM_CHARS; i++)
        {
            if(canonicalList[i].code != NULL)
            {
                BitArrayDestroy(canonicalList[i].code);
            }
        }

        return -1;
    }

    /* now we have a huffman code that matches the code used on the encode */

    /* create an index of first code at each possible length */
    for (i = 0; i < NUM_CHARS; i++)
    {
        lenIndex[i] = NUM_CHARS;
    }

    for (i = 0; i < NUM_CHARS; i++)
    {
        if (lenIndex[canonicalList[i].codeLen] > i)
        {
            /* first occurance of this code length */
            lenIndex[canonicalList[i].codeLen] = i;
        }
    }

    /* decode input file */
    length = 0;
    BitArrayClearAll(code);
    decodedEOF = 0;

    while(((newBit = BitFileGetBit(bInFile)) != EOF) && (!decodedEOF))
    {
        if (newBit != 0)
        {
            BitArraySetBit(code, length);
        }

        length++;

        if (lenIndex[length] != NUM_CHARS)
        {
            /* there are code of this length */
            for(i = lenIndex[length];
                (i < NUM_CHARS) && (canonicalList[i].codeLen == length);
                i++)
            {
                if (BitArrayCompare(canonicalList[i].code, code) == 0)
                {
                    /* we just read a symbol output decoded value */
                    if (canonicalList[i].value != EOF_CHAR)
                    {
                        fputc(canonicalList[i].value, outFile);
                    }
                    else
                    {
                        decodedEOF = 1;
                    }

                    BitArrayClearAll(code);
                    length = 0;

                    break;
                }
            }
        }
    }

    /* clean up */
    inFile = BitFileToFILE(bInFile);            /* make file normal again */

    return 0;
}

/****************************************************************************
*   Function   : CanonicalShowTree
*   Description: This routine genrates a huffman tree optimized for a file
*                and writes out an ASCII representation of the code
*                represented by the tree.
*   Parameters : inFile - Open file pointer for file to create the tree for
*                outFile - Open file pointer for file to write the tree to
*   Effects    : Huffman tree is written out to a file
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.  Either way, inFile and outFile will
*                be left open.
****************************************************************************/
int CanonicalShowTree(FILE *inFile, FILE *outFile)
{
    huffman_node_t *huffmanTree;                /* root of huffman tree */
    int i, length;
    canonical_list_t canonicalList[NUM_CHARS];  /* list of canonical codes */

    /* validate input and output files */
    if ((NULL == inFile) || (NULL == outFile))
    {
        errno = ENOENT;
        return -1;
    }

    /* build tree */
    if ((huffmanTree = GenerateTreeFromFile(inFile)) == NULL)
    {
        return -1;
    }

    /* use tree to generate a canonical code */
    if (!BuildCanonicalCode(huffmanTree, canonicalList))
    {
        FreeHuffmanTree(huffmanTree);     /* free allocated memory */
        return -1;
    }

    FreeHuffmanTree(huffmanTree);     /* free allocated memory */

    /* write out canonical code */
    /* print heading to make things look pretty (int is 10 char max) */
    fprintf(outFile, "Char  CodeLen  Encoding\n");
    fprintf(outFile, "----- -------- ----------------\n");

    for(i = 0; i < NUM_CHARS; i++)
    {
        if(canonicalList[i].codeLen > 0)
        {
            if (canonicalList[i].value != EOF_CHAR)
            {
                fprintf(outFile,
                        "0x%02X  %02d       ",
                        canonicalList[i].value, canonicalList[i].codeLen);
            }
            else
            {
                fprintf(outFile,
                        "EOF   %02d       ", canonicalList[i].codeLen);
            }

            /* now write out the code bits */
            for(length = 0; length < canonicalList[i].codeLen; length++)
            {
                if (BitArrayTestBit(canonicalList[i].code, length))
                {
                    fputc('1', outFile);
                }
                else
                {
                    fputc('0', outFile);
                }
            }

            fputc('\n', outFile);
        }
    }

    return 0;
}

/****************************************************************************
*   Function   : CompareByCodeLen
*   Description: Compare function to be used by qsort for sorting canonical
*                list items by code length.  In the event of equal lengths,
*                the symbol value will be used.
*   Parameters : item1 - pointer canonical list item
*                item2 - pointer canonical list item
*   Effects    : None
*   Returned   : 1 if item1 > item2
*                -1 if item1 < item 2
*                0 if something went wrong (means item1 == item2)
****************************************************************************/
static int CompareByCodeLen(const void *item1, const void *item2)
{
    if (((canonical_list_t *)item1)->codeLen >
        ((canonical_list_t *)item2)->codeLen)
    {
        /* item1 > item2 */
        return 1;
    }
    else if (((canonical_list_t *)item1)->codeLen <
        ((canonical_list_t *)item2)->codeLen)
    {
        /* item1 < item2 */
        return -1;
    }
    else
    {
        /* both have equal code lengths break the tie using value */
        if (((canonical_list_t *)item1)->value >
            ((canonical_list_t *)item2)->value)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    return 0;   /* we should never get here */
}

/****************************************************************************
*   Function   : CompareBySymbolValue
*   Description: Compare function to be used by qsort for sorting canonical
*                list items by symbol value.
*   Parameters : item1 - pointer canonical list item
*                item2 - pointer canonical list item
*   Effects    : None
*   Returned   : 1 if item1 > item2
*                -1 if item1 < item 2
****************************************************************************/
static int CompareBySymbolValue(const void *item1, const void *item2)
{
    if (((canonical_list_t *)item1)->value >
        ((canonical_list_t *)item2)->value)
    {
        /* item1 > item2 */
        return 1;
    }

    /* it must be the case that item1 < item2 */
    return -1;
}

/****************************************************************************
*   Function   : BuildCanonicalCode
*   Description: This function builds a canonical Huffman code from a
*                Huffman tree.
*   Parameters : ht - pointer to root of tree
*                cl - pointer to canonical list
*   Effects    : cl is filled with the canonical codes sorted by the value
*                of the charcter to be encode.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
static int BuildCanonicalCode(huffman_node_t *ht, canonical_list_t *cl)
{
    int i;
    byte_t depth = 0;

    /* initialize list */
    for(i = 0; i < NUM_CHARS; i++)
    {
        cl[i].value = i;
        cl[i].codeLen = 0;
        cl[i].code = NULL;
    }

    /* fill list with code lengths (depth) from tree */
    for(;;)
    {
        /* follow this branch all the way left */
        while (ht->left != NULL)
        {
            ht = ht->left;
            depth++;
        }

        if (ht->value != COMPOSITE_NODE)
        {
            /* handle one symbol trees */
            if (depth == 0)
            {
                depth++;
            }

            /* enter results in list */
            cl[ht->value].codeLen = depth;
        }

        while (ht->parent != NULL)
        {
            if (ht != ht->parent->right)
            {
                /* try the parent's right */
                ht = ht->parent->right;
                break;
            }
            else
            {
                /* parent's right tried, go up one level yet */
                depth--;
                ht = ht->parent;
            }
        }

        if (ht->parent == NULL)
        {
            /* we're at the top with nowhere to go */
            break;
        }
    }

    /* sort by code length */
    qsort(cl, NUM_CHARS, sizeof(canonical_list_t), CompareByCodeLen);

    if (0 == AssignCanonicalCodes(cl))
    {
        /* re-sort list in lexical order for use by encode algorithm */
        qsort(cl, NUM_CHARS, sizeof(canonical_list_t), CompareBySymbolValue);
        return 0;       /* success */
    }

    perror("Code assignment failed");
    return -1;          /* assignment failed */
}

/****************************************************************************
*   Function   : AssignCanonicalCode
*   Description: This function accepts a list of symbols sorted by their
*                code lengths, and assigns a canonical Huffman code to each
*                symbol.
*   Parameters : cl - sorted list of symbols to have code values assigned
*   Effects    : cl stores a list of canonical codes sorted by the length
*                of the code used to encode the symbol.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
static int AssignCanonicalCodes(canonical_list_t *cl)
{
    int i;
    byte_t length;
    bit_array_t *code;

    /* assign the new codes */
    code = BitArrayCreate(EOF_CHAR);
    BitArrayClearAll(code);

    length = cl[(NUM_CHARS - 1)].codeLen;

    for(i = (NUM_CHARS - 1); i >= 0; i--)
    {
        /* bail if we hit a zero len code */
        if (cl[i].codeLen == 0)
        {
            break;
        }

        /* adjust code if this length is shorter than the previous */
        if (cl[i].codeLen < length)
        {
            BitArrayShiftRight(code, (length - cl[i].codeLen));
            length = cl[i].codeLen;
        }

        /* assign left justified code */
        if ((cl[i].code = BitArrayDuplicate(code)) == NULL)
        {
            perror("Duplicating code");
            BitArrayDestroy(code);
            return -1;
        }

        BitArrayShiftLeft(cl[i].code, EOF_CHAR - length);

        BitArrayIncrement(code);
    }

    BitArrayDestroy(code);
    return 0;
}

/****************************************************************************
*   Function   : WriteHeader
*   Description: This function writes the code size for each symbol and the
*                total number of characters in the original file to the
*                specified output file.  If the same algorithm that produced
*                produced the original canonical code is used with these code
*                lengths, an exact copy of the code will be produced.
*   Parameters : cl - pointer to list of canonical Huffman codes
*                bfp - pointer to open binary file to write to.
*   Effects    : Symbol code lengths and symbol count are written to the
*                output file.
*   Returned   : None
****************************************************************************/
static void WriteHeader(canonical_list_t *cl, bit_file_t *bfp)
{
    int i;

    /* write out code size for each symbol */
    for (i = 0; i < NUM_CHARS; i++)
    {
        BitFilePutChar(cl[i].codeLen, bfp);
    }
}

/****************************************************************************
*   Function   : ReadHeader
*   Description: This function reads the header information stored by
*                WriteHeader.  If the same algorithm that produced the
*                original tree is used with these counts, an exact copy of
*                the tree will be produced.
*   Parameters : cl - pointer to list of canonical Huffman codes
*                bfp - file to read from
*   Effects    : Code lengths and symbols are read into the canonical list.
*                Total number of symbols encoded is store in totalCount
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
static int ReadHeader(canonical_list_t *cl, bit_file_t *bfp)
{
    int c;
    int i;

    /* read the code length */
    for (i = 0; i < NUM_CHARS; i++)
    {
        c = BitFileGetChar(bfp);

        if (c != EOF)
        {
            cl[i].value = i;
            cl[i].codeLen = (byte_t)c;
        }
        else
        {
            fprintf(stderr, "error: malformed file header.\n");
            errno = EILSEQ;     /* Illegal byte sequence seems reasonable */
            return -1;
        }
    }

    return 0;
}
