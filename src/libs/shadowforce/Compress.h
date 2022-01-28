//------------------
// Compress.h
// Jason Hughes
// Origin Systems
// 1998
//------------------
// This is intended to be a base class for all data compression objects
// Each one should publicly inherit this, so they may be added to a compression object list
// and be able to use each individually to determine the best performance in either size or
// speed.
//------------------
// The identify() function simply returns a string which tells what the name of the compressor is.
// The enumerate() function simply returns a unique number which will always identify this compression scheme for Vision.
// The compress() function will allocate and compress data from the passed in buffer to the allocated buffer.
// The decompres() function will decompress from the source buffer to the destination buffer.
//------------------

#ifndef COMPRESS_H
#define COMPRESS_H

#include "first.h"

//------------------  
// DO NOT RENUMBER THESE.  Data files store this number to know what algorithm to call to decompress itself
enum CompressorTypes  
{
	CT_RawCompressor   = 0,
	CT_HughesTransform = 1,
	CT_LZ77            = 2,
	CT_ELSCoder        = 3,
	CT_RefPack         = 4,
};

//------------------  
// This is a data class.  
// Its sole purpose is for new compressors which require special data
// to be able to sneak in new information without breaking old compressors.
// Any additional data a compressor needs must come from a derivative of CompressorData
class CompressorData
{
public:
};

//------------------
// Notice that the compressor and decompressor input/output pairs are symmetric, so you should be able to pass directly from
// one function to the next without doing any conversion.  Data is always conserved from one conversion to the next.  Neat, huh?
//------------------
// These are defined because I don't know how these parameters may change, depending on each compressor's needs.
struct CompressorInput
{
	void    *buffer;
	u_int32  lengthInBytes;

	CompressorData *data;
};

//------------------  
// These are defined because I don't know how these parameters may change, depending on each decompressor's needs.
struct DecompressorInput
{
	void    *buffer;
	u_int32  lengthInBytes;
};

//------------------

class Compressor
{
public:
	Compressor(void);
	virtual ~Compressor(void);

	virtual const char      *identify(void) = 0;
	virtual CompressorTypes  enumerate(void) = 0;
	virtual void             compress(const CompressorInput &input,DecompressorInput *output) = 0;
	virtual void             decompress(const DecompressorInput &input,CompressorInput *output) = 0;
};

//------------------

#endif