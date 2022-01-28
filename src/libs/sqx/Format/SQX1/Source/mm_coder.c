/*********************************************************
MM_CODER.C


C/C++ version Copyright (c) 2002
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

#if !defined(__ARCIO_H)   
#include "arcio.h"
#endif

#if !defined(__RCODER_H)   
#include "rcoder.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

#define		MM_NC		256
#define		ARI_INC		8
#define		MM_ORDER_0_ADAPT	(1 << 12)

static RANGE_CODER	RangeCoder;
static RC_MODEL	MM_Coder[4];

static 
void FreeMMCoders(void)
{
	RC_ModelFree(&MM_Coder[0]);
	RC_ModelFree(&MM_Coder[1]);
	RC_ModelFree(&MM_Coder[2]);
	RC_ModelFree(&MM_Coder[3]);
}

static 
int CreateMMCoders(void)
{
	memset(&MM_Coder,0,sizeof(&MM_Coder));
	
	if( RC_ModelCreate(&MM_Coder[0],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1,NULL,NULL) != RC_OK )
	{
		FreeMMCoders();
		return (int)FALSE;
	}

	if( RC_ModelCreate(&MM_Coder[1],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1,NULL,NULL) != RC_OK )
	{
		FreeMMCoders();
		return (int)FALSE;
	}
	
	if( RC_ModelCreate(&MM_Coder[2],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1,NULL,NULL) != RC_OK )
	{
		FreeMMCoders();
		return (int)FALSE;
	}
	
	if( RC_ModelCreate(&MM_Coder[3],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1,NULL,NULL) != RC_OK )
	{
		FreeMMCoders();
		return (int)FALSE;
	}
	
	return (int)TRUE;
}

static 
void StartMMDecoders(void)
{
	RC_StartDecode(&RangeCoder);
}

static 
void ResetMMCoders(void)
{
	RC_ModelInit(&MM_Coder[0],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1);
	RC_ModelInit(&MM_Coder[1],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1);
	RC_ModelInit(&MM_Coder[2],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1);
	RC_ModelInit(&MM_Coder[3],MM_NC,NULL,ARI_INC,MM_ORDER_0_ADAPT,1);
}

static 
void DecodeRcData(UWORD32 uNumCoder,UBYTE *pBuf,UWORD32 uSize)
{
UWORD32		uCount;
UWORD32		uCountMax;
UWORD32		uLeft;
UWORD32		uCIndex;

	uCountMax = uSize;
	uCountMax /= uNumCoder;
	uCountMax *= uNumCoder;
	uCount = uCIndex = 0;
	uLeft = uSize - uCountMax;

	ResetMMCoders();
	StartMMDecoders();
	
	while( (uCount < uCountMax) && (ArcStruct.ArchiveStatus == SQX_EC_OK) )
	{
		pBuf[uCount++] = (UBYTE)RC_DecodeSymbol(&RangeCoder,&MM_Coder[uCIndex++]);
		if( uCIndex == uNumCoder )
			uCIndex = 0;
	}
	
	if( uLeft > 0 )
	{
		for( uCountMax = 0; uCountMax < uLeft; uCountMax++ )
		{
			pBuf[uCount++] = (UBYTE)NEEDBITS(8);
			DUMPBITS(8);
		}
	}
}

