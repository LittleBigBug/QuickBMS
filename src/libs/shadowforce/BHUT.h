#ifndef BHUT_H
#define BHUT_H

#include "Bitstream.h"
#include "Compress.h"

class HughesTransform : public Compressor
{
protected:
	Bitstream *srcBits;    // source bit buffer
	Bitstream *destBits;   // destination bit buffer

	void release(void);

	void *copyStream(void) const;

	u_int32 countBits(bool flavor,u_int32 a,u_int32 b,const Bitstream *bucket);

	void applyHT(u_int32 start,u_int32 stop,u_int32 numZeros);
	void applyHTInverse(u_int32 start,u_int32 stop,u_int32 numZeros);

public:
	HughesTransform(void);
	virtual ~HughesTransform(void);

	virtual const char      *identify(void);
	virtual CompressorTypes  enumerate(void);
	virtual void             compress(const CompressorInput &input,DecompressorInput *output);
	virtual void             decompress(const DecompressorInput &input,CompressorInput *output);
};

#endif