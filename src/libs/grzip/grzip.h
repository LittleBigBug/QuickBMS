/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor             grzip.h */
/* GRZipII/libGRZip Global Header File             */
/*-------------------------------------------------*/

/*--
  This file is a part of GRZipII and/or libGRZip, a program
  and library for lossless, block-sorting data compression.

  Copyright (C) 2002-2004 Grebnov Ilya. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  Grebnov Ilya, Ivanovo, Russian Federation.
  Ilya.Grebnov@magicssoft.ru, http://magicssoft.ru/

  This program is based on (at least) the work of:
  Juergen Abel, Jon L. Bentley, Edgar Binder,
  Charles Bloom, Mike Burrows, Andrey Cadach,
  Damien Debin, Sebastian Deorowicz, Peter Fenwick,
  George Plechanov, Michael Schindler, Robert Sedgewick,
  Julian Seward, David Wheeler, Vadim Yoockin.

  For more information on these sources, see the manual.
--*/


#ifndef _GRZLIB_H
#define _GRZLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

#define GRZ_NO_ERROR                0
#define GRZ_NOT_ENOUGH_MEMORY      -1
#define GRZ_CRC_ERROR              -2
#define GRZ_UNEXPECTED_EOF         -3

#define GRZ_NOT_COMPRESSIBLE       -4
#define GRZ_FAST_BWT_FAILS         -5

#define GRZ_MaxBlockSize           8*1024*1024-512
#define GRZ_Log2MaxBlockSize       23

#define GRZ_Disable_LZP            0x000

#define GRZ_LZP_HTS10              0x007
#define GRZ_LZP_HTS11              0x008
#define GRZ_LZP_HTS12              0x009
#define GRZ_LZP_HTS13              0x00A
#define GRZ_LZP_HTS14              0x00B
#define GRZ_LZP_HTS15              0x00C
#define GRZ_LZP_HTS16              0x00D
#define GRZ_LZP_HTS17              0x00E
#define GRZ_LZP_HTS18              0x00F

#define GRZ_LZP_MML2               0x000
#define GRZ_LZP_MML5               0x010
#define GRZ_LZP_MML8               0x020
#define GRZ_LZP_MML11              0x030
#define GRZ_LZP_MML14              0x040
#define GRZ_LZP_MML17              0x050
#define GRZ_LZP_MML20              0x060
#define GRZ_LZP_MML23              0x070
#define GRZ_LZP_MML26              0x080
#define GRZ_LZP_MML29              0x090
#define GRZ_LZP_MML32              0x0A0
#define GRZ_LZP_MML35              0x0B0
#define GRZ_LZP_MML38              0x0C0
#define GRZ_LZP_MML41              0x0D0
#define GRZ_LZP_MML44              0x0E0
#define GRZ_LZP_MML47              0x0F0

#define GRZ_Enable_DeltaFlt        0x000
#define GRZ_Disable_DeltaFlt       0x800

#define GRZ_Compression_BWT        0x000
#define GRZ_Compression_ST4        0x100

#define GRZ_Compression_WFC        0x000
#define GRZ_Compression_MTF        0x200

#define GRZ_BWTSorting_Strong      0x000
#define GRZ_BWTSorting_Fast        0x400

typedef unsigned char      uint8;
typedef   signed int       sint32;

extern sint32 GRZip_GetAdaptativeBlockSize(uint8 * Input,sint32 Size);

extern sint32 GRZip_StoreBlock(uint8 * Input ,sint32 Size,
                               uint8 * Output,sint32 Mode);

extern sint32 GRZip_CheckBlockSign(uint8 * Input,sint32 Size);

extern sint32 GRZip_CompressBlock(uint8 * Input ,sint32 Size,
                                  uint8 * Output,sint32 Mode);

extern sint32 GRZip_DecompressBlock(uint8 * Input,sint32 Size,uint8 * Output);

#ifdef __cplusplus
}
#endif

#endif
