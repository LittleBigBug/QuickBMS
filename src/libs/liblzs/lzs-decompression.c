/*****************************************************************************
 *
 * \file
 *
 * \brief LZS Decompression
 *
 * This implements LZS (Lempel-Ziv-Stac) decompression, which is an LZ77
 * derived algorithm with a 2kB sliding window and Huffman coding.
 *
 * See:
 *     * ANSI X3.241-1994
 *     * RFC 1967
 *     * RFC 1974
 *     * RFC 2395
 *     * RFC 3943
 *
 * This code is licensed according to the MIT license as follows:
 * ----------------------------------------------------------------------------
 * Copyright (c) 2017 Craig McQueen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ----------------------------------------------------------------------------
 ****************************************************************************/


/*****************************************************************************
 * Includes
 ****************************************************************************/

#include "lzs.h"
#include "lzs-common.h"

#include <stdint.h>

//#include <inttypes.h>
//#include <ctype.h>
//#include <stdio.h>


/*****************************************************************************
 * Defines
 ****************************************************************************/

#define LENGTH_DECODE_METHOD_CODE   0
#define LENGTH_DECODE_METHOD_TABLE  1u
// Choose which method to use
#define LENGTH_DECODE_METHOD        LENGTH_DECODE_METHOD_TABLE

//#define LZS_DEBUG(X)    printf X
#define LZS_DEBUG(X)

#define LZS_ASSERT(X)


/*****************************************************************************
 * Typedefs
 ****************************************************************************/

typedef enum
{
    DECOMPRESS_NORMAL,
    DECOMPRESS_EXTENDED
} SimpleDecompressState_t;

typedef enum
{
    DECOMPRESS_COPY_DATA,           // Must come before DECOMPRESS_GET_TOKEN_TYPE, so state transition can be done by increment
    DECOMPRESS_GET_TOKEN_TYPE,
    DECOMPRESS_GET_LITERAL,
    DECOMPRESS_GET_OFFSET_TYPE,
    DECOMPRESS_GET_OFFSET_SHORT,
    DECOMPRESS_GET_OFFSET_LONG,
    DECOMPRESS_GET_LENGTH,
    DECOMPRESS_COPY_EXTENDED_DATA,  // Must come before DECOMPRESS_GET_EXTENDED_LENGTH, so state transition can be done by increment
    DECOMPRESS_GET_EXTENDED_LENGTH,

    NUM_DECOMPRESS_STATES
} LzsDecompressState_t;


/*****************************************************************************
 * Tables
 ****************************************************************************/

#if LENGTH_DECODE_METHOD == LENGTH_DECODE_METHOD_TABLE

static const uint8_t lengthDecodeTable[(1u << LENGTH_MAX_BIT_WIDTH)] =
{
    /* Length is encoded as:
     *  0b00 --> 2
     *  0b01 --> 3
     *  0b10 --> 4
     *  0b1100 --> 5
     *  0b1101 --> 6
     *  0b1110 --> 7
     *  0b1111 xxxx --> 8 (extended)
     */
    // Look at 4 bits. Map 0bWXYZ to a length value, and a number of bits actually used for symbol.
    // High 4 bits are length value. Low 4 bits are the width of the bit field.
    0x22, 0x22, 0x22, 0x22,     // 0b00 --> 2
    0x32, 0x32, 0x32, 0x32,     // 0b01 --> 3
    0x42, 0x42, 0x42, 0x42,     // 0b10 --> 4
    0x54, 0x64, 0x74, 0x84,     // 0b11xy --> 5, 6, 7, and also 8 (see MAX_SHORT_LENGTH) which goes into extended lengths
};
#endif


static const uint_fast8_t StateBitMinimumWidth[NUM_DECOMPRESS_STATES] =
{
    0,                          // DECOMPRESS_COPY_DATA,
    1u,                         // DECOMPRESS_GET_TOKEN_TYPE,
    8u,                         // DECOMPRESS_GET_LITERAL,
    1u,                         // DECOMPRESS_GET_OFFSET_TYPE,
    SHORT_OFFSET_BITS,          // DECOMPRESS_GET_OFFSET_SHORT,
    LONG_OFFSET_BITS,           // DECOMPRESS_GET_OFFSET_LONG,
    0,                          // DECOMPRESS_GET_LENGTH,
    0,                          // DECOMPRESS_COPY_EXTENDED_DATA,
    LENGTH_MAX_BIT_WIDTH,       // DECOMPRESS_GET_EXTENDED_LENGTH,
};


/*****************************************************************************
 * Functions
 ****************************************************************************/

/*
 * Single-call decompression
 *
 * No state is kept between calls. Decompression is expected to complete in a single call.
 * It will stop if/when it reaches the end of either the input or the output buffer,
 * or when it reaches an end-marker.
 */
size_t lzs_decompress(uint8_t * a_pOutData, size_t a_outBufferSize, const uint8_t * a_pInData, size_t a_inLen)
{
    const uint8_t     * inPtr;
    uint8_t           * outPtr;
    size_t              inRemaining;        // Count of remaining bytes of input
    size_t              outCount;           // Count of output bytes that have been generated
    uint32_t            bitFieldQueue;      // Code assumes bits will disappear past MS-bit 31 when shifted left.
    uint_fast8_t        bitFieldQueueLen;
    uint_fast16_t       offset = 0;
    uint_fast8_t        length;
    uint8_t             temp8;
    SimpleDecompressState_t state;


    bitFieldQueue = 0;
    bitFieldQueueLen = 0;
    inPtr = a_pInData;
    outPtr = a_pOutData;
    inRemaining = a_inLen;
    outCount = 0;
    state = DECOMPRESS_NORMAL;

    for (;;)
    {
        // Load input data into the bit field queue
        while ((inRemaining > 0) && (bitFieldQueueLen <= BIT_QUEUE_BITS - 8u))
        {
            bitFieldQueue |= (*inPtr++ << (BIT_QUEUE_BITS - 8u - bitFieldQueueLen));
            bitFieldQueueLen += 8u;
            //LZS_DEBUG(("Load queue: %04X\n", bitFieldQueue));
            inRemaining--;
        }
        // Check if we've reached the end of our input data
        if (bitFieldQueueLen == 0)
        {
            break;
        }
        if (bitFieldQueueLen > BIT_QUEUE_BITS)
        {
            // It is an error if we ever get here.
            LZS_ASSERT(0);
            break;
        }
        // Check if we've run out of output buffer space
        if (outCount >= a_outBufferSize)
        {
            break;
        }

        switch (state)
        {
            case DECOMPRESS_NORMAL:
                // Get token-type bit
                //      0 means literal byte
                //      1 means offset/length token

                // We don't need to check bitFieldQueueLen here because
                // we already checked above that there is at least 1 bit.
                temp8 = (bitFieldQueue & (1u << (BIT_QUEUE_BITS - 1u))) ? 1u : 0;
                bitFieldQueue <<= 1u;
                bitFieldQueueLen--;
                if (temp8 == 0)
                {
                    // Literal
                    if (bitFieldQueueLen < 8u)
                    {
                        goto finish;
                    }
                    temp8 = (uint8_t) (bitFieldQueue >> (BIT_QUEUE_BITS - 8u));
                    bitFieldQueue <<= 8u;
                    bitFieldQueueLen -= 8u;
                    LZS_DEBUG(("Literal %c (%02X)\n", isprint(temp8) ? temp8 : '?', temp8));

                    // Write to output
                    // Not necessary to check for space, because that was done at the top of the main loop.
                    *outPtr++ = temp8;
                    outCount++;
                }
                else
                {
                    // Offset+length token
                    // Decode offset
                    if (bitFieldQueueLen < 1u)
                    {
                        goto finish;
                    }
                    temp8 = (bitFieldQueue & (1u << (BIT_QUEUE_BITS - 1u))) ? 1u : 0;
                    bitFieldQueue <<= 1u;
                    bitFieldQueueLen--;
                    if (temp8)
                    {
                        // Short offset
                        if (bitFieldQueueLen < SHORT_OFFSET_BITS)
                        {
                            goto finish;
                        }
                        offset = bitFieldQueue >> (BIT_QUEUE_BITS - SHORT_OFFSET_BITS);
                        bitFieldQueue <<= SHORT_OFFSET_BITS;
                        bitFieldQueueLen -= SHORT_OFFSET_BITS;
                        if (offset == 0)
                        {
                            LZS_DEBUG(("End marker\n"));
#if 1
                            // Stop at end marker
                            goto finish;
#else
                            // Discard any bits that are fractions of a byte, to align with a byte boundary
                            temp8 = bitFieldQueueLen % 8u;
                            bitFieldQueue <<= temp8;
                            bitFieldQueueLen -= temp8;
#endif
                        }
                    }
                    else
                    {
                        // Long offset
                        if (bitFieldQueueLen < LONG_OFFSET_BITS)
                        {
                            goto finish;
                        }
                        offset = bitFieldQueue >> (BIT_QUEUE_BITS - LONG_OFFSET_BITS);
                        bitFieldQueue <<= LONG_OFFSET_BITS;
                        bitFieldQueueLen -= LONG_OFFSET_BITS;
                    }
                    if (offset != 0)
                    {
                        // Decode length and copy characters
#if LENGTH_DECODE_METHOD == LENGTH_DECODE_METHOD_CODE
                        /* Length is encoded as:
                         *  0b00 --> 2
                         *  0b01 --> 3
                         *  0b10 --> 4
                         *  0b1100 --> 5
                         *  0b1101 --> 6
                         *  0b1110 --> 7
                         *  0b1111 xxxx --> 8 (extended)
                         */
                        // Get 4 bits
                        temp8 = (uint8_t) (bitFieldQueue >> (BIT_QUEUE_BITS - 4u));
                        if (temp8 < 0xC)    // 0xC is 0b1100
                        {
                            // Length of 2, 3 or 4, encoded in 2 bits
                            if (bitFieldQueueLen < 2u)
                            {
                                goto finish;
                            }
                            length = (temp8 >> 2u) + 2u;
                            bitFieldQueue <<= 2u;
                            bitFieldQueueLen -= 2u;
                        }
                        else
                        {
                            // Length (encoded in 4 bits) of 5, 6, 7, or (8 + extended)
                            if (bitFieldQueueLen < 4u)
                            {
                                goto finish;
                            }
                            length = (temp8 - 0xC + 5u);
                            bitFieldQueue <<= 4u;
                            bitFieldQueueLen -= 4u;
                            if (length == 8u)
                            {
                                // We must go into extended length decode mode
                                state = DECOMPRESS_EXTENDED;
                            }
                        }
#endif
#if LENGTH_DECODE_METHOD == LENGTH_DECODE_METHOD_TABLE
                        // Get 4 bits, then look up decode data
                        temp8 = lengthDecodeTable[
                                                  (uint8_t) (bitFieldQueue >> (BIT_QUEUE_BITS - LENGTH_MAX_BIT_WIDTH))
                                                 ];
                        // Length value is in upper nibble
                        length = temp8 >> 4u;
                        // Number of bits for this length token is in the lower nibble
                        temp8 &= 0xF;
                        if (bitFieldQueueLen < temp8)
                        {
                            goto finish;
                        }
                        bitFieldQueue <<= temp8;
                        bitFieldQueueLen -= temp8;
                        if (length == MAX_SHORT_LENGTH)
                        {
                            // We must go into extended length decode mode
                            state = DECOMPRESS_EXTENDED;
                        }
#endif
                        LZS_DEBUG(("(%"PRIuFAST16", %"PRIuFAST8")\n", offset, length));
                        // Now copy (offset, length) bytes
                        for (temp8 = 0; temp8 < length; temp8++)
                        {
                            // Check offset is within range of valid history.
                            // If it's not, then write zeros. Avoid information leak.
                            if (outPtr - offset >= a_pOutData)
                            {
                                *outPtr = *(outPtr - offset);
                            }
                            else
                            {
                                *outPtr = 0;
                            }
                            ++outPtr;
                            ++outCount;

                            if (outCount >= a_outBufferSize)
                            {
                                goto finish;
                            }
                        }
                    }
                }
                break;

            case DECOMPRESS_EXTENDED:
                // Extended length token
                // Get 4 bits
                if (bitFieldQueueLen < LENGTH_MAX_BIT_WIDTH)
                {
                    goto finish;
                }
                length = (uint8_t) (bitFieldQueue >> (BIT_QUEUE_BITS - LENGTH_MAX_BIT_WIDTH));
                bitFieldQueue <<= LENGTH_MAX_BIT_WIDTH;
                bitFieldQueueLen -= LENGTH_MAX_BIT_WIDTH;
                // Now copy (offset, length) bytes
                for (temp8 = 0; temp8 < length; temp8++)
                {
                    // Check offset is within range of valid history.
                    // If it's not, then write zeros. Avoid information leak.
                    if (outPtr - offset >= a_pOutData)
                    {
                        *outPtr = *(outPtr - offset);
                    }
                    else
                    {
                        *outPtr = 0;
                    }
                    ++outPtr;
                    ++outCount;

                    if (outCount >= a_outBufferSize)
                    {
                        goto finish;
                    }
                }
                if (length != MAX_EXTENDED_LENGTH)
                {
                    // We're finished with extended length decode mode; go back to normal
                    state = DECOMPRESS_NORMAL;
                }
                break;
        }
    }

finish:
    return outCount;
}


/*
 * \brief Initialise incremental decompression
 */
void lzs_decompress_init(LzsDecompressParameters_t * pParams)
{
    pParams->status = LZS_D_STATUS_NONE;
    pParams->bitFieldQueue = 0;
    pParams->bitFieldQueueLen = 0;
    pParams->state = DECOMPRESS_GET_TOKEN_TYPE;
    pParams->historyLatestIdx = 0;
    pParams->historyLen = 0;
}


/*
 * \brief Incremental decompression
 *
 * State is kept between calls, so decompression can be done gradually, and flexibly
 * depending on the application's needs for input/output buffer handling.
 *
 * It will stop if/when it reaches the end of either the input or the output buffer.
 * It will also stop if/when it reaches an end marker.
 */
size_t lzs_decompress_incremental(LzsDecompressParameters_t * pParams)
{
    size_t              outCount;           // Count of output bytes that have been generated
    uint_fast16_t       offset;
    uint_fast8_t        temp8;


    pParams->status = LZS_D_STATUS_NONE;
    outCount = 0;

    for (;;)
    {
        // Load input data into the bit field queue
        while ((pParams->inLength > 0) && (pParams->bitFieldQueueLen <= BIT_QUEUE_BITS - 8u))
        {
            pParams->bitFieldQueue |= (*pParams->inPtr++ << (BIT_QUEUE_BITS - 8u - pParams->bitFieldQueueLen));
            pParams->bitFieldQueueLen += 8u;
            //LZS_DEBUG(("Load queue: %04X\n", pParams->bitFieldQueue));
            pParams->inLength--;
        }
        // Check if we've reached the end of our input data
        if (pParams->bitFieldQueueLen == 0)
        {
            pParams->status |= LZS_D_STATUS_INPUT_FINISHED | LZS_D_STATUS_INPUT_STARVED;
        }
        if (pParams->bitFieldQueueLen > BIT_QUEUE_BITS)
        {
            // It is an error if we ever get here.
            LZS_ASSERT(0);
            pParams->status |= LZS_D_STATUS_ERROR | LZS_D_STATUS_INPUT_FINISHED | LZS_D_STATUS_INPUT_STARVED;
        }
        // Check if we have enough input data to do something useful
        if (pParams->bitFieldQueueLen < StateBitMinimumWidth[pParams->state])
        {
            // We don't have enough input bits, so we're done for now.
            pParams->status |= LZS_D_STATUS_INPUT_STARVED;
        }

        // Check if we need to finish for whatever reason
        if (pParams->status != LZS_D_STATUS_NONE)
        {
            // Break out of the top-level loop
            break;
        }

        // Process input data in a state machine
        switch (pParams->state)
        {
            case DECOMPRESS_GET_TOKEN_TYPE:
                // Get token-type bit
                if (pParams->bitFieldQueue & (1u << (BIT_QUEUE_BITS - 1u)))
                {
                    pParams->state = DECOMPRESS_GET_OFFSET_TYPE;
                }
                else
                {
                    pParams->state = DECOMPRESS_GET_LITERAL;
                }
                pParams->bitFieldQueue <<= 1u;
                pParams->bitFieldQueueLen--;
                break;

            case DECOMPRESS_GET_LITERAL:
                // Literal
                // Check if we have space in the output buffer
                if (pParams->outLength == 0)
                {
                    pParams->status |= LZS_D_STATUS_NO_OUTPUT_BUFFER_SPACE;
                }
                else
                {
                    temp8 = (uint8_t) (pParams->bitFieldQueue >> (BIT_QUEUE_BITS - 8u));
                    pParams->bitFieldQueue <<= 8u;
                    pParams->bitFieldQueueLen -= 8u;
                    LZS_DEBUG(("Literal %c (%02X)\n", isprint(temp8) ? temp8 : '?', temp8));

                    *pParams->outPtr++ = temp8;
                    pParams->outLength--;
                    outCount++;

                    // Write to history
                    pParams->historyBuffer[pParams->historyLatestIdx] = temp8;

                    pParams->historyLatestIdx = lzs_idx_inc_wrap(pParams->historyLatestIdx, 1u,
                                                                sizeof(pParams->historyBuffer));
                    pParams->historyLen = LZSMIN(pParams->historyLen + 1u, LZS_MAX_HISTORY_SIZE);

                    pParams->state = DECOMPRESS_GET_TOKEN_TYPE;
                }
                break;

            case DECOMPRESS_GET_OFFSET_TYPE:
                // Offset+length token
                // Decode offset
                temp8 = (pParams->bitFieldQueue & (1u << (BIT_QUEUE_BITS - 1u))) ? 1u : 0;
                pParams->bitFieldQueue <<= 1u;
                pParams->bitFieldQueueLen--;
                pParams->state = temp8 ? DECOMPRESS_GET_OFFSET_SHORT : DECOMPRESS_GET_OFFSET_LONG;
                break;

            case DECOMPRESS_GET_OFFSET_SHORT:
                // Short offset
                offset = pParams->bitFieldQueue >> (BIT_QUEUE_BITS - SHORT_OFFSET_BITS);
                pParams->bitFieldQueue <<= SHORT_OFFSET_BITS;
                pParams->bitFieldQueueLen -= SHORT_OFFSET_BITS;
                if (offset == 0)
                {
                    LZS_DEBUG(("End marker\n"));
                    // Discard any bits that are fractions of a byte, to align with a byte boundary
                    temp8 = pParams->bitFieldQueueLen % 8u;
                    pParams->bitFieldQueue <<= temp8;
                    pParams->bitFieldQueueLen -= temp8;

                    // Set status saying we found an end marker
                    pParams->status |= LZS_D_STATUS_END_MARKER;

                    pParams->state = DECOMPRESS_GET_TOKEN_TYPE;
                }
                else
                {
                    LZS_DEBUG(("Short offset %"PRIuFAST16"\n", offset));
                    pParams->offset = offset;
                    pParams->state = DECOMPRESS_GET_LENGTH;
                }
                break;

        case DECOMPRESS_GET_OFFSET_LONG:
                // Long offset
                pParams->offset = pParams->bitFieldQueue >> (BIT_QUEUE_BITS - LONG_OFFSET_BITS);
                LZS_DEBUG(("Long offset %"PRIuFAST16"\n", pParams->offset));
                pParams->bitFieldQueue <<= LONG_OFFSET_BITS;
                pParams->bitFieldQueueLen -= LONG_OFFSET_BITS;

                pParams->state = DECOMPRESS_GET_LENGTH;
                break;

            case DECOMPRESS_GET_LENGTH:
                // Decode length and copy characters
#if LENGTH_DECODE_METHOD == LENGTH_DECODE_METHOD_CODE
                /* Length is encoded as:
                 *  0b00 --> 2
                 *  0b01 --> 3
                 *  0b10 --> 4
                 *  0b1100 --> 5
                 *  0b1101 --> 6
                 *  0b1110 --> 7
                 *  0b1111 xxxx --> 8 (extended)
                 */
                // Get 4 bits
                temp8 = (uint8_t) (pParams->bitFieldQueue >> (BIT_QUEUE_BITS - 4u));
                if (temp8 < 0xC)    // 0xC is 0b1100
                {
                    // Length of 2, 3 or 4, encoded in 2 bits
                    pParams->length = (temp8 >> 2u) + 2u;
                    temp8 = 2u;
                }
                else
                {
                    // Length (encoded in 4 bits) of 5, 6, 7, or (8 + extended)
                    pParams->length = (temp8 - 0xC + 5u);
                    temp8 = 4u;
                }
#endif
#if LENGTH_DECODE_METHOD == LENGTH_DECODE_METHOD_TABLE
                // Get 4 bits, then look up decode data
                temp8 = lengthDecodeTable[
                                          pParams->bitFieldQueue >> (BIT_QUEUE_BITS - LENGTH_MAX_BIT_WIDTH)
                                         ];
                // Length value is in upper nibble
                pParams->length = temp8 >> 4u;
                // Number of bits for this length token is in the lower nibble
                temp8 &= 0xF;
#endif
                if (pParams->bitFieldQueueLen < temp8)
                {
                    // We don't have enough input bits, so we're done for now.
                    pParams->status |= LZS_D_STATUS_INPUT_STARVED;
                }
                else
                {
                    LZS_DEBUG(("Length %"PRIuFAST8"\n", pParams->length));
                    pParams->bitFieldQueue <<= temp8;
                    pParams->bitFieldQueueLen -= temp8;
                    if (pParams->length == MAX_SHORT_LENGTH)
                    {
                        // We must go into extended length decode mode
                        pParams->state = DECOMPRESS_COPY_EXTENDED_DATA;
                    }
                    else
                    {
                        pParams->state = DECOMPRESS_COPY_DATA;
                    }

                    // Do some offset calculations before beginning to copy
                    offset = pParams->offset;
                    LZS_ASSERT(offset <= sizeof(pParams->historyBuffer));
                    pParams->historyReadIdx = lzs_idx_dec_wrap(pParams->historyLatestIdx, offset,
                                                                sizeof(pParams->historyBuffer));
                    //LZS_DEBUG(("(%"PRIuFAST16", %"PRIuFAST8")\n", offset, pParams->length));
                }
                break;

            case DECOMPRESS_COPY_DATA:
            case DECOMPRESS_COPY_EXTENDED_DATA:
                // Copy (offset, length) bytes.
                // Offset has already been used to calculate pParams->historyReadIdx.
                offset = pParams->offset;
                for (;;)
                {
                    if (pParams->length == 0)
                    {
                        // We're finished copying. Change state, and exit this inner copying loop.
                        pParams->state++;   // Goes to either DECOMPRESS_GET_TOKEN_TYPE or DECOMPRESS_GET_EXTENDED_LENGTH
                        break;
                    }
                    // Check if we have space in the output buffer
                    if (pParams->outLength == 0)
                    {
                        // We're out of space in the output buffer.
                        // Set status, exit this inner copying loop, but maintain the current state.
                        pParams->status |= LZS_D_STATUS_NO_OUTPUT_BUFFER_SPACE;
                        break;
                    }

                    // Get byte from history.
                    // Check offset is within range of valid history.
                    // If it's not, then write zeros. Avoid information leak.
                    if (offset <= pParams->historyLen)
                    {
                        temp8 = pParams->historyBuffer[pParams->historyReadIdx];
                    }
                    else
                    {
                        temp8 = 0;
                    }

                    pParams->historyReadIdx = lzs_idx_inc_wrap(pParams->historyReadIdx, 1u,
                                                                sizeof(pParams->historyBuffer));

                    // Write to output
                    *pParams->outPtr++ = temp8;
                    pParams->outLength--;
                    pParams->length--;
                    ++outCount;

                    // Write to history
                    pParams->historyBuffer[pParams->historyLatestIdx] = temp8;

                    pParams->historyLatestIdx = lzs_idx_inc_wrap(pParams->historyLatestIdx, 1u,
                                                                sizeof(pParams->historyBuffer));
                    pParams->historyLen = LZSMIN(pParams->historyLen + 1u, LZS_MAX_HISTORY_SIZE);
                }
                break;

            case DECOMPRESS_GET_EXTENDED_LENGTH:
                // Extended length token
                // Get 4 bits
                pParams->length = (uint8_t) (pParams->bitFieldQueue >> (BIT_QUEUE_BITS - LENGTH_MAX_BIT_WIDTH));
                pParams->bitFieldQueue <<= LENGTH_MAX_BIT_WIDTH;
                pParams->bitFieldQueueLen -= LENGTH_MAX_BIT_WIDTH;
                LZS_DEBUG(("Extended length %"PRIuFAST8"\n", pParams->length));
                if (pParams->length == MAX_EXTENDED_LENGTH)
                {
                    // We stay in extended length decode mode
                    pParams->state = DECOMPRESS_COPY_EXTENDED_DATA;
                }
                else
                {
                    // We're finished with extended length decode mode; go back to normal
                    pParams->state = DECOMPRESS_COPY_DATA;
                }
                break;

            default:
                // It is an error if we ever get here.
                LZS_ASSERT(0);
                // Reset state, although following output will probably be rubbish.
                pParams->state = DECOMPRESS_GET_TOKEN_TYPE;
                pParams->status |= LZS_D_STATUS_ERROR;
                break;
        }
    }

    return outCount;
}
