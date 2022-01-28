//-----------------------
// RefPack.h
// Jason Hughes
// Origin Systems
// 1998
//-----------------------

#ifndef REFPACK_H
#define REFPACK_H

#include "Compress.h"

class RefPack : public Compressor
{
protected:
	static u_int32 crctab[256];      /* CRC combining table */

	u_int32        matchlen(const u_int8 *s,const u_int8 *d, u_int32 maxmatch);
	u_int32        HASH(const u_int8 *s);

public:
	RefPack(void);
	virtual ~RefPack(void);

	virtual const char      *identify(void);
	virtual CompressorTypes  enumerate(void);
	virtual void             compress(const CompressorInput &input,DecompressorInput *output);
	virtual void             decompress(const DecompressorInput &input,CompressorInput *output);
};

//-----------------------

inline u_int32 RefPack::HASH(const u_int8 *s)
{
	u_int32 crc;

	crc = crctab[static_cast<u_int8>(s[0])];
	crc = crctab[static_cast<u_int8>(crc ^ s[1])] ^ (crc>>8);
	crc = crctab[static_cast<u_int8>(crc ^ s[2])] ^ (crc>>8);

	return crc;
}

#endif