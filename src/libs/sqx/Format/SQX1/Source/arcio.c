/***************************************************************************
ARCIO.C  


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

****************************************************************************/


#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

#if !defined(__CX_ERR_H)   
#include "cx_err.h"
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

#if !defined(__UARDE_H)   
#include "uarde.h"
#endif

#if !defined(__T_CODER_H)
#include "t_coder.h"
#endif

#if !defined(__AES_C_H)   
//#include "crypt/aes/aes_c.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

#if defined(_MSC_VER)
	#pragma warning(disable : 4146)
#endif

//- in sqx_dec.c
/*extern*/ static int OpenNextVolume(void);

static UWORD32 uBitBuf;			//- bits in bitbuf
static UWORD32 uBitBufBits;		//- # of bits valid in bitbuf
static UWORD32 uBitBufPos;			//- pos in bitbuf
static UWORD32 *ualBitStreamBuf;	//- bit buffer
static UWORD32 uBTBlockPtr;		//- # of compressed signals

static
int InitBitExtraction(void)
{
	ualBitStreamBuf = malloc(INBUF_SIZE);
	if( ualBitStreamBuf == NULL )
		return (int)FALSE;
	return (int)TRUE;
}

static
void DoneBitExtraction(void)
{
	free(ualBitStreamBuf);
}

static
SLONG32 ReadDataBlock(UBYTE *pBuf,UWORD32 uCount)
{
SLONG32 lRet;
SLONG32 lRead;
UBYTE	*pTemBuf;
	
	pTemBuf = pBuf;
	lRet = 0;
	while( uCount > 0 )
	{
		if( (SLONG64)uCount < IoStruct.l64CompSize )
			lRead = (SLONG32)uCount;
		else
			lRead = (SLONG32)IoStruct.l64CompSize;

		lRead = os_read(pTemBuf,lRead,ArcStruct.arcfile);
		IoStruct.l64CompSize -= lRead;
		uCount -= lRead;
		pTemBuf += lRead;
		lRet += lRead;

		if( ( ArcStruct.iFileWasSplit == OPTION_SET ) && ( IoStruct.l64CompSize == 0 ) )
		{
			if( OpenNextVolume() == (int)FALSE )
				return -1;
		}
		else
			break;
	}

	if( ArcStruct.iEncryption != SQX_CRYPT_NONE )
	{
		switch(ArcStruct.iEncryption)
		{
			case SQX_CRYPT_RIJNDAEL_128BIT:
				//RijndaelDecrypt128(pBuf,(UWORD32)lRet);
				break;
			default:;
		}
	}

	return lRet;
}

static
SLONG32 InitBitStream(void)
{
SLONG32 lRes;
	lRes = ReadDataBlock((UBYTE *)&ualBitStreamBuf[0],INBUF_SIZE);
	uBitBufPos = 0;
	uBitBufBits = 0;
	uBitBuf = ualBitStreamBuf[0];
	return lRes;
}

static
SLONG32 ReadBitStream(void)
{
SLONG32 lRes;
	memmove(&ualBitStreamBuf[0],&ualBitStreamBuf[INBUF_POS_MAX],INBUF_DIFF_BLOCK);
	lRes = ReadDataBlock((UBYTE *)&ualBitStreamBuf[INBUF_DIFF],INBUF_SIZE_MAX);
	uBitBufPos -= INBUF_POS_MAX;
	return lRes;
}

static
void WriteOutBuf(UBYTE *pBuf,UWORD32 uCount)
{
SLONG32 l32Ret;
SLONG32	lTemp;
SLONG32	lIndex;

	memmove(pImageBuf,pBuf,uCount);

	if( (DeltaCtrl.uCodeDeltas == OPTION_SET) && (uCount > 0) )
	{
		if( DeltaCtrl.lNeedOfs == OPTION_SET )
		{
			if( DeltaCtrl.lOffsetSoFar + (SLONG32)uCount > DeltaCtrl.lOffset )		
			{
				DeltaCtrl.lNeedOfs = OPTION_NONE;
				InitConvR_LZ_Type();
				lTemp = DeltaCtrl.lOffsetSoFar + (SLONG32)uCount - DeltaCtrl.lOffset;
				lIndex = (SLONG32)uCount - lTemp;
				DeconvR_LZ_Type(&pImageBuf[lIndex],uCount - (UWORD32)lIndex);
			}
			else
				DeltaCtrl.lOffsetSoFar += (SLONG32)uCount;
		}
		else
			DeconvR_LZ_Type(pImageBuf,uCount);
	}

	if( (IoStruct.usExtCompressor & SQX_IA32IMAGE_TYPE_FLAG) != OPTION_NONE )
		ReImage(pImageBuf,uCount);

	if( ArcStruct.iIgnoreWrite == OPTION_SET )
		l32Ret = (SLONG32)uCount;
	else
		l32Ret = os_write(pImageBuf,(SLONG32)uCount,ArcStruct.outfile);
	
	if( l32Ret != (SLONG32)uCount )
	{
		ArcStruct.ArchiveStatus = SQX_EC_DISK_FULL;
		return;
	}

	if( uCount > 0 )
		IoStruct.u32FileCRC = crc32(IoStruct.u32FileCRC,pImageBuf,uCount);
}

static
void WriteDictionary(void)
{

	if( IoStruct.l64ToFlush < 1 )
		return;
	
	if( (uCurrPos == CUR_MAX_DIC_SIZE) && (uWritePos == CUR_MAX_DIC_SIZE) )
	{
		WriteOutBuf(SWindow,uCurrPos);
		uWritePos = 0;
		IoStruct.l64ToFlush -= CUR_MAX_DIC_SIZE;
	}
	else if( uCurrPos < uWritePos )
	{
		WriteOutBuf(&SWindow[uWritePos],-uWritePos & (CUR_MAX_DIC_SIZE - 1));
		IoStruct.l64ToFlush -= (-uWritePos & (CUR_MAX_DIC_SIZE - 1));
		WriteOutBuf(SWindow,uCurrPos);
		IoStruct.l64ToFlush -= uCurrPos;
		uWritePos = uCurrPos;
	}
	else
	{
		WriteOutBuf(&SWindow[uWritePos],uCurrPos - uWritePos);
		IoStruct.l64ToFlush -= (uCurrPos - uWritePos);
		uWritePos = uCurrPos;
	}
}

#if defined(_MSC_VER)
	#pragma warning(default : 4146)
#endif
