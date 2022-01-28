/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor               ST4.c */
/* ST Order-4 Sorting Functions                    */
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

  ST memory use         : 5*BlockLen     + 256Kb
  Reverse ST memory use : 5.125*BlockLen + 256Kb

  For more information on these sources, see the manual.
--*/

#include <stdio.h>
#include "libGRZip.h"

#define ST_MaxByte           256
#define ST_MaxWord           65536
#define ST_INDIRECT          0x800000

sint32 GRZip_ST4_Encode(uint8 * Input, sint32 Size, uint8 * Output)
{
  sint32    FBP,i;
  uint32    W;

  sint32 *  Counter=(sint32 *)malloc(ST_MaxWord*sizeof(sint32));
  if (Counter==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  uint32 *  Context=(uint32 *)malloc(Size*sizeof(uint32));
  if (Context==NULL) {free(Counter);return (GRZ_NOT_ENOUGH_MEMORY);}

  memset    (Counter,0,ST_MaxWord*sizeof(sint32));

  W=Input[Size-1]<<8; for (i=0;i<Size;i++) Counter[(W=(W>>8)|(Input[i]<<8))]++;

  for (W=0,i=0;i<ST_MaxWord;i++) W+=Counter[i],Counter[i]=W-Counter[i];

  W=(Input[Size-4]<<8)|Input[Size-5];
  if (W==0xFFFF) FBP=Size-1; else FBP=Counter[W+1]-1;

  W=(Input[Size-1]<<24)|(Input[Size-2]<<16)|(Input[Size-3]<<8)|(Input[Size-4]);

  for (i=0;i<Size;i++)
  {
    uint8 c=Input[i];
    Context[Counter[W&0x0000FFFF]++]=(W&0xFFFF0000)|c;
    W=(W>>8)|(c<<24);
  }

  for (i=Size-1;i>=FBP;i--) Output[--Counter[Context[i]>>16]]=Context[i]&0xFF;
  FBP=Counter[Context[FBP]>>16];
  for (;i>=0;i--) Output[--Counter[Context[i]>>16]]=Context[i]&0xFF;

  free(Context);
  free(Counter);

  return FBP;
}

#define ST_SetBit(Bit) (Flag[Bit>>3]|=(1<<(Bit & 7)))
#define ST_GetBit(Bit) (Flag[Bit>>3]&(1<<(Bit & 7)))

sint32 GRZip_ST4_Decode(uint8 * Input, sint32 Size, sint32 FBP)
{
  sint32    LastSeen[ST_MaxByte],T[ST_MaxByte],S[ST_MaxByte];
  sint32    CStart,Sum,i,j;

  sint32 *  Context2=(sint32 *)malloc(ST_MaxWord*sizeof(sint32));
  if (Context2==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  uint8  *  Flag=(uint8 *)malloc(((Size+8)>>3)*sizeof(uint8));
  if (Flag==NULL) {free(Context2);return (GRZ_NOT_ENOUGH_MEMORY);}

  uint32 *  Table=(uint32 *)malloc((Size+1)*sizeof(uint32));
  if (Table==NULL) {free(Flag);free(Context2);return (GRZ_NOT_ENOUGH_MEMORY);}

  memset    (Context2,0,ST_MaxWord*sizeof(sint32));
  memset    (Flag,0,((Size+8)>>3)*sizeof(uint8));

  memset    (T,0,ST_MaxByte*sizeof(sint32));
  memset    (LastSeen,0xFF,ST_MaxByte*sizeof(sint32));

  for (i=0;i<Size;i++) T[Input[i]]++;

  for (i=0,j=0,Sum=0;i<ST_MaxByte;i++)
    for (Sum+=T[i],T[i]=Sum-T[i];j<Sum;j++)
      Context2[(Input[j]<<8)|i]++;

  memcpy(S,T,ST_MaxByte*sizeof(sint32));

  for (i=0,j=0,Sum=0;i<ST_MaxWord;i++)
    for (CStart=Sum,Sum+=Context2[i];j<Sum;j++)
    {
      uint8 c=Input[j];
      if (LastSeen[c]!=CStart)
      {
        LastSeen[c]=CStart;
        ST_SetBit(T[c]);
      }
      T[c]++;
    }

  memset(LastSeen,0,ST_MaxByte*sizeof(sint32));

  for (CStart=0,i=0;i<Size;i++)
  {
    uint8 c=Input[i];
    if (ST_GetBit(i)) CStart=i;
    if (LastSeen[c]<=CStart)
    {
      Table[i]=S[c];
      LastSeen[c]=i+1;
    }
    else
      Table[i]=(LastSeen[c]-1)|ST_INDIRECT;
    S[c]++;
    Table[i]|=(c<<24);
  }
  Table[Size]=ST_INDIRECT;

  free(Context2);
  free(Flag);

  for (j=FBP,Sum=Table[FBP],i=0;i<Size;i++)
    if (Sum&ST_INDIRECT)
    {
      j=((Table[Sum&(ST_INDIRECT-1)]++)&(ST_INDIRECT-1));
      Sum=Table[j];
      Input[i]=Sum>>24;
    }
    else
    {
      Table[j]++; Sum=Table[j=Sum&(ST_INDIRECT-1)];
      Input[i]=Sum>>24;
    }
  free(Table);
  return GRZ_NO_ERROR;
}

#undef ST_MaxByte
#undef ST_MaxWord
#undef ST_INDIRECT
#undef ST_SetBit
#undef ST_GetBit

/*-------------------------------------------------*/
/* End                                       ST4.c */
/*-------------------------------------------------*/
