/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor               LZP.c */
/* LZP Preprocessing Functions                     */
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
  grib@crazy.ru

  This program is based on (at least) the work of:
  Juergen Abel, Jon L. Bentley, Edgar Binder,
  Charles Bloom, Mike Burrows, Andrey Cadach,
  Damien Debin, Sebastian Deorowicz, Peter Fenwick,
  Michael Schindler, Robert Sedgewick, Julian Seward,
  David Wheeler, Vadim Yoockin.

  LZP memory use         : 2*BlockLen + [16-1024] Kb
  Reverse LZP memory use : 2*BlockLen + [16-1024] Kb

  For more information on these sources, see the manual.
--*/

#include <stdio.h>
#include "libGRZip.h"

#define LZP_MatchFlag    0xF2
#define LZP_RunFlag      0xF3
#define LZP_XorFlag      (uint8)(0xFF^LZP_RunFlag)

#define LZP_AllocHashTable()                                          \
  uint8 ** Contexts=(uint8 **)malloc((LZP_HT_Size+1)*sizeof(uint8 *));\
  if (Contexts==NULL) return (GRZ_NOT_ENOUGH_MEMORY);                 \
  memset(Contexts,0,(LZP_HT_Size+1)*sizeof(uint8 *));

#define LZP_FreeHashTable() free(Contexts);

sint32 GRZip_LZP_Encode(uint8 * Input,uint32 Size,uint8 * Output,uint8 Mode)
{
  uint32 LZP_HT_Size=(1<<(3+(Mode&0xF)))-1;
  uint32 LZP_MinMatchLen=2+3*(Mode>>4);

  LZP_AllocHashTable();

  uint8  * InputEnd=Input+Size;
  uint8  * OutputEnd=Output+Size-1;

  *((uint32 *)Output)=*((uint32 *)Input);

  uint32 Ctx=(Input[3]+(Input[2]<<8)+(Input[1]<<16)+(Input[0]<<24));

  Input+=4;
  Output+=4;

  while ((Input<InputEnd)&&(Output<OutputEnd))
  {
    uint32  HashIndex=((Ctx>>15)^Ctx^(Ctx>>3))&LZP_HT_Size;
    uint8 * Pointer=Contexts[HashIndex];
    Contexts[HashIndex]=Input;

    if (Pointer)
     {
       uint32  CommonLength=0;
       uint8 * Ptr=Input;
       while (Ptr<InputEnd)
       {
         if (*Ptr++!=*Pointer++) break;
         CommonLength++;
       }
       if (CommonLength<LZP_MinMatchLen) CommonLength=0;
       if (CommonLength)
        {
          Input+=CommonLength;
          Ctx=(Input[-1]+(Input[-2]<<8)+(Input[-3]<<16)+(Input[-4]<<24));
          CommonLength=CommonLength-LZP_MinMatchLen+1;
          (*Output++)=LZP_MatchFlag;
          while (CommonLength>254)
          {
            (*Output++)=LZP_RunFlag;
            if (Output>=OutputEnd)
             {
               LZP_FreeHashTable();
               return (GRZ_NOT_COMPRESSIBLE);
             }
            CommonLength-=255;
          }
          (*Output++)=((uint8)(CommonLength^LZP_XorFlag));
        }
       else
        {
          uint8 Ch=((*Output++)=(*Input++));
          Ctx=(Ctx<<8)|Ch;
          if (Ch==LZP_MatchFlag) *Output++=LZP_XorFlag;
        }
     }
    else
      Ctx=(Ctx<<8)|((*Output++)=(*Input++));
  }
  LZP_FreeHashTable();
  if (Output>=OutputEnd) return (GRZ_NOT_COMPRESSIBLE);
  return (Output+Size-OutputEnd-1);
}

sint32 GRZip_LZP_Decode(uint8 * Input,uint32 Size,uint8 * Output,uint8 Mode)
{
  uint32 LZP_HT_Size=(1<<(3+(Mode&0xF)))-1;
  uint32 LZP_MinMatchLen=2+3*(Mode>>4);

  LZP_AllocHashTable();
  uint8  * InputEnd=Input+Size;
  uint8  * OutputBeg=Output;

  *((uint32 *)Output)=*((uint32 *)Input);
  uint32 Ctx=(Input[3]+(Input[2]<<8)+(Input[1]<<16)+(Input[0]<<24));

  Input+=4;
  Output+=4;

  while (Input<InputEnd)
  {
    uint32  HashIndex=((Ctx>>15)^Ctx^(Ctx>>3))&LZP_HT_Size;
    uint8 * Pointer=Contexts[HashIndex];

    Contexts[HashIndex]=Output;

    if (Pointer)
     {
       if ((*Input++)!=LZP_MatchFlag) Ctx=(Ctx<<8)|(*Output++=*(Input-1));
       else
       {
         uint32 CommonLength=0;
         while (CommonLength+=((*Input)^LZP_XorFlag),(*Input++)==LZP_RunFlag);
         if (CommonLength)
          {
            CommonLength=CommonLength+LZP_MinMatchLen-1;
            while (CommonLength--) *Output++=*Pointer++;
            Ctx=(Output[-1]+(Output[-2]<<8)+(Output[-3]<<16)+(Output[-4]<<24));
          }
         else
          Ctx=(Ctx<<8)|(*Output++=LZP_MatchFlag);
       }
     }
    else
      Ctx=(Ctx<<8)|((*Output++)=(*Input++));
  }
  LZP_FreeHashTable();
  return (Output-OutputBeg);
}

#undef LZP_MatchFlag
#undef LZP_RunFlag
#undef LZP_XorFlag
#undef LZP_InitHashTables
#undef LZP_FreeHashTables

/*-------------------------------------------------*/
/* End                                       LZP.c */
/*-------------------------------------------------*/
