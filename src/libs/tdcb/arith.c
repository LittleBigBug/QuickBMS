/************************** Start of ARITH.C *************************
 *
 * This is the arithmetic coding module used in Chapter 5.
 * Compile with BITIO.C, ERRHAND.C, and either MAIN-C.C or MAIN-E.C
 */
#include <stdio.h>
#include <stdlib.h>
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

/*
 * Internal function prototypes, with or without ANSI prototypes.
 */

#ifdef __STDC__

void arith_build_model( memory_file *input, memory_file *output );
void arith_scale_counts( unsigned long counts[], unsigned char scaled_counts[] );
void arith_build_totals( unsigned char scaled_counts[] );
void arith_count_bytes( memory_file *input, unsigned long counts[] );
void arith_output_counts( memory_file *output, unsigned char scaled_counts[] );
void arith_input_counts( memory_file *stream );
void arith_convert_int_to_symbol( int symbol, SYMBOL *s );
void arith_get_symbol_scale( SYMBOL *s );
int arith_convert_symbol_to_int( int count, SYMBOL *s );
void arith_initialize_arithmetic_encoder( void );
void arith_encode_symbol( BIT_FILE *stream, SYMBOL *s );
void arith_flush_arithmetic_encoder( BIT_FILE *stream );
short int arith_get_current_count( SYMBOL *s );
void arith_initialize_arithmetic_decoder( BIT_FILE *stream );
void arith_remove_symbol_from_stream( BIT_FILE *stream, SYMBOL *s );

#else

void arith_build_model();
void arith_scale_counts();
void arith_build_totals();
void arith_count_bytes();
void arith_output_counts();
void arith_input_counts();
void arith_convert_int_to_symbol();
void arith_get_symbol_scale();
int arith_convert_symbol_to_int();
void arith_initialize_arithmetic_encoder();
void arith_encode_symbol();
void arith_flush_arithmetic_encoder();
short int arith_get_current_count();
void arith_initialize_arithmetic_decoder();
void arith_remove_symbol_from_stream();

#endif

#define END_OF_STREAM 256
#define totals  arith_totals
short int totals[ 258 ];            /* The cumulative totals                */

//char *CompressionName = "Fixed order 0 model with arithmetic coding";
//char *Usage           = "in-file out-file\n\n";

/*
 * This compress file routine is a fairly orthodox compress routine.
 * It first gathers statistics, and initializes the arithmetic
 * encoder.  It then encodes all the characters in the file, followed
 * by the EOF character.  The output stream is then flushed, and we exit.
 * Note that an extra two bytes are output.  When decoding an arithmetic
 * stream, we have to read in extra bits.  The decoding process takes
 * place in the msb of the low and high range ints, so when we are
 * decoding our last bit we will still have to have at least 15 junk
 * bits loaded into the registers.  The extra two bytes account for
 * that.
 */
/*
void arith_CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    int c;
    SYMBOL s;

    arith_build_model( input, output->file );
    arith_initialize_arithmetic_encoder();

    while ( ( c = mn_getc( input ) ) != EOF ) {
	arith_convert_int_to_symbol( c, &s );
	arith_encode_symbol( output, &s );
    }
    arith_convert_int_to_symbol( END_OF_STREAM, &s );
    arith_encode_symbol( output, &s );
    arith_flush_arithmetic_encoder( output );
    OutputBits( output, 0L, 16 );
    while ( argc-- > 0 ) {
        printf( "Unused argument: %s\n", *argv );
        argv++;
    }
}*/

/*
 * This expand routine is also very conventional.  It reads in the
 * model, initializes the decoder, then sits in a loop reading in
 * characters.  When we decode an END_OF_STREAM, it means we can close
 * up the files and exit.  Note decoding a single character is a three
 * step process:  first we determine what the scale is for the current
 * symbol by looking at the difference between the high an low values.
 * We then see where the current input values fall in that range.
 * Finally, we look in our totals array to find out what symbol is
 * a match.  After that is done, we still have to remove that symbol
 * from the decoder.  Lots of work.
 */
/*
void ExpandFile( input, output, argc, argv )
BIT_FILE *input;
FILE *output;
int argc;
char *argv[];
{*/
QUICK_EXPAND(arith)
    SYMBOL s;
    int c;
    int count;

    arith_input_counts( &input->file );
    arith_initialize_arithmetic_decoder( input );
    for ( ; ; ) {
	arith_get_symbol_scale( &s );
	count = arith_get_current_count( &s );
	c = arith_convert_symbol_to_int( count, &s );
	if ( c == END_OF_STREAM )
            break;
	arith_remove_symbol_from_stream( input, &s );
        mn_putc( (char) c, &output );
    }
    while ( argc-- > 0 ) {
        printf( "Unused argument: %s\n", *argv );
        argv++;
    }
    CloseInputBitFile(input);
    return(output.data - out);
}

/*
 * This is the routine that is called to scan the input file, scale
 * the counts, build the totals array, the output the scaled counts
 * to the output file.
 */

void arith_build_model( input, output )
memory_file *input;
memory_file *output;
{
    unsigned long counts[ 256 ];
    unsigned char scaled_counts[ 256 ];

    arith_count_bytes( input, counts );
    arith_scale_counts( counts, scaled_counts );
    arith_output_counts( output, scaled_counts );
    arith_build_totals( scaled_counts );
}

/*
 * This routine runs through the file and counts the appearances of each
 * character.
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

void arith_count_bytes( input, counts )
memory_file *input;
unsigned long counts[];
{
    long input_marker;
    int i;
    int c;

    for ( i = 0 ; i < 256 ; i++ )
        counts[ i ] = 0;
    input_marker = input->data - input->datas; //ftell( input );
    while ( ( c = mn_getc( input )) != EOF )
	counts[ c ]++;
    input->data = input->datas + input_marker;  //fseek( input, input_marker, SEEK_SET );
}

/*
 * This routine is called to scale the counts down.  There are two types
 * of scaling that must be done.  First, the counts need to be scaled
 * down so that the individual counts fit into a single unsigned char.
 * Then, the counts need to be rescaled so that the total of all counts
 * is less than 16384.
 */
void arith_scale_counts( counts, scaled_counts )
unsigned long counts[];
unsigned char scaled_counts[];
{
    int i;
    unsigned long max_count;
    unsigned int total;
    unsigned long scale;

/*
 * The first section of code makes sure each count fits into a single byte.
 */
    max_count = 0;
    for ( i = 0 ; i < 256 ; i++ )
       if ( counts[ i ] > max_count )
	   max_count = counts[ i ];
    scale = max_count / 256;
    scale = scale + 1;
    for ( i = 0 ; i < 256 ; i++ ) {
        scaled_counts[ i ] = (unsigned char ) ( counts[ i ] / scale );
        if ( scaled_counts[ i ] == 0 && counts[ i ] != 0 )
            scaled_counts[ i ] = 1;
    }
/*
 * This next section makes sure the total is less than 16384.  I initialize
 * the total to 1 instead of 0 because there will be an additional 1 added
 * in for the END_OF_STREAM symbol;
 */
    total = 1;
    for ( i = 0 ; i < 256 ; i++ )
        total += scaled_counts[ i ];
    if ( total > ( 32767 - 256 ) )
        scale = 4;
    else if ( total > 16383 )
        scale = 2;
    else
        return;
    for ( i = 0 ; i < 256 ; i++ )
        scaled_counts[ i ] /= scale;
}

/*
 * This routine is used by both the encoder and decoder to build the
 * table of cumulative totals.  The counts for the characters in the
 * file are in the counts array, and we know that there will be a single
 * instance of the EOF symbol.
 */
void arith_build_totals( scaled_counts )
unsigned char scaled_counts[];
{
    int i;

    totals[ 0 ] = 0;
    for ( i = 0 ; i < END_OF_STREAM ; i++ )
        totals[ i + 1 ] = totals[ i ] + scaled_counts[ i ];
    totals[ END_OF_STREAM + 1 ] = totals[ END_OF_STREAM ] + 1;
}

/*
 * In order for the compressor to build the same model, I have to store
 * the symbol counts in the compressed file so the expander can read
 * them in.  In order to save space, I don't save all 256 symbols
 * unconditionally.  The format used to store counts looks like this:
 *
 *  start, stop, counts, start, stop, counts, ... 0
 *
 * This means that I store runs of counts, until all the non-zero
 * counts have been stored.  At this time the list is terminated by
 * storing a start value of 0.  Note that at least 1 run of counts has
 * to be stored, so even if the first start value is 0, I read it in.
 * It also means that even in an empty file that has no counts, I have
 * to pass at least one count.
 *
 * In order to efficiently use this format, I have to identify runs of
 * non-zero counts.  Because of the format used, I don't want to stop a
 * run because of just one or two zeros in the count stream.  So I have
 * to sit in a loop looking for strings of three or more zero values in
 * a row.
 *
 * This is simple in concept, but it ends up being one of the most
 * complicated routines in the whole program.  A routine that just
 * writes out 256 values without attempting to optimize would be much
 * simpler, but would hurt compression quite a bit on small files.
 *
 */
void arith_output_counts( output, scaled_counts )
memory_file *output;
unsigned char scaled_counts[];
{
    int first;
    int last;
    int next;
    int i;

    first = 0;
    while ( first < 255 && scaled_counts[ first ] == 0 )
	    first++;
/*
 * Each time I hit the start of the loop, I assume that first is the
 * number for a run of non-zero values.  The rest of the loop is
 * concerned with finding the value for last, which is the end of the
 * run, and the value of next, which is the start of the next run.
 * At the end of the loop, I assign next to first, so it starts in on
 * the next run.
 */
    for ( ; first < 256 ; first = next ) {
	last = first + 1;
	for ( ; ; ) {
	    for ( ; last < 256 ; last++ )
                if ( scaled_counts[ last ] == 0 )
		    break;
	    last--;
	    for ( next = last + 1; next < 256 ; next++ )
                if ( scaled_counts[ next ] != 0 )
		    break;
	    if ( next > 255 )
		break;
	    if ( ( next - last ) > 3 )
		break;
	    last = next;
	};
/*
 * Here is where I output first, last, and all the counts in between.
 */
        if ( mn_putc( first, output ) != first )
	    fatal_error( "Error writing byte counts\n" );
	if ( mn_putc( last, output ) != last )
	    fatal_error( "Error writing byte counts\n" );
	for ( i = first ; i <= last ; i++ ) {
            if ( mn_putc( scaled_counts[ i ], output ) !=
                 (int) scaled_counts[ i ] )
		fatal_error( "Error writing byte counts\n" );
	}
    }
    if ( mn_putc( 0, output ) != 0 )
	    fatal_error( "Error writing byte counts\n" );
}

/*
 * When expanding, I have to read in the same set of counts.  This is
 * quite a bit easier that the process of writing them out, since no
 * decision making needs to be done.  All I do is read in first, check
 * to see if I am all done, and if not, read in last and a string of
 * counts.
 */

void arith_input_counts( input )
memory_file *input;
{
    int first;
    int last;
    int i;
    int c;
    unsigned char scaled_counts[ 256 ];

    for ( i = 0 ; i < 256 ; i++ )
        scaled_counts[ i ] = 0;
    if ( ( first = mn_getc( input ) ) == EOF )
	fatal_error( "Error reading byte counts\n" );
    if ( ( last = mn_getc( input ) ) == EOF )
	fatal_error( "Error reading byte counts\n" );
    for ( ; ; ) {
	for ( i = first ; i <= last ; i++ )
            if ( ( c = mn_getc( input ) ) == EOF )
		fatal_error( "Error reading byte counts\n" );
	    else
                scaled_counts[ i ] = (unsigned char) c;
        if ( ( first = mn_getc( input ) ) == EOF )
	    fatal_error( "Error reading byte counts\n" );
	if ( first == 0 )
	    break;
        if ( ( last = mn_getc( input ) ) == EOF )
	    fatal_error( "Error reading byte counts\n" );
    }
    arith_build_totals( scaled_counts );
}

/*
 * Everything from here down define the arithmetic coder section
 * of the program.
 */

/*
 * These four variables define the current state of the arithmetic
 * coder/decoder.  They are assumed to be 16 bits long.  Note that
 * by declaring them as short ints, they will actually be 16 bits
 * on most 80X86 and 680X0 machines, as well as VAXen.
 */
#define code    arith_code
static unsigned short int code;  /* The present input code value       */
#define low     arith_low
static unsigned short int low;   /* Start of the current code range    */
#define high    arith_high
static unsigned short int high;  /* End of the current code range      */
#define underflow_bits  arith_underflow_bits
long underflow_bits;             /* Number of underflow bits pending   */

/*
 * This routine must be called to initialize the encoding process.
 * The high register is initialized to all 1s, and it is assumed that
 * it has an infinite string of 1s to be shifted into the lower bit
 * positions when needed.
 */
void arith_initialize_arithmetic_encoder()
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
void arith_flush_arithmetic_encoder( stream )
BIT_FILE *stream;
{
    OutputBit( stream, low & 0x4000 );
    underflow_bits++;
    while ( underflow_bits-- > 0 )
        OutputBit( stream, ~low & 0x4000 );
}

/*
 * Finding the low count, high count, and scale for a symbol
 * is really easy, because of the way the totals are stored.
 * This is the one redeeming feature of the data structure used
 * in this implementation.
 */
void arith_convert_int_to_symbol( c, s )
int c;
SYMBOL *s;
{
    s->scale = totals[ END_OF_STREAM + 1 ];
    s->low_count = totals[ c ];
    s->high_count = totals[ c + 1 ];
}

/*
 * Getting the scale for the current context is easy.
 */
void arith_get_symbol_scale( s )
SYMBOL *s;
{
    s->scale = totals[ END_OF_STREAM + 1 ];
}

/*
 * During decompression, we have to search through the table until
 * we find the symbol that straddles the "count" parameter.  When
 * it is found, it is returned. The reason for also setting the
 * high count and low count is so that symbol can be properly removed
 * from the arithmetic coded input.
 */
int arith_convert_symbol_to_int( count, s)
int count;
SYMBOL *s;
{
    int c;

    for ( c = END_OF_STREAM ; count < totals[ c ] ; c-- )
	;
    s->high_count = totals[ c + 1 ];
    s->low_count = totals[ c ];
    return( c );
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
void arith_encode_symbol( stream, s )
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
short int arith_get_current_count( s )
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
void arith_initialize_arithmetic_decoder( stream )
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
void arith_remove_symbol_from_stream( stream, s )
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
/************************** End of ARITH.C ****************************/

