/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor           WFC_Ari.c */
/* RLE + WFC + Entropy Coder Functions             */
/*-------------------------------------------------*/

/*--
  This file is a part of GRZipII and/or libGRZip, a program
  and library for lossless, block-sorting data compression.

  Copyright (C) 2002-2003 Grebnov Ilya. All rights reserved.

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
  Michael Schindler, Robert Sedgewick, Julian Seward,
  David Wheeler, Vadim Yoockin.

  For more information on these sources, see the manual.
--*/

#include <stdlib.h>
#include "libGRZip.h"
#include "WFC_MTF.h"

#define ARI_MaxByte            256
#define WFC_MaxByte            256

#define WFC_Val0               131072
#define WFC_Val1               114688
#define WFC_Val2               7272
#define WFC_Val3               4240
#define WFC_Val4               2364
#define WFC_Val5               1263
#define WFC_Val6               649
#define WFC_Val7               320
#define WFC_Val8               153
#define WFC_Val9               70
#define WFC_Val10              31
#define WFC_Val11              14
#define WFC_Val12              8

#define WFC_Pos1               1
#define WFC_Pos2               2
#define WFC_Pos3               4
#define WFC_Pos4               8
#define WFC_Pos5               16
#define WFC_Pos6               32
#define WFC_Pos7               64
#define WFC_Pos8               128
#define WFC_Pos9               256
#define WFC_Pos10              512
#define WFC_Pos11              1024
#define WFC_Pos12              2048

#define WFC_Init()                                    \
  uint8 * WFCBuf=(uint8 *)malloc(Size);               \
  if (WFCBuf==NULL) return (GRZ_NOT_ENOUGH_MEMORY);   \
                                                      \
  sint32  WFC_List[WFC_MaxByte+1];                    \
  sint32  Char2Index[WFC_MaxByte+1];                  \
  sint32  CharWeight[WFC_MaxByte+1];                  \
                                                      \
  sint32  WFCBufPos=0;                                \
                                                      \
  for (i=0;i<=WFC_MaxByte;i++)                        \
  {                                                   \
    WFC_List[i]=Char2Index[i]=i;                      \
    CharWeight[i]=0;                                  \
  }                                                   \
  CharWeight[WFC_MaxByte]=-1;                         \

#define WFC_Finish()  free(WFCBuf);                   \

#define Update_Weight0(c)                             \
{                                                     \
  CharWeight[c]+=WFC_Val0;                            \
  sint32 j=Char2Index[c];                             \
  if (j>0) do {                                       \
    Char2Index[WFC_List[j]=WFC_List[j-1]]=j;          \
  } while (--j);                                      \
  WFC_List[0]=c;                                      \
  Char2Index[c]=0;                                    \
}

#define Update_Weight(c,Weight)                       \
{                                                     \
  sint32 W=(CharWeight[c]-=Weight);                   \
  if (c!=Char)                                        \
  {                                                   \
    sint32  j=Char2Index[c];                          \
    sint32* List=&WFC_List[j+1];                      \
    while (CharWeight[*List]>W)                       \
     {                                                \
       Char2Index[List[-1]=*List]=j++;                \
       List++;                                        \
     }                                                \
    WFC_List[j]=c;                                    \
    Char2Index[c]=j;                                  \
  }                                                   \
}

#define Update_Weight_Full(Weight,Pos)                \
  if (WFCBufPos>=Pos)                                 \
  {                                                   \
    uint8 c=WFCBuf[WFCBufPos-Pos];                    \
    Update_Weight(c,Weight);                          \
  }                                                   \

#define ARI_RangeTOP           (1<<24)

#define ARI_OutTgtByte(B)      (*Output++=B)

#define Model_NumBits          11
#define Model_MaxFreq          (1<<Model_NumBits)

#define M_Log2RLE_Shift_0      6
#define M_Log2RLE_Shift_1      3
#define M_Log2RLE_Shift_2      6

#define M_L1_Shift_0           4
#define M_L1_Shift_1           6

#define M_L2_Shift             7

#define ModelL0_0MaxFreq       58
#define ModelL0_1MaxFreq       62
#define ModelL0_2MaxFreq       204

#define ARI_ShiftLow()                              \
{                                                   \
  if ((Low^0xFF000000)>0xFFFFFF)                    \
  {                                                 \
    ARI_OutTgtByte((uint8)(Cache+(Low>>32)));       \
    sint32 c = (sint32)(0xFF+(Low>>32));            \
    while (FFNum) ARI_OutTgtByte(c), FFNum--;       \
    Cache = (uint32)(Low)>>24;                      \
  } else FFNum++;                                   \
  Low = (uint32)(Low)<<8;                           \
}

#define UpdateVCtx_1(P,NumShiftBits)                \
{                                                   \
  VCtx[P]-=(VCtx[P]>>NumShiftBits);                 \
}

#define UpdateVCtx_0(P,NumShiftBits)                \
{                                                   \
  VCtx[P]+=((Model_MaxFreq-VCtx[P])>>NumShiftBits); \
}

#define UpdateUCtx_1(P,NumShiftBits)                \
{                                                   \
  UCtx[P]-=(UCtx[P]>>NumShiftBits);                 \
}

#define UpdateUCtx_0(P,NumShiftBits)                \
{                                                   \
  UCtx[P]+=((Model_MaxFreq-UCtx[P])>>NumShiftBits); \
}

#define Update_Model_L0()                           \
{                                                   \
  if (Model_L0_0[4]>ModelL0_0MaxFreq)               \
  {                                                 \
    sint32 Sum=(Model_L0_0[0]=(Model_L0_0[0]+1)>>1);\
    Sum+=(Model_L0_0[1]=(Model_L0_0[1]+1)>>1);      \
    Sum+=(Model_L0_0[2]=(Model_L0_0[2]+1)>>1);      \
    Sum+=(Model_L0_0[3]=(Model_L0_0[3]+1)>>1);      \
    Model_L0_0[4]=Sum;                              \
  }                                                 \
  if (VCtx[4]>ModelL0_1MaxFreq)                     \
  {                                                 \
    sint32 Sum=(VCtx[0]>>=1);                       \
    Sum+=(VCtx[1]>>=1);Sum+=(VCtx[2]>>=1);          \
    Sum+=(VCtx[3]>>=1);VCtx[4]=Sum;                 \
  }                                                 \
  if (UCtx[4]>ModelL0_2MaxFreq)                     \
  {                                                 \
    sint32 Sum=(UCtx[0]>>=1);                       \
    Sum+=(UCtx[1]>>=1);Sum+=(UCtx[2]>>=1);          \
    Sum+=(UCtx[3]>>=1);UCtx[4]=Sum;                 \
  }                                                 \
}                                                   \

#define Init_Models()                               \
                                                    \
  uint32  Model_L0_0[5];                            \
  uint32  Model_L0_1[ARI_MaxByte][5];               \
  uint32  Model_L0_2[4*ARI_MaxByte][5];             \
                                                    \
  uint32  Model_L1_0[8];                            \
  uint32  Model_L1_1[8][8];                         \
                                                    \
  uint32  Model_L2_0[8][128];                       \
                                                    \
  uint32  Model_Log2RLE_0[64][GRZ_Log2MaxBlockSize+1];\
  uint32  Model_Log2RLE_2[GRZ_Log2MaxBlockSize+1][GRZ_Log2MaxBlockSize+1];\
                                                    \
  uint32* Model_Log2RLE_1;                          \
                                                    \
  sint32 i,j,CtxRLE=0,CtxL0=0,CtxL1=0,CtxL2;        \
                                                    \
  WFC_Init()                                        \
                                                    \
  Model_Log2RLE_1=(uint32*)malloc(ARI_MaxByte*(GRZ_Log2MaxBlockSize+1)*sizeof(uint32));\
  if (Model_Log2RLE_1==NULL) {WFC_Finish();return (GRZ_NOT_ENOUGH_MEMORY);}\
                                                    \
  Model_L0_0[0]=Model_L0_0[1]=1;                    \
  Model_L0_0[2]=Model_L0_0[3]=1;                    \
  Model_L0_0[4]=4;                                  \
                                                    \
  Model_L1_0[0]=Model_L1_0[1]=Model_MaxFreq>>1;     \
  Model_L1_0[2]=Model_L1_0[3]=Model_MaxFreq>>1;     \
  Model_L1_0[4]=Model_L1_0[5]=Model_MaxFreq>>1;     \
  Model_L1_0[6]=Model_L1_0[7]=Model_MaxFreq>>1;     \
                                                    \
  memset(Model_L0_1,0,5*ARI_MaxByte*sizeof(uint32));\
  memset(Model_L0_2,0,4*5*ARI_MaxByte*sizeof(uint32));\
                                                    \
  for (i=0;i<8;i++)                                 \
    for (j=0;j<128;j++)                             \
      Model_L2_0[i][j]=Model_MaxFreq>>1;            \
                                                    \
  for (i=0;i<7;i++)                                 \
  {                                                 \
    Model_L1_1[i][0]=Model_L1_1[i][1]=Model_MaxFreq>>1;\
    Model_L1_1[i][2]=Model_L1_1[i][3]=Model_MaxFreq>>1;\
    Model_L1_1[i][4]=Model_L1_1[i][5]=Model_MaxFreq>>1;\
    Model_L1_1[i][6]=Model_L1_1[i][7]=Model_MaxFreq>>1;\
  }                                                 \
                                                    \
  for (j=0;j<64;j++)                                \
    for (i=0;i<=GRZ_Log2MaxBlockSize;i++)           \
      Model_Log2RLE_0[j][i]=Model_MaxFreq>>1;       \
                                                    \
  for (j=0;j<=GRZ_Log2MaxBlockSize;j++)             \
    for (i=0;i<=GRZ_Log2MaxBlockSize;i++)           \
      Model_Log2RLE_2[j][i]=Model_MaxFreq>>1;       \
                                                    \
  uint32* VCtx;                                     \
  uint32* UCtx;                                     \
                                                    \
  for (UCtx=Model_Log2RLE_1,i=0;i<ARI_MaxByte;i++)  \
    for (j=0;j<=GRZ_Log2MaxBlockSize;j++)           \
      *UCtx++=Model_MaxFreq>>1;                     \

sint32 GRZip_WFC_Ari_Encode(uint8* Input,sint32 Size,uint8* Output)
{
  uint8 * InputEnd=Input+Size;
  uint8 * OutputEnd=Output+Size-24;

  sint64 Low=0;
  uint32 FFNum=0;
  uint32 Cache=0;
  uint32 Range=(uint32)(-1);

#define ARI_Encode(TFreq,TCumFreq,TTotFreq)                \
{                                                          \
  Low+=((sint32)TCumFreq)*(Range/=((sint32)TTotFreq));     \
  Range*=(sint32)TFreq;                                    \
  while(Range<ARI_RangeTOP) {ARI_ShiftLow();Range<<=8;}    \
}                                                          \

#define ARI_Encode_0(FFreq0)                               \
{                                                          \
  ARI_Encode(((sint32)FFreq0),0,Model_MaxFreq)             \
}                                                          \

#define ARI_Encode_1(FFreq0)                               \
{                                                          \
  sint32 FTFreq0=FFreq0;                                   \
  ARI_Encode(Model_MaxFreq-FTFreq0,FTFreq0,Model_MaxFreq)  \
}                                                          \

  Init_Models();

  uint32 Mask=0,Log2RunSize,RunSize,WFCMTF_Rank,PredChar,Char=0;

  while (Input<InputEnd)
  {

    if (Output>=OutputEnd)
    {
      WFC_Finish();
      free(Model_Log2RLE_1);
      return (GRZ_NOT_COMPRESSIBLE);
    }

    PredChar=Char; Char=*Input++;

    RunSize=1;
    while ((Input<InputEnd)&&(*Input==Char)) RunSize++,Input++;

    WFCBuf[WFCBufPos]=Char; WFCMTF_Rank=Char2Index[Char];
    Update_Weight0(Char);
    Update_Weight_Full(WFC_Val1,WFC_Pos1);
    Update_Weight_Full(WFC_Val2,WFC_Pos2);
    Update_Weight_Full(WFC_Val3,WFC_Pos3);
    Update_Weight_Full(WFC_Val4,WFC_Pos4);
    Update_Weight_Full(WFC_Val5,WFC_Pos5);
    Update_Weight_Full(WFC_Val6,WFC_Pos6);
    Update_Weight_Full(WFC_Val7,WFC_Pos7);
    Update_Weight_Full(WFC_Val8,WFC_Pos8);
    Update_Weight_Full(WFC_Val9,WFC_Pos9);
    Update_Weight_Full(WFC_Val10,WFC_Pos10);
    Update_Weight_Full(WFC_Val11,WFC_Pos11);
    Update_Weight_Full(WFC_Val12,WFC_Pos12);

    WFCBufPos++;

    WFCMTF_Rank=(WFCMTF_Rank-1)&0xFF;

    VCtx=&Model_L0_1[PredChar][0]; UCtx=&Model_L0_2[4*CtxL0+(CtxRLE&3)][0];

    if (WFCMTF_Rank<3)
     {
       uint32 Tmp,Cum=0;
       for (Tmp=0;Tmp<WFCMTF_Rank;Tmp++) Cum+=UCtx[Tmp]+VCtx[Tmp]+Model_L0_0[Tmp];
       ARI_Encode(Model_L0_0[WFCMTF_Rank]+UCtx[WFCMTF_Rank]+VCtx[WFCMTF_Rank],Cum,
                  Model_L0_0[4]+UCtx[4]+VCtx[4]);

       Model_L0_0[WFCMTF_Rank]+=2;UCtx[WFCMTF_Rank]+=2;VCtx[WFCMTF_Rank]+=2;
       Model_L0_0[4]+=2;UCtx[4]+=2;VCtx[4]+=2;
       Update_Model_L0();
     }
    else
     {
       uint32 Cum=Model_L0_0[4]+UCtx[4]+VCtx[4]-Model_L0_0[3]-UCtx[3]-VCtx[3];
       ARI_Encode(Model_L0_0[3]+UCtx[3]+VCtx[3],Cum,
                  Model_L0_0[4]+UCtx[4]+VCtx[4]);

       Model_L0_0[3]+=2;UCtx[3]+=2;VCtx[3]+=2;
       Model_L0_0[4]+=2;UCtx[4]+=2;VCtx[4]+=2;
       Update_Model_L0();

       sint32 Mask,GrNum,GrPos;

       GrNum=WFCMTF_Rank2GrNum[WFCMTF_Rank];
       GrPos=WFCMTF_Rank2GrPos[WFCMTF_Rank]; Mask=WFCMTF_Rank2Mask[WFCMTF_Rank];

       VCtx=Model_L1_0; UCtx=&Model_L1_1[CtxL1][0]; CtxL1=GrNum;

       for (i=0;i<GrNum;i++)
       {
         ARI_Encode_1((VCtx[0]+UCtx[0])>>1);
         UpdateVCtx_1(0,M_L1_Shift_0); UpdateUCtx_1(0,M_L1_Shift_1);
         VCtx++;UCtx++;
       }

       if (GrNum!=6)
       {
         ARI_Encode_0((VCtx[0]+UCtx[0])>>1);
         UpdateVCtx_0(0,M_L1_Shift_0); UpdateUCtx_0(0,M_L1_Shift_1);
       }

       VCtx=&Model_L2_0[GrNum][0];

       for (CtxL2=1,i=0;i<=GrNum;i++,Mask>>=1)
         if (GrPos&Mask)
          {
            ARI_Encode_1(VCtx[CtxL2]);
            UpdateVCtx_1(CtxL2,M_L2_Shift);
            CtxL2=(CtxL2<<1)|1;
          }
         else
          {
            ARI_Encode_0(VCtx[CtxL2]);
            UpdateVCtx_0(CtxL2,M_L2_Shift);
            CtxL2<<=1;
          }
     }

    if (WFCMTF_Rank>3) WFCMTF_Rank=3;
    CtxL0=((CtxL0<<2)|WFCMTF_Rank)&0xFF;

    sint32 Tmp=RunSize;

    if (Tmp==1) Log2RunSize=0;
    else
     {
       Log2RunSize=1;Mask=1;
       while (1)
       {
         if ((Tmp>>=1)==1) break;
         Log2RunSize++;Mask<<=1;
       }
     }

    VCtx=&Model_Log2RLE_0[CtxRLE+16*WFCMTF_Rank][0];
    UCtx=Model_Log2RLE_1+Char*(GRZ_Log2MaxBlockSize+1);

    for (i=0;i<Log2RunSize;i++)
    {
      ARI_Encode_1((VCtx[0]+UCtx[0])>>1);
      UpdateVCtx_1(0,M_Log2RLE_Shift_0); UpdateUCtx_1(0,M_Log2RLE_Shift_1);
      VCtx++;UCtx++;
    }

    ARI_Encode_0((VCtx[0]+UCtx[0])>>1);
    UpdateVCtx_0(0,M_Log2RLE_Shift_0); UpdateUCtx_0(0,M_Log2RLE_Shift_1);

    if (Log2RunSize<2)
      CtxRLE=(CtxRLE<<1)&0xF;
    else
      CtxRLE=((CtxRLE<<1)|1)&0xF;

    VCtx=&Model_Log2RLE_2[Log2RunSize][0];

    for (i=0;i<Log2RunSize;i++,Mask>>=1,VCtx++)
      if (RunSize&Mask)
      {
        ARI_Encode_1(VCtx[0]);
        UpdateVCtx_1(0,M_Log2RLE_Shift_2);
      }
      else
      {
        ARI_Encode_0(VCtx[0]);
        UpdateVCtx_0(0,M_Log2RLE_Shift_2);
      }
  }

  VCtx=&Model_L0_1[Char][0]; UCtx=&Model_L0_2[4*CtxL0+(CtxRLE&3)][0];
  uint32 Cum=Model_L0_0[4]+UCtx[4]+VCtx[4]-Model_L0_0[3]-UCtx[3]-VCtx[3];
  ARI_Encode(Model_L0_0[3]+UCtx[3]+VCtx[3],Cum,
             Model_L0_0[4]+UCtx[4]+VCtx[4]);

  Model_L0_0[3]+=2;UCtx[3]+=2;VCtx[3]+=2;
  Model_L0_0[4]+=2;UCtx[4]+=2;VCtx[4]+=2;
  Update_Model_L0();

  VCtx=Model_L1_0; UCtx=&Model_L1_1[CtxL1][0];

  for (i=0;i<6;i++)
  {
    ARI_Encode_1((VCtx[0]+UCtx[0])>>1);
    UpdateVCtx_1(0,M_L1_Shift_0); UpdateUCtx_1(0,M_L1_Shift_1);
    VCtx++;UCtx++;
  }

  VCtx=&Model_L2_0[6][0];

  for (Mask=64,CtxL2=1,i=0;i<=6;i++,Mask>>=1)
  {
    ARI_Encode_1(VCtx[CtxL2]);
    UpdateVCtx_1(CtxL2,M_L2_Shift);
    CtxL2=(CtxL2<<1)|1;
  }
  WFC_Finish();
  free(Model_Log2RLE_1);
  Low+=(Range>>=1); ARI_ShiftLow(); ARI_ShiftLow();
  ARI_ShiftLow(); ARI_ShiftLow(); ARI_ShiftLow();
  return (Output+Size-OutputEnd-24);
}

#undef ARI_OutTgtByte
#undef ARI_ShiftLow
#undef ARI_Encode
#undef ARI_Encode_0
#undef ARI_Encode_1


#define ARI_InTgtByte()      (*Input++)
#define ARI_GetFreq(TotFreq) (ARICode/(Range/=(TotFreq)))

sint32 GRZip_WFC_Ari_Decode(uint8* Input,uint32 Size,uint8* Output)
{
  uint8* SOutput=Output;
  uint32 ARICode=0;
  uint32 Range=(uint32)(-1);

#define ARI_Decode(TFreq,TCumFreq,TTotFreq)                \
{                                                          \
  ARICode-=((uint32)TCumFreq)*Range;                       \
  Range*=((uint32)TFreq);                                  \
  while(Range<ARI_RangeTOP)                                \
    {ARICode=(ARICode<<8)|ARI_InTgtByte();Range<<=8;}      \
}                                                          \

#define ARI_Decode_0(FFreq0)                               \
{                                                          \
  uint32 FAFreq=FFreq0;                                    \
  ARI_Decode(FAFreq,0,Model_MaxFreq);                      \
}                                                          \

#define ARI_Decode_1(FFreq0)                               \
{                                                          \
  uint32 FAFreq=FFreq0;                                    \
  ARI_Decode(Model_MaxFreq-FAFreq,FAFreq,Model_MaxFreq);   \
}                                                          \

  ARICode|=ARI_InTgtByte();ARICode<<=8;
  ARICode|=ARI_InTgtByte();ARICode<<=8;
  ARICode|=ARI_InTgtByte();ARICode<<=8;
  ARICode|=ARI_InTgtByte();ARICode<<=8;
  ARICode|=ARI_InTgtByte();

  Init_Models();

  uint32 Log2RunSize,RunSize,WFCMTF_Rank,PredChar,Char=0;

  while (1)
  {
    PredChar=Char;

    VCtx=&Model_L0_1[PredChar][0]; UCtx=&Model_L0_2[4*CtxL0+(CtxRLE&3)][0];
    uint32 Cum=0,Frq=ARI_GetFreq(Model_L0_0[4]+UCtx[4]+VCtx[4]);

    WFCMTF_Rank=0;while (Frq>=Cum) {Cum+=(Model_L0_0[WFCMTF_Rank]+UCtx[WFCMTF_Rank]+VCtx[WFCMTF_Rank]);WFCMTF_Rank++;}
    WFCMTF_Rank--; Cum-=(Model_L0_0[WFCMTF_Rank]+UCtx[WFCMTF_Rank]+VCtx[WFCMTF_Rank]);

    ARI_Decode(Model_L0_0[WFCMTF_Rank]+UCtx[WFCMTF_Rank]+VCtx[WFCMTF_Rank],Cum,
               Model_L0_0[4]+UCtx[4]+VCtx[4]);

    Model_L0_0[WFCMTF_Rank]+=2;UCtx[WFCMTF_Rank]+=2;VCtx[WFCMTF_Rank]+=2;
    Model_L0_0[4]+=2;UCtx[4]+=2;VCtx[4]+=2;
    Update_Model_L0();

    if (WFCMTF_Rank==3)
    {

      VCtx=Model_L1_0; UCtx=&Model_L1_1[CtxL1][0];

      sint32 GrNum,GrPos=0;

      GrNum=0;

      while (GrNum!=6)
      {
        if (ARI_GetFreq(Model_MaxFreq)<((VCtx[0]+UCtx[0])>>1))
         {
           ARI_Decode_0((VCtx[0]+UCtx[0])>>1);
           UpdateVCtx_0(0,M_L1_Shift_0); UpdateUCtx_0(0,M_L1_Shift_1);
           break;
         }
        ARI_Decode_1((VCtx[0]+UCtx[0])>>1);
        UpdateVCtx_1(0,M_L1_Shift_0); UpdateUCtx_1(0,M_L1_Shift_1);
        VCtx++;UCtx++;
        GrNum++;
      }

      CtxL1=GrNum;

      VCtx=&Model_L2_0[GrNum][0];

      for (CtxL2=1,i=0;i<=GrNum;i++)
        if (ARI_GetFreq(Model_MaxFreq)<VCtx[CtxL2])
         {
           ARI_Decode_0(VCtx[CtxL2]);
           UpdateVCtx_0(CtxL2,M_L2_Shift);
           CtxL2<<=1;GrPos<<=1;
         }
        else
         {
           ARI_Decode_1(VCtx[CtxL2]);
           UpdateVCtx_1(CtxL2,M_L2_Shift);
           CtxL2=(CtxL2<<1)|1;GrPos=(GrPos<<1)|1;
         }
      if ((GrNum==6)&&(GrPos==127)) break;

      WFCMTF_Rank=WFCMTF_GrNum2GrBegin[GrNum]+GrPos;

    }

    WFCMTF_Rank=(WFCMTF_Rank+1)&0xFF;

    Char=WFC_List[WFCMTF_Rank];WFCBuf[WFCBufPos]=Char;

    Update_Weight0(Char);
    Update_Weight_Full(WFC_Val1,WFC_Pos1);
    Update_Weight_Full(WFC_Val2,WFC_Pos2);
    Update_Weight_Full(WFC_Val3,WFC_Pos3);
    Update_Weight_Full(WFC_Val4,WFC_Pos4);
    Update_Weight_Full(WFC_Val5,WFC_Pos5);
    Update_Weight_Full(WFC_Val6,WFC_Pos6);
    Update_Weight_Full(WFC_Val7,WFC_Pos7);
    Update_Weight_Full(WFC_Val8,WFC_Pos8);
    Update_Weight_Full(WFC_Val9,WFC_Pos9);
    Update_Weight_Full(WFC_Val10,WFC_Pos10);
    Update_Weight_Full(WFC_Val11,WFC_Pos11);
    Update_Weight_Full(WFC_Val12,WFC_Pos12);

    WFCBufPos++;

    WFCMTF_Rank=(WFCMTF_Rank-1)&0xFF;

    if (WFCMTF_Rank>3) WFCMTF_Rank=3;
    CtxL0=((CtxL0<<2)|WFCMTF_Rank)&0xFF;

    VCtx=&Model_Log2RLE_0[CtxRLE+16*WFCMTF_Rank][0];
    UCtx=Model_Log2RLE_1+Char*(GRZ_Log2MaxBlockSize+1);

    Log2RunSize=0;

    while (1)
    {
      if (ARI_GetFreq(Model_MaxFreq)<((VCtx[0]+UCtx[0])>>1))
       {
         ARI_Decode_0((VCtx[0]+UCtx[0])>>1);
         UpdateVCtx_0(0,M_Log2RLE_Shift_0); UpdateUCtx_0(0,M_Log2RLE_Shift_1);
         break;
       }
      ARI_Decode_1((VCtx[0]+UCtx[0])>>1);
      UpdateVCtx_1(0,M_Log2RLE_Shift_0); UpdateUCtx_1(0,M_Log2RLE_Shift_1);
      VCtx++;UCtx++;
      Log2RunSize++;
    }

    if (Log2RunSize<2)
      CtxRLE=(CtxRLE<<1)&0xF;
    else
      CtxRLE=((CtxRLE<<1)|1)&0xF;

    VCtx=&Model_Log2RLE_2[Log2RunSize][0];

    RunSize=0;

    for (i=0;i<Log2RunSize;i++,VCtx++)
      if (ARI_GetFreq(Model_MaxFreq)<VCtx[0])
       {
         ARI_Decode_0(VCtx[0]);
         UpdateVCtx_0(0,M_Log2RLE_Shift_2);
         RunSize<<=1;
       }
      else
       {
         ARI_Decode_1(VCtx[0]);
         UpdateVCtx_1(0,M_Log2RLE_Shift_2);
         RunSize=(RunSize<<1)|1;
       }
    RunSize+=WFCMTF_Log2RLESize[Log2RunSize];

    while (RunSize--) *Output++=Char;

  }
  WFC_Finish();
  free(Model_Log2RLE_1);
  return (Output-SOutput);
}

#undef ARI_InTgtByte
#undef ARI_GetFreq
#undef ARI_Decode
#undef ARI_Decode_0
#undef ARI_Decode_1

#undef WFC_Val0
#undef WFC_Val1
#undef WFC_Val2
#undef WFC_Val3
#undef WFC_Val4
#undef WFC_Val5
#undef WFC_Val6
#undef WFC_Val7
#undef WFC_Val8
#undef WFC_Val9
#undef WFC_Val10
#undef WFC_Val11
#undef WFC_Val12
#undef WFC_Pos1
#undef WFC_Pos2
#undef WFC_Pos3
#undef WFC_Pos4
#undef WFC_Pos5
#undef WFC_Pos6
#undef WFC_Pos7
#undef WFC_Pos8
#undef WFC_Pos9
#undef WFC_Pos10
#undef WFC_Pos11
#undef WFC_Pos12
#undef WFC_Init
#undef WFC_Finish
#undef Update_Weight
#undef Update_Weight0
#undef Update_Weight_Full

#undef ARI_MaxByte
#undef WFC_MaxByte
#undef ARI_RangeTOP
#undef Model_NumBits
#undef Model_MaxFreq
#undef M_Log2RLE_Shift_0
#undef M_Log2RLE_Shift_1
#undef M_Log2RLE_Shift_2
#undef M_L1_Shift_0
#undef M_L1_Shift_1
#undef M_L2_Shift
#undef ModelL0_0MaxFreq
#undef ModelL0_1MaxFreq
#undef ModelL0_2MaxFreq
#undef UpdateVCtx_0
#undef UpdateVCtx_1
#undef UpdateUCtx_0
#undef UpdateUCtx_1
#undef Update_Model_L0
#undef Init_Models

/*-------------------------------------------------*/
/* End                                   WFC_Ari.c */
/*-------------------------------------------------*/
