/****************************************************************************
CONFIG.H

C/C++ version Copyright (c) 1999 - 2002
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

#if !defined(__CONFIG_H)   //- include this file only once per compilation
#define __CONFIG_H

//- uncomment the next define if you want to compile
//- unsqx for Win32
#define __SQX_OS_WIN32__


//- uncomment the next define if you want to compile
//- unsqx for DOS.
//#define __SQX_OS_SIMPLE_32BIT_DOS__


//- uncomment the next define if you want to compile
//- unsqx for Linux
//- #define __SQX_OS_LINUX__


//- uncomment the next define if you want to make
//- the decompressor use single code tables instead 
//- of flat code tables
#define __SQX_SINGLE_CTAB__

//- ##################################################################################

//- _MSC_VER		MS VC compiler
//- _BCB50_VER		Borland C++ Builder 5.x
//- _BC50_VER		Borland C 5.x

#if defined(__SQX_OS_WIN32__) 

	#if defined(_MSC_VER) || (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0550)) || (defined(__WATCOMC__) && (__WATCOMC__ >= 1100))
		#define __SQX_LARGE_FILE_SUPPORT__
	#endif

#endif

#endif //- #if !defined(__CONFIG_H)
