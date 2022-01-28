/*********************************************************
SQX_H.H


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

#if !defined(__SQX_H_H)   //- include this file only once per compilation
#define __SQX_H_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#if !defined(_AES_H)   
//#include "crypt/aes/aes.h"
#endif

#if !defined(__BF_CRYPT_H)
//#include "crypt/bf/bf_crypt.h"
#endif

/**************************** archive headers ****************************/

#pragma pack(1)

#define CRC32_INIT					0xFFFFFFFF

#define SQX_ARC_VER					11		//- current SQX version
#define SQX_VENDOR_ID				0x5153	//- SQX "vendor ID"

//- compression method codes
#define SQX_C1_STORED				0x00	//- stored (no compression), compression type 1
#define SQX_C1_NORMAL				0x01	//- compressed, normal compression, compression type 1
#define SQX_C1_GOOD					0x02	//- compressed, good compression, compression type 1
#define SQX_C1_HIGH					0x03	//- compressed, high compression, compression type 1
#define SQX_C1_BEST					0x04	//- compressed, best compression, compression type 1
#define SQX_C1_AUDIO				0x05	//- true audio (wav, pcm) compression
//--------------------------------------------------------------------------------------------------
#define SQX_C1_EX_NORMAL			0x06
#define SQX_C1_EX_GOOD				0x07
#define SQX_C1_EX_HIGH				0x08
#define SQX_C1_EX_BEST				0x09
#define SQX_C1_EX_AUDIO				0x0A
//--------------------------------------------------------------------------------------------------
#define SQX_SH_NORMAL				0x0B	//- SQX 2
#define SQX_SH_GOOD					0x0C	//- SQX 2
#define SQX_SH_HIGH					0x0D	//- SQX 2
#define SQX_SH_BEST					0x0E	//- SQX 2
#define SQX_SH_AUDIO				0x0F	//- SQX 2

#define MAX_COMMENT_SIZE			0x2000	

//- host OS types
#define	OS_MSDOS_OR_WIN				0		//- OS/FSys is supported, compatibility to scb!
#define	OS_MSDOS					1		//- OS/FSys is supported
#define	OS_OS2						2
#define	OS_WIN32					3		//- OS/FSys is supported
#define	OS_WINNT					4		//- OS/FSys is supported
#define	OS_UNIX						5
#define	OS_MACOS					6
#define	OS_WINNTALPHA				7		//- OS/FSys is supported
#define	OS_ATARI					8
#define	OS_VAXVMS					9
#define	OS_AMIGA					10
#define	OS_NEXT						11
#define	OS_LINUX					12		//- hopefully soon :)	
#define	OS_CPM						13	
#define	OS_ZSYS						14	
#define	OS_VMCMS					15
#define	OS_BEOS						16
#define	OS_UNKNOWN					17		//- OS/FSys is supported, compatibility to scb!
#define	OS_OS2_HPFS					18
#define	OS_WIN32_FAT32				19		//- OS/FSys is supported
#define	OS_WINNT_NTFS				20		//- OS/FSys is supported
#define	OS_WINNTALPHA_NTFS			21		//- OS/FSys is supported

#define	SQX_MAIN_HEAD_ID			0x52	//- BLOCK_TYPE = 0x52	archive main header 
#define	SQX_FILE_HEAD_ID			0x44	//- BLOCK_TYPE = 0x44   archive file block  
#define	SQX_DIR_HEAD_ID				0x56	//- BLOCK_TYPE = 0x56   archive directory block  
#define	SQX_END_ARC_HEAD_ID			0x53	//- BLOCK_TYPE = 0x53   archive 'end of archive' block 
#define	SQX_COMMENT_HEAD_ID			0x43	//- BLOCK_TYPE = 0x43   archive comment block 
#define	SQX_RDATA_HEAD_ID			0x58	//- BLOCK_TYPE = 0x58   archive data recovery record 
#define	SQX_AV_DATA_HEAD_ID			0x41	//- BLOCK_TYPE = 0x41   archive authenticity information 
#define	SQX_EXTERNAL_HEAD_ID		0x45	//- BLOCK_TYPE = 0x45	archive external block (reserved) 
#define	SQX_INTERNAL_HEAD_ID		0x49	//- BLOCK_TYPE = 0x49	archive internal block (reserved)

#define	SQX_ENC_HEADS_FLAG			0x0002	//- if set, the headers of the archive file block are encrypted
#define	SQX_SOLID_ARC_FLAG			0x0004	//- if set, the archive is a solid archive
#define	SQX_MV_ARC_FLAG				0x0008	//- if set, the archive is a multi volume archive
#define	SQX_LV_ARC_FLAG				0x0010	//- if set, the archive is the last volume of a a multi volume archive
#define	SQX_ACOMMENT_FLAG			0x0020	//- if set, the archive has a main comment
#define	SQX_EXT_RDATA_FLAG			0x0040	//- if set, external recovery data was created for this archive

#define	SQX_LARGE_BLOCK_FLAG		0x0001	//- if set, usHeadSize is 4 bytes instead of 2 bytes
#define	SQX_ENC_FILE_FLAG			0x0002	//- if set, the archive file is encrypted (rijndael 128bit)
#define	SQX_SOLID_FILE_FLAG			0x0004	//- solid flag, if set, statistics and data from previous files were used on compressing
#define	SQX_PREV_VOL_FLAG			0x0008	//- if set, the archive file is continued from previous volume
#define	SQX_NEXT_VOL_FLAG			0x0010	//- if set, the archive file is continued from previous volume
#define	SQX_FCOMMENT_FLAG			0x0020	//- if set, the archive file has a main comment
#define	SQX_ASCII_FLAG				0x0040	//- if set, the file name was converted to the ASCII charset
#define	SQX_FILE64_FLAG				0x0080	//- if set, either the archive file, the orig. file or both are larger than 4GB
#define	SQX_NEXT_BLOCK_FLAG			0x8000	//- if set, another block or subblock follows

#define	SQX_EXTRA_COMPRESSOR_FLAG	0x0001	//- if set, one or more compressor extensions were used

#define	SQX_TEXT_TYPE_FLAG			0x0001	
#define	SQX_IA32IMAGE_TYPE_FLAG		0x0002	
#define	SQX_DELTA_TYPE_FLAG			0x0004	

#define	SQX_FILENAME_PLAIN_FLAG		0
#define	SQX_FILENAME_ASCII_FLAG		1
#define	SQX_FILENAME_UNICODE_FLAG	2

#define HEAD_READ_SIZE				(1 << 16)
#define PRE_HEAD_SIZE				7

#define SQX_INDEX_DIC_SIZE_32K		3
#define SQX_INDEX_DIC_SIZE_64K		4
#define SQX_INDEX_DIC_SIZE_128K		5
#define SQX_INDEX_DIC_SIZE_256K		6
#define SQX_INDEX_DIC_SIZE_512K		7
#define SQX_INDEX_DIC_SIZE_1024K	8
#define SQX_INDEX_DIC_SIZE_2048K	9
#define SQX_INDEX_DIC_SIZE_4096K	10

#define SQX_DIC_SIZE_32K			32
#define SQX_DIC_SIZE_64K			64
#define SQX_DIC_SIZE_128K			128
#define SQX_DIC_SIZE_256K			256
#define SQX_DIC_SIZE_512K			512
#define SQX_DIC_SIZE_1024K			1024
#define SQX_DIC_SIZE_2048K			2048
#define SQX_DIC_SIZE_4096K			4096

#define SQX_DIC_32K_FLAG			0x0000
#define SQX_DIC_64K_FLAG			0x0100
#define SQX_DIC_128K_FLAG			0x0200
#define SQX_DIC_256K_FLAG			0x0300
#define SQX_DIC_512K_FLAG			0x0400
#define SQX_DIC_1024K_FLAG			0x0500
#define SQX_DIC_2048K_FLAG			0x0600
#define SQX_DIC_4096K_FLAG			0x0700

#define SQX_CRYPT_NONE				0x0000
#define SQX_CRYPT_RIJNDAEL_128BIT	0x0001

#define FILENAME_FLAG_ANSI			0
#define FILENAME_FLAG_ASCII			1
#define FILENAME_FLAG_UNICODE		2

#define DCODER_8BIT					0
#define DCODER_16BIT				1

#define DCODER_ONE					1
#define DCODER_TWO					2
#define DCODER_THREE				3
#define DCODER_FOUR					4

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to usReserved3)
	UBYTE		cHeadType;		//- 0x52
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to usReserved3
	UBYTE		cHeadID[5];		//- "-sqx-"	
	UBYTE		cArcerVer;		//- SQX archiver version number used to create this archive
	UWORD16		usArcVolNum;	//- volume number of this archive. it is only used if bit 3 in usHeadFlags is set 
	UWORD32		u32ArcVolId;	//- unique volume ID of this archive. it is only used if bit 3 in usHeadFlags is set 
	UWORD16		usReserved1;	//- reserved
	UWORD16		usReserved2;	//- reserved
	UWORD16		usReserved3;	//- reserved
	//-			[comment block] it is only present if bit 5 in usHeadFlags is set 
	//-			(variable)
}SQX_MAIN_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to FILE_NAME)
	UBYTE		cHeadType;		//- 0x44
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to FILE_NAME (including)
	UBYTE		cCompFlags;		//- compressor flags
	UWORD16		usExtraFlags;	//- EXTRA_FLAGS
	UBYTE		cOsFilesys;		//- OS/filesys the archiver were running under when compressing this file 
	UBYTE		cArcerExVer;	//- SQX archiver version number required to extract this file
	UBYTE		cArcMethod;		//- compression/archiving method
	UWORD32		u32FileCRC32;	//- standard 32bit file crc
	UWORD32		u32FileAttr;	//- file attributes
	UWORD32		u32FileTime;	//- MSDOS file time stamp
	UWORD32		u32CompSize;	//- compressed file size
	UWORD32		u32OrigSize;	//- uncompressed file size
	UWORD16		usFileNameLen;	//- length of file name
	//-			FILE_NAME		file name - array of usFileNameLen bytes 
	//-			(variable)
	//-			[comment block] it is only present if bit 5 in usHeadFlags is set 
	//-			(variable)
	//-			[compressed data]			
	//-			(variable)
}SQX_FILE_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to FILE_NAME)
	UBYTE		cHeadType;		//- 0x44
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to FILE_NAME (including)
	UBYTE		cCompFlags;		//- compressor flags
	UWORD16		usExtraFlags;	//- EXTRA_FLAGS
	UBYTE		cOsFilesys;		//- OS/filesys the archiver were running under when compressing this file 
	UBYTE		cArcerExVer;	//- SQX archiver version number required to extract this file
	UBYTE		cArcMethod;		//- compression/archiving method
	UWORD32		u32FileCRC32;	//- standard 32bit file crc
	UWORD32		u32FileAttr;	//- file attributes
	UWORD32		u32FileTime;	//- MSDOS file time stamp
	UWORD32		u32CompSize;	//- compressed file size
	UWORD32		u32OrigSize;	//- uncompressed file size
	UWORD32		u32CompSize64;	//- high dword of the compressed file size. it is only present if bit 7 in usHeadFlags is set
	UWORD32		u32OrigSize64;	//- high dword of the compressed file size. it is only present if bit 7 in usHeadFlags is set
	UWORD16		usFileNameLen;	//- length of file name
	//-			FILE_NAME		file name - array of usFileNameLen bytes 
	//-			(variable)
	//-			[comment block] it is only present if bit 5 in usHeadFlags is set 
	//-			(variable)
	//-			[compressed data]			
	//-			(variable)
}SQX_EX_FILE_HEAD;

typedef struct
{
	UBYTE	cCoderType;
	UBYTE	cCoderNum;
	SLONG32	lStartOffset;
}SQX_DELTA_HEAD;

typedef struct
{
	SLONG32		lSkipSize;
	SLONG32		lFileTail;
	UWORD32		uRawDataSize;
	UBYTE		cChannels;
	UBYTE		cChannelBits;
	UWORD32		uExtra;
}SQX_AUDIO_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to DIR_NAME)
	UBYTE		cHeadType;		//- 0x56
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to DIR_NAME (including)
	UWORD32		u32DirAttr;		//- directory attributes
	UWORD16		usDirNameLen;	//- length of directory name
	//-			DIR_NAME       	directory name - array of usDirNameLen bytes 
	//-			(variable)
	//-			[comment block]	it is only present if bit 5 in usHeadFlags is set 
	//-			(variable)
}SQX_DIR_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to usHeadSize)
	UBYTE		cHeadType;		//- 0x53
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- always 7
}SQX_END_ARC_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to usOrigSize)
	UBYTE		cHeadType;		//- 0x43
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to usOrigSize
	UBYTE		cArcerExVer;	//- SQX archiver version number required to process this comment
	UBYTE		cArcMethod;		//- compression/archiving method
	UWORD16		usCompSize;		//- compressed comment size
	UWORD16		usOrigSize;		//- uncompressed comment size
	//-			[comment data]   	
	//-			(variable)
}SQX_COMMENT_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to u32RDataCRC32)
	UBYTE		cHeadType;		//- 0x58
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- block size from usHeadCRC to u32RDataCRC32
	UBYTE		cRDataID[5];	//- "SQ4RD"
	SLONG32		lRecoveryLevel;	//- size of recovery data in percent
	SLONG64		l64FileBlocks;	//- # of file blocks the compressed data was devided into
	SLONG64		l64RDataBlocks;	//- # of recovery blocks created from the compressed data 
	UBYTE		cArcerExVer;	//- SQX archiver version number required to process this recovery data
	SLONG64		l64DataSize;	//- total size of original data
	UWORD32		u32RDataCRC32;	//- crc over the recovery data
	//-			[recovery data]
	//-			F_BLOCK_CRC_1	crc (2 bytes) over first file block
	//-			...
	//-			F_BLOCK_CRC_N	crc (2 bytes) over last file block (N = u32FileBlocks)
	//-			...
	//-			RD_BLOCK_1		first block with raw recovery data
	//-			...
	//-			RD_BLOCK_N		last block with raw recovery data (N = u32RDataBlocks)
}SQX_RDATA_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to u32RDataCRC32)
	UBYTE		cHeadType;		//- 0x49
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD16		usHeadSize;		//- size of block including data
	UWORD16		usSubID;		//- unique sub id for this block type
	UWORD16		usVendorID;		//- unique third party vendor ID
	//-			[data]   	
	//-			(variable)
}SQX_INTERNAL_HEAD;

typedef struct
{
	UWORD16		usHeadCRC;		//- crc over block (cHeadType to u32RDataCRC32)
	UBYTE		cHeadType;		//- 0x49
	UWORD16		usHeadFlags;	//- BLOCK_FLAGS
	UWORD32		u32HeadSize;	//- size of block including data
	UWORD16		usSubID;		//- unique sub id for this block type
	UWORD16		usVendorID;		//- unique third party vendor ID
	//-			[data]   	
	//-			(variable)
}SQX_EX_INTERNAL_HEAD;

#pragma pack()                            

/**************************** archive headers end ****************************/

typedef struct
{
	char			szArcName[CX_MAXPATH];
	char			szOutPath[CX_MAXPATH];
	SLONG32			ArchiveStatus;
	int				iCryptedHeads;
	int				iSolidFlag;
	int				iEncryption;
	int				iMVFlag;
	int				iLastVolFlag;
	int				iFileWasSplit;
	int				iVolumeNum;
	int				iIgnoreWrite;
	SLONG64			l64TempArcOfs;
	UWORD32			uCFileCRC;
	OS_FILE			arcfile;
	OS_FILE			outfile;
	char			szPswd[CX_MAXPATH];
	char			szHeadPswd[CX_MAXPATH];
	//aes_ctx			AesCtx;
	//BLOWFISH_CTX	BfCtx;
	int				iTestErrorsArc;
	int				iStripPath;
	int				iRepAlways;
	char			*szFileMasks;
}ARC_STRUCT;

typedef struct
{
	char			szProgName[CX_MAXPATH];
	int				iNewProg;
	SLONG64			l64Size;
	SLONG64			l64Processed;
}PROG_STRUCT;

typedef struct
{
	UWORD32	uCFileCRC;
	UWORD32	u32FileCRC;
	UBYTE	cCompMethod;
	UBYTE	cCompFlags;
	UWORD32	u32FileTime;
	SLONG64	l64OrigSize;
	SLONG64	l64CompSize;
	SLONG64	l64ToFlush;
	UWORD16 usExtCompressor;
	SLONG32 lSubCompressor;
	SLONG32	lImageLastCount;
	SLONG32	lImageSize;
}IO_STRUCT;

typedef struct
{
	UWORD32	uCoderNum;
	UWORD32	uCoderType;
	UWORD32	uCoderStart;
	UWORD32 uCodeDeltas;
	UWORD32 lNeedOfs;
	SLONG32 lOffset;
	SLONG32 lOffsetSoFar;
}DELTA_CTRL;

typedef struct 
{
	SLONG32				lTagged;
	SLONG32				lCryptFlag;
	char				szFName[CX_MAXPATH];
	SLONG64				l64NewSize;
	SLONG64				l64OrigSize;
	SLONG32				lFileTime;		
	SLONG32				lAttr;			
	SLONG32				lVersion;
	UWORD32				uFileCRC;			
	SLONG32				lHostOS;			
	SLONG32				lMethod;			
	SLONG32				lFlags;
	SLONG32				lBlockType;
	SLONG32				lCompFlags;
	SLONG32				lTotalBlockSize;
	SLONG64				l64FileHdrStart;
}ARC_NODE;


#endif //- #if !defined(__SQX_H_H)  
