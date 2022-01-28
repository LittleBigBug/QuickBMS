#include "first.h"
#include "Bitstream.h"

//---------------------------------------------------

static u_int8 bitMask[8] = { 1,2,4,8,16,32,64,128 };  // faster than a shift, I hope

//---------------------------------------------------

Bitstream::Bitstream(const u_int8 *buffer,u_int32 lengthInBytes)
{
	stream=NULL;
	lengthOfStream=0;
	streamIndex=0;

	resizeStream(lengthInBytes);

	memcpy(stream,buffer,lengthInBytes);
	repositionStream(0);
}

Bitstream::Bitstream(u_int32 lengthInBytes)
{
	stream=NULL;
	lengthOfStream=0;
	streamIndex=0;

	resizeStream(lengthInBytes);
}

Bitstream::~Bitstream(void)
{
	delete []stream;
	stream=NULL;

	lengthOfStream=0;
	streamIndex=0;
}

void *Bitstream::copyStream(void) const
{
	char *retBuf;

	retBuf=NEW char[lengthOfStream];
	memcpy(retBuf,stream,lengthOfStream);

	return static_cast<void *>(retBuf);
}

//---------------------------------------------------

u_int32 Bitstream::getBitIndex(u_int32 bitIndex,u_int32 byteIndex) const
{
	u_int32 fetchFromByte,bitLoc;

	fetchFromByte=bitIndex/8+byteIndex;
	bitLoc=bitIndex & 7;
	FATAL(fetchFromByte>=lengthOfStream,("Bitstream::getBitIndex() bitIndex is out of range.\n"));
	return (stream[fetchFromByte] & bitMask[bitLoc]) >> bitLoc;  // shift it down to 1 or 0
}

void Bitstream::setBitIndex(u_int32 bit,u_int32 bitIndex,u_int32 byteIndex)  // directly twiddle a bit
{
	u_int32 writeToByte,bitLoc;

	writeToByte=bitIndex/8+byteIndex;
	bitLoc=bitIndex & 7;
	FATAL(writeToByte>=lengthOfStream,("Bitstream::setBitIndex() bitIndex is out of range.\n"));

	stream[writeToByte]&=~bitMask[bitLoc];             // always clear this bit first
	if (bit)									 
			stream[writeToByte]|=bitMask[bitLoc];      // if the bit value is non-zero, set it again
}

u_int32 Bitstream::readBit(void) const
{
	FATAL(streamIndex/8>=lengthOfStream,("Bitstream::readBit() bitIndex is out of range.\n"));
	return (stream[streamIndex/8] & bitMask[streamIndex & 7]) >> ((streamIndex++) & 7);
}

void Bitstream::writeBit(u_int32 bit)
{
	if (streamIndex/8>=lengthOfStream)                     // if we're writing to a stream, just let it grow to fit
		resizeStream(lengthOfStream*2);
	stream[streamIndex/8]&=~bitMask[streamIndex & 7];      // always clare this bit first
	if (bit)
		stream[streamIndex/8]|=bitMask[streamIndex & 7];   // if the bit value is non-zero, set it again
	streamIndex++;
}

void Bitstream::repositionStream(u_int32 newIndex)
{
	u_int32 fetchFromByte;

	fetchFromByte=newIndex/8;
	FATAL(fetchFromByte<0 || fetchFromByte>=lengthOfStream,("Bitstream::repositionStream() repositioned to invalid location.\n"));
	streamIndex=newIndex;
}

u_int32 Bitstream::getPosition(void) const
{
	return streamIndex;
}

u_int32 Bitstream::getLength(void) const
{
	return lengthOfStream;
}

void Bitstream::writeBuffer(const void *buffer,u_int32 lengthInBytes)
{
	u_int32 loop;

	for (loop=0; loop<lengthInBytes*8; loop++)
		writeBit(static_cast<const u_int8 *>(buffer)[loop/8] & bitMask[loop & 7]);  // from LSB to MSB for each byte
}

void Bitstream::readBuffer(void *buffer,u_int32 lengthInBytes) const
{
	u_int32 loop;

	memset(buffer,0,lengthInBytes);
	for (loop=0; loop<lengthInBytes*8; loop++)
		static_cast<char *>(buffer)[loop/8]|=readBit() << (loop & 7);  // from LSB to MSB for each byte
}

void Bitstream::resizeStream(u_int32 lengthInBytes)
{
	u_int8 *newStream;

	FATAL(lengthInBytes<lengthOfStream,("Cannot resize a BitStream smaller than it already is.\n"));

	newStream=NEW u_int8[lengthInBytes];
	memset(newStream,0,lengthInBytes);        // clear all of the stream first
	if (stream)
	{
		memcpy(newStream,stream,lengthOfStream);  // duplicate what was already in this stream
		delete []stream;
	}

	stream=newStream;

	lengthOfStream=lengthInBytes;
}