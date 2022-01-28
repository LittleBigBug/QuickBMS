/***************************************************************************
*                Common Huffman Encoding and Decoding Header
*
*   File    : huflocal.c
*   Purpose : Common functions used in Huffman library, but not used by
*             caller of public Huffman library functions.
*   Author  : Michael Dipperstein
*   Date    : May 21, 2005
*
****************************************************************************
*
* Huffman: An ANSI C Huffman Encoding/Decoding Routine
* Copyright (C) 2005, 2007, 2014 by
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
#include "huflocal.h"
#include "huffman.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

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

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : GenerateTreeFromFile
*   Description: This routine creates a huffman tree optimized for encoding
*                the file passed as a parameter.
*   Parameters : inFile - Name of file to create tree for
*   Effects    : Huffman tree is built for file.
*   Returned   : Pointer to resulting tree.  NULL on failure.
****************************************************************************/
huffman_node_t *GenerateTreeFromFile(FILE *inFile)
{
    huffman_node_t *huffmanArray[NUM_CHARS];    /* array of all leaves */
    huffman_node_t *huffmanTree;                /* root of huffman tree */
    int c;

    /* allocate array of leaves for all possible characters */
    for (c = 0; c < NUM_CHARS; c++)
    {
        if ((huffmanArray[c] = AllocHuffmanNode(c)) == NULL)
        {
            /* allocation failed clear existing allocations */
            for (c--; c >= 0; c--)
            {
                free(huffmanArray[c]);
            }
            return NULL;
        }
    }

    /* assume that there will be exactly 1 EOF */
    huffmanArray[EOF_CHAR]->count = 1;
    huffmanArray[EOF_CHAR]->ignore = 0;

    /* count occurrence of each character */
    while ((c = fgetc(inFile)) != EOF)
    {
        if (huffmanArray[c]->count < COUNT_T_MAX)
        {
            /* increment count for character and include in tree */
            huffmanArray[c]->count++;
            huffmanArray[c]->ignore = 0;
        }
        else
        {
            fprintf(stderr,
                "Input file contains too many 0x%02X to count.\n", c);
            return NULL;
        }
    }

    /* put array of leaves into a huffman tree */
    huffmanTree = BuildHuffmanTree(huffmanArray, NUM_CHARS);

    return huffmanTree;
}

/****************************************************************************
*   Function   : AllocHuffmanNode
*   Description: This routine allocates and initializes memory for a node
*                (tree entry for a single character) in a Huffman tree.
*   Parameters : value - character value represented by this node
*   Effects    : Memory for a huffman_node_t is allocated from the heap
*   Returned   : Pointer to allocated node.  NULL on failure to allocate.
****************************************************************************/
huffman_node_t *AllocHuffmanNode(int value)
{
    huffman_node_t *ht;

    ht = (huffman_node_t *)(malloc(sizeof(huffman_node_t)));

    if (ht != NULL)
    {
        ht->value = value;
        ht->ignore = 1;         /* will be 0 if one is found */

        /* at this point, the node is not part of a tree */
        ht->count = 0;
        ht->level = 0;
        ht->left = NULL;
        ht->right = NULL;
        ht->parent = NULL;
    }
    else
    {
        perror("Allocate Node");
    }

    return ht;
}

/****************************************************************************
*   Function   : AllocHuffmanCompositeNode
*   Description: This routine allocates and initializes memory for a
*                composite node (tree entry for multiple characters) in a
*                Huffman tree.  The number of occurrences for a composite is
*                the sum of occurrences of its children.
*   Parameters : left - left child in tree
*                right - right child in tree
*   Effects    : Memory for a huffman_node_t is allocated from the heap
*   Returned   : Pointer to allocated node
****************************************************************************/
static huffman_node_t *AllocHuffmanCompositeNode(huffman_node_t *left,
    huffman_node_t *right)
{
    huffman_node_t *ht;

    ht = (huffman_node_t *)(malloc(sizeof(huffman_node_t)));

    if (ht != NULL)
    {
        ht->value = COMPOSITE_NODE;     /* represents multiple chars */
        ht->ignore = 0;
        ht->count = left->count + right->count;     /* sum of children */
        ht->level = max(left->level, right->level) + 1;

        /* attach children */
        ht->left = left;
        ht->left->parent = ht;
        ht->right = right;
        ht->right->parent = ht;
        ht->parent = NULL;
    }
    else
    {
        perror("Allocate Composite");
        return NULL;
    }

    return ht;
}

/****************************************************************************
*   Function   : FreeHuffmanTree
*   Description: This is a recursive routine for freeing the memory
*                allocated for a node and all of its descendants.
*   Parameters : ht - structure to delete along with its children.
*   Effects    : Memory for a huffman_node_t and its children is returned to
*                the heap.
*   Returned   : None
****************************************************************************/
void FreeHuffmanTree(huffman_node_t *ht)
{
    if (ht->left != NULL)
    {
        FreeHuffmanTree(ht->left);
    }

    if (ht->right != NULL)
    {
        FreeHuffmanTree(ht->right);
    }

    free(ht);
}

/****************************************************************************
*   Function   : FindMinimumCount
*   Description: This function searches an array of HUFFMAN_STRCUT to find
*                the active (ignore == 0) element with the smallest
*                frequency count.  In order to keep the tree shallow, if two
*                nodes have the same count, the node with the lower level
*                selected.
*   Parameters : ht - pointer to array of structures to be searched
*                elements - number of elements in the array
*   Effects    : None
*   Returned   : Index of the active element with the smallest count.
*                NONE is returned if no minimum is found.
****************************************************************************/
static int FindMinimumCount(huffman_node_t **ht, int elements)
{
    int i;                      /* array index */
    int currentIndex;           /* index with lowest count seen so far */
    unsigned int currentCount;  /* lowest count seen so far */
    int currentLevel;           /* level of lowest count seen so far */

    currentIndex = NONE;
    currentCount = UINT_MAX;
    currentLevel = INT_MAX;

    /* sequentially search array */
    for (i = 0; i < elements; i++)
    {
        /* check for lowest count (or equally as low, but not as deep) */
        if ((ht[i] != NULL) && (!ht[i]->ignore) &&
            (ht[i]->count < currentCount ||
                (ht[i]->count == currentCount && ht[i]->level < currentLevel)))
        {
            currentIndex = i;
            currentCount = ht[i]->count;
            currentLevel = ht[i]->level;
        }
    }

    return currentIndex;
}

/****************************************************************************
*   Function   : BuildHuffmanTree
*   Description: This function builds a huffman tree from an array of
*                HUFFMAN_STRCUT.
*   Parameters : ht - pointer to array of structures to be searched
*                elements - number of elements in the array
*   Effects    : Array of huffman_node_t is built into a huffman tree.
*   Returned   : Pointer to the root of a Huffman Tree
****************************************************************************/
huffman_node_t *BuildHuffmanTree(huffman_node_t **ht, int elements)
{
    int min1, min2;     /* two nodes with the lowest count */

    /* keep looking until no more nodes can be found */
    for (;;)
    {
        /* find node with lowest count */
        min1 = FindMinimumCount(ht, elements);

        if (min1 == NONE)
        {
            /* no more nodes to combine */
            break;
        }

        ht[min1]->ignore = 1;       /* remove from consideration */

        /* find node with second lowest count */
        min2 = FindMinimumCount(ht, elements);

        if (min2 == NONE)
        {
            /* no more nodes to combine */
            break;
        }

        ht[min2]->ignore = 1;       /* remove from consideration */

        /* combine nodes into a tree */
        if ((ht[min1] = AllocHuffmanCompositeNode(ht[min1], ht[min2])) == NULL)
        {
            return NULL;
        }

        ht[min2] = NULL;
    }

    return ht[min1];
}
