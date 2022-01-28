/* ctwtree - has the following functions:
   finding and updating pathes in the ctw trees;
   managing the array containing the data of the ctw trees */
   
#define EMPTY_NODE   0xffffffffU	/* value of an empty node */

/* ShiftMask is defined in ctwtree.c. it is used to get the "prefixing bits" in the current
   byte */
typedef unsigned short NUMBERS[8];
extern const NUMBERS ShiftMask;

/* BytePrefix is used to get the bits in b "at the left side" of bit f */
#define BytePrefix(b,f) (b & ShiftMask[f])
/* ByteBit is used to get bit f in byte b */
#define ByteBit(b,f)    ((b >> (7-f)) & 1)

/* tree_frozen indicates if the tree structure is frozen or not. In a frozen tree structure no
   more new nodes will be created. The tree structure is frozen if the file buffer is full,
   because every node in the tree contains a pointer to this file buffer (which is impossible if
   nrsymbols >= settings.filebufsize). */
#define tree_frozen	(nrsymbols >= settings.filebufsize)


/* in filebuffer, the first settings.filebufsize bytes of the file will be stored. This is
   needed because of the tree pruning concept: a tree is pruned if a certain context is unique.
   To reproduce the entire context it is nescessary to have access to the entire file history.
   Space for the file buffer is allocated dynamically by init_filebuffer. */
extern unsigned char *filebuffer;

/* nrnodes => the number of nodes in the tree array. Only for diagnostic purpose
   nrsymbols => counts how many uncompressed bytes are decoded/encoded.
   nrfailed => number of failed (node not found) searches in tree array. Only for diagnostics */
extern unsigned int nrnodes, nrsymbols, nrfailed;

/* TreeNode is a struct that contains the information about a node. The tree array is an array
   of these nodes. explanation of the fields:
   symbol contains 3 information fields:
   	bit 31-27 the number of tries that was needed to find the node
   	bit 26-24 the phase the node belongs to
   	bit 23-0 contain an index into the filebuffer (defined above) where the current
   	  position of the first occurence of the current context can be found
   data contains the counts of zeros and ones and the logarithm of beta (see ctwmath.h)
   The total information record size is: 8 bytes per tree node */
struct TreeNode {
  unsigned int     symbol;
  struct CTWRecord data;
};


/* init_filebuffer allocates space for the file buffer (depending on settings.filebufsize) */
boolean init_filebuffer();

/* init_treestruct must be called before working with the CTW tree. It does the following:
	Dynamically allocate space for the tree array (depending on settings.maxnrnodes);
	Set tree_frozen to false, so new nodes in the tree can be created;
	Fill the entire tree array with the initial value 0xff (see ctwtree.c);
	Create 8 root nodes (1 for each tree) in the tree array;
	and initialize variables nrnodes and nrfailed
   sym is used as context for the root nodes that will be created
   init_treestruct returns true if memory allocation succeeded, else it returns false */
boolean init_tree(unsigned char sym);

/* UpdatePath updates the nodes in the Path previously found by FindPath with the nodes
   specified in newinfo[] */
void UpdatePath(struct CTWRecord newinfo[]);

/* FindPath searches the path of the given context and returns the CTW info on the path.
   Parameters:
   phase: specifies which context tree to be used. There are 8 context trees, one for each bit;
   context: specifies index into "filebuffer" where the current position of the first occurence
            of the current context can be found;
   ctxstring: holds a copy of the current context;
   *curdepth: in curdepth, the number of nodes on the found path is returned;
   ctwinfo: in this array the CTW information of the nodes on the found path are returned */
void FindPath(int phase, unsigned int context, unsigned char ctxstring[],
              int *curdepth, struct CTWRecord ctwinfo[]);

/* free_memory frees memory that was dynamically assigned by init_filebuffer and init_treestruct 
   for the filebuffer and the tree array */
void free_memory();
