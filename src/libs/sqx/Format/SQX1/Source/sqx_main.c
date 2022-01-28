/*********************************************************
SQX_MAIN.C


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

#if !defined(__CX_TYPE_H)
#include "cx_type.h"
#endif

#if !defined(__CX_ERR_H)
#include "cx_err.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

#if !defined(__SQX_OS_H)
#include "os/sqx_os.h"
#endif

extern int InitArchiver(void);
extern void DoneArchiver(void);
extern void ExtractFiles(void);

char *szCopyrightStr	= "\n\nunSQX sample. Copyright 1999 - 2002 R.Nausedat. All rights reserved.\n";
char *szHelpStr			= "Usage:  unsqx <command> archive [file,...]\n\nCommands\n\nE       Extract archive\nT       Test archive\nX       Extract archive (create directories)\n\nSample: unsqx x backup.sqx *.doc *.xls\n";
//-
char *szErrorNoError	= "\n\nunsqx: No error(s) detected.";
char *szErrorNoAccess	= "\n\nError: Access denied.";
char *szErrorMemory		= "\n\nError: Insufficient memory.";
char *szErrorDiskFull	= "\n\nError: Disk is full.";
char *szErrorArcCrypted	= "\n\nError: Archive is encrypted.";
char *szErrorBadCrc		= "\n\nError: Bad file crc.";
char *szErrorBadArc		= "\n\nError: Bad archive data.";
char *szErrorCommError	= "\n\nError code %lu (see cx_err.h).";
char *szUserBreak		= "\n\nAction canceled by user.";
//-
char *szGetHeadKey		= "\nThe directory of this archive is encrypted. Please enter\nthe correct password. Password : ";
char *szGetFileKey		= "\nThe archive is encrypted. Please enter the correct password.\nPassword : ";
char *szGetNextVolume	= "\n\nPlease insert disk %lu in drive %s or enter a new path.\nENTER = OK, ESC = Cancel\nPath: ";
char *szOwPermission	= "\nThe file %s already exists. Overwrite?\n(y = yes | n = no | c = cancel | a = always) : \n";
char *szProgrUnpack		= "unSqueezing %s, %lu%%\r";
char *szProgrTest		= "Testing %s, %lu%%\r";
//-
char *szFCtrl			= "YyNnCcAa";
char *szCCtrl			= "EeXxTt";

void cx_cputs(const char *string)
{
	//-
	fwrite(string,1,strlen(string),stdout);
}

void ErrorMessage(SLONG32 lError)
{
char	szMess[CX_MAXPATH];
char	*pError;

	pError = NULL;

	switch(lError)
	{
		case SQX_EC_OK:
			pError = szErrorNoError;
			break;
		case SQX_EC_ACCESS_DENIED:
			pError = szErrorNoAccess;
			break;
		case SQX_EC_OUT_OF_MEMORY:
			pError = szErrorMemory;
			break;
		case SQX_EC_DISK_FULL:
			pError = szErrorDiskFull;
			break;
		case SQX_EC_FILE_ENCRYPTED:
			pError = szErrorArcCrypted;
			break;
		case SQX_EC_BAD_FILE_CRC:
			pError = szErrorBadCrc;
			break;
		case SQX_EC_BAD_FILE_FORMAT:
			pError = szErrorBadArc;
			break;
		case SQX_EC_USER_ABORT:
			pError = szUserBreak;
			break;
	}

	if( pError == NULL )
	{
		sprintf(szMess,szErrorCommError,lError);
		pError = szMess;
	}

	cx_cputs(pError);
}

char *gets_ex(char *s,int mask_out)
{
int		c, len, maxlen;
char	*p = s + 2;

  len = 0;
  maxlen = s[ 0 ] & 0xff;

  	while( 1 )
  	{
    	switch( c = getch() )
      	{
      		case 0:
      			if( getch() != 75 )
      			break;

      		case '\b':
      			if( len )
            	{
                    putch('\b');
                    putch(' ');
                    putch('\b');
                    --len;
                    --p;
                }
                break;

            case 27:
                s[1] = 0;
                s[2] = 0;
				return (char *)( s + 2 );
				//- break;

			case '\r':
				*p = '\0';
				s[1] = len;
				return(char *)( s + 2 );
				//- break;

			default:
				if( len < maxlen - 1 )
				{
					if( mask_out == 0 )
						putch((char)c);
					else
					   putch('*');
					*p++ = c;
					++len;
				}
				break;
		}
	}
}

int creadstr(int iLen,char *szRet,int mask_out)
{
char szTemp[CX_MAXPATH];

	memset(szTemp,0,sizeof(szTemp));
	memset(szRet,0,(size_t)iLen);
	if( iLen == 0 )
		return (int)0;
	if( iLen > 80 )
		iLen = 80;
	szTemp[0] = (unsigned char)iLen;
	gets_ex((char *)&szTemp[0],mask_out);

	if( szTemp[1] > 0 )
	{
		strcpy(szRet,&szTemp[2]);
		return (int)szTemp[1];
	}
	else
		return (int)0;
}

int UserWantBreak(void)
{
int				iKStroke;

	if( kbhit() != (int)FALSE )
	{
		iKStroke = getch();
		if( iKStroke == 27 )
			return (int)TRUE;
		else
		{
			ungetch(iKStroke);
			return (int)FALSE;
		}
	}
	return (int)FALSE;
}

void SetProgress(int iFinished)
{
char	szMess[CX_MAXPATH];
SLONG32	lRatio;
SLONG64	lTempRatio;

	if( ProgressStruct.iNewProg == OPTION_SET )
	{
		ProgressStruct.iNewProg = OPTION_NONE;
		cx_cputs("\n");
	}

	lRatio = -1;
	if( iFinished == (int)TRUE )
		lRatio = 100;
	else if( ProgressStruct.l64Size > 0 )
	{
		lTempRatio = ProgressStruct.l64Processed * 100;
		lRatio = (SLONG32)(lTempRatio / ProgressStruct.l64Size);
		if( lRatio > 100 )
			lRatio = 100;
	}

	if( lRatio > -1 )
	{
		if( ArcStruct.iTestErrorsArc == OPTION_NONE )
			sprintf(szMess,szProgrUnpack,ProgressStruct.szProgName,lRatio);
		else
			sprintf(szMess,szProgrTest,ProgressStruct.szProgName,lRatio);
		cx_cputs(szMess);
	}

	if( UserWantBreak() == (int)TRUE )
		ArcStruct.ArchiveStatus = SQX_EC_USER_ABORT;
}

int GetNetVolPath(char *szVolPath,int iVolNum)
{
int				iKStroke;
char 			szNewDir[CX_MAXPATH];
char 			szMess[250];
                              
	sprintf(szMess,szGetNextVolume,iVolNum,szVolPath);
    cx_cputs(szMess);

	while( kbhit() == (int)FALSE );
	
	iKStroke = getch();

	if( iKStroke == 13 )
	{
		cx_cputs("\n\n");
		return (int)TRUE;
	}
	else if( iKStroke == 27 )
	{
		cx_cputs("\n");
		return (int)FALSE;
	}
	else
		ungetch(iKStroke);

	creadstr((int)60,szNewDir,(int)0);  
	
	cputs("\n\n");
	
	if(strlen(szNewDir) == 0 )
		return (int)TRUE;
   
   	strcpy(szVolPath,szNewDir);
	
	return (int)TRUE;
}

int GetCryptKey(char *szCKey,char *FileName,int iReason)
{
char	szNewKey[CX_MAXPATH];

	if( iReason == 0 )
		cx_cputs(szGetFileKey);
	else
		cx_cputs(szGetHeadKey);
	szNewKey[0] = '\0';
	creadstr(60,szNewKey,(int)1);
	cx_cputs("\n\n");
	if( strlen(szNewKey) > 0 )
	{
		strcpy(szCKey,szNewKey);
		return (int)TRUE;
	}
	return (int)FALSE;
}

int GetOwPermission(char *FileName)
{
char	szFile[CX_MAXPATH];
char	szMess[CX_MAXPATH];
int		iRes;

	if( ArcStruct.iRepAlways == OPTION_SET )
		return 0;

	os_get_file_name(FileName,szFile);
	sprintf(szMess,szOwPermission,szFile);
	cx_cputs(szMess);

	iRes = -1;
	while( iRes == -1 )
	{
		iRes = getch();
		if( strchr(szFCtrl,iRes) != NULL )
		{
			switch(iRes)
			{
				case 'Y':
				case 'y':
					iRes = 0;
					break;

				case 'N':
				case 'n':
					iRes = 1;
					break;

				case 'A':
				case 'a':
						ArcStruct.iRepAlways = OPTION_SET;
						iRes = 0;
					break;

				case 'C':
				case 'c':
					iRes = 2;
					break;
                default:;
			}
		}
	}

	return iRes;
}

void PrintCopyright(void)
{
	cx_cputs(szCopyrightStr);
}

void PrintHelp(void)
{
	PrintCopyright();
	cx_cputs(szHelpStr);
}

int getopts(int argc,char *argv[])
{
int		iCount;
UWORD32	uLen;
UWORD32	uBufLen;
char	*pTemp;

	if( argc < 3 )
	{
		PrintHelp();
		return (int)FALSE;
	}

	if( strchr(szCCtrl,argv[1][0]) != NULL )
	{
		if( (argv[1][0] == 'E') || (argv[1][0] == 'e') )
			ArcStruct.iStripPath = OPTION_SET;
		else if( (argv[1][0] == 'T') || (argv[1][0] == 't') )
			ArcStruct.iTestErrorsArc = OPTION_SET;
	}
	else
	{
		PrintHelp();
		return (int)FALSE;
	}

	strcpy(ArcStruct.szArcName,argv[2]);

	uBufLen = 0;
	for( iCount = 3; iCount < argc; iCount++ )
	{
		uLen = strlen(argv[iCount]);
		if( argv[iCount][uLen - 1] == OS_PATH_SEP )
			strcpy(ArcStruct.szOutPath,argv[iCount]);
		else if( strcmp(argv[iCount],"*.*") != 0 )
			uBufLen += uLen + 1;
	}

	if( strlen(ArcStruct.szOutPath) < 1 )
	{
		os_current_directory(ArcStruct.szOutPath,CX_MAXPATH - 1);
		os_addslash(ArcStruct.szOutPath);
	}

	if( uBufLen == 0 )
		return (int)TRUE;

	uBufLen += 2;
	ArcStruct.szFileMasks = (char *)malloc(uBufLen);
	if( ArcStruct.szFileMasks == NULL )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		return (int)FALSE;
	}

	memset(ArcStruct.szFileMasks,0,uBufLen);

	pTemp = ArcStruct.szFileMasks;
	for( iCount = 3; iCount < argc; iCount++ )
	{
		uLen = strlen(argv[iCount]);
		if( (argv[iCount][uLen - 1] != OS_PATH_SEP ) && (strcmp(argv[iCount],"*.*") != 0) )
		{
			strcpy(pTemp,argv[iCount]);
			pTemp += uLen + 1;
		}
	}

	return (int)TRUE;
}

int main(int argc,char *argv[])
{
SLONG32	lError;

	if( InitArchiver() == (int)FALSE )
	{
		ErrorMessage(SQX_EC_OUT_OF_MEMORY);
		return (int)FALSE;
	}

	if( getopts(argc,argv) == (int)FALSE )
	{
		if( ArcStruct.ArchiveStatus != SQX_EC_OK )
			ErrorMessage(ArcStruct.ArchiveStatus);
		DoneArchiver();
		return (int)FALSE;
	}

	PrintCopyright();

	ExtractFiles();
	lError = ArcStruct.ArchiveStatus;
	DoneArchiver();

	ErrorMessage(lError);

	if( lError != SQX_EC_OK )
		return (int)FALSE;
	else
		return (int)TRUE;
}
