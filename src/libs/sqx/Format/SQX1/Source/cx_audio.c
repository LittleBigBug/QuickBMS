/*********************************************************
CX_AUDIO.C


C/C++ version Copyright (c) 1998 - 2002
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

#if !defined(__CX_ERR_H)   
#include "cx_err.h"
#endif

#if !defined(__CTAB_H)   
#include "ctab.h"
#endif

#if !defined(__ARCIO_H) 
#include "arcio.h"
#endif

#if !defined(__SQX_OS_H)   
#include "os/sqx_os.h"
#endif

#if !defined(__CRC32_H)
#include "crc32.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

//- in sqx_main.c
extern void SetProgress(int iFinished);

#define PD_COUNT				2
#define ANC						0x102
#define SAMP_PAGE_MAX			0x9000
#define MAX_PAGE_BUF			0xA000
#define SAMP_FRAME_MAX			384			
#define HEAD_T_BITS				1
#define CHANNEL_DEC_BITS		2
#define PRED_BITS				3
#define SHIFT_BITS				4
#define MAX_FRAME_BITS			11
#define FRAME_TYPE_STD			0x00000000
#define FRAME_TYPE_LAST			0x00000001
#define	MAX_8CODE_BITS			14


typedef struct
{
	UWORD32		uFreqs[6][256];
	UBYTE		cSamps[4];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
}AUDIO_DELTA_STRUCT;

typedef struct
{
	UBYTE		cSamps[4];
	UWORD32		uDelta;
	UWORD32		uUpdateFreq;
	UWORD32		uCurrCount;
}AUDIO_DELTA_SUB_STRUCT;


typedef struct
{
	INT32			iStartV;
	INT32			LV[8];
	UWORD32			u32PredT;
}ER_STRUCT;


CTAB_STRUCT		CTabAudio[2];
SQX_AUDIO_HEAD	AudioHead;
SLONG32			lSPageIndex;
INT16			*usSamples;
INT32			*pLeftOut[PD_COUNT];
INT32			*pRightOut[PD_COUNT];


void FreeAudioDecompressor(void)
{
SLONG32	lCount;

	for( lCount = 0; lCount < PD_COUNT; lCount++ )
	{
		if( pLeftOut[lCount] != NULL )
		{
			free(pLeftOut[lCount]);
			pLeftOut[lCount] = NULL;
		}
		if( pRightOut[lCount] != NULL )
		{
			free(pRightOut[lCount]);
			pRightOut[lCount] = NULL;
		}

	}
		
	if( usSamples != NULL )
	{
		free(usSamples);
		usSamples = NULL;
	}
	
	FreeCTab(&CTabAudio[0]);
	FreeCTab(&CTabAudio[1]);
}

int InitAudioDecompressor(void)
{
SLONG32	lCount;

	usSamples = NULL;
	memset(&CTabAudio,0,sizeof(CTabAudio));
	for( lCount = 0; lCount < PD_COUNT; lCount++ )
	{
		pLeftOut[lCount] = NULL;
		pRightOut[lCount] = NULL;
	}

	for( lCount = 0; lCount < PD_COUNT; lCount++ )
	{
		pLeftOut[lCount] = malloc(SAMP_FRAME_MAX * sizeof(INT32));
		if( pLeftOut[lCount] == NULL )
		{
			ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
			FreeAudioDecompressor();
			return (int)FALSE;
		}
		
		pRightOut[lCount] = malloc(SAMP_FRAME_MAX * sizeof(INT32));
		if( pRightOut[lCount] == NULL )
		{
			ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
			FreeAudioDecompressor();
			return (int)FALSE;
		}
	}
	
	usSamples = malloc(SAMP_PAGE_MAX);
	if( usSamples == NULL )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		FreeAudioDecompressor();
		return (int)FALSE;
	}

	if( InitCTab(&CTabAudio[0],ANC,LCODE_BITS) == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		FreeAudioDecompressor();
		return (int)FALSE;
	}
	if( InitCTab(&CTabAudio[1],ANC,LCODE_BITS) == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		FreeAudioDecompressor();
		return (int)FALSE;
	}

	return (int)TRUE;
}

void InitAudioDelta(AUDIO_DELTA_STRUCT *AudioStruct,UWORD32 uStartDelta,UWORD32 uUpdateFreq)
{
	memset(AudioStruct,0,sizeof(AUDIO_DELTA_STRUCT));
	AudioStruct->uDelta = uStartDelta;
	AudioStruct->uUpdateFreq = uUpdateFreq;
}

UBYTE DecodeAudioDelta(AUDIO_DELTA_STRUCT *AudioStruct,UBYTE cDeltaByte)
{
UBYTE		cError0;
UBYTE		cError1;
UBYTE		cError2;
UBYTE		cError3;
UBYTE		cError4;
UBYTE		cError5;
UBYTE		uCurByte;
UWORD32		uFreqs_Count[8];
UWORD32		uCount;
UWORD32		uSmallest;
UWORD32		uSmallestCount;


	switch(AudioStruct->uDelta)
	{
		case 0:
			uCurByte = cDeltaByte;
			break;
		case 1:
			uCurByte = AudioStruct->cSamps[0] - cDeltaByte;
			break;
		case 2:
			uCurByte = ((2 * AudioStruct->cSamps[0]) - AudioStruct->cSamps[1]) - cDeltaByte;
			break;
		case 3:
			uCurByte = ((3 * AudioStruct->cSamps[0]) - (3 * AudioStruct->cSamps[1]) + AudioStruct->cSamps[2]) - cDeltaByte;
			break;
		case 4:
			uCurByte = ((4 * AudioStruct->cSamps[0] - 3 * AudioStruct->cSamps[1] + AudioStruct->cSamps[2]) >> 1) - cDeltaByte;
			break;
		case 5:
			uCurByte = ((4 * AudioStruct->cSamps[0]) + (6 * AudioStruct->cSamps[1]) - (4 * AudioStruct->cSamps[2]) + AudioStruct->cSamps[3]) - cDeltaByte;
			break;
	}
	
	cError0 = uCurByte;
	cError1 = AudioStruct->cSamps[0] - uCurByte;
	cError2 = ((2 * AudioStruct->cSamps[0]) - AudioStruct->cSamps[1]) - uCurByte;
	cError3 = ((3 * AudioStruct->cSamps[0]) - (3 * AudioStruct->cSamps[1]) + AudioStruct->cSamps[2]) - uCurByte;
	cError4 = ((4 * AudioStruct->cSamps[0] - 3 * AudioStruct->cSamps[1] + AudioStruct->cSamps[2]) >> 1) - uCurByte;
	cError5 = ((4 * AudioStruct->cSamps[0]) + (6 * AudioStruct->cSamps[1]) - (4 * AudioStruct->cSamps[2]) + AudioStruct->cSamps[3]) - uCurByte;

	AudioStruct->uFreqs[0][cError0]++;
	AudioStruct->uFreqs[1][cError1]++;
	AudioStruct->uFreqs[2][cError2]++;
	AudioStruct->uFreqs[3][cError3]++;
	AudioStruct->uFreqs[4][cError4]++;
	AudioStruct->uFreqs[5][cError5]++;

	AudioStruct->cSamps[3] = AudioStruct->cSamps[2];
	AudioStruct->cSamps[2] = AudioStruct->cSamps[1];
	AudioStruct->cSamps[1] = AudioStruct->cSamps[0];
	AudioStruct->cSamps[0] = uCurByte;

	if( (AudioStruct->uCurrCount & AudioStruct->uUpdateFreq) == AudioStruct->uUpdateFreq )
	{
		memset(uFreqs_Count,0,sizeof(uFreqs_Count));

		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[0][uCount] > 0 )
				uFreqs_Count[0]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[1][uCount] > 0 )
				uFreqs_Count[1]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[2][uCount] > 0 )
				uFreqs_Count[2]++;
		}
			
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[3][uCount] > 0 )
				uFreqs_Count[3]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[4][uCount] > 0 )
				uFreqs_Count[4]++;
		}
				
		for( uCount = 0; uCount < 256; uCount++ )
		{
			if( AudioStruct->uFreqs[5][uCount] > 0 )
				uFreqs_Count[5]++;
		}

		uSmallest = uFreqs_Count[0];
		uSmallestCount = 0;

		for( uCount = 0; uCount < 6; uCount++ )
		{
			if( uFreqs_Count[uCount] < uSmallest )
			{
				uSmallest = uFreqs_Count[uCount];
				uSmallestCount = uCount;
			}
		}
		AudioStruct->uDelta = uSmallestCount;
		memset(AudioStruct,0,sizeof(AUDIO_DELTA_STRUCT) - sizeof(AUDIO_DELTA_SUB_STRUCT));
	}

	AudioStruct->uCurrCount++;

	return uCurByte;
}

void ReadData(ER_STRUCT *ErStruct,UWORD32 uFSize,INT32 *pOutDataPtr)
{
UWORD32		uCount;
INT32		iShift;
INT32		iRes;
INT32		iCount;
INT32		iSign;

	ErStruct->u32PredT = NEEDBITS(PRED_BITS);
	DUMPBITS(PRED_BITS);
	
	iShift = NEEDBITS(SHIFT_BITS);
	DUMPBITS(SHIFT_BITS);

	for( uCount = 0; uCount < uFSize; uCount++ )
	{
		iSign = NEEDBITS(1);
		DUMPBITS(1);
	
		iCount = 0;
		iRes = NEEDBITS(1);
		
		while( iRes == 0 )
		{
			iCount++;
			DUMPBITS(1);
			iRes = (long)NEEDBITS(1);
		}
		DUMPBITS(1);
		
		if( iShift > 0 )
		{
			iRes = NEEDBITS(iShift);
			DUMPBITS(iShift);
		}
		else
			iRes = 0;

		if( iCount > 0 )
			iRes |= iCount << iShift;
		if( iSign == 1 )
			iRes = -iRes;
		pOutDataPtr[uCount] = iRes;
	}
}

void FlushSamples(void)
{
	if( lSPageIndex > 0 )
	{
		os_write(usSamples,(SLONG32)(lSPageIndex * 2),ArcStruct.outfile);
		IoStruct.u32FileCRC = crc32(IoStruct.u32FileCRC,(UBYTE *)usSamples,(UWORD32)(lSPageIndex * 2));
		ProgressStruct.l64Processed += lSPageIndex * 2;
		SetProgress(0);
		lSPageIndex = 0;
	}
}

void SplitChannels(INT32 *pLData,INT32 *pRData,SLONG32 lICD,SLONG32 lSize)
{
SLONG32		lCount;
INT32		iLeft;
INT32		iRight;
	
	if( lSPageIndex + 2 * lSize > SAMP_PAGE_MAX / 2 )
		FlushSamples();
	
	lCount = 0;
	while( lCount < lSize )
	{
		iLeft = pLData[lCount];
		iRight = pRData[lCount++];
		
		switch(lICD)
		{
			case 1:
				iRight -= (iLeft >> 1);
				iLeft += iRight;
				break;
			case 2:
				iRight -= (iLeft >> 1);
				iLeft += (iRight >> 1);
				break;
		}
		usSamples[lSPageIndex++] = (INT16)iLeft;
		usSamples[lSPageIndex++] = (INT16)iRight;
	}
}

void MoveChannels(INT32 *pLData,SLONG32 lSize)
{
SLONG32		lCount;
INT32		iLeft;
	
	if( lSPageIndex + 2 * lSize > SAMP_PAGE_MAX / 2 )
		FlushSamples();
	
	lCount = 0;
	while( lCount < lSize )
	{
		iLeft = pLData[lCount++];
		usSamples[lSPageIndex++] = (INT16)iLeft;
	}
}

void DeSample(ER_STRUCT *ErStruct,INT32 *pDataPtr,UWORD32 u32FSize,INT32 *pOutDataPtr)
{
UWORD32		u32PredT;
UWORD32		u32BSizeBase;
INT32		iSample;
INT32		iLE0;
INT32		iLE1;
INT32		iLE2;
INT32		iLE4;
INT32		iLE0T;
INT32		iLE1T;
INT32		iLE1TT;
INT32		iLE2T;
INT32		iLE2TT;
INT32		iLE4T;
INT32		iLE4TT;
INT32		iBase;
INT32		iDiff;


	iLE0T = iLE1T = iLE1TT = iLE2T = iLE2TT = iLE4T = iLE4TT = 0;

	u32PredT = ErStruct->u32PredT;
	for( u32BSizeBase = 0; u32BSizeBase < u32FSize; u32BSizeBase++ )
	{
		iBase = pDataPtr[u32BSizeBase];
		switch(u32PredT)
		{
			case 0:
				iLE0 = iBase;
				iDiff = 0;
				if( iLE0T < 0 )
				{
					iDiff = abs(iLE0T) >> 4;
					iDiff = -iDiff;
				}
				else if( iLE0T > 0 )
					iDiff = abs(iLE0T) >> 4;
				iLE0 += iDiff;
				iLE0T = iBase;
				iSample = iLE0 + ErStruct->iStartV;
				break;
			case 1:
				iLE1 = iBase;
				iDiff = 0;
				if( iLE1T < 0 )
				{
					iDiff = (abs(iLE1T) >> 1);
					if( iLE1TT < 0 )
						iDiff += (iDiff >> 1);
					iDiff = -iDiff;
				}
				else if( iLE1T > 0 )
				{
					iDiff = (abs(iLE1T) >> 1);
					if( iLE1TT > 0 )
						iDiff += (iDiff >> 1);
				}
				iLE1 += iDiff;
				iSample = iLE1 + ErStruct->LV[0];
				iLE1TT = iLE1T;
				iLE1T = iBase;
				break;
			case 2:
				iLE2 = iBase;
				iDiff = 0;
				if( iLE2T < 0 )
				{
					iDiff = (abs(iLE2T) >> 1);

					if( iLE2TT < 0 )
						iDiff += (iDiff >> 1);
					iDiff = -iDiff;
				}
				else if( iLE2T > 0 )
				{
					iDiff = (abs(iLE2T) >> 1);
					if( iLE2TT > 0 )
						iDiff += (iDiff >> 1);
				}
				iLE2 += iDiff;
				iSample = iLE2 + ((2 * ErStruct->LV[0]) - ErStruct->LV[1]);
				iLE2TT = iLE2T;
				iLE2T = iBase;
				break;
			case 3:
				iSample = iBase + ((3 * ErStruct->LV[0]) - (3 * ErStruct->LV[1]) + ErStruct->LV[2]);
				break;
			case 4:
				iLE4  = iBase;
				iDiff = 0;
				if( iLE4T < 0 )
				{
					iDiff = (abs(iLE4T) >> 1);
					if( iLE4TT < 0 )
						iDiff += (iDiff >> 1);
					iDiff = -iDiff;
				}
				else if( iLE4T > 0 )
				{
					iDiff = (abs(iLE4T) >> 1);
					if( iLE4TT > 0 )
						iDiff += (iDiff >> 1);
				}
		
				iLE4 += iDiff;
				iSample = iLE4 + ((4 * ErStruct->LV[0] - 3 * ErStruct->LV[1] + ErStruct->LV[2]) >> 1);
				iLE4TT = iLE4T;
				iLE4T = iBase;
				break;
		}

		ErStruct->LV[3] = ErStruct->LV[2];
		ErStruct->LV[2] = ErStruct->LV[1];
		ErStruct->LV[1] = ErStruct->LV[0];
		ErStruct->LV[0] = ErStruct->iStartV = iSample;
		pOutDataPtr[u32BSizeBase] = iSample;
	}
}

void Flush8BitSamples(void)
{
	if( lSPageIndex > 0 )
	{
		os_write(usSamples,(SLONG32)lSPageIndex,ArcStruct.outfile);
		IoStruct.u32FileCRC = crc32(IoStruct.u32FileCRC,(UBYTE *)usSamples,(UWORD32)lSPageIndex);
		ProgressStruct.l64Processed += lSPageIndex;
		SetProgress(0);
		lSPageIndex = 0;
	}
}

void UnompressData1x8(void)
{
AUDIO_DELTA_STRUCT	AudioStruct;
UBYTE				*pData8;
UBYTE				cDiff;
UWORD32				uCount;

	
	InitAudioDelta(&AudioStruct,0,1023);
	uBTBlockPtr = 0;

	pData8 = (UBYTE *)usSamples;
	lSPageIndex = 0;

	while( ArcStruct.ArchiveStatus == SQX_EC_OK )
	{
		if( AudioHead.uRawDataSize == 0 )
			break;

		if( uBTBlockPtr == 0 )
		{
			uBTBlockPtr = NEEDBITS(MAX_8CODE_BITS);
			DUMPBITS(MAX_8CODE_BITS);

			for( uCount = 0; uCount < BitCTab.uNumCode; uCount++ )
			{
				BitCTab.uLens[uCount] = NEEDBITS(4);
				DUMPBITS(4);
			}
	
			if( GenerateUCTab(&BitCTab) == (int)FALSE )
			{
				ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
				break;
			}

			if( ReadOneTabTree(&CTabAudio[0]) == (int)FALSE )
			{
				ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
				break;
			}
		}

		cDiff = (UBYTE)PEEKCODE(CTabAudio[0]);
		DUMPBITS(CTabAudio[0].uLens[cDiff]);
		uBTBlockPtr--;

		pData8[lSPageIndex++] = DecodeAudioDelta(&AudioStruct,cDiff);
		AudioHead.uRawDataSize--;

		if( lSPageIndex >= SAMP_PAGE_MAX )
			Flush8BitSamples();
	}
	Flush8BitSamples();
	SetProgress(1);
}

void UnompressData2x8(void)
{
AUDIO_DELTA_STRUCT	AudioStruct[2];
UBYTE				*pData8;
UBYTE				cDiff;
UWORD32				uTwoTrees;
UWORD32				uCount;
UWORD32				CIndex;
	
	uBTBlockPtr = CIndex = 0;

	pData8 = (UBYTE *)usSamples;
	lSPageIndex = 0;
	uTwoTrees = OPTION_NONE;
	
	InitAudioDelta(&AudioStruct[0],0,511);
	InitAudioDelta(&AudioStruct[1],0,511);

	while( ArcStruct.ArchiveStatus == SQX_EC_OK )
	{
		if( AudioHead.uRawDataSize == 0 )
			break;

		if( uBTBlockPtr == 0 )
		{
			uBTBlockPtr = NEEDBITS(MAX_8CODE_BITS);
			DUMPBITS(MAX_8CODE_BITS);

			uTwoTrees = NEEDBITS(1);
			DUMPBITS(1);

			for( uCount = 0; uCount < BitCTab.uNumCode; uCount++ )
			{
				BitCTab.uLens[uCount] = NEEDBITS(4);
				DUMPBITS(4);
			}
	
			if( GenerateUCTab(&BitCTab) == (int)FALSE )
			{
				ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
				break;
			}

			if( ReadOneTabTree(&CTabAudio[0]) == (int)FALSE )
			{
				ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
				break;
			}

			if( uTwoTrees == OPTION_SET )
			{
				if( ReadOneTabTree(&CTabAudio[1]) == (int)FALSE )
				{
					ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
					break;
				}
			}
			CIndex = 0;
		}

		cDiff = (UBYTE)PEEKCODE(CTabAudio[0]);
		DUMPBITS(CTabAudio[0].uLens[cDiff]);
		uBTBlockPtr--;
		pData8[lSPageIndex++] = DecodeAudioDelta(&AudioStruct[CIndex],cDiff);
		if( uTwoTrees == OPTION_NONE )
			CIndex ^= 1;
		
		AudioHead.uRawDataSize--;
		if( lSPageIndex >= SAMP_PAGE_MAX )
			Flush8BitSamples();
		
		if( uTwoTrees == OPTION_SET )
		{
			cDiff = (UBYTE)PEEKCODE(CTabAudio[1]);
			DUMPBITS(CTabAudio[1].uLens[cDiff]);
			pData8[lSPageIndex++] = DecodeAudioDelta(&AudioStruct[1],cDiff);
			
			AudioHead.uRawDataSize--;
			if( lSPageIndex >= SAMP_PAGE_MAX )
				Flush8BitSamples();
		}
	}
	Flush8BitSamples();
	SetProgress(1);
}

void UnompressData1x16(void)
{
SLONG32		lHead;	
UWORD32		uSize;
ER_STRUCT	ErStructL;

	memset(&ErStructL,0,sizeof(ER_STRUCT));
	lSPageIndex = 0;

	while( ArcStruct.ArchiveStatus == SQX_EC_OK )
	{
		if( AudioHead.uRawDataSize < 4 )
			break;

		lHead = NEEDBITS(HEAD_T_BITS);
		DUMPBITS(HEAD_T_BITS);
        //- !!
		DUMPBITS(CHANNEL_DEC_BITS);

		if( lHead == FRAME_TYPE_LAST )
		{
			uSize = NEEDBITS(MAX_FRAME_BITS);
			DUMPBITS(MAX_FRAME_BITS);
		}
		else
			uSize = SAMP_FRAME_MAX;

		AudioHead.uRawDataSize -= (uSize << 1);

		ReadData(&ErStructL,uSize,pLeftOut[0]);
		DeSample(&ErStructL,pLeftOut[0],uSize,pLeftOut[1]);
		MoveChannels(pLeftOut[1],(SLONG32)uSize);
	}

	FlushSamples();
	SetProgress(1);
}

void UnompressData2x16(void)
{
SLONG32		lHead;	
SLONG32		lICD;	
UWORD32		uSize;	
ER_STRUCT	ErStructL;
ER_STRUCT	ErStructR;
	
	memset(&ErStructL,0,sizeof(ER_STRUCT));
	memset(&ErStructR,0,sizeof(ER_STRUCT));
	lSPageIndex = 0;
	
	while( ArcStruct.ArchiveStatus == SQX_EC_OK )
	{
		if( AudioHead.uRawDataSize < 4 )
			break;

		lHead = NEEDBITS(HEAD_T_BITS);
		DUMPBITS(HEAD_T_BITS);
		lICD = NEEDBITS(CHANNEL_DEC_BITS);
		DUMPBITS(CHANNEL_DEC_BITS);

		if( lHead == FRAME_TYPE_LAST )
		{
			uSize = NEEDBITS(MAX_FRAME_BITS);
			DUMPBITS(MAX_FRAME_BITS);
		}
		else
			uSize = SAMP_FRAME_MAX;

		AudioHead.uRawDataSize -= (uSize << 2);

		ReadData(&ErStructL,uSize,pLeftOut[0]);
		ReadData(&ErStructR,uSize,pRightOut[0]);
		DeSample(&ErStructL,pLeftOut[0],uSize,pLeftOut[1]);
		DeSample(&ErStructR,pRightOut[0],uSize,pRightOut[1]);
		SplitChannels(pLeftOut[1],pRightOut[1],lICD,uSize);
	}

	FlushSamples();
	SetProgress(1);
}

void GetExtraData(SLONG32 lSize)
{
SLONG32		lCount;
SLONG32		lToWrite;
UBYTE		*pData;
UBYTE		cData;

	pData = (UBYTE *)usSamples;
	
	while( lSize > 0 )
	{
		if( lSize > SAMP_PAGE_MAX )
			lToWrite = SAMP_PAGE_MAX;
		else
			lToWrite = lSize;

		for( lCount = 0; lCount < lToWrite; lCount++ )
		{
			cData = NEEDBITS(8);
			DUMPBITS(8)
			pData[lCount] = cData;
		}

		os_write(usSamples,lToWrite,ArcStruct.outfile);
		IoStruct.u32FileCRC = crc32(IoStruct.u32FileCRC,(UBYTE *)usSamples,(UWORD32)lToWrite);
		lSize -= lToWrite;
	}
}

void UncompressAudioFile(void)
{
SLONG32		lCount;
UBYTE		*pData;
UBYTE		cData;
	
	InitBitStream();

	pData = (UBYTE *)&AudioHead;
	for( lCount = 0; lCount < (SLONG32)sizeof(SQX_AUDIO_HEAD); lCount++ )
	{
		cData = NEEDBITS(8);
		DUMPBITS(8)
		pData[lCount] = cData;
	}
	
	IoStruct.u32FileCRC = crc32(IoStruct.u32FileCRC,(UBYTE *)&AudioHead,(UWORD32)sizeof(SQX_AUDIO_HEAD));
	
	GetExtraData(AudioHead.lSkipSize);
	
	if( AudioHead.cChannelBits == 16 )
	{
		if( AudioHead.cChannels == 2 )
			UnompressData2x16();
		else
			UnompressData1x16();
	}
	else
	{
		if( AudioHead.cChannels == 2 )
			UnompressData2x8();
		else
			UnompressData1x8();
	}

	GetExtraData(AudioHead.lFileTail);
}
