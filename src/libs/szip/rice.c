/*==============================================================================
The SZIP Science Data Lossless Compression Program is Copyright (C) 2001 Science
& Technology Corporation @ UNM.  All rights released.  Copyright (C) 2003 Lowell
H. Miles and Jack A. Venbrux.  Licensed to ICs Corp. for distribution by the
University of Illinois' National Center for Supercomputing Applications as a
part of the HDF data storage and retrieval file format and software library
products package.  All rights reserved.  Do not modify or use for other
purposes.

SZIP implements an extended Rice adaptive lossless compression algorithm
for sample data.  The primary algorithm was developed by R. F. Rice at
Jet Propulsion Laboratory.

SZIP embodies certain inventions patented by the National Aeronautics &
Space Administration.  United States Patent Nos. 5,448,642, 5,687,255,
and 5,822,457 have been licensed to ICs Corp. for distribution with the
HDF data storage and retrieval file format and software library products.
All rights reserved.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP decompression software
routines and underlying patents is hereby granted by ICs Corp. to all users
of and in conjunction with HDF data storage and retrieval file format and
software library products.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP compression software
routines and underlying patents for non-commercial, scientific use only
is hereby granted by ICs Corp. to users of and in conjunction with HDF
data storage and retrieval file format and software library products.

For commercial use license to SZIP compression software routines and underlying
patents please contact ICs Corp. at ICs Corp., 721 Lochsa Street, Suite 8,
Post Falls, ID 83854.  (208) 262-2008.

==============================================================================*/
/*==============================================================================
To compile szip on the following operating systems do:

UNIX:      cc -o szip rice.c
MSDOS:     cc -o szip -DMSDOS rice.c
WINDOWS95: cc -o szip -DWINDOWS95 rice.c
WINDOSNT:  cc -o szip -DWINDOWS95 rice.c

If you compiler is not named "cc", then replace "cc" above with the name of
your compiler.

The szip program will run much faster when compiler optimization options are
used.
==============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#if defined(MSDOS) || defined(WINDOWS95)
#include <fcntl.h>
#endif
#if defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "rice.h"

#define HDF 1    /*** 1 if part of HDF package ***/

/* Uncomment the following line to compile out szip encoding */
/* #define REMOVE_SZIP_ENCODER */

#if HDF
#include "ricehdf.h"

#define compress_memory szip_compress_memory
#define uncompress_memory szip_uncompress_memory
#define output_buffer_full szip_output_buffer_full
#define check_params szip_check_params
#endif /* HDF */

#if !HDF
static char sccs_id[] = "@(#) szip: Combined Version: 1.5, Revised: 06/11/02 15:01:11";
#endif /* !HDF */

char copyright[] = "@(#) (C) Copyright 1993,1994,1996 University of New Mexico.  All Rights Reserved.";

#if !HDF
static char credits1[] = "This software uses an extended Rice algorithm and was developed at the";
static char credits2[] = "University of New Mexico.";
static char credits3[] = "Software emulates two common modes of the extended Rice Algorithm chip set.";
#endif /* !HDF */

static boolean allow_k13 = TRUE;
#if !HDF
static boolean encoding;
static boolean keep_compressed_file = FALSE;
static boolean keep_image_file = FALSE;
#endif /* !HDF */
static boolean msb_first = TRUE;
static boolean raw_mode = FALSE;

#if !HDF
static int verbose_mode;
static short msb_test = 0x0001;
#endif /* !HDF */

#ifndef REMOVE_SZIP_ENCODER
static int (*winner_function)();
static int (*winner_ref_function)();
#endif /* !defined(REMOVE_SZIP_ENCODER) */

static boolean compress_exactly_as_chip;

#if !HDF
static int input_mode;
static int output_mode;
#endif /* !HDF */

boolean output_buffer_full;

/*** variables for reading data ***/
static long input_byte_count;
#ifndef REMOVE_SZIP_ENCODER
static long input_pixel_count;
#endif /* !defined(REMOVE_SZIP_ENCODER) */
static unsigned char *input_byte_data;

static int bits_per_pixel;
static int blocks_per_scanline;
static int bytes_per_pixel;
static int compression_mode;
static int padded_pixels_per_scanline;
static int pixels_per_block;
static int pixels_per_scanline;

static int default_id;
#ifndef REMOVE_SZIP_ENCODER
static int id_bits;
static int masknot[25];
static int pixel_bit_count[MAX_PIXELS_PER_BLOCK+1];
#endif /* !defined(REMOVE_SZIP_ENCODER) */
static int xmax;

#ifndef REMOVE_SZIP_ENCODER
static long total_coded_bytes;
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if !HDF
static FILE *fp_in;
static FILE *fp_out;
#endif /* !HDF */

static long output_pixel_count;

#if !HDF
static char base_file_name[MAX_FILENAME_SIZE];
static char input_file_name[MAX_FILENAME_SIZE];
static char output_file_name[MAX_FILENAME_SIZE];

#ifndef REMOVE_SZIP_ENCODER
static long input_file_size;
#endif /* !defined(REMOVE_SZIP_ENCODER) */
#endif /* !HDF */

static char *bptr;
static char *bmid;
#ifndef REMOVE_SZIP_ENCODER
static char *global_bptr;
#endif /* !defined(REMOVE_SZIP_ENCODER) */
static char output_buffer[OUTPUT_BUFFER_SIZE];

#ifndef REMOVE_SZIP_ENCODER
static unsigned char ext2_array[MAX_EXT2+1][MAX_EXT2+1];
#endif /* !defined(REMOVE_SZIP_ENCODER) */
static unsigned char ext2_array1[MAX_EXT2_SUM+1];
static unsigned char ext2_array2[MAX_EXT2_SUM+1];

static int leading_zeros[256];

#if !HDF
static char *file_array[MAX_COMMAND_LINE_FILES];
static int file_count;

static char msg[256];
#endif /* !HDF */
static int error_count;
static int warning_count;

#ifndef REMOVE_SZIP_ENCODER
static unsigned long global_packed_value;
static unsigned long global_packed_bits = 32;
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if 0  /*** set to 1 to check packed bits ***/
#define check_value(x, n) \
    { \
    if (x > n) \
        internal_error(__FILE__, __LINE__); \
    }
#else
#define check_value(x, n)
#endif

#define pack1(value, pbits) \
    { \
    packed_bits -= pbits; \
    check_value(packed_bits, 32); \
    packed_value |= value << packed_bits; \
    if (packed_bits <= 16) \
        { \
        unsigned long v16; \
        v16 = packed_value >> 16; \
        *bptr++ = (char)(v16 >> 8); \
        *bptr++ = (char)(v16 & 0xff); \
        packed_value <<= 16; \
        packed_bits += 16; \
        } \
    }

#define pack2(xvalue, xbits) \
    { \
    unsigned long value; \
    long pbits; \
    \
    pbits = xbits; \
    value = xvalue; \
    if (pbits > 16) \
        { \
        unsigned long v16; \
        v16 = value >> 16; \
        pbits -= 16; \
        pack1(v16, pbits); \
        value &= 0xffff; \
        pbits = 16; \
        } \
    \
    pack1(value, pbits); \
    }

#define packfs(xbits) \
    { \
    long pbits; \
    \
    pbits = xbits; \
    while (pbits > 16) \
        { \
        pack1(0, 16); \
        pbits -= 16; \
        } \
    pack1(1, pbits); \
    }

static void
warning(const char *fmt, ...)
{
/* Disable warning output if compiled as part of the HDF library */
#if !HDF
        va_list ap;

        va_start(ap, fmt);
        fprintf(stderr, "WARNING: %s: ", input_file_name);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
#endif /* !HDF */
    warning_count++;
}

static void
error(const char *fmt, ...)
{
#if !HDF
        va_list ap;

        va_start(ap, fmt);
        fprintf(stderr, "ERROR: %s: ", input_file_name);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
#endif /* !HDF */
    error_count++;
}

#if !HDF
static void
binary_mode()
{
#if defined(MSDOS) || defined(WINDOWS95) || defined(WIN32)
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
        {
        error("Could not change standard input mode to binary.\n");
        exit(1);
        }

    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        {
        error("Could not change standard output mode to binary.\n");
        exit(1);
        }
#endif
}
#endif /* !HDF */

static int
getch()
{
    int ch;

#if !HDF
    if (input_mode == FILE_DATA)
        {
        ch = getc(fp_in);
        if (ch == EOF)
            {
            error("Premature eof while reading header.\n");
            return 0;
            }

        return ch;
        }
    else
#endif /* !HDF */
        {
        ch = *input_byte_data++;
        if (input_byte_count == 0)
            {
            error("Premature end of memory while reading header.\n");
            return 0;
            }

        input_byte_count--;
        return ch;
        }
}

static void
read_header()
{
    int mode;
    int mult;
    long scanline_count;
    unsigned long value;

    value = getch();
    value = (value << 8) | getch();
    if (value & 0x8000)
        {
        /*** 1bppnnnjjjssss-- ***/
        msb_first = (value >> 14) & 1;
        mode = (value >> 12) & 3;
        bits_per_pixel = short_header.bits_per_pixel[(value >> 9) & 7];
        pixels_per_block = short_header.pixels_per_block[(value >> 6) & 7];
        mult = short_header.pixels_per_block_mult[(value >> 2) & 15];
        pixels_per_scanline = pixels_per_block * mult;
        }
    else if (value & 0x4000)
        {
        /*** 01bpppnnnnjjjjjssssssssssssss--- ***/
        value = (value << 8) | getch();
        value = (value << 8) | getch();
        msb_first = (value >> 29) & 1;
        mode = (value >> 26) & 7;
        bits_per_pixel = ((value >> 22) & 0xf) + 1;
        pixels_per_block = ((value >> 17) & 0x1f) * 2 + 2;
        pixels_per_scanline = ((value >> 3) & 0x3fff) + 1;
        }
    else if (value & 0x2000)
        {
        /*** 001bpppnnnnnnjjjjjssssssssssssss ***/
        value = (value << 8) | getch();
        value = (value << 8) | getch();
        msb_first = (value >> 28) & 1;
        mode = (value >> 25) & 7;
        bits_per_pixel = ((value >> 19) & 0x3f) + 1;
        pixels_per_block = ((value >> 14) & 0x1f) * 2 + 2;
        pixels_per_scanline = (value & 0x3fff) + 1;
        }
    else
        {
        error("Header format error - sz file has been corrupted.\n");
        return;
        }

    value = getch();
    if (value == 0)
        output_pixel_count = 0x7fffffff;
    else if (value & 0x80)
        {
        /*** 1vvvvvvv ***/
        scanline_count = short_header.scanlines_per_file[value & 0x7f];
        output_pixel_count = scanline_count * pixels_per_scanline;
        }
    else if (value & 0x40)
        {
        /*** 01vvvvvvvvvvvvvvvvvvvvvvvvvvvvv ***/
        value = (value << 8) | getch();
        value = (value << 8) | getch();
        value = (value << 8) | getch();

        output_pixel_count = value & 0x3fffffff;
        }
    else
        {
        error("Unknown file size format in input file.\n");
        return;
        }

    if (mode == 0 || mode == 1)
        compression_mode = mode ? NN_MODE : EC_MODE;
    else
        {
        error("This decoder program does not support the encoded mode.\n");
        return;
        }

#if !HDF
    if (input_mode == FILE_DATA && ferror(fp_in))
        {
        error("While reading header.\n");
        return;
        }
#endif /* !HDF */
}

#ifndef REMOVE_SZIP_ENCODER
static void
write_header()
{
    int i;
    int j;
    int m;
    int mult;
    int n;
    int scanline_count;
    unsigned long value;

    for (j = 0; j < 8; j++)
        if (pixels_per_block == short_header.pixels_per_block[j])
            break;

    for (n = 0; n < 8; n++)
        if (bits_per_pixel == short_header.bits_per_pixel[n])
            break;

    mult = pixels_per_scanline/pixels_per_block;
    if (mult * pixels_per_block == pixels_per_scanline)
        {
        for (m = 0; m < 16; m++)
            if (mult == short_header.pixels_per_block_mult[m])
                break;
        }
    else
        m = 16;

    if (j < 8 && m < 16 && n < 8)
        {
        /*** 1bppnnnjjjssss-- ***/
        value = 1 << 15;
        value |= (msb_first == 1) << 14;
        value |= (compression_mode == NN_MODE) << 12;
        value |= n << 9;
        value |= j << 6;
        value |= m << 2;
        *global_bptr++ = (char)(value >> 8);
        *global_bptr++ = (char)(value);
        }
    else if (bits_per_pixel <= 16)
        {
        /*** 01bpppnnnnjjjjjssssssssssssss--- ***/
        value = 1 << 30;
        value |= (msb_first == 1) << 29;
        value |= (compression_mode == NN_MODE) << 26;
        value |= (bits_per_pixel - 1) << 22;
        value |= (pixels_per_block/2 - 1) << 17;
        value |= (pixels_per_scanline - 1) << 3;

        *global_bptr++ = (char)(value >> 24);
        *global_bptr++ = (char)(value >> 16);
        *global_bptr++ = (char)(value >>  8);
        *global_bptr++ = (char)(value);
        }
    else
        {
        /*** 001bpppnnnnnnjjjjjssssssssssssss ***/
        value = 1 << 29;
        value |= (msb_first == 1) << 28;
        value |= (compression_mode == NN_MODE) << 25;
        value |= (bits_per_pixel - 1) << 19;
        value |= (pixels_per_block/2 - 1) << 14;
        value |= (pixels_per_scanline - 1);

        *global_bptr++ = (char)(value >> 24);
        *global_bptr++ = (char)(value >> 16);
        *global_bptr++ = (char)(value >>  8);
        *global_bptr++ = (char)(value);
        }

#if !HDF
    if (fp_in == stdin)
        *global_bptr++ = 0;
    else
#endif /* !HDF */
        {
        scanline_count = input_pixel_count / pixels_per_scanline;
        for (i = 0; i < 128; i++)
            if (scanline_count == short_header.scanlines_per_file[i])
                break;

        if (i != 128 && input_pixel_count % pixels_per_scanline == 0)
            {
            /*** 1vvvvvvv ***/
            value = 1 << 7;
            value |= i;
            *global_bptr++ = (char)(value);
            }
        else if (input_pixel_count <= 0x3fffffff)
            {
            /*** 01vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv ***/
            value = 1 << 30;
            value |= input_pixel_count;
            *global_bptr++ = (char)(value >> 24);
            *global_bptr++ = (char)(value >> 16);
            *global_bptr++ = (char)(value >>  8);
            *global_bptr++ = (char)(value);
            }
        else
            {
            error("Maximum pixels per image exceeded: %d.\n", 0x3fffffff);
            return;
            }
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

/*------------------------------------------------------------------------
Fix the last scan line of input by filling the rest of the scanline pixels
with the last pixel (nn coding), or 0 (entropy coding).
---------------------------------------------------------------------------*/
#ifndef REMOVE_SZIP_ENCODER
static void
fix_last_scanline(byte_buffer, n)
unsigned char *byte_buffer;
int n;
{
    unsigned char *end;
    unsigned char *p;
    int bytes;
    int ch;
    int ch1;
    int ch2;
    int ch3;
    int ch4;

    warning("Premature end of last scanline.  Check -n, -j, or -s settings.\n");
    if (bytes_per_pixel == 1)
        {
        ch = compression_mode == NN_MODE ? byte_buffer[n-1] : 0;
        memset(byte_buffer + n, ch, pixels_per_scanline - n);
        }
    else if (bytes_per_pixel == 2)
        {
        bytes = 2*n;
        ch1 = compression_mode == NN_MODE ? byte_buffer[bytes-2] : 0;
        ch2 = compression_mode == NN_MODE ? byte_buffer[bytes-1] : 0;
        end = byte_buffer + 2*pixels_per_scanline;
        p = byte_buffer + bytes;
        while (p < end)
            {
            *p++ = ch1;
            *p++ = ch2;
            }
        }
    else
        {
        bytes = 4*n;
        ch1 = compression_mode == NN_MODE ? byte_buffer[bytes-4] : 0;
        ch2 = compression_mode == NN_MODE ? byte_buffer[bytes-3] : 0;
        ch3 = compression_mode == NN_MODE ? byte_buffer[bytes-2] : 0;
        ch4 = compression_mode == NN_MODE ? byte_buffer[bytes-1] : 0;
        end = byte_buffer + 4*pixels_per_scanline;
        p = byte_buffer + bytes;
        while (p < end)
            {
            *p++ = ch1;
            *p++ = ch2;
            *p++ = ch3;
            *p++ = ch4;
            }
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if !HDF
#ifndef REMOVE_SZIP_ENCODER
static void
internal_error(file_name, line_number)
char *file_name;
int line_number;
{
    fprintf(stderr, "INTERNAL ERROR: File: %s, Line: %d\n", file_name, line_number);
    exit(1);
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */
#endif /* !HDF */

#ifndef REMOVE_SZIP_ENCODER
static int
encode_scanline()
{
    char *bptr;
    int del;
    int fsval;
    int fsval2;
    int i;
    int id;
    int ksplits;
    int n;
    int newbits;
    int x;
    int xp;
    int x1;
    int x2;
    int x3;
    int x4;
    int zero_blocks;
    unsigned char *b;
    unsigned char byte_buffer[4*MAX_PIXELS_PER_SCANLINE];
    unsigned *end;
    unsigned mask;
    unsigned num;
    unsigned *s;
    unsigned *s1;
    unsigned sigma[MAX_PIXELS_PER_SCANLINE];
    unsigned long packed_value;
    unsigned long packed_bits;
    unsigned long value;

    bptr = global_bptr;
    packed_value = global_packed_value;
    packed_bits = global_packed_bits;

    mask = masknot[bits_per_pixel];
    if (bytes_per_pixel == 2)
        {
#if !HDF
        if (input_mode == FILE_DATA)
            {
            n = fread((void*) byte_buffer, (size_t) 1, (size_t) 2*pixels_per_scanline, fp_in);
            if (n & 1)
                {
                warning("One byte of last pixel missing; padding with 0.\n");
                byte_buffer[n++] = 0;
                }

            n >>= 1;
            }
        else
#endif /* !HDF */
            {
            n = input_pixel_count >= pixels_per_scanline ? pixels_per_scanline : input_pixel_count;
            input_pixel_count -= n;
            memcpy(byte_buffer, input_byte_data, 2*n);
            input_byte_data += 2*n;
            }

        if (n == 0)
            return 0;

        if (n < pixels_per_scanline)
            fix_last_scanline(byte_buffer, n);

        b = byte_buffer;
        s = sigma;
        end = sigma + pixels_per_scanline;
        if (msb_first)
            {
            if (compression_mode == NN_MODE)
                {
                xp = 0;
                if ((pixels_per_block & 7) == 0)
                    while (s < end)
                        {
                        x1 = ((*(b+0) << 8) + *(b+1)) & mask;
                        x2 = ((*(b+2) << 8) + *(b+3)) & mask;
                        x3 = ((*(b+4) << 8) + *(b+5)) & mask;
                        x4 = ((*(b+6) << 8) + *(b+7)) & mask;
                        b += 8;
                        del = x1-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x1;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x1;
                            }

                        del = x2-x1;
                        if (del >= 0)
                            {
                            if (del <= x1)
                                *s++ = del << 1;
                            else
                                *s++ = x2;
                            }
                        else
                            {
                            if (del >= (x1-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x2;
                            }

                        del = x3-x2;
                        if (del >= 0)
                            {
                            if (del <= x2)
                                *s++ = del << 1;
                            else
                                *s++ = x3;
                            }
                        else
                            {
                            if (del >= (x2-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x3;
                            }

                        del = x4-x3;
                        if (del >= 0)
                            {
                            if (del <= x3)
                                *s++ = del << 1;
                            else
                                *s++ = x4;
                            }
                        else
                            {
                            if (del >= (x3-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x4;
                            }

                        xp = x4;
                        }
                else
                    while (s < end)
                        {
                        x = ((*b << 8) + *(b+1)) & mask;
                        b += 2;
                        del = x-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x;
                            }

                        xp = x;
                        }
                }
            else
                {
                while (s < end)
                    {
                    *s++ = ((*b << 8) + *(b+1)) & mask;
                    b += 2;
                    }
                }
            }
        else
            {
            if (compression_mode == NN_MODE)
                {
                xp = 0;
                if ((pixels_per_block & 7) == 0)
                    while (s < end)
                        {
                        x1 = ((*(b+1) << 8) + *(b+0)) & mask;
                        x2 = ((*(b+3) << 8) + *(b+2)) & mask;
                        x3 = ((*(b+5) << 8) + *(b+4)) & mask;
                        x4 = ((*(b+7) << 8) + *(b+6)) & mask;
                        b += 8;
                        del = x1-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x1;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x1;
                            }

                        del = x2-x1;
                        if (del >= 0)
                            {
                            if (del <= x1)
                                *s++ = del << 1;
                            else
                                *s++ = x2;
                            }
                        else
                            {
                            if (del >= (x1-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x2;
                            }

                        del = x3-x2;
                        if (del >= 0)
                            {
                            if (del <= x2)
                                *s++ = del << 1;
                            else
                                *s++ = x3;
                            }
                        else
                            {
                            if (del >= (x2-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x3;
                            }

                        del = x4-x3;
                        if (del >= 0)
                            {
                            if (del <= x3)
                                *s++ = del << 1;
                            else
                                *s++ = x4;
                            }
                        else
                            {
                            if (del >= (x3-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x4;
                            }

                        xp = x4;
                        }
                else
                    while (s < end)
                        {
                        x = ((*(b+1) << 8) + *b) & mask;
                        b += 2;
                        del = x-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x;
                            }

                        xp = x;
                        }
                }
            else
                {
                while (s < end)
                    {
                    *s++ =((*(b+1) << 8) + *b) & mask;
                    b += 2;
                    }
                }
            }


        if (pixels_per_scanline < padded_pixels_per_scanline)
            {
            /*** Pad scanline with last pixel value ***/
            s = sigma + pixels_per_scanline;
            end = sigma + padded_pixels_per_scanline;
            while (s < end)
                *s++ = 0;
            }

        n *= 2;
        }
    else if (bytes_per_pixel == 1)
        {
#if !HDF
        if (input_mode == FILE_DATA)
            n = fread((void*) byte_buffer, (size_t) 1, (size_t) pixels_per_scanline, fp_in);
        else
#endif /* !HDF */
            {
            n = input_pixel_count >= pixels_per_scanline ? pixels_per_scanline : input_pixel_count;
            input_pixel_count -= n;
            memcpy(byte_buffer, input_byte_data, n);
            input_byte_data += n;
            }

        if (n == 0)
            return 0;

        if (n < pixels_per_scanline)
            fix_last_scanline(byte_buffer, n);

        b = byte_buffer;
        s = sigma;
        end = sigma + pixels_per_scanline;
        if (compression_mode == NN_MODE)
            {
            xp = 0;
            if ((pixels_per_block & 7) == 0)
                while (s < end)
                    {
                    x1 = *b++ & mask;
                    x2 = *b++ & mask;
                    x3 = *b++ & mask;
                    x4 = *b++ & mask;
                    del = x1-xp;
                    if (del >= 0)
                        {
                        if (del <= xp)
                            *s++ = del << 1;
                        else
                            *s++ = x1;
                        }
                    else
                        {
                        if (del >= (xp-xmax))
                            *s++ = ((-del)<<1) - 1;
                        else
                            *s++ = xmax-x1;
                        }

                    del = x2-x1;
                    if (del >= 0)
                        {
                        if (del <= x1)
                            *s++ = del << 1;
                        else
                            *s++ = x2;
                        }
                    else
                        {
                        if (del >= (x1-xmax))
                            *s++ = ((-del)<<1) - 1;
                        else
                            *s++ = xmax-x2;
                        }

                    del = x3-x2;
                    if (del >= 0)
                        {
                        if (del <= x2)
                            *s++ = del << 1;
                        else
                            *s++ = x3;
                        }
                    else
                        {
                        if (del >= (x2-xmax))
                            *s++ = ((-del)<<1) - 1;
                        else
                            *s++ = xmax-x3;
                        }

                    del = x4-x3;
                    if (del >= 0)
                        {
                        if (del <= x3)
                            *s++ = del << 1;
                        else
                            *s++ = x4;
                        }
                    else
                        {
                        if (del >= (x3-xmax))
                            *s++ = ((-del)<<1) - 1;
                        else
                            *s++ = xmax-x4;
                        }

                    xp = x4;
                    }
            else
                while (s < end)
                    {
                    x = *b++ & mask;
                    del = x-xp;
                    if (del >= 0)
                        {
                        if (del <= xp)
                            *s++ = del << 1;
                        else
                            *s++ = x;
                        }
                    else
                        {
                        if (del >= (xp-xmax))
                            *s++ = ((-del)<<1) - 1;
                        else
                            *s++ = xmax-x;
                        }

                    xp = x;
                    }
            }
        else
            {
            while (s < end)
                *s++ = *b++ & mask;
            }

        if (pixels_per_scanline < padded_pixels_per_scanline)
            {
            /*** Pad scanline with last pixel value ***/
            s = sigma + pixels_per_scanline;
            end = sigma + padded_pixels_per_scanline;
            while (s < end)
                *s++ = 0;
            }
        }
    else
        {
#if !HDF
        if (input_mode == FILE_DATA)
            {
            n = fread((void*) byte_buffer, (size_t) 1, (size_t) 4*pixels_per_scanline, fp_in);
            if (n & 3)
                {
                warning("Missing byte(s) of last pixel missing; padding with 0.\n");
                byte_buffer[n++] = 0;
                }

            n >>= 2;
            }
        else
#endif /* !HDF */
            {
            n = input_pixel_count >= pixels_per_scanline ? pixels_per_scanline : input_pixel_count;
            input_pixel_count -= n;
            memcpy(byte_buffer, input_byte_data, 4*n);
            input_byte_data += 4*n;
            }

        if (n == 0)
            return 0;

        if (n < pixels_per_scanline)
            fix_last_scanline(byte_buffer, n);

        b = byte_buffer;
        s = sigma;
        end = sigma + pixels_per_scanline;
        if (msb_first)
            {
            if (compression_mode == NN_MODE)
                {
                xp = 0;
                if ((pixels_per_block & 7) == 0)
                    while (s < end)
                        {
                        x1 = ((*(b+1)  << 16) | (*(b+2)  << 8) | *(b+3))  & mask;
                        x2 = ((*(b+5)  << 16) | (*(b+6)  << 8) | *(b+7))  & mask;
                        x3 = ((*(b+9)  << 16) | (*(b+10) << 8) | *(b+11)) & mask;
                        x4 = ((*(b+13) << 16) | (*(b+14) << 8) | *(b+15)) & mask;
                        b += 16;
                        del = x1-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x1;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x1;
                            }

                        del = x2-x1;
                        if (del >= 0)
                            {
                            if (del <= x1)
                                *s++ = del << 1;
                            else
                                *s++ = x2;
                            }
                        else
                            {
                            if (del >= (x1-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x2;
                            }

                        del = x3-x2;
                        if (del >= 0)
                            {
                            if (del <= x2)
                                *s++ = del << 1;
                            else
                                *s++ = x3;
                            }
                        else
                            {
                            if (del >= (x2-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x3;
                            }

                        del = x4-x3;
                        if (del >= 0)
                            {
                            if (del <= x3)
                                *s++ = del << 1;
                            else
                                *s++ = x4;
                            }
                        else
                            {
                            if (del >= (x3-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x4;
                            }

                        xp = x4;
                        }
                else
                    while (s < end)
                        {
                        x = ((*(b+1) << 16) | (*(b+2) << 8) | *(b+3)) & mask;
                        b += 4;
                        del = x-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x;
                            }

                        xp = x;
                        }
                }
            else
                {
                while (s < end)
                    {
                    *s++ = ((*(b+1) << 16) | (*(b+2) << 8) | *(b+3)) & mask;
                    b += 4;
                    }
                }
            }
        else
            {
            if (compression_mode == NN_MODE)
                {
                xp = 0;
                if ((pixels_per_block & 7) == 0)
                    while (s < end)
                        {
                        x1 = ((*(b+2)  << 16) | (*(b+1)  << 8) | *(b+0))  & mask;
                        x2 = ((*(b+6)  << 16) | (*(b+5)  << 8) | *(b+4))  & mask;
                        x3 = ((*(b+10) << 16) | (*(b+9)  << 8) | *(b+8))  & mask;
                        x4 = ((*(b+14) << 16) | (*(b+13) << 8) | *(b+12)) & mask;
                        b += 16;
                        del = x1-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x1;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x1;
                            }

                        del = x2-x1;
                        if (del >= 0)
                            {
                            if (del <= x1)
                                *s++ = del << 1;
                            else
                                *s++ = x2;
                            }
                        else
                            {
                            if (del >= (x1-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x2;
                            }

                        del = x3-x2;
                        if (del >= 0)
                            {
                            if (del <= x2)
                                *s++ = del << 1;
                            else
                                *s++ = x3;
                            }
                        else
                            {
                            if (del >= (x2-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x3;
                            }

                        del = x4-x3;
                        if (del >= 0)
                            {
                            if (del <= x3)
                                *s++ = del << 1;
                            else
                                *s++ = x4;
                            }
                        else
                            {
                            if (del >= (x3-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x4;
                            }

                        xp = x4;
                        }
                else
                    while (s < end)
                        {
                        x = ((*(b+2) << 16) | (*(b+1) << 8) | *b) & mask;
                        b += 4;
                        del = x-xp;
                        if (del >= 0)
                            {
                            if (del <= xp)
                                *s++ = del << 1;
                            else
                                *s++ = x;
                            }
                        else
                            {
                            if (del >= (xp-xmax))
                                *s++ = ((-del)<<1) - 1;
                            else
                                *s++ = xmax-x;
                            }

                        xp = x;
                        }
                }
            else
                {
                while (s < end)
                    {
                    *s++ =((*(b+2) << 16) | (*(b+1) << 8) | *b) & mask;
                    b += 4;
                    }
                }
            }


        if (pixels_per_scanline < padded_pixels_per_scanline)
            {
            /*** Pad scanline with last pixel value ***/
            s = sigma + pixels_per_scanline;
            end = sigma + padded_pixels_per_scanline;
            while (s < end)
                *s++ = 0;
            }

        n *= 4;
        }

    end = sigma + pixels_per_block;
    s = sigma;
    zero_blocks = 0;
    if (compression_mode == NN_MODE)
        {
        /*** Next encode the first block of the scanline ***/
        /*** which contains the reference value ***/
        s++;
        id = (*winner_ref_function)(s, end);
        if (id == ID_ZERO)
            {
            pack1(0, id_bits+1);
            pack2(*sigma, bits_per_pixel);
            zero_blocks = 1;
            if (blocks_per_scanline == 1)
                {
                pack1(1, 1);
                zero_blocks = 0;
                }

            s = end;
            }
        else
            {
            if (id >= ID_K1)
                {
                if (id >= default_id || id >= bits_per_pixel)
                    id = default_id;

                pack1(id, id_bits);
                pack2(*sigma, bits_per_pixel);
                if (id == default_id)
                    {
                    while (s < end)
                        pack2(*s++, bits_per_pixel);
                    }
                else
                    {
                    ksplits = id-1;
                    s1 = s;
                    while (s1 < end)
                        {
                        fsval = (*s1++ >> ksplits) + 1;
                        packfs(fsval);
                        }

                    mask = masknot[ksplits];
                    while (s < end)
                        {
                        num = *s++ & mask;
                        pack2(num, ksplits);
                        }
                    }
                }
            else if (id == ID_FS)
                {
                pack1(id, id_bits);
                pack2(*sigma, bits_per_pixel);
                while (s < end)
                    {
                    fsval = *s++ + 1;
                    packfs(fsval);
                    }
                }
            else
                {
                pack1(1, id_bits+1);
                pack2(*sigma, bits_per_pixel);
                fsval = ext2_array[0][*s++];
                packfs(fsval);
                while (s < end)
                    {
                    fsval = ext2_array[*s][*(s+1)];
                    packfs(fsval);
                    s += 2;
                    }
                }
            }

        i = 2;
        end += pixels_per_block;
        }
    else
        {
        i = 1;
        s = sigma;
        }

    /*** Now encode all blocks after the reference block (if any) in the scanline ***/
    for (; i <= blocks_per_scanline; i++)
        {
        id = (*winner_function)(s, end);
        if (id == ID_ZERO)
            {
            if (zero_blocks == 0)
                pack1(0, id_bits+1);

            zero_blocks++;
            if ((i & (MAX_ZERO_BLOCKS-1)) == 0 || i == blocks_per_scanline)
                {
                if (zero_blocks <= 4)
                    newbits = zero_blocks;
                else
                    newbits = 5;

                packfs(newbits);
                zero_blocks = 0;
                }

            s = end;
            }
        else
            {
            if (zero_blocks)
                {
                if (zero_blocks <= 4)
                    newbits = zero_blocks;
                else
                    newbits = zero_blocks + 1;

                packfs(newbits);
                zero_blocks = 0;
                }

            if (id >= ID_K1)
                {
                if (id >= default_id || id >= bits_per_pixel)
                    id = default_id;

                pack1(id, id_bits);
                if (id == default_id)
                    {
                    while (s < end)
                        pack2(*s++, bits_per_pixel);
                    }
                else
                    {
                    ksplits = id-1;
                    s1 = s;
                    switch (ksplits)
                        {
                        case 1:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 1) + 1;
                                fsval2 = (*s1++ >> 1) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            if ((pixels_per_block & 0x7) == 0)
                                {
                                while (s < end)
                                    {
                                    value = *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    pack1(value, 8);
                                    }
                                }
                            else
                                while (s < end)
                                    {
                                    value = *s++ & 1;
                                    value = (value << 1) | *s++ & 1;
                                    pack1(value, 2);
                                    }
                            break;

                        case 2:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 2) + 1;
                                fsval2 = (*s1++ >> 2) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            if ((pixels_per_block & 0x7) == 0)
                                {
                                while (s < end)
                                    {
                                    value = *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    pack1(value, 16);
                                    }
                                }
                            else
                                while (s < end)
                                    {
                                    value = *s++ & 3;
                                    value = (value << 2) | *s++ & 3;
                                    pack1(value, 4);
                                    }
                            break;

                        case 3:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 3) + 1;
                                fsval2 = (*s1++ >> 3) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            if ((pixels_per_block & 0x7) == 0)
                                {
                                while (s < end)
                                    {
                                    value = *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    pack1(value, 15);

                                    value = *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    pack1(value, 9);
                                    }
                                }
                            else
                                while (s < end)
                                    {
                                    value = *s++ & 7;
                                    value = (value << 3) | *s++ & 7;
                                    pack1(value, 6);
                                    }
                            break;

                        case 4:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 4) + 1;
                                fsval2 = (*s1++ >> 4) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            if ((pixels_per_block & 0x7) == 0)
                                {
                                while (s < end)
                                    {
                                    value = *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    pack1(value, 16);

                                    value = *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    pack1(value, 16);
                                    }
                                }
                            else
                                while (s < end)
                                    {
                                    value = *s++ & 0xf;
                                    value = (value << 4) | *s++ & 0xf;
                                    pack1(value, 8);
                                    }
                            break;

                        case 5:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 5) + 1;
                                fsval2 = (*s1++ >> 5) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            if ((pixels_per_block & 0x7) == 0)
                                {
                                while (s < end)
                                    {
                                    value = *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    pack1(value, 15);

                                    value = *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    pack1(value, 15);

                                    value = *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    pack1(value, 10);
                                    }
                                }
                            else
                                while (s < end)
                                    {
                                    value = *s++ & 0x1f;
                                    value = (value << 5) | *s++ & 0x1f;
                                    pack1(value, 10);
                                    }
                            break;

                        case 6:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 6) + 1;
                                fsval2 = (*s1++ >> 6) + 1;
                                if (fsval + fsval2 <= 16)
                                    {
                                    value = (1 << fsval2) | 1;
                                    pack1(value, fsval + fsval2);
                                    }
                                else
                                    {
                                    packfs(fsval);
                                    packfs(fsval2);
                                    }
                                }

                            while (s < end)
                                {
                                value = *s++ & 0x3f;
                                value = (value << 6) | *s++ & 0x3f;
                                pack1(value, 12);
                                }
                            break;

                        case 7:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 7) + 1;
                                fsval2 = (*s1++ >> 7) + 1;
                                packfs(fsval);
                                packfs(fsval2);
                                }

                            while (s < end)
                                {
                                value = *s++ & 0x7f;
                                value = (value << 7) | *s++ & 0x7f;
                                pack1(value, 14);
                                }
                            break;

                        case 8:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> 8) + 1;
                                fsval2 = (*s1++ >> 8) + 1;
                                packfs(fsval);
                                packfs(fsval2);
                                }

                            while (s < end)
                                {
                                value = *s++ & 0xff;
                                value = (value << 8) | *s++ & 0xff;
                                pack1(value, 16);
                                }
                            break;

                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 13:
                        case 14:
                        case 15:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> ksplits) + 1;
                                fsval2 = (*s1++ >> ksplits) + 1;
                                packfs(fsval);
                                packfs(fsval2);
                                }

                            mask = masknot[ksplits];
                            while (s < end)
                                {
                                num = *s++ & mask;
                                pack1(num, ksplits);
                                }
                            break;

                        default:
                            while (s1 < end)
                                {
                                fsval  = (*s1++ >> ksplits) + 1;
                                fsval2 = (*s1++ >> ksplits) + 1;
                                packfs(fsval);
                                packfs(fsval2);
                                }

                            mask = masknot[ksplits];
                            while (s < end)
                                {
                                num = *s++ & mask;
                                pack2(num, ksplits);
                                }
                            break;
                        }
                    }
                }
            else if (id == ID_FS)
                {
                pack1(id, id_bits);
                while (s < end)
                    {
                    fsval = *s++ + 1;
                    packfs(fsval);
                    }
                }
            else
                {
                packfs(id_bits+1);
                while (s < end)
                    {
                    fsval = ext2_array[*s][*(s+1)];
                    packfs(fsval);
                    s += 2;
                    }
                }
            }

        end += pixels_per_block;
        }

    global_bptr = bptr;
    global_packed_value = packed_value;
    global_packed_bits = packed_bits;
    return n;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if !HDF
#ifndef REMOVE_SZIP_ENCODER
static void
flush_encoded_buffer()
{
    if (output_mode == FILE_DATA)
        {
        total_coded_bytes += global_bptr - output_buffer;
        if (global_bptr > output_buffer)
            fwrite((void*) output_buffer, (size_t) (global_bptr-output_buffer), (size_t) 1, fp_out);

        global_bptr = output_buffer;
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */
#endif /* !HDF */

/*---------------------------------------------------------------------------
Flush remaining bits left over in the pack() function variables.
If possible Output one byte instead of two.
---------------------------------------------------------------------------*/
#ifndef REMOVE_SZIP_ENCODER
static void
flush_encoded_bits()
{
    unsigned v16;

    if (global_packed_bits < 32)
        {
        v16 = global_packed_value >> 16;
        *global_bptr++ = v16 >> 8;
        if (global_packed_bits < 24)
            *global_bptr++ = v16;
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

/*--------------------------------------------------------------------
Encode the image by reading in one scanline at a time; encode each scanline
block by calling code_block().  The function read_scanline() reads the
next scanline from a file or memory.
*--------------------------------------------------------------------*/
#ifndef REMOVE_SZIP_ENCODER
static long
rice_encode()
{
    int bytes_read;
    int pixels_read;
    int total_bytes_read;

    if (!raw_mode)
        write_header();

    total_bytes_read = 0;
    while (1)
        {
        bytes_read = encode_scanline();
        total_bytes_read += bytes_read;
        if (bytes_read == 0)
            break;

#if !HDF
        flush_encoded_buffer();
#endif /* !HDF */
        }

    flush_encoded_bits();
#if !HDF
    flush_encoded_buffer();
#endif /* !HDF */

    if (raw_mode
#if !HDF
             || fp_in == stdin
#endif /* !HDF */
             )
        {
        pixels_read = total_bytes_read;
        if (bits_per_pixel > 16)
            pixels_read >>= 2;
        else if (bits_per_pixel > 8)
            pixels_read >>= 1;

        if (pixels_read % pixels_per_scanline)
            {
            warning("More data will be decoded than was encoded.\n");
            }
        }

    if (total_bytes_read == 0)
        {
        warning("Input file is empty.\n");
        return 0;
        }

    return total_bytes_read;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
c_ext2(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int total;
    unsigned *s;

    s = sigma;
    if ((end - s) & 1)
        {
        if (*s > MAX_EXT2)
            return 9999;

        total = ext2_array[0][*s++];
        }
    else
        total = 0;

    while (s < end)
        {
        if (*s + *(s+1) > MAX_EXT2)
            return 9999;

        total += ext2_array[*s][*(s+1)];
        s += 2;
        }

    return total+1;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_winner8(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 8 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        {
        sum += *s++;
        sum += *s++;
        }

    /*** J = 8; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 4 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 12)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 8)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 28)
        return ID_K1;
    else if (sum <= 60)
        return ID_K2;
    else if (sum <= 124)
        return ID_K3;
    else if (sum <= 252)
        return ID_K4;
    else if (sum <= 508)
        return ID_K5;
    else if (sum <= 1020)
        return ID_K6;
    else if (sum <= 2044)
        return ID_K7;
    else if (sum <= 4092)
        return ID_K8;
    else if (sum <= 8188)
        return ID_K9;
    else if (sum <= 16380)
        return ID_K10;
    else if (sum <= 32764)
        return ID_K11;
    else if (sum <= 65532)
        return ID_K12;
    else if (sum <= 131068)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 262140)
        return ID_K14;
    else if (sum <= 524284)
        return ID_K15;
    else if (sum <= 1048572)
        return ID_K16;
    else if (sum <= 2097148)
        return ID_K17;
    else if (sum <= 4194300)
        return ID_K18;
    else if (sum <= 8388604)
        return ID_K19;
    else if (sum <= 16777212)
        return ID_K20;
    else if (sum <= 33554428)
        return ID_K21;
    else if (sum <= 67108860)
        return ID_K22;
    else if (sum <= 134217724)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_ref_winner8(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 8 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        sum += *s++;

    /*** J = 7; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 3 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 11)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 7)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 25)
        return ID_K1;
    else if (sum <= 53)
        return ID_K2;
    else if (sum <= 109)
        return ID_K3;
    else if (sum <= 221)
        return ID_K4;
    else if (sum <= 445)
        return ID_K5;
    else if (sum <= 893)
        return ID_K6;
    else if (sum <= 1789)
        return ID_K7;
    else if (sum <= 3581)
        return ID_K8;
    else if (sum <= 7165)
        return ID_K9;
    else if (sum <= 14333)
        return ID_K10;
    else if (sum <= 28669)
        return ID_K11;
    else if (sum <= 57341)
        return ID_K12;
    else if (sum <= 114685)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 229373)
        return ID_K14;
    else if (sum <= 458749)
        return ID_K15;
    else if (sum <= 917501)
        return ID_K16;
    else if (sum <= 1835005)
        return ID_K17;
    else if (sum <= 3670013)
        return ID_K18;
    else if (sum <= 7340029)
        return ID_K19;
    else if (sum <= 14680061)
        return ID_K20;
    else if (sum <= 29360125)
        return ID_K21;
    else if (sum <= 58720253)
        return ID_K22;
    else if (sum <= 117440509)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_winner10(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 10 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        {
        sum += *s++;
        sum += *s++;
        }

    /*** J = 10; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 5 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 15)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 10)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 35)
        return ID_K1;
    else if (sum <= 75)
        return ID_K2;
    else if (sum <= 155)
        return ID_K3;
    else if (sum <= 315)
        return ID_K4;
    else if (sum <= 635)
        return ID_K5;
    else if (sum <= 1275)
        return ID_K6;
    else if (sum <= 2555)
        return ID_K7;
    else if (sum <= 5115)
        return ID_K8;
    else if (sum <= 10235)
        return ID_K9;
    else if (sum <= 20475)
        return ID_K10;
    else if (sum <= 40955)
        return ID_K11;
    else if (sum <= 81915)
        return ID_K12;
    else if (sum <= 163835)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 327675)
        return ID_K14;
    else if (sum <= 655355)
        return ID_K15;
    else if (sum <= 1310715)
        return ID_K16;
    else if (sum <= 2621435)
        return ID_K17;
    else if (sum <= 5242875)
        return ID_K18;
    else if (sum <= 10485755)
        return ID_K19;
    else if (sum <= 20971515)
        return ID_K20;
    else if (sum <= 41943035)
        return ID_K21;
    else if (sum <= 83886075)
        return ID_K22;
    else if (sum <= 167772155)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_ref_winner10(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 9 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        sum += *s++;

    /*** J = 9; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 4 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 14)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 9)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 32)
        return ID_K1;
    else if (sum <= 68)
        return ID_K2;
    else if (sum <= 140)
        return ID_K3;
    else if (sum <= 284)
        return ID_K4;
    else if (sum <= 572)
        return ID_K5;
    else if (sum <= 1148)
        return ID_K6;
    else if (sum <= 2300)
        return ID_K7;
    else if (sum <= 4604)
        return ID_K8;
    else if (sum <= 9212)
        return ID_K9;
    else if (sum <= 18428)
        return ID_K10;
    else if (sum <= 36860)
        return ID_K11;
    else if (sum <= 73724)
        return ID_K12;
    else if (sum <= 147452)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 294908)
        return ID_K14;
    else if (sum <= 589820)
        return ID_K15;
    else if (sum <= 1179644)
        return ID_K16;
    else if (sum <= 2359292)
        return ID_K17;
    else if (sum <= 4718588)
        return ID_K18;
    else if (sum <= 9437180)
        return ID_K19;
    else if (sum <= 18874364)
        return ID_K20;
    else if (sum <= 37748732)
        return ID_K21;
    else if (sum <= 75497468)
        return ID_K22;
    else if (sum <= 150994940)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_winner16(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 16 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        {
        sum += *s++;
        sum += *s++;
        }

    /*** J = 16; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 8 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 24)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 16)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 56)
        return ID_K1;
    else if (sum <= 120)
        return ID_K2;
    else if (sum <= 248)
        return ID_K3;
    else if (sum <= 504)
        return ID_K4;
    else if (sum <= 1016)
        return ID_K5;
    else if (sum <= 2040)
        return ID_K6;
    else if (sum <= 4088)
        return ID_K7;
    else if (sum <= 8184)
        return ID_K8;
    else if (sum <= 16376)
        return ID_K9;
    else if (sum <= 32760)
        return ID_K10;
    else if (sum <= 65528)
        return ID_K11;
    else if (sum <= 131064)
        return ID_K12;
    else if (sum <= 262136)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 524280)
        return ID_K14;
    else if (sum <= 1048568)
        return ID_K15;
    else if (sum <= 2097144)
        return ID_K16;
    else if (sum <= 4194296)
        return ID_K17;
    else if (sum <= 8388600)
        return ID_K18;
    else if (sum <= 16777208)
        return ID_K19;
    else if (sum <= 33554424)
        return ID_K20;
    else if (sum <= 67108856)
        return ID_K21;
    else if (sum <= 134217720)
        return ID_K22;
    else if (sum <= 268435448)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_ref_winner16(sigma, end)
unsigned *sigma;
unsigned *end;
{
    int bits;
    int sum;
    unsigned *s;

    /*** Find best coding for 16 pixels per block. ***/
    /*** The compression may be slightly sub optimum ***/
    s = sigma;
    sum = 0;
    while (s < end)
        sum += *s++;

    /*** J = 15; V(k) = J * 2^k - J/2; V(k) = 2*V(k-1) + 7 ***/
    if (sum == 0)
        return ID_ZERO;
    else if (sum <= 3)
        return ID_LOW;
    else if (sum <= 23)
        {
        bits = c_ext2(sigma, end);
        if (bits < sum + 15)
            return ID_LOW;
        else
            return ID_FS;
        }
    else if (sum <= 53)
        return ID_K1;
    else if (sum <= 113)
        return ID_K2;
    else if (sum <= 233)
        return ID_K3;
    else if (sum <= 473)
        return ID_K4;
    else if (sum <= 953)
        return ID_K5;
    else if (sum <= 1913)
        return ID_K6;
    else if (sum <= 3833)
        return ID_K7;
    else if (sum <= 7673)
        return ID_K8;
    else if (sum <= 15353)
        return ID_K9;
    else if (sum <= 30713)
        return ID_K10;
    else if (sum <= 61433)
        return ID_K11;
    else if (sum <= 122873)
        return ID_K12;
    else if (sum <= 245753)
        return allow_k13 ? ID_K13 : ID_DEFAULT;
    else if (sum <= 491513)
        return ID_K14;
    else if (sum <= 983033)
        return ID_K15;
    else if (sum <= 1966073)
        return ID_K16;
    else if (sum <= 3932153)
        return ID_K17;
    else if (sum <= 7864313)
        return ID_K18;
    else if (sum <= 15728633)
        return ID_K19;
    else if (sum <= 31457273)
        return ID_K20;
    else if (sum <= 62914553)
        return ID_K21;
    else if (sum <= 125829113)
        return ID_K22;
    else if (sum <= 251658233)
        return ID_K23;
    else
        return ID_DEFAULT;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static int
find_winner(start, end)
unsigned *start;
unsigned *end;
{
    int msb1;
    int msb2;
    int msb3;
    int minbits;
    int i;
    int id;
    int jcnt;
    unsigned *s;

    jcnt = end - start;

    msb1 = 3*jcnt;
    msb2 = 4*jcnt;
    for (s = start; s < end; s++)
        {
        msb1 += *s >> 2;
        msb2 += *s >> 3;
        }

    minbits = msb1;
    id = ID_K2;
    if (msb2 < msb1)
        {
        minbits = msb2;
        id = ID_K3;
        }

    if (id == ID_K2) /* low entropy options possible */
        {
        msb1 = jcnt;
        msb2 = 2*jcnt; /* jcnt fsbits */
        for (s = start; s < end; s++)
            {
            msb1 += *s;
            msb2 += *s >> 1;
            }

        if (msb2 <= minbits)
            {
            if (msb1 <= msb2)
                {
                if (msb1 == jcnt)
                    {
                    minbits = 0;  /* zero cnt calculated in encode_zero fcn */
                    id = ID_ZERO;
                    }
                else
                    {
                    msb3 = c_ext2(start, end); /* this includes toggle bit*/
                    if (msb3 <= msb1)
                        {
                        minbits = msb3;
                        id = ID_LOW;
                        }
                    else
                        {
                        minbits = msb1;
                        id = ID_FS;
                        }
                    }
                }
            else
                {
                minbits = msb2;
                id = ID_K1;
                }
            }
        }
    else
        {
        msb1 = 5*jcnt;
        msb2 = 6*jcnt;
        for (s = start; s < end; s++)
            {
            msb1 += *s >> 4;
            msb2 += *s >> 5;
            }

        if (msb1 < minbits)
            {
            if (msb2 < msb1)
                {
                minbits = msb2;
                id = ID_K5;
                }
            else
                {
                minbits = msb1;
                id = ID_K4;
                }
            }
        }

    if (bits_per_pixel > 8 && id >= ID_K5)
        {
        msb1 = 7*jcnt;
        msb2 = 8*jcnt;
        for (s = start; s < end; s++)
            {
            msb1 += *s >> 6;
            msb2 += *s >> 7;
            }

        if (msb1 < minbits)
            {
            if (msb2 <msb1)
                {
                minbits = msb2;
                id = ID_K7;
                }
            else
                {
                minbits = msb1;
                id = ID_K6;
                }

            msb1 = 9*jcnt;
            msb2 = 10*jcnt;
            for (s = start; s < end; s++)
                {
                msb1 += *s >> 8;
                msb2 += *s >> 9;
                }

            if (msb1 < minbits)
                {
                if (msb2 < msb1)
                    {
                    minbits = msb2;
                    id = ID_K9;
                    }
                else
                    {
                    minbits = msb1;
                    id = ID_K8;
                    }

                msb1 = 11*jcnt;
                msb2 = 12*jcnt;
                for (s = start; s < end; s++)
                    {
                    msb1 += *s >> 10;
                    msb2 += *s >> 11;
                    }

                if (msb1 < minbits)
                    {
                    if (msb2 < msb1)
                        {
                        minbits = msb2;
                        id = ID_K11;
                        }
                    else
                        {
                        minbits = msb1;
                        id = ID_K10;
                        }

                    msb1 = 13*jcnt;
                    for (s = start; s < end; s++)
                        msb1 += *s >>12;

                    if (msb1 < minbits)
                        {
                        minbits = msb1;
                        id = ID_K12;
                        }
                    }
                }
            }

        if (id == ID_K12)
            {
            for (i = allow_k13 ? 13 : 14; i < bits_per_pixel-1; i++)
                {
                msb1 = (i+1)*jcnt;
                for (s = start; s < end; s++)
                    msb1 += *s >> i;

                if (msb1 < minbits)
                    {
                    minbits = msb1;
                    id = K_FACTOR + i;
#if 0
                    printf("k=%d: jcnt=%d minbits = %d\n", i, jcnt, minbits);
#endif
                    }
                }
            }
        }

    if (pixel_bit_count[jcnt] <= minbits)
        {
        id = default_id;
        minbits = pixel_bit_count[jcnt];
        }

    return id;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if !HDF
static void
set_defaults()
{
    compression_mode = NN_MODE;
    msb_first = *((char *) &msb_test) == 0;

    bits_per_pixel = DEFAULT_BITS_PER_PIXEL;
    pixels_per_block = DEFAULT_PIXELS_PER_BLOCK;
    pixels_per_scanline = DEFAULT_PIXELS_PER_SCANLINE;
}

static void
help(name)
char *name;
{
    fprintf(stderr, "%s: Science data lossless compression program.\n", name);
    fprintf(stderr, "%s\n", copyright+5);
    fprintf(stderr, "%s\n", sccs_id);
    fprintf(stderr, "UNM, Microelectronics Research Center, Albuquerque, New Mexico 87131 USA\n");
    fprintf(stderr, "===============================================================================\n");
    fprintf(stderr, "%s\n", credits1);
    fprintf(stderr, "%s\n", credits2);
    fprintf(stderr, "%s\n", credits3);
    fprintf(stderr, "===============================================================================\n");

    if (encoding)
        fprintf(stderr, "%s [options] [file ...]\n", name);
    else
        fprintf(stderr, "%s [options] [file[.sz] ...]\n", name);

    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -chip ................. Compress exactly as chip.  Slower than default.\n");
    fprintf(stderr, " -d .................... Decode (decompress) file(s).\n");
    fprintf(stderr, " -ec ................... Entropy coding compression mode.\n");
    fprintf(stderr, " -help ................. Print help message.\n");
    fprintf(stderr, " -j pixels-per-block ... Default is %d.  Faster compression for values 8,10,16.\n", DEFAULT_PIXELS_PER_BLOCK);
    fprintf(stderr, " -ki ................... Keep image file.  Default removes image file.\n");
    fprintf(stderr, " -kz ................... Keep .sz compressed file: file.sz\n");
    fprintf(stderr, " -lsb .................. Least significant byte first when bits-per-pixel > 8.\n");
    fprintf(stderr, " -msb .................. Most significant byte first when bits-per-pixel > 8.\n");
    fprintf(stderr, " -n bits-per-pixel ..... Must be in range [3..16].  Default is %d.\n", DEFAULT_BITS_PER_PIXEL);
    fprintf(stderr, " -nn ................... Nearest neighbor compression mode.  Default.\n");
    fprintf(stderr, " -raw .................. Omit header.  Must decode with encoder's options.\n");
    fprintf(stderr, " -s pixels-per-scanline  Default is %d.\n", DEFAULT_PIXELS_PER_SCANLINE);
    fprintf(stderr, " -v .................... Print stats for file compression.\n");
    fprintf(stderr, " -V .................... Print stats for bit compression.\n");
    fprintf(stderr, "\n");

    if (encoding)
        {
        fprintf(stderr, "For each file, create an encoded (compressed) file named: file.sz.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "NOTE: USES chip supports bits-per-pixel in range [4..15].\n");
        }
    else
        {
        fprintf(stderr, "For each file.sz, create a decoded (uncompressed) file named: file.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "NOTE: No options are necessary unless file encoded with -raw option.\n");
        }
}

static void
parse_args(argc, argv)
int argc;
char **argv;
{
    int i;

    file_count = 0;
    strcpy(input_file_name, "command-line");
    for (i = 1; i < argc; i++)
        {
        if (*argv[i] == '-')
            {
            if (eq(argv[i], "-chip"))
                {
                compress_exactly_as_chip = TRUE;
                allow_k13 = FALSE;
                }
            else if (eq(argv[i], "-d"))
                encoding = FALSE;
            else if (eq(argv[i], "-ec"))
                compression_mode = EC_MODE;
            else if (eq(argv[i], "-help"))
                {
                help(argv[0]);
                exit(1);
                }
            else if (eq(argv[i], "-j"))
                {
                if (++i < argc)
                    {
                    if (sscanf(argv[i], "%d", &pixels_per_block) != 1 || pixels_per_block < 2 || pixels_per_block > MAX_PIXELS_PER_BLOCK)
                        {
                        error("-j value must be an even integer in range 2..%d.\n", MAX_PIXELS_PER_BLOCK);
                        exit(1);
                        }

                    if (pixels_per_block & 1)
                        {
                        error("-j value must even.\n");
                        exit(1);
                        }
                    }
                else
                    {
                    error("-j option requires next argument to be an even integer.\n");
                    exit(1);
                    }
                }
            else if (eq(argv[i], "-ki"))
                keep_image_file = TRUE;
            else if (eq(argv[i], "-kz"))
                keep_compressed_file = TRUE;
            else if (eq(argv[i], "-lsb"))
                msb_first = FALSE;
            else if (eq(argv[i], "-msb"))
                msb_first = TRUE;
            else if (eq(argv[i], "-n"))
                {
                if (++i < argc)
                    {
                    if (sscanf(argv[i], "%d", &bits_per_pixel) != 1 || bits_per_pixel < 1 || bits_per_pixel > 24)
                        {
                        error("-n value must be an integer in range 1..24.\n");
                        exit(1);
                        }
                    }
                else
                    {
                    error("-n option requires next argument to be an integer.\n");
                    exit(1);
                    }
                }
            else if (eq(argv[i], "-nn"))
                compression_mode = NN_MODE;
            else if (eq(argv[i], "-nok13"))
                allow_k13 = FALSE;
            else if (eq(argv[i], "-raw"))
                {
                raw_mode = TRUE;
                keep_image_file = TRUE;
                }
            else if (eq(argv[i], "-s"))
                {
                if (++i < argc)
                    {
                    if (sscanf(argv[i], "%d", &pixels_per_scanline) != 1 || pixels_per_scanline < 2 || pixels_per_scanline > MAX_PIXELS_PER_SCANLINE)
                        {
                        error("-s value must be an integer in range 2..%d.\n", MAX_PIXELS_PER_SCANLINE);
                        exit(1);
                        }
                    }
                else
                    {
                    error("-s option requires next argument to be an integer.\n");
                    exit(1);
                    }
                }
            else if (eq(argv[i], "-v"))
                verbose_mode = 1;
            else if (eq(argv[i], "-V"))
                verbose_mode = 2;
            else
                {
                error("%s is not a valid option.\n", argv[i]);
                exit(1);
                }
            }
        else
            {
            if (file_count >= MAX_COMMAND_LINE_FILES)
                {
                error("Maximum number of files exceeded.\n");
                exit(1);
                }

            file_array[file_count++] = argv[i];
            }
        }
}
#endif /* !HDF */

#ifndef REMOVE_SZIP_ENCODER
static void
check_args()
{
    if (pixels_per_block & 1)
        {
        error("Pixels per block must be even.\n");
        return;
        }

    if (pixels_per_block > pixels_per_scanline)
        {
        error("Pixels per block is greater than pixels per scanline.\n");
        return;
        }

    if (blocks_per_scanline > MAX_BLOCKS_PER_SCANLINE)
        {
        error("Maximum %d blocks_per_scanline exceeded.\n", MAX_BLOCKS_PER_SCANLINE);
        return;
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if !HDF
#ifndef REMOVE_SZIP_ENCODER
static void
print_stats(bytes_read, bytes_written)
long bytes_read;
long bytes_written;
{
    FILE *fp_stats;
    double bit_compression_percent;
    double bit_compression_ratio;
    double compression_percent;
    double compression_ratio;
    double compression_bits_per_pixel;
    long image_bits;
    long image_bytes;

    if (bytes_read == 0)
        return;

    fp_stats = fp_out == stdout ? stderr : stdout;

    if (bits_per_pixel > 8)
        image_bits = bits_per_pixel * bytes_read / 2;
    else
        image_bits = bits_per_pixel * bytes_read;

    image_bytes = (image_bits + 7) / 8;
    if (compression_mode == EC_MODE)
        fprintf(fp_stats, "%s:sz:ec ", input_file_name);
    else
        fprintf(fp_stats, "%s:sz:nn ", input_file_name);

    bit_compression_percent = (1.0 - (double) bytes_written / image_bytes) * 100.0;
    compression_percent = (1.0 - (double) bytes_written / bytes_read) * 100.0;

    bit_compression_ratio = (double) image_bytes / bytes_written;
    compression_ratio = (double) bytes_read / bytes_written;

    compression_bits_per_pixel = ((double) bits_per_pixel * bytes_written) / image_bytes;

    fprintf(fp_stats, "%d:%d:%d ", bits_per_pixel, pixels_per_block, pixels_per_scanline);
    fprintf(fp_stats, "%cC ", verbose_mode == 2 ? 'I' : 'F');
    fprintf(fp_stats, "%0.2lf%% ", verbose_mode == 2 ? bit_compression_percent : compression_percent);
    fprintf(fp_stats, "%0.2lf:1 ", verbose_mode == 2 ? bit_compression_ratio : compression_ratio);
    fprintf(fp_stats, "Nav=%0.2lf ", compression_bits_per_pixel);
    fprintf(fp_stats, "I=%ld O=%ld\n", image_bytes, bytes_written);
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static void
open_encoder_files(file_name)
char *file_name;
{
    struct stat stat_buffer;

    strcpy(input_file_name, file_name);
    sprintf(output_file_name, "%s.sz", file_name);
    if ((fp_in = fopen(input_file_name, "rb")) == 0)
        {
        error("Unable to open binary input file: %s\n", input_file_name);
        return;
        }

    if (fstat(fileno(fp_in), &stat_buffer) != 0)
        {
        error("Unable to fstat() input file: %s\n", input_file_name);
        return;
        }

    input_file_size = stat_buffer.st_size;

    input_pixel_count = input_file_size;
    if (bits_per_pixel > 16)
        input_pixel_count >>= 2;
    else if (bits_per_pixel > 8)
        input_pixel_count >>= 1;

    if ((fp_out = fopen(output_file_name, "wb")) == 0)
        {
        error("Unable to open output file: %s\n", output_file_name);
        return;
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

static void
open_decoder_files(file_name)
char *file_name;
{
    int len;

    strcpy(base_file_name, file_name);
    len = strlen(base_file_name);
    if (len > 3 && eqn(base_file_name + len - 3, ".sz", 3))
        base_file_name[len-3] = 0;

    sprintf(input_file_name,  "%s.sz", base_file_name);
    sprintf(output_file_name, "%s.sd", base_file_name);
    if ((fp_in = fopen(input_file_name, "rb")) == 0)
        {
        error("Unable to open sz file\n");
        return;
        }

    if ((fp_out = fopen(output_file_name, "wb")) == 0)
        {
        error(msg, "Unable to open file: %s\n", output_file_name);
        return;
        }
}
#endif /* !HDF */

#ifndef REMOVE_SZIP_ENCODER
static void
encode_initialize()
{
    int i;
    int j;

    global_packed_bits = 32;
    global_packed_value = 0;

    blocks_per_scanline = (pixels_per_scanline+pixels_per_block-1)/pixels_per_block;
    padded_pixels_per_scanline = blocks_per_scanline*pixels_per_block;

    if (bits_per_pixel > 16)
        {
        bytes_per_pixel = 4;
        id_bits = 5;
        default_id = ID_DEFAULT3;
        }
    else if (bits_per_pixel > 8)
        {
        bytes_per_pixel = 2;
        id_bits = 4;
        default_id = ID_DEFAULT2;
        }
    else
        {
        bytes_per_pixel = 1;
        id_bits = 3;
        default_id = ID_DEFAULT1;
        }

    masknot[0] = 0;
    for (i = 1; i <= 24; i++)
        masknot[i] = (1 << i) - 1;

    for (i = 1; i <= MAX_PIXELS_PER_BLOCK; i++)
        pixel_bit_count[i] = pixel_bit_count[i-1] + bits_per_pixel;

    for (i = 0; i <= MAX_EXT2; i++)
        for (j = 0; j <= MAX_EXT2-i; j++)
            ext2_array[i][j] = (i+j)*(i+j+1)/2 + j + 1;

    total_coded_bytes = 0;
    xmax = (1 << bits_per_pixel) - 1;    /* max pos integer AFTER level shifting*/

    bmid = output_buffer + sizeof(output_buffer)/2;

    if (compress_exactly_as_chip)
        {
        winner_function = find_winner;
        winner_ref_function = find_winner;
        }
    else if (pixels_per_block == 8)
        {
        winner_function = find_winner8;
        winner_ref_function = find_ref_winner8;
        }
    else if (pixels_per_block == 10)
        {
        winner_function = find_winner10;
        winner_ref_function = find_ref_winner10;
        }
    else if (pixels_per_block == 16)
        {
        winner_function = find_winner16;
        winner_ref_function = find_ref_winner16;
        }
    else
        {
        winner_function = find_winner;
        winner_ref_function = find_winner;
        }
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

static void
decode_initialize()
{
    int i;
    int index;
    int j;
    int k;
    int *p;

    output_pixel_count = 0x7fffffff;
    if (!raw_mode)
        {
        read_header();
        if (error_count)
            return;
        }

    blocks_per_scanline = (pixels_per_scanline + pixels_per_block - 1)/pixels_per_block;
    padded_pixels_per_scanline = blocks_per_scanline * pixels_per_block;

    if (bits_per_pixel > 16)
        {
        bytes_per_pixel = 4;
        default_id = ID_DEFAULT3;
        }
    else if (bits_per_pixel > 8)
        {
        bytes_per_pixel = 2;
        default_id = ID_DEFAULT2;
        }
    else
        {
        bytes_per_pixel = 1;
        default_id = ID_DEFAULT1;
        }

    xmax = (1 << bits_per_pixel) - 1;

    bptr = output_buffer;
    bmid = output_buffer + sizeof(output_buffer)/2;

    p = leading_zeros;
    *p++ = 8;
    for (i = 1, k = 7; i < 256; i += i, k--)
        for (j = 0; j < i; j++)
            *p++ = k;

    for (i = 0; i <= MAX_EXT2; i++)
        for (j = 0; j <= MAX_EXT2-i; j++)
            {
            index = (i+j)*(i+j+1)/2 + j;
            ext2_array1[index] = i;
            ext2_array2[index] = j;
            }

    output_buffer_full = FALSE;
}

#if !HDF
#ifndef REMOVE_SZIP_ENCODER
static void
post_encode()
{
    if (ferror(fp_in))
        {
        fclose(fp_in);
        fclose(fp_out);
        remove(output_file_name);
        error("Read of input file failed.\n");
        return;
        }

    fclose(fp_in);
    if (ferror(fp_out))
        {
        fclose(fp_out);
        remove(output_file_name);
        error("Write of output file failed: %s\n", output_file_name);
        return;
        }

    fclose(fp_out);
    if (!keep_image_file && fp_in != stdin)
        remove(input_file_name);
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_file(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, file_name)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
char *file_name;
{
    long bytes_read;

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    keep_compressed_file = (new_options_mask & KEEP_COMPRESSED_OPTION_MASK) != 0;
    keep_image_file = (new_options_mask & KEEP_IMAGE_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = new_bits_per_pixel;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

    input_mode  = FILE_DATA;
    output_mode = FILE_DATA;

    open_encoder_files(file_name);
    if (error_count)
        return -1;

    global_bptr = output_buffer;
    encode_initialize();
    check_args();
    if (error_count)
        return -1;

    bytes_read = rice_encode();
    post_encode();
    if (error_count)
        return -1;

    if (verbose_mode)
        print_stats(bytes_read, total_coded_bytes);

    return total_coded_bytes;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */
#endif /* !HDF */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_memory_bytes(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, pixels, out)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
#if !HDF
    long bytes_read;
#endif /* !HDF */
    long bytes_written;

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    allow_k13 = (new_options_mask & ALLOW_K13_OPTION_MASK) != 0;
    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = new_bits_per_pixel;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = (unsigned char *) in;
    input_pixel_count = pixels;

#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    global_bptr = out;
    encode_initialize();
    check_args();
    if (error_count)
        return PARAM_ERROR;

#if HDF
    rice_encode();
#else
    bytes_read = rice_encode();
#endif /* HDF */
    bytes_written = global_bptr - out;

#if !HDF
    if (verbose_mode)
        print_stats(bytes_read, bytes_written);
#endif /* !HDF */

    return bytes_written;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_memory_words(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, pixels, out)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
#if !HDF
    long bytes_read;
#endif /* !HDF */
    long bytes_written;

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    allow_k13 = (new_options_mask & ALLOW_K13_OPTION_MASK) != 0;
    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = new_bits_per_pixel;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = (unsigned char *) in;
    input_pixel_count = pixels;
#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    global_bptr = out;
    encode_initialize();
    check_args();
    if (error_count)
        return PARAM_ERROR;

#if HDF
    rice_encode();
#else
    bytes_read = rice_encode();
#endif /* HDF */
    bytes_written = global_bptr - out;

#if !HDF
    if (verbose_mode)
        print_stats(bytes_read, bytes_written);
#endif /* !HDF */

    return bytes_written;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_memory_longs(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, pixels, out)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
#if !HDF
    long bytes_read;
#endif /* !HDF */
    long bytes_written;

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    allow_k13 = (new_options_mask & ALLOW_K13_OPTION_MASK) != 0;
    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = new_bits_per_pixel;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = (unsigned char *) in;
    input_pixel_count = pixels;
#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    global_bptr = out;
    encode_initialize();
    check_args();
    if (error_count)
        return PARAM_ERROR;

#if HDF
    rice_encode();
#else
    bytes_read = rice_encode();
#endif /* HDF */
    bytes_written = global_bptr - out;

#if !HDF
    if (verbose_mode)
        print_stats(bytes_read, bytes_written);
#endif /* !HDF */

    return bytes_written;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static void
interleave(in, bytes, bits, out)
char *in;
long bytes;
int bits;
char *out;
{
    char *b;
    char *s;
    int i;
    int j;
    int words;
    int word_size;

    word_size = bits/8;
    words = bytes/word_size;

    b = out;
    for (j = 0; j < word_size; j++)
        {
        s = in;
        s += j;
#if 0
        if (msb_first)
            s += j;
        else
            s += word_size - j - 1;
#endif

        for (i = 0; i < words; i++)
            {
            *b++ = *s;
            s += word_size;
            }
        }
}
#endif /* REMOVE_SZIP_ENCODER */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_memory_floats(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, pixels, out)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
    static unsigned char *interleave_array;
#if !HDF
    long bytes_read;
#endif /* !HDF */
    long bytes_written;

    if (new_bits_per_pixel != 32)
        {
        error("I only know how to compress 32 bit floats.\n");
        return PARAM_ERROR;
        }

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    allow_k13 = (new_options_mask & ALLOW_K13_OPTION_MASK) != 0;
    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = 8;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

    interleave_array = (unsigned char *) malloc(4 * pixels);
    if (interleave_array == 0)
        {
        error("Out of Memory.\n");
        return MEMORY_ERROR;
        }

    interleave((char *) in, pixels*4, new_bits_per_pixel, (char *) interleave_array);

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = interleave_array;
    input_pixel_count = pixels*4;
#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    global_bptr = out;
    encode_initialize();
    check_args();
    if (error_count)
        {
        free(interleave_array);
        return PARAM_ERROR;
        }

#if HDF
    rice_encode();
#else
    bytes_read = rice_encode();
#endif /* HDF */
    bytes_written = global_bptr - out;

#if !HDF
    if (verbose_mode)
        print_stats(bytes_read, bytes_written);
#endif /* !HDF */

    free(interleave_array);
    return bytes_written;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static long
compress_memory_doubles(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, pixels, out)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
    static unsigned char *interleave_array;
#if !HDF
    long bytes_read;
#endif /* !HDF */
    long bytes_written;

    if (new_bits_per_pixel != 64)
        {
        error("I only know how to compress 64 bit doubles.\n");
        return PARAM_ERROR;
        }

    new_options_mask |= ALLOW_K13_OPTION_MASK;

    allow_k13 = (new_options_mask & ALLOW_K13_OPTION_MASK) != 0;
    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    compress_exactly_as_chip = (new_options_mask & CHIP_OPTION_MASK) != 0;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    if (compress_exactly_as_chip)
        allow_k13 = FALSE;

    bits_per_pixel = 8;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

    interleave_array = (unsigned char *) malloc(8 * pixels);
    if (interleave_array == 0)
        {
        error("Out of Memory.\n");
        return MEMORY_ERROR;
        }

    interleave((char *) in, pixels*8, new_bits_per_pixel, (char *) interleave_array);

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = interleave_array;
    input_pixel_count = pixels*8;
#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    global_bptr = out;
    encode_initialize();
    check_args();
    if (error_count)
        {
        free(interleave_array);
        return PARAM_ERROR;
        }

#if HDF
    rice_encode();
#else
    bytes_read = rice_encode();
#endif
    bytes_written = global_bptr - out;

#if !HDF
    if (verbose_mode)
        print_stats(bytes_read, bytes_written);
#endif /* !HDF */

    free(interleave_array);
    return bytes_written;
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

/****************************************************************
* Must be external to be linked to HDF code.                    *
* This function calls the proper function to encode byte, word, *
* long, float, and double data types.                           *
* Valid bits_per_pixel are: 1-24,32,64.                         *
****************************************************************/
long
compress_memory(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out)
int options_mask;
int bits_per_pixel;
int pixels_per_block;
int pixels_per_scanline;
const void *in;
long pixels;
char *out;
{
#ifndef REMOVE_SZIP_ENCODER
    long out_bytes;
#endif /* !defined(REMOVE_SZIP_ENCODER) */

    /*** reset error_count; if non zero an error occured during compression ***/
    error_count = 0;
    warning_count = 0;

#ifdef REMOVE_SZIP_ENCODER
    error("This executable has szip encoding compiled out.\n");
    return NO_ENCODER_ERROR;
#else
#if HDF
    if (!szip_allow_encoding)
        {
        error("This executable does not allow szip encoding.\n");
        return NO_ENCODER_ERROR;
        }
#endif

    if (bits_per_pixel <= 8)
        out_bytes = compress_memory_bytes(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out);
    else if (bits_per_pixel <= 16)
        out_bytes = compress_memory_words(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out);
    else if (bits_per_pixel <= 24)
        out_bytes = compress_memory_longs(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out);
    else if (bits_per_pixel == 32)
        out_bytes = compress_memory_floats(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out);
    else if (bits_per_pixel == 64)
        out_bytes = compress_memory_doubles(options_mask, bits_per_pixel, pixels_per_block, pixels_per_scanline, in, pixels, out);
    else
        {
        error("compress_memory: szip compression does not work on %d bit data.\n", bits_per_pixel);
        return PARAM_ERROR;
        }

    return out_bytes;
#endif /* defined(REMOVE_SZIP_ENCODER) */
}

#if !HDF
#ifndef REMOVE_SZIP_ENCODER
static void
alternate_encoding(argc, argv)
int argc;
char **argv;
{
    FILE *fp;
    boolean chip;
    boolean msb;
    boolean nn;
    boolean raw;
    char file_name[256];
    char *in;
    char *out;
    int bytes;
    int j;
    int n;
    int option_mask;
    int s;
    int size;
    long pixels;

    if (argc != 10)
        {
        fprintf(stderr, "Usage: %s file-name pixels raw chip nn msb n j s\n", argv[0]);
        exit(1);
        }

    if ((fp = fopen(argv[1], "rb")) == 0)
        {
        fprintf(stderr, "Could not open input file: %s\n", argv[1]);
        exit(1);
        }

    pixels = atoi(argv[2]);
    raw    = atoi(argv[3]);
    chip   = atoi(argv[4]);
    nn     = atoi(argv[5]);
    msb    = atoi(argv[6]);
    n      = atoi(argv[7]);
    j      = atoi(argv[8]);
    s      = atoi(argv[9]);

    verbose_mode = 1;

    if (n <= 8)
        bytes = pixels;
    else if (n <= 16)
        bytes = 2*pixels;
    else if (n <= 32)
        bytes = 4*pixels;
    else if (n <= 64)
        bytes = 8*pixels;

    in = (char *) malloc(bytes);
    out = (char *) malloc(bytes+bytes/4 + 8);
    fread(in, 1, bytes, fp);
    fclose(fp);

    option_mask = 0;
    if (raw)
        option_mask |= RAW_OPTION_MASK;

    if (msb)
        option_mask |= MSB_OPTION_MASK;

    if (nn)
        option_mask |= NN_OPTION_MASK;

    if (chip)
        compress_exactly_as_chip = TRUE;
    else
        {
        compress_exactly_as_chip = FALSE;
        option_mask |= ALLOW_K13_OPTION_MASK;
        }

    size = compress_memory(option_mask, n, j, s, (float *) in, pixels, out);
#if 0
    if (n > 24)
        {
        if (n == 32)
            size = compress_memory_floats(option_mask, n, j, s, (float *) in, pixels, out);
        else if (n == 64)
            size = compress_memory_doubles(option_mask, n, j, s, (double *) in, pixels, out);
        else
            {
            error("Bits per pixel (n) out of range 1..24,32,64.\n");
            exit(1);
            }
        }
    else if (n > 16)
        size = compress_memory_longs(option_mask, n, j, s, (long *) in, pixels, out);
    else if (n > 8)
        size = compress_memory_words(option_mask, n, j, s, (short *) in, pixels, out);
    else
        size = compress_memory_bytes(option_mask, n, j, s, in, pixels, out);
#endif

    sprintf(file_name, "%s.sz", argv[1]);
    if ((fp = fopen(file_name, "wb")) == 0)
        {
        error("Unable to open output file: %s\n", file_name);
        exit(1);
        }

    fwrite(out, 1, size, fp);
    fclose(fp);

    exit(0);
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#ifndef REMOVE_SZIP_ENCODER
static void
alternate_encoding2(argc, argv)
int argc;
char **argv;
{
    char *file_name;
    int j;
    int n;
    int s;
    int size;

    if (argc != 5)
        {
        fprintf(stderr, "Usage: %s file-name n j s\n", argv[0]);
        exit(1);
        }

    file_name = argv[1];
    n = atoi(argv[2]);
    j = atoi(argv[3]);
    s = atoi(argv[4]);

    verbose_mode = 1;

    size = compress_file(ALLOW_K13_OPTION_MASK | MSB_OPTION_MASK | NN_OPTION_MASK, n, j, s, file_name);
    printf("size = %d\n", size);
    exit(0);
}
#endif /* !defined(REMOVE_SZIP_ENCODER) */
#endif /* !HDF */

static void
deinterleave(in, bytes, bits, out)
char *in;
long bytes;
int bits;
char *out;
{
    char *b;
    char *s;
    int i;
    int j;
    int words;
    int word_size;

    word_size = bits/8;
    words = bytes/word_size;
    b = in;
    for (j = 0; j < word_size; j++)
        {
        s = out + j;
        for (i = 0; i < words; i++)
            {
            *s = *b++;
            s += word_size;
            }
        }
}

static void
unmap_nn(sigma, pixels)
unsigned *sigma;
int pixels;
{
    char *xptr;
    int sig1;
    int sig2;
    int x;
    unsigned *end;
    unsigned *s;

    end = sigma + pixels;
    s = sigma;
    xptr = bptr;
    if (pixels & 1)
        {
        if (bytes_per_pixel == 1)
            {
            x = *s++;
            *xptr++ = x;

            while (s < end)
                {
                sig1 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = x;
                }

            bptr += pixels;
            }
        else if (bytes_per_pixel == 2)
            {
            if (msb_first)
                {
                x = *s++;
                *xptr++ = (unsigned) x >> 8;
                *xptr++ = x;

                while (s < end)
                    {
                    sig1 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = (unsigned) x >> 8;
                    *xptr++ = x;
                    }

                bptr += pixels << 1;
                }
            else
                {
                x = *s++;
                *xptr++ = x;
                *xptr++ = (unsigned) x >> 8;

                while (s < end)
                    {
                    sig1 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >> 8;
                    }

                bptr += pixels << 1;
                }
            }
        else
            {
            if (msb_first)
                {
                x = *s++;
                *xptr++ = (unsigned) x >> 24;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = x;

                while (s < end)
                    {
                    sig1 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = (unsigned) x >> 24;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = x;
                    }

                bptr += pixels << 2;
                }
            else
                {
                x = *s++;
                *xptr++ = x;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >> 24;

                while (s < end)
                    {
                    sig1 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >> 24;
                    }

                bptr += pixels << 2;
                }
            }
        }
    else
        {
        if (bytes_per_pixel == 1)
            {
            x = *s++;
            *xptr++ = x;

            sig1 = *s++;
            if (sig1 >= (x << 1))
                x = sig1;
            else if (sig1 > (xmax - x) << 1)
                x = xmax - sig1;
            else if (sig1 & 1)
                x = x - ((sig1 + 1) >> 1);
            else
                x = x + (sig1 >> 1);

            *xptr++ = x;

            while (s < end)
                {
                sig1 = *s++;
                sig2 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = x;

                if (sig2 >= (x << 1))
                    x = sig2;
                else if (sig2 > (xmax - x) << 1)
                    x = xmax - sig2;
                else if (sig2 & 1)
                    x = x - ((sig2 + 1) >> 1);
                else
                    x = x + (sig2 >> 1);

                *xptr++ = x;
                }

            bptr += pixels;
            }
        else if (bytes_per_pixel == 2)
            {
            if (msb_first)
                {
                x = *s++;
                *xptr++ = (unsigned) x >> 8;
                *xptr++ = x;

                sig1 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = (unsigned) x >> 8;
                *xptr++ = x;

                while (s < end)
                    {
                    sig1 = *s++;
                    sig2 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = (unsigned) x >> 8;
                    *xptr++ = x;

                    if (sig2 >= (x << 1))
                        x = sig2;
                    else if (sig2 > (xmax - x) << 1)
                        x = xmax - sig2;
                    else if (sig2 & 1)
                        x = x - ((sig2 + 1) >> 1);
                    else
                        x = x + (sig2 >> 1);

                    *xptr++ = (unsigned) x >> 8;
                    *xptr++ = x;
                    }

                bptr += pixels << 1;
                }
            else
                {
                x = *s++;
                *xptr++ = x;
                *xptr++ = (unsigned) x >> 8;

                sig1 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = x;
                *xptr++ = (unsigned) x >> 8;

                while (s < end)
                    {
                    sig1 = *s++;
                    sig2 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >> 8;

                    if (sig2 >= (x << 1))
                        x = sig2;
                    else if (sig2 > (xmax - x) << 1)
                        x = xmax - sig2;
                    else if (sig2 & 1)
                        x = x - ((sig2 + 1) >> 1);
                    else
                        x = x + (sig2 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >> 8;
                    }

                bptr += pixels << 1;
                }
            }
        else
            {
            if (msb_first)
                {
                x = *s++;
                *xptr++ = (unsigned) x >> 24;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = x;

                sig1 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = (unsigned) x >> 24;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = x;

                while (s < end)
                    {
                    sig1 = *s++;
                    sig2 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = (unsigned) x >> 24;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = x;

                    if (sig2 >= (x << 1))
                        x = sig2;
                    else if (sig2 > (xmax - x) << 1)
                        x = xmax - sig2;
                    else if (sig2 & 1)
                        x = x - ((sig2 + 1) >> 1);
                    else
                        x = x + (sig2 >> 1);

                    *xptr++ = (unsigned) x >> 24;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = x;
                    }

                bptr += pixels << 2;
                }
            else
                {
                x = *s++;
                *xptr++ = x;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >> 24;

                sig1 = *s++;
                if (sig1 >= (x << 1))
                    x = sig1;
                else if (sig1 > (xmax - x) << 1)
                    x = xmax - sig1;
                else if (sig1 & 1)
                    x = x - ((sig1 + 1) >> 1);
                else
                    x = x + (sig1 >> 1);

                *xptr++ = x;
                *xptr++ = (unsigned) x >>  8;
                *xptr++ = (unsigned) x >> 16;
                *xptr++ = (unsigned) x >> 24;

                while (s < end)
                    {
                    sig1 = *s++;
                    sig2 = *s++;
                    if (sig1 >= (x << 1))
                        x = sig1;
                    else if (sig1 > (xmax - x) << 1)
                        x = xmax - sig1;
                    else if (sig1 & 1)
                        x = x - ((sig1 + 1) >> 1);
                    else
                        x = x + (sig1 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >> 24;

                    if (sig2 >= (x << 1))
                        x = sig2;
                    else if (sig2 > (xmax - x) << 1)
                        x = xmax - sig2;
                    else if (sig2 & 1)
                        x = x - ((sig2 + 1) >> 1);
                    else
                        x = x + (sig2 >> 1);

                    *xptr++ = x;
                    *xptr++ = (unsigned) x >>  8;
                    *xptr++ = (unsigned) x >> 16;
                    *xptr++ = (unsigned) x >> 24;
                    }

                bptr += pixels << 2;
                }
            }
        }
}

static void
output_decoded_data(sigma)
unsigned *sigma;
{
    int i;
    int pixels;

    pixels = (output_pixel_count < pixels_per_scanline) ? output_pixel_count : pixels_per_scanline;
    output_pixel_count -= pixels;

    if (pixels == 0)
        output_buffer_full = TRUE;

    if (compression_mode == NN_MODE)
        unmap_nn(sigma, pixels);
    else
        {
        if (bytes_per_pixel == 1)
            for (i = 0; i < pixels; i++)
                *bptr++ = sigma[i];
        else if (bytes_per_pixel == 2)
            {
            if (msb_first)
                for (i = 0; i < pixels; i++)
                    {
                    *bptr++ = sigma[i] >> 8;
                    *bptr++ = sigma[i];
                    }
            else
                for (i = 0; i < pixels; i++)
                    {
                    *bptr++ = sigma[i];
                    *bptr++ = sigma[i] >> 8;
                    }
            }
        else
            {
            if (msb_first)
                for (i = 0; i < pixels; i++)
                    {
                    *bptr++ = sigma[i] >> 24;
                    *bptr++ = sigma[i] >> 16;
                    *bptr++ = sigma[i] >> 8;
                    *bptr++ = sigma[i];
                    }
            else
                for (i = 0; i < pixels; i++)
                    {
                    *bptr++ = sigma[i];
                    *bptr++ = sigma[i] >> 8;
                    *bptr++ = sigma[i] >> 16;
                    *bptr++ = sigma[i] >> 24;
                    }
            }
        }

#if !HDF
    if (bptr > bmid)
        {
        if (output_mode == FILE_DATA)
            {
            fwrite((void *) output_buffer, bptr - output_buffer, (size_t) 1, fp_out);
            bptr = output_buffer;
            }
        }
#endif /* !HDF */
}

#if !HDF
static void
flush_decoded_buffer()
{
    if (bptr != output_buffer)
        {
        if (output_mode == FILE_DATA)
            {
            fwrite((void*) output_buffer, (size_t) (bptr-output_buffer), (size_t) 1, fp_out);
            bptr = output_buffer;
            }
        }
}
#endif /* !HDF */

static void
rice_decode()
{
    int ext2_bit;
    int i;
    int id;
    int k_bits;
    int zero_blocks;
    unsigned *end;
    unsigned *s;
    unsigned *s1;
    unsigned sigma[MAX_PIXELS_PER_SCANLINE];
    unsigned char *b;
    unsigned char byte_buffer[4*INPUT_BUFFER_SIZE];
    int count;
    int extra;
    int n;
    unsigned short *pend;
    unsigned short *p;
    unsigned data_word;
    int data_bits;
    unsigned short input_buffer[INPUT_BUFFER_SIZE+2];
    unsigned short *input_ptr;
    unsigned short *input_end;

    input_end = input_buffer;
    input_ptr = input_buffer;

    data_bits = 0;
    data_word = 0;
    while (1)
        {
        if (input_ptr + pixels_per_scanline*4 >= input_end)
            {
#if !HDF
            if (input_mode == FILE_DATA)
                n = fread((void*) byte_buffer, (size_t) 1, (size_t) INPUT_BUFFER_SIZE, fp_in);
            else
#endif /* !HDF */
                {
                n = input_byte_count >= INPUT_BUFFER_SIZE ? INPUT_BUFFER_SIZE : input_byte_count;
                input_byte_count -= n;
                memcpy(byte_buffer, input_byte_data, n);
                input_byte_data += n;
                }

            if (n != 0)
                {
                if (n & 1)
                    {
                    byte_buffer[n] = 0;
                    n++;
                    }

                count = input_end - input_ptr;
                memcpy(input_buffer, input_ptr, count*sizeof(short));
                p = input_buffer + count;

                pend = input_buffer + (n >> 1) + count;
                b = byte_buffer;
                while (p < pend)
                    {
                    *p++ = (*b << 8) | *(b+1);
                    b += 2;
                    }

                *p++ = 0;
                *p++ = 0;

                input_end = pend;
                input_ptr = input_buffer;
                }
            else if (data_word == 0 && input_ptr >= input_end)
                break;

            if (data_bits == 0)
                {
                data_word = *input_ptr++ << 16;
                data_word |= *input_ptr++;
                data_bits = 32;
                }
            }

        s = sigma;
        end = s + pixels_per_block;
        if (compression_mode == NN_MODE)
            {
            i = 1;
            s++;
            /*** do first block of scanline since it contains the reference ***/
            if (bytes_per_pixel == 1)
                {
                id = data_word >> 29;
                data_word <<= 3;
                data_bits -= 3;
                }
            else if (bytes_per_pixel == 2)
                {
                id = data_word >> 28;
                data_word <<= 4;
                data_bits -= 4;
                }
            else
                {
                id = data_word >> 27;
                data_word <<= 5;
                data_bits -= 5;
                }

            if (id == ID_LOW)
                {
                ext2_bit = data_word & 0x80000000;
                data_word <<= 1;
                data_bits--;
                }

            if (data_bits <= 16)
                {
                data_word |= *input_ptr++ << (16 - data_bits);
                data_bits += 16;
                }

            k_bits = id - K_FACTOR;
            extra = bits_per_pixel - 16;
            if (extra <= 0)
                {
                *sigma = data_word >> (32 - bits_per_pixel);
                data_word <<= bits_per_pixel;
                data_bits -= bits_per_pixel;
                }
            else
                {
                *sigma = data_word >> 16;
                data_word <<= 16;
                data_bits -= 16;

                data_word |= *input_ptr++ << (16 - data_bits);
                data_bits += 16;

                *sigma <<= extra;
                *sigma |= data_word >> (32 - extra);
                data_word <<= extra;
                data_bits -= extra;
                }

            if (data_bits <= 16)
                {
                data_word |= *input_ptr++ << (16 - data_bits);
                data_bits += 16;
                }

            zero_blocks = 0;
            if (id >= ID_K1)
                {
                if (id == default_id)
                    {
                    int bits;
                    int shift;

                    extra = bits_per_pixel - 16;
                    if (extra <= 0)
                        {
                        shift = 32 - bits_per_pixel;
                        while (s < end)
                            {
                            bits = data_word >> shift;
                            data_word <<= bits_per_pixel;
                            data_bits -= bits_per_pixel;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = bits;
                            }
                        }
                    else
                        {
                        while (s < end)
                            {
                            bits = data_word >> 16;
                            data_word <<= 16;
                            data_bits -= 16;
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;

                            bits <<= extra;
                            bits |= data_word >> (32 - extra);
                            data_word <<= extra;
                            data_bits -= extra;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = bits;
                            }
                        }
                    }
                else
                    {
                    int bits;
                    int shift;
                    int zero_count;
                    int big_zero_count;

                    s1 = s;
                    while (s < end)
                        {
                        big_zero_count = 0;
                        while ((data_word & 0xff000000) == 0)
                            {
                            big_zero_count += 8;
                            data_word <<= 8;
                            data_bits -= 8;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }
                            }

                        zero_count = leading_zeros[data_word >> 24];
                        data_word <<= zero_count+1;
                        data_bits -= zero_count+1;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }

                        *s++ = zero_count + big_zero_count;
                        }

                    s = s1;
                    shift = 32 - k_bits;
                    extra = k_bits - 16;
                    if (extra <= 0)
                        {
                        while (s < end)
                            {
                            bits = data_word >> shift;
                            data_word <<= k_bits;
                            data_bits -= k_bits;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s = (*s << k_bits) | bits;
                            s++;
                            }
                        }
                    else
                        {
                        while (s < end)
                            {
                            bits = data_word >> 16;
                            data_word <<= 16;
                            data_bits -= 16;
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;

                            bits <<= extra;
                            bits |= data_word >> (32 - extra);

                            data_word <<= extra;
                            data_bits -= extra;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s = (*s << k_bits) | bits;
                            s++;
                            }
                        }
                    }
                }
            else if (id == ID_FS)
                {
                int big_zero_count;
                int zero_count;

                while (s < end)
                    {
                    big_zero_count = 0;
                    while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }
                        }

                    zero_count = leading_zeros[data_word >> 24];
                    data_word <<= zero_count+1;
                    data_bits -= zero_count+1;
                    if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                    *s++ = zero_count + big_zero_count;
                    }
                }
            else
                {
                if (ext2_bit)
                    {
                    int big_zero_count;
                    int m;
                    int zero_count;
                    unsigned *t;
                    unsigned *tend;
                    unsigned temp[MAX_PIXELS_PER_BLOCK];

                    /*** Note: The value, m, must be range checked, to avoid a segmentation violation ***/
                    /*** caused by the data being decoded and encoded with a different number of ***/
                    /*** pixels_per_scanline. ***/

                    /*** Read EXT2 FS values ***/
                    t = temp;
                    tend = temp + pixels_per_block/2;
                    while (t < tend)
                        {
                        big_zero_count = 0;
                        while ((data_word & 0xff000000) == 0)
                            {
                            big_zero_count += 8;
                            data_word <<= 8;
                            data_bits -= 8;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }
                            }

                        zero_count = leading_zeros[data_word >> 24];
                        data_word <<= zero_count+1;
                        data_bits -= zero_count+1;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }

                        *t++ = zero_count + big_zero_count;
                        }

                    t = temp;
                    m = *t++;
                    if (m >= MAX_EXT2_SUM)
                        m = 0;

                    *s++ = ext2_array2[m];
                    while (s < end)
                        {
                        m = *t++;
                        if (m >= MAX_EXT2_SUM)
                            m = 0;

                        *s++ = ext2_array1[m];
                        *s++ = ext2_array2[m];
                        }
                    }
                else
                    {
                    int bits;
                    int big_zero_count;
                    int zero_count;

                    big_zero_count = 0;
                    while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }
                        }

                    zero_count = leading_zeros[data_word >> 24];
                    data_word <<= zero_count+1;
                    data_bits -= zero_count+1;
                    if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                    bits = zero_count + big_zero_count + 1;

                    if (bits < 5)
                        zero_blocks = bits;
                    else if (bits == 5)
                        zero_blocks = blocks_per_scanline > MAX_ZERO_BLOCKS ? MAX_ZERO_BLOCKS : blocks_per_scanline;
                    else
                        zero_blocks = bits - 1;

                    i += zero_blocks-1;
                    if (i > blocks_per_scanline)
                        {
                        error("Decoded more blocks than in scanline.  Check -s value.\n");
                        return;
                        }

                    end += (zero_blocks-1) * pixels_per_block;
                    memset(s, 0, (end-s)*sizeof(int));
                    }
                }

            s = end;
            end += pixels_per_block;
            i++;
            }
        else
            i = 1;


        for (; i <= blocks_per_scanline; i++)
            {
            /*** do rest of blocks on the scanline since they contains no references ***/
            if (bytes_per_pixel == 1)
                {
                id = data_word >> 29;
                data_word <<= 3;
                data_bits -= 3;
                }
            else if (bytes_per_pixel == 2)
                {
                id = data_word >> 28;
                data_word <<= 4;
                data_bits -= 4;
                }
            else
                {
                id = data_word >> 27;
                data_word <<= 5;
                data_bits -= 5;
                }

            if (id == ID_LOW)
                {
                ext2_bit = data_word & 0x80000000;
                data_word <<= 1;
                data_bits--;
                }

            if (data_bits <= 16)
                {
                data_word |= *input_ptr++ << (16 - data_bits);
                data_bits += 16;
                }

            k_bits = id - K_FACTOR;
            zero_blocks = 0;
            if (id >= ID_K1)
                {
                if (id == default_id)
                    {
                    int bits;
                    int shift;

                    extra = bits_per_pixel - 16;
                    if (extra <= 0)
                        {
                        shift = 32 - bits_per_pixel;
                        while (s < end)
                            {
                            bits = data_word >> shift;
                            data_word <<= bits_per_pixel;
                            data_bits -= bits_per_pixel;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = bits;
                            }
                        }
                    else
                        {
                        while (s < end)
                            {
                            bits = data_word >> 16;
                            data_word <<= 16;
                            data_bits -= 16;
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;

                            bits <<= extra;
                            bits |= data_word >> (32 - extra);
                            data_word <<= extra;
                            data_bits -= extra;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = bits;
                            }
                        }
                    }
                else
                    {
                    int shift;
                    int zero_count;
                    int big_zero_count;

                    s1 = s;
#if 0
                    while (s < end)
                        {
                        big_zero_count = 0;
                        while ((data_word & 0xff000000) == 0)
                            {
                            big_zero_count += 8;
                            data_word <<= 8;
                            data_bits -= 8;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }
                            }

                        zero_count = leading_zeros[data_word >> 24];
                        data_word <<= zero_count+1;
                        data_bits -= zero_count+1;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }

                        *s++ = zero_count + big_zero_count;
                        }
#endif
                    while (s < end)
                        {
                        if ((data_word >> 30) == 3)
                            {
                            data_word <<= 2;
                            data_bits -= 2;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = 0;
                            *s++ = 0;
                            }
                        else if ((data_word >> 29) == 3)
                            {
                            data_word <<= 3;
                            data_bits -= 3;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = 1;
                            *s++ = 0;
                            }
                        else if ((data_word >> 29) == 5)
                            {
                            data_word <<= 3;
                            data_bits -= 3;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = 0;
                            *s++ = 1;
                            }
                        else if ((data_word >> 28) == 5)
                            {
                            data_word <<= 4;
                            data_bits -= 4;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = 1;
                            *s++ = 1;
                            }
                        else
                            {
                            big_zero_count = 0;
                            while ((data_word & 0xff000000) == 0)
                                {
                                big_zero_count += 8;
                                data_word <<= 8;
                                data_bits -= 8;
                                if (data_bits <= 16)
                                    {
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;
                                    }
                                }

                            zero_count = leading_zeros[data_word >> 24];
                            data_word <<= zero_count+1;
                            data_bits -= zero_count+1;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = zero_count + big_zero_count;

                            big_zero_count = 0;
                            while ((data_word & 0xff000000) == 0)
                                {
                                big_zero_count += 8;
                                data_word <<= 8;
                                data_bits -= 8;
                                if (data_bits <= 16)
                                    {
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;
                                    }
                                }

                            zero_count = leading_zeros[data_word >> 24];
                            data_word <<= zero_count+1;
                            data_bits -= zero_count+1;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }

                            *s++ = zero_count + big_zero_count;
                            }
                        }

                    s = s1;
                    if ((pixels_per_block & 7) == 0 && k_bits <= 16)
                        switch (k_bits)
                            {
                            case 1:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 1) | (data_word >> 31);
                                    *(s+1) = (*(s+1) << 1) | (data_word >> 30) & 1;
                                    *(s+2) = (*(s+2) << 1) | (data_word >> 29) & 1;
                                    *(s+3) = (*(s+3) << 1) | (data_word >> 28) & 1;
                                    *(s+4) = (*(s+4) << 1) | (data_word >> 27) & 1;
                                    *(s+5) = (*(s+5) << 1) | (data_word >> 26) & 1;
                                    *(s+6) = (*(s+6) << 1) | (data_word >> 25) & 1;
                                    *(s+7) = (*(s+7) << 1) | (data_word >> 24) & 1;
                                    s += 8;
                                    data_word <<= 8;
                                    data_bits -= 8;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;

                            case 2:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 2) | (data_word >> 30) & 3;
                                    *(s+1) = (*(s+1) << 2) | (data_word >> 28) & 3;
                                    *(s+2) = (*(s+2) << 2) | (data_word >> 26) & 3;
                                    *(s+3) = (*(s+3) << 2) | (data_word >> 24) & 3;
                                    *(s+4) = (*(s+4) << 2) | (data_word >> 22) & 3;
                                    *(s+5) = (*(s+5) << 2) | (data_word >> 20) & 3;
                                    *(s+6) = (*(s+6) << 2) | (data_word >> 18) & 3;
                                    *(s+7) = (*(s+7) << 2) | (data_word >> 16) & 3;
                                    s += 8;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;
                                    }
                                break;

                            case 3:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 3) | (data_word >> 29) & 7;
                                    *(s+1) = (*(s+1) << 3) | (data_word >> 26) & 7;
                                    *(s+2) = (*(s+2) << 3) | (data_word >> 23) & 7;
                                    *(s+3) = (*(s+3) << 3) | (data_word >> 20) & 7;
                                    *(s+4) = (*(s+4) << 3) | (data_word >> 17) & 7;
                                    s += 5;
                                    data_word <<= 15;
                                    data_bits -= 15;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 3) | (data_word >> 29) & 7;
                                    *(s+1) = (*(s+1) << 3) | (data_word >> 26) & 7;
                                    *(s+2) = (*(s+2) << 3) | (data_word >> 23) & 7;
                                    s += 3;
                                    data_word <<= 9;
                                    data_bits -= 9;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;

                            case 4:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 4) | (data_word >> 28) & 0xf;
                                    *(s+1) = (*(s+1) << 4) | (data_word >> 24) & 0xf;
                                    *(s+2) = (*(s+2) << 4) | (data_word >> 20) & 0xf;
                                    *(s+3) = (*(s+3) << 4) | (data_word >> 16) & 0xf;
                                    s += 4;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;

                                    *(s+0) = (*(s+0) << 4) | (data_word >> 28) & 0xf;
                                    *(s+1) = (*(s+1) << 4) | (data_word >> 24) & 0xf;
                                    *(s+2) = (*(s+2) << 4) | (data_word >> 20) & 0xf;
                                    *(s+3) = (*(s+3) << 4) | (data_word >> 16) & 0xf;
                                    s += 4;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;
                                    }
                                break;

                            case 5:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                                    *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                                    *(s+2) = (*(s+2) << 5) | (data_word >> 17) & 0x1f;
                                    s += 3;
                                    data_word <<= 15;
                                    data_bits -= 15;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                                    *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                                    *(s+2) = (*(s+2) << 5) | (data_word >> 17) & 0x1f;
                                    s += 3;
                                    data_word <<= 15;
                                    data_bits -= 15;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                                    *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                                    s += 2;
                                    data_word <<= 10;
                                    data_bits -= 10;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;

                            case 6:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                                    *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                                    s += 2;
                                    data_word <<= 12;
                                    data_bits -= 12;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                                    *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                                    s += 2;
                                    data_word <<= 12;
                                    data_bits -= 12;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                                    *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                                    s += 2;
                                    data_word <<= 12;
                                    data_bits -= 12;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                                    *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                                    s += 2;
                                    data_word <<= 12;
                                    data_bits -= 12;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;

                            case 7:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                                    *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                                    s += 2;
                                    data_word <<= 14;
                                    data_bits -= 14;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                                    *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                                    s += 2;
                                    data_word <<= 14;
                                    data_bits -= 14;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                                    *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                                    s += 2;
                                    data_word <<= 14;
                                    data_bits -= 14;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                                    *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                                    s += 2;
                                    data_word <<= 14;
                                    data_bits -= 14;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;

                            case 8:
                                while (s < end)
                                    {
                                    *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                                    *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                                    s += 2;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;

                                    *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                                    *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                                    s += 2;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;

                                    *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                                    *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                                    s += 2;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;

                                    *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                                    *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                                    s += 2;
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;
                                    }
                                break;

                            default:
                                shift = 32 - k_bits;
                                while (s < end)
                                    {
                                    *s = (*s << k_bits) | (data_word >> shift);
                                    s++;
                                    data_word <<= k_bits;
                                    data_bits -= k_bits;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *s = (*s << k_bits) | (data_word >> shift);
                                    s++;
                                    data_word <<= k_bits;
                                    data_bits -= k_bits;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                break;
                            }
                        else
                            {
                            extra = k_bits - 16;
                            if (extra <= 0)
                                {
                                shift = 32 - k_bits;
                                while (s < end)
                                    {
                                    *s = (*s << k_bits) | (data_word >> shift);
                                    s++;
                                    data_word <<= k_bits;
                                    data_bits -= k_bits;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }

                                    *s = (*s << k_bits) | (data_word >> shift);
                                    s++;
                                    data_word <<= k_bits;
                                    data_bits -= k_bits;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                }
                            else
                                {
                                while (s < end)
                                    {
                                    *s = (*s << 16) | (data_word >> 16);
                                    data_word <<= 16;
                                    data_bits -= 16;
                                    data_word |= *input_ptr++ << (16 - data_bits);
                                    data_bits += 16;

                                    *s = (*s << extra) | (data_word >> (32 - extra));
                                    s++;
                                    data_word <<= extra;
                                    data_bits -= extra;
                                    if (data_bits <= 16)
                                        {
                                        data_word |= *input_ptr++ << (16 - data_bits);
                                        data_bits += 16;
                                        }
                                    }
                                }
                            }
                    }
                }
            else if (id == ID_FS)
                {
                int big_zero_count;
                int zero_count;

                while (s < end)
                    {
                    big_zero_count = 0;
                    while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }
                        }

                    zero_count = leading_zeros[data_word >> 24];
                    data_word <<= zero_count+1;
                    data_bits -= zero_count+1;
                    if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                    *s++ = zero_count + big_zero_count;
                    }
                }
            else
                {
                if (ext2_bit)
                    {
                    int big_zero_count;
                    int m;
                    int zero_count;
                    unsigned *t;
                    unsigned *tend;
                    unsigned temp[MAX_PIXELS_PER_BLOCK];

                    /*** Note: The value, m, must be range checked, to avoid a segmentation violation ***/
                    /*** caused by the data being decoded and encoded with a different number of ***/
                    /*** pixels_per_scanline. ***/

                    /*** Read EXT2 FS values ***/
                    t = temp;
                    tend = temp + pixels_per_block/2;
                    while (t < tend)
                        {
                        big_zero_count = 0;
                        while ((data_word & 0xff000000) == 0)
                            {
                            big_zero_count += 8;
                            data_word <<= 8;
                            data_bits -= 8;
                            if (data_bits <= 16)
                                {
                                data_word |= *input_ptr++ << (16 - data_bits);
                                data_bits += 16;
                                }
                            }

                        zero_count = leading_zeros[data_word >> 24];
                        data_word <<= zero_count+1;
                        data_bits -= zero_count+1;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }

                        *t++ = zero_count + big_zero_count;
                        }

                    t = temp;
                    while (s < end)
                        {
                        m = *t++;
                        if (m >= MAX_EXT2_SUM)
                            m = 0;

                        *s++ = ext2_array1[m];
                        *s++ = ext2_array2[m];
                        }
                    }
                else
                    {
                    int bits;
                    int big_zero_count;
                    int zero_count;

                    big_zero_count = 0;
                    while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                            {
                            data_word |= *input_ptr++ << (16 - data_bits);
                            data_bits += 16;
                            }
                        }

                    zero_count = leading_zeros[data_word >> 24];
                    data_word <<= zero_count+1;
                    data_bits -= zero_count+1;
                    if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                    bits = zero_count + big_zero_count + 1;

                    if (bits < 5)
                        zero_blocks = bits;
                    else if (bits == 5)
                        {
                        /*** Zero blocks till the end of the scanline or to the nearest scanline block that is ***/
                        /*** a multiple of MAX_ZERO_BLOCKS. ***/
                        /*** Hardware is limited to 64 zero blocks and 64 blocks per scanline ***/
                        zero_blocks = MAX_ZERO_BLOCKS - ((i-1) & (MAX_ZERO_BLOCKS-1));
                        if (blocks_per_scanline - (i-1) < zero_blocks)
                            zero_blocks = blocks_per_scanline - (i-1);
                        }
                    else
                        zero_blocks = bits - 1;

                    i += zero_blocks-1;
                    if (i > blocks_per_scanline)
                        {
                        error("Decoded more blocks than in scanline.  Check -s value.\n");
                        return;
                        }

                    end += (zero_blocks-1) * pixels_per_block;
                    memset(s, 0, (end-s)*sizeof(int));
                    }
                }

            s = end;
            end += pixels_per_block;
            }

        output_decoded_data(sigma);
        }

#if !HDF
    flush_decoded_buffer();
#endif /* !HDF */
}

/****************************************************************
* Must be external to be linked to HDF code.                    *
* This function calls the proper function to encode byte, word, *
* long, float, and double data types.                           *
* Valid bits_per_pixel are: 1-24,32,64.                         *
****************************************************************/
long
uncompress_memory(new_options_mask, new_bits_per_pixel, new_pixels_per_block, new_pixels_per_scanline, in, in_bytes, out, out_pixels)
int new_options_mask;
int new_bits_per_pixel;
int new_pixels_per_block;
int new_pixels_per_scanline;
const char *in;
long in_bytes;
void *out;
long out_pixels;
{
    static char *interleave_array;
    long bytes_written;
    long out_bytes;

    /*** reset error_count; if non zero an error occured during decompression ***/
    error_count = 0;
    warning_count = 0;

    compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
    msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
    raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

    bits_per_pixel = new_bits_per_pixel;
    pixels_per_block = new_pixels_per_block;
    pixels_per_scanline = new_pixels_per_scanline;

#if !HDF
    input_mode  = MEMORY_DATA;
    output_mode = MEMORY_DATA;
#endif /* !HDF */

    input_byte_data = (unsigned char *) in;
    input_byte_count = in_bytes;
#if !HDF
    strcpy(input_file_name, "*memory*");
#endif /* !HDF */

    if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
        {
        if (interleave_array)
            free(interleave_array);

        out_bytes = out_pixels * (new_bits_per_pixel >> 3);
        interleave_array = (char *) malloc(out_bytes);
        if (interleave_array == 0)
            {
            error("Out of Memory.\n");
            return MEMORY_ERROR;
            }

        bits_per_pixel = 8;
        }

    decode_initialize();
#if !HDF
    /*** For HDF typically there is no header information ***/
    /*** This function only uncompresses out_pixels of the image ***/
    if (output_pixel_count != 0x7fffffff)
        {
        /*** output_pixel_count read from header ***/
        if (output_pixel_count > out_pixels)
            {
            error("Allocated memory for decoded output is %ld pixels too small.\n", output_pixel_count - out_pixels);
            return PARAM_ERROR;
            }
        }
#endif /* !HDF */

    if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
        {
        bptr = interleave_array;
        output_pixel_count = out_pixels * (new_bits_per_pixel >> 3);
        }
    else
        {
        bptr = out;
        output_pixel_count = out_pixels;
        }

    rice_decode();
    if (error_count)
        return PARAM_ERROR;

    if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
        {
        bytes_written = bptr - interleave_array;
        deinterleave(interleave_array, bytes_written, new_bits_per_pixel, out);
        }
    else
        bytes_written = bptr - (char *) out;

    return bytes_written;
}

#if !HDF
static void
alternate_decoding(argc, argv)
int argc;
char **argv;
{
    struct stat stat_buffer;
    boolean chip;
    boolean msb;
    boolean nn;
    boolean raw;
    char file_name[128];
    char *in;
    char *out;
    int j;
    int option_mask;
    int n;
    int s;
    long pixels;
    long in_bytes;
    long out_bytes;
    long size;

    if (argc != 10)
        {
        fprintf(stderr, "Usage: %s file pixels raw chip nn msb n j s\n", argv[0]);
        exit(1);
        }

    sprintf(file_name, "%s.sz", argv[1]);
    if ((fp_in = fopen(file_name, "rb")) == 0)
        {
        fprintf(stderr, "Could not open input file: %s.sz\n", argv[1]);
        exit(1);
        }

    pixels = atoi(argv[2]);
    raw    = atoi(argv[3]);
    chip   = atoi(argv[4]);
    nn     = atoi(argv[5]);
    msb    = atoi(argv[6]);
    n      = atoi(argv[7]);
    j      = atoi(argv[8]);
    s      = atoi(argv[9]);

    out_bytes = pixels;
    if (n == 64)
        out_bytes *= 8;
    else if (n == 32)
        out_bytes *= 4;
    else if (n > 16)
        out_bytes *= 4;
    else if (n > 8)
        out_bytes *= 2;

    printf("out_bytes = %ld\n", out_bytes);
    if (fstat(fileno(fp_in), &stat_buffer) != 0)
        {
        fprintf(stderr, "Unable to fstat() input file: %s\n", argv[1]);
        exit(1);
        }

    option_mask = 0;
    if (raw)
        option_mask |= RAW_OPTION_MASK;

    if (nn)
        option_mask |= NN_OPTION_MASK;

    if (msb)
        option_mask |= MSB_OPTION_MASK;

    verbose_mode = 1;
#if 0
    orig_n = n;
    if (n == 32 || n == 64)
        n = 8;
#endif

    in_bytes = stat_buffer.st_size;
    in = (char *) malloc(in_bytes);
    out = (char *) malloc(out_bytes + 32);
    fread(in, 1, in_bytes, fp_in);
    fclose(fp_in);
    size = uncompress_memory(option_mask, n, j, s, in, in_bytes, out, pixels);
#if 0
    if (orig_n == 32 || orig_n == 64)
        {
        printf("deinterleaving: ...\n");
        interleave_array = malloc(out_bytes);
        deinterleave(out, out_bytes, orig_n, interleave_array);
        out = interleave_array;
        }
#endif

    sprintf(file_name, "%s.sd", argv[1]);
    if ((fp_out = fopen(file_name, "wb")) == 0)
        {
        fprintf(stderr, "Unable to open output file: %s\n", file_name);
        exit(1);
        }

    fwrite(out, 1, size, fp_out);
    fclose(fp_out);

#if 0
    if (interleave_array)
        free(interleave_array);
#endif

    exit(0);

    if (chip)
        ;    /*** removes chip not used warning ***/
}

static void
alternate_decoding2(argc, argv)
int argc;
char **argv;
{
    struct stat stat_buffer;
    char file_name[128];
    char *in;
    char *out;
    int i;
    int iterations;
    long in_bytes;
    long out_bytes;
    long size;

    if (argc == 2)
        iterations = 1;
    else if (argc == 3)
        iterations = atoi(argv[2]);
    else
        {
        fprintf(stderr, "Usage: %s file [iterations]\n", argv[0]);
        exit(1);
        }

    sprintf(file_name, "%s.sz", argv[1]);
    if ((fp_in = fopen(file_name, "rb")) == 0)
        {
        fprintf(stderr, "Could not open input file: %s\n", argv[1]);
        exit(1);
        }

    if (fstat(fileno(fp_in), &stat_buffer) != 0)
        {
        fprintf(stderr, "Unable to fstat() input file: %s\n", argv[1]);
        exit(1);
        }

    verbose_mode = 1;

    in_bytes = stat_buffer.st_size;
    out_bytes = 512*512;
#if 0
    out_bytes = in_bytes * 3.0;
#endif
    in = (char *) malloc(in_bytes);
    out = (char *) malloc(out_bytes);
    fread(in, 1, in_bytes, fp_in);
    fclose(fp_in);
    for (i = 0; i < iterations; i++)
        size = uncompress_memory(0, 8, 16, 1000, in, in_bytes, out, out_bytes);

    sprintf(file_name, "%s.sd", argv[1]);
    if ((fp_out = fopen(file_name, "wb")) == 0)
        {
        fprintf(stderr, "Unable to open output file: %s\n", file_name);
        exit(1);
        }

    fwrite(out, 1, size, fp_out);
    fclose(fp_out);
    exit(0);
}

static void
post_decode()
{
    if (ferror(fp_in))
        {
        fclose(fp_in);
        fclose(fp_out);
        remove(output_file_name);
        error("Input file read failed.\n");
        return;
        }

    fclose(fp_in);
    if (ferror(fp_out))
        {
        fclose(fp_out);
        remove(output_file_name);
        error("Output file write failed: %s\n", output_file_name);
        return;
        }

    fclose(fp_out);
    if (!keep_image_file && fp_out != stdout)
        {
        if (rename(output_file_name, base_file_name) != 0)
            {
            error("Cannot rename to file: %s\n", base_file_name);
            remove(output_file_name);
            return;
            }
        }

    if (!keep_compressed_file && fp_in != stdin)
        remove(input_file_name);
}
#endif /* !HDF */

int
check_params(bits_per_pixel, pixels_per_block, pixels_per_scanline, image_pixels, msg)
int bits_per_pixel;
int pixels_per_block;
int pixels_per_scanline;
long image_pixels;
char **msg;
{
    if (bits_per_pixel >= 1 && bits_per_pixel <= 24)
        ;
    else if (bits_per_pixel == 32 || bits_per_pixel == 64)
        ;
    else
        {
        *msg = "bits per pixel must be in range 1..24,32,64";
        return 0;
        }

    if (pixels_per_block > MAX_PIXELS_PER_BLOCK)
        {
        *msg = "maximum pixels per block exceeded";
        return 0;
        }

    if (pixels_per_block & 1)
        {
        *msg = "pixels per block must be even";
        return 0;
        }

    if (pixels_per_block > pixels_per_scanline)
        {
        *msg = "pixels per block > pixels per scanline";
        return 0;
        }

    if (pixels_per_scanline > MAX_PIXELS_PER_SCANLINE)
        {
        *msg = "maximum pixels per scanline exceeded";
        return 0;
        }

    if (image_pixels < pixels_per_scanline)
        {
        *msg = "image pixels less than pixels per scanline";
        return 0;
        }

    return 1;
}

#if HDF

/*********************************************************************/
/*** Make sure that the defines in ricehdf.h match those in rice.h ***/
/*********************************************************************/

#if ALLOW_K13_OPTION_MASK != SZ_ALLOW_K13_OPTION_MASK
#error "define ALLOW_K13_OPTION_MASK != SZ_ALLOW_K13_OPTION_MASK"
#endif

#if CHIP_OPTION_MASK != SZ_CHIP_OPTION_MASK
#error "define CHIP_OPTION_MASK != SZ_CHIP_OPTION_MASK"
#endif

#if EC_OPTION_MASK != SZ_EC_OPTION_MASK
#error "define EC_OPTION_MASK != SZ_EC_OPTION_MASK"
#endif

#if LSB_OPTION_MASK != SZ_LSB_OPTION_MASK
#error "define LSB_OPTION_MASK != SZ_LSB_OPTION_MASK"
#endif

#if MSB_OPTION_MASK != SZ_MSB_OPTION_MASK
#error "define MSB_OPTION_MASK != SZ_MSB_OPTION_MASK"
#endif

#if NN_OPTION_MASK != SZ_NN_OPTION_MASK
#error "define NN_OPTION_MASK  != SZ_NN_OPTION_MASK "
#endif

#if RAW_OPTION_MASK != SZ_RAW_OPTION_MASK
#error "define NN_RAW_OPTION_MASK != SZ_RAW_OPTION_MASK "
#endif

#if MEMORY_ERROR != SZ_MEM_ERROR
#error "define MEMORY_ERROR != SZ_MEM_ERROR"
#endif

#if PARAM_ERROR != SZ_PARAM_ERROR
#error "define PARAM_ERROR != SZ_PARAM_ERROR"
#endif

#if NO_ENCODER_ERROR != SZ_NO_ENCODER_ERROR
#error "define NO_ENCODER_ERROR != SZ_NO_ENCODER_ERROR"
#endif

#if MAX_BLOCKS_PER_SCANLINE != SZ_MAX_BLOCKS_PER_SCANLINE
#error "define MAX_BLOCKS_PER_SCANLINE != SZ_MAX_BLOCKS_PER_SCANLINE"
#endif

#if MAX_PIXELS_PER_BLOCK != SZ_MAX_PIXELS_PER_BLOCK
#error "define MAX_PIXELS_PER_BLOCK != SZ_MAX_PIXELS_PER_BLOCK"
#endif

#if MAX_PIXELS_PER_SCANLINE != SZ_MAX_PIXELS_PER_SCANLINE
#error "define MAX_PIXELS_PER_SCANLINE != SZ_MAX_PIXELS_PER_SCANLINE"
#endif

#endif /*** HDF ***/

#if !HDF
int main(argc, argv)
int argc;
char *argv[];
{
    char *name;
    char *s;
    int i;
#ifndef REMOVE_SZIP_ENCODER
    int len;
    long bytes_read;
#endif /* !defined(REMOVE_SZIP_ENCODER) */

#if 1
#ifndef REMOVE_SZIP_ENCODER
    if (eq(argv[0], "szipa"))
        alternate_encoding(argc, argv);

    if (eq(argv[0], "szipf"))
        alternate_encoding2(argc, argv);
#endif /* !defined(REMOVE_SZIP_ENCODER) */

    if (eq(argv[0], "sunzipa"))
        alternate_decoding(argc, argv);

    if (eq(argv[0], "sunzipt"))
        alternate_decoding2(argc, argv);
#endif

    name = argv[0];
#if defined(MSDOS) || defined(WINDOWS95)
    s = name;
    while (*s)
        {
        if (*s == '\\')
            *s = '/';

        if (isupper(*s))
            *s = tolower(*s);

        s++;
        }

    len = strlen(name);
    if (len >= 4)
        {
        s = name + strlen(name) - 4;
        if (eq(s, ".exe"))
            *s = 0;
        }
#endif /* defined(MSDOS) || defined(WINDOWS95) */

    s = name + strlen(name);
    while (s > name && *s != '/')
        s--;

    if (*s == '/')
        s++;

    if (eq(s, SZIP_PROGRAM_NAME))
        encoding = TRUE;
    else if (eq(s, SUNZIP_PROGRAM_NAME))
        {
#ifdef REMOVE_SZIP_ENCODER
        error("This executable has szip encoding compiled out.\n");
        exit(1);
#endif /* defined(REMOVE_SZIP_ENCODER) */
        encoding = FALSE;
        }
    else
        {
        fprintf(stderr, "ERROR: Executable name does not match known encoder or decoder name.\n");
        exit(1);
        }

    input_mode  = FILE_DATA;
    output_mode = FILE_DATA;
    set_defaults();
    parse_args(argc, argv);

    if (encoding)
        {
#ifdef REMOVE_SZIP_ENCODER
        error("This executable has szip encoding compiled out.\n");
        return 1;
#else
        if (file_count == 0)
            {
            strcpy(input_file_name, "stdin");
            strcpy(output_file_name, "stdout");
            fp_in = stdin;
            fp_out = stdout;
            binary_mode();

            global_bptr = output_buffer;
            encode_initialize();
            check_args();
            if (error_count)
                exit(1);

            bytes_read = rice_encode();
            post_encode();
            if (error_count)
                exit(1);

            if (verbose_mode)
                print_stats(bytes_read, total_coded_bytes);
            }

        for (i = 0; i < file_count; i++)
            {
            len = strlen(file_array[i]);
            if (len > 3 && eqn(file_array[i] + len - 3, ".sz", 3))
                {
                warning("File already compressed: %s.\n", file_array[i]);
                continue;
                }

            open_encoder_files(file_array[i]);
            if (error_count)
                exit(1);

            global_bptr = output_buffer;
            encode_initialize();
            check_args();
            if (error_count)
                exit(1);

            bytes_read = rice_encode();
            post_encode();
            if (error_count)
                exit(1);

            if (verbose_mode)
                print_stats(bytes_read, total_coded_bytes);
            }
#endif /* defined(REMOVE_SZIP_ENCODER) */
        }
    else
        {
        if (file_count == 0)
            {
            strcpy(input_file_name, "stdin");
            strcpy(output_file_name, "stdout");
            fp_in = stdin;
            fp_out = stdout;
            binary_mode();

            decode_initialize();
            if (error_count)
                exit(1);

            rice_decode();
            post_decode();
            if (error_count)
                exit(1);
            }

        for (i = 0; i < file_count; i++)
            {
            open_decoder_files(file_array[i]);
            if (error_count)
                exit(1);

            decode_initialize();
            if (error_count)
                exit(1);

            rice_decode();
            post_decode();
            if (error_count)
                exit(1);
            }
        }

    return 0;
}
#endif /* !HDF */
