/*****************************************************************************
 *
 * \file
 *
 * \brief Common declarations for LZS Compression and Decompression
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

#ifndef __LZS_COMMON_H
#define __LZS_COMMON_H

/*****************************************************************************
 * Implementation Defines
 ****************************************************************************/

#define SHORT_OFFSET_BITS           7u
#define LONG_OFFSET_BITS            11u
#define EXTENDED_LENGTH_BITS        4u
#define BIT_QUEUE_BITS              32u

#define SHORT_OFFSET_MAX            ((1u << SHORT_OFFSET_BITS) - 1u)
#define LONG_OFFSET_MAX             ((1u << LONG_OFFSET_BITS) - 1u)

#if (LZS_MAX_HISTORY_SIZE < ((1u << LONG_OFFSET_BITS) - 1u))
#error LZS_MAX_HISTORY_SIZE is too small
#endif

#define LENGTH_MAX_BIT_WIDTH        4u
#define MIN_LENGTH                  2u
#define MAX_SHORT_LENGTH            8u
#define MAX_EXTENDED_LENGTH         ((1u << EXTENDED_LENGTH_BITS) - 1u)

#define LZSMIN(X,Y)                 (((X) < (Y)) ? (X) : (Y))


/*****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline uint_fast16_t lzs_idx_inc_wrap(uint_fast16_t idx, uint_fast16_t inc, uint_fast16_t array_size)
{
    uint_fast16_t new_idx;

    new_idx = idx + inc;
    if (new_idx >= array_size)
    {
        new_idx -= array_size;
    }
    return new_idx;
}

static inline uint_fast16_t lzs_idx_dec_wrap(uint_fast16_t idx, uint_fast16_t dec, uint_fast16_t array_size)
{
    // This relies on calculation overflows wrapping as expected --
    // true as long as ints are unsigned.
    if (idx < dec)
    {
        idx += array_size;
    }
    return idx - dec;
}

static inline uint_fast16_t lzs_idx_delta_wrap(uint_fast16_t idx1, uint_fast16_t idx2, uint_fast16_t array_size)
{
    // This relies on calculation overflows wrapping as expected --
    // true as long as ints are unsigned.
    if (idx1 < idx2)
    {
        idx1 += array_size;
    }
    return idx1 - idx2;
}

// Like the above, except that if idx1 == idx2, return value is array_size not zero.
static inline uint_fast16_t lzs_idx_delta2_wrap(uint_fast16_t idx1, uint_fast16_t idx2, uint_fast16_t array_size)
{
    // This relies on calculation overflows wrapping as expected --
    // true as long as ints are unsigned.
    if (idx1 <= idx2)
    {
        idx1 += array_size;
    }
    return idx1 - idx2;
}


#endif // !defined(__LZS_COMMON_H)
