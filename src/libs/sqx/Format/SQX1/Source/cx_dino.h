/*********************************************************
CX_DINO.H

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

***********************************************************/

#if !defined(__CX_DINO_H)   //- include this file only once per compilation
#define __CX_DINO_H

#if !defined(__CONFIG_H)
#include "config.h"
#endif

#if defined(__SQX_OS_WIN32__)   

	#define STRICT

	#include <windows.h>
	#include <memory.h>
	#include <string.h>
	#include <malloc.h>
	#include <conio.h>
	#include <stdio.h>

#elif defined(__SQX_OS_SIMPLE_32BIT_DOS__)
	
	#include <memory.h>
	#include <string.h>
	#include <malloc.h>
	#include <conio.h>
	#include <stdio.h>
	#include <io.h>
	#include <dos.h>	
	#include <dir.h>
	#include <stdlib.h>

#endif //- #if defined(__SQX_OS_SIMPLE_32BIT_DOS__)   


#endif//#if !defined(__CX_DINO_H)
