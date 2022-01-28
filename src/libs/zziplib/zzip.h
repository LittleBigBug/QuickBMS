#define ZZLIB   // added by Luigi Auriemma

/*---------------------------------------------*/
/* Zzip/Zzlib compressor                zzip.h */
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

#ifndef ZZIP_H
#define ZZIP_H

#include "global.h"

extern block_param_s	block;
extern time_stat_s		time_stat;
extern bool				verbose;

uint32         Zip_SM0(uint32, uint8*);
void         Unzip_SM0(uint32, uint8*);

uint32         Zip_SM1(uint32, uint8*);
void         Unzip_SM1(uint32, uint8*);

uint32      RLE_Coding(uint8*, uint8*, uint8*);
uint32    RLE_Decoding(uint8*, uint8*, uint8*);

void      M1FF2_Coding(uint8*, uint8*);
void    M1FF2_Decoding(uint8*, uint8*);

void        BWT_Coding(uint32, uint32*, uint8*);
void      BWT_Decoding(uint32, uint32, uint8*, uint32*);

void     Reverse_Block(uint8*, uint8*);
void          Analysis(uint8*, uint8*);

uint32           Split(uint8*, uint8*, uint8*);
void           UnSplit(uint8*, uint8*, uint8*);

void      Win32_Coding(uint8*, uint8*);
void    Win32_Decoding(uint8*, uint8*);

uint32         Filter1(uint8*, uint8*, uint32);
uint32       UnFilter1(uint8*, uint32);

uint32         Filter2(uint8*, uint8*);
uint32       UnFilter2(uint8*, uint8*, uint8*);

uint           MM_Test(uint8*, uint8*);
void         MM_Coding(uint8*, uint8*);
void       MM_Decoding(uint8*, uint8*);

void      Coding_Alpha(uint8*, uint32, uint32*);
void    Decoding_Alpha(uint8*, uint32, uint32*);

uint32     Crc32(uint8*, uint8*, uint32);
#define    Crc32_2(a,b)		Crc32(a,b,0xFFFFFFFFUL)

#ifdef ZZLIB

#undef	DLL_EXPORT
//#define	DLL_EXPORT __declspec (dllexport) 
#define DLL_EXPORT  // so it can work also on Linux

DLL_EXPORT int    ZzCompressBlock(unsigned char*, unsigned int, unsigned int, unsigned int);
DLL_EXPORT int  ZzUncompressBlock(unsigned char*);

DLL_EXPORT  int       OpenArchive(actions, char*, info_s*);
DLL_EXPORT void      CloseArchive(int, info_s*);
DLL_EXPORT void SetArchivePointer(int, char*, info_s*);
DLL_EXPORT void           AddFile(int, char*, unsigned int, unsigned int, unsigned int, info_s*);
DLL_EXPORT void      TestNextFile(int, info_s*);
DLL_EXPORT void   ExtractNextFile(int, char*, unsigned int, info_s*);
DLL_EXPORT void      ListNextFile(int, info_s*);
DLL_EXPORT void    DeleteNextFile(int);
DLL_EXPORT void       ListAllFile(int, info_s**);
DLL_EXPORT void       CleanMemory();
DLL_EXPORT  int    Get_last_error();

DLL_EXPORT extern int last_error;

#else  /* ZZLIB */

 int       OpenArchive(actions, char*, info_s*);
void      CloseArchive(info_s*);
void SetArchivePointer(char*, info_s*);
void           AddFile(char*, unsigned int, unsigned int, unsigned int, info_s*);
void      TestNextFile(info_s*);
void   ExtractNextFile(char*, unsigned int, info_s*);
void      ListNextFile(info_s*);
void    DeleteNextFile();
void       ListAllFile(info_s**);
void       CleanMemory();
 
extern int last_error;

#endif /* ZZLIB */

#endif /* !ZZIP_H */

/*---------------------------------------------*/
/* end                                  zzip.h */
/*---------------------------------------------*/
