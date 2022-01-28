/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor           Rec_Flt.c */
/* Data reordering and Delta Filter                */
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

#include <stdlib.h>
#include <math.h>

#include "libGRZip.h"

#define Rec_MaxByte                 256

sint32 GRZip_Rec_Test(uint8 * Input,sint32 Size)
{
  uint8 * SInput=Input;
  uint8 * InputEnd=Input+Size;

  sint32 Freq0[Rec_MaxByte];
  sint32 Freq2[2][Rec_MaxByte];
  sint32 Freq4[4][Rec_MaxByte];
  memset(Freq0,0,Rec_MaxByte*sizeof(Rec_MaxByte));
  memset(Freq2,0,2*Rec_MaxByte*sizeof(Rec_MaxByte));
  memset(Freq4,0,4*Rec_MaxByte*sizeof(Rec_MaxByte));

  sint32 j,i,Min,Pos2=0,Pos4=0;
  while (Input<InputEnd)
  {
    uint8 Char=*Input++;
    Freq0[Char]++;Freq2[Pos2][Char]++;Freq4[Pos4][Char]++;
    Pos2=(Pos2+1)&0x1; Pos4=(Pos4+1)&0x3;
  }

  double MinEntropy=0,Entropy; Min=0;

  for (i=0;i<Rec_MaxByte;i++)
  {
    sint32 Freq=Freq0[i];
    if (Freq) MinEntropy-=Freq*log10(((double)Freq)/Size);
  }

  MinEntropy*=0.93;

  Entropy=0;
  for (j=0;j<2;j++)
    for (i=0;i<Rec_MaxByte;i++)
    {
      sint32 Freq=Freq2[j][i];
      if (Freq) Entropy-=Freq*log10(((double)2*Freq)/Size);
    }

  if (Entropy<MinEntropy) {MinEntropy=0.95*Entropy;Min=1;};

  Entropy=0;
  for (j=0;j<4;j++)
    for (i=0;i<Rec_MaxByte;i++)
    {
      sint32 Freq=Freq4[j][i];
      if (Freq) Entropy-=Freq*log10(((double)4*Freq)/Size);
    }

  if (Entropy<MinEntropy) Min=2;

  if (Min==1)
  {
    sint64 Sum=0,SumDelta=0;
    uint16 MinCode=0xFFFF,Delta,PredCode=0;
    uint16 * Inp=(uint16 *)SInput;
    uint16 * InpEnd=Inp+(Size>>1);
    while (Inp<InpEnd)
    {
      uint16 Code=*Inp++;

      if (Code<MinCode) MinCode=Code;

      Sum+=Code; Delta=Code-PredCode; PredCode=Code;
      if (Delta&0x8000) Delta=((~Delta)<<1)|1; else Delta<<=1;
      SumDelta+=Delta;
    }
    if (Sum-MinCode*(Size>>1)>(SumDelta+(SumDelta>>4))) Min=3;
  }

  if (Min==2)
  {
    sint64 Sum=0,SumDelta=0;
    uint32 MinCode=0xFFFFFFFF,Delta,PredCode=0;
    uint32 * Inp=(uint32 *)SInput;
    uint32 * InpEnd=Inp+(Size>>2);
    while (Inp<InpEnd)
    {
      uint32 Code=*Inp++;

      if (Code<MinCode) MinCode=Code;

      Sum+=Code; Delta=Code-PredCode; PredCode=Code;

      if (Delta&0x80000000) Delta=((~Delta)<<1)|1; else Delta<<=1;

      SumDelta+=Delta;
    }
    if (Sum-MinCode*(Size>>2)>(SumDelta+(SumDelta>>4))) Min=4;
  }

  return Min;
}

void GRZip_Rec_Encode(uint8 * Input, sint32 Size,
                      uint8 * Output,sint32 Mode)
{
  if (Mode==3)
  {
    sint32 NumRecords=(Size>>1);
    uint16 Delta,PredCode=0;
    uint16 * Inp=(uint16 *)Input;
    uint16 * InpEnd=Inp+NumRecords;
    while (Inp<InpEnd)
    {
      uint16 Code=*Inp++;
      Delta=Code-PredCode; PredCode=Code;
      if (Delta&0x8000) Delta=((~Delta)<<1)|1; else Delta<<=1;
      *(Output+NumRecords)=Delta&0xFF;
      *(Output++)=Delta>>8;
    }
    sint32 i=2*NumRecords;
    while (i<Size)
    {
      *(Output+NumRecords)=Input[i];
      i++;Output++;
    }
  }

  if (Mode==4)
  {
    sint32 NumRecords=(Size>>2);
    sint32 P1=NumRecords;
    sint32 P2=2*NumRecords;
    sint32 P3=3*NumRecords;
    uint32 Delta,PredCode=0;
    uint32 * Inp=(uint32 *)Input;
    uint32 * InpEnd=Inp+NumRecords;
    while (Inp<InpEnd)
    {
      uint32 Code=*Inp++;
      Delta=Code-PredCode; PredCode=Code;
      if (Delta&0x80000000) Delta=((~Delta)<<1)|1; else Delta<<=1;
      *(Output+P1)=Delta&0xFF;Delta>>=8;
      *(Output+P2)=Delta&0xFF;Delta>>=8;
      *(Output+P3)=Delta&0xFF;*(Output++)=Delta>>8;
    }
    sint32 i=4*NumRecords;
    while (i<Size)
    {
      *(Output+P3)=Input[i];
      i++;Output++;
    }
  }

  if (Mode==1)
  {
    sint32 i;
    for(i=0;i<Size;i+=2) *(Output++)=Input[i];
    for(i=1;i<Size;i+=2) *(Output++)=Input[i];
  }

  if (Mode==2)
  {
    sint32 i;
    for(i=0;i<Size;i+=4) *(Output++)=Input[i];
    for(i=1;i<Size;i+=4) *(Output++)=Input[i];
    for(i=2;i<Size;i+=4) *(Output++)=Input[i];
    for(i=3;i<Size;i+=4) *(Output++)=Input[i];
  }

}

void GRZip_Rec_Decode(uint8 * Input, sint32 Size,
                      uint8 * Output,sint32 Mode)
{
  if (Mode==3)
  {
    sint32 NumRecords=(Size>>1);
    uint16 Code,PredCode=0;
    uint16 * Outp=(uint16 *)Output;
    uint16 * OutpEnd=Outp+NumRecords;
    while (Outp<OutpEnd)
    {
      uint16 Delta=*Input;
      Delta=(Delta<<8)|(*(Input+NumRecords));Input++;
      if (Delta&1) Delta=~(Delta>>1); else Delta>>=1;
      Code=Delta+PredCode; PredCode=Code;
      *Outp++=Code;
    }

    sint32 i=2*NumRecords;
    while (i<Size)
    {
      Output[i]=*(Input+NumRecords);
      i++;Input++;
    }
  }

  if (Mode==4)
  {
    sint32 NumRecords=(Size>>2);
    sint32 P1=NumRecords;
    sint32 P2=2*NumRecords;
    sint32 P3=3*NumRecords;
    uint32 Code,PredCode=0;
    uint32 * Outp=(uint32 *)Output;
    uint32 * OutpEnd=Outp+NumRecords;
    while (Outp<OutpEnd)
    {
      uint32 Delta=*Input;
      Delta=(Delta<<8)|(*(Input+P3));
      Delta=(Delta<<8)|(*(Input+P2));
      Delta=(Delta<<8)|(*(Input+P1)); Input++;
      if (Delta&1) Delta=~(Delta>>1); else Delta>>=1;
      Code=Delta+PredCode; PredCode=Code;
      *Outp++=Code;
    }

    sint32 i=4*NumRecords;
    while (i<Size)
    {
      Output[i]=*(Input+P3);
      i++;Input++;
    }
  }

  if (Mode==1)
  {
    sint32 i;
    for(i=0;i<Size;i+=2) Output[i]=*(Input++);
    for(i=1;i<Size;i+=2) Output[i]=*(Input++);
  }

  if (Mode==2)
  {
    sint32 i;
    for(i=0;i<Size;i+=4) Output[i]=*(Input++);
    for(i=1;i<Size;i+=4) Output[i]=*(Input++);
    for(i=2;i<Size;i+=4) Output[i]=*(Input++);
    for(i=3;i<Size;i+=4) Output[i]=*(Input++);
  }

}

/*-------------------------------------------------*/
/* End                                 Rec_Flt.c.c */
/*-------------------------------------------------*/
