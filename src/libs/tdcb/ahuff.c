/************************** Start of AHUFF.C *************************
 *
 * This is the adaptive Huffman coding module used in Chapter 4.
 * Compile with BITIO.C, ERRHAND.C, and either MAIN-C.C or MAIN-E.C
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mn_incs.h"

//char *CompressionName = "Adaptive Huffman coding, with escape codes";
//char *Usage           = "infile outfile [ -d ]";

#define END_OF_STREAM     256
#define ESCAPE            257
#define SYMBOL_COUNT      258
#define NODE_TABLE_COUNT  ( ( SYMBOL_COUNT * 2 ) - 1 )
#define ROOT_NODE         0
#define MAX_WEIGHT        0x8000
#define TRUE              1
#define FALSE             0

/*
 * This data structure is all that is needed to maintain an adaptive
 * Huffman tree for both encoding and decoding.  The leaf array is a
 * set of indices into the nodes that indicate which node is the
 * parent of a symbol.  For example, to encode 'A', we would find the
 * leaf node by way of leaf[ 'A' ].  The next_free_node index is used
 * to tell which node is the next one in the array that can be used.
 * Since nodes are allocated when characters are read in for the first
 * time, this pointer keeps track of where we are in the node array.
 * Finally, the array of nodes is the actual Huffman tree.  The child
 * index is either an index pointing to a pair of children, or an
 * actual symbol value, depending on whether 'child_is_leaf' is true
 * or false.
 */

typedef struct tree {
    int leaf[ SYMBOL_COUNT ];
    int next_free_node;
    struct node {
        unsigned int weight;
        int parent;
        int child_is_leaf;
        int child;
    } nodes[ NODE_TABLE_COUNT ];
} TREE;

/*
 * The Tree used in this program is a global structure.  Under other
 * circumstances it could just as well be a dynamically allocated
 * structure built when needed, since all routines here take a TREE
 * pointer as an argument.
 */

//static TREE Tree;

/*
 * Function prototypes for both ANSI C compilers and their K&R brethren.
 */

#ifdef __STDC__

void ahuff_CompressFile( memory_file *input, BIT_FILE *output, int argc, char *argv[] );
void ahuff_ExpandFile( BIT_FILE *input, memory_file *output, int argc, char *argv[] );
void ahuff_InitializeTree( TREE *tree );
void ahuff_EncodeSymbol( TREE *tree, unsigned int c, BIT_FILE *output );
int ahuff_DecodeSymbol( TREE *tree, BIT_FILE *input );
void ahuff_UpdateModel( TREE *tree, int c );
void ahuff_RebuildTree( TREE *tree );
void ahuff_swap_nodes( TREE *tree, int i, int j );
void ahuff_add_new_node( TREE *tree, int c );
void ahuff_PrintTree( TREE *tree );
void ahuff_ahuff_print_codes( TREE *tree );
void ahuff_print_code( TREE *tree, int c );
void ahuff_calculate_rows( TREE *tree, int node, int level );
int ahuff_calculate_columns( TREE *tree, int node, int starting_guess );
int ahuff_find_minimum_column( TREE *tree, int node, int max_row );
void ahuff_rescale_columns( int factor );
void ahuff_print_tree( TREE *tree, int first_row, int last_row );
void ahuff_print_connecting_lines( TREE *tree, int row );
void ahuff_print_node_numbers( int row );
void ahuff_print_weights( TREE *tree, int row );
void ahuff_print_symbol( TREE *tree, int row );

#else

void ahuff_CompressFile();
void ahuff_ExpandFile();
void ahuff_InitializeTree();
void ahuff_EncodeSymbol();
int ahuff_DecodeSymbol();
void ahuff_UpdateModel();
void ahuff_RebuildTree();
void ahuff_swap_nodes();
void ahuff_add_new_node();
void ahuff_PrintTree();
void ahuff_ahuff_print_codes();
void ahuff_print_code();
void ahuff_calculate_rows();
int ahuff_calculate_columns();
void ahuff_rescale_columns();
void ahuff_print_tree();
void ahuff_print_connecting_lines();
void ahuff_print_node_numbers();
void ahuff_print_weights();
void ahuff_print_symbol();

#endif

/*
 * The high level view of the compression routine is very simple.
 * First, we initialize the Huffman tree, with just the ESCAPE and
 * END_OF_STREAM symbols.  Then, we sit in a loop, encoding symbols,
 * and adding them to the model.  When there are no more characters
 * to send, the special END_OF_STREAM symbol is encoded.  The decoder
 * will later be able to use this symbol to know when to quit.
 *
 * This routine will accept a single additional argument.  If the user
 * passes a "-d" argument, the function will dump out the Huffman tree
 * to stdout when the program is complete.  The code to accomplish this
 * is thrown in as a bonus., and not documented in the book.
 */
/*
void ahuff_CompressFile( input, output, argc, argv )
memory_file *input;
BIT_FILE *output;
int argc;                
char *argv[];
{
    int c;

    ahuff_InitializeTree( &Tree );
    while ( ( c = mn_getc( input ) ) != EOF ) {
        ahuff_EncodeSymbol( &Tree, c, output );
        ahuff_UpdateModel( &Tree, c );
    }
    ahuff_EncodeSymbol( &Tree, END_OF_STREAM, output );
    while ( argc-- > 0 ) {
        if ( strcmp( *argv, "-d" ) == 0 )
            ahuff_PrintTree( &Tree );
        else
            printf( "Unused argument: %s\n", *argv );
        argv++;
    }
}*/

/*
 * The Expansion routine looks very much like the compression routine.
 * It first initializes the Huffman tree, using the same routine as
 * the compressor did.  It then sits in a loop, decoding characters and
 * updating the model until it reads in an END_OF_STREAM symbol.  At
 * that point, it is time to quit.
 *
 * This routine will accept a single additional argument.  If the user
 * passes a "-d" argument, the function will dump out the Huffman tree
 * to stdout when the program is complete.
 */
/*
void ahuff_ExpandFile( input, output, argc, argv )
BIT_FILE *input;
memory_file *output;
int argc;
char *argv[];
{
*/
QUICK_EXPAND(ahuff)
    TREE Tree;

    int c;

    ahuff_InitializeTree( &Tree );
    while ( ( c = ahuff_DecodeSymbol( &Tree, input ) ) != END_OF_STREAM ) {
        if ( mn_putc( c, &output ) == EOF )
            fatal_error( "Error writing character" );
        ahuff_UpdateModel( &Tree, c );
    }
    while ( argc-- > 0 ) {
        if ( strcmp( *argv, "-d" ) == 0 )
            ahuff_PrintTree( &Tree );
        else
            printf( "Unused argument: %s\n", *argv );
        argv++;
    }
    CloseInputBitFile(input);
    return(output.data - out);
}

/*
 * When performing adaptive compression, the Huffman tree starts out
 * very nearly empty.  The only two symbols present initially are the
 * ESCAPE symbol and the END_OF_STREAM symbol.  The ESCAPE symbol has to
 * be included so we can tell the expansion prog that we are transmitting a
 * previously unseen symbol.  The END_OF_STREAM symbol is here because
 * it is greater than eight bits, and our ESCAPE sequence only allows for
 * eight bit symbols following the ESCAPE code.
 *
 * In addition to setting up the root node and its two children, this
 * routine also initializes the leaf array.  The ESCAPE and END_OF_STREAM
 * leaf elements are the only ones initially defined, the rest of the leaf
 * elements are set to -1 to show that they aren't present in the
 * Huffman tree yet.
 */

void ahuff_InitializeTree( tree )
TREE *tree;
{
    int i;

    tree->nodes[ ROOT_NODE ].child             = ROOT_NODE + 1;
    tree->nodes[ ROOT_NODE ].child_is_leaf     = FALSE;
    tree->nodes[ ROOT_NODE ].weight            = 2;
    tree->nodes[ ROOT_NODE ].parent            = -1;

    tree->nodes[ ROOT_NODE + 1 ].child         = END_OF_STREAM;
    tree->nodes[ ROOT_NODE + 1 ].child_is_leaf = TRUE;
    tree->nodes[ ROOT_NODE + 1 ].weight        = 1;
    tree->nodes[ ROOT_NODE + 1 ].parent        = ROOT_NODE;
    tree->leaf[ END_OF_STREAM ]                = ROOT_NODE + 1;

    tree->nodes[ ROOT_NODE + 2 ].child         = ESCAPE;
    tree->nodes[ ROOT_NODE + 2 ].child_is_leaf = TRUE;
    tree->nodes[ ROOT_NODE + 2 ].weight        = 1;
    tree->nodes[ ROOT_NODE + 2 ].parent        = ROOT_NODE;
    tree->leaf[ ESCAPE ]                       = ROOT_NODE + 2;

    tree->next_free_node                       = ROOT_NODE + 3;

    for ( i = 0 ; i < END_OF_STREAM ; i++ )
        tree->leaf[ i ] = -1;
}

/*
 * This routine is responsible for taking a symbol, and converting
 * it into the sequence of bits dictated by the Huffman tree.  The
 * only complication is that we are working are way up from the leaf
 * to the root, and hence are getting the bits in reverse order.  This
 * means we have to rack up the bits in an integer and then send them
 * out after they are all accumulated.  In this version of the program,
 * we keep our codes in a long integer, so the maximum count is set
 * to an arbitray limit of 0x8000.  It could be set as high as 65535
 * if desired.
 */

void ahuff_EncodeSymbol( tree, c, output )
TREE *tree;
unsigned int c;
BIT_FILE *output;
{
    unsigned long code;
    unsigned long current_bit;
    int code_size;
    int current_node;

    code = 0;
    current_bit = 1;
    code_size = 0;
    current_node = tree->leaf[ c ];
    if ( current_node == -1 )
        current_node = tree->leaf[ ESCAPE ];
    while ( current_node != ROOT_NODE ) {
        if ( ( current_node & 1 ) == 0 )
            code |= current_bit;
        current_bit <<= 1;
        code_size++;
        current_node = tree->nodes[ current_node ].parent;
    };
    OutputBits( output, code, code_size );
    if ( tree->leaf[ c ] == -1 ) {
        OutputBits( output, (unsigned long) c, 8 );
        ahuff_add_new_node( tree, c );
    }
}

/*
 * Decoding symbols is easy.  We start at the root node, then go down
 * the tree until we reach a leaf.  At each node, we decide which
 * child to take based on the next input bit.  After getting to the
 * leaf, we check to see if we read in the ESCAPE code.  If we did,
 * it means that the next symbol is going to come through in the next
 * eight bits, unencoded.  If that is the case, we read it in here,
 * and add the new symbol to the table.
 */

int ahuff_DecodeSymbol( tree, input )
TREE *tree;
BIT_FILE *input;
{
    int current_node;
    int c;

    current_node = ROOT_NODE;
    while ( !tree->nodes[ current_node ].child_is_leaf ) {
        current_node = tree->nodes[ current_node ].child;
        current_node += InputBit( input );
    }
    c = tree->nodes[ current_node ].child;
    if ( c == ESCAPE ) {
        c = (int) InputBits( input, 8 );
        ahuff_add_new_node( tree, c );
    }
    return( c );
}

/*
 * ahuff_UpdateModel is called to increment the count for a given symbol.
 * After incrementing the symbol, this code has to work its way up
 * through the parent nodes, incrementing each one of them.  That is
 * the easy part.  The hard part is that after incrementing each
 * parent node, we have to check to see if it is now out of the proper
 * order.  If it is, it has to be moved up the tree into its proper
 * place.
 */
void ahuff_UpdateModel( tree, c )
TREE *tree;
int c;
{
    int current_node;
    int new_node;

    if ( tree->nodes[ ROOT_NODE].weight == MAX_WEIGHT )
        ahuff_RebuildTree( tree );
    current_node = tree->leaf[ c ];
    while ( current_node != -1 ) {
        tree->nodes[ current_node ].weight++;
        for ( new_node = current_node ; new_node > ROOT_NODE ; new_node-- )
            if ( tree->nodes[ new_node - 1 ].weight >=
                 tree->nodes[ current_node ].weight )
                break;
        if ( current_node != new_node ) {
            ahuff_swap_nodes( tree, current_node, new_node );
            current_node = new_node;
        }
        current_node = tree->nodes[ current_node ].parent;
    }
}

/*
 * Rebuilding the tree takes place when the counts have gone too
 * high.  From a simple point of view, rebuilding the tree just means that
 * we divide every count by two.  Unfortunately, due to truncation effects,
 * this means that the tree's shape might change.  Some nodes might move
 * up due to cumulative increases, while others may move down.
 */

void ahuff_RebuildTree( tree )
TREE *tree;
{
    int i;
    int j;
    int k;
    unsigned int weight;

/*
 * To start rebuilding the table,  I collect all the leaves of the Huffman
 * tree and put them in the end of the tree.  While I am doing that, I
 * scale the counts down by a factor of 2.
 */
    printf( "R" );
    j = tree->next_free_node - 1;
    for ( i = j ; i >= ROOT_NODE ; i-- ) {
        if ( tree->nodes[ i ].child_is_leaf ) {
            tree->nodes[ j ] = tree->nodes[ i ];
            tree->nodes[ j ].weight = ( tree->nodes[ j ].weight + 1 ) / 2;
            j--;
        }
    }

/*
 * At this point, j points to the first free node.  I now have all the
 * leaves defined, and need to start building the higher nodes on the
 * tree. I will start adding the new internal nodes at j.  Every time
 * I add a new internal node to the top of the tree, I have to check to
 * see where it really belongs in the tree.  It might stay at the top,
 * but there is a good chance I might have to move it back down.  If it
 * does have to go down, I use the memmove() function to scoot everyone
 * bigger up by one node.  Note that memmove() may have to be change
 * to memcpy() on some UNIX systems.  The parameters are unchanged, as
 * memmove and  memcpy have the same set of parameters.
 */
    for ( i = tree->next_free_node - 2 ; j >= ROOT_NODE ; i -= 2, j-- ) {
        k = i + 1;
        tree->nodes[ j ].weight = tree->nodes[ i ].weight +
                                  tree->nodes[ k ].weight;
        weight = tree->nodes[ j ].weight;
        tree->nodes[ j ].child_is_leaf = FALSE;
        for ( k = j + 1 ; weight < tree->nodes[ k ].weight ; k++ )
            ;
        k--;
        memmove( &tree->nodes[ j ], &tree->nodes[ j + 1 ],
                 ( k - j ) * sizeof( struct node ) );
        tree->nodes[ k ].weight = weight;
        tree->nodes[ k ].child = i;
        tree->nodes[ k ].child_is_leaf = FALSE;
    }
/*
 * The final step in tree reconstruction is to go through and set up
 * all of the leaf and parent members.  This can be safely done now
 * that every node is in its final position in the tree.
 */
    for ( i = tree->next_free_node - 1 ; i >= ROOT_NODE ; i-- ) {
        if ( tree->nodes[ i ].child_is_leaf ) {
            k = tree->nodes[ i ].child;
            tree->leaf[ k ] = i;
        } else {
            k = tree->nodes[ i ].child;
            tree->nodes[ k ].parent = tree->nodes[ k + 1 ].parent = i;
        }
    }
}

/*
 * Swapping nodes takes place when a node has grown too big for its
 * spot in the tree.  When swapping nodes i and j, we rearrange the
 * tree by exchanging the children under i with the children under j.
 */

void ahuff_swap_nodes( tree, i, j )
TREE *tree;
int i;
int j;
{
    struct node temp;

    if ( tree->nodes[ i ].child_is_leaf )
        tree->leaf[ tree->nodes[ i ].child ] = j;
    else {
        tree->nodes[ tree->nodes[ i ].child ].parent = j;
        tree->nodes[ tree->nodes[ i ].child + 1 ].parent = j;
    }
    if ( tree->nodes[ j ].child_is_leaf )
        tree->leaf[ tree->nodes[ j ].child ] = i;
    else {
        tree->nodes[ tree->nodes[ j ].child ].parent = i;
        tree->nodes[ tree->nodes[ j ].child + 1 ].parent = i;
    }
    temp = tree->nodes[ i ];
    tree->nodes[ i ] = tree->nodes[ j ];
    tree->nodes[ i ].parent = temp.parent;
    temp.parent = tree->nodes[ j ].parent;
    tree->nodes[ j ] = temp;
}

/*
 * Adding a new node to the tree is pretty simple.  It is just a matter
 * of splitting the lightest-weight node in the tree, which is the highest
 * valued node.  We split it off into two new nodes, one of which is the
 * one being added to the tree.  We assign the new node a weight of 0,
 * so the tree doesn't have to be adjusted.  It will be updated later when
 * the normal update process occurs.  Note that this code assumes that
 * the lightest node has a leaf as a child.  If this is not the case,
 * the tree would be broken.
 */
void ahuff_add_new_node( tree, c )
TREE *tree;
int c;
{
    int lightest_node;
    int new_node;
    int zero_weight_node;

    lightest_node = tree->next_free_node - 1;
    new_node = tree->next_free_node;
    zero_weight_node = tree->next_free_node + 1;
    tree->next_free_node += 2;

    tree->nodes[ new_node ] = tree->nodes[ lightest_node ];
    tree->nodes[ new_node ].parent = lightest_node;
    tree->leaf[ tree->nodes[ new_node ].child ] = new_node;

    tree->nodes[ lightest_node ].child         = new_node;
    tree->nodes[ lightest_node ].child_is_leaf = FALSE;

    tree->nodes[ zero_weight_node ].child           = c;
    tree->nodes[ zero_weight_node ].child_is_leaf   = TRUE;
    tree->nodes[ zero_weight_node ].weight          = 0;
    tree->nodes[ zero_weight_node ].parent          = lightest_node;
    tree->leaf[ c ] = zero_weight_node;
}

/*
 * All the code from here down is concerned with printing the tree.
 * Printing the tree out is basically a process of walking down through
 * all the nodes, with each new node to be printed getting nudged over
 * far enough to make room for everything that has come before.
 */

/*
 * This array is used to keep track of all the nodes that are in a given
 * row.  The nodes are kept in a linked list.  This array is used to keep
 * track of the first member.  The subsequent members will be found in
 * a linked list in the positions[] array.
 */

#define rows    ahuff_rows
struct row {
    int first_member;
    int count;
} rows[ 32 ];

/*
 * The positions[] array is used to keep track of the row and column of each
 * node in the tree.  The next_member element points to the next node
 * in the row for the given node.  The column is calculated on the fly,
 * and represents the actual column that a given number is to be printed in.
 * Note that the column for a node is not an actual column on the page.  For
 * purposes of analysis, it is assumed that each node takes up exactly one
 * column.  So, if printing out the actual values in a node takes up for
 * spaces on the printed page, we might want to allocate five physical print
 * columns for each column in the array.
 */

#define positions   ahuff_positions
struct location {
    int row;
    int next_member;
    int column;
} positions[ NODE_TABLE_COUNT ];

/*
 * This is the main routine called to print out a Huffman tree.  It first
 * calls the ahuff_ahuff_print_codes function, which prints out the binary codes
 * for each symbol.  After that, it calculates the row and column that
 * each node will be printed in, then prints the tree out.  This code
 * is not documented in the book, since it is essentially irrelevant to
 * the data compression process.  However, it is nice to be able to
 * print out the tree.
 */
void ahuff_PrintTree( tree )
TREE *tree;
{
    int i;
    int min;

    ahuff_ahuff_print_codes( tree );
    for ( i = 0 ; i < 32 ; i++ ) {
        rows[ i ].count = 0;
        rows[ i ].first_member = -1;
    }
    ahuff_calculate_rows( tree, ROOT_NODE, 0 );
    ahuff_calculate_columns( tree, ROOT_NODE, 0 );

    min = ahuff_find_minimum_column( tree, ROOT_NODE, 31 );
    ahuff_rescale_columns( min );
    ahuff_print_tree( tree, 0, 31 );
}

/*
 * This routine is called to print out the Huffman code for each symbol.
 * The real work is done by the ahuff_print_code routine, which racks up the
 * bits and puts them out in the right order.
 */

void ahuff_ahuff_print_codes( tree )
TREE *tree;
{
    int i;

    printf( "\n" );
    for ( i = 0 ; i < SYMBOL_COUNT  ; i++ )
        if ( tree->leaf[ i ] != -1 ) {
             if ( isprint( i ) )
                 printf( "%5c: ", i );
             else
                 printf( "<%3d>: ", i );
            printf( "%5u", tree->nodes[ tree->leaf[ i ] ].weight );
            printf( " " );
            ahuff_print_code( tree, i );
            printf( "\n" );
        }
}

/*
 * ahuff_print_code is a workhorse routine that prints out the Huffman code for
 * a given symbol.  It ends up looking a lot like ahuff_EncodeSymbol(), since
 * it more or less has to do the same work.  The major difference is that
 * instead of calling OutputBit, this routine calls putc, with a character
 * argument.
 */

void ahuff_print_code( tree, c )
TREE *tree;
int c;
{
    unsigned long code;
    unsigned long current_bit;
    int code_size;
    int current_node;
    int i;

    code = 0;
    current_bit = 1;
    code_size = 0;
    current_node = tree->leaf[ c ];
    while ( current_node != ROOT_NODE ) {
        if ( current_node & 1 )
            code |= current_bit;
        current_bit <<= 1;
        code_size++;
        current_node = tree->nodes[ current_node ].parent;
    };
    for ( i = 0 ; i < code_size ; i++ ) {
        current_bit >>= 1;
        if ( code & current_bit )
            putc( '1', stdout );
        else
            putc( '0', stdout );
    }
}

/*
 * In order to print out the tree, I need to calculate the row and column
 * where each node will be printed.  The rows are easier than the columns,
 * and I do them first.  It is easy to keep track of what row a node is
 * in as I walk through the tree.  As I walk through the tree, I also keep
 * track of the order the nodes appear in a given row, by adding them to
 * a linked list in the proper order.  After ahuff_calculate_rows() has been
 * recursively called all the way through the tree, I have a linked list of
 * nodes for each row.  This same linked list is used later to calculate
 * which column each node appears in.
 */

void ahuff_calculate_rows( tree, node, level )
TREE *tree;
int node;
int level;
{
    if ( rows[ level ].first_member == -1 ) {
        rows[ level ].first_member = node;
        rows[ level ].count = 0;
        positions[ node ].row = level;
        positions[ node ].next_member = -1;
    } else {
        positions[ node ].row = level;
        positions[ node ].next_member = rows[ level ].first_member;
        rows[ level ].first_member = node;
        rows[ level ].count++;
    }
    if ( !tree->nodes[ node ].child_is_leaf ) {
        ahuff_calculate_rows( tree, tree->nodes[ node ].child, level + 1 );
        ahuff_calculate_rows( tree, tree->nodes[ node ].child + 1, level + 1 );
    }
}

/*
 * After I know which row each of the nodes is in, I can start the
 * hard work, which is calculating the columns.  This routine gets
 * called recursively.  It starts off with a starting guess for where
 * we want the node to go, and returns the actual result, which is
 * the column the node ended up in.  For example, I might want my node
 * to print in column 0.  After recursively evaluating everything under
 * the node, I may have been pushed over to node -10 ( the tree is
 * evaluated down the right side first ).  I return that to whoever called
 * this routine so it can use the nodes position to calculate where
 * the node in a higher row is to be placed.
 */

int ahuff_calculate_columns( tree, node, starting_guess )
TREE *tree;
int node;
int starting_guess;
{
    int next_node;
    int right_side;
    int left_side;

/*
 * The first thing I check is to see if the node on my immediate right has
 * already been placed.  If it has, I need to make sure that I am at least
 * 4 columns to the right of it.  This allows me to print 3 characters plus
 * leave a blank space between us.
 */
    next_node = positions[ node ].next_member;
    if ( next_node != -1 ) {
        if ( positions[ next_node ].column < ( starting_guess + 4 ) )
            starting_guess = positions[ next_node ].column - 4;
    }
    if ( tree->nodes[ node ].child_is_leaf ) {
        positions[ node ].column = starting_guess;
        return( starting_guess );
    }
/*
 * After I have adjusted my starting guess, I calculate the actual position
 * of the right subtree of this node.  I pass it a guess for a starting
 * node based on my starting guess.  Naturally, what comes back may be
 * moved over quite a bit.
 */
    right_side = ahuff_calculate_columns( tree, tree->nodes[ node ].child, starting_guess + 2 );
/*
 * After figuring out where the right side lands, I do the same for the
 * left side.  After doing the right side, I have a pretty good guess where
 * the starting column for the left side might go, so I can pass it a good
 * guess for a starting column.
 */
    left_side = ahuff_calculate_columns( tree, tree->nodes[ node ].child + 1, right_side - 4 );
/*
 * Once I know where the starting column for the left and right subtrees
 * are going to be for sure, I know where this node should go, which is
 * right in the middle between the two.  I calcluate the column, store it,
 * then return the result to whoever called me.
 */
    starting_guess = ( right_side + left_side ) / 2;
    positions[ node ].column = starting_guess;
    return( starting_guess );
}

int ahuff_find_minimum_column( tree, node, max_row )
TREE *tree;
int node;
int max_row;
{
    int min_right;
    int min_left;

    if ( tree->nodes[ node ].child_is_leaf || max_row == 0 )
        return( positions[ node ].column );
    max_row--;
    min_right = ahuff_find_minimum_column( tree, tree->nodes[ node ].child + 1, max_row );
    min_left = ahuff_find_minimum_column( tree, tree->nodes[ node ].child, max_row );
    if ( min_right < min_left )
        return( min_right );
    else
        return( min_left );
}

/*
 * Once the columns of each node have been calculated, I go back and rescale
 * the columns to be actual printer columns.  In this particular program,
 * each node takes three characters to print, plus one space to keep nodes
 * separate.  We take advantage of the fact that every node has at least one
 * logical column between it and the ajacent node, meaning that we can space
 * nodes only two physical columns apart.  The spacing here consists of
 * rescaling each column so that the smallest column is at zero, then
 * multiplying by two to get a physical printer column.
 */

void ahuff_rescale_columns( factor )
int factor;
{
    int i;
    int node;

/*
 * Once min is known, we can rescale the tree so that column min is
 * pushed over to column 0, and each logical column is set to be two
 * physical columns on the printer.
 */
    for ( i = 0 ; i < 30 ; i++ ) {
        if ( rows[ i ].first_member == -1 )
            break;
        node = rows[ i ].first_member;
        do {
            positions[ node ].column -= factor;
            node = positions[ node ].next_member;
        } while ( node != -1 );
    }
}

/*
 * ahuff_print_tree is called after the row and column of each node have been
 * calculated.  It just calls the four workhorse routines that are
 * responsible for printing out the four elements that go on each row.
 * At the top of the row are the connecting lines hooking the tree
 * together.  On the next line of the row are the node numbers.  Below
 * them are the weights, and finally the symbol, if there is one.
 */

void ahuff_print_tree( tree, first_row, last_row )
TREE *tree;
int first_row;
int last_row;
{
    int row;

    for ( row = first_row ; row <= last_row ; row++ ) {
        if ( rows[ row ].first_member == -1 )
            break;
        if ( row > first_row )
            ahuff_print_connecting_lines( tree, row );
        ahuff_print_node_numbers( row );
        ahuff_print_weights( tree, row );
        ahuff_print_symbol( tree, row );
    }
}

/*
 * Printing the connecting lines means connecting each pair of nodes.
 * I use the IBM PC character set here.  They can easily be replaced
 * with more standard alphanumerics.
 */

#ifndef ALPHANUMERIC

#define LEFT_END  218
#define RIGHT_END 191
#define CENTER    193
#define LINE      196
#define VERTICAL  179

#else

#define LEFT_END  '+'
#define RIGHT_END '+'
#define CENTER    '+'
#define LINE      '-'
#define VERTICAL  '|'

#endif


void ahuff_print_connecting_lines( tree, row )
TREE *tree;
int row;
{
    int current_col;
    int start_col;
    int end_col;
    int center_col;
    int node;
    int parent;

    current_col = 0;
    node = rows[ row ].first_member;
    while ( node != -1 ) {
        start_col = positions[ node ].column + 2;
        node = positions[ node ].next_member;
        end_col = positions[ node ].column + 2;
        parent = tree->nodes[ node ].parent;
        center_col = positions[ parent ].column;
        center_col += 2;
        for ( ; current_col < start_col ; current_col++ )
            putc( ' ', stdout );
        putc( LEFT_END, stdout );
        for ( current_col++ ; current_col < center_col ; current_col++ )
            putc( LINE, stdout );
        putc( CENTER, stdout );
        for ( current_col++; current_col < end_col ; current_col++ )
            putc( LINE, stdout );
        putc( RIGHT_END, stdout );
        current_col++;
        node = positions[ node ].next_member;
    }
    printf( "\n" );
}

/*
 * Printing the node numbers is pretty easy.
 */

void ahuff_print_node_numbers( row )
int row;
{
    int current_col;
    int node;
    int print_col;

    current_col = 0;
    node = rows[ row ].first_member;
    while ( node != -1 ) {
        print_col = positions[ node ].column + 1;
        for ( ; current_col < print_col ; current_col++ )
            putc( ' ', stdout );
        printf( "%03d", node );
        current_col += 3;
        node = positions[ node ].next_member;
    }
    printf( "\n" );
}

/*
 * Printing the weight of each node is easy too.
 */

void ahuff_print_weights( tree, row )
TREE *tree;
int row;
{
    int current_col;
    int print_col;
    int node;
    int print_size;
    int next_col;
    char buffer[ 10 ];

    current_col = 0;
    node = rows[ row ].first_member;
    while ( node != -1 ) {
        print_col = positions[ node ].column + 1;
        sprintf( buffer, "%u", tree->nodes[ node ].weight );
        if ( strlen( buffer ) < 3 )
            sprintf( buffer, "%03u", tree->nodes[ node ].weight );
        print_size = 3;
        if ( strlen( buffer ) > 3 ) {
            if ( positions[ node ].next_member == -1 )
                print_size = strlen( buffer );
            else {
                next_col = positions[ positions[ node ].next_member ].column;
                if ( ( next_col - print_col ) > 6 )
                    print_size = strlen( buffer );
                else {
                    strcpy( buffer, "---" );
                    print_size = 3;
                }
            }
        }
        for ( ; current_col < print_col ; current_col++ )
            putc( ' ', stdout );
        printf( buffer );
        current_col += print_size;
        node = positions[ node ].next_member;
    }
    printf( "\n" );
}

/*
 * Printing the symbol values is a little more complicated.  If it is a
 * printable symbol, I print it between simple quote characters.  If
 * it isn't printable, I print a hex value, which also only takes up three
 * characters.  If it is an internal node, it doesn't have a symbol,
 * which means I just print the vertical line.  There is one complication
 * in this routine.  In order to save space, I check first to see if
 * any of the nodes in this row have a symbol.  If none of them have
 * symbols, we just skip this part, since we don't have to print the
 * row at all.
 */

void ahuff_print_symbol( tree, row )
TREE *tree;
int row;
{
    int current_col;
    int print_col;
    int node;

    current_col = 0;
    node = rows[ row ].first_member;
    while ( node != -1 ) {
        if ( tree->nodes[ node ].child_is_leaf )
            break;
        node = positions[ node ].next_member;
    }
    if ( node == -1 )
        return;
    node = rows[ row ].first_member;
    while ( node != -1 ) {
        print_col = positions[ node ].column + 1;
        for ( ; current_col < print_col ; current_col++ )
            putc( ' ', stdout );
        if ( tree->nodes[ node ].child_is_leaf ) {
            if ( isprint( tree->nodes[ node ].child ) )
                printf( "'%c'", tree->nodes[ node ].child );
            else if ( tree->nodes[ node ].child == END_OF_STREAM )
                printf( "EOF" );
            else if ( tree->nodes[ node ].child == ESCAPE )
                printf( "ESC" );
            else
                printf( "%02XH", tree->nodes[ node ].child );
        } else
            printf( " %c ", VERTICAL );
        current_col += 3;
        node = positions[ node ].next_member;
    }
    printf( "\n" );
}

/************************** End of AHUFF.C ****************************/

