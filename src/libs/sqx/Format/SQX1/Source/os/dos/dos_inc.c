/**************************************************************************
DOS_INC.C

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
	return (SLONG64)fseek(OsFile,l64Pos,Mode);
}

SLONG64 os_filelength(OS_FILE OsFile)
{
	return (SLONG64)filelength(fileno(OsFile));
}

SLONG64 os_tell(OS_FILE OsFile)
{
	return (SLONG64)ftell(OsFile);
}

SLONG32 os_read(void *pvoid,SLONG32 l32Count,OS_FILE OsFile)
{
	return (SLONG32)fread(pvoid,1,(size_t)l32Count,OsFile);
}

SLONG32 os_write(void *pvoid,SLONG32 l32Count,OS_FILE OsFile)
{
	return (SLONG32)fwrite(pvoid,1,(size_t)l32Count,OsFile);
}

OS_FILE os_open(char *szFname, char *Mode)
{
	return fopen(szFname,Mode);
}

SLONG32 os_close(OS_FILE OsFile)
{
	return (SLONG32)fclose(OsFile);
}

SLONG32 os_remove(char *szFileName)
{
	return (SLONG32)remove(szFileName);
}

UWORD32 os_setfileattr(char *szFileName,UWORD32 u32Attribute)
{
	 return (UWORD32)_dos_setfileattr(szFileName,(unsigned)u32Attribute);
}

UWORD32 os_setdosftime(OS_FILE OsFile,UWORD32 u32Date,UWORD32 u32Time)
{
	return (UWORD32)_dos_setftime(fileno(OsFile),(unsigned short)u32Date,(unsigned short)u32Time);
}

//- ###########################################################################################################

int os_current_directory(char *szCurrDir,int iBufLen)
{
	if( getcwd(szCurrDir,iBufLen) == NULL )
		return (int)FALSE;
	else
		return (int)TRUE;
}

int os_create_dir(char *szName)
{
	if( mkdir(szName) == 0 )
		return (int)TRUE;
	return (int)FALSE;
}

int os_directory_exists(char *szName)
{
unsigned uAttr;
unsigned uRes;

	uRes = _dos_getfileattr(szName,&uAttr);
	if( (uRes != 0) || ((uAttr & _A_SUBDIR ) == 0) )
		return (int)FALSE;
	return (int)TRUE;
}

int os_file_exist(char *szName)
{
unsigned uAttr;
unsigned uRes;

	uRes = _dos_getfileattr(szName,&uAttr);
	if( (uRes != 0) || ((uAttr & (_A_SUBDIR |_A_VOLID)) != 0) )
		return (int)FALSE;
	return (int)TRUE;
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
		szTemp--;
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
		if( (szExt == NULL) || (szExt[0] == '\0') )
			*szTemp = '\0';
		else
			strcat(szName,szExt);
	}
	else
		strcpy(szTemp,szExt);
	
	return (int)TRUE;
}

char *os_find_file_name(char *szName)
{
char	*szTemp;
	
	if( szName == NULL )	
		return NULL;

	szTemp = szName + strlen(szName);
	szTemp--;

	while( szTemp > szName )
	{
		if( *szTemp == OS_PATH_SEP )
			return szTemp + 1;
		szTemp--;
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
