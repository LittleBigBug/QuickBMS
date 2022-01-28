/************************** Start of lzw15vV.C *************************
 *
 * This is the LZW module which implements a more powerful version
 * of the algorithm.  This version of the program has three major
 * improvements over LZW12.C.  First, it expands the maximum code size
 * to 15 bits.  Second, it starts encoding with 9 bit codes, working
 * its way up in bit size only as necessary.  Finally, it flushes the
 * dictionary when done.
 *
 * Note that under MS-DOS this program needs to be built using the
 * Compact or Large memory model.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mn_incs.h"

/*
 * Constants used throughout the program.  BITS defines the maximum
 * number of bits that can be used in the output code.  TABLE_SIZE defines
 * the size of the dictionary table.  TABLE_BANKS are the number of
 * 256 element dictionary pages needed.  The code defines should be
 * self-explanatory.
 */

#define BITS                       15
#define MAX_CODE                   ( ( 1 << BITS ) - 1 )
#define TABLE_SIZE                 35023L
#define TABLE_BANKS                ( ( TABLE_SIZE >> 8 ) + 1 )
#define END_OF_STREAM              256
#define BUMP_CODE                  257
#define FLUSH_CODE                 258
#define FIRST_CODE                 259
#define UNUSED                     -1

/*
 * Local prototypes.
 */

#ifdef __STDC__
unsigned int lzw15v_find_child_node( int parent_code, int child_character );
unsigned int lzw15v_decode_string( unsigned int offset, unsigned int code );
#else
unsigned int lzw15v_find_child_node();
unsigned int lzw15v_decode_string();
#endif


//char *CompressionName = "LZW 15 Bit Variable Rate Encoder";
//char *Usage           = "in-file out-file\n\n";

/*
 * This data structure defines the dictionary.  Each entry in the dictionary
 * has a code value.  This is the code emitted by the compressor.  Each
 * code is actually made up of two pieces:  a parent_code, and a
 * character.  Code values of less than 256 are actually plain
 * text codes.
 *
 * Note that in order to handle 16 bit segmented compilers, such as most
 * of the MS-DOS compilers, it was necessary to break up the dictionary
 * into a table of smaller dictionary pointers.  Every reference to the
 * dictionary was replaced by a macro that did a pointer dereference first.
 * By breaking up the index along byte boundaries we should be as efficient
 * as possible.
 */

#define dict    lzw15v_dict
static struct dictionary {
    int code_value;
    int parent_code;
    char character;
} *dict[ TABLE_BANKS ] = {NULL};

#define DICT( i ) dict[ i >> 8 ][ i & 0xff ]

/*
 * Other global data structures.  The decode_stack is used to reverse
 * strings that come out of the tree during decoding.  next_code is the
 * next code to be added to the dictionary, both during compression and
 * decompression.  current_code_bits defines how many bits are currently
 * being used for output, and next_bump_code defines the code that will
 * trigger the next jump in word size.
 */

#define decode_stack        lzw15v_decode_stack
char decode_stack[ TABLE_SIZE ];
#define next_code           lzw15v_next_code
unsigned int next_code;
#define current_code_bits   lzw15v_current_code_bits
int current_code_bits;
#define next_bump_code      lzw15v_next_bump_code
unsigned int next_bump_code;

/*
 * This routine is used to initialize the dictionary, both when the
 * compressor or decompressor first starts up, and also when a flush
 * code comes in.  Note that even thought the decompressor sets all
 * the code_value elements to UNUSED, it doesn't really need to.
 */

void lzw15v_InitializeDictionary()
{
    unsigned int i;

    for ( i = 0 ; i < TABLE_SIZE ; i++ )
        DICT( i ).code_value = UNUSED;
    next_code = FIRST_CODE;
    //putc( 'F', stdout );
    current_code_bits = 9;
    next_bump_code = 511;
}

/*
 * This routine allocates the dictionary.  Since the total size of the
 * dictionary is much larger than 64K, it can't be allocated as a single
 * object.  Instead, it is allocated as a set of pointers to smaller
 * dictionary objects.  The special DICT() macro is used to translate
 * indices into pairs of references.
 */

void lzw15v_InitializeStorage()
{
    int i;

    for ( i = 0 ; i < TABLE_BANKS ; i++ ) {
        if(dict[i]) memset(dict[i], 0, 256 * sizeof ( struct dictionary ));
        else
        dict[ i ] = (struct dictionary *)
                     calloc( 256, sizeof ( struct dictionary ) );
        if ( dict[ i ] == NULL )
            fatal_error( "Error allocating dictionary space" );
    }
}

/*
 * The compressor is short and simple.  It reads in new symbols one
 * at a time from the input file.  It then  checks to see if the
 * combination of the current symbol and the current code are already
 * defined in the dictionary.  If they are not, they are added to the
 * dictionary, and we start over with a new one symbol code.  If they
 * are, the code for the combination of the code and character becomes
 * our new code.  Note that in this enhanced version of LZW, the
 * encoder needs to check the codes for boundary conditions.
 */
/*
void CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    int character;
    int string_code;
    unsigned int index;

    lzw15v_InitializeStorage();
    lzw15v_InitializeDictionary();
    if ( ( string_code = mn_getc( input ) ) == EOF )
        string_code = END_OF_STREAM;
    while ( ( character = mn_getc( input ) ) != EOF ) {
        index = lzw15v_find_child_node( string_code, character );
        if ( DICT( index ).code_value != - 1 )
            string_code = DICT( index ).code_value;
        else {
            DICT( index ).code_value = next_code++;
            DICT( index ).parent_code = string_code;
            DICT( index ).character = (char) character;
            OutputBits( output,
                        (unsigned long) string_code, current_code_bits );
            string_code = character;
            if ( next_code > MAX_CODE ) {
                OutputBits( output,
                            (unsigned long) FLUSH_CODE, current_code_bits );
                lzw15v_InitializeDictionary();
            } else if ( next_code > next_bump_code ) {
                OutputBits( output,
                            (unsigned long) BUMP_CODE, current_code_bits );
                current_code_bits++;
                next_bump_code <<= 1;
                next_bump_code |= 1;
                //putc( 'B', stdout );
            }
        }
    }
    OutputBits( output, (unsigned long) string_code, current_code_bits );
    OutputBits( output, (unsigned long) END_OF_STREAM, current_code_bits);
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
}*/

/*
 * The file expander operates much like the encoder.  It has to
 * read in codes, the convert the codes to a string of characters.
 * The only catch in the whole operation occurs when the encoder
 * encounters a CHAR+STRING+CHAR+STRING+CHAR sequence.  When this
 * occurs, the encoder outputs a code that is not presently defined
 * in the table.  This is handled as an exception.  All of the special
 * input codes are handled in various ways.
 */

QUICK_EXPAND(lzw15v)
    unsigned int new_code;
    unsigned int old_code;
    int character;
    unsigned int count;

    lzw15v_InitializeStorage();
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
    for ( ; ; ) {
        lzw15v_InitializeDictionary();
        old_code = (unsigned int) InputBits( input, current_code_bits );
        if ( old_code == END_OF_STREAM )
            goto quit;
        character = old_code;
        mn_putc( old_code, &output );
        for ( ; ; ) {
            new_code = (unsigned int) InputBits( input, current_code_bits );
            if ( new_code == END_OF_STREAM )
                goto quit;
            if ( new_code == FLUSH_CODE )
                break;
            if ( new_code == BUMP_CODE ) {
                current_code_bits++;
                //putc( 'B', stdout );
                continue;
            }
            if ( new_code >= next_code ) {
                decode_stack[ 0 ] = (char) character;
                count = lzw15v_decode_string( 1, old_code );
            }
            else
                count = lzw15v_decode_string( 0, new_code );
            character = decode_stack[ count - 1 ];
            while ( count > 0 )
                mn_putc( decode_stack[ --count ], &output );
            DICT( next_code ).parent_code = old_code;
            DICT( next_code ).character = (char) character;
            next_code++;
            old_code = new_code;
        }
    }
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

unsigned int lzw15v_find_child_node( parent_code, child_character )
int parent_code;
int child_character;
{
    unsigned int index;
    int offset;

    index = ( child_character << ( BITS - 8 ) ) ^ parent_code;
    if ( index == 0 )
        offset = 1;
    else
        offset = TABLE_SIZE - index;
    for ( ; ; ) {
        if ( DICT( index ).code_value == UNUSED )
            return( (unsigned int) index );
        if ( DICT( index ).parent_code == parent_code &&
             DICT( index ).character == (char) child_character )
            return( index );
        if ( (int) index >= offset )
            index -= offset;
        else
            index += TABLE_SIZE - offset;
    }
}

/*
 * This routine decodes a string from the dictionary, and stores it
 * in the decode_stack data structure.  It returns a count to the
 * calling program of how many characters were placed in the stack.
 */

unsigned int lzw15v_decode_string( count, code )
unsigned int count;
unsigned int code;
{
    while ( code > 255 ) {
        decode_stack[ count++ ] = DICT( code ).character;
        code = DICT( code ).parent_code;
    }
    decode_stack[ count++ ] = (char) code;
    return( count );
}


/************************** End of lzw15vV.C *************************/
