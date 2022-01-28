#ifndef __IINOUTSTREAMS_H
#define __IINOUTSTREAMS_H

#include "Portable.h"

class ISequentialInStream
{
	const char* data;
	INT size;
public:
	ISequentialInStream(const char* Adata, INT Asize) : data(Adata), size(Asize) { }

	HRESULT Read(void *aData, INT aSize, INT *aProcessedSize);
};

class ISequentialOutStream
{
	char* data;
	INT size;
	bool overflow;
	INT total;
public:
	ISequentialOutStream(char* Adata, unsigned Asize) : data(Adata), size(Asize), overflow(false), total(0) { }

	bool overflow_get() const { return overflow; }
	INT size_get() const { return total; }

	HRESULT Write(const void *aData, INT aSize, INT *aProcessedSize);
};

#endif
