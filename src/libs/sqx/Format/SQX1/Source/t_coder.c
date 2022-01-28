/***************************************************************************
T_CODER.C


C/C++ version Copyright (c) 1995 - 2002
Rainer Nausedat
Bahnhofstrasse 32
26316 Varel

All rights reserved.

-------------------------------------------------------------------------

 License Terms

 The free use of this software in both source and binary 
 form is allowed (with or without changes) provided that:

   1. you comply with the End-User License Agreement for
      this product. Please refer to lincense.txt for the
      complete End-User License Agreement.
   
   2. distributions of this source code include the above copyright 
      notice, this list of conditions, the complete End-User License 
      Agreement and the following disclaimer;

   3. the copyright holder's name is not used to endorse products 
      built using this software without specific written permission. 

 Disclaimer

 This software is provided 'as is' with no explcit or implied warranties
 in respect of any properties, including, but not limited to, correctness 
 and fitness for purpose.

***************************************************************************/

#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

#define STD_DELTA_FREQ	0xFF

#define IA32_IMGAGE_TAG		0xE8
#define IMGAGE_MIN			(-2147483647L - 1) 
#define IMGAGE_MAX			2147483647L   

typedef struct
{
	UWORD32		uFreqs[6][256];
	UBYTE		cSamps[8];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
	UWORD32		uDeltaFreq[8];
	UWORD32		uDeltaTotalFreq;
}DELTA_STRUCT;

typedef struct
{
	UBYTE		cSamps[8];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
	UWORD32		uDeltaFreq[8];
	UWORD32		uDeltaTotalFreq;
}DELTA_SUB_STRUCT;

typedef struct
{
	UWORD32		uFreqsHigh[4][256];
	UWORD32		uFreqsLow[4][256];
	UWORD16		usSamps[8];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
}DELTA_16_STRUCT;


typedef struct
{
	UWORD16		usSamps[8];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
}DELTA_16_SUB_STRUCT;

static DELTA_STRUCT LzDelta[4];
static DELTA_16_STRUCT LzDelta16;

static DELTA_STRUCT StdDelta[4];
static DELTA_16_STRUCT StdDelta16;

static 
void InitDelta16(DELTA_16_STRUCT *DeltaStruct,UWORD32 uStartDelta,UWORD32 uUpdateFreq)
{
	memset(DeltaStruct,0,sizeof(DELTA_16_STRUCT));
	DeltaStruct->uDelta = uStartDelta;
	DeltaStruct->uUpdateFreq = uUpdateFreq;
}

static 
void InitDelta(DELTA_STRUCT *DeltaStruct,UWORD32 uStartDelta,UWORD32 uUpdateFreq)
{
	memset(DeltaStruct,0,sizeof(DELTA_STRUCT));
	DeltaStruct->uDelta = uStartDelta;
	DeltaStruct->uUpdateFreq = uUpdateFreq;
}

static 
UWORD16 DecodeDelta16(DELTA_16_STRUCT *DeltaStruct,UWORD16 usDelta)
{
UWORD16		usError0;
UWORD16		usError1;
UWORD16		usError2;
UWORD16		usCurWord;
UWORD32		uFreqs_Count[8];
UWORD32		uCount;
UWORD32		uSmallest;
UWORD32		uSmallestCount;
	
	switch(DeltaStruct->uDelta)
	{
		case 0:
			usCurWord = DeltaStruct->usSamps[0] - usDelta;
			break;
		case 1:
			usCurWord = DeltaStruct->usSamps[1] - usDelta;
			break;
		case 2:
			usCurWord = DeltaStruct->usSamps[2] - usDelta;
			break;
	}

	usError0 = DeltaStruct->usSamps[0] - usCurWord;
	usError1 = DeltaStruct->usSamps[1] - usCurWord;
	usError2 = DeltaStruct->usSamps[2] - usCurWord;

	DeltaStruct->uFreqsHigh[0][(usError0 >> 8)]++;
	DeltaStruct->uFreqsHigh[1][(usError1 >> 8)]++;
	DeltaStruct->uFreqsHigh[2][(usError2 >> 8)]++;
	
	DeltaStruct->uFreqsLow[0][(usError0 & 0xFF)]++;
	DeltaStruct->uFreqsLow[1][(usError1 & 0xFF)]++;
	DeltaStruct->uFreqsLow[2][(usError2 & 0xFF)]++;
	

	DeltaStruct->usSamps[2] = DeltaStruct->usSamps[1];
	DeltaStruct->usSamps[1] = DeltaStruct->usSamps[0];
	DeltaStruct->usSamps[0] = usCurWord;
	
	if( (DeltaStruct->uCurrCount & DeltaStruct->uUpdateFreq) == DeltaStruct->uUpdateFreq )
	{
		memset(uFreqs_Count,0,sizeof(uFreqs_Count));

		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqsHigh[0][uCount] > 0 )
				uFreqs_Count[0]++;
			if( DeltaStruct->uFreqsLow[0][uCount] > 0 )
				uFreqs_Count[0]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqsHigh[1][uCount] > 0 )
				uFreqs_Count[1]++;
			if( DeltaStruct->uFreqsLow[1][uCount] > 0 )
				uFreqs_Count[1]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqsHigh[2][uCount] > 0 )
				uFreqs_Count[2]++;
			if( DeltaStruct->uFreqsLow[2][uCount] > 0 )
				uFreqs_Count[2]++;
		}
			
		uSmallest = uFreqs_Count[0];
		uSmallestCount = 0;

		for( uCount = 1; uCount < 3; uCount++ )
		{
			if( uFreqs_Count[uCount] < uSmallest )
			{
				uSmallest = uFreqs_Count[uCount];
				uSmallestCount = uCount;
			}
		}
		DeltaStruct->uDelta = uSmallestCount;
		memset(DeltaStruct,0,sizeof(DELTA_16_STRUCT) - sizeof(DELTA_16_SUB_STRUCT));
	}

	DeltaStruct->uCurrCount++;

	return usCurWord;
}

static 
UBYTE DecodeDelta(DELTA_STRUCT *DeltaStruct,UBYTE cDeltaByte)
{
UBYTE		cError0;
UBYTE		cError1;
UBYTE		cError2;
UBYTE		cError3;
UBYTE		cError4;
UBYTE		uCurByte;
UWORD32		uFreqs_Count[8];
UWORD32		uCount;
UWORD32		uSmallest;
UWORD32		uSmallestCount;

	switch(DeltaStruct->uDelta)
	{
		case 0:
			uCurByte = cDeltaByte;
			break;
		case 1:
			uCurByte = DeltaStruct->cSamps[0] - cDeltaByte;
			break;
		case 2:
			uCurByte = DeltaStruct->cSamps[1] - cDeltaByte;
			break;
		case 3:
			uCurByte = DeltaStruct->cSamps[2] - cDeltaByte;
			break;
		case 4:
			uCurByte = DeltaStruct->cSamps[3] - cDeltaByte;
			break;
	}

	cError0 = uCurByte;
	cError1 = DeltaStruct->cSamps[0] - uCurByte;
	cError2 = DeltaStruct->cSamps[1] - uCurByte;
	cError3 = DeltaStruct->cSamps[2] - uCurByte;
	cError4 = DeltaStruct->cSamps[3] - uCurByte;

	DeltaStruct->uFreqs[0][cError0]++;
	DeltaStruct->uFreqs[1][cError1]++;
	DeltaStruct->uFreqs[2][cError2]++;
	DeltaStruct->uFreqs[3][cError3]++;
	DeltaStruct->uFreqs[4][cError4]++;

	DeltaStruct->cSamps[3] = DeltaStruct->cSamps[2];
	DeltaStruct->cSamps[2] = DeltaStruct->cSamps[1];
	DeltaStruct->cSamps[1] = DeltaStruct->cSamps[0];
	DeltaStruct->cSamps[0] = uCurByte;

	if( (DeltaStruct->uCurrCount & DeltaStruct->uUpdateFreq) == DeltaStruct->uUpdateFreq )
	{
		memset(uFreqs_Count,0,sizeof(uFreqs_Count));

		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqs[0][uCount] > 0 )
				uFreqs_Count[0]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqs[1][uCount] > 0 )
				uFreqs_Count[1]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqs[2][uCount] > 0 )
				uFreqs_Count[2]++;
		}
			
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqs[3][uCount] > 0 )
				uFreqs_Count[3]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( DeltaStruct->uFreqs[4][uCount] > 0 )
				uFreqs_Count[4]++;
		}
				
		uSmallest = uFreqs_Count[0];
		uSmallestCount = 0;

		for( uCount = 1; uCount < 5; uCount++ )
		{
			if( uFreqs_Count[uCount] < uSmallest )
			{
				uSmallest = uFreqs_Count[uCount];
				uSmallestCount = uCount;
			}
		}

		if( DeltaStruct->uDelta == uSmallestCount )
		{
			DeltaStruct->uUpdateFreq += DeltaStruct->uUpdateFreq + 1;
			DeltaStruct->uUpdateFreq &= 4095;
		}
		else
			DeltaStruct->uUpdateFreq = 0x1FF;

		DeltaStruct->uDelta = uSmallestCount;
		memset(DeltaStruct,0,sizeof(DELTA_STRUCT) - sizeof(DELTA_SUB_STRUCT));
	}

	DeltaStruct->uCurrCount++;

	return uCurByte;
}

static 
void DeconvRxType(DELTA_STRUCT *DeltaStruct,UWORD32 uNumCoder,UBYTE *pBuf,UWORD32 uSize)
{
UWORD32		uMaxCount;
UWORD32		uCount;
UWORD32		uCIndex;

	uMaxCount = uSize;
	uMaxCount /= uNumCoder;
	uMaxCount *= uNumCoder;
	uCount = uCIndex = 0;
	
	while( uCount < uMaxCount )
	{
		pBuf[uCount++] = DecodeDelta(&DeltaStruct[uCIndex++],pBuf[uCount]);
		if( uCIndex == uNumCoder )
			uCIndex = 0;
	}
}

static 
void DeconvR4_16Type(DELTA_16_STRUCT *DeltaStruct,UBYTE *pBuf,UWORD32 uSize)
{
UWORD32			uCount;
UWORD32			uMaxCount;
UWORD16			*pWordBuf;

	pWordBuf = (UWORD16 *)pBuf;
	uMaxCount = uSize;
	uMaxCount >>= 1;
	uCount = 0;
	while( uCount < uMaxCount )
		pWordBuf[uCount++] = DecodeDelta16(DeltaStruct,pWordBuf[uCount]);
}

static 
void InitLzDeltas(void)
{
	InitDelta(&LzDelta[0],0,STD_DELTA_FREQ);
	InitDelta(&LzDelta[1],0,STD_DELTA_FREQ);
	InitDelta(&LzDelta[2],0,STD_DELTA_FREQ);
	InitDelta(&LzDelta[3],0,STD_DELTA_FREQ);
}

static 
void InitStdDeltas(UWORD32 uStartDelta)
{
	InitDelta(&StdDelta[0],uStartDelta,STD_DELTA_FREQ);
	InitDelta(&StdDelta[1],uStartDelta,STD_DELTA_FREQ);
	InitDelta(&StdDelta[2],uStartDelta,STD_DELTA_FREQ);
	InitDelta(&StdDelta[3],uStartDelta,STD_DELTA_FREQ);
}

static 
void DeconvRType(UWORD32 uStartDeltaCode,UWORD32 uCoderType,UWORD32 uCoderNum,UBYTE *pBuf,UWORD32 uSize)
{
	if( uCoderType == DCODER_16BIT )
	{
		InitDelta16(&StdDelta16,uStartDeltaCode,STD_DELTA_FREQ);
		DeconvR4_16Type(&StdDelta16,pBuf,uSize);
	}
	else if( uCoderType == DCODER_8BIT )
	{
		InitStdDeltas(uStartDeltaCode);
		DeconvRxType((DELTA_STRUCT *)&StdDelta,uCoderNum,pBuf,uSize);
	}
}

static 
void InitConvR_LZ_Type(void)
{
	if( DeltaCtrl.uCoderType == DCODER_8BIT )
		InitLzDeltas();
	else if( DeltaCtrl.uCoderType == DCODER_16BIT )
		InitDelta16(&LzDelta16,0,STD_DELTA_FREQ);
}

static 
void DeconvR_LZ_Type(UBYTE *pBuf,UWORD32 uSize)
{
	if( DeltaCtrl.uCoderType == DCODER_16BIT )
		DeconvR4_16Type(&LzDelta16,pBuf,uSize);
	else if( DeltaCtrl.uCoderType == DCODER_8BIT )
		DeconvRxType((DELTA_STRUCT *)&LzDelta,DeltaCtrl.uCoderNum,pBuf,uSize);
}

static 
void ReImage(UBYTE *pBuffer,UWORD32 uBuflen)
{
SLONG32	lAbsA;
SLONG32	lCount;
SLONG32	lLastCount;
SLONG32	lImageSize;
SLONG32	*pDataPtr;

	if( uBuflen < 0x100 )
	{
		IoStruct.lImageLastCount += (long)uBuflen;
		return;
	}

	lLastCount = IoStruct.lImageLastCount;
	lImageSize = IoStruct.lImageSize;

	for( lCount = 0; lCount < (long)uBuflen - 7; lCount++)
	{
		if( pBuffer[lCount] == IA32_IMGAGE_TAG )
		{
			pDataPtr = (SLONG32 *)&pBuffer[lCount + 1];
			lAbsA = *pDataPtr;

			if( (lAbsA == IMGAGE_MIN) || (lAbsA == IMGAGE_MAX) )
			{
				lCount += 4;
				continue;
			}

			if( lAbsA > 0 && lAbsA < lImageSize)
				*pDataPtr = lAbsA - (lCount + lLastCount);
			else if( ((-lAbsA) < (lCount + lLastCount)) && ((-lAbsA) > 0) )
				*pDataPtr = lImageSize + lAbsA;
			
			lCount += 4;
		}
	}
	IoStruct.lImageLastCount += (SLONG32)uBuflen;
}

