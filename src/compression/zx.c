// modified by Luigi Auriemma
/*
 * ZX0 decompressor - by Einar Saukas
 * https://github.com/einar-saukas/ZX0
 */
/*
 * ZX1 decompressor - by Einar Saukas
 * https://github.com/einar-saukas/ZX1
 */
/*
 * ZX2 decompressor - by Einar Saukas
 * https://github.com/einar-saukas/ZX2
 */
/*
 * ZX5 decompressor - by Einar Saukas
 * https://github.com/einar-saukas/ZX5
 */

/*
The ZX0 data compression format and algorithm was designed and implemented by Einar Saukas. Special thanks to introspec/spke for several suggestions and improvements, and together with uniabis for providing the "Fast" decompressor. Also special thanks to Urusergi for additional ideas and improvements.

The optimal C compressor is available under the "BSD-3" license. In practice, this is relevant only if you want to modify its source code and/or incorporate the compressor within your own products. Otherwise, if you just execute it to compress files, you can simply ignore these conditions.

The decompressors can be used freely within your own programs (either for the ZX Spectrum or any other platform), even for commercial releases. The only condition is that you must indicate somehow in your documentation that you have used ZX0.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_OFFSET 1

#define FALSE 0
#define TRUE 1

static unsigned char *input_data;
static unsigned char *output_data;
static size_t input_index;
static size_t output_index;
static size_t input_size;
static int bit_mask;
static int bit_value;
static int backtrack;
static int last_byte;
static int ahead_bit;

static int read_byte() {
    if(input_index >= input_size) return -1;
    last_byte = input_data[input_index++];
    return last_byte;
}

static int read_bit() {
    if (backtrack) {
        backtrack = FALSE;
        return last_byte & 1;
    }
    bit_mask >>= 1;
    if (bit_mask == 0) {
        bit_mask = 128;
        bit_value = read_byte();
    }
    return bit_value & bit_mask ? 1 : 0;
}

static int read_interlaced_elias_gamma(int inverted) {
    int value = 1;
    while (!read_bit()) {
        value = (value << 1) | (read_bit() ^ inverted);
    }
    return value;
}

static void write_byte(int value) {
    output_data[output_index++] = value;
}

static void write_bytes(int offset, int length) {
    int i;

    if (offset > output_index) {
        fprintf(stderr, "Error: Invalid data in input file %s\n", ""/*input_name*/);
        exit(1);
    }
    while (length-- > 0) {
        i = output_index-offset;
        write_byte(output_data[i]); //i >= 0 ? i : BUFFER_SIZE+i]);
    }
}

static void decompress0(int classic_mode) {
    int last_offset = INITIAL_OFFSET;
    int length;
    int i;

COPY_LITERALS:
    length = read_interlaced_elias_gamma(FALSE);
    for (i = 0; i < length; i++)
        write_byte(read_byte());
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;

/*COPY_FROM_LAST_OFFSET:*/
    length = read_interlaced_elias_gamma(FALSE);
    write_bytes(last_offset, length);
    if (!read_bit())
        goto COPY_LITERALS;

COPY_FROM_NEW_OFFSET:
    last_offset = read_interlaced_elias_gamma(!classic_mode);
    if (last_offset == 256) {
        return;
    }
    last_offset = last_offset*128-(read_byte()>>1);
    backtrack = TRUE;
    length = read_interlaced_elias_gamma(FALSE)+1;
    write_bytes(last_offset, length);
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;
    else
        goto COPY_LITERALS;
}

static void decompress1() {
    int last_offset = INITIAL_OFFSET;
    int length;
    int i;

COPY_LITERALS:
    length = read_interlaced_elias_gamma(FALSE);
    for (i = 0; i < length; i++)
        write_byte(read_byte());
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;

/*COPY_FROM_LAST_OFFSET:*/
    length = read_interlaced_elias_gamma(FALSE);
    write_bytes(last_offset, length);
    if (!read_bit())
        goto COPY_LITERALS;

COPY_FROM_NEW_OFFSET:
    last_offset = read_byte();
    if (last_offset & 1) {
        i = read_byte();
        last_offset = 32512-(i&254)*128-(last_offset&254)-(i&1);
    } else {
        last_offset = 128-last_offset/2;
    }
    if (last_offset <= 0) {

        return;
    }
    length = read_interlaced_elias_gamma(FALSE)+1;
    write_bytes(last_offset, length);
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;
    else
        goto COPY_LITERALS;
}

static void decompress2(/*int last_offset, int min_length, int limited_length*/) {
    int last_offset = INITIAL_OFFSET;
    int min_length = 2;

    int length;
    int i;

COPY_LITERALS:
    length = read_interlaced_elias_gamma(FALSE);
    for (i = 0; i < length; i++)
        write_byte(read_byte());
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;

/*COPY_FROM_LAST_OFFSET:*/
    length = read_interlaced_elias_gamma(FALSE);
    write_bytes(last_offset, length);
    if (!read_bit())
        goto COPY_LITERALS;

COPY_FROM_NEW_OFFSET:
    last_offset = 255-read_byte();                   
    if (!last_offset) {
        return;
    }
    length = read_interlaced_elias_gamma(FALSE)+min_length-1;
    write_bytes(last_offset, length);
    if (read_bit())
        goto COPY_FROM_NEW_OFFSET;
    else
        goto COPY_LITERALS;
}

static void decompress5(int classic_mode) {
    int last_offset1 = INITIAL_OFFSET;
    int last_offset2 = 0;
    int last_offset3 = 0;
    int length;
    int i;

COPY_LITERALS:
    length = read_interlaced_elias_gamma(FALSE);
    for (i = 0; i < length; i++)
        write_byte(read_byte());
    if (read_bit())
        goto COPY_FROM_OTHER_OFFSET;

/*COPY_FROM_LAST_OFFSET:*/
    length = read_interlaced_elias_gamma(FALSE);
    write_bytes(last_offset1, length);
    if (!read_bit())
        goto COPY_LITERALS;

COPY_FROM_OTHER_OFFSET:
    if (!read_bit()) {

/*COPY_FROM_PREVIOUS_OFFSET:*/
        if (!read_bit()) {
            i = last_offset2;
            last_offset2 = last_offset1;
            last_offset1 = i;
        } else {
            i = last_offset3;
            last_offset3 = last_offset2;
            last_offset2 = last_offset1;
            last_offset1 = i;
        }
        length = read_interlaced_elias_gamma(FALSE);
        write_bytes(last_offset1, length);
    } else {

/*COPY_FROM_NEW_OFFSET:*/
        ahead_bit = read_bit();
        last_offset3 = last_offset2;
        last_offset2 = last_offset1;
        last_offset1 = read_interlaced_elias_gamma(!classic_mode);
        if (last_offset1 == 256) {
            return;
        }
        last_offset1 = last_offset1*256-read_byte();
        backtrack = TRUE;
        length = read_interlaced_elias_gamma(FALSE)+1;
        write_bytes(last_offset1, length);
    }
    if (read_bit())
        goto COPY_FROM_OTHER_OFFSET;
    else
        goto COPY_LITERALS;
}



int dzx_decompress(int ver, unsigned char *in, int insz, unsigned char *out) {
    int classic_mode = 0;

    input_index = 0;
    output_index = 0;
    bit_mask = 0;
    backtrack = FALSE;

    input_data  = in;
    input_size  = insz;
    output_data = out;

    if(ver == 1) {
        decompress1(classic_mode);
    } else if(ver == 2) {
        decompress2(classic_mode);
    } else if(ver == 5) {
        decompress5(classic_mode);
    } else {
        decompress0(classic_mode);
    }
    return output_index;
}
