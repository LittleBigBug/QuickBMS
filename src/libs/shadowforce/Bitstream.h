#ifndef BITSTREAM_H
#define BITSTREAM_H

class Bitstream
{
protected:
	u_int8         *stream;           // array of bits
	u_int32         lengthOfStream;   // number of bytes in stream currently allocated
	mutable u_int32 streamIndex;      // iterator for reading or writing to stream

public:
	// this constructor is used when you have a buffer with data you want to operate on already loaded
	Bitstream(const u_int8 *buffer,u_int32 lengthInBytes);

	// this constructor is to create a buffer of some reasonable size to put data in, and pull back out
	Bitstream(u_int32 lengthInBytes);
	~Bitstream(void);

	void    resizeStream(u_int32 lengthInBytes);
	u_int32 getLength(void) const;

	// these functions allow you to directly query or set bits anywhere without affecting the streamIndex
	// note that (bitIndex % 8==0) means LSB, and (bitIndex % 8==7) means MSB
	u_int32 getBitIndex(u_int32 bitIndex,u_int32 byteIndex=0) const;
	void    setBitIndex(u_int32 bit,u_int32 bitIndex,u_int32 byteIndex=0);

	// these functions allow you to walk through the data from LSB to MSB
	u_int32 readBit(void) const;
	void    writeBit(u_int32 bit);

	// this function allows you to change where the readBit/writeBit functions are looking
	void    repositionStream(u_int32 newIndex);
	u_int32 getPosition(void) const;

	// these functions read to and write from a buffer in whole bytes only
	void writeBuffer(const void *buffer,u_int32 lengthInBytes);
	void readBuffer(void *buffer,u_int32 lengthInBytes) const;

	// this function allows you to retrieve the constructed buffer by creating a new memory block and returning it
	void *copyStream(void) const;
};

#endif