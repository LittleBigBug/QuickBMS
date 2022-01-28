/*********************************************************
SQX_VARS.H


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

#if !defined(__SQX_VARS_H)   //- include this file only once per compilation
#define __SQX_VARS_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#if !defined(__SQX_H_H) 
#include "sqx_h.h"
#endif

extern UBYTE		*pCommBuf;
extern ARC_STRUCT	ArcStruct;
extern IO_STRUCT	IoStruct;;
extern DELTA_CTRL	DeltaCtrl;
extern PROG_STRUCT	ProgressStruct;

#endif //- #if !defined(__SQX_VARS_H) 
