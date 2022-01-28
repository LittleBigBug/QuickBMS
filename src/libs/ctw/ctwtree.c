/* ctwtree - has the following functions:
   finding and updating pathes in the ctw trees
   managing tree array (which contains the ctw trees) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctw-settings.h"
#include "ctwmath.h"
#include "ctwtree.h"

const NUMBERS ShiftMask = { 	/* use the most significant part first */
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe
};

/* in filebuffer, the first settings.filebufsize bytes of the file will be stored. This is
   needed because of the tree pruning concept: a tree is pruned if a certain context is unique.
   To reproduce the entire context it is nescessary to have access to the entire file history.
   Space for the file buffer is allocated dynamically by init_filebuffer. */
unsigned char *filebuffer;

/* nrnodes => the number of nodes in the tree array. Only for diagnostic purpose
   nrsymbols => counts how many uncompressed bytes are decoded/encoded.
   nrfailed => number of failed (node not found) searches in tree array. Only for diagnostics */
unsigned int nrnodes, nrsymbols, nrfailed;

/* Pointer to the tree array. Table is dynamically allocated because its size is not known
   at compile time */
static struct TreeNode    *tree=NULL;	

/* localindex array: stores the route that is found by FindPath(), expressed in indices
   in the tree array where the nodes on the path can be found. */
unsigned int       localindex[MAX_TREEDEPTH+1];

/* localdepth: the depth that is reached in the (sometimes pruned) tree afther FindPath() */
unsigned int       localdepth;

/* rootindex: indices in the tree array where the 8 root nodes are stored. to get faster and
   easier access to those nodes */
unsigned int       rootindex[8];

/* ****************************************************************************************** */

/* Derived offset table. This is a 'pseudo-random' table that is used to find a location in the
   tree array */
unsigned int Tperm[256] = {
  175715u,11428377u, 6429025u, 1663333u,23160013u,23383373u,13454579u,21820291u,
15958541u,25300137u,  829939u,11137997u,32754777u,30169415u, 5850653u,21372299u,
 1936299u,25930603u,28011331u,23806635u,21146549u,11252897u,28614785u,10519007u,
 8511025u,31338949u, 3261913u,29743389u,31005773u,18632081u, 5083357u,26271075u,
14508753u,23253199u,13684507u,13573115u,18611199u,33291877u,33449115u, 6593227u,
10144419u,13279781u,10626139u, 2382529u, 5947455u,12599229u, 4176947u,29110999u,
 3331965u,14122125u,24939693u, 9219547u,11394017u,31187013u,31474833u, 4493797u,
 9561129u,31730093u, 2731497u,28174791u,32098091u,29830103u,19650243u,30852053u,
12833907u,30700077u, 7482489u, 2914805u, 7992485u,32810335u,10837921u,23044107u,
27265791u,  720783u,16748255u,26140285u,14581007u, 8196081u,17822045u,32595283u,
22893479u,22259317u,27686021u, 7636277u, 8729813u,20239751u,13993963u,25684823u,
32200227u,22422391u, 2324333u,24604007u,23946753u,23462375u,  124681u,31918193u,
17330473u, 7415959u,19437313u, 9896203u,16845629u,17513673u,20760837u,13174013u,
17104055u,16561691u,11934515u, 1782765u,20180401u,32354743u,28423919u,28765833u,
15632831u, 9027229u,29269159u,10266289u,10924435u,11637447u,26396405u,13038615u,
15996601u, 1488961u,12075281u, 4264165u,17884265u,14968853u, 6821141u, 1381437u,
18103393u, 3957103u, 6385465u,24066119u,20465275u, 4618805u, 8008991u, 3481237u,
18781687u, 9828029u,32947459u,12387141u,16991359u,21266225u, 8335701u,20009999u,
22286055u,  976719u,15159267u,22012829u,31693831u,27002669u,  470127u,19689079u,
 7239471u, 7811001u,19904693u,28882027u,11823663u, 6958855u, 3081979u,17234779u,
16472607u,22683613u, 2088095u,31235775u,10403507u,12497441u,11673811u, 2151187u,
13833155u,18072513u,29606323u,29471553u,28524619u,20990711u, 4912877u,16182419u,
15503877u, 9569595u,  342621u,20602089u, 6088723u,15209251u, 1254157u,19074505u,
17680799u,29990825u,27240853u,27891119u,26586763u,28216267u, 9161271u,30029689u,
 3635335u,24676089u, 8845649u,16339449u,22149205u,33051657u, 5507131u,  539353u,
 3856427u,14167023u, 2879015u,32384923u, 2595407u,26890135u, 5216211u,26726993u,
30560629u, 5338407u,24455053u,19369345u,26050871u,25245251u,20333385u, 4409727u,
21593797u,25085337u,12949835u,26823529u,21719275u,23653017u,15374617u,10033225u,
18368933u, 4826457u,27613267u,22565485u, 5401919u, 7159313u,20844915u, 1143761u,
24367331u,30466953u,14911951u,25808479u,30301989u, 6235377u,19198055u,15754883u,
 6718009u, 8534305u, 3744253u,19004859u,33405627u,29014907u,12286853u,24872215u,
25499361u,18276439u,14702223u, 5672667u, 9362289u,14381475u,24224259u,27394735u
};

#define hash1(c) (Tperm[c] & (settings.maxnrnodes-1))	/* macro to get a value from Tperm */

/* ****************************************************************************************** */

/* Macro to calculate a value for storage in struct TreeNode.symbol (defined in ctwtree.h) */
#define itocptr(i,nrtries,phase) ((((((nrtries)-1)<<3)|(phase))<<24) | (i))

/* Get the index in filebuffer from a struct TreeNode.symbol value */
#define cptrtoi(p) ((p)&0x00ffffff)
/* Get the nr of tries from a struct Treenode.symbol value */
//#define cptrtonrtries(p) (((p) >> 27)+1)

/* local function that searches for the child node of a node in a tree. Parameters:
	phase: specifies which tree to search in
	curindex: index value of start node, whose child has to be found
	ctxdepth: the depth in tree of the node that has to be found
	ctxstring: the current context
	newindex: used to return the index value of found/new node
   return values:
   	>=1: new node; that means an empty position in the hashtable is found. returns value
   	0: old node found
   	-1: failure (node could not be found or allocated) */
int FindIndex(int phase, unsigned int curindex, int ctxdepth,
              unsigned char *ctxstring, unsigned int *newindex) {
  int offset;
  int c = ctxstring[ctxdepth-1];
  unsigned char s;
  unsigned int nrtries;
  
  /* get a pseudo random offset to use to find the child node. The value depends on the last
     character in the context. The value from hash1() is ORed with (phase+1)<<1 to get different
     offset values for each of the trees. The shift is required to keep the result odd. */
  offset = hash1(c) ^ ((phase+1)<<1);
  for (nrtries = 1; nrtries <= settings.maxnrtries; nrtries++) {
    curindex = (curindex + offset) & (settings.maxnrnodes-1); /* calculate next index to try */
    if(tree[curindex].symbol == EMPTY_NODE) {
      if (tree_frozen) return(-1); /* do not create a new node if the tree structure is frozen */
      *newindex = curindex;
      return nrtries; /* indicates a new node is found */
    } else
      if ((((nrtries-1)<<3) | phase) == (tree[curindex].symbol>>24)) {
      	if (ctxdepth == 1)
      	  s = BytePrefix(filebuffer[cptrtoi(tree[curindex].symbol)], phase);
      	else
      	  s = filebuffer[cptrtoi(tree[curindex].symbol)];
      	if (ctxstring[ctxdepth-1] == s) {
          *newindex = curindex;
          return 0;	       /* indicates an old node has been found */
        }
      }
  }
  nrfailed++;
  return -1;     /* indicates failure, no node found after settings.maxnrtries tries */
}

/* ****************************************************************************************** */

boolean init_filebuffer() {
  /* allocate memory for file buffer and return false on failure */
  filebuffer = malloc(settings.filebufsize);
  if(filebuffer == NULL) {
    fprintf(stderr, "Error: unable to allocate memory for file buffer\n");
    return false;
  }
  return true;
}

boolean init_tree(unsigned char sym) {
  unsigned int i, f;

  /* allocate memory for tree array and return false on failure */
  tree = malloc(settings.maxnrnodes * sizeof(struct TreeNode));
  if(tree == NULL) {
    fprintf(stderr, "Error: unable to allocate memory for tree array\n");
    return false;
  }

  /* Fill the entire tree array with the initial value 0xff. This is required because testing
     if a certain position in the tree array is used, is done by checking if all bits of this
     byte are one. All ones in this byte is a value that cannot occur in any other way. */
  memset(tree, 0xff, settings.maxnrnodes * sizeof(struct TreeNode));

  /* Create 8 root nodes (1 for each tree) in the tree array */
  for (f = 0; f < 8; f++) {
    rootindex[f] = i = hash1(f); /* get index for root node in tree array */
    /* store information on root node */
    tree[i].symbol = itocptr(settings.treedepth + 2, 1, f);
    tree[i].data   = CTW_data_from_one_count(ByteBit(sym,f));
  }
  /* note: at this moment, all trees are actually pruned trees! */

  nrnodes        = 8;	/* number of nodes used up to now */
  nrfailed       = 0;	/* no failed probes in the tree array yet */
  
  return true;
}

/* ****************************************************************************************** */

/* local function that searches for the child node of a node in a tree, using FindIndex.
   if FindIndex finds an empty node, this node is initialized with the right values.
   The function is used by FindPath to add "old leafs"; leaves that where at first not added
   because of tree pruning, but appear to be nescessary. 
   Parameters:
	phase: specifies which tree to search in
	curindex: index value of start node, whose child has to be found
	depth: the depth in tree of node specified by curindex
	ctxstring: the current context
	context: position in filebuffer where the context occurs */
void FindOrCreateOldLeaf(int phase, unsigned int curindex, int depth,
                  unsigned char *ctxstring, unsigned int context) {
  unsigned int oldindex;
  int n;

  switch (n = FindIndex(phase, curindex, depth, ctxstring, &oldindex)) {
  case -1:
    break;
  case 0:
    break;
  default: /* n >= 1 */
    /* initialize values of new node */
    tree[oldindex].symbol = itocptr(context, n, phase);
    tree[oldindex].data   = tree[curindex].data;
    nrnodes++;
  }
}

/* ****************************************************************************************** */

void UpdatePath(struct CTWRecord newinfo[]) {

  /* the indices in the tree table are stored in localindex[] so updating the tree
     with the new information is straightforward */
  while (localdepth-- > 0)
    tree[localindex[localdepth]].data = newinfo[localdepth];
}


void FindPath(int phase, unsigned int context, unsigned char ctxstring[],
              int *curdepth, struct CTWRecord ctwinfo[]) {
  /* local variables: are explained below when they are used */
  unsigned int depth, curindex, newindex, previndex, d;
  unsigned char newsym, oldsym;
  boolean same;
  int n;

  /* init: curindex points to the root of the tree, depth to current depth */
  depth = 0;
  curindex = rootindex[phase];

  /* loop that walks through the tree "as far as possible" */
  while(true) {
    localindex[depth] = curindex;
    ctwinfo[depth] = tree[curindex].data; /* make copy of CTW info of found node */
    if(depth == settings.treedepth + 1) {	/* then end of tree reached */
      localdepth = *curdepth = settings.treedepth + 2;
      return;
    }
    depth++;
    context--;
    
    /* find index of child node in tree array, using FindIndex */
    switch (n = FindIndex(phase, curindex, depth, ctxstring, &newindex)) {
    case -1: /* failure; forced end of search, no continuation possible */
      localdepth = *curdepth = depth;
      return;
    case 0: /* continue with the path */
      curindex = newindex;
      break;
    default: /* n>=1, we reached a node but can't proceed with it, because it's an empty node */
      /* in a loop we extend the old context as needed and possible */
      while(true) {
        /* == ctxstring & depth include new symbol
           == curindex is old processed node
           == newindex is node to be used for new context */
	previndex = cptrtoi(tree[curindex].symbol) - 1; /* points to last context symb */

        /* First check if the old and new context are exactly the same. If this is true, a new
           node is not needed at all in the pruned tree. If the node is created the pruning 
           is not optimal. But performing this check may slow down the encoding/decoding
           process! Is only checked if settings.strictpruning is true (EXPERIMENTAL) */
	if (settings.strictpruning && previndex != (context + 1))
	{
	  /* if not first occurence of context (and strict pruning option enabled),
	     check if total old and new context are the same */
	  for (d = depth, same = true; (d <= settings.treedepth + 1) && same; d++)
	  {
  	    if (d == 1)
	      oldsym = BytePrefix(filebuffer[previndex + depth - d], phase);
	    else
 	      oldsym = filebuffer[previndex + depth - d];
	    newsym = ctxstring[d - 1];
	    if (oldsym != newsym)
	      same = false;
	  }
  	  if (same)
  	  {
	    /* old and new context are exactly the same, we are finished */
	    localdepth = *curdepth = depth;
	    return;
	  }
        }
        
        /* create the new extension */
        tree[newindex].symbol = itocptr(context+1, n, phase);
        tree[newindex].data   = CTW_DATA_ZERO;
        nrnodes++;
        localindex[depth] = newindex;
        ctwinfo[depth] = CTW_DATA_ZERO;

	/* get last symbols of old and new context */
        newsym = ctxstring[depth-1];
        if(depth==1) {
          oldsym = BytePrefix(filebuffer[previndex], phase);
        } else {
          oldsym = filebuffer[previndex];
        }

        /* check if the (last symbol) of the old and new contexts coincide */
        if (oldsym==newsym) {  /* if contexts coincide, loop with newly created node */
          /* but we must make it an old node; it must point to the first occurence of the
             context */
          tree[newindex].symbol = itocptr(previndex, n, phase);
          tree[newindex].data   = tree[curindex].data;
          curindex = newindex;
          ctwinfo[depth] = tree[curindex].data;
        } else {              /* else create a second, old, leaf and stop */
          /* this occurs in a situation where the tree was pruned, but it appears that there
             are different contexts in the pruned part of the tree, so the tree is extended */
          ctxstring[depth-1] = oldsym;
          FindOrCreateOldLeaf(phase, curindex, depth, ctxstring, previndex);
          ctxstring[depth-1] = newsym;
          localdepth = *curdepth = depth + 1;
          return;
        }
        
        /* note: the program gets here when the last symbol of the context coincided.
           check if we are finished */
        if(depth == settings.treedepth + 1) {
          localdepth = *curdepth = settings.treedepth + 2;
          return;
        }
        /* otherwise proceed finding node at next depth */
        depth++;
        context--;
        switch(n = FindIndex(phase, curindex, depth, ctxstring, &newindex)) {
        case -1: /* failure; must stop, our new context can't be extended,
                    but we must try to create an extra old node
                    (not on our information and update list however) */
          newsym = ctxstring[depth-1];        /* current new context symbol */
          previndex = cptrtoi(tree[curindex].symbol)-1;
          oldsym = filebuffer[previndex];
          /* Try to create a node for the old context. note: not yet checked if
             these contexts coincide, but that does not really matter because then
             FindOrCreateOldLeaf will simply fail. However it would be more efficient
             to simply prevent this. */
          ctxstring[depth-1] = oldsym;
          if (oldsym != newsym)
	    FindOrCreateOldLeaf(phase, curindex, depth, ctxstring, previndex);
          ctxstring[depth-1] = newsym;
          localdepth = *curdepth = depth;
          return;
        case 0:
          /* Should not happen, because that would mean that the newly created node already had
             a child, which is impossible! If this occurs, it must be a bug! */
	  /* output error */
          fprintf(stderr, "CTW internal error 1: unexpected node found\n");
	  localdepth = *curdepth = depth;
          return;
        default:  /* We can create a new node and now are ready for the next loop */
          break;
        }
      }
    }
  }
}


void free_memory()
{
  if (filebuffer != NULL) free(filebuffer);
  if (tree != NULL) free(tree);
}
