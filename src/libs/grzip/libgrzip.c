/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor          libGRZip.c */
/* libGRZip Compression(Decompression) Functions   */
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

  Normal compression mode:
    Compression     memory use : [7-9]*BlockLen  + 1Mb
    Decompression   memory use : 5*BlockLen      + 1Mb
  Fast compression mode:
    Compression     memory use : 5*BlockLen      + 1Mb
    Decompression   memory use : 5.125*BlockLen  + 1Mb

  For more information on these sources, see the manual.
--*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libGRZip.h"
#include "CRC32.c"
#include "BWT.c"
#include "LZP.c"
#include "ST4.c"
#include "MTF_Ari.c"
#include "WFC_Ari.c"
#include "Rec_Flt.c"

sint32 GRZip_StoreBlock(uint8 * Input ,sint32 Size,
                        uint8 * Output,sint32 Mode)
{
  *(sint32 *)(Output+4)=-1;
  *(sint32 *)(Output+8)=Mode&0xFF;
  *(sint32 *)(Output+12)=0;
  *(sint32 *)(Output+16)=Size;
  memcpy(Output+28,Input,Size);
  *(sint32 *)(Output+20)=GRZip_GetCRC32(Output+28,Size);
  *(sint32 *)(Output+24)=GRZip_GetCRC32(Output,24);
  return (Size+28);
}

sint32 GRZip_CompressBlock(uint8 * Input ,sint32 Size,
                           uint8 * Output,sint32 Mode)
{
  sint32 SSize=Size;

  *(sint32 *)Output=Size;

  if ((Size<32)||(Size>GRZ_MaxBlockSize))
    return(GRZip_StoreBlock(Input,Size,Output,0));

  if (Size<1024) Mode|=GRZ_Compression_ST4;

  if ((Size>1024)&&((Mode&GRZ_Disable_DeltaFlt)==0))
  {
    sint32 RecMode=GRZip_Rec_Test((uint8 *)Input,Size);
    if (RecMode)
    {
      sint32 NewSize=0;
      uint8 * Buffer=(uint8 *)malloc(Size+1024);
      if (Buffer==NULL) return(GRZip_StoreBlock(Input,Size,Output,0));
      GRZip_Rec_Encode(Input,Size,Buffer,RecMode); Mode+=GRZ_Disable_DeltaFlt;
      if ((RecMode&1)==1)
      {
        sint32 PartSize=(Size>>1);
        sint32 Result=GRZip_CompressBlock(Buffer,PartSize,Output+28,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize=Result;
        Result=GRZip_CompressBlock(Buffer+PartSize,Size-PartSize,Output+28+NewSize,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize+=Result;
      }
      if ((RecMode&1)==0)
      {
        sint32 PartSize=(Size>>2);
        sint32 Result=GRZip_CompressBlock(Buffer,PartSize,Output+28,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize=Result;
        Result=GRZip_CompressBlock(Buffer+PartSize,PartSize,Output+28+NewSize,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize+=Result;
        Result=GRZip_CompressBlock(Buffer+2*PartSize,PartSize,Output+28+NewSize,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize+=Result;
        Result=GRZip_CompressBlock(Buffer+3*PartSize,Size-3*PartSize,Output+28+NewSize,Mode);
        if (Result<0) {free(Buffer);return(GRZip_StoreBlock(Input,Size,Output,0));}
        NewSize+=Result;
      }
      free(Buffer);

      if (NewSize>=Size) return(GRZip_StoreBlock(Input,Size,Output,0));

      *(sint32 *)(Output+4)=-2;
      *(sint32 *)(Output+8)=RecMode;
      *(sint32 *)(Output+16)=NewSize;
      *(sint32 *)(Output+20)=GRZip_GetCRC32(Output+28,NewSize);
      *(sint32 *)(Output+24)=GRZip_GetCRC32(Output,24);

      return (NewSize+28);
    }
  }

  uint8 * LZPBuffer=(uint8 *)malloc(Size+1024);
  if (LZPBuffer==NULL) return(GRZip_StoreBlock(Input,Size,Output,0));

  if (Mode&0x0F)
  {
    sint32 Result=GRZip_LZP_Encode(Input,Size,LZPBuffer,Mode&0xFF);
    if (Result==GRZ_NOT_ENOUGH_MEMORY)
    {
      free(LZPBuffer);
      return(GRZip_StoreBlock(Input,Size,Output,0));
    };
    if (Result==GRZ_NOT_COMPRESSIBLE)
    {
      Mode=Mode&0xFFFFFF00;
      memcpy(LZPBuffer,Input,Size);
      *(sint32 *)(Output+8)=Size;
    }
    else
     { *(sint32 *)(Output+8)=Result,Size=Result;}
  }
  else
  {
    memcpy(LZPBuffer,Input,Size);
    *(sint32 *)(Output+8)=Size;
  }
  sint32 Result;

  for (Result=0;Result<8;Result++) LZPBuffer[Result+Size]=0;
  Size=(Size+7)&(~7);

  if (Mode&GRZ_Compression_ST4)
    Result=GRZip_ST4_Encode(LZPBuffer,Size,LZPBuffer);
  else
    Result=GRZip_BWT_Encode(LZPBuffer,Size,LZPBuffer,Mode&GRZ_BWTSorting_Fast);

  if (Result==GRZ_NOT_ENOUGH_MEMORY)
  {
    if (Mode&0x0F)
    {
      sint32 Result=GRZip_LZP_Encode(Input,SSize,LZPBuffer,Mode&0xFF);
      if (Result==GRZ_NOT_ENOUGH_MEMORY)
      {
        free(LZPBuffer);
        return(GRZip_StoreBlock(Input,SSize,Output,0));
      };
      Result=GRZip_StoreBlock(LZPBuffer,Result,Output,Mode);
      free(LZPBuffer);
      return (Result);
    }
    free(LZPBuffer);
    return(GRZip_StoreBlock(Input,SSize,Output,0));
  };

  *(sint32 *)(Output+12)=Result;

  if (Mode&GRZ_Compression_MTF)
    Result=GRZip_MTF_Ari_Encode(LZPBuffer,Size,Output+28);
  else
    Result=GRZip_WFC_Ari_Encode(LZPBuffer,Size,Output+28);

  if ((Result==GRZ_NOT_ENOUGH_MEMORY)||(Result==GRZ_NOT_COMPRESSIBLE))
  {
    if (Mode&0x0F)
    {
      sint32 Result=GRZip_LZP_Encode(Input,SSize,LZPBuffer,Mode&0xFF);
      if (Result==GRZ_NOT_ENOUGH_MEMORY)
      {
        free(LZPBuffer);
        return(GRZip_StoreBlock(Input,SSize,Output,0));
      };
      Result=GRZip_StoreBlock(LZPBuffer,Result,Output,Mode);
      free(LZPBuffer);
      return (Result);
    }
    free(LZPBuffer);
    return(GRZip_StoreBlock(Input,SSize,Output,0));
  };

  *(sint32 *)(Output+4)=Mode;
  *(sint32 *)(Output+16)=Result;
  *(sint32 *)(Output+20)=GRZip_GetCRC32(Output+28,Result);
  *(sint32 *)(Output+24)=GRZip_GetCRC32(Output,24);

  free(LZPBuffer);
  return (Result+28);
}

sint32 GRZip_CheckBlockSign(uint8 * Input,sint32 Size)
{
  if (Size<28) return (GRZ_UNEXPECTED_EOF);
  if ((*(sint32 *)(Input+24))!=GRZip_GetCRC32(Input,24))
    return (GRZ_CRC_ERROR);
  return (GRZ_NO_ERROR);
}

sint32 GRZip_DecompressBlock(uint8 * Input,sint32 Size,uint8 * Output)
{
  if (Size<28) return (GRZ_UNEXPECTED_EOF);
  if ((*(sint32 *)(Input+24))!=GRZip_GetCRC32(Input,24))
    return (GRZ_CRC_ERROR);
  if ((*(sint32 *)(Input+16))+28>Size) return (GRZ_UNEXPECTED_EOF);
  if ((*(sint32 *)(Input+20))!=GRZip_GetCRC32(Input+28,*(sint32 *)(Input+16)))
    return (GRZ_CRC_ERROR);
  sint32 Mode=*(sint32 *)(Input+4);
  sint32 Result=*(sint32 *)(Input+16);
  if (Mode==-1)
  {
    Mode=*(sint32 *)(Input+8);
    if (Mode==0)
    {
      memcpy(Output,Input+28,Result);
      return (Result);
    }
    Result=GRZip_LZP_Decode(Input+28,Result,Output,Mode&0xFF);
    return (Result);
  }

  if (Mode==-2)
  {
    sint32 RecMode=*(sint32 *)(Input+8);
              Size=*(sint32 *)(Input);

    uint8 * Buffer=(uint8 *)malloc(Size+1024);
    if (Buffer==NULL) return(GRZ_NOT_ENOUGH_MEMORY);

    uint8 * Tmp=(Input+28);
    sint32  OutputPos=0;

    if ((RecMode&1)==1)
    {
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
      OutputPos+=Result;
      Tmp=Tmp+(*(sint32 *)(Tmp+16))+28;
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
    }
    if ((RecMode&1)==0)
    {
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
      OutputPos+=Result;
      Tmp=Tmp+(*(sint32 *)(Tmp+16))+28;
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
      OutputPos+=Result;
      Tmp=Tmp+(*(sint32 *)(Tmp+16))+28;
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
      OutputPos+=Result;
      Tmp=Tmp+(*(sint32 *)(Tmp+16))+28;
      Result=GRZip_DecompressBlock(Tmp,(*(sint32 *)(Tmp+16))+28,Buffer+OutputPos);
      if (Result<0) {free(Buffer);return Result;};
    }
    GRZip_Rec_Decode(Buffer,Size,Output,RecMode);
    free(Buffer);
    return (Size);
  }

  uint8 * LZPBuffer=(uint8 *)malloc(*(sint32 *)(Input+8)+1024);
  if (LZPBuffer==NULL) return(GRZ_NOT_ENOUGH_MEMORY);

  sint32 TSize;

  if (Mode&GRZ_Compression_MTF)
    TSize=GRZip_MTF_Ari_Decode(Input+28,LZPBuffer);
  else
    TSize=GRZip_WFC_Ari_Decode(Input+28,(*(sint32 *)(Input+8)),LZPBuffer);

  if (Result==GRZ_NOT_ENOUGH_MEMORY)
  {
    free(LZPBuffer);
    return(GRZ_NOT_ENOUGH_MEMORY);
  };

  Result=*(sint32 *)(Input+12);

  if (Mode&GRZ_Compression_ST4)
    Result=GRZip_ST4_Decode(LZPBuffer,TSize,Result);
  else
    Result=GRZip_BWT_Decode(LZPBuffer,TSize,Result);

  if (Result==GRZ_NOT_ENOUGH_MEMORY)
  {
    free(LZPBuffer);
    return(GRZ_NOT_ENOUGH_MEMORY);
  };

  TSize=*(sint32 *)(Input+8);

  if (Mode&0x0F)
  {
    sint32 Result=GRZip_LZP_Decode(LZPBuffer,TSize,Output,Mode&0xFF);
    if (Result==GRZ_NOT_ENOUGH_MEMORY)
    {
      free(LZPBuffer);
      return(GRZ_NOT_ENOUGH_MEMORY);
    };
  }
  else
    memcpy(Output,LZPBuffer,TSize);

  free(LZPBuffer);
  return (*(sint32 *)Input);
}

#define ABS_MaxByte      256
#define ABS_MinBlockSize 24*1024

sint32 GRZip_GetAdaptativeBlockSize(uint8 * Input,sint32 Size)
{
  sint32  TotFreq[ABS_MaxByte];
  sint32     Freq[ABS_MaxByte];

  if (Size<=ABS_MinBlockSize) return Size;

  memset(TotFreq,0,ABS_MaxByte*sizeof(sint32));

  uint8 * SInput=Input;
  uint8 * InputEnd=Input+ABS_MinBlockSize;
  while  (Input<InputEnd) TotFreq[*Input++]++;

  sint32 Pos=ABS_MinBlockSize,BlockSize=ABS_MinBlockSize/2;

  while (Pos+BlockSize<Size)
  {
    memset(Freq,0,ABS_MaxByte*sizeof(sint32));

    sint32 i=0,Sum=BlockSize+(Pos>>1);

    uint8 * Ptr=SInput+Pos;
    uint8 * PtrEnd=Ptr+BlockSize;
    while (Ptr<PtrEnd) Freq[*Ptr++]++;

    double AvgSize=0,RealSize=0;
    for (i=0;i<ABS_MaxByte;i++)
      if (Freq[i])
      {
        sint32 Fr=Freq[i];
        RealSize-=Fr*log10((double)Fr/BlockSize);
         AvgSize-=Fr*log10((double)(Fr+(TotFreq[i]>>1))/Sum);
      }

    if (AvgSize>1.25*RealSize)
    {
       if (BlockSize<256)
         return Pos;
       else
         {BlockSize>>=1;continue;}
    }

    for (i=0;i<ABS_MaxByte;i++) TotFreq[i]+=Freq[i];
    Pos+=BlockSize;
  }
  return Size;
}

#undef ABS_MaxByte
#undef ABS_MinBlockSize

/*-------------------------------------------------*/
/* End                                  libgrzip.c */
/*-------------------------------------------------*/
