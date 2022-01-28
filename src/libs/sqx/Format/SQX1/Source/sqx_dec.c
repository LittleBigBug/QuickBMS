/*********************************************************
SQX_DEC.C


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

#if !defined(__CTAB_H)   
#include "ctab.h"
#endif

#if !defined(__CX_AUDIO_H)   
#include "cx_audio.h"
#endif

#if !defined(__SQX_OS_H)   
#include "os/sqx_os.h"
#endif

#if !defined(__ARCIO_H) 
#include "arcio.h"
#endif

#if !defined(__ARCIO_H) 
#include "arcio.h"
#endif

#if !defined(__UARDE_H)   
#include "uarde.h"
#endif

#if !defined(__AES_C_H)   
#include "crypt/aes/aes_c.h"
#endif

#if !defined(__BF_CRYPT_H)
#include "crypt/bf/bf_crypt.h"
#endif

#if !defined(__CRC32_H)
#include "crc32.h"
#endif

char *SQX_ARC_MARK = "-sqx-";
#define SQX_MARK_SIZE	5

//- in sqx_main.c
extern int GetNetVolPath(char *szVolPath,int iVolNum);
extern int GetCryptKey(char *szCKey,char *FileName,int iReason);
extern int GetOwPermission(char *FileName);
extern void SetProgress(int iFinished);


int GetOWritePermission(ARC_NODE *ALNode,char *szFileName)
{
	if( os_file_exist(szFileName) == (int)TRUE )
		return GetOwPermission(szFileName);
	return 0;
}

int MatchesMask(const char *szName,const char *szMask)
{  
int iCount;
int	iLen;
	
	if( (*szMask == '\0') && (*szName == '\0') )
		return (int)TRUE;

	if( *szMask == '\0' )
		return (int)FALSE;

	if( *szMask == '*' )
    { 
		if( *(szMask + 1) == '\0' )
			return (int)TRUE;

		iLen = strlen(szName);
		for( iCount = 0; iCount <= iLen; iCount++ )
			if( (*(szName + iCount) == *(szMask + 1)) || (*(szMask + 1) == '?') )
				if( MatchesMask(szName + iCount + 1,szMask + 2) == (int)TRUE )
					return (int)TRUE;
	}
	else
    { 
		if (*szName == '\0')
			return (int)FALSE;
		if( (*szMask == '?') || (*szMask == *szName) )
			if( MatchesMask(szName + 1,szMask + 1) == (int)TRUE )
				return (int)TRUE;
    }
	return (int)FALSE;
}

int ArcNodeMatches(ARC_NODE *ALNode)
{
UWORD32	uLen;
char	*pTemp;

	if( ArcStruct.szFileMasks == NULL )
		return (int)TRUE;

	pTemp = ArcStruct.szFileMasks;
	while( *pTemp )
	{
		uLen = strlen(pTemp);
		if( MatchesMask(ALNode->szFName,pTemp) == (int)TRUE )
			return (int)TRUE;
		
		pTemp += uLen + 1;
	}

	return (int)FALSE;
}

int CharCompare(void *pVoid1,void *pVoid2,UWORD32 uCount)
{
char	*pPtr1;
char	*pPtr2;
	
	if( uCount == 0 )
		return 0;

	pPtr1 = (char *)pVoid1;
	pPtr2 = (char *)pVoid2;

	while( --uCount && *pPtr1 == *pPtr2 ) 
	{
		pPtr1++;
		pPtr2++;
	}
	return *((UBYTE *)pPtr1) - *((UBYTE *)pPtr2);
}

int DecryptHeader(UBYTE *pData,UWORD32 uSize)
{
	if( strlen(ArcStruct.szHeadPswd) == 0 )
	{
		if( GetCryptKey(ArcStruct.szHeadPswd,"",1) == (int)FALSE )
		{
			ArcStruct.ArchiveStatus = SQX_EC_FILE_ENCRYPTED;
			return (int)FALSE;
		}
	}
	InitBfEncrypt();
	BfDecrypt(pData,uSize);
	return (int)TRUE;
}

int GetFileKey(char *szFileName)
{
	if( strlen(ArcStruct.szPswd) == 0 )
	{
		if( GetCryptKey(ArcStruct.szPswd,szFileName,0) == (int)FALSE )
		{
			ArcStruct.ArchiveStatus = SQX_EC_FILE_ENCRYPTED;
			return (int)FALSE;
		}
	}
	InitRijndaelDecrypt128();
	return (int)TRUE;
}

int rsh(UBYTE *pDataPtr,UWORD16 *usHeadSize,OS_FILE OsFile,int iEncryptedHeaders)
{
SQX_FILE_HEAD	*SqxFileHead;
UWORD32			uHeadCRC;
SLONG32			lRead;

	SqxFileHead = (SQX_FILE_HEAD *)pDataPtr;

	if( iEncryptedHeaders == (int)TRUE )
	{
		lRead = os_read(usHeadSize,(SLONG32)sizeof(UWORD16),OsFile);
		if( lRead != (SLONG32)(SLONG32)sizeof(UWORD16) )
		{
			if( lRead != 0 )
				ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
			return (int)FALSE;
		}
		
		if( os_read(pDataPtr,(SLONG32)*usHeadSize,OsFile) != (SLONG32)*usHeadSize )
		{
			ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
			return (int)FALSE;
		}
		
		if( DecryptHeader(pDataPtr,(UWORD32)*usHeadSize) == (int)FALSE )
			return (int)FALSE;
	}
	else
	{
		lRead = os_read(pDataPtr,(SLONG32)PRE_HEAD_SIZE,OsFile);
		if( lRead != PRE_HEAD_SIZE )
		{
			if( lRead != 0 )
				ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
			return (int)FALSE;
		}
		
		//- !!
		if( os_read(&pDataPtr[PRE_HEAD_SIZE],(SLONG32)SqxFileHead->usHeadSize - PRE_HEAD_SIZE,OsFile) != (SLONG32)SqxFileHead->usHeadSize - PRE_HEAD_SIZE )
		{
			ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
			return (int)FALSE;
		}
	}
	
	uHeadCRC = CRC32_INIT;
	uHeadCRC = crc32(uHeadCRC,&SqxFileHead->cHeadType,SqxFileHead->usHeadSize - sizeof(UWORD16));
	uHeadCRC &= 0xFFFF;
	
	if( (UWORD16)uHeadCRC != SqxFileHead->usHeadCRC )
	{
		if( iEncryptedHeaders == (int)TRUE )
			ArcStruct.ArchiveStatus = SQX_EC_FILE_ENCRYPTED;
		else
			ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
		return (int)FALSE;
	}
	
	return (int)TRUE;
}

SLONG32 ReadComment(OS_FILE OsFile,SLONG64 l64Ofs)
{
SQX_COMMENT_HEAD	*SqxCommentHead;
UWORD16				usHeadSize;

	if( l64Ofs != - 1)
		os_seek(OsFile,l64Ofs,SEEK_SET);

	SqxCommentHead = (SQX_COMMENT_HEAD *)pCommBuf;
	
	if( rsh(pCommBuf,&usHeadSize,ArcStruct.arcfile,ArcStruct.iCryptedHeads) == (int)FALSE )
		return 0;
	
	if( (SqxCommentHead->cHeadType != SQX_COMMENT_HEAD_ID) || (SqxCommentHead->usHeadSize > MAX_COMMENT_SIZE) )
	{
		ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
		return 0;
	}

	if( ArcStruct.iCryptedHeads == OPTION_SET )
		return (SLONG32)usHeadSize + sizeof(UWORD16);
	else
		return (SLONG32)SqxCommentHead->usHeadSize;
}

int FindArcMainHeader(OS_FILE OsFile,SLONG32 lMaxSearchSize,SLONG64 *l64HeadOfs,SLONG32 *lHeadSize)
{
SQX_MAIN_HEAD		*SqxMainHead;
char				*pReadBuf;
SLONG64				l64FileSize;
SLONG32				lBufSize;
SLONG32				lLastPos;
UWORD32				uHeadCRC;

	os_seek(OsFile,(SLONG64)0, SEEK_SET);
	l64FileSize = os_filelength(OsFile);
	
	//- ...!!	 
	if( lMaxSearchSize < l64FileSize )
		lBufSize = (SLONG32)lMaxSearchSize;
	else
		lBufSize = (SLONG32)l64FileSize;
	
	pReadBuf = malloc((size_t)lBufSize);
	if( NULL == pReadBuf)
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		return (int)FALSE;
	}
	
	if( os_read(pReadBuf,lBufSize,OsFile) != lBufSize )
	{
		ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
		free(pReadBuf);
		return (int)FALSE;
	}

	lLastPos = 0;
	while( lLastPos <= lBufSize - (SLONG32)sizeof(SQX_MAIN_HEAD) + PRE_HEAD_SIZE )
	{
		if( CharCompare(SQX_ARC_MARK,&pReadBuf[lLastPos],SQX_MARK_SIZE) == 0 )
		{
			//- !!
			if( PRE_HEAD_SIZE > lLastPos )
			{
				lLastPos++;
				continue;
			}

			SqxMainHead = (SQX_MAIN_HEAD *)&pReadBuf[lLastPos - PRE_HEAD_SIZE];
			
			//- !!
			if( SQX_MAIN_HEAD_ID != SqxMainHead->cHeadType )
			{
				lLastPos++;
				continue;
			}
			
			//- !!
			uHeadCRC = CRC32_INIT;
			uHeadCRC = crc32(uHeadCRC,&SqxMainHead->cHeadType,SqxMainHead->usHeadSize - sizeof(UWORD16));
			if( (UWORD16)uHeadCRC == SqxMainHead->usHeadCRC )
			{
				*l64HeadOfs = (SLONG64)(lLastPos - PRE_HEAD_SIZE);
				*lHeadSize = (SLONG32)SqxMainHead->usHeadSize;
				
				if( 0 != (SqxMainHead->usHeadFlags & SQX_ENC_HEADS_FLAG) )
					ArcStruct.iCryptedHeads = OPTION_SET;
				
				if( 0 != (SqxMainHead->usHeadFlags & SQX_SOLID_ARC_FLAG) )
					ArcStruct.iSolidFlag = OPTION_SET;
				
				if( 0 != (SqxMainHead->usHeadFlags & SQX_MV_ARC_FLAG) )
					ArcStruct.iMVFlag = OPTION_SET;

				if( 0 != (SqxMainHead->usHeadFlags & SQX_LV_ARC_FLAG) )
					ArcStruct.iLastVolFlag = OPTION_SET;

				if( 0 != (SqxMainHead->usHeadFlags & SQX_ACOMMENT_FLAG) )
					*lHeadSize += ReadComment(OsFile,*l64HeadOfs + *lHeadSize);

				free(pReadBuf);
				
				return (int)TRUE; 
			}
		}
		lLastPos++;
	}
	free(pReadBuf);
	return (int)FALSE;
}

int ReadNextArcFHDR(ARC_NODE *ALNode)
{
SQX_EX_FILE_HEAD		*SqxExFileHead;
SQX_FILE_HEAD			*SqxFileHead;
SQX_INTERNAL_HEAD		*SqxInternalHead;
SQX_DIR_HEAD			*SqxDirHead;
UWORD32					uNameSize;
UWORD32					uNameIndex;
SLONG32					lReadComment;
SLONG64					l64AddSize;
UWORD16					usHeadSize;
#if defined(__SQX_LARGE_FILE_SUPPORT__)   
LARGE_INTEGER			li64Orig;
LARGE_INTEGER			li64New;
#endif	
	//- set file pointer 
	os_seek(ArcStruct.arcfile,ArcStruct.l64TempArcOfs,SEEK_SET);
	
	ALNode->l64FileHdrStart = ArcStruct.l64TempArcOfs;

	//- read in crc,blocktype etc.
	SqxFileHead = (SQX_FILE_HEAD *)pCommBuf;
	SqxExFileHead = (SQX_EX_FILE_HEAD *)pCommBuf;
	//- ...
	l64AddSize = 0;

	if( rsh(pCommBuf,&usHeadSize,ArcStruct.arcfile,ArcStruct.iCryptedHeads) == (int)FALSE )
		return (int)FALSE;
	
	//- 'end of archive' marker or data recovery block
	if( (SqxFileHead->cHeadType == SQX_END_ARC_HEAD_ID) || (SqxFileHead->cHeadType == SQX_RDATA_HEAD_ID) || (SqxFileHead->cHeadType == SQX_AV_DATA_HEAD_ID) )
		return (int)FALSE;
	
	//- !!
	lReadComment = OPTION_NONE;
	
	if( SqxFileHead->cHeadType == SQX_FILE_HEAD_ID )
	{
		ALNode->lTagged = OPTION_SET;

		if( (SqxFileHead->usHeadFlags & SQX_FILE64_FLAG) != 0 )
		{
			uNameIndex = sizeof(SQX_EX_FILE_HEAD);
			uNameSize = SqxExFileHead->usFileNameLen;
		}
		else
		{
			uNameIndex = sizeof(SQX_FILE_HEAD);
			uNameSize = SqxFileHead->usFileNameLen;
		}
		
		if( uNameSize > CX_MAXPATH - 1 )
			uNameSize = CX_MAXPATH - 1;
		memmove(ALNode->szFName,&pCommBuf[uNameIndex],uNameSize);

#if defined(__SQX_OS_WIN32__)   
		if( (SqxFileHead->usHeadFlags & SQX_ASCII_FLAG) != 0 )
			OemToChar(ALNode->szFName,ALNode->szFName);
#endif

		if( (SqxFileHead->usHeadFlags & SQX_FCOMMENT_FLAG) != 0 )
			lReadComment = OPTION_SET;

#if defined(__SQX_LARGE_FILE_SUPPORT__)   
		li64Orig.LowPart = SqxFileHead->u32OrigSize;
		li64New.LowPart = SqxFileHead->u32CompSize;
		if( (SqxFileHead->usHeadFlags & SQX_FILE64_FLAG) != 0 )
		{
			li64Orig.HighPart = SqxExFileHead->u32OrigSize64;
			li64New.HighPart = SqxExFileHead->u32CompSize64;
		}
		else
		{
			li64Orig.HighPart = 0;
			li64New.HighPart = 0;
		}
		
		ALNode->l64NewSize = (SLONG64)li64New.QuadPart;
		ALNode->l64OrigSize = (SLONG64)li64Orig.QuadPart;
#else	
		if( (SqxFileHead->usHeadFlags & SQX_FILE64_FLAG) != 0 )
		{
			ArcStruct.ArchiveStatus = SQX_EC_NO_OS_LARGE_FILE_SUPPORT;
			return (int)FALSE;
		}
		
		ALNode->l64NewSize = (SLONG64)SqxFileHead->u32CompSize;
		ALNode->l64OrigSize = (SLONG64)SqxFileHead->u32OrigSize;
#endif
		
		ALNode->lFlags = (SLONG32)SqxFileHead->usHeadFlags;

		if( ((ALNode->lFlags & SQX_PREV_VOL_FLAG) != OPTION_NONE) && ( ArcStruct.iVolumeNum == 0) )
		{
			ArcStruct.ArchiveStatus = SQX_EC_NEED_FIRST_VOLUME;
			return (int)FALSE;
		}

		if( (ALNode->lFlags & SQX_ENC_FILE_FLAG) != 0 )
			ALNode->lCryptFlag = SQX_CRYPT_RIJNDAEL_128BIT;

		ALNode->lFileTime = (SLONG32)SqxFileHead->u32FileTime;
		ALNode->lAttr = (SLONG32)SqxFileHead->u32FileAttr;
		ALNode->lVersion = (SLONG32)SqxFileHead->cArcerExVer;
		ALNode->lMethod = (SLONG32)SqxFileHead->cArcMethod;
		ALNode->uFileCRC = SqxFileHead->u32FileCRC32;
		ALNode->lHostOS = (SLONG32)SqxFileHead->cOsFilesys;
		ALNode->lBlockType = (SLONG32)SqxFileHead->cHeadType;
		
		if( ArcStruct.iCryptedHeads == OPTION_SET )
			ALNode->lTotalBlockSize = usHeadSize + (sizeof(UWORD16));
		else
			ALNode->lTotalBlockSize = (SLONG32)SqxFileHead->usHeadSize;
		
		ALNode->lCompFlags = (SLONG32)SqxFileHead->cCompFlags;

		//- !!
		l64AddSize += ALNode->l64NewSize;
		l64AddSize += (SLONG64)ALNode->lTotalBlockSize;
	}
	else if( SqxFileHead->cHeadType == SQX_DIR_HEAD_ID )
	{
		ALNode->lTagged = OPTION_SET;
		
		SqxDirHead = (SQX_DIR_HEAD *)pCommBuf;
		uNameIndex = sizeof(SQX_DIR_HEAD);
		uNameSize = SqxDirHead->usDirNameLen;
		if( uNameSize > CX_MAXPATH - 1 )
			uNameSize = CX_MAXPATH - 1;
		
		memmove(ALNode->szFName,&pCommBuf[uNameIndex],uNameSize);

#if defined(__SQX_OS_WIN32__)   
		if( (SqxFileHead->usHeadFlags & SQX_ASCII_FLAG) != 0 )
			OemToChar(ALNode->szFName,ALNode->szFName);
#endif		

		ALNode->lAttr = (SLONG32)SqxDirHead->u32DirAttr;
		ALNode->l64NewSize = 0;
		ALNode->l64OrigSize = 0;
		ALNode->lBlockType = (SLONG32)SqxDirHead->cHeadType;
		
		if( ArcStruct.iCryptedHeads == OPTION_SET )
			ALNode->lTotalBlockSize += (sizeof(UWORD16));

		if( (SqxDirHead->usHeadFlags & SQX_FCOMMENT_FLAG) != 0 )
			lReadComment = OPTION_SET;

		if( ArcStruct.iCryptedHeads == OPTION_SET )
			ALNode->lTotalBlockSize = (usHeadSize + (sizeof(UWORD16)));
		else
			ALNode->lTotalBlockSize = (SLONG32)SqxDirHead->usHeadSize;
		l64AddSize += (SLONG64)ALNode->lTotalBlockSize;
	}
	else 
	{
		SqxInternalHead = (SQX_INTERNAL_HEAD *)pCommBuf;
		
		ALNode->l64NewSize = 0;
		ALNode->l64OrigSize = 0;
		ALNode->lBlockType = (SLONG32)SqxInternalHead->cHeadType;
		
		if( ArcStruct.iCryptedHeads == OPTION_SET )
			ALNode->lTotalBlockSize = (usHeadSize + (sizeof(UWORD16)));
		else
			ALNode->lTotalBlockSize = (SLONG32)SqxFileHead->usHeadSize;
		l64AddSize += (SLONG64)ALNode->lTotalBlockSize;
	}
	
	//- are the some other blocks to read ??
	SqxInternalHead = (SQX_INTERNAL_HEAD *)pCommBuf;

	while( (SqxInternalHead->usHeadFlags & SQX_NEXT_BLOCK_FLAG) != 0 )
	{
		if( rsh(pCommBuf,&usHeadSize,ArcStruct.arcfile,ArcStruct.iCryptedHeads) == (int)FALSE )
			return (int)FALSE;
		
		if( ArcStruct.iCryptedHeads == OPTION_SET )
		{
			ALNode->lTotalBlockSize += (usHeadSize + (sizeof(UWORD16)));
			l64AddSize += (usHeadSize + (sizeof(UWORD16)));
		}
		else
		{
			ALNode->lTotalBlockSize += (SLONG32)SqxInternalHead->usHeadSize;
			l64AddSize += (SLONG64)SqxInternalHead->usHeadSize;
		}
	}

	if( lReadComment == OPTION_SET )
	{
		lReadComment = ReadComment(ArcStruct.arcfile,-1);
		ALNode->lTotalBlockSize += lReadComment;
		l64AddSize += lReadComment;
	}
	
	ArcStruct.l64TempArcOfs += l64AddSize;
	return (int)TRUE;
}

int InitDecompressor(void)
{
	if( InitBitExtraction() == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		return (int)FALSE;
	}
	
	if( InitDecodeLz() == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		DoneBitExtraction();
		return (int)FALSE;
	}
	
	if( AllocateDecodeTabs() == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		DoneBitExtraction();
		FreeDecodeLz();
		return (int)FALSE;
	}

	if( InitAudioDecompressor() == (int)FALSE )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		DoneBitExtraction();
		FreeDecodeLz();
		FreeDecodeTabs();
		return (int)FALSE;
	}
	
	return (int)TRUE;
}

void FreeDecompressor(void)
{
	DoneBitExtraction();
	FreeDecodeLz();
	FreeDecodeTabs();
	FreeAudioDecompressor();
}

int InitArchiver(void)
{
	pCommBuf = malloc(HEAD_READ_SIZE);
	if( pCommBuf == NULL )
	{
		ArcStruct.ArchiveStatus = SQX_EC_OUT_OF_MEMORY;
		return (int)FALSE;
	}
	
	memset(&ArcStruct,0,sizeof(ARC_STRUCT));
	ArcStruct.outfile = OS_INVALID_FILE;
	ArcStruct.arcfile = OS_INVALID_FILE;
	
	return (int)TRUE;
}

void DoneArchiver(void)
{
	if( pCommBuf != NULL )
		free(pCommBuf);
	if( ArcStruct.szFileMasks != NULL )
		free(ArcStruct.szFileMasks);
	memset(&ArcStruct,0,sizeof(ARC_STRUCT));
}

void GetOutFileName(char *szFileName,char *szOutFName)
{
char	szTemp[CX_MAXPATH];

	if( ArcStruct.iStripPath == OPTION_SET )
	{
		os_get_file_name(szFileName,szTemp);
		strcpy(szOutFName,ArcStruct.szOutPath);
		strcat(szOutFName,szTemp);
	}
	else
	{
		strcpy(szOutFName,ArcStruct.szOutPath);
		strcat(szOutFName,szFileName);
	}
}

void UnstoreArcFile(void)
{
UWORD32	uRead;
	
	memset(&DeltaCtrl,0,sizeof(DELTA_CTRL));

	while( IoStruct.l64CompSize > 0 )
	{
		uRead = (UWORD32)ReadDataBlock(pCommBuf,HEAD_READ_SIZE);
		if( (SLONG64)uRead > IoStruct.l64OrigSize )
			uRead = (UWORD32)IoStruct.l64OrigSize;
		IoStruct.l64OrigSize -= uRead;
		WriteOutBuf(pCommBuf,uRead);
		if( ArcStruct.ArchiveStatus != SQX_EC_OK )
			return;
	}
	SetProgress(1);
}

void DecodeAudioFile(void)
{
	UncompressAudioFile();
}

void ExtractArcFile(ARC_NODE *ALNode)
{
	ProgressStruct.iNewProg = OPTION_SET;
	ProgressStruct.l64Processed = 0;
	ProgressStruct.l64Size = ALNode->l64OrigSize;
	strcpy(ProgressStruct.szProgName,os_find_file_name(ALNode->szFName));

	if( ALNode->lMethod == SQX_C1_STORED )
		UnstoreArcFile();
	else if( ALNode->lMethod == SQX_C1_AUDIO )
		DecodeAudioFile();
	else
		DecodeFile(ALNode->lCompFlags,ALNode->lFlags & SQX_SOLID_FILE_FLAG,(1U << (((ALNode->lFlags >> 8) & 15) + 15)));

	if( (ArcStruct.iFileWasSplit == OPTION_NONE) && (IoStruct.u32FileCRC != ArcStruct.uCFileCRC) )
	{
		if( ArcStruct.iEncryption != 0 )
			ArcStruct.ArchiveStatus = SQX_EC_FILE_ENCRYPTED;
		else
			ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_CRC;
	}
}

int GetNextVolumeName(SLONG32 lFoceVolumeName)
{
char	szArcName[CX_MAXPATH];
char	szTempExt[CX_MAXPATH];
char	szNumStr[20];

	if( ArcStruct.ArchiveStatus != SQX_EC_OK )
		return (int)FALSE;
	
	os_get_file_name(ArcStruct.szArcName,szArcName);

	ArcStruct.iVolumeNum++;

	ltoa((long)ArcStruct.iVolumeNum,szNumStr,10);
	if( ArcStruct.iVolumeNum < 10 )
	{
		strcpy(szTempExt,".00");
		strcat(szTempExt,szNumStr);
	}
	else if( ArcStruct.iVolumeNum < 100 )
	{
		strcpy(szTempExt,".0");
		strcat(szTempExt,szNumStr);
	}
	else
	{
		strcpy(szTempExt,".");
		strcat(szTempExt,szNumStr);
	}

	os_set_file_ext(szArcName,szTempExt);
	os_get_dir_name(ArcStruct.szArcName,ArcStruct.szArcName);
	strcat(ArcStruct.szArcName,szArcName);

	if( os_file_exist(ArcStruct.szArcName) == (int)TRUE )
		return (int)TRUE;

	//- uncompressor says that we have no splitted file
	//- pending, so it should be save to return...
	if( lFoceVolumeName == OPTION_NONE )
		return (int)FALSE;

	while( TRUE )
	{
		os_get_dir_name(ArcStruct.szArcName,ArcStruct.szArcName);

		if( GetNetVolPath(ArcStruct.szArcName,ArcStruct.iVolumeNum + 1) == (int)FALSE )
		{
			ArcStruct.ArchiveStatus = SQX_EC_USER_ABORT;
			return (int)FALSE;
		}
		
		os_addslash(ArcStruct.szArcName);
		strcat(ArcStruct.szArcName,szArcName);
		
		if( os_file_exist(ArcStruct.szArcName) == (int)TRUE )
			return (int)TRUE;
	}
	
	ArcStruct.ArchiveStatus = SQX_EC_MISSING_VOLUME;
	
	return (int)FALSE;;
}

int OpenNextVolume(void)
{
ARC_NODE	AlNode;
SLONG64		l64HeadOfs;
SLONG32		lHeadSize;

	if( GetNextVolumeName(1) == (int)FALSE )
		return (int)FALSE;
	
	if( ArcStruct.arcfile != OS_INVALID_FILE )
		os_close(ArcStruct.arcfile);

	ArcStruct.arcfile = os_open(ArcStruct.szArcName,"rb");
	if( ArcStruct.arcfile == OS_INVALID_FILE )
		return (int)FALSE;
		
	if( FindArcMainHeader(ArcStruct.arcfile,0x20000,&l64HeadOfs,&lHeadSize) == (int)FALSE )
		return (int)FALSE;
		
	ArcStruct.l64TempArcOfs = l64HeadOfs + lHeadSize;

	memset(&AlNode,0,sizeof(ARC_NODE));
	if( ReadNextArcFHDR(&AlNode) == (int)FALSE )
		return (int)FALSE;

	if( (AlNode.lFlags & SQX_NEXT_VOL_FLAG) != OPTION_NONE )
		ArcStruct.iFileWasSplit = OPTION_SET;
	else
		ArcStruct.iFileWasSplit = OPTION_NONE;
	
	//- !!
	IoStruct.l64CompSize = AlNode.l64NewSize;
	ArcStruct.uCFileCRC = AlNode.uFileCRC;

	return (int)TRUE;
}

void ExtractFiles(void)
{
ARC_NODE	AlNode;
SLONG64		l64HeadOfs;
SLONG32		lHeadSize;
SLONG32		lProcessMember;
SLONG32		lSkipFlag;
char		szFileToSkip[CX_MAXPATH];
char		szOutFName[CX_MAXPATH];

	if( InitDecompressor() == (int)FALSE )
		return;

	lSkipFlag = OPTION_NONE;

	while( TRUE )
	{
		ArcStruct.arcfile = os_open(ArcStruct.szArcName,"rb");
		if( ArcStruct.arcfile == OS_INVALID_FILE )
			break;
		
		if( FindArcMainHeader(ArcStruct.arcfile,0x20000,&l64HeadOfs,&lHeadSize) == (int)FALSE )
			break;
		
		ArcStruct.l64TempArcOfs = l64HeadOfs + lHeadSize;

		while( TRUE )
		{
			memset(&AlNode,0,sizeof(ARC_NODE));
			if( ReadNextArcFHDR(&AlNode) == (int)FALSE )
				break;

			if( AlNode.lTagged == OPTION_NONE )
				continue;
			
			if( lSkipFlag == OPTION_SET )
			{
				if( strcmp(szFileToSkip,AlNode.szFName) == 0 )
					continue;
				lSkipFlag = OPTION_NONE;
			}

			lProcessMember = ArcNodeMatches(&AlNode);

			if( (lProcessMember == OPTION_NONE) && (ArcStruct.iSolidFlag == OPTION_NONE) )
				continue;
			
			IoStruct.l64CompSize = AlNode.l64NewSize;
			IoStruct.l64OrigSize = AlNode.l64OrigSize;
			IoStruct.l64ToFlush = AlNode.l64OrigSize;
			IoStruct.lImageSize = (SLONG32)AlNode.l64OrigSize;
			IoStruct.u32FileCRC = CRC32_INIT;
			IoStruct.lImageLastCount = 0;
			IoStruct.usExtCompressor = 0;
			ArcStruct.uCFileCRC = AlNode.uFileCRC;

			GetOutFileName(AlNode.szFName,szOutFName);

			os_seek(ArcStruct.arcfile,AlNode.l64FileHdrStart + AlNode.lTotalBlockSize,SEEK_SET);
			ArcStruct.iIgnoreWrite = OPTION_NONE;

			if( (lProcessMember != OPTION_NONE) && (ArcStruct.iTestErrorsArc != OPTION_SET) )
			{
				switch( GetOWritePermission(&AlNode,szOutFName) )
				{
					case 1:
						if( ArcStruct.iSolidFlag == OPTION_SET )
							ArcStruct.iIgnoreWrite = OPTION_SET;
						else
						{
							strcpy(szFileToSkip,AlNode.szFName);
							lSkipFlag = OPTION_SET;
							continue;
						}
						break;
				
					case 2:
						ArcStruct.ArchiveStatus = SQX_EC_USER_ABORT;
						break;
					default: //- 0
						break;
				}
			}
			else
				ArcStruct.iIgnoreWrite = OPTION_SET;

			if( ArcStruct.iTestErrorsArc == OPTION_SET )
				ArcStruct.iIgnoreWrite = OPTION_SET;

			if( AlNode.lBlockType == SQX_DIR_HEAD_ID )
			{
				if( ArcStruct.iIgnoreWrite == OPTION_NONE )
				{
					if( os_create_full_file_path(szOutFName) == (int)FALSE )
						ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
#if !defined(__SQX_OS_LINUX__)   
					os_setfileattr(szOutFName,(AlNode.lAttr & ~OS_ATTRIBUTE_DIRECTORY));
#else
					//- TODO Linux
#endif
				}
				continue;
			}
			else
			{
				if( (ArcStruct.iIgnoreWrite == OPTION_NONE) && (ArcStruct.iFileWasSplit == OPTION_NONE) )
				{
					if( os_create_ouput_file(&ArcStruct.outfile,szOutFName) == (int)FALSE )
					{
						ArcStruct.ArchiveStatus = SQX_EC_ACCESS_DENIED;
						break;
					}
				}

				ArcStruct.iEncryption = (int)AlNode.lCryptFlag;

				if( AlNode.lCryptFlag != OPTION_NONE )
				{
					if( GetFileKey(AlNode.szFName) == (int)FALSE )
						break;
				}
				
				if( (AlNode.lFlags & SQX_NEXT_VOL_FLAG) != OPTION_NONE )
					ArcStruct.iFileWasSplit = OPTION_SET;
				else
					ArcStruct.iFileWasSplit = OPTION_NONE;
				
				ExtractArcFile(&AlNode);

				if( ArcStruct.ArchiveStatus != SQX_EC_OK )
					break;

				if( (ArcStruct.iIgnoreWrite == OPTION_NONE)  && (ArcStruct.iFileWasSplit == OPTION_NONE) )
				{
#if !defined(__SQX_OS_LINUX__)   
					os_setdosftime(ArcStruct.outfile,(AlNode.lFileTime >> 16),(AlNode.lFileTime & 0xFFFF));
					os_close(ArcStruct.outfile);
					ArcStruct.outfile = OS_INVALID_FILE;
					os_setfileattr(szOutFName,AlNode.lAttr);
#else
					//- TODO Linux
#endif
				}
			}
			
		} //- while #2
	
		if( (ArcStruct.iMVFlag == OPTION_NONE) || ((ArcStruct.iMVFlag == OPTION_SET) && (ArcStruct.iLastVolFlag == OPTION_SET)) || (ArcStruct.ArchiveStatus != SQX_EC_OK) )
			break;
		
		if( GetNextVolumeName(1) == (int)FALSE )
			break;

	}//- while #1
	
	if( ArcStruct.outfile != OS_INVALID_FILE )
		os_close(ArcStruct.outfile);
	
	if( ArcStruct.arcfile != OS_INVALID_FILE )
		os_close(ArcStruct.arcfile);

	FreeDecompressor();
}