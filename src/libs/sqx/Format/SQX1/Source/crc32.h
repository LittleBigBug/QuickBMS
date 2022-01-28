/****************************************************************************
CRC32.H  3.00

C/C++ version Copyright (c) 1995 - 2002
Rainer Nausedat
Bahnhofstrasse 32
26316 Varel / Germany

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

#if !defined(__CRC32_H)   //- include this file only once per compilation
#define __CRC32_H

#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

//extern UWORD32 CRC32Table[256];

static UWORD32 crc32(UWORD32 uCrc,UBYTE *pBuf,UWORD32 uLen);

#endif //- #if !defined(__CRC32_H)   

