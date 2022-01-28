/*************************************************************************
* Name:        rice.c
* Author:      Marcus Geelnard
* Description: Rice coder/decoder implementation.
* Reentrant:   Yes
*
* Rice coding is a popular compression method for integers that need many
* bits (e.g. 16 or 32 bits), and the data is such that most of the values
* are close to zero. Although Huffman coding should be optimal, it is not
* a very suitable method due to several reasons (for instance, a 32-bit
* word size would require a 16 GB histogram buffer to encode the Huffman
* tree).
*
* The basic idea behind Rice coding is to store as many words as possible
* with less bits than in the original representation (just as with Huffman
* coding). In fact, one can think of the Rice code as a fixed Huffman code
* (i.e. the codes are not determined by the actual statistical content of
* the data, but by the assumption that lower values are more common than
* higher values).
*
* The coding is very simple: Encode the value X with X '1' bits followed by
* a '0' bit. However, there are some optimizations in this implementation:
*
* 1) The k least significant bits of each word are stored as is, and the
* N-k most significant bits are encoded with Rice coding. k is chosen as
* the average number of bits for the previous few samples in the stream.
* This usually makes the best use of the Rice coding, "hides" noise from
* the Rice coder, and does not result in very long Rice codes for signals
* with varying dynamic range.
*
* 2) If the rice code becomes longer than a fixed threshold, T, an
* alternate coding is used: output T '1' bits, followed by
* floor(log2(X-T)) '1' bits, and one '0' bit, followed by X-T (represented
* by the least significant floor(log2(X-T))-1  bits). This gives pretty
* efficient coding even for large values, and prevents ridiculously long
* Rice codes (in the worst case scenario, a single Rice code for a 32-bit
* word may become as long as 2^32 bits, or 512 MB).
*
* If the threshold is set to 4, then the following is the resulting code
* table:
*
*  X  bin      Rice                  Thresholded Rice     Difference
* ------------------------------------------------------------------
*  0  00000    0                     0
*  1  00001    10                    10
*  2  00010    110                   110
*  3  00011    1110                  1110
*  4  00100    11110                 11110
*  5  00101    111110                111110
*  6  00110    1111110               11111100             +1
*  7  00111    11111110              11111101
*  8  01000    111111110             1111111000           +1
*  9  01001    1111111110            1111111001
* 10  01010    11111111110           1111111010           -1
* 11  01011    111111111110          1111111011           -2
* 12  01100    1111111111110         111111110000
* 13  01101    11111111111110        111111110001         -1
* 14  01110    111111111111110       111111110010         -2
* 15  01111    1111111111111110      111111110011         -3
* 16  10000    11111111111111110     111111110100         -4
* 17  10001    111111111111111110    111111110101         -5
* 18  10010    1111111111111111110   111111110110         -6
* 19  10011    11111111111111111110  111111110111         -7
* 20  10100    111111111111111111110 11111111100000       -5
* ...
*
* As you can see, only two codes result in a worse representation with the
* threshold method used in this implementation. The rest of the codes
* result in shorter or equally short codes as for standard Rice coding.
*
* 3) In the worst case scenario, the output buffer may grow by several
* orders of magnitude compared to the input buffer. Therefor the Rice
* coder in this implementation aborts if the output becomes larger than
* the input by simply making a copy of the input buffer to the output
* buffer, with a leading zero byte (making the output at most one byte
* larger than the input).
*
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

#include "rice.h"



/*************************************************************************
* Constants used for Rice coding
*************************************************************************/

/* Number of words to use for determining the optimum k */
#define RICE_HISTORY 16

/* Maximum length of Rice codes */
#define RICE_THRESHOLD 8


/*************************************************************************
* Types used for Rice coding
*************************************************************************/

typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
    unsigned int  NumBytes;
} rice_bitstream_t;


/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _Rice_NumBits() - Determine number of information bits in a word.
*************************************************************************/

static int _Rice_NumBits( unsigned int x )
{
    int n;
    for( n = 32; !(x & 0x80000000) && (n > 0); -- n ) x <<= 1;
    return n;
}


/*************************************************************************
* _Rice_InitBitstream() - Initialize a bitstream.
*************************************************************************/

static void _Rice_InitBitstream( rice_bitstream_t *stream,
    void *buf, unsigned int bytes )
{
    stream->BytePtr  = (unsigned char *) buf;
    stream->BitPos   = 0;
    stream->NumBytes = bytes;
}


/*************************************************************************
* _Rice_ReadBit() - Read a bit from the input stream.
*************************************************************************/

static int _Rice_ReadBit( rice_bitstream_t *stream )
{
    unsigned int x, bit, idx;

    idx = stream->BitPos >> 3;
    if( idx < stream->NumBytes )
    {
        bit = 7 - (stream->BitPos & 7);
        x = (stream->BytePtr[ idx ] >> bit) & 1;
        ++ stream->BitPos;
    }
    else
    {
        x = 0;
    }
    return x;
}


/*************************************************************************
* _Rice_WriteBit() - Write a bit to the output stream.
*************************************************************************/

static void _Rice_WriteBit( rice_bitstream_t *stream, int x )
{
    unsigned int bit, idx, mask, set;

    idx = stream->BitPos >> 3;
    if( idx < stream->NumBytes )
    {
        bit  = 7 - (stream->BitPos & 7);
        mask = 0xff ^ (1 << bit);
        set  = (x & 1) << bit;
        stream->BytePtr[ idx ] = (stream->BytePtr[ idx ] & mask) | set;
        ++ stream->BitPos;
    }
}


/*************************************************************************
* _Rice_EncodeWord() - Encode and write a word to the output stream.
*************************************************************************/

static void _Rice_EncodeWord( unsigned int x, int k,
    rice_bitstream_t *stream )
{
    unsigned int q, i;
    int          j, o;

    /* Determine overflow */
    q = x >> k;

    /* Too large rice code? */
    if( q > RICE_THRESHOLD )
    {
        /* Write Rice code (except for the final zero) */
        for( j = 0; j < RICE_THRESHOLD; ++ j )
        {
            _Rice_WriteBit( stream, 1 );
        }

        /* Encode the overflow with alternate coding */
        q -= RICE_THRESHOLD;

        /* Write number of bits needed to represent the overflow */
        o = _Rice_NumBits( q );
        for( j = 0; j < o; ++ j )
        {
            _Rice_WriteBit( stream, 1 );
        }
        _Rice_WriteBit( stream, 0 );

        /* Write the o-1 least significant bits of q "as is" */
        for( j = o-2; j >= 0; -- j )
        {
            _Rice_WriteBit( stream, (q >> j) & 1 );
        }
    }
    else
    {
        /* Write Rice code */
        for( i = 0; i < q; ++ i )
        {
            _Rice_WriteBit( stream, 1 );
        }
        _Rice_WriteBit( stream, 0 );
    }

    /* Encode the rest of the k bits */
    for( j = k-1; j >= 0; -- j )
    {
        _Rice_WriteBit( stream, (x >> j) & 1 );
    }
}


/*************************************************************************
* _Rice_DecodeWord() - Read and decode a word from the input stream.
*************************************************************************/

static unsigned int _Rice_DecodeWord( int k, rice_bitstream_t *stream )
{
    unsigned int x, q;
    int          i, o;

    /* Decode Rice code */
    q = 0;
    while( _Rice_ReadBit( stream ) )
    {
        ++ q;
    }

    /* Too large Rice code? */
    if( q > RICE_THRESHOLD )
    {
        /* Bits needed for the overflow part... */
        o = q - RICE_THRESHOLD;

        /* Read additional bits (MSB is always 1) */
        x = 1;
        for( i = 0; i < o-1; ++ i )
        {
            x = (x<<1) | _Rice_ReadBit( stream );
        }

        /* Add Rice code */
        x += RICE_THRESHOLD;
    }
    else
    {
        x = q;
    }

    /* Decode the rest of the k bits */
    for( i = k-1; i >= 0; -- i )
    {
        x = (x<<1) | _Rice_ReadBit( stream );
    }

    return x;
}


/*************************************************************************
* _Rice_ReadWord() - Read a word from the input stream, and convert it to
* a signed magnitude 32-bit representation (regardless of input format).
*************************************************************************/

static unsigned int _Rice_ReadWord( void *ptr, unsigned int idx,
    int format )
{
    int            sx;
    unsigned int   x;

    /* Read a word with the appropriate format from the stream */
    switch( format )
    {
        case RICE_FMT_INT8:
            sx = (int)((signed char *) ptr)[ idx ];
            x = sx < 0 ? -1-(sx<<1) : sx<<1;
            break;
        case RICE_FMT_UINT8:
            x = (unsigned int)((unsigned char *) ptr)[ idx ];
            break;

        case RICE_FMT_INT16:
            sx = (int)((signed short *) ptr)[ idx ];
            x = sx < 0 ? -1-(sx<<1) : sx<<1;
            break;
        case RICE_FMT_UINT16:
            x = (unsigned int)((unsigned short *) ptr)[ idx ];
            break;

        case RICE_FMT_INT32:
            sx = ((int *) ptr)[ idx ];
            x = sx < 0 ? -1-(sx<<1) : sx<<1;
            break;
        case RICE_FMT_UINT32:
            x = ((unsigned int *) ptr)[ idx ];
            break;

        default:
            x = 0;
    }

    return x;
}


/*************************************************************************
* _Rice_WriteWord() - Convert a signed magnitude 32-bit word to the given
* format, and write it to the otuput stream.
*************************************************************************/

static void _Rice_WriteWord( void *ptr, unsigned int idx, int format,
    unsigned int x )
{
    int sx;

    /* Write a word with the appropriate format to the stream */
    switch( format )
    {
        case RICE_FMT_INT8:
            sx = (x & 1) ? -(int)((x+1)>>1) : (int)(x>>1);
            ((signed char *) ptr)[ idx ] = sx;
            break;
        case RICE_FMT_UINT8:
            ((unsigned char *) ptr)[ idx ] = x;
            break;

        case RICE_FMT_INT16:
            sx = (x & 1) ? -(int)((x+1)>>1) : (int)(x>>1);
            ((signed short *) ptr)[ idx ] = sx;
            break;
        case RICE_FMT_UINT16:
            ((unsigned short *) ptr)[ idx ] = x;
            break;

        case RICE_FMT_INT32:
            sx = (x & 1) ? -(int)((x+1)>>1) : (int)(x>>1);
            ((int *) ptr)[ idx ] = sx;
            break;
        case RICE_FMT_UINT32:
            ((unsigned int *) ptr)[ idx ] = x;
            break;
    }
}



/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/


/*************************************************************************
* Rice_Compress() - Compress a block of data using a Rice coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must one byte larger
*           than the input buffer.
*  insize - Number of input bytes.
*  format - Binary format (see rice.h)
* The function returns the size of the compressed data.
*************************************************************************/

int Rice_Compress( void *in, void *out, unsigned int insize, int format )
{
    rice_bitstream_t stream;
    unsigned int     i, x, k, n, wordsize, incount;
    unsigned int     hist[ RICE_HISTORY ];
    int              j;

    /* Calculate number of input words */
    switch( format )
    {
        case RICE_FMT_INT8:
        case RICE_FMT_UINT8:  wordsize = 8; break;
        case RICE_FMT_INT16:
        case RICE_FMT_UINT16: wordsize = 16; break;
        case RICE_FMT_INT32:
        case RICE_FMT_UINT32: wordsize = 32; break;
        default: return 0;
    }
    incount = insize / (wordsize>>3);

    /* Do we have anything to compress? */
    if( incount == 0 )
    {
        return 0;
    }

    /* Initialize output bitsream */
    _Rice_InitBitstream( &stream, out, insize+1 );

    /* Determine a good initial k */
    k = 0;
    for( i = 0; (i < RICE_HISTORY) && (i < incount); ++ i )
    {
        n = _Rice_NumBits( _Rice_ReadWord( in, i, format ) );
        k += n;
    }
    k = (k + (i>>1)) / i;
    if( k == 0 ) k = 1;

    /* Write k to the output stream (the decoder needs it) */
    ((unsigned char *) out)[0] = k;
    stream.BitPos = 8;

    /* Encode input stream */
    for( i = 0; (i < incount) && ((stream.BitPos>>3) <= insize); ++ i )
    {
        /* Revise optimum k? */
        if( i >= RICE_HISTORY )
        {
            k = 0;
            for( j = 0; j < RICE_HISTORY; ++ j )
            {
                k += hist[ j ];
            }
            k = (k + (RICE_HISTORY>>1)) / RICE_HISTORY;
        }

        /* Read word from input buffer */
        x = _Rice_ReadWord( in, i, format );

        /* Encode word to output buffer */
        _Rice_EncodeWord( x, k, &stream );

        /* Update history */
        hist[ i % RICE_HISTORY ] = _Rice_NumBits( x );
    }

    /* Was there a buffer overflow? */
    if( i < incount )
    {
        /* Indicate that the buffer was not compressed */
        ((unsigned char *) out)[0] = 0;

        /* Rewind bitstream and fill it with raw words */
        stream.BitPos = 8;
        for( i = 0; i < incount; ++ i )
        {
            x = _Rice_ReadWord( in, i, format );
            for( j = wordsize-1; j >= 0; -- j )
            {
                _Rice_WriteBit( &stream, (x >> j) & 1 );
            }
        }
    }

    return (stream.BitPos+7) >> 3;
}


/*************************************************************************
* Rice_Uncompress() - Uncompress a block of data using a Rice decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*  outsize - Number of output bytes.
*  format  - Binary format (see rice.h)
*************************************************************************/

void Rice_Uncompress( void *in, void *out, unsigned int insize,
  unsigned int outsize, int format )
{
    rice_bitstream_t stream;
    unsigned int     i, x, k, wordsize, outcount;
    unsigned int     hist[ RICE_HISTORY ];
    int              j;

    /* Calculate number of output words */
    switch( format )
    {
        case RICE_FMT_INT8:
        case RICE_FMT_UINT8:  wordsize = 8; break;
        case RICE_FMT_INT16:
        case RICE_FMT_UINT16: wordsize = 16; break;
        case RICE_FMT_INT32:
        case RICE_FMT_UINT32: wordsize = 32; break;
        default: return;
    }
    outcount = outsize / (wordsize>>3);

    /* Do we have anything to decompress? */
    if( outcount == 0 )
    {
        return;
    }

    /* Initialize input bitsream */
    _Rice_InitBitstream( &stream, in, insize );

    /* Get initial k */
    k = ((unsigned char *) in)[0];
    stream.BitPos = 8;

    /* Was the buffer not compressed */
    if( k == 0 )
    {
        /* Copy raw words from input stream */
        for( i = 0; i < outcount; ++ i )
        {
            x = 0;
            for( j = wordsize-1; j >= 0; -- j )
            {
                x = (x<<1) | _Rice_ReadBit( &stream );
            }
            _Rice_WriteWord( out, i, format, x );
        }
    }
    else
    {
        /* Decode input stream */
        for( i = 0; i < outcount; ++ i )
        {
            /* Revise optimum k? */
            if( i >= RICE_HISTORY )
            {
                k = 0;
                for( j = 0; j < RICE_HISTORY; ++ j )
                {
                    k += hist[ j ];
                }
                k = (k + (RICE_HISTORY>>1)) / RICE_HISTORY;
            }

            /* Decode word from input buffer */
            x = _Rice_DecodeWord( k, &stream );

            /* Write word to output buffer */
            _Rice_WriteWord( out, i, format, x );

            /* Update history */
            hist[ i % RICE_HISTORY ] = _Rice_NumBits( x );
        }
    }
}
