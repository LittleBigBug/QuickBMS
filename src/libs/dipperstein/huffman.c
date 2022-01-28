/***************************************************************************
*                       Huffman Encoding and Decoding
*
*   File    : huffman.c
*   Purpose : Use huffman coding to compress/decompress files
*   Author  : Michael Dipperstein
*   Date    : November 20, 2002
*
****************************************************************************
*
* Huffman: An ANSI C Huffman Encoding/Decoding Routine
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

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct code_list_t
{
    byte_t codeLen;     /* number of bits used in code (1 - 255) */
    bit_array_t *code;  /* code used for symbol (left justified) */
} code_list_t;

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
/* creates look up table from tree */
static int MakeCodeList(huffman_node_t *ht, code_list_t *codeList);

/* reading/writing tree to file */
static void WriteHeader(huffman_node_t *ht, bit_file_t *bfp);
static int ReadHeader(huffman_node_t **ht, bit_file_t *bfp);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : HuffmanEncodeFile
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
int HuffmanEncodeFile(FILE *inFile, FILE *outFile)
{
    huffman_node_t *huffmanTree;        /* root of huffman tree */
    code_list_t codeList[NUM_CHARS];    /* table for quick encode */
    bit_file_t *bOutFile;
    int c;

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

    /* build a list of codes for each symbol */

    /* initialize code list */
    for (c = 0; c < NUM_CHARS; c++)
    {
        codeList[c].code = NULL;
        codeList[c].codeLen = 0;
    }

    if (0 != MakeCodeList(huffmanTree, codeList))
    {
        outFile = BitFileToFILE(bOutFile);
        return -1;
    }

    /* write out encoded file */

    /* write header for rebuilding of tree */
    WriteHeader(huffmanTree, bOutFile);

    /* read characters from file and write them to encoded file */
    rewind(inFile);         /* start another pass on the input file */

    while((c = fgetc(inFile)) != EOF)
    {
        BitFilePutBits(bOutFile,
            BitArrayGetBits(codeList[c].code),
            codeList[c].codeLen);
    }

    /* now write EOF */
    BitFilePutBits(bOutFile,
        BitArrayGetBits(codeList[EOF_CHAR].code),
        codeList[EOF_CHAR].codeLen);

    /* free the code list */
    for (c = 0; c < NUM_CHARS; c++)
    {
        if (codeList[c].code != NULL)
        {
            BitArrayDestroy(codeList[c].code);
        }
    }

    /* clean up */
    outFile = BitFileToFILE(bOutFile);          /* make file normal again */
    FreeHuffmanTree(huffmanTree);               /* free allocated memory */

    return 0;
}

/****************************************************************************
*   Function   : HuffmanDecodeFile
*   Description: This routine reads a Huffman coded file and writes out a
*                decoded version of that file.
*   Parameters : inFile - Open file pointer for file to decode
*                outFile - Open file pointer for file receiving decoded data
*   Effects    : Huffman encoded file is decoded
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.  Either way, inFile and outFile will
*                be left open.
****************************************************************************/
int HuffmanDecodeFile(FILE *inFile, FILE *outFile)
{
    huffman_node_t *huffmanArray[NUM_CHARS];    /* array of all leaves */
    huffman_node_t *huffmanTree;
    huffman_node_t *currentNode;
    int i, c;
    bit_file_t *bInFile;

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

    /* allocate array of leaves for all possible characters */
    for (i = 0; i < NUM_CHARS; i++)
    {
        if ((huffmanArray[i] = AllocHuffmanNode(i)) == NULL)
        {
            /* allocation failed clear existing allocations */
            for (i--; i >= 0; i--)
            {
                free(huffmanArray[i]);
            }

            inFile = BitFileToFILE(bInFile);
            return -1;
        }
    }

    /* populate leaves with frequency information from file header */
    if (0 != ReadHeader(huffmanArray, bInFile))
    {
        for (i = 0; i < NUM_CHARS; i++)
        {
            free(huffmanArray[i]);
        }

        inFile = BitFileToFILE(bInFile);
        return -1;
    }

    /* put array of leaves into a huffman tree */
    if ((huffmanTree = BuildHuffmanTree(huffmanArray, NUM_CHARS)) == NULL)
    {
        FreeHuffmanTree(huffmanTree);
        inFile = BitFileToFILE(bInFile);
        return -1;
    }

    /* now we should have a tree that matches the tree used on the encode */
    currentNode = huffmanTree;

    while ((c = BitFileGetBit(bInFile)) != EOF)
    {
        /* traverse the tree finding matches for our characters */
        if (c != 0)
        {
            currentNode = currentNode->right;
        }
        else
        {
            currentNode = currentNode->left;
        }

        if (currentNode->value != COMPOSITE_NODE)
        {
            /* we've found a character */
            if (currentNode->value == EOF_CHAR)
            {
                /* we've just read the EOF */
                break;
            }

            fputc(currentNode->value, outFile); /* write out character */
            currentNode = huffmanTree;          /* back to top of tree */
        }
    }

    /* clean up */
    inFile = BitFileToFILE(bInFile);            /* make file normal again */
    FreeHuffmanTree(huffmanTree);     /* free allocated memory */

    return 0;
}

/****************************************************************************
*   Function   : HuffmanShowTree
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
int HuffmanShowTree(FILE *inFile, FILE *outFile)
{
    huffman_node_t *huffmanTree;        /* root of huffman tree */
    huffman_node_t *htp;                /* pointer into tree */
    char code[NUM_CHARS - 1];           /* 1s and 0s in character's code */
    int depth = 0;                      /* depth of tree */

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

    /* write out tree */
    /* print heading to make things look pretty (int is 10 char max) */
    fprintf(outFile, "Char  Count      Encoding\n");
    fprintf(outFile, "----- ---------- ----------------\n");

    htp = huffmanTree;
    for(;;)
    {
        /* follow this branch all the way left */
        while (htp->left != NULL)
        {
            code[depth] = '0';
            htp = htp->left;
            depth++;
        }

        if (htp->value != COMPOSITE_NODE)
        {
            /* handle the case of a single symbol code */
            if (depth == 0)
            {
                code[depth] = '0';
                depth++;
            }

            /* we hit a character node, print its code */
            code[depth] = '\0';

            if (htp->value != EOF_CHAR)
            {
                fprintf(outFile, "0x%02X  %10d %s\n",
                    htp->value, htp->count, code);
            }
            else
            {
                fprintf(outFile, "EOF   %10d %s\n", htp->count, code);
            }
        }

        while (htp->parent != NULL)
        {
            if (htp != htp->parent->right)
            {
                /* try the parent's right */
                code[depth - 1] = '1';
                htp = htp->parent->right;
                break;
            }
            else
            {
                /* parent's right tried, go up one level yet */
                depth--;
                htp = htp->parent;
                code[depth] = '\0';
            }
        }

        if (htp->parent == NULL)
        {
            /* we're at the top with nowhere to go */
            break;
        }
    }

    /* clean up */
    FreeHuffmanTree(huffmanTree);     /* free allocated memory */

    return 0;
}

/****************************************************************************
*   Function   : MakeCodeList
*   Description: This function uses a huffman tree to build a list of codes
*                and their length for each encoded symbol.  This simplifies
*                the encoding process.  Instead of traversing a tree in
*                search of the code for any symbol, the code maybe found
*                by accessing codeList[symbol].code.
*   Parameters : ht - pointer to root of huffman tree
*                codeList - code list to populate.
*   Effects    : Code values are filled in for symbols in a code list.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
static int MakeCodeList(huffman_node_t *ht, code_list_t *codeList)
{
    bit_array_t *code;
    byte_t depth = 0;

    if((code = BitArrayCreate(EOF_CHAR)) == NULL)
    {
        perror("Unable to allocate bit array");
        return -1;
    }

    BitArrayClearAll(code);

    for(;;)
    {
        /* follow this branch all the way left */
        while (ht->left != NULL)
        {
            BitArrayShiftLeft(code, 1);
            ht = ht->left;
            depth++;
        }

        if (ht->value != COMPOSITE_NODE)
        {
            /* enter results in list */
            codeList[ht->value].codeLen = depth;
            codeList[ht->value].code = BitArrayDuplicate(code);
            if (codeList[ht->value].code == NULL)
            {
                perror("Unable to allocate bit array");
                BitArrayDestroy(code);
                return -1;
            }

            /* now left justify code */
            BitArrayShiftLeft(codeList[ht->value].code, EOF_CHAR - depth);
        }

        while (ht->parent != NULL)
        {
            if (ht != ht->parent->right)
            {
                /* try the parent's right */
                BitArraySetBit(code, (EOF_CHAR - 1));
                ht = ht->parent->right;
                break;
            }
            else
            {
                /* parent's right tried, go up one level yet */
                depth--;
                BitArrayShiftRight(code, 1);
                ht = ht->parent;
            }
        }

        if (ht->parent == NULL)
        {
            /* we're at the top with nowhere to go */
            break;
        }
    }

    BitArrayDestroy(code);
    return 0;
}

/****************************************************************************
*   Function   : WriteHeader
*   Description: This function writes the each symbol contained in a tree
*                as well as its number of occurrences in the original file
*                to the specified output file.  If the same algorithm that
*                produced the original tree is used with these counts, an
*                exact copy of the tree will be produced.
*   Parameters : ht - pointer to root of huffman tree
*                bfp - pointer to open binary file to write to.
*   Effects    : Symbol values and symbol counts are written to a file.
*   Returned   : None
****************************************************************************/
static void WriteHeader(huffman_node_t *ht, bit_file_t *bfp)
{
    unsigned int i;

    for(;;)
    {
        /* follow this branch all the way left */
        while (ht->left != NULL)
        {
            ht = ht->left;
        }

        if ((ht->value != COMPOSITE_NODE) &&
            (ht->value != EOF_CHAR))
        {
            /* write symbol and count to header */
            BitFilePutChar(ht->value, bfp);
            BitFilePutBits(bfp, (void *)&(ht->count), 8 * sizeof(count_t));
        }

        while (ht->parent != NULL)
        {
            if (ht != ht->parent->right)
            {
                ht = ht->parent->right;
                break;
            }
            else
            {
                /* parent's right tried, go up one level yet */
                ht = ht->parent;
            }
        }

        if (ht->parent == NULL)
        {
            /* we're at the top with nowhere to go */
            break;
        }
    }

    /* now write end of table char 0 count 0 */
    BitFilePutChar(0, bfp);
    for(i = 0; i < sizeof(count_t); i++)
    {
        BitFilePutChar(0, bfp);
    }
}

/****************************************************************************
*   Function   : ReadHeader
*   Description: This function reads the header information stored by
*                WriteHeader.  If the same algorithm that produced the
*                original tree is used with these counts, an exact copy of
*                the tree will be produced.
*   Parameters : ht - pointer to array of pointers to tree leaves
*                inFile - file to read from
*   Effects    : Frequency information is read into the node of ht
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
static int ReadHeader(huffman_node_t **ht, bit_file_t *bfp)
{
    count_t count;
    int c;
    int status = -1;        /* in case of premature EOF */

    while ((c = BitFileGetChar(bfp)) != EOF)
    {
        BitFileGetBits(bfp, (void *)(&count), 8 * sizeof(count_t));

        if ((count == 0) && (c == 0))
        {
            /* we just read end of table marker */
            status = 0;
            break;
        }

        ht[c]->count = count;
        ht[c]->ignore = 0;
    }

    /* add assumed EOF */
    ht[EOF_CHAR]->count = 1;
    ht[EOF_CHAR]->ignore = 0;

    if (0 != status)
    {
        /* we hit EOF before we read a full header */
        fprintf(stderr, "error: malformed file header.\n");
        errno = EILSEQ;     /* Illegal byte sequence seems reasonable */
    }

    return status;
}
