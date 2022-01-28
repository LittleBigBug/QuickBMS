//------------------------------
// ELSCoder.h
// integrated by Jason Hughes
// taken from Witter's internet 
// publicized "elscoder.pdf"
// Orign Systems
// 1998
//------------------------------

#ifndef ELSCODER_H
#define ELSCODER_H

#include "Compress.h"

#include "Bitstream.h"

//------------------------------

class ELSCoder : public Compressor
{
protected:
	enum
	{
		JOTS_PER_BYTE = 754,   // this is the optimal setting.  Don't touch.
	};

//------------------------------

	struct LadderStruct
	{
		u_int16 *threshold;
		int16    codeLength0;
		int16    codeLength1;
		u_int8   next0;
		u_int8   next1;
	};

//------------------------------

	struct DecodeInfo
	{
		u_int16    internalBuffer;
		int32      jotCount;
		u_int32    externalIndex;
		u_int8    *externalIn;
	};

//------------------------------

	struct EncodeInfo
	{
		u_int32    min;
		int32      jotCount;
		int32      backlog;
		Bitstream *externalOut;
	};	

//------------------------------

	static LadderStruct ladder[256];
	static u_int16      allowable[2*JOTS_PER_BYTE];

	EncodeInfo   encodeInfo;
	DecodeInfo   decodeInfo;
	u_int8       context[256];

	int  elsDecodeOk(void);                    // Perform consistency check prior to ending decoding
	int  elsDecodeBit(u_int8 *rung);           // Decode a single bit
	void elsEncodeBit(u_int8 *rung, int bit);  // Encode a single bit
	void elsEncodeEnd(void);                   // Conclude encoding

public:
	ELSCoder(void);
	virtual ~ELSCoder(void);

	virtual const char      *identify(void);
	virtual CompressorTypes  enumerate(void);
	virtual void             compress(const CompressorInput &input,DecompressorInput *output);
	virtual void             decompress(const DecompressorInput &input,CompressorInput *output);
};

#endif
