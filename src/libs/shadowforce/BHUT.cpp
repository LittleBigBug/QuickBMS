#include "first.h"
#include "BHUT.h"

//--------------------------------------------------------------------

static u_int32 bitMask[32] = { 1,2,4,8,16,32,64,128,
                             256,512,1024,2048,4096,8192,16384,32768,
                             65536,131072,262144,524288,1048576,2097152,4194304,8388608,
                             16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648UL };

//--------------------------------------------------------------------

HughesTransform::HughesTransform(void)
{
	srcBits=NULL;
	destBits=NULL;
}

HughesTransform::~HughesTransform(void)
{
	release();  // just in case something weird happened
}

void HughesTransform::release(void)
{
	delete srcBits;
	srcBits=NULL;

	delete destBits;
	destBits=NULL;
}

//--------------------------------------------------------------------

u_int32 calculateRequiredBits(u_int32 value)
{
	u_int32 count=0;
	while (value)
	{
		value>>=1;
		count++;
	}
	return count;
}

u_int32 HughesTransform::countBits(bool flavor,u_int32 a,u_int32 b,const Bitstream *bucket)
{
	u_int32 count,loop;

	for (count=0, loop=a; loop<=b; loop++)
		count+=bucket->getBitIndex(loop);  // count only ones

	if (!flavor)
		count=(b-a+1)-count;               // invert the count if we're looking for zeroes

	return count;
}

//-----------------------------------------------------------------
// Here come the higher level interface and guts
//-----------------------------------------------------------------

// applyHT will convert the data from srcBits to 
void HughesTransform::applyHT(u_int32 start,u_int32 stop,u_int32 numZeros)
{
	u_int32 loop;
	u_int32 numOnes;

	numOnes=stop-start+1-numZeros;

	if (numOnes==0 || numZeros==0)
	{
		// there's really nothing to do, because this entire region is intrinsically same bits
	}
	else
	{
		u_int32 leftStop;  // now we choose where to split the region
		u_int32 dataSize;             // denotes the actual number of bits required by the algorithm to represent cnt
		u_int32 numZerosLeft,numOnesLeft;  // destination should look like this

		leftStop=stop-((stop-start) >> 1)-1;  // divide this region into two regions, rounding split nodes to the right

		numOnesLeft=0;
		numZerosLeft=0;

		if (numOnes<numZeros)  // let's tell where the ones are going
		{
			dataSize=calculateRequiredBits(min(numOnes,leftStop-start+1));  // get the number of bits required to represent this value
			numOnesLeft=countBits(true,start,leftStop,srcBits);  // get actual destination value

			numZerosLeft=leftStop-start+1-numOnesLeft;

			for (loop=0; loop<dataSize; loop++)
				destBits->writeBit(numOnesLeft & bitMask[loop]);                  // store number of (1/0)? bits to put (L/R)?, from LSB to MSB
		}
		else
		{
			dataSize=calculateRequiredBits(min(numZeros,leftStop-start+1));  // get the number of bits required to represent this value
			numZerosLeft=countBits(false,start,leftStop,srcBits);  // get actual destination value
			numOnesLeft=leftStop-start+1-numZerosLeft;

			for (loop=0; loop<dataSize; loop++)
				destBits->writeBit(numZerosLeft & bitMask[loop]);                  // store number of (1/0)? bits to put (L/R)?, from LSB to MSB
		}

/*		for (loop=0; loop<start; loop++)
			printf(" ");
		for (loop=0; loop<numZerosLeft; loop++)
			printf("0");
		for (loop=0; loop<numOnesLeft; loop++)
			printf("1");
		for (loop=0; loop<numZerosRight; loop++)
			printf("0");
		for (loop=0; loop<numOnesRight; loop++)
			printf("1");
		printf("\n");
*/
		applyHT(start,leftStop,numZerosLeft);  // recurse the left side
		applyHT(leftStop+1,stop,numZeros-numZerosLeft); // recurse the right side
	}
}

void HughesTransform::applyHTInverse(u_int32 start,u_int32 stop,u_int32 numZeros)
{
	u_int32 loop;
	u_int32 numOnes;

	numOnes=stop-start+1-numZeros;

	if (!numZeros)
	{
		// there's really nothing to do, because this entire region is intrinsically same bits
		FATAL(numOnes!=stop-start+1,("Wackadoo is stuck in gear seven.\n"));
	 	for (loop=0; loop<numOnes; loop++)
	 		destBits->setBitIndex(1,loop+start);
	}
	else if (!numOnes)
	{
		// there's really nothing to do, because this entire region is intrinsically same bits
		FATAL(numZeros!=stop-start+1,("Wackadoo is stuck in gear seven.\n"));
	 	for (loop=0; loop<numZeros; loop++)
	 		destBits->setBitIndex(0,loop+start);
	}
	else
	{
		u_int32 leftStop,rightStart;  // now we choose where to split the region
		u_int32 dataSize;             // denotes the actual number of bits required by the algorithm to represent cnt
		u_int32 numZerosLeft,numOnesLeft,numZerosRight,numOnesRight;  // destination should look like this

		rightStart=stop-((stop-start) >> 1);
		leftStop=rightStart-1;

		numOnesLeft=0;
		numOnesRight=0;
		numZerosLeft=0;
		numZerosRight=0;

		if (numOnes<numZeros)  // let's tell where the ones are going
		{
			dataSize=calculateRequiredBits(min(numOnes,leftStop-start+1));  // get the number of bits required to represent this value

			for (loop=0; loop<dataSize; loop++)
				numOnesLeft|=srcBits->readBit() << loop;
			numZerosLeft=leftStop-start+1-numOnesLeft;
			numOnesRight=numOnes-numOnesLeft;
			numZerosRight=numZeros-numZerosLeft;
		}
		else
		{
			dataSize=calculateRequiredBits(min(numZeros,leftStop-start+1)); // get the number of bits required to represent this value

			for (loop=0; loop<dataSize; loop++)
				numZerosLeft|=srcBits->readBit() << loop;
			numOnesLeft=leftStop-start+1-numZerosLeft;
			numZerosRight=numZeros-numZerosLeft;
			numOnesRight=numOnes-numOnesLeft;
		}

		// now, we know how many of whatever type we need to send, and where.
		// Let's do it.
		/*{
			for (loop=0; loop<start; loop++)
				printf(" ");
			for (loop=0; loop<numZerosLeft; loop++)
			{
				printf("0");
			}
			for (loop=0; loop<numOnesLeft; loop++)
			{
				printf("1");
			}
			for (loop=0; loop<numZerosRight; loop++)
			{
				printf("0");
			}
			for (loop=0; loop<numOnesRight; loop++)
			{
				printf("1");
			}
			printf("\n");
		}*/

		applyHTInverse(start,leftStop,numZerosLeft);  // recurse the left side
		applyHTInverse(rightStart,stop,numZerosRight);  // recurse the right side
	}
}

//--------------------------------------------------------------------

const char *HughesTransform::identify(void)
{
	return "Hughes Transform";
}

CompressorTypes HughesTransform::enumerate(void)
{
	return CT_HughesTransform;
}

void HughesTransform::compress(const CompressorInput &input,DecompressorInput *output)
{
	u_int32 loop,zeroesCounter;
	u_int32 headerLength;

	srcBits=NEW Bitstream(static_cast<const u_int8 *>(input.buffer),input.lengthInBytes);
	destBits=NEW Bitstream(srcBits->getLength()*2);                // just to be safe

	headerLength=30 +                                              // 30 bits for the length of the source data in BYTES (1 terabyte maximum)
	        calculateRequiredBits(srcBits->getLength()*8);         // number of zeroes in source data (number of bits possible)

	zeroesCounter=countBits(false,0,srcBits->getLength()*8-1,srcBits);

	for (loop=0; loop<30; loop++)
		destBits->writeBit(srcBits->getLength() & bitMask[loop]);  // go LSB to MSB order
	for (loop=30; loop<headerLength; loop++)
		destBits->writeBit(zeroesCounter & bitMask[loop-30]);      // again, go LSB to MSB

	applyHT(0,srcBits->getLength()*8-1,zeroesCounter);

	output->buffer=MEM_OWN(destBits->copyStream());
	output->lengthInBytes=destBits->getPosition()/8+((destBits->getPosition() & 7)?1:0);

	release();                                                     // delete the srcBits and destBits structures
}

void HughesTransform::decompress(const DecompressorInput &input,CompressorInput *output)
{
	u_int32 loop,zeroesCounter,zeroesCounterBits,numDestBytes;

	srcBits=NEW Bitstream(static_cast<const u_int8 *>(input.buffer),input.lengthInBytes);

	for (loop=0, numDestBytes=0; loop<30; loop++)                  // reconstruct the number of bits in the source image, LSB to MSB
		numDestBytes|=srcBits->readBit() << loop;

	destBits=NEW Bitstream(numDestBytes);                          // make sure there are enough bytes to hold all the bits

	zeroesCounterBits=calculateRequiredBits(numDestBytes*8);       // number of binary places we need to load to find out the number of zeroes in source data

	for (loop=0, zeroesCounter=0; loop<zeroesCounterBits; loop++)  // reconstruct the number of zero bits in the source image
		zeroesCounter|=srcBits->readBit() << loop;

	applyHTInverse(0,destBits->getLength()*8-1,zeroesCounter);

	output->buffer=MEM_OWN(destBits->copyStream());
	output->lengthInBytes=destBits->getLength();

	release();
}

//--------------------------------------------------------------------
