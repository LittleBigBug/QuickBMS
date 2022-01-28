/*************************************************************************
* Name:        shannonfano.c
* Author:      Marcus Geelnard
* Description: Shannon-Fano coder/decoder implementation.
* Reentrant:   Yes
*
* This is a very straight forward implementation of a Shannon-Fano coder
* and decoder. The Shannon-Fano coding method is the predecessor of the
* Huffman coding method. In practice, the Shannon-Fano coding method is
* almost never used, since the Huffman coder is almost identical with
* respect to complexity, but performs better.
*
* The Shannon-Fano coder is included in the Basic Compression Library
* mostly for completeness.
*
* Primary flaws with this primitive implementation are:
*  - Slow bit stream implementation
*  - Maximum tree depth of 32 (the coder aborts if any code exceeds a
*    size of 32 bits). If I'm not mistaking, this should not be possible
*    unless the input buffer is larger than 2^32 bytes, which is not
*    supported by the coder anyway (max 2^32-1 bytes can be specified with
*    an unsigned 32-bit integer).
*
* On the other hand, there are a few advantages of this implementation:
*  - The tree is stored in a very compact form, requiring only 12 bits
*    per symbol (for 8 bit symbols), meaning a maximum of 384 bytes
*    overhead.
*  - The code should be fairly easy to follow, if you are familiar with
*    how the Shannon-Fano compression algorithm works.
*-------------------------------------------------------------------------
* Copyright (c) 2003-2006 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/



/*************************************************************************
* Types used for Shannon-Fano coding
*************************************************************************/

typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
} sf_bitstream_t;

typedef struct {
    unsigned int Symbol;
    unsigned int Count;
    unsigned int Code;
    unsigned int Bits;
} sf_sym_t;

typedef struct sf_treenode_struct sf_treenode_t;

struct sf_treenode_struct {
    sf_treenode_t *ChildA, *ChildB;
    int Symbol;
};


/*************************************************************************
* Constants for Shannon-Fano decoding
*************************************************************************/

/* The maximum number of nodes in the Shannon-Fano tree is
  2^(8+1)-1 = 511 */
#define MAX_TREE_NODES 511



/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _SF_InitBitstream() - Initialize a bitstream.
*************************************************************************/

static void _SF_InitBitstream( sf_bitstream_t *stream,
    unsigned char *buf )
{
    stream->BytePtr  = buf;
    stream->BitPos   = 0;
}


/*************************************************************************
* _SF_ReadBit() - Read one bit from a bitstream.
*************************************************************************/

static unsigned int _SF_ReadBit( sf_bitstream_t *stream )
{
    unsigned int  x, bit;
    unsigned char *buf;

    /* Get current stream state */
    buf = stream->BytePtr;
    bit = stream->BitPos;

    /* Extract bit */
    x = (*buf & (1<<(7-bit))) ? 1 : 0;
    bit = (bit+1) & 7;
    if( !bit )
    {
        ++ buf;
    }

    /* Store new stream state */
    stream->BitPos = bit;
    stream->BytePtr = buf;

    return x;
}


/*************************************************************************
* _SF_Read8Bits() - Read eight bits from a bitstream.
*************************************************************************/

static unsigned int _SF_Read8Bits( sf_bitstream_t *stream )
{
    unsigned int  x, bit;
    unsigned char *buf;

    /* Get current stream state */
    buf = stream->BytePtr;
    bit = stream->BitPos;

    /* Extract byte */
    x = (*buf << bit) | (buf[1] >> (8-bit));
    ++ buf;

    /* Store new stream state */
    stream->BytePtr = buf;

    return x;
}


/*************************************************************************
* _SF_WriteBits() - Write bits to a bitstream.
*************************************************************************/

static void _SF_WriteBits( sf_bitstream_t *stream, unsigned int x,
    unsigned int bits )
{
    unsigned int  bit, count;
    unsigned char *buf;
    unsigned int  mask;

    /* Get current stream state */
    buf = stream->BytePtr;
    bit = stream->BitPos;

    /* Append bits */
    mask = 1 << (bits-1);
    for( count = 0; count < bits; ++ count )
    {
        *buf = (*buf & (0xff^(1<<(7-bit)))) +
               ((x & mask ? 1 : 0) << (7-bit));
        x <<= 1;
        bit = (bit+1) & 7;
        if( !bit )
        {
            ++ buf;
        }
    }

    /* Store new stream state */
    stream->BytePtr = buf;
    stream->BitPos  = bit;
}


/*************************************************************************
* _SF_Hist() - Calculate (sorted) histogram for a block of data.
*************************************************************************/

static void _SF_Hist( unsigned char *in, sf_sym_t *sym,
    unsigned int size )
{
    int k, swaps;
    sf_sym_t tmp;

    /* Clear/init histogram */
    for( k = 0; k < 256; ++ k )
    {
        sym[k].Symbol = k;
        sym[k].Count  = 0;
        sym[k].Code   = 0;
        sym[k].Bits   = 0;
    }

    /* Build histogram */
    for( k = size; k; -- k )
    {
        sym[*in ++].Count ++;
    }

    /* Sort histogram - most frequent symbol first (bubble sort) */
    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Count < sym[k+1].Count )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );
}


/*************************************************************************
* _SF_MakeTree() - Generate a Shannon-Fano tree.
*************************************************************************/

static void _SF_MakeTree( sf_sym_t *sym, sf_bitstream_t *stream,
    unsigned int code, unsigned int bits, unsigned int first,
    unsigned int last )
{
    unsigned int k, size, size_a, size_b, last_a, first_b;

    /* Is this a leaf node? */
    if( first == last )
    {
        /* Append symbol to tree description */
        _SF_WriteBits( stream, 1, 1 );
        _SF_WriteBits( stream, sym[first].Symbol, 8 );

        /* Store code info in symbol array */
        sym[first].Code = code;
        sym[first].Bits = bits;
        return;
    }
    else
    {
        /* This was not a leaf node */
        _SF_WriteBits( stream, 0, 1 );
    }

    /* Total size of interval */
    size = 0;
    for( k = first; k <= last; ++ k )
    {
        size += sym[k].Count;
    }

    /* Find size of branch a */
    size_a = 0;
    for( k = first; size_a < ((size+1)>>1) && k < last; ++ k )
    {
        size_a += sym[k].Count;
    }

    /* Non-empty branch? */
    if( size_a > 0 )
    {
        /* Continue branching */
        _SF_WriteBits( stream, 1, 1 );

        /* Branch a cut in histogram */
        last_a  = k-1;

        /* Create branch a */
        _SF_MakeTree( sym, stream, (code<<1)+0, bits+1,
                               first, last_a );
    }
    else
    {
        /* This was an empty branch */
        _SF_WriteBits( stream, 0, 1 );
    }

    /* Size of branch b */
    size_b = size - size_a;

    /* Non-empty branch? */
    if( size_b > 0 )
    {
        /* Continue branching */
        _SF_WriteBits( stream, 1, 1 );

        /* Branch b cut in histogram */
        first_b = k;

        /* Create branch b */
        _SF_MakeTree( sym, stream, (code<<1)+1, bits+1,
                               first_b, last );
    }
    else
    {
        /* This was an empty branch */
        _SF_WriteBits( stream, 0, 1 );
    }
}


/*************************************************************************
* _SF_RecoverTree() - Recover a Shannon-Fano tree from a bitstream.
*************************************************************************/

static sf_treenode_t * _SF_RecoverTree( sf_treenode_t *nodes,
    sf_bitstream_t *stream, unsigned int *nodenum )
{
    sf_treenode_t * this_node;

    /* Pick a node from the node array */
    this_node = &nodes[*nodenum];
    *nodenum = *nodenum + 1;

    /* Clear the node */
    this_node->Symbol = -1;
    this_node->ChildA = (sf_treenode_t *) 0;
    this_node->ChildB = (sf_treenode_t *) 0;

    /* Is this a leaf node? */
    if( _SF_ReadBit( stream ) )
    {
        /* Get symbol from tree description and store in lead node */
        this_node->Symbol = _SF_Read8Bits( stream );

        return this_node;
    }

    /* Non-empty branch A? */
    if( _SF_ReadBit( stream ) )
    {
        /* Create branch A */
        this_node->ChildA = _SF_RecoverTree( nodes, stream, nodenum );
    }

    /* Non-empty branch B? */
    if( _SF_ReadBit( stream ) )
    {
        /* Create branch B */
        this_node->ChildB = _SF_RecoverTree( nodes, stream, nodenum );
    }

    return this_node;
}



/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/


/*************************************************************************
* SF_Compress() - Compress a block of data using a Shannon-Fano coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 384 bytes
*           larger than the input buffer.
*  insize - Number of input bytes.
* The function returns the size of the compressed data.
*************************************************************************/

int SF_Compress( unsigned char *in, unsigned char *out,
    unsigned int insize )
{
    sf_sym_t       sym[256], tmp;
    sf_bitstream_t stream;
    unsigned int     k, total_bytes, swaps, symbol, last_symbol;

    /* Do we have anything to compress? */
    if( insize < 1 ) return 0;

    /* Initialize bitstream */
    _SF_InitBitstream( &stream, out );

    /* Calculate and sort histogram for input data */
    _SF_Hist( in, sym, insize );

    /* Find number of used symbols */
    for( last_symbol = 255; sym[last_symbol].Count == 0; -- last_symbol );

    /* Special case: In order to build a correct tree, we need at least
       two symbols (otherwise we get zero-bit representations). */
    if( last_symbol == 0 ) ++ last_symbol;

    /* Build Shannon-Fano tree */
    _SF_MakeTree( sym, &stream, 0, 0, 0, last_symbol );

    /* Sort histogram - first symbol first (bubble sort) */
    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Symbol > sym[k+1].Symbol )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );

    /* Encode input stream */
    for( k = 0; k < insize; ++ k )
    {
        symbol = in[k];
        _SF_WriteBits( &stream, sym[symbol].Code,
                            sym[symbol].Bits );
    }

    /* Calculate size of output data */
    total_bytes = (int)(stream.BytePtr - out);
    if( stream.BitPos > 0 )
    {
        ++ total_bytes;
    }

    return total_bytes;
}


/*************************************************************************
* SF_Uncompress() - Uncompress a block of data using a Shannon-Fano
* decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*  outsize - Number of output bytes.
*************************************************************************/

void SF_Uncompress( unsigned char *in, unsigned char *out,
    unsigned int insize, unsigned int outsize )
{
    sf_treenode_t  nodes[MAX_TREE_NODES], *root, *node;
    sf_bitstream_t stream;
    unsigned int     k, node_count;
    unsigned char    *buf;

    /* Do we have anything to decompress? */
    if( insize < 1 ) return;

    /* Initialize bitstream */
    _SF_InitBitstream( &stream, in );

    /* Recover Shannon-Fano tree */
    node_count = 0;
    root = _SF_RecoverTree( nodes, &stream, &node_count );

    /* Decode input stream */
    buf = out;
    for( k = 0; k < outsize; ++ k )
    {
        /* Traverse tree until we find a matching leaf node */
        node = root;
        while( node->Symbol < 0 )
        {
          /* Get next node */
          if( _SF_ReadBit( &stream ) )
            node = node->ChildB;
          else
            node = node->ChildA;
        }

        /* We found the matching leaf node and have the symbol */
        *buf ++ = (unsigned char) node->Symbol;
    }
}
