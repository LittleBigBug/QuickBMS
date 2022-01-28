/************************** Start of LZSS.C *************************
 *
 * This is the LZSS module, which implements an LZ77 style compression
 * algorithm.  As iplemented here it uses a 12 bit index into the sliding
 * window, and a 4 bit length, which is adjusted to reflect phrase lengths
 * of between 2 and 17 bytes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mn_incs.h"

/*
 * Various constants used to define the compression parameters.  The
 * lzss_INDEX_BIT_COUNT tells how many bits we allocate to indices into the
 * text window.  This directly determines the WINDOW_SIZE.  The
 * lzss_LENGTH_BIT_COUNT tells how many bits we allocate for the length of
 * an encode phrase. This determines the size of the look ahead buffer.
 * The TREE_ROOT is a special node in the tree that always points to
 * the root node of the binary phrase tree.  lzss_END_OF_STREAM is a special
 * index used to flag the fact that the file has been completely
 * encoded, and there is no more data.  UNUSED is the null index for
 * the tree. MOD_WINDOW() is a macro used to perform arithmetic on tree
 * indices.
 *
 */

static int lzss_INDEX_BIT_COUNT   = 12;
static int lzss_LENGTH_BIT_COUNT  = 4;
static int lzss_DUMMY9            = 9;
static int lzss_END_OF_STREAM     = 0;
#define WINDOW_SIZE          ( 1 << lzss_INDEX_BIT_COUNT )
#define RAW_LOOK_AHEAD_SIZE  ( 1 << lzss_LENGTH_BIT_COUNT )
#define BREAK_EVEN           ( ( 1 + lzss_INDEX_BIT_COUNT + lzss_LENGTH_BIT_COUNT ) / lzss_DUMMY9 )
#define LOOK_AHEAD_SIZE      ( RAW_LOOK_AHEAD_SIZE + BREAK_EVEN )
#define TREE_ROOT            WINDOW_SIZE
#define MOD_WINDOW( a )      ( ( a ) & ( WINDOW_SIZE - 1 ) )

//char *CompressionName = "LZSS Encoder";
//char *Usage           = "in-file out-file\n\n";


/*
 * This is the expansion routine for the LZSS algorithm.  All it has
 * to do is read in flag bits, decide whether to read in a character or
 * a index/length pair, and take the appropriate action.
*/

QUICK_EXPAND(lzss)
    unsigned char window[WINDOW_SIZE];
    int i;
    int current_position;
    int c;
    int match_length;
    int match_position;

    // alloca() works on the stack so no free()
    //window = alloca(WINDOW_SIZE);
    //if(!window) return(-1);

    current_position = 1;
    for ( ; ; ) {
        if ( InputBit( input ) ) {
            c = (int) InputBits( input, 8 );
            mn_putc( c, &output );
            window[ current_position ] = (unsigned char) c;
            current_position = MOD_WINDOW( current_position + 1 );
        } else {
            match_position = (int) InputBits( input, lzss_INDEX_BIT_COUNT );
            if ( match_position == lzss_END_OF_STREAM )
                break;
            match_length = (int) InputBits( input, lzss_LENGTH_BIT_COUNT );
            match_length += BREAK_EVEN;
            for ( i = 0 ; i <= match_length ; i++ ) {
                c = window[ MOD_WINDOW( match_position + i ) ];
                mn_putc( c, &output );
                window[ current_position ] = (unsigned char) c;
                current_position = MOD_WINDOW( current_position + 1 );
            }
        }
    }
    CloseInputBitFile(input);
    return(output.data - out);
}



int tdcb_lzss_init(int x1, int x2, int x3, int x4) {
    lzss_INDEX_BIT_COUNT   = x1;
    lzss_LENGTH_BIT_COUNT  = x2;
    lzss_DUMMY9            = x3;
    lzss_END_OF_STREAM     = x4;
    return(0);
}



/************************** End of LZSS.C *************************/

