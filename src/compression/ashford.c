// modified by Luigi Auriemma

// the code was completely empty regarding author/license informations
// anyway the original author is Charles Ashford
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define far
#define farmalloc   malloc
#define farfree     free
#define movmem      memmove
static void CloseOutputFile(void) { }
static
unsigned char   *infile   = NULL,
                *infilel  = NULL,
                *outfile  = NULL,
                *outfilel = NULL;
static int ReadInputFile(void) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
static int WriteOutputFile(int chr) {
    if(outfile >= outfilel) return(-1);
    *outfile++ = chr;
    return(chr);
}


#define FAR_TABLES
#undef SPLIT_TABLES

/*  Define size of dictionary and other useful parameters  */

#define NDICT          28000U	/* Size of circular dictionary */
#define MAX_ORDER      8		/* Maximum order accomodated by the model */
#define MAX_CHAR_CODE  256      /* Number of symbols accepted by model */

#define HTBL1_SIZE     256		/* Hash table size - equals character set size */

#define MIN_STR        3		/* Minimum string length */
#define MAX_STR_CODE   64       /* Maximum code value for string length */

#define MAX_STR        (MIN_STR+MAX_STR_CODE+254)
#define MAX_STR_SAVE   (MIN_STR)
#define MAX_SYM        (MAX_CHAR_CODE + 2)

#define MAX_DICT       (NDICT+MAX_ORDER)
#define NIL_DICT_PTR   0

#define SWITCH_SYM     -1
#define END_OF_FILE    MAX_CHAR_CODE
#define START_STRING   (MAX_CHAR_CODE + 1)


int InitModel (int);
void CompressSymbol (int);
int ExpandSymbol (void);
void CloseModel (void);

#define MIN_RANGE 0x4001
#define MAX_RANGE 0xFFFF

struct coder_state

{
	unsigned low;
	unsigned range;
	int uflow;
	int bits;
	int fpos;
};


void InitCoder (void);
void CloseCoder (void);
void EncodeArith (unsigned, unsigned, unsigned);
void StartDecode (void);
int DecodeArith (unsigned);
void UpdateDecoder (unsigned, unsigned, unsigned);
void SaveCoderState (struct coder_state *);
void RestoreCoderState (struct coder_state *);
int CodeLength (struct coder_state *);
int ResetOutputPointer (int);

/*
	Declare variables use for arithmetic compression
*/

static int under_flow_count = 0;
static unsigned int low = 0;
static unsigned int range = MAX_RANGE;
static unsigned int value = 0;

static unsigned int code_value = 0;
static unsigned int code_bit_count = 0;
static unsigned codesize = 0;


static int shift_count (unsigned);

static void put_bit (int);
static void put_bit_string (unsigned, int);
static void clear_under_flow (unsigned);

static unsigned get_bit_string (int);
static unsigned get_bit (void);



/*
	Initialize arithmetic coder at start of any compress/expand procedure
*/

void InitCoder (void)

{
	low = 0;
	range = MAX_RANGE;
}



/*
	Terminate arithmetic code operation
	Write bit string whose value is within active code range
	Force final byte to be output to file
*/

void CloseCoder (void)

{
	int i;

	put_bit (1);
	for (i = 0; i < 7; i++) put_bit (0);
	CloseOutputFile ();
}



/*
	Save state of arithmetic coder
	Allows state to be restored at a later time
*/

void SaveCoderState (struct coder_state *p)

{
	p -> low = low;
	p -> range = range;
	p -> uflow = under_flow_count;
	p -> bits = code_bit_count;
	p -> fpos = codesize;
}

int ResetOutputPointer (int pos)
{
	return outfile[-pos];
}

/*
	Restore arithmetic coder state from saved value
	Repositin output file
	Set all internal values to their original state
*/

void RestoreCoderState (struct coder_state *p)

{
	int n;
	int cv;
	static unsigned char reset_mask [] =

	{
		0x00,  0x80,  0xC0,  0xE0,  0xF0,  0xF8,  0xFC,  0xFE,  0xFF,
	};

	n = codesize - p -> fpos;
	cv = ResetOutputPointer (n);

	low = p -> low;
	range = p -> range;

	under_flow_count = p -> uflow;
	code_bit_count = p -> bits;
	code_value = cv & reset_mask [code_bit_count];
	codesize -= n;
}


/*
	Estimate length of output code string
	Uses previously saved coder state
	Returns difference between saved position and current position
*/

int CodeLength (struct coder_state *csptr)

{
	int len;

	len = codesize - csptr -> fpos;
	len *= 8;
	len += under_flow_count - csptr -> uflow;
	len += code_bit_count - csptr -> bits;

	return len;
}


/*
	Arithmetic coder
	Encode symbol using input frequencies
	Output as many bits as possible to output file
	Update coder values for next symbol
*/

void EncodeArith (unsigned base, unsigned freq, unsigned cmax)

{
	unsigned long t;
	unsigned x;

	int n1, n2;

	t  = (long) range * (long) base;
	t += (long) base;
	low  += (unsigned) (t / cmax);
	x     = (unsigned) (t % cmax);

	t = (long) range * (long) freq;

	t += (long) (freq + x);
	t -= cmax;
	range = (unsigned) (t / cmax);

	n1 = shift_count ((low + range) ^ low);
	if (n1 && under_flow_count)
	{
		clear_under_flow ((low & 0x8000) != 0);
		low ^= 0x8000;
	}

	if (n1)
	{
		put_bit_string (low, n1);
		low <<= n1;
		range = ((range + 1) << n1) - 1;
	}

	if (range < MIN_RANGE)
	{
		n2 = shift_count (range - 1) - 1;

		under_flow_count += n2;
		low <<= n2;
		low &= 0x7FFF;
		range = ((range + 1) << n2) - 1;
	}
}


/*
	Send bit string based on underflow count
	Uses bit value followed by its complement

	generates: 0111... or 1000...
*/
	
static void clear_under_flow (unsigned bit)

{
	put_bit (bit);
	bit ^= 0x01;
	while (-- under_flow_count) put_bit (bit);
}



/*
	Send bit string
*/

static void put_bit_string (unsigned x, int count)

{
	while (count)
	{
		put_bit ((x & 0x8000) != 0);
		x <<= 1;
		count --;
	}
}



/*
	Write single bit to output file using internal buffer
*/

static void put_bit (int bit)

{
	static unsigned char mask [] =
	{
		0x80,	0x40,	0x20,	0x10,	0x08,	0x04,	0x02,	0x01,
	};

	if (bit) code_value |= mask [code_bit_count];
	if (++ code_bit_count == 8)
	{
		WriteOutputFile (code_value);
		code_value = 0;
		code_bit_count = 0;
		codesize ++;
	}
}



/*
	Determine number of leading zero bits on unsigned value
*/

static int shift_count (unsigned n)

{
	int i;

	i = 0;
	while ((n & 0x8000) == 0 && i < 16)
	{
		i ++;
		n <<= 1;
	}

	return i;
}



/*
	Read bit string from input stream
	Length is limited to word size
*/

static unsigned get_bit_string (int n)

{
	unsigned x = 0;

	while (n)
	{
		x <<= 1;
		x += get_bit ();
		n --;
	}

	return x;
}



/*
	Read a single bit from input stream
	Uses a one byte buffer for intermediate values
*/

static unsigned get_bit (void)

{
	int n;

	if (code_bit_count == 0)
	{
		code_bit_count = 8;
		n = ReadInputFile ();
		code_value = n >= 0 ? n : 0;
	}

	code_bit_count --;
	n = code_value & 0x80;
	code_value <<= 1;

	return n != 0;
}



/*
	Initialize arithmetic decode procedure
*/

void StartDecode (void)

{
	int i;

	value = 0;
	for (i = 0; i < 16; i ++)
	{
		value <<= 1;
		value |= get_bit ();
	}
}


/*
	Return value of next input code
	Input consists of total frequency for active symbol set
	Note that decoder must be updated with actual frequencies used
*/

int DecodeArith (unsigned cmax)

{
	unsigned long t;

	t = (long) (value - low + 1) * (long) cmax;
	t -= 1;
	t /= (long) range + 1;

	return (unsigned) t;
}


/*
	Update arithmetic decoder using actual symbol frequencies
	Read additional bits from input based on symbol values
*/

void UpdateDecoder (unsigned base, unsigned freq, unsigned cmax)

{
	unsigned long t;
	unsigned x, y;

	int n1, n2;

	t  = (long) range * (long) base;
	t += (long) base;
	x  = (unsigned) (t / cmax);
	y  = (unsigned) (t % cmax);

	low += x;

	t = (long) range * (long) freq;
	t += (long) (freq + y);
	t -= cmax;
	range = (unsigned) (t / cmax);

	n1 = shift_count ((low + range) ^ low);
	if (n1)
	{
		low <<= n1;
		range = ((range + 1) << n1) - 1;
		value <<= n1;
		value |= get_bit_string (n1);
	}

	if (range < MIN_RANGE)
	{
		n2 = shift_count (range - 1) - 1;
		value -= low;
		low <<= n2;
		low &= 0x7FFF;
		range = ((range + 1) << n2) - 1;
		value <<= n2;
		value |= get_bit_string (n2);
		value += low;
	}
}




#define MAX_CUM_FREQ   0x4000	/* maximum cumulative frequency */

struct model

{
	int initial_level;
	int match_level;

	unsigned base_count;
	unsigned sym_freq;

	unsigned order_cum_freq [MAX_ORDER + 2];
	unsigned order_sym_count [MAX_ORDER + 2];
};



/*
	Define circular dictionary large enough to hold complete
	set of active input symbols.

	Extra entries are included in the dictionary corresponding
	to a string of length equal to the maximum order to facilitate
	searching across the end of the dictionary.  This extra space
	is allocated at the front of the dictionary, so that the first
	few entries are never referenced directly.

	An additional table is allocated consisting of one word entries
	used to link equivalent dictionary entries where a hash table is
	used to locate the start of each search chain.

	The last character of the test string is used locate the
	initial hash table entry.  The hash table entries are stored
	as an extension to the link table.

	The link table is accessed through the use of macros to allow
	easy access to this table when its size excceds 64K.
*/

#ifndef FAR_TABLES

static unsigned char dict [MAX_DICT];
static unsigned char base_level [MAX_DICT];
static unsigned int next_dict [MAX_DICT + HTBL1_SIZE];

#define DICT_WORD_PTR(i) ((unsigned *) (&dict [i]));
#define NEXT_DICT(i) next_dict [i]

#else
static unsigned char far *dict = NULL;
static unsigned char far *base_level = NULL;
#define DICT_WORD_PTR(i) ((unsigned far *) (&dict [i]));

#ifdef SPLIT_TABLES

static unsigned int far *next_dict [2] = NULL;
#define NEXT_DICT(i) next_dict [i & 1] [i >> 1]

#else

static unsigned int far *next_dict = NULL;
#define NEXT_DICT(i) next_dict [i]

#endif
#endif


static unsigned int node;
static int new_symbol_count;
static unsigned int last_search_node;

static int max_order;
static int max_pos_size;

/*
	Define table of entries for each order giving number of symbols
	and total cumulative frequency for each order
*/

static struct model active_state;
static struct model last_state;


/*
	Define set of save areas for the state at the potential start any string
	The areas are used in rotation until a substring is matched equal to
	the minimum string length.  Normal symbol compression is performed
	during the time that the substring matches the input data.
		
	When the first non matching character is encountered, the lengths of
	the string and the compressed symbols are compared.  If the string
	length is less, the output is repositioned as specified by the
	state at the start of the string, and the string selection sequence
	overwrites the output.
*/

static struct

{
	int i;
	struct coder_state cs;
	struct model mod;
} save_state [MAX_STR_SAVE];


static int save_state_index;
static unsigned int string_pos;
static int string_len;
static int string_start;
static int skip_count;

/*
	States used while testing for text string replacements
*/

static enum
{
	STRING_WAIT,  STRING_SEARCH,  STRING_START,  STRING_ACTIVE,
	STRING_COMP,
} string_state;


/*
	Define combined set of tables giving the order and
	frequency count for each symbol encountered during dictionary scan
	Also accumulate base frequency values for each order
*/

static unsigned char level [MAX_SYM];
static unsigned int freq [MAX_SYM];
static unsigned int base_freq [MAX_ORDER + 2];

static unsigned int dup_count = 0;
static int dup_char = -1;

/*
	Build frequency table for zero and default orders
	Used to initialize state tables at start of each dictionary scan

	Always contains nonzero count for every symbol
	Includes order for each symbol selecting zero or default orders
	Initially set to all defaults with symbol frequency one
*/

static unsigned int sym_count_zero;
static unsigned int cum_freq_zero;
static unsigned int freq_zero [MAX_SYM];
static unsigned char order_zero [MAX_SYM];


/*
	Prototypes
*/

static void clear_context (void);
static void scan_dict (int);
static void scale_freq_tbl (int, int);
static void calc_symbol_freq (int);
static void select_output_symbol (void);
static int decode_symbol (void);
static void update_model (int);

static void encode_symbol (struct model *);
static void generate_switch_code (struct model *);
static void generate_symbol_code (struct model *);
static void generate_value (unsigned, unsigned);

static int start_decode_string (void);
static int decode_active_state (void);
static int decode_string_char (void);
static unsigned decode_value (unsigned);

static unsigned switch_char_freq (unsigned, unsigned);

static void delete_dict_entry (int);
static void test_string_state (void);
static void clear_text_string (void);

static void check_string_cont (void);
static void test_string_start (unsigned pos, int n);
static void test_string_resume (unsigned pos, int n);
static void replace_text_string (void);

static int log2x (unsigned n);
static void scale_binary_value (long, int *, unsigned *);
static void update_bit_len (unsigned, unsigned, int *, unsigned *);
static int find_string_len (void);



/*
	Initialize model at start of compression or expansion
	Alocate and initialize tables
*/

int InitModel (int n)

{
	unsigned i;

	InitCoder ();

	active_state.initial_level = 1;
	node = MAX_ORDER;
	max_pos_size = log2x (NDICT - 1) + 1;
	max_order = n;
	if (max_order == 0) max_order = 1;

	sym_count_zero = 0;
	cum_freq_zero = 0;

#ifdef FAR_TABLES

	if(!dict) dict = farmalloc (MAX_DICT * sizeof (unsigned char));
	if(!base_level) base_level = farmalloc (MAX_DICT * sizeof (unsigned char));

#ifndef SPLIT_TABLES

	if(!next_dict) next_dict = farmalloc ((MAX_DICT + HTBL1_SIZE) * sizeof (unsigned int));

	if (dict == NULL || base_level == NULL || next_dict == NULL)
	{
		//printf ("Memory allocation failure\n");
		return(-1);
	}

#else

	if(!next_dict [0]) next_dict [0] = farmalloc ((MAX_DICT + HTBL1_SIZE + 1) / 2 * sizeof (unsigned int));
	if(!next_dict [1]) next_dict [1] = farmalloc ((MAX_DICT + HTBL1_SIZE + 1) / 2 * sizeof (unsigned int));

	if (dict == NULL || base_level == NULL || next_dict [1] == NULL)
	{
		//printf ("Memory allocation failure\n");
		return(-1);
	}

	#endif  /* SPLIT_TABLES */

#endif	/* FAR_TABLES */

	for (i = 0; i < MAX_SYM; i ++)
	{
		freq_zero [i] = 1;
		order_zero [i] = 0;
	}

	for (i = 0; i < MAX_DICT + HTBL1_SIZE; i ++)
		NEXT_DICT (i) = NIL_DICT_PTR;

	for (i = 0; i < MAX_DICT; i ++)
	{
		dict [i] = 0;
		base_level [i] = 0;
	}

	save_state_index = 0;
	string_pos = 0;
	string_len = 0;
	string_start = 0;
	skip_count = MIN_STR;
	string_state = STRING_WAIT;
    return(0);
}




/*
	Compress next symbol
*/

void CompressSymbol (int ch)

{
	int i;
	unsigned cfreq;

	if (string_state == STRING_ACTIVE) check_string_cont ();

	if (dup_count > (unsigned int)(max_order + 2))
	{
		++ active_state.order_cum_freq [max_order + 1];
		++ freq [dup_char];

		if (ch != dup_char)
		{
			for (i = 0, cfreq = 0; i < ch; i ++)
			{
				if (level [i] == level [ch]) cfreq += freq [i];
			}
			
			base_freq [level [ch]] = cfreq;
		}
	}
	else
	{
		clear_context ();
		for (i = 0; i < max_order + 2; i ++) base_freq [i] = 0;
		scan_dict (ch);
	}

	for (i = 1; i <= active_state.initial_level; i ++)
	{
		while (active_state.order_cum_freq [i] > MAX_CUM_FREQ)
			scale_freq_tbl (i, ch);
	}

	test_string_state ();

	calc_symbol_freq (ch);
	select_output_symbol ();
	update_model (ch);
}



/*
	Expand next symbol from input stream
*/

int ExpandSymbol (void)

{
	int ch;

	if (dup_count > (unsigned int)(max_order + 2))
	{
		++ active_state.order_cum_freq [max_order + 1];
		++ freq [dup_char];
	}
	else
	{
		clear_context ();
		scan_dict (0);
	}

	ch = string_len == 0 ? decode_symbol () : decode_string_char ();
	update_model (ch);
	
	return ch;
}



/*
	Update tables used by model for new symbol
	Link new symbol into dictionary
	Delete oldest symbol in dictionary, if required
	Update zero order frequencies if no higher level context found
*/

static void update_model (int ch)

{
	int n;

	NEXT_DICT (node) = NIL_DICT_PTR;
	NEXT_DICT (last_search_node) = node;
	last_search_node = node;

	if (active_state.match_level < 2)
	{
		cum_freq_zero ++;
		if (order_zero [ch])
			freq_zero [ch] ++;
		else
		{
			order_zero [ch] = 1;
			sym_count_zero ++;
		}
	}

	dict [node] = ch;
	if (ch == dup_char)
		dup_count ++;
	else
	{
		dup_char = ch;
		dup_count = 0;
	}

	n = active_state.match_level;
	if (n == 0) n = 1;
	base_level [node] = n;

	delete_dict_entry (node + max_order);

	active_state.initial_level = active_state.match_level;
	if (active_state.initial_level <= max_order)
		active_state.initial_level ++;

	if (++ node == MAX_DICT)
	{
		node = MAX_ORDER;
		for (n = 1; n <= max_order; n ++)
			dict [MAX_ORDER - n] = dict [MAX_DICT - n];
	}
}



/*
	Delete oldest symbol from dictionary
	Update frequency counts if added for lower order
*/

static void delete_dict_entry (int i)

{
	unsigned int n;
	int j;

	n = i;
	if (n >= MAX_DICT) n -= NDICT;

	switch (base_level [n])
	{
		case 0:
			break;

		case 1:
			j = dict [n];
			cum_freq_zero --;
			if (-- freq_zero [j] == 0)
			{
				order_zero [j] = 0;
				freq_zero [j] = 1;
				sym_count_zero --;
			}

		default:
			j = dict [n - 1];
			NEXT_DICT (j + MAX_DICT) = NEXT_DICT (n);
			break;
	}
}


/*
	Initialize tables at start of dictionary scan
	Establish counts based on low order tables
*/

static void clear_context (void)

{
	int i;

	for (i = 2; i < max_order + 2; i ++)
	{
		active_state.order_sym_count [i] = 0;
		active_state.order_cum_freq [i] = 0;
	}

	active_state.order_sym_count [1] = sym_count_zero;
	active_state.order_sym_count [0] = MAX_SYM - sym_count_zero;

	active_state.order_cum_freq [1] = cum_freq_zero;
	active_state.order_cum_freq [0] = MAX_SYM - sym_count_zero;

	movmem (freq_zero, freq, sizeof freq);
	movmem (order_zero, level, sizeof level);
}



/*
	Perform search of dictionary to locate active contexts
	Accumulate freqencies for all symbols encounered
	Use initial character of input string to select the starting table value
*/

static void scan_dict (int base_char)

{
	unsigned int k;
	unsigned int jnode;
	int ch;
	int n;

	last_search_node = dict [node - 1] + MAX_DICT;
	jnode = NEXT_DICT (last_search_node);

	while (jnode != NIL_DICT_PTR)
	{
		ch = dict [jnode];
		for (n = 2; n < max_order + 1; n ++)
		{
			if (dict [node - n] != dict [jnode - n]) break;
		}

		switch (string_state)
		{
			case STRING_SEARCH:
			case STRING_START:
				test_string_start (jnode, n);
				break;

			case STRING_COMP:
				test_string_resume (jnode, n);
				break;
            default: break;
		}

		if (base_level [jnode] <= n)
		{
			k = level [ch];
			if (k < (unsigned int)n)
			{
				active_state.order_cum_freq [k] -= freq [ch];
				active_state.order_sym_count [k] --;

				active_state.order_cum_freq [n] ++;
				active_state.order_sym_count [n] ++;

				if (ch < base_char)
				{
					base_freq [k] -= freq [ch];
					base_freq [n] ++;
				}

				level [ch] = n;
				freq [ch] = 1;

				if (n > active_state.initial_level) active_state.initial_level = n;
			}
			else
			if (k == (unsigned int)n)
			{
				active_state.order_cum_freq [n] ++;
				freq [ch] ++;
				if (ch < base_char) base_freq [n] ++;
			}
		}

		last_search_node = jnode;
		jnode = NEXT_DICT (jnode);
	}
}



/*
	Test for continued text substring
	Executed before the start of each dictionary scan during compression
*/

static void check_string_cont (void)

{
	int j;

	j = string_pos + string_len;
	if (j >= (int)MAX_DICT) j -= NDICT;

	if (dict [node - 1] == dict [j])
		string_len ++;
	else
		string_state = STRING_COMP;
}



/*
	Test for start of potential text substring
	Executed during dictionary scan based on string state variable
*/

static void test_string_start (unsigned pos, int n)

{
	if (n > MIN_STR)
	{
		string_pos = pos;
		if (string_pos < MIN_STR + MAX_ORDER) string_pos += NDICT;
		string_pos -= MIN_STR;
		string_len = MIN_STR;
		string_state = STRING_START;
	}
}



static void test_string_resume (unsigned pos, int n)

{
	if (n > string_len + 1)
	{
		string_state = STRING_ACTIVE;
		string_len ++;
		string_pos = pos;
		if (string_pos < (unsigned int)(string_len + MAX_ORDER)) string_pos += NDICT;
		string_pos -= string_len;
	}
	else
	if (n == max_order + 1)
	{
		unsigned i2, j2, n2;

		i2 = node - max_order - 1;
		j2 = pos - max_order - 1;

		for (n2 = max_order + 1; n2 <= (unsigned)(string_len + 1); n2 ++)
		{
			if (i2 < MAX_ORDER) i2 += NDICT;
			if (j2 < MAX_ORDER) j2 += NDICT;
			if (dict [i2 --] != dict [j2 --]) break;
		}

		if (n2 > (unsigned)(string_len + 1))
		{
			string_state = STRING_ACTIVE;
			string_len ++;
			string_pos = pos;
			if (string_pos < (unsigned)(string_len + MAX_ORDER)) string_pos += NDICT;
			string_pos -= string_len;
		}
	}
}



/*
	Test status of text substring match procedure
	Performed after each dictionary scan
*/

static void test_string_state (void)

{
	switch (string_state)
	{
		case STRING_WAIT:
			save_state [string_start].mod = active_state;
			SaveCoderState (&save_state [string_start].cs);

			if (++ string_start == MAX_STR_SAVE) string_start = 0;
			if (-- skip_count == 0) string_state = STRING_SEARCH;
			break;

		case STRING_SEARCH:
			save_state [string_start].mod = active_state;
			SaveCoderState (&save_state [string_start].cs);
			if (++ string_start == MAX_STR_SAVE) string_start = 0;
			break;

		case STRING_ACTIVE:
			if (string_len > MAX_STR)
			{
				string_len --;
				clear_text_string ();
			}
			break;

		case STRING_COMP:
			clear_text_string ();
			break;

        default: break;
	}
}



/*
	End of text substring
	Test for minimum code length of dtring versus coded symbols
	Reposition output and write string selection if less
	Set up for start of next string
*/

static void clear_text_string (void)

{
	int nbits;
	int i;

	if (string_len >= MIN_STR)
	{
		nbits = find_string_len ();
		if (nbits > 0)
		{
			replace_text_string ();

			i = node;
			if (i <= string_len) i += NDICT;
			i -= string_len + 1;
		}
	}

	save_state [string_start].mod = last_state;
	SaveCoderState (&save_state [string_start].cs);

	if (++ string_start == MAX_STR_SAVE) string_start = 0;

	encode_symbol (&last_state);

	save_state [string_start].mod = active_state;
	SaveCoderState (&save_state [string_start].cs);

	if (++ string_start == MAX_STR_SAVE) string_start = 0;
	skip_count = MIN_STR - 2;
	string_len = 0;

	string_state = STRING_WAIT;
}



/*
	Estimate size of string reference
	Returns size relative to actual length used by coded symbols
*/

static int find_string_len (void)

{
	int nbits;
	unsigned f;

	struct model start_context;
	unsigned i;
	unsigned n;
	unsigned j;
	int m;
	unsigned c1;

	nbits = 0;
	f = 0;

	new_symbol_count = MAX_SYM;
	m = save_state [string_start].mod.match_level;
	start_context = save_state [string_start].mod;

/*
	Accumulate switch characters for default context
	Include start of string symbol
*/

	do
	{
		new_symbol_count -= start_context.order_sym_count [m];

		c1 = start_context.order_cum_freq [m];
		n = switch_char_freq (start_context.order_sym_count [m], c1);

		update_bit_len (n, c1 + n, &nbits, &f);
	} while (-- m > 0);

	c1 = start_context.order_cum_freq [0];
	update_bit_len (1, c1, &nbits, &f);

/*
	Include string length
*/

	nbits += 6;
	if (string_len - MIN_STR >= 63) nbits += 8;

/*
	Calculate relative location of start of string
	Output as bit count and offset
*/

	update_bit_len (1, max_pos_size, &nbits, &f);

	i = string_pos + string_len;
	if (i >= MAX_DICT) i -= NDICT;
	j = i < node ? node - i - 1 : node + NDICT - 1 - i;
	nbits += log2x (j);

	return CodeLength (&save_state [string_start].cs) - nbits;
}



/*
	Generate coded symbols for text substring
	Used when string length is less length used by actual symbols
*/

static void replace_text_string (void)

{
	struct model start_context;
	unsigned n;
	unsigned i, j;

	RestoreCoderState (&save_state [string_start].cs);

	new_symbol_count = MAX_SYM;
	start_context = save_state [string_start].mod;

/*
	Output switch codes for default context and start string symbol
*/

	start_context.match_level = 0;
	start_context.base_count = start_context.order_cum_freq [0] - 1;
	start_context.sym_freq = 1;

	encode_symbol (&start_context);

/*
	Output string length
*/
	n = string_len - MIN_STR;
	if (n < MAX_STR_CODE - 1)
		generate_value (n, MAX_STR_CODE);
	else
	{
		generate_value (MAX_STR_CODE - 1, MAX_STR_CODE);
		generate_value (n + 1 - MAX_STR_CODE, 256);
	}

/*
	Determine relative location of start of string
*/

	i = string_pos + string_len;
	if (i >= MAX_DICT) i -= NDICT;
	j = i < node ? node - i - 1 : node - i - 1 + NDICT;
	n = 2;
	if (j < 2)
		i = 0;
	else
	{
		for (i = 1; 2 * n <= j && n < 0x8000; i ++, n <<= 1);
		j -= n;
	}

/*
	Output bit length of relative string location
*/

	generate_value (i, max_pos_size);

/*
	Output string location in 8 bit pices if required
*/

	if (i > 8)
	{
		generate_value (j & 0xFF, 256);
		j >>= 8;
		n >>= 8;
	}

	generate_value (j, n);
}


/*
	Scale frequency tables if total cumulative frequency exceeds maximum
	Also recalculates frequencies for current input symbol
*/

static void scale_freq_tbl (int order, int ch)

{
	int i;
	unsigned cfreq;
	unsigned t;

	i = 0;
	cfreq = 0;

	if (level [ch] == order)
	{
		for (; i < ch; i ++)
		{
			if (level [i] == order)
			{
				t = (freq [i] + 1) >> 1;
				freq [i] = t;
				cfreq += t;
			}
		}

		base_freq [order] = cfreq;

		t = (freq [i] + 1) >> 1;
		freq [i] = t;
		i ++;

		cfreq += t;
	}

	for (; i < MAX_CHAR_CODE; i ++)
	{
		if (level [i] == order)
		{
			t = (freq [i] + 1) >> 1;
			freq [i] = t;
			cfreq += t;
		}
	}

	active_state.order_cum_freq [order] = cfreq;
}


/*
	Determine symbol frequency counts for coder
*/

static void calc_symbol_freq (int ch)

{
	int i;

	active_state.match_level = level [ch];
	active_state.sym_freq = freq [ch];

	if (active_state.match_level > 1)
		active_state.base_count = base_freq [active_state.match_level];
	else
	{
		active_state.base_count = 0;
		for (i = 0; i < ch; i ++)
		{
			if (level [i] == active_state.match_level)
				active_state.base_count += freq [i];
		}
	}
}


/*
	Select state structure containing symbol to be output
	Generate coded symbol value if required

	Note that there is a one character delay during substring matching
	so that the non matching character at end of string is not output
	until after string replacement has occurred
*/

static void select_output_symbol (void)

{
	switch (string_state)
	{
		case STRING_START:
			last_state = active_state;
			string_state = STRING_ACTIVE;
			break;

		case STRING_ACTIVE:
			encode_symbol (&last_state);
			last_state = active_state;
			break;

		default:
			encode_symbol (&active_state);
			break;
	}
}


/*
	Generate coded value for symbol defined by input state variable
	State includes symbol frequencies and all values used by coder
*/

static void encode_symbol (struct model *cptr)

{
	int i;

	new_symbol_count = MAX_SYM;
	i = cptr -> match_level;
	while (i < cptr -> initial_level)
	{
		new_symbol_count -= cptr -> order_sym_count [cptr -> initial_level];
		generate_switch_code (cptr);
		cptr -> initial_level --;
	}

	new_symbol_count -= cptr -> order_sym_count [i];
	generate_symbol_code (cptr);
}


/*
	Generate code for switch character to select next lower context
	Input consists of state variable for symbol
*/

static void generate_switch_code (struct model *cptr)

{
	unsigned c1;
	unsigned n, m;

	n = cptr -> initial_level;
	c1 = cptr -> order_cum_freq [n];
	m = switch_char_freq (cptr -> order_sym_count [n], c1);

	EncodeArith (c1, m, c1 + m);
}


/*
	Generate code for symbol defined by input state variable
*/

static void generate_symbol_code (struct model *cptr)

{
	unsigned int c1, c2, c3;
	int n;

	n = cptr -> initial_level;
	c1 = cptr -> base_count;
	c2 = cptr -> sym_freq;
	c3 = cptr -> order_cum_freq [n];

	if (n > 0) c3 += switch_char_freq (cptr -> order_sym_count [n], c3);

	EncodeArith (c1, c2, c3);
}



/*
	Estimate frequency to be allocated to the switch character
	Use number of symbols referenced in current context and the
	number of unreferenced symbols so far

	Value should reflect probability of encountering a symbol
	already present in the active context
*/

static unsigned switch_char_freq (unsigned scount, unsigned cmax)

{	unsigned n;

	n = (scount + 1) * new_symbol_count / (scount + new_symbol_count);
	if (n + cmax > MAX_CUM_FREQ) n = MAX_CUM_FREQ + 1 - cmax;

	return n;
}


/*
	End of compression procedure
	Terminate active substring processing
	Close arithmetic coder and flush output
*/

void CloseModel (void)

{
	if (string_state == STRING_ACTIVE) clear_text_string ();
	CloseCoder ();
}



/*
	Decode next input symbol from input stream
	Test for context switch and repeat until non switch symbol encountered
*/

static int decode_symbol (void)

{
	int ch;

	new_symbol_count = MAX_SYM;
	active_state.match_level = active_state.initial_level;

	new_symbol_count -= active_state.order_sym_count [active_state.match_level];
	ch = decode_active_state ();

	while (ch == SWITCH_SYM)
	{
		active_state.match_level --;
		new_symbol_count -= active_state.order_sym_count [active_state.match_level];
		ch = decode_active_state ();
	}

	if (ch == START_STRING) ch = start_decode_string ();

	return ch;
}



/*
	Start of string symbol encountered
	Extract string length and position from input stream
	Returns first character in string
*/

static int start_decode_string (void)

{
	unsigned i, j, k;
	unsigned n;

	string_len = decode_value (MAX_STR_CODE);
	if (string_len == MAX_STR_CODE - 1) string_len += decode_value (256);
	string_len += MIN_STR;

	i = decode_value (max_pos_size);
	if (i == 0)
	{
		i = 2;
		j = decode_value (2);
	}
	else
	if (i > 8)
	{
		j = decode_value (256);
		i -= 8;
		n = 1 << i;
		k = decode_value (n);
		k += n;
		k <<= 8;
		j += k;
	}
	else
	{
		n = 1 << i;
		j = decode_value (n);
		j += n;
	}

	string_pos = node < j + MAX_ORDER ? node + NDICT - j : node - j;

	return decode_string_char ();
}



/*
	Locate next character in text substring
	Increment string pointers and decrement length
	Set parameters used for updating state variable
*/

static int decode_string_char (void)

{
	int ch;
	unsigned int j;

	ch = dict [string_pos];
	active_state.match_level = level [ch];

	string_len --;
	if (++ string_pos == MAX_DICT) string_pos = MAX_ORDER;

	last_search_node = dict [node - 1] + MAX_DICT;
	j = NEXT_DICT (last_search_node);

	while (j != NIL_DICT_PTR)
	{
		last_search_node = j;
		j = NEXT_DICT (j);
	}

	return ch;
}


/*
	Extract next value from input stream
	Search frequency tables to convert to symbol
	Update arithmetic decoder using symbol frequencies
*/

static int decode_active_state (void)

{
	unsigned c1, c2, c3;
	unsigned sym;
	unsigned m;
	int i;
	int n;

	n = active_state.match_level;
	c2 = active_state.order_cum_freq [n];

	while (c2 > MAX_CUM_FREQ)
	{
	 	scale_freq_tbl (n, END_OF_FILE);
		c2 = active_state.order_cum_freq [n];
	}

	m = n > 0 ? switch_char_freq (active_state.order_sym_count [n], c2) : 0;
	c3 = c2 + m;

	sym = DecodeArith (c3);
	if (sym < c2)
	{
		c1 = 0;
		for (i = 0; i < MAX_SYM; i ++)
		{
			if (level [i] == n)
			{
				m = freq [i];
				c1 += m;
				if (sym < c1) break;
			}
		}

		c1 -= m;
	}
	else
	{
		c1 = c2;
		i = SWITCH_SYM;
	}

	UpdateDecoder (c1, m, c3);

	return i;
}



/*
	Generate coded output for a constant value within a fixed range
	Value is treated as a symbol with frequency one
*/

static void generate_value (unsigned valuex, unsigned rangex)

{
	EncodeArith (valuex, 1, rangex);
}


/*
	Extract constant value within fixed range
	Each possible value is treated as symbol with frequency one
*/

static unsigned decode_value (unsigned rangex)

{	unsigned valuex;

	valuex = DecodeArith (rangex);
	UpdateDecoder (valuex, 1, rangex);

	return valuex;
}



/*
	Determine integer value of base 2 logarithm of integer value
	Use smallest power of two less than input value
*/

static int log2x (unsigned n)

{
	int i;

	for (i = 0; n > 1; i ++, n >>= 1);
	return i;
}



/*
	Scale binary value as part of log calulation
	Input is a binary fraction (32 bits, 16 binary places)
	Extract integer log to base 2
	Return fractional part and accumulate integer part
*/

static void scale_binary_value (long x, int *nbits, unsigned *frac)

{
	int i;

	i = log2x ((unsigned) (x >> 16));
	*nbits += i;
	*frac = (unsigned) (x >> i);
}


/*
	Estimate bit size of arithmetic coded values

	Input consists of symbol frequency (p) and cumulative frequency (q)
	Actual bit size is log to base 2 of (q / p)
	
	Maintain intermediate values as:
	
		n + log2x (1 + f)

	where n is integer and f is the remainder (between 0 and 1.0)
	stored as a 16 bit binary fraction.

	The pair (nbits, frac) contains the starting value for the bit length
	This is updated to include the latest symbol size
*/

static void update_bit_len (unsigned p, unsigned q, int *nbits, unsigned *frac)

{
	long x;
	unsigned f2;

	x = ((long) q << 16) / p;
	scale_binary_value (x, nbits, &f2);

	x = (long) f2 * (long) *frac;
	x >>= 16;
	x += f2;
	x += *frac;
	x += 0x10000L;
	scale_binary_value (x, nbits, frac);
}



int unashford(unsigned char *in, int insz, unsigned char *out, int outsz) {
	int ch;

    // necessary for the next runs?
    under_flow_count = 0;
    low = 0;
    range = MAX_RANGE;
    value = 0;
    code_value = 0;
    code_bit_count = 0;
    codesize = 0;

    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;

	if(InitModel (MAX_ORDER) < 0) return(-1);
	StartDecode ();

    for(;;) {
        ch = ExpandSymbol ();
        if(ch == END_OF_FILE) break;
		WriteOutputFile (ch);
    }
    return(outfile - out);
}

