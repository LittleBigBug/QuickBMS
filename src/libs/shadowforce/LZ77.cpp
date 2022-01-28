#include "first.h"

#include "LZ77.h"
#include "Bitstream.h"

#define MOD_WINDOW(a)      ((a) & (WINDOW_SIZE - 1))

LZ77::LZ77(void)
{
}

LZ77::~LZ77(void)
{
}

//---------------------------------------------------

// Initialize root node and clear tree data to 0.
void LZ77::initTree(u_int32 r)
{
	memset(tree,0,sizeof(TreeNode)*(WINDOW_SIZE + 1));

    tree[TREE_ROOT].largeChild = r;
    tree[r].parent = TREE_ROOT;
    tree[r].largeChild = UNUSED;
    tree[r].smallChild = UNUSED;
}

// This routine is used when a node is being deleted.  The link to
// its descendant is broken by pulling the descendant in to overlay
// the existing link.
void LZ77::contractNode(u_int32 oldNode,u_int32 newNode)
{
    tree[newNode].parent = tree[oldNode].parent;

    if (tree[tree[oldNode].parent].largeChild == oldNode)
        tree[tree[oldNode].parent].largeChild = newNode;
    else tree[tree[oldNode].parent].smallChild = newNode;

    tree[oldNode].parent = UNUSED;
}

// This routine is also used when a node is being deleted.  However,
// in this case, it is being replaced by a node that was not previously
// in the tree.
void LZ77::replaceNode(u_int32 oldNode,u_int32 newNode)
{
    u_int32 parent = tree[oldNode].parent;

    if (tree[parent].smallChild == oldNode)
        tree[parent].smallChild = newNode;
    else tree[parent].largeChild = newNode;

    tree[newNode] = tree[oldNode];
    tree[tree[newNode].smallChild].parent = newNode;
    tree[tree[newNode].largeChild].parent = newNode;
    tree[oldNode].parent = UNUSED;
}

// This routine is used to find the next smallest node after the node
// argument.  It assumes that the node has a smaller child.  We find
// the next smallest child by going to the smallChild node, then
// going to the end of the largeChild descendant chain.
u_int32 LZ77::findNextNode(u_int32 node)
{
    u_int32 next = tree[node].smallChild;

    while (tree[next].largeChild != UNUSED)
        next = tree[next].largeChild;

    return next;
}

// This routine performs the classic binary tree deletion algorithm.
// If the node to be deleted has a null link in either direction, we
// just pull the non-null link up one to replace the existing link.
// If both links exist, we instead delete the next link in order, which
// is guaranteed to have a null link, then replace the node to be deleted
// with the next link.
void LZ77::deleteString(u_int32 p)
{
    if (tree[p].parent == UNUSED)
        return;

    if (tree[p].largeChild == UNUSED)
        contractNode(p, tree[p].smallChild);
    else if (tree[p].smallChild == UNUSED)
        contractNode(p, tree[p].largeChild);
    else 
	{
        u_int32 replacement = findNextNode(p);
        deleteString(replacement);
        replaceNode(p, replacement);
    }
}

// This where most of the work done by the encoder takes place.  This
// routine is responsible for adding the new node to the binary tree.
// It also has to find the best match among all the existing nodes in
// the tree, and return that to the calling routine.  To make matters
// even more complicated, if the newNode has a duplicate in the tree,
// the oldNode is deleted, for reasons of efficiency.
u_int32 LZ77::addString(u_int32 newNode,u_int32 *matchPosition)
{
	u_int32 i;
    u_int32 *child;
    int32 delta;
	u_int32 testNode;
	u_int32 matchLength;

    if (newNode == END_OF_STREAM)
        return(0);

	delta = 0;

    testNode = tree[TREE_ROOT].largeChild;
    matchLength = 0;

    for (;;)
	{
        for (i = 0 ; i < LOOK_AHEAD_SIZE ; i++) 
		{
            delta = window[MOD_WINDOW(newNode + i)] -
                    window[MOD_WINDOW(testNode + i)];

            if (delta != 0)
                break;
        }

        if (i >= matchLength) 
		{
            matchLength = i;
            *matchPosition = testNode;

            if (matchLength >= LOOK_AHEAD_SIZE) 
			{
                replaceNode(testNode, newNode);
                return matchLength;
            }
        }

        if (delta >= 0)
            child = &tree[testNode].largeChild;
        else child = &tree[testNode].smallChild;

        if (*child == UNUSED) 
		{
            *child = newNode;
            tree[newNode].parent = testNode;
            tree[newNode].largeChild = UNUSED;
            tree[newNode].smallChild = UNUSED;
            return matchLength;
        }

        testNode = *child;
    }
}

//---------------------------------------------------

const char *LZ77::identify(void)
{
	return "Lempel-Ziv 77";
}

CompressorTypes LZ77::enumerate(void)
{
	return CT_LZ77;
}

void LZ77::compress(const CompressorInput &input,DecompressorInput *output)
{
    u_int32 inputBufferPos;
    u_int8  c;
	u_int32 i;
    u_int32 curPos = 1;
    u_int32 replaceCount;
    u_int32 matchLength = 0;
    u_int32 matchPosition = 0;

	Bitstream outBits(input.lengthInBytes);

	memset(window,0,sizeof(window));
    for (inputBufferPos=0; inputBufferPos<LOOK_AHEAD_SIZE; inputBufferPos++) 
	{
		if (inputBufferPos>=input.lengthInBytes)
			break;
		c=*(static_cast<u_int8 *>(input.buffer)+inputBufferPos);  // read a byte
        window[curPos + inputBufferPos] = c;
    }

    u_int32 lookAheadBytes = inputBufferPos;

    initTree(curPos);

    while (lookAheadBytes > 0) 
	{
        matchLength=min(matchLength,lookAheadBytes);

        if (matchLength <= BREAK_EVEN) 
		{
            replaceCount = 1;
            outBits.writeBit(1);
            outBits.writeBuffer(&window[curPos], 1);
        } 
		else 
		{
            outBits.writeBit(0);
			for (i=0; i<INDEX_BIT_COUNT; i++)
	            outBits.writeBit(matchPosition & (1 << i));
			for (i=0; i<LENGTH_BIT_COUNT; i++)
	            outBits.writeBit((matchLength-(BREAK_EVEN+1)) & (1 << i));
            replaceCount = matchLength;
        }

        for (i=0; i<replaceCount; i++)
		{
            deleteString(MOD_WINDOW(curPos + LOOK_AHEAD_SIZE));

			if (inputBufferPos>=input.lengthInBytes)
                lookAheadBytes--;
            else 
			{
				c=*(static_cast<u_int8 *>(input.buffer)+inputBufferPos);
				inputBufferPos++;
				window[MOD_WINDOW(curPos + LOOK_AHEAD_SIZE)] = c;
			}

            curPos = MOD_WINDOW(curPos + 1);

            if (lookAheadBytes)
                matchLength = addString(curPos, &matchPosition);
        }
    }

    outBits.writeBit(0);
	for (i=0; i<INDEX_BIT_COUNT; i++)  // mark this with an end of stream token
	    outBits.writeBit(0);

	output->lengthInBytes=outBits.getPosition()/8+((outBits.getPosition() & 7)?1:0);
	output->buffer=MEM_OWN(outBits.copyStream());
}

// This is the expansion routine for the LZSS algorithm.  All it has
// to do is read in flag bits, decide whether to read in a character or
// a index/length pair, and take the appropriate action.
void LZ77::decompress(const DecompressorInput &input,CompressorInput *output)
{
    u_int32 matchPosition = 0;
    u_int32 windowIndex;
	u_int32 matchLength;
    u_int8  c;

	Bitstream bits(static_cast<const u_int8 *>(input.buffer),input.lengthInBytes);
	Bitstream outbits(input.lengthInBytes);          // unknown what the destination size will be?

	windowIndex=1;

    for (;;) 
	{
        if (bits.readBit()) 
		{
            bits.readBuffer(&c,1);
			outbits.writeBuffer(&c,1);
            window[windowIndex] = c;
            windowIndex = (windowIndex + 1) & (WINDOW_SIZE-1);
        } 
		else 
		{
            int loop;
			for (loop=0, matchPosition=0; loop<INDEX_BIT_COUNT; loop++)
	            matchPosition|=bits.readBit() << loop;

            if (matchPosition == END_OF_STREAM)
                break;

			for (loop=0, matchLength=0; loop<LENGTH_BIT_COUNT; loop++)
	            matchLength|=bits.readBit() << loop;
            matchLength += BREAK_EVEN;

            for (u_int32 i=0; i<=matchLength; i++) 
			{
                c = window[(matchPosition + i) & (WINDOW_SIZE-1)];
				outbits.writeBuffer(&c,1);
                window[windowIndex]=c;
                windowIndex = (windowIndex + 1) & (WINDOW_SIZE-1);
            }
        }
    }

	output->buffer=MEM_OWN(outbits.copyStream());
	output->lengthInBytes=outbits.getPosition()/8+((outbits.getPosition() & 7)?1:0);
}

