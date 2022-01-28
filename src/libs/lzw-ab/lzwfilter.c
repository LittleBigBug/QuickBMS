////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                     Copyright (c) 2016 David Bryant                    //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#endif

#include "lzw-lib.h"

/* This module provides a command-line filter for testing the lzw library.
 * It can also optionally calculate and display the compression ratio and
 * a simple checksum for informational purposes. Other command-line
 * arguments select decoding mode or the maximum symbol size (9 to 12 bits)
 * for encoding.
 */

static const char *usage =
" Usage:     LZW-AB [-options] [< infile] [> outfile]\n\n"
" Operation: compression is default, use -d to decompress\n\n"
" Options:  -d     = decompress\n"
"           -h     = display this \"help\" message\n"
"           -1     = maximum symbol size = 9 bits\n"
"           -2     = maximum symbol size = 10 bits\n"
"           -3     = maximum symbol size = 11 bits\n"
"           -4     = maximum symbol size = 12 bits (default)\n"
"           -v     = verbose (display ratio and checksum)\n\n"
" Web:       Visit www.github.com/dbry/lzw-ab for latest version and info\n\n";

static unsigned char read_buffer [65536], write_buffer [65536];
static size_t read_count, write_count;
static int read_checksum, write_checksum;
static int read_head, read_tail, write_head;

static int read_buff (void)
{
    int value;

    if (read_head == read_tail)
        read_tail = (read_head = 0) + fread (read_buffer, 1, sizeof (read_buffer), stdin);

    if (read_head < read_tail) {
        value = read_buffer [read_head++];
        read_checksum = read_checksum * 3 + (unsigned char) value;
        read_count++;
    }
    else
        value = EOF;

    return value;
}

static void write_buff (int value)
{
    if (value == EOF) {
        fwrite (write_buffer, 1, write_head, stdout);
        return;
    }

    write_buffer [write_head++] = value;

    if (write_head == sizeof (write_buffer)) {
        fwrite (write_buffer, 1, write_head, stdout);
        write_head = 0;
    }

    write_checksum = write_checksum * 3 + (unsigned char) value;
    write_count++;
}

int main (int argc, char **argv)
{
    int decompress = 0, maxbits = 12, verbose = 0, error = 0;

    read_checksum = write_checksum = -1;

    while (--argc) {
        if ((**++argv == '-') && (*argv)[1])
            while (*++*argv)
                switch (**argv) {
                    case '1':
                        maxbits = 9;
                        break;

                    case '2':
                        maxbits = 10;
                        break;

                    case '3':
                        maxbits = 11;
                        break;

                    case '4':
                        maxbits = 12;
                        break;

                    case 'D': case 'd':
                        decompress = 1;
                        break;

                    case 'H': case 'h':
                        fprintf (stderr, "%s", usage);
                        return 0;
                        break;

                    case 'V': case 'v':
                        verbose = 1;
                        break;

                    default:
                        fprintf (stderr, "illegal option: %c !\n", **argv);
                        error = 1;
                        break;
                }
        else {
           fprintf (stderr, "unknown argument: %s\n", *argv);
           error = 1;
        }
    }

    if (error) {
        fprintf (stderr, "%s", usage);
        return 0;
    }

#ifdef _WIN32
    setmode (fileno (stdin), O_BINARY);
    setmode (fileno (stdout), O_BINARY);
#endif

    if (decompress) {
        if (lzw_decompress (write_buff, read_buff)) {
            fprintf (stderr, "lzw_decompress() returned non-zero!\n");
            return 1;
        }

        write_buff (EOF);
            
        if (verbose && write_count)
            fprintf (stderr, "output checksum = %x, ratio = %.2f%%\n", write_checksum, read_count * 100.0 / write_count);
    }
    else {
        if (lzw_compress (write_buff, read_buff, maxbits)) {
            fprintf (stderr, "lzw_compress() returned non-zero!\n");
            return 1;
        }

        write_buff (EOF);

        if (verbose && read_count)
            fprintf (stderr, "source checksum = %x, ratio = %.2f%%\n", read_checksum, write_count * 100.0 / read_count);
    }

    return 0;
}
