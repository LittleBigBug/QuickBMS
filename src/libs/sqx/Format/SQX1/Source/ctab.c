/*********************************************************
CTAB.C


C/C++ version Copyright (c) 1999 - 2002
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

*********************************************************/

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#if !defined(__CX_ERR_H)   
#include "cx_err.h"
#endif

#if !defined(__CTAB_H)   
#include "ctab.h"
#endif

#if !defined(__MM_CODER_H)   
#include "mm_coder.h"
#endif

#if !defined(__T_CODER_H)
#include "t_coder.h"
#endif

#if !defined(__ARCIO_H) 
#include "arcio.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif


#define LZ_BLOCK_TYPE		0
#define MM_BLOCK_TYPE		1

#define DELTA_8_BITS		0
#define DELTA_16_BITS		1

#define START_DELTA_BITS	3
#define DELTA_NUM_BITS		2
#define CODER_NUM_BITS		2

#define HUFF_LEN			0x4
#define HUFF_CTRL			0x1

#define CODE_TREE_TABS		0x0
#define CODE_FLAT_TABS		0x1

#define CODE_THREE_TREES	0x0
#define CODE_FOUR_TREES		0x1

static UWORD32 uLenCount[16 + 1];
static UWORD16 usNextCode[16 + 2];
static UWORD16 usCCount[16 + 1];

static UWORD32 Len2Offsets[LEN2_CODES]				= {0,4,8,16,32,64,128,192};
static UWORD32 Len2EBits[LEN2_CODES]				= {2,2,3, 4, 5, 6,  6,  6};

static UWORD32 Len3Offsets[LEN3_CODES]				= {0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192};
static UWORD32 Len3EBits[LEN3_CODES]				= {0,0,1,2,3, 4, 5, 6,  7, 8,   9,   10,  11, 12,  13};

static UWORD32 LenOffsets[LEN_CODES]				= {0,1,2,3,4,5,6,7,8,10,12,16,20,24,32,40,48,64,80,96,128,160,192,224,0};
static UWORD32 LenEBits[LEN_CODES]					= {0,0,0,0,0,0,0,0,1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,  5,  5,  5,  5,0};

static UWORD32 BaseDTable_1M[DIST_CODES_1M + 1]	= {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152,65536,98304,131072,196608,262144,327680,393216,458752,524288,589824,655360,720896,786432,851968,917504,983040,1048576};
static UWORD32 BaseDBits_1M[DIST_CODES_1M]			= {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,   14,    14,  15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16};

static UWORD32 BaseDTable_2M[DIST_CODES_2M + 1]	= {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152,65536,98304,131072,196608,262144,393216,524288,655360,786432,917504,1048576,1179648,1310720,1441792,1572864,1703936,1835008,1966080,2097152};
static UWORD32 BaseDBits_2M[DIST_CODES_2M]			= {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,   14,   14,   15,   15,    16,    16,    17,    17,    17,    17,    17,    17,     17,     17,     17,     17,     17,     17,     17,     17};

static UWORD32 BaseDTable_4M[DIST_CODES_4M + 1]	= {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152,65536,98304,131072,196608,262144,393216,524288,786432,1048576,1310720,1572864,1835008,2097152,2359296,2621440,2883584,3145728,3407872,3670016,3932160,4194304};
static UWORD32 BaseDBits_4M[DIST_CODES_4M]			= {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,   14,   14,   15,   15,    16,    16,    17,    17,    18,    18,     18,     18,     18,     18,     18,     18,     18,     18,     18,     18,     18,     18};

static UWORD32 *BaseDTable;
static UWORD32 *BaseDBits;
static UWORD32 CUR_DIST_CODES;

static CTAB_STRUCT	BitCTab;
static CTAB_STRUCT LitLenCTab[2];
static CTAB_STRUCT RepCTab;
static CTAB_STRUCT DistCTab;

static UWORD32 uLastLen = 0;
static UWORD32 uLastDist = 0;

static UWORD32  PrevDists[4] = {0,0,0,0};
static UWORD32  uPrevDistIndex = 0;


static 
void FreeCTab(CTAB_STRUCT *CTab)
{
	if( CTab->uLens != NULL )
	{
		free(CTab->uLens);
		CTab->uLens = NULL;
	}

	if( CTab->uCodes != NULL )
	{
		free(CTab->uCodes);
		CTab->uCodes = NULL;
	}
}

static 
int InitCTab(CTAB_STRUCT *CTab,UWORD32 uMaxCode,UWORD32 uMBCode)
{
	memset(CTab,0,sizeof(CTAB_STRUCT));

	CTab->uMaxTabCode = 1 << uMBCode;
	CTab->uMBCode = uMBCode;
	CTab->uNumCode = uMaxCode;

#if defined(__SQX_SINGLE_CTAB__)   
	CTab->uLens = (UBYTE *)malloc(uMaxCode * sizeof(UBYTE));
	CTab->uCodes = (CODE_ENTRY *)malloc((uMaxCode + 1) * sizeof(CODE_ENTRY));
#else
	CTab->uLens = (UWORD32 *)malloc(uMaxCode * sizeof(UWORD32));
	CTab->uCodes = (UWORD32 *)malloc((size_t) (1U << uMBCode) * sizeof(UWORD32));
#endif
	if( ( CTab->uLens == NULL ) || ( CTab->uCodes == NULL ) )
	{
		FreeCTab(CTab);
		return (int)FALSE;
	}
	return (int)TRUE;
}


#if defined(__SQX_SINGLE_CTAB__)   

static 
void CodeOrder(CODE_ENTRY *CodeEntries,UWORD32 uNumCodes)
{
UWORD32	uCount;
UWORD32	uCount2;
UWORD32	uCountL;
UWORD32	uCountR;
UWORD32	uNewCode;

	while( uNumCodes > 1 )
	{
		uNewCode = CodeEntries[0].uFltCode;
		CodeEntries[0].uFltCode = CodeEntries[uNumCodes / 2].uFltCode;
		CodeEntries[uNumCodes / 2].uFltCode = uNewCode;

		for( uCount = 0, uCount2 = uNumCodes; ; )
		{
			do
			{
				--uCount2;
			}
			while( CodeEntries[uCount2].sc.usCode > CodeEntries[0].sc.usCode );
			
			do
			{
				++uCount;
			}
			while( uCount < uCount2 && CodeEntries[uCount].sc.usCode < CodeEntries[0].sc.usCode );
			
			if( uCount >= uCount2 )
				break;
			
			uNewCode = CodeEntries[uCount].uFltCode;
			CodeEntries[uCount].uFltCode = CodeEntries[uCount2].uFltCode;
			CodeEntries[uCount2].uFltCode = uNewCode;
		}
		
		uNewCode = CodeEntries[uCount2].uFltCode;
		CodeEntries[uCount2].uFltCode = CodeEntries[0].uFltCode;
		CodeEntries[0].uFltCode = uNewCode;
		
		uCountL = uCount2;
		uCountR = uNumCodes - ++uCount2;
            
		if( uCountL < uCountR )
		{
			CodeOrder(CodeEntries,uCountL);
			CodeEntries += uCount2;
			uNumCodes = uCountR;
		}
		else
		{
			CodeOrder(CodeEntries + uCount2,uCountR);
			uNumCodes = uCountL;
		}
	}
}

static 
int GenerateUCTab(CTAB_STRUCT *CTab)
{
UWORD32		uNext;
UWORD32		uCount;
UWORD32		uCurrCode;
UWORD32		uBLen;

	memset(&uLenCount,0,sizeof(uLenCount));
	for( uCount = 0; uCount < CTab->uNumCode; uCount++ )
		uLenCount[CTab->uLens[uCount]]++;

	usNextCode[1] = 0;
	for( uCount = 1; uCount <= CTab->uMBCode; uCount++ )
		usNextCode[uCount + 1] = usNextCode[uCount] + (uLenCount[uCount] << (CTab->uMBCode - uCount));

	if( usNextCode[CTab->uMBCode + 1] != (UWORD16)(1U << CTab->uMBCode) )
	{
		ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
		return (int)FALSE;
	}
	
	for( uCount = 1; uCount <= CTab->uMBCode; uCount++ )
		usCCount[uCount] = 1 << (CTab->uMBCode - uCount);

	for( uCurrCode = 0; uCurrCode < CTab->uNumCode; uCurrCode++ )
	{
		CTab->uCodes[uCurrCode].sc.usSym = uCurrCode;

		uBLen = CTab->uLens[uCurrCode];
		if( uBLen == 0 )
		{
			CTab->uCodes[uCurrCode].sc.usCode = 0xFFFF;
			continue;
		}
		
		uNext = usNextCode[uBLen] + usCCount[uBLen];

		if( uNext > CTab->uMaxTabCode )
		{
			ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
			return (int)FALSE;
		}

		CTab->uCodes[uCurrCode].sc.usCode = usNextCode[uBLen];
		usNextCode[uBLen] = uNext;
	}
	
	CTab->uCodes[CTab->uNumCode].sc.usCode = 0xFFFF;
	CodeOrder(CTab->uCodes,CTab->uNumCode);
	return (int)TRUE;
}

static 
UWORD32 peekcode(CTAB_STRUCT *CTab,UWORD16 uBitCode)
{
UWORD32	uCode;
UWORD32	uMaxCode;
UWORD32	uTempCode;
UWORD32	uIndex;
UWORD32	uNumCodes;

	uCode = 0;
	uMaxCode = CTab->uNumCode - 1;
	uNumCodes = CTab->uNumCode;
	while( uCode <= uMaxCode )
	{
		if( uIndex = uNumCodes / 2 )
		{
			uTempCode = uCode + (uNumCodes & 1 ? uIndex : (uIndex - 1));

			if( (CTab->uCodes[uTempCode].sc.usCode <= uBitCode) && (CTab->uCodes[uTempCode + 1].sc.usCode > uBitCode) )
				return CTab->uCodes[uTempCode].sc.usSym;
			
			if( CTab->uCodes[uTempCode].sc.usCode > uBitCode )
			{
				uMaxCode = uTempCode - 1;
				uNumCodes = uNumCodes & 1 ? uIndex : uIndex - 1;
			}
			else    
			{
				uCode = uTempCode + 1;
				uNumCodes = uIndex;
			}
		}
		else
		{
			if( (CTab->uCodes[uCode].sc.usCode <= uBitCode) && (CTab->uCodes[uCode + 1].sc.usCode > uBitCode) )
				return CTab->uCodes[uCode].sc.usSym;
		}
	}
	return 0xFFFF;
}

#else

static 
int GenerateUCTab(CTAB_STRUCT *CTab)
{
UWORD32		uNext;
UWORD32		uCount;
UWORD32		uCurrCode;
UWORD32		uBLen;

	memset(&uLenCount,0,sizeof(uLenCount));
	for( uCount = 0; uCount < CTab->uNumCode; uCount++ )
		uLenCount[CTab->uLens[uCount]]++;

	usNextCode[1] = 0;
	for( uCount = 1; uCount <= CTab->uMBCode; uCount++ )
		usNextCode[uCount + 1] = usNextCode[uCount] + (uLenCount[uCount] << (CTab->uMBCode - uCount));

	if( usNextCode[CTab->uMBCode + 1] != (UWORD16)(1U << CTab->uMBCode) )
	{
		ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
		return (int)FALSE;
	}
	
	memset(CTab->uCodes,0,sizeof(UWORD32) * (1U << CTab->uMBCode));

	for( uCount = 1; uCount <= CTab->uMBCode; uCount++ )
		usCCount[uCount] = 1 << (CTab->uMBCode - uCount);

	for( uCurrCode = 0; uCurrCode < CTab->uNumCode; uCurrCode++ )
	{
		uBLen = CTab->uLens[uCurrCode];
		if( uBLen == 0 )
			continue;
		
		uNext = usNextCode[uBLen] + usCCount[uBLen];

		if( uNext > CTab->uMaxTabCode )
		{
			ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
			return (int)FALSE;
		}

		for( uCount = usNextCode[uBLen]; uCount < uNext; uCount++ )
			CTab->uCodes[uCount] = uCurrCode;

		usNextCode[uBLen] = uNext;
	}
	return (int)TRUE;
}

#endif

static 
void FreeDecodeTabs(void)
{
	FreeCTab(&BitCTab);
	FreeCTab(&LitLenCTab[0]);
	FreeCTab(&LitLenCTab[1]);
	FreeCTab(&RepCTab);
	FreeCTab(&DistCTab);
	FreeMMCoders();
}

static 
int AllocateDecodeTabs(void)
{
	if( CreateMMCoders() == (int)FALSE )
		return (int)FALSE;

	if( InitCTab(&BitCTab,BCODES,BCODE_BITS) == (int)FALSE )
		return (int)FALSE;
	
	if( InitCTab(&LitLenCTab[0],NC,LCODE_BITS) == (int)FALSE )
	{
		FreeDecodeTabs();
		return (int)FALSE;
	}
	
	if( InitCTab(&LitLenCTab[1],NC,LCODE_BITS) == (int)FALSE )
	{
		FreeDecodeTabs();
		return (int)FALSE;
	}

	if( InitCTab(&RepCTab,LEN_CODES,LCODE_BITS) == (int)FALSE )
	{
		FreeDecodeTabs();
		return (int)FALSE;
	}
	
	if( InitCTab(&DistCTab,DIST_CODES,LCODE_BITS) == (int)FALSE )
	{
		FreeDecodeTabs();
		return (int)FALSE;
	}

	return (int)TRUE;
}

static 
void SetBaseDistTabs(UWORD32 uCurDicSize)
{
	BaseDTable = &BaseDTable_1M[0];
	BaseDBits = &BaseDBits_1M[0];
	CUR_DIST_CODES = DIST_CODES_1M;

	switch(uCurDicSize)
	{
		case SQX_DIC_SIZE_2048K:
			BaseDTable = &BaseDTable_2M[0];
			BaseDBits = &BaseDBits_2M[0];
			CUR_DIST_CODES = DIST_CODES_2M;
			break;
		case SQX_DIC_SIZE_4096K:
			BaseDTable = &BaseDTable_4M[0];
			BaseDBits = &BaseDBits_4M[0];
			CUR_DIST_CODES = DIST_CODES_4M;
			break;
	}
}

static 
int ReadOneTabFlat(CTAB_STRUCT *CTabStruct)
{
UWORD32	uCount;
UWORD32	uCodeNum;
UWORD32	uCode;

	uCodeNum = NEEDBITS(HUFF_LEN);
	DUMPBITS(HUFF_LEN);
	CTabStruct->uLens[0] = uCodeNum;
	
	uCount = 1;
	
	while( uCount < CTabStruct->uNumCode )
	{
		
		uCode = NEEDBITS(HUFF_CTRL);
		DUMPBITS(HUFF_CTRL);
		if( uCode == OPTION_NONE )
		{
			CTabStruct->uLens[uCount++] = uCodeNum;
			continue;
		}

		uCode = NEEDBITS(HUFF_CTRL);
		DUMPBITS(HUFF_CTRL);
		if( uCode == OPTION_NONE )
		{
			CTabStruct->uLens[uCount++] = ++uCodeNum;
			continue;
		}

		uCode = NEEDBITS(HUFF_CTRL);
		DUMPBITS(HUFF_CTRL);
		if( uCode == OPTION_SET )
		{
			uCodeNum = NEEDBITS(HUFF_LEN);
			DUMPBITS(HUFF_LEN);
			CTabStruct->uLens[uCount++] = uCodeNum;
			continue;
		}

		uCode = NEEDBITS(HUFF_CTRL);
		DUMPBITS(HUFF_CTRL);
		if( uCode == OPTION_NONE )
			CTabStruct->uLens[uCount++] = --uCodeNum;
		else
			CTabStruct->uLens[uCount++] = (uCodeNum += 2);

	}
	return GenerateUCTab(CTabStruct);
}

static 
int ReadOneTabTree(CTAB_STRUCT *CTabStruct)
{
UWORD32	uCount;
UWORD32	uRepCount;
UWORD32	uCodeNum;

	uCount = 0;
	
	while( uCount < CTabStruct->uNumCode )
	{
		uCodeNum = PEEKCODE(BitCTab);
		DUMPBITS(BitCTab.uLens[uCodeNum]);

		if( uCodeNum < 16 )
		{
			CTabStruct->uLens[uCount] = uCodeNum;
			uCount++;
		}
		else
		{
			if( uCodeNum == 16 )
			{
				uRepCount = 3 + NEEDBITS(2);
				DUMPBITS(2);
				while( ( uCount < CTabStruct->uNumCode ) && ( uRepCount-- > 0 ) )
				{
					CTabStruct->uLens[uCount] = CTabStruct->uLens[uCount - 1];
					uCount++;
				}
			}
			else
			{
				if( uCodeNum == 17 )
				{
					uRepCount = 3 + NEEDBITS(3);
					DUMPBITS(3);
				}
				else
				{
					uRepCount = 11 + NEEDBITS(7);
					DUMPBITS(7);
				}
				while( ( uCount < CTabStruct->uNumCode ) && ( uRepCount-- > 0 ) )
				{
					CTabStruct->uLens[uCount] = 0;
					uCount++;
				}
			}
		}
	}
	return GenerateUCTab(CTabStruct);
}

static 
int ReadTabs(UWORD32 *uMaxTreeCode,UWORD32 *uFourTrees,UBYTE *pMMBufPtr,UWORD32 *uMMBlock)
{
UWORD32		uCount;
UWORD32		uCodeType;
UWORD32		uBlockType;
UWORD32		uDeltaBase;
UWORD32		uDeltaStart;
UWORD32		uNumDeltaCoders;
UWORD32		uNumEntropyCoders;

	uBlockType = NEEDBITS(1);
	DUMPBITS(1);
	
	if( uBlockType == LZ_BLOCK_TYPE )
	{
		*uMMBlock = OPTION_NONE;
		uBTBlockPtr = NEEDBITS(15);
		DUMPBITS(15);

		*uFourTrees = NEEDBITS(1);
		DUMPBITS(1);

		if( *uFourTrees != CODE_FOUR_TREES )
		{
			uCodeType = NEEDBITS(1);
			DUMPBITS(1);

			if( uCodeType == CODE_TREE_TABS )
			{
				for( uCount = 0; uCount < BitCTab.uNumCode; uCount++ )
				{
					BitCTab.uLens[uCount] = NEEDBITS(4);
					DUMPBITS(4);
				}
	
				if( GenerateUCTab(&BitCTab) == (int)FALSE )
					return (int)FALSE;

				if( ReadOneTabTree(&LitLenCTab[0]) == (int)FALSE )
					return (int)FALSE;
	
				if( ReadOneTabTree(&RepCTab) == (int)FALSE )
					return (int)FALSE;

				if( ReadOneTabTree(&DistCTab) == (int)FALSE )
					return (int)FALSE;
			}
			else
			{
				if( ReadOneTabFlat(&LitLenCTab[0]) == (int)FALSE )
					return (int)FALSE;
	
				if( ReadOneTabFlat(&RepCTab) == (int)FALSE )
					return (int)FALSE;

				if( ReadOneTabFlat(&DistCTab) == (int)FALSE )
					return (int)FALSE;
			}
		}
		else
		{
			*uMaxTreeCode = NEEDBITS(9);
			DUMPBITS(9);

			for( uCount = 0; uCount < BitCTab.uNumCode; uCount++ )
			{
				BitCTab.uLens[uCount] = NEEDBITS(4);
				DUMPBITS(4);
			}

			if( GenerateUCTab(&BitCTab) == (int)FALSE )
				return (int)FALSE;

			if( ReadOneTabTree(&LitLenCTab[0]) == (int)FALSE )
				return (int)FALSE;
	
			if( ReadOneTabTree(&LitLenCTab[1]) == (int)FALSE )
				return (int)FALSE;

			if( ReadOneTabFlat(&RepCTab) == (int)FALSE )
				return (int)FALSE;

			if( ReadOneTabFlat(&DistCTab) == (int)FALSE )
				return (int)FALSE;

		}
	}
	else
	{
		*uMMBlock = OPTION_SET;

		uDeltaBase = NEEDBITS(1);
		DUMPBITS(1);
		
		uDeltaStart = NEEDBITS(START_DELTA_BITS);
		DUMPBITS(START_DELTA_BITS);

		uNumDeltaCoders = NEEDBITS(DELTA_NUM_BITS);
		DUMPBITS(DELTA_NUM_BITS);
		uNumDeltaCoders++;

		uNumEntropyCoders = NEEDBITS(CODER_NUM_BITS);
		DUMPBITS(CODER_NUM_BITS);
		uNumEntropyCoders++;

		uBTBlockPtr = NEEDBITS(15);
		DUMPBITS(15);

		DecodeRcData(uNumEntropyCoders,pMMBufPtr,uBTBlockPtr);

		if( ArcStruct.ArchiveStatus != SQX_EC_OK )
			return (int)FALSE;
		
		DeconvRType(uDeltaStart,uDeltaBase,uNumDeltaCoders,pMMBufPtr,uBTBlockPtr);
	}
	return (int)TRUE;
}
