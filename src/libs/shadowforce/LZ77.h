//------------------------------------
// LZ77.h
// Jason Hughes
// (adapted from Ala Diaz's code)
// Origin Systems 
// 1998
//------------------------------------
#ifndef LZ77_H
#define LZ77_H

#include "Compress.h"

class LZ77 : public Compressor
{
protected:
	//------------------------------------
	// The tree[] structure
	// contains the binary tree of all of the strings in the window sorted
	// in order.
	struct TreeNode
	{
		u_int32 parent;
		u_int32 smallChild;
		u_int32 largeChild;
	};

	//------------------------------------

	enum
	{
		INDEX_BIT_COUNT     = 12,
		LENGTH_BIT_COUNT    = 4,
		WINDOW_SIZE         = (1 << INDEX_BIT_COUNT),
		RAW_LOOK_AHEAD_SIZE = (1 << LENGTH_BIT_COUNT),
		BREAK_EVEN          = ((1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT) / 9),
		LOOK_AHEAD_SIZE     = (RAW_LOOK_AHEAD_SIZE + BREAK_EVEN),
		TREE_ROOT           = WINDOW_SIZE,
		END_OF_STREAM       = 0,
		UNUSED              = 0,
	};

	//------------------------------------

	u_int8    window[WINDOW_SIZE];
	TreeNode  tree[WINDOW_SIZE + 1];

	void    initTree(u_int32 r);
	void    contractNode(u_int32 oldNode,u_int32 newNode);
	void    replaceNode(u_int32 oldNode,u_int32 newNode);
	u_int32 findNextNode(u_int32 node);
	void    deleteString(u_int32 p);
	u_int32 addString(u_int32 newNode,u_int32 *matchPosition);

public:
	LZ77(void);
	virtual ~LZ77(void);

	virtual const char      *identify(void);
	virtual CompressorTypes  enumerate(void);
	virtual void             compress(const CompressorInput &input,DecompressorInput *output);
	virtual void             decompress(const DecompressorInput &input,CompressorInput *output);
};

#endif