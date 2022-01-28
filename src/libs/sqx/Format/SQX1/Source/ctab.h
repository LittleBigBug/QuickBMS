/*********************************************************
CTAB.H


C/C++ version Copyright (c) 1995 - 2002
Rainer Nausedat
Bahnhofstrasse 32
26316 Varel

All rights reserved.


build code tables

CTAB.H is a part of the SQX Archiver

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


#if !defined(__CTAB_H)   //- include this file only once per compilation
#define __CTAB_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#if defined(__SQX_SINGLE_CTAB__)   

//- NC * 5 + 12

typedef union 
{ 
	struct
	{
		UWORD16	usSym;
		UWORD16	usCode;
	 }sc;
    UWORD32	uFltCode;
}CODE_ENTRY; 

typedef struct
{
	UWORD32		uNumCode;
	UWORD32		uMaxTabCode;
	UWORD32		uMBCode;
	UBYTE		*uLens;
	CODE_ENTRY	*uCodes;
}CTAB_STRUCT;

#else

//- ((1 << MAX_BIT_CODE) * sizeof(UWORD32)) + (NC * sizeof(UWORD32)) + 12

typedef struct
{
	UWORD32		uNumCode;
	UWORD32		uMaxTabCode;
	UWORD32		uMBCode;
	UWORD32		*uLens;
	UWORD32		*uCodes;
}CTAB_STRUCT;

#endif

#define BCODE_BITS		7
#define BCODES			19
#define LCODE_BITS		15

#define	LIT_CODES		256
#define	DL_CODES		1
#define	REP_CODES		4
#define LEN2_CODES		8
#define LEN3_CODES		15
#define	LEN_CODES		24 + 1

#define DIST_CODES_1M	48
#define DIST_CODES_2M	50
#define DIST_CODES_4M	52

#define	DIST_CODES		56

#define DL_START		(LIT_CODES)
#define REP_START		(LIT_CODES + DL_CODES)
#define LEN2_START		(LIT_CODES + DL_CODES + REP_CODES)
#define LEN3_START		(LIT_CODES + DL_CODES + REP_CODES + LEN2_CODES)
#define LEN_START		(LIT_CODES + DL_CODES + REP_CODES + LEN2_CODES + LEN3_CODES)
#define NC				(LIT_CODES + DL_CODES + REP_CODES + LEN2_CODES + LEN3_CODES + LEN_CODES) + 1

#define UPDATEDIST(MatchPos) PrevDists[uPrevDistIndex++ & 3] = MatchPos;

#if defined(__SQX_SINGLE_CTAB__)   

static
UWORD32 peekcode(CTAB_STRUCT *CTab,UWORD16 uBitCode);
#define PEEKCODE(CTab) peekcode(&CTab,(UWORD16)(uBitBuf >> (INBUF_BIT_SIZE - CTab.uMBCode)));

#else

#define PEEKCODE(CTab) (CTab.uCodes[uBitBuf >> (INBUF_BIT_SIZE - CTab.uMBCode)])

#endif

/*extern*/ static UWORD32 Len2Offsets[LEN2_CODES];
/*extern*/ static UWORD32 Len2EBits[LEN2_CODES];
/*extern*/ static UWORD32 Len3Offsets[LEN3_CODES];
/*extern*/ static UWORD32 Len3EBits[LEN3_CODES];
/*extern*/ static UWORD32 LenOffsets[LEN_CODES];
/*extern*/ static UWORD32 LenEBits[LEN_CODES];

/*extern*/ static UWORD32 *BaseDTable;
/*extern*/ static UWORD32 *BaseDBits;
/*extern*/ static UWORD32 CUR_DIST_CODES;

/*extern*/ static CTAB_STRUCT BitCTab;
/*extern*/ static CTAB_STRUCT LitLenCTab[2];
/*extern*/ static CTAB_STRUCT RepCTab;
/*extern*/ static CTAB_STRUCT DistCTab;

/*extern*/ static UWORD32 uLastLen;
/*extern*/ static UWORD32 uLastDist;
/*extern*/ static UWORD32 PrevDists[4];
/*extern*/ static UWORD32 uPrevDistIndex;

static int InitCTab(CTAB_STRUCT *CTabStruct,UWORD32 uMaxCode,UWORD32 uMBCode);
static void FreeCTab(CTAB_STRUCT *CTabStruct);
static int GenerateUCTab(CTAB_STRUCT *CTabStruct);
static void FreeDecodeTabs(void);
static int AllocateDecodeTabs(void);
static void SetBaseDistTabs(UWORD32 uCurDicSize);
static int ReadOneTabTree(CTAB_STRUCT *CTabStruct);
static int ReadTabs(UWORD32 *uMaxTreeCode,UWORD32 *uFourTrees,UBYTE *pMMBufPtr,UWORD32 *uMMBlock);

#endif //- #if !defined(__CTAB_H)   
