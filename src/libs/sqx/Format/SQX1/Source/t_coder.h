/***************************************************************************
T_CODER.H


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

#if !defined(__T_CODER_H)
#define __T_CODER_H

#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

static void DeconvRType(UWORD32 uStartDeltaCode,UWORD32 uCoderType,UWORD32 uCoderNum,UBYTE *pBuf,UWORD32 uSize);
static void InitConvR_LZ_Type(void);
static void DeconvR_LZ_Type(UBYTE *pBuf,UWORD32 uSize);
static void ReImage(UBYTE *pBuffer,UWORD32 uBuflen);

#endif //- #if !defined(__T_CODER_H)
