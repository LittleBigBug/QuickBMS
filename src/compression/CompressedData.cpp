// modified by Luigi Auriemma

//***************************************************************************
//
// Implementation of DecompressJCALG1 in C++ by Jeffrey Lim
// 
// JCALG1 is copyright (C) Jeremy Collake
//
// Note: This code should work on any little-endian system that supports
//		 mis-aligned reads. To support non-little endian, the functions
//		 CompressionSourceData::GetBit() and CompressionSourceData::GetBits(int)
//		 need to be modified.
//		 If the target system does not support mis-aligned reads, either
//		 the actual decompresssion data has to be made to align to 4-byte 
//		 boundaries [note that there's a 10-byte header on JCALG1], or else 
//		 the CompressionSourceData routines need to be modified.
//
//***************************************************************************

//#include "CompressedData.h"
#include <string.h>

//***************************************************************************

class CompressionSourceData
{
private:
	const unsigned *Data;
	unsigned		BitBuffer;
	int				BitsRemaining;

public:
	CompressionSourceData(const unsigned *iData);

	int GetBit();
	int GetBits(int Count);
	int GetInteger();
};

CompressionSourceData::CompressionSourceData(const unsigned *iData)
{
	Data			= iData;
	BitBuffer		= 0;
	BitsRemaining	= 0;
}

int CompressionSourceData::GetBit()
{
	int ReturnValue;

	if(!BitsRemaining)
	{
		BitsRemaining = 32;
		BitBuffer = *Data++;
	}

	ReturnValue = BitBuffer >> 31;
	BitBuffer <<= 1;
	BitsRemaining--;
	return ReturnValue;
}

int CompressionSourceData::GetBits(int Count)
{
	if(BitsRemaining >= Count)
	{
		int ReturnValue = BitBuffer >> (32-Count);
		BitBuffer <<= Count;
		BitsRemaining -= Count;
		return ReturnValue;
	}
	else
	{
		int Remainder = Count-BitsRemaining;

		int ReturnValue = BitBuffer >> (32-BitsRemaining) << Remainder;
		BitBuffer = *Data++;

		ReturnValue |= BitBuffer >> (32-Remainder);
		BitsRemaining = 32-Remainder;
		BitBuffer <<= Remainder;

		return ReturnValue;
	}
}

int CompressionSourceData::GetInteger()
{
	int Value = 1;
	do
	{
		Value = (Value<<1) + GetBit();
	} while(GetBit());

	return Value;
}

//***************************************************************************

struct CompressionState
{
	int		LastIndex;
	int		IndexBase;
	int		LiteralBits;
	int		LiteralOffset;

	CompressionState() { LastIndex = 1; IndexBase = 8; LiteralBits = 0; LiteralOffset = 0; }
};

//***************************************************************************

void TransferMatch(unsigned char* &Destination, int MatchOffset, int MatchLength)
{
	unsigned char* p = Destination;
	unsigned char* s = p-MatchOffset;

	do
	{
		*p++ = *s++;
	} while(--MatchLength);

	Destination = p;
}

//***************************************************************************

extern "C" int DecompressJCALG1(void *pDestination, const unsigned *pSource)
{
	CompressionState		State;
	CompressionSourceData	Source(pSource+10);		// +10 skips the header of JCALG. A more robust routine would check this
	unsigned char *Destination = (unsigned char*) pDestination;

	while(1)
	{
		if(Source.GetBit())
		{
			// Is Literal
			*Destination++ = Source.GetBits(State.LiteralBits) + State.LiteralOffset;
		}
		else
		{
			// Isn't literal
			if(Source.GetBit())
			{
				// Normal phrase
				int HighIndex = Source.GetInteger();

				if(HighIndex == 2) // Use the last index
				{
					int PhraseLength = Source.GetInteger();
					TransferMatch(Destination, State.LastIndex, PhraseLength);
				}
				else
				{
					State.LastIndex = ((HighIndex-3) << State.IndexBase) + Source.GetBits(State.IndexBase);

					int PhraseLength = Source.GetInteger();

					if(State.LastIndex >= 0x10000)		PhraseLength += 3;
					else if(State.LastIndex >= 0x37FF)	PhraseLength += 2;
					else if(State.LastIndex >= 0x27F)	PhraseLength++;
					else if(State.LastIndex <= 127)		PhraseLength += 4;

					TransferMatch(Destination, State.LastIndex, PhraseLength);
				}
			}
			else
			{
				if(Source.GetBit())
				{
					// OneBytePhrase or literal size change
					int OneBytePhraseValue = Source.GetBits(4) - 1;
					if(OneBytePhraseValue == 0)
					{
						*Destination++ = 0;
					}
					else if(OneBytePhraseValue > 0)
					{
						*Destination = *(Destination-OneBytePhraseValue);
						Destination++;
					}
					else
					{
						if(Source.GetBit())
						{
							// Next block
							do
							{
								for(int i = 0; i < 256; i++) *Destination++ = Source.GetBits(8);
							} while(Source.GetBit());
						}
						else
						{
							// New literal size
							State.LiteralBits = 7+Source.GetBit();
							State.LiteralOffset = 0;
							if(State.LiteralBits != 8)
							{
								State.LiteralOffset = Source.GetBits(8);
							}
						}
					}
				}
				else
				{
					// Short match
					int NewIndex = Source.GetBits(7);
					int MatchLength = 2+Source.GetBits(2);

					if(NewIndex == 0)
					{
						// Extended short
						if(MatchLength == 2) break;	// End of decompression

						State.IndexBase = Source.GetBits(MatchLength+1);
					}
					else
					{
						State.LastIndex = NewIndex;
						TransferMatch(Destination, State.LastIndex, MatchLength);
					}
				}
			}
		}
	}
    return Destination - (unsigned char*)pDestination;
}

//***************************************************************************
