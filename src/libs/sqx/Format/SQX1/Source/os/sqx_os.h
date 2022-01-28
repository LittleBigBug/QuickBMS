/**************************************************************************
SQX_OS.H

Based on R_OS.H

C/C++ version Copyright (c) 1995 - 2001
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

#if !defined(__SQX_OS_H)   //- include this file only once per compilation
#define __SQX_OS_H

#if !defined(__CX_TYPE_H)
#include "´../cx_type.h"
#endif

static SLONG64 os_seek(OS_FILE OsFile,SLONG64 l64Pos,long Mode);
static SLONG64 os_filelength(OS_FILE OsFile);
static SLONG64 os_tell(OS_FILE OsFile);
static SLONG32 os_read(void *pvoid,SLONG32 l32Count,OS_FILE OsFile);
static SLONG32 os_write(void *pvoid,SLONG32 l32Count,OS_FILE OsFile);
static OS_FILE os_open(char *szFname, char *Mode);
static SLONG32 os_close(OS_FILE OsFile);
static UWORD32 os_setfileattr(char *szFileName,UWORD32 u32Attribute);
static UWORD32 os_getfileattr(char *szFileName,UWORD32 *u32Attribute);
static UWORD32 os_setdosftime(OS_FILE OsFile,UWORD32 u32Date,UWORD32 u32Time);
static SLONG32 os_remove(char *szFileName);

//- ###########################################################################################################

static int os_current_directory(char *szCurrDir,int iBufLen);
static int os_create_dir(char *szName);
static int os_directory_exists(char *szName);
static int os_file_exist(char *szName);
static char *os_addslash(char *szName);
static char *os_removeslash(char *szName);
static char *os_find_file_ext(char *szName);
static char *os_get_file_ext(char *szName,char *szRet);
static int os_set_file_ext(char *szName,char *szExt);
static char *os_find_file_name(char *szName);
static char *os_get_file_name(char *szName,char *szRet);
static char *os_get_dir_name(char *szName,char *szRet);
static int os_create_full_file_path(char *szName);
static int os_create_ouput_file(OS_FILE *OsFile, char *szName);

#endif //- #if !defined(__SQX_OS_H)   

