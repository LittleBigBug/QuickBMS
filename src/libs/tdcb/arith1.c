/************************** Start of ARITH1.C *************************
 *
 * This is the order-1 arithmetic coding module used in the first
 * part of chapter 6. Compile with BITIO.C, ERRHAND.C, and either
 * MAIN-C.C or MAIN-E.C.  Under MS-DOS, you will need to use the
 * compact or large models, as this routine allocates a lot of memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mn_incs.h"

/*
 * The SYMBOL structure is what is used to define a symbol in
 * arithmetic coding terms.  A symbol is defined as a range between
 * 0 and 1.  Since we are using integer math, instead of using 0 and 1
 * as our end points, we have an integer scale.  The low_count and
 * high_count define where the symbol falls in the range.
 */

typedef struct {
    unsigned short int low_count;
    unsigned short int high_count;
    unsigned short int scale;
} SYMBOL;

#define MAXIMUM_SCALE   16383  /* Maximum allowed frequency count        */
#define END_OF_STREAM   256    /* The EOF symbol                         */

//extern long underflow_bits;    /* The present underflow count in         */
                               /* the arithmetic coder.                  */
#define totals  arith1_totals
static int *totals[ 257 ] = {NULL};     /* Pointers to the 257 context tables     */

/*
 * Local Function prototypes.
 */

#ifdef __STDC__
void arith1_initialize_arithmetic_decoder( BIT_FILE *stream );
void arith1_remove_symbol_from_stream( BIT_FILE *stream, SYMBOL *s );
void arith1_initialize_arithmetic_encoder( void );
void arith1_encode_symbol( BIT_FILE *stream, SYMBOL *s );
void arith1_flush_arithmetic_encoder( BIT_FILE *stream );
short int arith1_get_current_count( SYMBOL *s );
void arith1_initialize_model( void );
void arith1_update_model( int symbol, int context );
void arith1_convert_int_to_symbol( int symbol, int context, SYMBOL *s );
void arith1_get_symbol_scale( int context, SYMBOL *s );
int arith1_convert_symbol_to_int( int count, int context, SYMBOL *s );
#else
void arith1_initialize_arithmetic_decoder();
void arith1_remove_symbol_from_stream();
void arith1_initialize_arithmetic_encoder();
void arith1_encode_symbol();
void arith1_flush_arithmetic_encoder();
short int arith1_get_current_count();
void arith1_initialize_model();
void arith1_update_model();
void arith1_convert_int_to_symbol();
void arith1_get_symbol_scale();
int arith1_convert_symbol_to_int();
#endif

//char *CompressionName = "Adaptive order 1 model with arithmetic coding";
//char *Usage           = "in-file out-file\n\n";

/*
 * This is the compression main loop.  It is called to compress the
 * already opened file.
 */
/*
void arith1_CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    SYMBOL s;
    int c;
    int context;

    context = 0;
    arith1_initialize_model();
    arith1_initialize_arithmetic_encoder();
    for ( ; ; ) {
        c = mn_getc( input );
        if ( c == EOF )
            c = END_OF_STREAM;
        arith1_convert_int_to_symbol( c, context, &s );
        arith1_encode_symbol( output, &s );
        if ( c == END_OF_STREAM )
	    break;
        arith1_update_model( c, context );
        context = c;
    }
    arith1_flush_arithmetic_encoder( output );
    //putchar( '\n' );
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
}*/

/*
 * This is the expansion routine.  It reads in bytes from the compressed
 * file and writes them out to the text file.
 */
/*
void arith1_ExpandFile( input, output, argc, argv )
BIT_FILE *input;
memory_file *output;
int argc;
char *argv[];
{*/
QUICK_EXPAND(arith1)
    SYMBOL s;
    int count;
    int c;
    int context;

    context = 0;
    arith1_initialize_model();
    arith1_initialize_arithmetic_decoder( input );
    for ( ; ; ) {
        arith1_get_symbol_scale( context, &s );
        count = arith1_get_current_count( &s );
        c = arith1_convert_symbol_to_int( count, context, &s );
        arith1_remove_symbol_from_stream( input, &s );
        if ( c == END_OF_STREAM )
            break;
        mn_putc( (char) c, &output );
        arith1_update_model( c, context );
        context = c;
    }
    //putchar( '\n' );
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
    CloseInputBitFile(input);
    return(output.data - out);
}

/*
 * The next five routines implement the modeling functions for the order-1
 * adpative model.  The first routine initializes the model, which consists
 * of allocating 256 context tables.  Note that even though there are
 * 257 symbols in the alphabet, after taking into account the END_OF_STREAM
 * symbol, we only need 256 tables.  END_OF_STREAM doesn't get a table of
 * its own, since it will never be used as a context.
 */

void arith1_initialize_model()
{
    int context;
    int i;

    for ( context = 0 ; context < END_OF_STREAM ; context++ ) {
        if(totals[ context ]) memset(totals[context], 0, (END_OF_STREAM + 2) * sizeof(int));
        else
        totals[ context ] = (int *) calloc( END_OF_STREAM + 2, sizeof(int) );
        if ( totals[ context ] == NULL )
            fatal_error( "Failure allocating context %d", context );
        for ( i = 0 ; i <= ( END_OF_STREAM + 1 ) ; i++ )
            totals[ context ][ i ] = i;
    }
}

/*
 * This routine is called to increment the counts for the current
 * context.  It is called after a character has been encoded or
 * decoded.  It has to increment all the counts, then check to see
 * if we have hit the maximum allowable count.  If we have, the
 * counts are rescaled, making sure that none of them have fallen to
 * zero.
 */

void arith1_update_model( symbol, context )
int symbol;
int context;
{
    int i;

    for ( i = symbol + 1 ; i <= ( END_OF_STREAM + 1 ) ; i++ )
        totals[ context ][ i ]++;
    if ( totals[ context ][ END_OF_STREAM + 1 ] < MAXIMUM_SCALE )
        return;
    for ( i = 1 ; i <= ( END_OF_STREAM + 1 ) ; i++ ) {
        totals[ context ][ i ] /= 2;
        if ( totals[ context ][ i ] <= totals[ context ][ i - 1 ] )
            totals[ context ][ i ] = totals[ context ][ i - 1 ] + 1;
    }
}

/*
 * Convert an integer to a symbol works just like the old arithmetic
 * coding routine in ARITH.C.  The only difference is that we have to
 * pass a context number so the routine knows which context table to
 * get the counts and scale from.
 */
void arith1_convert_int_to_symbol( c, context, s )
int c;
int context;
SYMBOL *s;
{
    s->scale = totals[ context ][ END_OF_STREAM + 1 ];
    s->low_count = totals[ context ][ c ];
    s->high_count = totals[ context ][ c + 1 ];
}

/*
 * Getting the current scale for the symbol just means pulling the number
 * out of the chosen context table.
 */

void arith1_get_symbol_scale( context, s )
int context;
SYMBOL *s;
{
    s->scale = totals[ context][ END_OF_STREAM + 1 ];
}

/*
 * Converting a symbol to an int means looking through the current
 * context for a symbol that spans the range that the SYMBOL parameter
 * has.
 */
int arith1_convert_symbol_to_int( count, context, s)
int count;
int context;
SYMBOL *s;
{
    int c;

    for ( c = 0; count >= totals[ context ][ c + 1 ] ; c++ )
        ;
    s->high_count = totals[ context ][ c + 1 ];
    s->low_count = totals[ context ][ c ];
    return( c );
}


/*
 * The remaining routines are the arithmetic coding routines.  They are
 * identical to those seen earlier in ARITH.C.
 */

/*
 * These four variables define the current state of the arithmetic
 * coder/decoder.  They are assumed to be 16 bits long.  Note that
 * by declaring them as short ints, they will actually be 16 bits
 * on most 80X86 and 680X0 machines, as well as VAXen.
 */
#define code    arith1_code
static unsigned short int code;  /* The present input code value       */
#define low     arith1_low
static unsigned short int low;   /* Start of the current code range    */
#define high    arith1_high
static unsigned short int high;  /* End of the current code range      */
#define underflow_bits  arith1_underflow_bits
long underflow_bits;             /* Number of underflow bits pending   */

/*
 * This routine must be called to initialize the encoding process.
 * The high register is initialized to all 1s, and it is assumed that
 * it has an infinite string of 1s to be shifted into the lower bit
 * positions when needed.
 */
void arith1_initialize_arithmetic_encoder()
{
    low = 0;
    high = 0xffff;
    underflow_bits = 0;
}

/*
 * At the end of the encoding process, there are still significant
 * bits left in the high and low registers.  We output two bits,
 * plus as many underflow bits as are necessary.
 */
void arith1_flush_arithmetic_encoder( stream )
BIT_FILE *stream;
{
    OutputBit( stream, low & 0x4000 );
    underflow_bits++;
    while ( underflow_bits-- > 0 )
        OutputBit( stream, ~low & 0x4000 );
    OutputBits( stream, 0L, 16 );
}

/*
 * This routine is called to encode a symbol.  The symbol is passed
 * in the SYMBOL structure as a low count, a high count, and a range,
 * instead of the more conventional probability ranges.  The encoding
 * process takes two steps.  First, the values of high and low are
 * updated to take into account the range restriction created by the
 * new symbol.  Then, as many bits as possible are shifted out to
 * the output stream.  Finally, high and low are stable again and
 * the routine returns.
 */
void arith1_encode_symbol( stream, s )
BIT_FILE *stream;
SYMBOL *s;
{
    long range;
/*
 * These three lines rescale high and low for the new symbol.
 */
    range = (long) ( high-low ) + 1;
    high = low + (unsigned short int)
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int)
                 (( range * s->low_count ) / s->scale );
/*
 * This loop turns out new bits until high and low are far enough
 * apart to have stabilized.
 */
    for ( ; ; ) {
/*
 * If this test passes, it means that the MSDigits match, and can
 * be sent to the output stream.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
            OutputBit( stream, high & 0x8000 );
            while ( underflow_bits > 0 ) {
                OutputBit( stream, ~high & 0x8000 );
                underflow_bits--;
            }
        }
/*
 * If this test passes, the numbers are in danger of underflow, because
 * the MSDigits don't match, and the 2nd digits are just one apart.
 */
        else if ( ( low & 0x4000 ) && !( high & 0x4000 )) {
            underflow_bits += 1;
            low &= 0x3fff;
            high |= 0x4000;
        } else
            return ;
        low <<= 1;
        high <<= 1;
        high |= 1;
    }
}

/*
 * When decoding, this routine is called to figure out which symbol
 * is presently waiting to be decoded.  This routine expects to get
 * the current model scale in the s->scale parameter, and it returns
 * a count that corresponds to the present floating point code:
 *
 *  code = count / s->scale
 */
short int arith1_get_current_count( s )
SYMBOL *s;
{
    long range;
    short int count;

    range = (long) ( high - low ) + 1;
    count = (short int)
            ((((long) ( code - low ) + 1 ) * s->scale-1 ) / range );
    return( count );
}

/*
 * This routine is called to initialize the state of the arithmetic
 * decoder.  This involves initializing the high and low registers
 * to their conventional starting values, plus reading the first
 * 16 bits from the input stream into the code value.
 */
void arith1_initialize_arithmetic_decoder( stream )
BIT_FILE *stream;
{
    int i;

    code = 0;
    for ( i = 0 ; i < 16 ; i++ ) {
        code <<= 1;
        code += InputBit( stream );
    }
    low = 0;
    high = 0xffff;
}

/*
 * Just figuring out what the present symbol is doesn't remove
 * it from the input bit stream.  After the character has been
 * decoded, this routine has to be called to remove it from the
 * input stream.
 */
void arith1_remove_symbol_from_stream( stream, s )
BIT_FILE *stream;
SYMBOL *s;
{
    long range;

/*
 * First, the range is expanded to account for the symbol removal.
 */
    range = (long)( high - low ) + 1;
    high = low + (unsigned short int)
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int)
                 (( range * s->low_count ) / s->scale );
/*
 * Next, any possible bits are shipped out.
 */
    for ( ; ; ) {
/*
 * If the MSDigits match, the bits will be shifted out.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
        }
/*
 * Else, if underflow is threatening, shift out the 2nd MSDigit.
 */
        else if ((low & 0x4000) == 0x4000  && (high & 0x4000) == 0 ) {
            code ^= 0x4000;
            low   &= 0x3fff;
            high  |= 0x4000;
        } else
 /*
 * Otherwise, nothing can be shifted out, so I return.
 */
            return;
        low <<= 1;
        high <<= 1;
        high |= 1;
        code <<= 1;
        code += InputBit( stream );
    }
}

/************************** End of ARITH1.C ****************************/

