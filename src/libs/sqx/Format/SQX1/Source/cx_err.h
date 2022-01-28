/****************************************************************************
CX_ERR.H  3.01

Based on RERR.H  2.01

C/C++ version Copyright (c) 1995 - 2002
Rainer Nausedat
Waisenhausstrasse 3
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

****************************************************************************/


//- error codes, checksumming, date and other miscellaneous routines

#if !defined(__CX_ERR_H)   // include this file only once per compilation
#define __CX_ERR_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif




/****************** error/status code constants ******************/

#define	SQX_EC_OK								0		//- no error
#define	SQX_EC_ARCHIVE_OK_RDATA_NOT				1		//- archive seems to be ok, but not the recovery data

#define	SQX_EC_FILE_NOT_FOUND					2		//- file not found
#define	SQX_EC_PATH_NOT_FOUND					3		//- path not found
#define	SQX_EC_TOO_MANY_FILES					4		//- too many open files
#define	SQX_EC_ACCESS_DENIED					5		//- file access denied
#define	SQX_EC_OUT_OF_MEMORY					8		//- insufficient memory
#define	SQX_EC_DISK_FULL						101		//- disk is full
#define	SQX_EC_NO_OS_LARGE_FILE_SUPPORT			107		//- the os we are running under does not support large files ( >2 GB resp. > 4 GB)

#define	SQX_EC_WRITE_PROTECTED					150		//- disk is write-protected
#define	SQX_EC_OUT_OF_PAPER						159		//- printer is out of paper

#define	SQX_EC_INVALID_DIC_SIZE					200		//- invalid dictionary size

#define	SQX_EC_USER_ABORT						2926	//- user aborted action
#define	SQX_EC_CANT_ACCESS_TEMP_DIR				8902	//- cannot acces temp dir
#define	SQX_EC_TEMP_DIR_FULL					8903	//- temp dir ( disk ) is full
#define	SQX_EC_CANT_CREATE_ARC_DIR				8904	//- can't create directory in arc, directory exists already
#define	SQX_EC_INVALID_DIR_NAME					8906	//- can't create directory in arc, the directory name contains invalid charaters
#define	SQX_EC_INVALID_FILENAME					8907	//- can't create file in arc, the file name contains invalid charaters
#define	SQX_EC_CANT_COPY_SOURCE_TO_SOURCE		8908	//- can't copy an archive to itself

#define	SQX_EC_EQUAL_PASSWORDS					9000	//- both passwords for the file and header encryption are equal. due to security reasons they are rejected and this error is generated
#define	SQX_EC_REQUIRES_ENCRYPTION				9001	//- header encryption requires at least standard encryption enabled
#define	SQX_EC_MISSING_HEADER_PASSWORD			9002	//- the password to encrypt the headers is missing
#define	SQX_EC_MISSING_SQX_AV_KEY				9004	//- the private key to create SQX AV data is missing
#define	SQX_EC_INVALID_AV_KEY					9006	//- the avkeyfile is invalid
#define	SQX_EC_AV_KEY_VERSION					9007	//- this SQ version cannot use the avkeyfile, please load the latest update
#define	SQX_EC_AV_DATA_VERSION					9008	//- this SQ version cannot process the avdata, please load the latest update
#define	SQX_EC_BROKEN_AV_RECORD					9009	//- this archive contains avdata, but the AV data does not match the archive. the archive were altered

#define	SQX_EC_WRONG_ARCHIVER_VERSION			9500	//- this version of the archiver cannot process all data
#define	SQX_EC_ARCHIVE_VERSION_TOO_HIGH			9501	//- archives with this version number can only be extracted

#define	SQX_EC_EXT_RDATA_DOES_NOT_MATCH			9800	//- external data recovery records do not match for a certain archive file

#define	SQX_EC_TOO_MANY_BROKEN_FBLOCKS			9896	//- to many file blocks are damaged, we cannot repair this archive file
#define	SQX_EC_DAMAGED_RDATA					9897	//- recovery data is damaged
#define	SQX_EC_NEW_RDATA_VER					9898	//- can't undamage archive, need a higher archiver version
#define	SQX_EC_RDATA_DOES_NOT_MATCH				9899	//- data recovery records do not match for a certain archive file
#define	SQX_EC_CANT_FIND_RDATA					9900	//- can't find data recovery records

#define	SQX_EC_NO_MATCHING_FILES				9906	//- no matching files in request

#define	SQX_EC_VOLUME_LIMIT_REACHED				9948	//- cannot create more volumes (999)
#define	SQX_EC_CANT_ADD_TO_MV_ARC				9949	//- cannot add files to a multi volume archive
#define	SQX_EC_CANT_DELETE_FROM_MV_ARC			9950	//- cannot delete files from a multi volume archive
#define	SQX_EC_NEED_FIRST_VOLUME				9951	//- archive is part of a multivolume archive but is not the first volume
#define	SQX_EC_MISSING_VOLUME					9952	//- archiver is missing a volume

#define	SQX_EC_ARCHIVE_LOCK						9954	//- Cant modify locked archive
#define	SQX_EC_COMMENT_BIGGER_4K				9957	//- Cant add comment bigger than 4K
#define	SQX_EC_CANT_UPDATE_ESAWP				9959	//- Cant update Encrypted Solid Archive With Password
#define	SQX_EC_UNKNOWN_METHOD					9960	//- unknown compression method
#define	SQX_EC_FILE_ENCRYPTED					9961	//- cannot extract--file is encrypted
#define	SQX_EC_BAD_FILE_CRC						9962	//- bad CRC--file is probably corrupted
#define	SQX_EC_CANNOT_CREATE					9963	//- unable to create output file
#define	SQX_EC_BAD_FILE_FORMAT					9964	//- bad archive file format
#define	SQX_EC_EMPTY_FILE_LIST					9967	//- file mask list is empty
#define	SQX_EC_NOT_A_SQX_FILE					9977	//- not an SQX file

#define	SQX_EC_FUNKTION_NOT_SUPPORTED			32769	//- function is not supported at all
#define	SQX_EC_FUNC_NOT_SUPPORTED_BY_ARCHIVE	32770	//- function is not supported for this archive type
#define	SQX_EC_SFX_STUB_NOT_INSTALLED			32771	//- the selected sfx stub was not installed

#endif  //#if !defined(__CX_ERR_H)
