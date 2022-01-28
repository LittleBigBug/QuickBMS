/*************************************************************************
RARCIO.H


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

*****************************************************************************/


#if !defined(__ARCIO_H)   //- include this file only once per compilation
#define __ARCIO_H

#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

#define INBUF_DIFF			0x00000008
#define INBUF_DIFF_BLOCK	0x00000020
#define INBUF_SHIFT			5
#define INBUF_BIT_SIZE		32
#define INBUF_MASK			INBUF_BIT_SIZE - 1
#define INBUF_SIZE			0x00001000 * sizeof(UWORD32)
#define INBUF_SIZE_MAX		0x00000FF8 * sizeof(UWORD32)
#define INBUF_POS_MAX		0x00000FF8

/*extern*/ static UWORD32 uBitBuf;				//- bits in bitbuf
/*extern*/ static UWORD32 uBitBufBits;			//- # of bits valid in bitbuf
/*extern*/ static UWORD32 uBitBufPos;			//- pos in bitbuf
/*extern*/ static UWORD32 *ualBitStreamBuf;	//- bit buffer
/*extern*/ static UWORD32 uBTBlockPtr;			//- # of compressed signals

static int InitBitExtraction(void);
static void DoneBitExtraction(void);
static void WriteOutBuf(UBYTE *pBuf,UWORD32 uCount);
static SLONG32 ReadDataBlock(UBYTE *pBuf,UWORD32 uCount);
static SLONG32 InitBitStream(void);
static SLONG32 ReadBitStream(void);
static void WriteDictionary(void);

#define NEEDBITS(NumBits)	(uBitBuf >> (INBUF_BIT_SIZE - NumBits))	
#define DUMPBITS(NumBits)																\
{																						\
	uBitBufBits += NumBits;																\
	uBitBufPos += uBitBufBits >> INBUF_SHIFT;											\
	uBitBufBits &= INBUF_MASK;															\
	if( uBitBufPos == INBUF_POS_MAX )													\
		ReadBitStream();																\
	uBitBuf = (ualBitStreamBuf[uBitBufPos] << uBitBufBits);								\
	if( uBitBufBits )																	\
		uBitBuf |= (ualBitStreamBuf[uBitBufPos + 1] >> (INBUF_BIT_SIZE - uBitBufBits));	\
}

#endif //- #if !defined(__ARCIO_H)   

