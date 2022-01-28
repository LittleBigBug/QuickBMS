/****************************************************************************
CX_TYPE.H

C/C++ version Copyright (c) 1999 - 2002
Rainer Nausedat
Bahnhofstrasse 32
26316 Varel / Germany

All rights reserved.

CX_TYPE.H is a part of the CX library

common defines and typedefs used/needed by the CX library and the
SQX archiver

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

#if !defined(__CX_TYPE_H)   //- include this file only once per compilation
#define __CX_TYPE_H

#if !defined(__CX_DINO_H)   
#include "cx_dino.h"
#endif

#if defined(__SQX_OS_WIN32__)   
	
	typedef unsigned char		UBYTE;		//- unsigned char,	1 byte
	typedef short int			INT16;		//- signed int,		2 bytes
	typedef int					INT32;		//- signed int,		4 bytes
	typedef unsigned short int	UWORD16;	//- unsigned,		2 bytes
	typedef unsigned			UWORD32;	//- unsigned,		4 bytes
	typedef long				SLONG32;	//- signed long,	4 bytes
#if defined(__SQX_LARGE_FILE_SUPPORT__) 
	typedef __int64				SLONG64;	//- signed long,	8 bytes
	typedef unsigned __int64	UWORD64;	//- unsigned long,	8 bytes
#else
	typedef long int			SLONG64;	//- faked SLONG64
	typedef unsigned long		UWORD64;	//- faked UWORD64
#endif
	
	typedef HANDLE OS_FILE;

	#if !defined(SEEK_SET)
		#define	SEEK_SET FILE_BEGIN
	#endif

	#if !defined(SEEK_CUR)
		#define	SEEK_CUR FILE_CURRENT
	#endif

	#if !defined(SEEK_END)
		#define	SEEK_END FILE_END
	#endif

	#define OS_INVALID_FILE INVALID_HANDLE_VALUE
	#define OS_ATTRIBUTE_DIRECTORY FILE_ATTRIBUTE_DIRECTORY
	#define OS_ATTRIBUTE_ARCHIVE FILE_ATTRIBUTE_ARCHIVE

	#define OS_UNC				"\\\\"
	#define OS_UNC_SEP			'\\'
	#define OS_PATH_SEP			'\\'
	#define OS_PATH_SEP_STR		"\\"
	#define OS_SLASH			'/'
	#define OS_ASTERIX			'*'
	#define OS_QMARK			'?'

	#define __SQX_SUPPORTED_OS_DEFINED__

#elif defined(__SQX_OS_SIMPLE_32BIT_DOS__)   
	
	typedef unsigned char		UBYTE;		//- unsigned char,	1 byte
	typedef short int			INT16;		//- signed int,		2 bytes
	typedef int					INT32;		//- signed int,		4 bytes
	typedef unsigned short int	UWORD16;	//- unsigned,		2 bytes
	typedef unsigned long		UWORD32;	//- unsigned,		4 bytes
	typedef long int			SLONG32;	//- signed long,	4 bytes
	typedef long int			SLONG64;	//- faked SLONG64
	typedef unsigned long		UWORD64;	//- faked UWORD64

	typedef FILE* OS_FILE;

	#define OS_INVALID_FILE NULL
	#define OS_ATTRIBUTE_DIRECTORY _A_SUBDIR
	#define OS_ATTRIBUTE_ARCHIVE _A_ARCH

	#define OS_UNC				"\\\\"
	#define OS_UNC_SEP			'\\'
	#define OS_PATH_SEP			'\\'
	#define OS_PATH_SEP_STR		"\\"
	#define OS_SLASH			'/'
	#define OS_ASTERIX			'*'
	#define OS_QMARK			'?'

	#define __SQX_SUPPORTED_OS_DEFINED__

#elif defined(__SQX_OS_LINUX__)   
	
	Error: Not yet supported. (hopefully to come soon :))

#endif   


#if !defined(__SQX_SUPPORTED_OS_DEFINED__)   
	
	Error: You did not define a target OS in config.h

#endif   


//- some  
#if !defined(TRUE)   
	#define TRUE 1
#endif

#if !defined(FALSE)   
	#define FALSE 0
#endif

#define OPTION_NONE			0
#define OPTION_SET			1

#define CX_MAXFNLEN	0xFF
#define CX_MAXPATH	(CX_MAXFNLEN + 16)

#endif //- #if !defined(__CX_TYPE_H)   
