/*---------------------------------------------*/
/* Zzlib compression library           zzlib.h */
/*---------------------------------------------*/

/*
  This file is a part of zzip and/or zzlib, a program and
  library for lossless, block-sorting data compression.
  Copyright (C) 1999-2001 Damien Debin. All Rights Reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the 
  Free Software Foundation, Inc., 
  59 Temple Place, Suite 330, 
  Boston, MA 02111-1307 USA

  Damien Debin
  <damien@debin.net>

  This program is based on (at least) the work of: Mike Burrows, 
  David Wheeler, Peter Fenwick, Alistair Moffat, Ian H. Witten, 
  Robert Sedgewick, Jon Bentley, Brenton Chapin, Stephen R. Tate, 
  Szymon Grabowski, Bernhard Balkenhol, Stefan Kurtz
*/

#ifndef ZZLIB_H
#define ZZLIB_H

#include <time.h>

/* error constants */
typedef enum
{
	OK                       =  0,
	NOT_ENOUGH_MEMORY        = -1,
    CRC_ERROR                = -2,
    TOO_MANY_SESSION         = -3,  /* too many sessions open at the same time */
	BAD_PARAMETER            = -4,  /* bad parameter in the function call      */
	UNEXPECTED_EOF           = -5,
	NOT_A_ZZIP_FILE          = -6,
	UNSUPPORTED_VERSION      = -7,  /* unsupported compression method          */
	CANNOT_CLOSE_INPUT_FILE  = -8,
	CANNOT_CLOSE_OUTPUT_FILE = -9,
	CANNOT_OPEN_INPUT_FILE   = -10,
	CANNOT_OPEN_OUTPUT_FILE  = -11,
	FSEEK_INPUT_FILE         = -12, /* error while doing a 'fseek'             */
	FSEEK_OUTPUT_FILE        = -13, /* error while doing a 'fseek'             */
	FILE_NOT_FOUND           = -14
} errors;

/* actions allowed for 'OpenArchive' */
typedef enum
{
	NONE,
	CREATE,  /* create an archive                               */
	EXTRACT, /* extract files from an archive                   */
	TEST,    /* test an archive                                 */
	LIST,    /* list content of an archive                      */
	UPDATE,  /* if you want to add files to an existing archive */
	DELETE   /* delete files in an archive                      */
} actions;

/* archive/file attributes */
typedef struct
{
	char         *filename;
	time_t       filetime;
	unsigned int input_size;
	unsigned int output_size;
	unsigned int nb_of_file;
} info_s;

/*
 * ZzCompressBlock
 * 
 * Compress a memory block and return the compressed block.
 * 
 * input : 
 *   buffer     : data to compress
 *   size       : size of the input block
 *   level      : 0/1 (1 = zzip option '-mx')
 *   multimedia : 0/1 multimedia compression
 * 
 * output : 
 *   buffer : compressed data 
 * 
 * return value :
 *   >=0 : size of the compressed block (everything is Ok)
 *   -1  : error
 * 
 * (the size of 'buffer' must be at least the size of the block to compress + 16
 *  bytes)
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 * 
 * ZzCompressBlock memory usage : 9 times the size of buffer
 */
__declspec (dllimport) int ZzCompressBlock(unsigned char *buffer, 
										   unsigned int  size, 
										   unsigned int  level,
										   unsigned int  multimedia);

/*
 * ZzUncompressBlock
 * 
 * Uncompress a memory block and return the uncompressed block.
 * 
 * input :
 *   buffer : data to uncompress
 * 
 * output : 
 *   buffer : uncompressed data 
 * 
 * return value :
 *   >=0 : size of the uncompressed block (everything is Ok)
 *   -1  : error
 * 
 * (the size of 'buffer' must be at least the size of the uncompressed block)
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 * 
 * The 32bit unsigned integer coded in the first four bytes of a compressed block 
 * is always greater or equal (100%<x<120%) to the size of the uncompressed block, 
 * you can use this information if you don't know the size of the uncompressed 
 * block.
 */
__declspec (dllimport) int ZzUncompressBlock(unsigned char *buffer);

/*
 * OpenArchive
 * 
 * Open/create a zzip archive file and return a handle corresponding to the 
 * session.
 * 
 * input :
 *   action : what you want to do with this archive !
 *   filename : name of the archive file
 * 
 * output :
 *   info : attributes updated :
 *            'nb_of_file' : number of file in this archive
 * 
 * return value :
 *   int : session handle
 * 
 * Archive pointer is set to the first file. (cf. SetArchivePointer)
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) int OpenArchive(actions action, 
									   char    *filename, 
									   info_s  *info);

/*
 * CloseArchive
 * 
 * Close a zzip archive file.
 * 
 * input :
 *   handle : session handle
 * 
 * output :
 *   info : attributes updated :
 *            'nbfile' : number of file in this archive
 * 	          'input_size' : input size of processed files
 *            'output_size' : output size of processed files
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void CloseArchive(int    handle, 
										 info_s *info);

/*
 * SetArchivePointer
 * 
 * Set the archive pointer to a given file, useful with 
 * (List|Test|Extract|Delete)NextFile
 * 
 * input :
 *   handle : session handle
 *   filename : name of the file where you want to set the pointer
 * 
 * output :
 *   info : file attributes updated :
 *            'filename' : name of the file
 * 	          'filetime' : date of the file
 * 	          'input_size' : original size of this file
 *            'output_size' : compressed size of this file
 * 
 * if filename==NULL, setArchivePointer moves the pointer to the first file
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void SetArchivePointer(int    handle, 
											  char   *filename, 
											  info_s *info);

/*
 * AddFile
 * 
 * Add a file to the archive.
 * 
 * input :
 *   handle : session handle
 *   filename : name of the file to add
 *   compression_level : 0/1 (1 = zzip option '-mx')
 *   multimedia : 0/1, 1: to use multimedia compression
 *   block_size : size of the memory block used for the BWT
 * 
 * output :
 *   info : archive attributes updated :
 * 	          'input_size' : original size of this file
 *            'output_size' : compressed size of this file
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void AddFile(int          handle, 
									char         *filename, 
									unsigned int compression_level, 
									unsigned int multimedia, 
									unsigned int block_size, 
									info_s       *info);

/*
 * TestNextFile
 * 
 * Test the next file, according to the archive pointer, and move the archive
 * pointer to the next file
 * 
 * input :
 *   handle : session handle
 * 
 * output :
 *   info : archive attributes updated :
 *            'filename' : name of the file
 * 	          'filetime' : date of the file
 * 	          'input_size' : compressed size of this file
 *            'output_size' : original size of this file
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void TestNextFile(int    handle, 
										 info_s *info);

/*
 * ExtractNextFile
 * 
 * Extract the next file, according to the archive pointer, and move the archive 
 * pointer to the next file
 * 
 * input :
 *   handle : session handle
 *   with_path : 0/1 create the file accordind to its path (if the path does not 
 *               exist, zzlib creates it)
 * 
 * output :
 *   info : archive attributes updated :
 *            'filename' : name of the file
 * 	          'filetime' : date of the file
 * 	          'input_size' : compressed size of this file
 *            'output_size' : original size of this file
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void ExtractNextFile(int          handle, 
											char         *filename, 
											unsigned int with_path, 
											info_s       *info);

/*
 * ListNextFile
 * 
 * Get info about the next file, according to the archive pointer, and move the
 * archive pointer to the next file. Function useful to skip a file.
 * 
 * input :
 *   handle : session handle
 * 
 * output :
 *   info : archive attributes updated :
 *            'filename' : name of the file
 * 	          'filetime' : date of the file
 * 	          'input_size' : original size of this file
 *            'output_size' : compressed size of this file
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void ListNextFile(int    handle,
										 info_s *info);

/*
 * ListAllFile
 * 
 * Get info about all files of this archive and move the archive pointer after 
 * the last file.
 * 
 * input :
 *   handle : session handle
 * 
 * output :
 *   info_array : info about the files (you have to allocate memory for 
 *                info_array before calling this function)
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void ListAllFile(int    handle, 
										info_s **info_array);

/*
 * DeleteNextFile
 * 
 * Delete the next file from the archive, according to the archive pointer.
 * 
 * input :
 *   handle : session handle
 * 
 * After the function call, you have to check the 'last_error' variable for 
 * possible errors.
 */
__declspec (dllimport) void DeleteNextFile(int handle);


/*
 * CleanMemory
 * 
 * Clean unused memory blocks. Useful after an error to clean everything.
 */
__declspec (dllimport) void CleanMemory();

/* 
 * last_error
 * 
 * This variable contains OK if nothing happened during the last call, otherwise
 * it contains the error number (if last_error>0, last_error stands for 'errno')
 * (cf. errno.h)
 */
__declspec (dllimport) int last_error;

__declspec (dllimport) int Get_last_error();

#endif /* !ZZLIB_H */

/*---------------------------------------------*/
/* end                                 zzlib.h */
/*---------------------------------------------*/
