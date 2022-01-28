////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                     Copyright (c) 2016 David Bryant                    //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzw-lib.h"

/* This library implements the LZW general-purpose data compression algorithm.
 * The algorithm was originally described as a hardware implementation by
 * Terry Welsh here:
 *
 *   Welch, T.A. “A Technique for High-Performance Data Compression.”
 *   IEEE Computer 17,6 (June 1984), pp. 8-19.
 *
 * Since then there have been enumerable refinements and variations on the
 * basic technique, and this implementation is no different. The target of
 * the present implementation is embedded systems, and so emphasis was placed
 * on simplicity, fast execution, and minimal RAM usage.
 *
 * The symbols are stored in adjusted binary, which provides considerably
 * better compression performance with virtually no speed penalty compared to
 * the fixed sizes normally used. To ensure good performance on data with
 * varying characteristics (like executable images) the encoder resets as
 * soon as the dictionary is full. Also, worst-case performance is limited
 * to about 8% inflation by catching poor performance and forcing an early
 * reset before longer symbols are sent.
 *
 * The maximum symbol size is configurable on the encode side (from 9 bits
 * to 12 bits) and determines the RAM footprint required by both sides and,
 * to a large extent, the compression performance. This information is
 * communicated to the decoder in the first stream byte so that it can
 * allocate accordingly. The RAM requirements are as follows:
 *
 *    maximum    encoder RAM   decoder RAM
 *  symbol size  requirement   requirement
 * -----------------------------------------
 *     9-bit     1792 bytes    1024 bytes
 *    10-bit     4352 bytes    3072 bytes
 *    11-bit     9472 bytes    7168 bytes
 *    12-bit     19712 bytes   15360 bytes
 * 
 * This implementation uses malloc(), but obviously an embedded version could
 * use static arrays instead if desired (assuming that the maxbits was
 * controlled outside).
 */

#define NULL_CODE       -1      // indicates a NULL prefix
#define CLEAR_CODE      256     // code to flush dictionary and restart decoder
#define FIRST_STRING    257     // code of first dictionary string

/* This macro writes the adjusted-binary symbol "code" given the maximum
 * symbol "maxcode". A macro is used here just to avoid the duplication in
 * the lzw_compress() function. The idea is that if "maxcode" is not one
 * less than a power of two (which it rarely will be) then this code can
 * often send fewer bits that would be required with a fixed-sized code.
 *
 * For example, the first code we send will have a "maxcode" of 257, so
 * every "code" would normally consume 9 bits. But with adjusted binary we
 * can actually represent any code from 0 to 253 with just 8 bits -- only
 * the 4 codes from 254 to 257 take 9 bits.
 */

#define WRITE_CODE(code,maxcode) do {                           \
    int code_bits = (maxcode) < 1024 ?                          \
        ((maxcode) < 512 ? 8 : 9) :                             \
        ((maxcode) < 2048 ? 10 : 11);                           \
    int extras = (1 << (code_bits + 1)) - (maxcode) - 1;        \
    if ((code) < extras) {                                      \
        shifter |= ((long)(code) << bits);                      \
        bits += code_bits;                                      \
    }                                                           \
    else {                                                      \
        shifter |= ((long)(((code) + extras) >> 1) << bits);    \
        bits += code_bits;                                      \
        shifter |= ((long)(((code) + extras) & 1) << bits++);   \
    }                                                           \
    do { (*dst)(shifter); shifter >>= 8; output_bytes++;        \
    } while ((bits -= 8) >= 8);                                 \
} while (0)

/* LZW compression function. Bytes (8-bit) are read and written through callbacks and the
 * "maxbits" parameter specifies the maximum symbol size (9-12), which in turn determines
 * the RAM requirement and, to a large extent, the level of compression achievable. A return
 * value of EOF from the "src" callback terminates the compression process. A non-zero return
 * value indicates one of the two possible errors -- bad "maxbits" param or failed malloc().
 */

int lzw_compress (void (*dst)(int), int (*src)(void), int maxbits)
{
    int next = FIRST_STRING, prefix = NULL_CODE, bits = 0, total_codes, c;
    unsigned long input_bytes = 0, output_bytes = 0;
    short *first_references, *next_references;
    unsigned char *terminators;
    unsigned long shifter = 0;

    if (maxbits < 9 || maxbits > 12)    // check for valid "maxbits" setting
        return 1;

    // based on the "maxbits" parameter, compute total codes and allocate dictionary storage

    total_codes = 1 << maxbits;
    first_references = malloc (total_codes * sizeof (first_references [0]));
    next_references = malloc ((total_codes - 256) * sizeof (next_references [0]));
    terminators = malloc ((total_codes - 256) * sizeof (terminators [0]));

    if (!first_references || !next_references || !terminators)
        return 1;                       // failed malloc()

    // clear the dictionary

    memset (first_references, 0, total_codes * sizeof (first_references [0]));
    memset (next_references, 0, (total_codes - 256) * sizeof (next_references [0]));
    memset (terminators, 0, (total_codes - 256) * sizeof (terminators [0]));

    (*dst)(maxbits - 9);    // first byte in output stream indicates the maximum symbol bits

    // This is the main loop where we read input bytes and compress them. We always keep track of the
    // "prefix", which represents a pending byte (if < 256) or string entry (if >= FIRST_STRING) that
    // has not been sent to the decoder yet. The output symbols are kept in the "shifter" and "bits"
    // variables and are sent to the output every time 8 bits are available (done in the macro).

    while ((c = (*src)()) != EOF) {
        int cti;                            // coding table index

        input_bytes++;

        if (prefix == NULL_CODE) {          // this only happens the very first byte when we don't yet have a prefix
            prefix = c;
            continue;
        }

        if ((cti = first_references [prefix])) {    // if any longer strings are built on the current prefix...
            while (1)
                if (terminators [cti - 256] == c) { // we found a matching string, so we just update the prefix
                    prefix = cti;                   // to that string and continue without sending anything
                    break;
                }
                else if (!next_references [cti - 256]) {    // this string did not match the new character and
                    next_references [cti - 256] = next;     // there aren't any more, so we'll add a new string
                    cti = 0;                                // and point to it with "next_reference"
                    break;
                }
                else
                    cti = next_references [cti - 256];      // there are more possible matches to check, so loop back
        }
        else                                        // no longer strings are based on the current prefix, so now
            first_references [prefix] = next;       // the current prefix plus the new byte will be the next string

        // If "cti" is zero, we could not simply extend our "prefix" to a longer string because we did not find a
        // dictionary match, so we send the symbol representing the current "prefix" and add the new string to the
        // dictionary. Since the current byte "c" was not included in the prefix, that now becomes our new prefix.

        if (!cti) {
            WRITE_CODE (prefix, next);              // send symbol for current prefix (0 to next-1)
            terminators [next - 256] = c;           // newly created string has current byte as the terminator
            prefix = c;                             // current byte also becomes new prefix for next string

            // This is where we bump the next string index and decide whether to clear the dictionary and start over.
            // The triggers for that are either the dictionary is full or we've been outputting too many bytes and
            // decide to cut our losses before the symbols get any larger. Note that for the dictionary full case we
            // do NOT send the CLEAR_CODE because the decoder knows about this and we don't want to be redundant.

            if (++next == total_codes || output_bytes > 8 + input_bytes + (input_bytes >> 4)) {
                if (next < total_codes)
                    WRITE_CODE (CLEAR_CODE, next);

                // clear the dictionary and reset the byte counters -- basically everything starts over
                // except that we keep the last pending "prefix" (which, of course, was never sent)

                memset (first_references, 0, total_codes * sizeof (first_references [0]));
                memset (next_references, 0, (total_codes - 256) * sizeof (next_references [0]));
                memset (terminators, 0, (total_codes - 256) * sizeof (terminators [0]));
                input_bytes = output_bytes = 0;
                next = FIRST_STRING;
            }
        }
    }

    // we're done with input, so if we've received anything we still need to send that pesky pending prefix...

    if (prefix != NULL_CODE) {
        WRITE_CODE (prefix, next);

        if (++next == total_codes)  // watch for clearing to the first string to stay in step with the decoder!
            next = FIRST_STRING;    // (this was actually a corner-case bug that did not trigger often)
    }

    WRITE_CODE (next, next);        // the maximum possible code is always reserved for our END_CODE

    if (bits)                       // finally, flush any pending bits from the shifter
        (*dst)(shifter);

    free (terminators); free (next_references); free (first_references);
    return 0;
}

/* LZW decompression function. Bytes (8-bit) are read and written through callbacks. The
 * "maxbits" parameter is read as the first byte in the stream and controls how much memory
 * is allocated for decoding. A return value of EOF from the "src" callback terminates the
 * compression process (although this should not normally occur). A non-zero return value
 * indicates an error, which in this case can be a bad "maxbits" read from the stream, a
 * failed malloc(), or if an EOF is read from the input stream before the compression
 * terminates naturally with END_CODE.
 */

int lzw_decompress (void (*dst)(int), int (*src)(void))
{
    int read_byte, next = FIRST_STRING, prefix = CLEAR_CODE, bits = 0, total_codes;
    unsigned char *terminators, *reverse_buffer;
    unsigned long shifter = 0;
    short *prefixes;

    if ((read_byte = ((*src)())) == EOF || (read_byte & 0xfc))  //sanitize first byte
        return 1;

    // based on the "maxbits" parameter, compute total codes and allocate dictionary storage

    total_codes = 512 << (read_byte & 0x3);
    reverse_buffer = malloc ((total_codes - 256) * sizeof (reverse_buffer [0]));
    prefixes = malloc ((total_codes - 256) * sizeof (prefixes [0]));
    terminators = malloc ((total_codes - 256) * sizeof (terminators [0]));

    if (!reverse_buffer || !prefixes || !terminators)       // check for mallco() failure
        return 1;

    // This is the main loop where we read input symbols. The values range from 0 to the code value
    // of the "next" string in the dictionary (although the actual "next" code cannot be used yet,
    // and so we reserve that code for the END_CODE). Note that receiving an EOF from the input
    // stream is actually an error because we should have gotten the END_CODE first.

    while (1) {
        int code_bits = next < 1024 ? (next < 512 ? 8 : 9) : (next < 2048 ? 10 : 11), code;
        int extras = (1 << (code_bits + 1)) - next - 1;

        do {
            if ((read_byte = ((*src)())) == EOF) {
                free (terminators); free (prefixes); free (reverse_buffer);
                return 1;
            }

            shifter |= (long) read_byte << bits;
        } while ((bits += 8) < code_bits);

        // first we assume the code will fit in the minimum number of required bits

        code = (int) shifter & ((1 << code_bits) - 1);
        shifter >>= code_bits;
        bits -= code_bits;

        // but if code >= extras, then we need to read another bit to calculate the real code
        // (this is the "adjusted binary" part)

        if (code >= extras) {
            if (!bits) {
                if ((read_byte = ((*src)())) == EOF) {
                    free (terminators); free (prefixes); free (reverse_buffer);
                    return 1;
                }

                shifter = (long) read_byte;
                bits = 8;
            }

            code = (code << 1) - extras + (shifter & 1);
            shifter >>= 1;
            bits--;
        }

        if (code == next)                   // sending the maximum code is reserved for the end of the file
            break;
        else if (code == CLEAR_CODE)        // otherwise check for a CLEAR_CODE to start over early
            next = FIRST_STRING;
        else if (prefix == CLEAR_CODE) {    // this only happens at the first symbol which is always sent
            (*dst)(code);                   // literally and becomes our initial prefix
            next++;
        }
        // Otherwise we have a valid prefix so we step through the string from end to beginning storing the
        // bytes in the "reverse_buffer", and then we send them out in the proper order. One corner-case
        // we have to handle here is that the string might be the same one that is actually being defined
        // now (code == next-1). Also, the first 256 entries of "terminators" and "prefixes" are fixed and
        // not allocated, so that messes things up a bit.
        else {
            int cti = (code == next-1) ? prefix : code;
            unsigned char *rbp = reverse_buffer, c;

            do *rbp++ = cti < 256 ? cti : terminators [cti - 256];      // step backward through string...
            while ((cti = (cti < 256) ? NULL_CODE : prefixes [cti - 256]) != NULL_CODE);

            c = *--rbp;     // the first byte in this string is the terminator for the last string, which is
                            // the one that we'll create a new dictionary entry for this time

            do (*dst)(*rbp);                        // send string in corrected order (except for the terminator
            while (rbp-- != reverse_buffer);        // which we don't know yet)

            if (code == next-1)
                (*dst)(c);

            prefixes [next - 1 - 256] = prefix;     // now update the next dictionary entry with the new string
            terminators [next - 1 - 256] = c;       // (but we're always one behind, so it's not the string just sent)

            if (++next == total_codes)              // check for full dictionary, which forces a reset (and, BTW,
                next = FIRST_STRING;                // means we'll never use the dictionary entry we just wrote)
        }

        prefix = code;      // the code we just received becomes the prefix for the next dictionary string entry
                            // (which we'll create once we find out the terminator)
    }

    free (terminators); free (prefixes); free (reverse_buffer);
    return 0;
}
