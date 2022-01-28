/*********************************************************
UARDE.H


C/C++ version Copyright (c) 1996 - 2002
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

#if !defined(__UARDE_H)   //- include this file only once per compilation
#define __UARDE_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#define MIN_LEN2			0x000000FF
#define MIN_LEN3			0x00003FFF
#define MIN_LEN4			0x0003FFFF
#define	MIN_MAX_DIC_SIZE	0x400000

/*extern*/ static UWORD32	CUR_MAX_DIC_SIZE;
/*extern*/ static UWORD32	uCurrPos;			
/*extern*/ static UWORD32	uWritePos;			
/*extern*/ static UBYTE	*SWindow;
/*extern*/ static UBYTE	*pImageBuf;

static int InitDecodeLz(void);
static void FreeDecodeLz(void);
static void DecodeFile(long lCompFlags,long lSolidFlag,UWORD32 uDecDicSize);

#endif //- #if !defined(__UARDE_H)   

