/**************************************************************************
WIN32_INC.C

Based on R_OS.C

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

SLONG64 os_seek(OS_FILE OsFile,SLONG64 l64Pos,long Mode)
{
#if defined(__SQX_LARGE_FILE_SUPPORT__) 
LARGE_INTEGER li64Temp;

	//- helper...
	li64Temp.QuadPart = (LONGLONG)l64Pos;
    
	//- set filepointer
	li64Temp.LowPart = SetFilePointer((HANDLE)OsFile,li64Temp.LowPart,&li64Temp.HighPart,(DWORD)Mode);
	//- check for errors
    if( li64Temp.LowPart == 0xFFFFFFFF )
    {
		if( GetLastError() != ERROR_SUCCESS )
			return -1;
    }
    return (SLONG64)li64Temp.QuadPart;

#else
SLONG64	l64Temp;

	l64Temp = SetFilePointer((HANDLE)OsFile,l64Pos,NULL,(DWORD)Mode);
	if( GetLastError() != ERROR_SUCCESS )
		return -1;
    return l64Temp;
#endif
}

SLONG64 os_filelength(OS_FILE OsFile)
{
#if defined(__SQX_LARGE_FILE_SUPPORT__) 
LARGE_INTEGER li64Temp;
	
	//- helper...
	li64Temp.QuadPart = 0;
	//- get filesize
	li64Temp.LowPart = GetFileSize((HANDLE)OsFile,(LPDWORD)&li64Temp.HighPart);
	//- check for errors
	if( li64Temp.LowPart == 0xFFFFFFFF )
    {
		if( GetLastError() != ERROR_SUCCESS )
			return -1;
    }
    
    return (SLONG64)li64Temp.QuadPart;
#else
SLONG64	l64Temp;

	l64Temp = GetFileSize((HANDLE)OsFile,NULL);
	if( GetLastError() != ERROR_SUCCESS )
		return -1;
    return l64Temp;
#endif
}

SLONG64 os_tell(OS_FILE OsFile)
{
#if defined(__SQX_LARGE_FILE_SUPPORT__) 
LARGE_INTEGER li64Temp;
	
	li64Temp.QuadPart = 0;
	li64Temp.LowPart = SetFilePointer((HANDLE)OsFile,0,&li64Temp.HighPart,FILE_CURRENT);
    if( li64Temp.LowPart == 0xFFFFFFFF )
    {
		if( GetLastError() != ERROR_SUCCESS )
			return (SLONG64)-1;
    }

    return (SLONG64)li64Temp.QuadPart;
#else
SLONG64	l64Temp;

	l64Temp = SetFilePointer((HANDLE)OsFile,0,NULL,FILE_CURRENT);
	if( GetLastError() != ERROR_SUCCESS )
		return -1;
    return l64Temp;
#endif
}

SLONG32 os_read(void *pvoid,SLONG32 l32Count,OS_FILE OsFile)
{
DWORD   dwRead;

	if( ReadFile((HANDLE)OsFile,pvoid,(DWORD)l32Count,&dwRead,NULL) == FALSE )
		return (SLONG32)-1;
	else
		return (SLONG32)dwRead;
}

SLONG32 os_write(void *pvoid,SLONG32 l32Count,OS_FILE OsFile)
{
DWORD   dwWritten;

	if( WriteFile((HANDLE)OsFile,pvoid,(DWORD)l32Count,&dwWritten,NULL) == FALSE )
		return (SLONG32)-1;
	else
		return (SLONG32)dwWritten;
}


OS_FILE os_open(char *szFname, char *Mode)
{
OS_FILE	OsFile;

	switch(Mode[0])
	{
		case 'w':
			OsFile = (OS_FILE)CreateFile(szFname,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
			break;

		case 'r':
			OsFile = (OS_FILE)CreateFile(szFname,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
			break;

		default:
			OsFile = (OS_FILE)OS_INVALID_FILE;
	}
	return OsFile;
}

SLONG32 os_close(OS_FILE OsFile)
{
	if( CloseHandle((HANDLE)OsFile) == TRUE )
		return (SLONG32)TRUE;
	else
		return (SLONG32)FALSE;
}

SLONG32 os_remove(char *szFileName)
{
	return (SLONG32)DeleteFile(szFileName);
}

UWORD32 os_setfileattr(char *szFileName,UWORD32 u32Attribute)
{
    if( SetFileAttributes((LPSTR)szFileName,(DWORD)u32Attribute) == FALSE)
    	return (UWORD32)GetLastError();
    else
    	return (UWORD32)0;
}

UWORD32 os_setdosftime(OS_FILE OsFile,UWORD32 u32Date,UWORD32 u32Time)
{
FILETIME ftFileTime;
FILETIME ftLocalFileTime;

    if( DosDateTimeToFileTime((WORD)u32Date, (WORD)u32Time,&ftLocalFileTime) != TRUE )
        return (UWORD32)GetLastError();

    LocalFileTimeToFileTime(&ftLocalFileTime,&ftFileTime);

    if( SetFileTime((HANDLE)OsFile,NULL,NULL,&ftFileTime) != TRUE )
        return (UWORD32)GetLastError();
    else
        return 0;
}

//- ###########################################################################################################

int os_current_directory(char *szCurrDir,int iBufLen)
{
DWORD	dwRes;
	dwRes = GetCurrentDirectory((DWORD)iBufLen,szCurrDir);
	if( (dwRes > (DWORD)iBufLen) || (dwRes == 0) )
		return (int)FALSE;
	else
		return (int)TRUE;
}

int os_create_dir(char *szName)
{
	if( CreateDirectory(szName,NULL) == FALSE )
		return (int)FALSE;
	return (int)TRUE;
}

int os_directory_exists(char *szName)
{
DWORD	Attr;

	Attr = GetFileAttributes(szName);
	if( ( Attr != (DWORD)-1 ) && ( (Attr & OS_ATTRIBUTE_DIRECTORY) != 0 ) )
		return (int)TRUE;
	else
		return (int)FALSE;
}

int os_file_exist(char *szName)
{
DWORD	Attr;

	Attr = GetFileAttributes(szName);
	if( ( Attr != (DWORD)-1 ) && ( (Attr & OS_ATTRIBUTE_DIRECTORY) == 0 ) )
		return (int)TRUE;
	else
		return (int)FALSE;
}

char *os_addslash(char *szName)
{
UWORD32	uLen;
char	*pTemp;
	
	if( (szName == NULL) || ((uLen = strlen(szName)) >= CX_MAXPATH) )
		return NULL;
	if( uLen > 0 )
	{
		pTemp = szName + uLen;
		if( pTemp[-1] != '\\' )
		{
			*pTemp++ = '\\';
			*pTemp = '\0';
		}
	}
	return szName;
}

char *os_removeslash(char *szName)
{
UWORD32	uLen;
	 
	if( szName != NULL )
	{
		uLen = strlen(szName);
		if( uLen > 3 )
			if( szName[uLen - 1] == OS_PATH_SEP )
				szName[uLen - 1] = '\0';
	}
	return szName;
}

char *os_find_file_ext(char *szName)
{
char	*szTemp;
int		iLen;
	
	if( szName == NULL )	
		return NULL;
	
	iLen = strlen(szName);
	szTemp = szName + iLen;
	szTemp--;

	while( iLen-- )
	{
		if( *szTemp == '.' )
			return szTemp;
		szTemp = CharPrevA(szName,szTemp);
	}

	if( *szTemp == '.' )
		return szTemp;
	return NULL;
}

char *os_get_file_ext(char *szName,char *szRet)
{
char	*szTemp;
UWORD32	uLen;
	
	if( (szRet == NULL) || (szName == NULL) || ((szTemp = os_find_file_ext(szName)) == NULL) )	
		return NULL;

	uLen = strlen(szTemp);
	
	if( uLen == 0 )
		szRet[0] = '\0';
	else
	{
		memmove(szRet,szTemp,uLen);
		szRet[uLen] = '\0';
	}
	
	return szRet;
}

int os_set_file_ext(char *szName,char *szExt)
{
char	*szTemp;
	
	if( szName == NULL )	
		return (int)FALSE;

	if( (szTemp = os_find_file_ext(szName)) == NULL )	
	{
		if( (szExt != NULL) && (szExt[0] != '\0') )
			strcat(szName,szExt);
	}
	else
	{
		if( (szExt == NULL) || (szExt[0] == '\0') )
			*szTemp = '\0';
		else
			strcpy(szTemp,szExt);
	}
	
	return (int)TRUE;
}

char *os_find_file_name(char *szName)
{
char	*szTemp;
int		iLen;
	
	if( szName == NULL )	
		return NULL;

	iLen = strlen(szName);
	szTemp = szName + iLen;
	szTemp--;

	while( iLen-- )
	{
		if( *szTemp == OS_PATH_SEP )
			return szTemp + 1;
		szTemp = CharPrevA(szName,szTemp);
	}

	szTemp = strchr(szName,':');
	if( szTemp != NULL )
		return szTemp + 1;
	else
		return szName;
}

char *os_get_file_name(char *szName,char *szRet)
{
char	*szTemp;
UWORD32	uLen;
	
	if( (szRet == NULL) || (szName == NULL) || ((szTemp = os_find_file_name(szName)) == NULL) )	
		return NULL;

	uLen = strlen(szTemp);
	
	if( uLen == 0 )
		szRet[0] = '\0';
	else
	{
		memmove(szRet,szTemp,uLen);
		szRet[uLen] = '\0';
	}
	
	return szRet;
}

char *os_get_dir_name(char *szName,char *szRet)
{
char	*szTemp;
UWORD32	uLen;	

	if( (szRet == NULL) || (szName == NULL) )
		return NULL;

	szTemp = os_find_file_name(szName);
	if( szTemp == szName )
		szRet[0] = '\0';
	else
	{
		uLen = szTemp - szName;
		memmove(szRet,szName,uLen);
		szRet[uLen] = '\0';
	}

	return szRet;
}

int os_find_slash(char *szPath,int iSlashNum)
{
int	iCount;
int	iStrLen;
int iSlash;

	iStrLen = strlen(szPath);
	iCount = iSlash = 0;
	for(; iCount < iStrLen; iCount++ )
		if( szPath[iCount] == OS_PATH_SEP )
			if( ++iSlash == iSlashNum )
				// changed 11/18/1999 R.N.
				return iCount;

	return iStrLen;
}

int os_create_full_path(char *szPath)
{
int		iSlashNum;
int		iCount;
char	szTempPath[CX_MAXPATH];

	if( os_directory_exists(szPath) == (int)TRUE )
		return (int)TRUE;
	
	if( strstr(szPath,OS_UNC) != NULL )
		iSlashNum = 5;
	else
		iSlashNum = 2;

	memset(szTempPath,0,sizeof(szTempPath));

	do
	{
		iCount = os_find_slash(szPath,iSlashNum);
		strncpy(szTempPath,szPath,iCount);
		if( os_directory_exists(szTempPath) == (int)FALSE )
			if( os_create_dir(szTempPath) == (int)FALSE )
				return (int)FALSE;
		iSlashNum++;
	}while( strlen(szTempPath) < strlen(szPath) );
	//- un wech...
	return (int)TRUE;
}

int os_create_full_file_path(char *szName)
{
char szTempPath[CX_MAXPATH];

	strcpy(szTempPath,szName);
	os_removeslash(szTempPath);
	return os_create_full_path(szTempPath);
}

int os_create_ouput_file(OS_FILE *OsFile, char *szName)
{
char      szDir[CX_MAXPATH];

	os_get_dir_name(szName,szDir);
	if( os_create_full_file_path(szDir) == (int)FALSE )
		return (int)FALSE;
	
	if( os_file_exist(szName) == (int)TRUE )
		os_setfileattr(szName,FILE_ATTRIBUTE_ARCHIVE);
	
	*OsFile = os_open(szName,"wb");

    if( *OsFile == OS_INVALID_FILE )
		return (int)FALSE;
	return (int)TRUE;	
}
