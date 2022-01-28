/************************** Start of COMPAND.C *************************
 *
 * This is the companding module used in Chapter 10.
 * Compile with BITIO.C, ERRHAND.C, and either MAIN-C.C or MAIN-E.C
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "mn_incs.h"

/*
 * These two strings are used by MAIN-C.C and MAIN-E.C to print
 * messages of importance to the user of the program.
 */
//char *CompressionName = "Sound sample companding";
//char *Usage =
//"infile outfile [n]\n\n n optionally sets the bits per sample\n\n";

#ifdef __STDC__

long compand_get_file_length( memory_file *file );

#else

long compand_get_file_length();

#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/*
 * The compression routine runs very quickly, since all it does is
 * perform a table lookup on each input byte.  The first part of the
 * routine builds that table.  After that it is just a matter of
 * reading bytes in and writing out the compressed value.
 *
 * Unlike all of the other compression routines in the book, this
 * routine does not have a special END_OF_STREAM code.  With as few
 * as 4 or 8 codes, they seem to precious to give one up.  Instead,
 * the file length is written out at the very start of the compressed
 * file, along with the number of bits used to encode the data.
 */
/*
void CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    int compress[ 256 ];
    int steps;
    int bits;
    int value;
    int i;
    int j;
    int c;

    if ( argc-- > 0 )
        bits = atoi( *argv );
    else
        bits = 4;
    printf( "Compressing using %d bits per sample...\n", bits );
    steps = ( 1 << ( bits - 1 ) );
    OutputBits( output, (unsigned long) bits, 8 );
    OutputBits( output, (unsigned long) compand_get_file_length( input ), 32 );

    for ( i = steps ; i > 0; i-- ) {
        value = (int)
           ( 128.0 * ( pow( 2.0, (double) i  /  steps ) - 1.0 ) + 0.5 );
        for ( j = value ; j > 0 ; j-- ) {
            compress[ j + 127 ] = i + steps - 1;
            compress[ 128 - j ] = steps - i;
        }
    }

    while ( ( c = mn_getc( input ) ) != EOF )
        OutputBits( output, (unsigned long) compress[ c ], bits );
}
*/
/*
 * The expansion routine gets the number of bits per code from the
 * compressed file, then builds an expansion table.  Each of the
 * "steps" codes expands to a unique eight bit code that lies on
 * the exponential encoding curve.
 */

QUICK_EXPAND(compand)
    int steps;
    int bits;
    int value;
    int last_value;
    int i;
    int c;
    long count;
    int expand[ 256 ];

/*
 * First this routine reads in the number of bits, then builds the
 * expansion table.  Once the table is built, expanding the file
 * is simply a matter of performing a table lookup on each code.
 */
    bits = (int) InputBits( input, 8 );
    printf( "Expanding using %d bits per sample...\n", bits );
    steps = ( 1 << ( bits - 1 ) );
    last_value = 0;
    for ( i = 1; i <= steps; i++ ) {
        value = (int)
            ( 128.0 * ( pow( 2.0, (double) i  /  steps ) - 1.0 ) + 0.5 );
        expand[ steps + i - 1 ] = 128 + ( value + last_value ) / 2;
        expand[ steps - i ] = 127 - ( value + last_value ) / 2;
        last_value = value;
    }
/*
 * The actual file size is stored at the start of the compressed file.
 * It is read in to determine how many codes need to be expanded.  Once
 * that is done, expansion takes place rapidly.
 */
    for ( count = InputBits( input, 32 ) ; count > 0 ; count-- ) {
        c = (int) InputBits( input, bits );
        mn_putc( expand[ c ], &output );
    }
    while ( argc-- > 0 )
        printf( "Unused argument: %s\n", *argv++ );
    CloseInputBitFile(input);
    return(output.data - out);
}

/*
 * This utility routine is used to determine the size of the input file.
 */

long compand_get_file_length( file )
memory_file *file;
{
    long marker;
    long eof_ftell;

    marker = file->data - file->datas;  //ftell( file );
    file->data = file->datal;           //fseek( file, 0L, SEEK_END );
    eof_ftell = file->data - file->datas;   //ftell( file );
    file->data = file->datas + marker;  //fseek( file, marker, SEEK_SET );
    return( eof_ftell - marker );
}

/*************************** End of COMPAND.C **************************/
