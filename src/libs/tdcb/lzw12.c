/************************** Start of LZW12.C *************************
 *
 * This is 12 bit LZW program, which is discussed in the first part
 * of the chapter.  It uses a fixed size code, and does not attempt
 * to flush the dictionary after it fills up.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mn_incs.h"

/*
 * Constants used throughout the program.  BITS defines how many bits
 * will be in a code.  TABLE_SIZE defines the size of the dictionary
 * table.
 */
#define BITS                       12
#define MAX_CODE                   ( ( 1 << BITS ) - 1 )
#define TABLE_SIZE                 5021
#define END_OF_STREAM              256
#define FIRST_CODE                 257
#define UNUSED                     -1

/*
 * Local prototypes.
 */

#ifdef __STDC__
unsigned int lzw12_find_child_node( int parent_code, int child_character );
unsigned int lzw12_decode_string( unsigned int offset, unsigned int code );
#else
unsigned int lzw12_find_child_node();
unsigned int lzw12_decode_string();
#endif

//char *CompressionName = "LZW 12 Bit Encoder";
//char *Usage           = "in-file out-file\n\n";

/*
 * This data structure defines the dictionary.  Each entry in the dictionary
 * has a code value.  This is the code emitted by the compressor.  Each
 * code is actually made up of two pieces:  a parent_code, and a
 * character.  Code values of less than 256 are actually plain
 * text codes.
 */

#define dict    lzw12_dict
struct dictionary {
    int code_value;
    int parent_code;
    char character;
} dict[ TABLE_SIZE ];

#define decode_stack    lzw12_decode_stack
char decode_stack[ TABLE_SIZE ];

/*
 * The compressor is short and simple.  It reads in new symbols one
 * at a time from the input file.  It then  checks to see if the
 * combination of the current symbol and the current code are already
 * defined in the dictionary.  If they are not, they are added to the
 * dictionary, and we start over with a new one symbol code.  If they
 * are, the code for the combination of the code and character becomes
 * our new code.
 */
/*
void lzw12_CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    int next_code;
    int character;
    int string_code;
    unsigned int index;
    unsigned int i;

    next_code = FIRST_CODE;
    for ( i = 0 ; i < TABLE_SIZE ; i++ )
        dict[ i ].code_value = UNUSED;
    if ( ( string_code = mn_getc( input ) ) == EOF )
        string_code = END_OF_STREAM;
    while ( ( character = mn_getc( input ) ) != EOF ) {
        index = lzw12_find_child_node( string_code, character );
        if ( dict[ index ].code_value != -1 )
            string_code = dict[ index ].code_value;
        else {
            if ( next_code <= MAX_CODE ) {
                dict[ index ].code_value = next_code++;
                dict[ index ].parent_code = string_code;
                dict[ index ].character = (char) character;
            }
            OutputBits( output, (unsigned long) string_code, BITS );
            string_code = character;
        }
    }
    OutputBits( output, (unsigned long) string_code, BITS );
    OutputBits( output, (unsigned long) END_OF_STREAM, BITS );
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
}*/

/*
 * The file expander operates much like the encoder.  It has to
 * read in codes, the convert the codes to a string of characters.
 * The only catch in the whole operation occurs when the encoder
 * encounters a CHAR+STRING+CHAR+STRING+CHAR sequence.  When this
 * occurs, the encoder outputs a code that is not presently defined
 * in the table.  This is handled as an exception.
 */
QUICK_EXPAND(lzw12)
    unsigned int next_code;
    unsigned int new_code;
    unsigned int old_code;
    int character;
    unsigned int count;

    next_code = FIRST_CODE;
    old_code = (unsigned int) InputBits( input, BITS );
    if ( old_code == END_OF_STREAM )
        goto quit;
    character = old_code;
    mn_putc( old_code, &output );

    while ( ( new_code = (unsigned int) InputBits( input, BITS ) )
              != END_OF_STREAM ) {
/*
** This code checks for the CHARACTER+STRING+CHARACTER+STRING+CHARACTER
** case which generates an undefined code.  It handles it by decoding
** the last code, and adding a single character to the end of the decode string.
*/
        if ( new_code >= next_code ) {
            decode_stack[ 0 ] = (char) character;
            count = lzw12_decode_string( 1, old_code );
        }
        else
            count = lzw12_decode_string( 0, new_code );
        character = decode_stack[ count - 1 ];
        while ( count > 0 )
            mn_putc( decode_stack[ --count ], &output );
        if ( next_code <= MAX_CODE ) {
            dict[ next_code ].parent_code = old_code;
            dict[ next_code ].character = (char) character;
            next_code++;
        }
        old_code = new_code;
    }
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
quit:
    CloseInputBitFile(input);
    return(output.data - out);
}

/*
 * This hashing routine is responsible for finding the table location
 * for a string/character combination.  The table index is created
 * by using an exclusive OR combination of the prefix and character.
 * This code also has to check for collisions, and handles them by
 * jumping around in the table.
 */
unsigned int lzw12_find_child_node( parent_code, child_character )
int parent_code;
int child_character;
{
    int index;
    int offset;

    index = ( child_character << ( BITS - 8 ) ) ^ parent_code;
    if ( index == 0 )
        offset = 1;
    else
        offset = TABLE_SIZE - index;
    for ( ; ; ) {
        if ( dict[ index ].code_value == UNUSED )
            return( index );
        if ( dict[ index ].parent_code == parent_code &&
             dict[ index ].character == (char) child_character )
            return( index );
        index -= offset;
        if ( index < 0 )
            index += TABLE_SIZE;
    }
}

/*
 * This routine decodes a string from the dictionary, and stores it
 * in the decode_stack data structure.  It returns a count to the
 * calling program of how many characters were placed in the stack.
 */

unsigned int lzw12_decode_string( count, code )
unsigned int count;
unsigned int code;
{
    while ( code > 255 ) {
        decode_stack[ count++ ] = dict[ code ].character;
        code = dict[ code ].parent_code;
    }
    decode_stack[ count++ ] = (char) code;
    return( count );
}

/************************** End of LZW12.C *************************/

