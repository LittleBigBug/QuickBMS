// modified by Luigi Auriemma
/*  dmc.c										T.L. Yu, Feb. 95
*  uses dynamic markov model to compress data.
*  The program has been compiled using TURBO C or WATCOM C;
*  so far, both resulted "exe" files work well.  If use TURBO C,
*  huge model should be chosen.
*  This program is for demonstration use.  Compression improvement
*  can be obtained by adjusting min_cnt1, min_cnt2 and the way of
*  reconstructing model when memory is full.
*  Usage --  to compress :   dmc  input_file   output_file
*            to expand   :   dmc -e input_file output_file
*
*/

#include	<stdio.h>
//#ifdef   	__TURBOC__
//#include 	<alloc.h>
//#else
//#include 	<malloc.h>
//#endif
#include    <string.h>
//#include    <io.h>
//#include    "iodmc.fun"			/* io functions */
#include	<stdlib.h>



static
unsigned char   *infile   = NULL,
                *infilel  = NULL,
                *outfile  = NULL,
                *outfilel = NULL;
static int xgetc(void *skip) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
static int xputc(int chr, void *skip) {
    if(outfile >= outfilel) return(-1);
    *outfile++ = chr;
    return(chr);
}



#define check_mem_error( x )    //    if ( x == NULL ){printf("\nout of memory");

#define rotater( x ) { x >>= 1; if ( x == 0) x = 0x80; }  /* rotate right */ 
#define input_bit( input, x )  {                    \
	rotater( in_control.mask );                     \
	if ( in_control.mask == 0x80 ){                 \
		in_control.code = xgetc( input );           \
		if ( in_control.code == EOF ){              \
			printf("\nerror! out of data");         \
			exit( 1 );                              \
		}             								\
	}     										    \
    x = in_control.mask & in_control.code ? 1 : 0 ; \
} 
#define output_bit( output,  bit ) {                \
	if ( bit )                                      \
		out_control.code |= out_control.mask;  	 	\
	rotater( out_control.mask )     	           	\
	if ( out_control.mask == 0x80 ){           	 	\
		if ( xputc( out_control.code, output ) != out_control.code ) \
			printf("\nfatal error in output_bit" ); \
		out_control.code = 0;                       \
	}                                               \
}
typedef unsigned int ui; 
typedef unsigned short  int us; 
typedef unsigned char uc; 
typedef unsigned long ul; 
 typedef struct { 
	uc  mask; 
	int code; 
} IO_CONTROL; 

IO_CONTROL out_control = { 0x80, 0 }; 
IO_CONTROL  in_control  = { 0x01, 0 }; 



/*	because we only have two symbols, we do not need higher precision */
#define     NBITS   15      	/* # of bits used in high, low */
#define		MSBIT	0x4000      /* most significant bit */
#define		MSMASK	0x7FFF		/* consider 15 bits */
#define     MASK2   0x1FFF		/* for underflow use */


//char        *info = "Dynamic Markov Compression ( DMC ) ";
static ui          code,  low,  high, mp;
//static int         underflow_bits;
//static int         times = 1;
//static ul			switch_delta = 5000;
//static ul			*switch_state;
//static ul          switch_len;
/*
void	put_file_length( long l, FILE *output )
{
	char	*pc= (char*) &l, i;

    //printf(" l = %ld ", l );
	for ( i = 0; i < sizeof( long ); ++i )
		xputc( *pc++, output );
}

long	get_file_length( FILE	*input )
{
	long	l;
	char	*pc = (char *) &l, i;

	for ( i = 0; i < sizeof( long ); ++i )
		*pc++ = xgetc( input );
	return( l );
}

void initialize_encoder()
{
    low = 0;
	high = MSMASK;
    underflow_bits = 0;
}
*/
static void initialize_decoder( void *input )
{
	int i, bit;

    code = 0;
	for ( i = 0 ; i < NBITS; i++ ) {
		code <<= 1;
		input_bit( input, bit );
		code += bit;
	}
	low = 0;
	high = MSMASK;
}

/* 	next_state[d][s]  = state reached from s after transition d
	trans_cnt[d][s]   = number of observations of input d when in state s
	state             = number of current state
	min_cnt1	      = minimum # of transitions from the current state
						to state s before s is eligible for cloning
	min_cnt2		  = minimum # of visits to a state s from all
						predecessors of S other than the current state
						before S is eligible for cloning.
                        A simple choice for min_cnt1 and min_cnt2 values
                        is to set them to 2, 2.
*/
/* int and unsigned int are 32 bits  for WATCOM C */
#ifdef   __TURBOC__
#define maxstates  	 32760			/* TURBO C can access low DOS mem only*/
#else
#define maxstates  	 500000l		/* for WATCOM C */
#endif
ui *next_state[2] = {NULL}, *trans_cnt[2] = {NULL};
ui state, last_state;
ui nbits, total_states;
int   min_cnt1 = 2, min_cnt2 = 2;/* for comparison, thus make it signed*/


/* initialize the model */
static void initialize_model()
{
     int i, j, k, m, n;
     static int initialized = 0;

	 min_cnt1 = min_cnt2 = 2;
	 if ( !initialized ) {
		if(!next_state[0]) next_state[0] = (ui *) malloc( maxstates*sizeof( ui ) );
        check_mem_error( next_state[0] );
		if(!next_state[1]) next_state[1] = (ui *) malloc( maxstates*sizeof( ui ) );
        check_mem_error( next_state[1] );
		if(!trans_cnt[0]) trans_cnt[0] = (ui *) malloc( maxstates*sizeof( ui ) );
        check_mem_error( trans_cnt[0] );
		if(!trans_cnt[1]) trans_cnt[1] = (ui *) malloc( maxstates*sizeof( ui ) );
        check_mem_error( trans_cnt[1] );
		initialized = 1;
	 } else {
		for ( i = 0; i < maxstates; ++i )
			trans_cnt[0][i] = trans_cnt[1][i] = 0;
	 }
     n = 8;
     //printf(" initialize_model %d times ", times++);
     m = 1;
     for ( i = 0; i < n; ++i )
         m = 2 * m;
     for ( i = 0; i < n; ++i ) {
         for ( j = 0; j < m; ++j ) {
             state = i + n * j;
             k = ( i + 1 ) % n;
             next_state[0][state] = k + (( 2*j ) % m ) * n;
             next_state[1][state] = k + ((2*j+1) % m ) * n;
             trans_cnt[0][state] = 1;   /* force this to 1 to avoid overflow*/
             trans_cnt[1][state] = 1;
         }
     }
     last_state = n * m - 1;
}


static void update_count( int   x )
/* x is current bit */
{
	int b;
    unsigned int nxt, nxt_cnt, new;

    if ( trans_cnt[x][state] > 0xfff1 ){
	   trans_cnt[0][state] /= 2;		/* rescale counts to avoid overflow*/
       trans_cnt[1][state] /= 2;
    }
	++trans_cnt[x][state];
	nxt = next_state[x][state];       /* next state */
					/* total transitions out of "nxt" on receiving 0, or 1 */
	nxt_cnt = trans_cnt[0][nxt] + trans_cnt[1][nxt];
	if ( (trans_cnt[x][state] > min_cnt1) &&
	 ((int)(nxt_cnt - trans_cnt[x][state])>min_cnt2) ){
		++last_state;
		new = last_state;		/* obtain a new state # */
		next_state[x][state] = new;
		for ( b = 0; b <= 1; ++b ){
			next_state[b][new] = next_state[b][nxt];
			trans_cnt[b][new] = (ui) ( (ul) trans_cnt[b][nxt] *
										trans_cnt[x][state] / nxt_cnt );
			trans_cnt[b][nxt] = trans_cnt[b][nxt] - trans_cnt[b][new];
		}
		nxt = new;
	}
	state = nxt;
}

/*
static void	flush_encoder( void	*output )
{
	int	b, i;
	output_bit( output, low & ( MSBIT >> 1 ) );
	underflow_bits++;
	b = (~low & ( MSBIT >> 1 ) ) ? 1 : 0;
	while ( underflow_bits-- > 0 )
		output_bit( output, b );
	b = 0;
	for ( i = 0; i < 16; ++i )
		output_bit( output, b );
}*/


static ui  get_mp ()
/*  get mid point of high-low interval */
{
    ui  p0, p1, mp;
	ul ps, range;

    p0 = trans_cnt[0][state] + 1;
	p1 = trans_cnt[1][state] + 1;
	ps = p0 + p1;
	ps = ( ul )p0 + ( ul ) p1;          /* ps is unsigned long */

    range = ( ul )( high - low ) + 1;
	mp = low + (ui)	 (( range * p0 ) / ps );
	if ( mp >= high ) mp = high - 1;         /* take care of roundoff error*/
	return( mp );
}

/*static void	shift_out_encoded_bits( void *output )
{
	int	b;

	for ( ; ; ) {
	if ( ( low & MSBIT ) == ( high & MSBIT ) ) {
		b = ( high & MSBIT ) ? 1 : 0;
		output_bit(output, b);
			b = b ? 0 : 1;
			while ( underflow_bits > 0 ){
				output_bit( output, b );
				underflow_bits--;
			}
	}
	else if ((low & ( MSBIT >> 1)) && !( high & (MSBIT >> 1) )) {
		underflow_bits += 1;
		low = low &  MASK2;
		high = high | (MSBIT>>1);
	} else
		break;
	low = ( low << 1) & MSMASK;
		high = ( high << 1) & MSMASK;
		high |= 1;
	}
}*/   /* shift_out_encoded_bits()	*/

/*
void encode( FILE *input, FILE *output )
{
	int		mark, c;
	int	i, j, k,  b;
    long   range;

	state = 0;
	do {
		mark = c = xgetc( input );

		for ( k = 0; k < 8; ++k ){
			b = 0x80 & c;
			b = ( b > 0 ) ? 1 : 0;
			mp = get_mp();
			if ( last_state == maxstates )
               initialize_model();
			update_count( b );
            c <<= 1;
			if ( b == 1 )
				low = mp;            // pick upper part of range
			else
				high = mp - 1;		// pick lower part of range
			shift_out_encoded_bits( output );
		}	// for k
	}	while ( mark != EOF );  // do loop
}*/	/* encode */


static void	remove_and_get_bits( void	*input )
{
	int	bit;

	for ( ; ; ) {
		/* If the MSBs match, shift out the bits.*/
		if ( ( high & MSBIT ) == ( low & MSBIT ) )
			;
		/* Else, throw away 2nd MSB to prevent underflow.*/
		else if ((low & (MSBIT>>1)) && ! (high & (MSBIT >> 1) ) ) {
			code ^= (MSBIT>>1);
			low   = low & MASK2;
			high  |= (MSBIT>>1);
        } else
		/* Otherwise, nothing to shift, so  return.*/
			break;
        low = ( low << 1) & MSMASK;
        high = ( high << 1) & MSMASK;
        high |= 1;
		code = ( code << 1 ) & MSMASK;
		input_bit( input, bit );
		code += bit;
	}    /* for (;;) */
}   /* remove_and_get_bits() */


static void decode( long	flen, 	void *input,	void *output )
{
	//FILE	*fp;
	int	b,/*n, i, j,*/ k=0;
	ul  len = 0;

    state = 0;
	while ( 1 ) {
        mp = get_mp();
		if ( code >= mp ){		/* determine if symbol is 0 or 1 */
				b = 1;
				low = mp;
			} else{
				b = 0;
				high = mp -1;
		}
		output_bit( output, b);			/* output a bit */
        if ( ++k == 8 ){
           ++len;
           k = 0;
        }
        if ( len == flen )
           break;
		if ( last_state == maxstates )
			initialize_model();
		update_count(b);   /* update state */

        /* Next, remove matched or underflow bits. */
 		remove_and_get_bits( input );
	}    /* while ( 1 ) */
}	/* decode */

/*
void	compress( char	*argv[] )
{
	FILE *input,  *output;
	long file_length,  file_size();				// in main.c
    int c;

	if ( ( input = fopen( *argv++, "rb" ) ) == NULL ){
		printf("\n%s doesn't exist ", *(argv-1) );
		exit( 1 );
	}
	output = fopen( *argv, "wb" );
	file_length = filelength ( fileno( input ) );
	put_file_length( file_length, output );
	initialize_model();			// initialize the model
	initialize_encoder();
	encode( input, output );
    printf("\nmin_cnt1 = %d ", min_cnt1 );
    flush_encoder( output );
    close_output( output );
    fclose( input );
    fclose( output );
}*/	/* compress */


int dmc2_uncompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

	//FILE	*input, *output;
	//long	file_length;

    //if ( ( input = fopen( *argv++, "rb" ) ) == NULL ){
		//printf("\n%s doesn't exist ", *(argv-1) );
		//exit( 1 );
	//}
	//output = fopen( *argv, "wb" );

	//file_length = get_file_length( input );
	initialize_model();			/* initialize the model */
	initialize_decoder( infile );
	decode( outsz, infile, outfile );
    //fclose( input );
    //fclose( output );

    return(outfile - out);
}	/* umcompress */

//#include    "main.c"		/* housekeeping */
