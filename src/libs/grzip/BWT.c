/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor               BWT.c */
/* BWT Sorting Functions                           */
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

  "Fast" BWT memory use   : 6*BlockLen + 256Kb
  "Strong" BWT memory use : 9*BlockLen + 1Mb
  Reverse BWT memory use  : 5*BlockLen

  For more information on these sources, see the manual.
--*/

#include <stdio.h>
#include "libGRZip.h"

#define BWT_MaxByte                 256
#define BWT_MaxWord                 65536
#define BWT_MinQSort                20
#define BWT_MedTrh                  96

#define BWT_SPush(L,H,D)            {Stack[sp]=L;Stack[sp+1]=H;Stack[sp+2]=D;sp+=3;}
#define BWT_SPop(L,H,D)             {sp-=3;L=Stack[sp];H=Stack[sp+1];D=Stack[sp+2];}
#define BWT_Swap(SC1,SC2)           {sint32 Stmp=SC1;SC1=SC2;SC2=Stmp;}
#define BWT_Min(A,B)                ((A) < (B)) ? (A): (B)

#define FastBWT_RepTreshStep2       0.35
#define FastBWT_RepTreshStep4       1.15

#define FastBWT_MaxQSortDepth       32
#define FastBWT_QSortStackSize      1024
#define FastBWT_NumOverShoot        80
#define FastBWT_NumGroups           8
#define FastBWT_NSize(Ns)           (NextHi[Ns]-NextLo[Ns])

#define FastBWT_NSwap(N1,N2)                                        \
{                                                                   \
  sint32 Ntmp;                                                      \
  Ntmp=NextLo[N1];NextLo[N1]=NextLo[N2];NextLo[N2]=Ntmp;            \
  Ntmp=NextHi[N1];NextHi[N1]=NextHi[N2];NextHi[N2]=Ntmp;            \
  Ntmp=NextDepth[N1];NextDepth[N1]=NextDepth[N2];NextDepth[N2]=Ntmp;\
}


#define StrongBWT_Flag              0x40000000

#define StrongBWT_TSortStackSize    3*65536
#define StrongBWT_BFreq(B)          (Buckets[(B+1) << 8] - Buckets[B << 8])
#define StrongBWT_CMask             0x3FFFFFFF
#define StrongBWT_SMask             0x40000000

#define BWT_VSwap(S1,S2,Num)               \
{                                          \
  sint32 TS1=(S1);                         \
  sint32 TS2=(S2);                         \
  sint32 TNum=(Num);                       \
  while (TNum)                             \
  {                                        \
    BWT_Swap(Index[TS1],Index[TS2]);       \
    TS1++;TS2++;TNum--;                    \
  }                                        \
}

INLINE static
uint32 BWT_Med3(uint32 MA,uint32 MB,uint32 MC)
{
  uint32 MT;
  if (MA>MB) {MT=MA;MA=MB;MB=MT;}
  if (MB>MC)
  {
    MB=MC;
    if (MA>MB) MB=MA;
  }
  return MB;
}

INLINE static
sint32 StrongBWT_SimpleCMP(uint32 * Group,sint32 i1,sint32 i2)
{
  uint32 CMP1,CMP2;
  CMP1=Group[i1];CMP2=Group[i2];
  while (CMP1==CMP2) {CMP1=Group[i1+=2];CMP2=Group[i2+=2];}
  return (CMP1>CMP2);
}

static
void StrongBWT_ShellSort(sint32 * Index,uint32 * Group,sint32 Lo,sint32 Hi,sint32 d)
{
  if (Lo==Hi)
  {
    Group[Index[Lo]]=(Group[Index[Lo]]&0xFF000000)|Lo;
    return;
  }
  sint32 i,j,s,h=1,n=Hi-Lo+1;
  while (h<n) h=h*3+1;
  do {
    h /= 3;
    for (i=Lo+h;i<=Hi;i++)
    {
      s=Index[i]+d;
      j=i;
      while (StrongBWT_SimpleCMP(Group,Index[j-h]+d,s))
      {
        Index[j]=Index[j-h];
        j -= h;
        if (j<h+Lo) break;
      }
      Index[j]=s-d;
    }
  } while (h!=1);
  for (;Lo<=Hi;Lo++)
    Group[Index[Lo]]=(Group[Index[Lo]]&0xFF000000)|Lo;
}

static
void StrongBWT_ShellSortDeph2(sint32 * Index,uint32 * Group,sint32 Lo,sint32 Hi)
{
  if (Hi==Lo)
  {
    Group[Index[Lo]]=(Group[Index[Lo]]&0xFF000000)|Lo;
    return;
  }
  sint32 i,j,s;
  if (Hi-Lo>13)
    for (i=Lo+13;i<=Hi;i++)
    {
      s=Index[i]+2;
      j=i-13;
      while (StrongBWT_SimpleCMP(Group,Index[j]+2,s))
      {
        Index[j+13]=Index[j];
        if ((j-=13)<Lo) break;
      }
      Index[j+13]=s-2;
    }
  if (Hi-Lo>4)
    for (i=Lo+4;i<=Hi;i++)
    {
      s=Index[i]+2;
      j=i-4;
      while (StrongBWT_SimpleCMP(Group,Index[j]+2,s))
      {
        Index[j+4]=Index[j];
        if ((j-=4)<Lo) break;
      }
      Index[j+4]=s-2;
    }
  for (i=Lo+1;i<=Hi;i++)
  {
    s=Index[i]+2;
    j=i-1;
    while (StrongBWT_SimpleCMP(Group,Index[j]+2,s))
    {
      Index[j+1]=Index[j];
      if ((--j)<Lo) break;
    }
    Index[j+1]=s-2;
  }
  for (;Lo<=Hi;Lo++)
    Group[Index[Lo]]=(Group[Index[Lo]]&0xFF000000)|Lo;
}

static
void StrongBWT_TernarySort(sint32 * Index,uint32 * Group,sint32 Lo,sint32 Hi,sint32 * Stack)
{
  sint32 A,B,C,D,N,M,d,sp=0;
  uint32 Med,Med1,Med2,Med3;
  BWT_SPush(Lo,Hi,2);
  while (sp)
  {
    BWT_SPop(Lo,Hi,d);
    if (Hi<Lo) continue;

    if ((Hi-Lo<BWT_MinQSort)||(sp>=StrongBWT_TSortStackSize))
    {
      StrongBWT_ShellSort(Index,Group,Lo,Hi,d);
      continue;
    }
    if ((Hi-Lo)<BWT_MedTrh)
      Med=BWT_Med3(Group[Index[(Lo+Hi)>>1]+d],Group[Index[Lo]+d],Group[Index[Hi]+d]);
    else
    {
      M=(Hi+Lo)>>1;
      N=(Hi-Lo)>>3;
      Med1=BWT_Med3(Group[Index[Lo]+d],Group[Index[Lo+N]+d],Group[Index[Lo+N+N]+d]);
      Med2=BWT_Med3(Group[Index[M-N]+d],Group[Index[M]+d],Group[Index[M+N]+d]);
      Med3=BWT_Med3(Group[Index[Hi-N-N]+d],Group[Index[Hi-N]+d],Group[Index[Hi]+d]);
      Med=BWT_Med3(Med1,Med2,Med3);
    }
    A=Lo;B=Lo;C=Hi;D=Hi;
    while (1)
    {
      while (1)
      {
        if (B>C) break;
        if (Group[Index[B]+d]==Med)
        {
          BWT_Swap(Index[A],Index[B]);
          A++;B++;
          continue;
        }
        if (Group[Index[B]+d]>Med) break;
        B++;
      }
      while (1)
      {
        if (B>C) break;
        if (Group[Index[C]+d]==Med)
        {
          BWT_Swap(Index[C],Index[D]);
          D--;C--;
          continue;
        }
        if (Group[Index[C]+d]<Med) break;
        C--;
      }
      if (B>C) break;
      BWT_Swap(Index[B],Index[C]);
      B++;C--;
    }

    if (D<A)
    {
      BWT_SPush(Lo,Hi,d+2);
      continue;
    }

    N=BWT_Min(A-Lo,B-A);BWT_VSwap(Lo,B-N,N);
    M=BWT_Min(Hi-D,D-C);BWT_VSwap(B,Hi-M+1,M);

    N=Lo+B-A;
    M=Hi-(D-C);

    BWT_SPush(M+1,Hi,d);
    BWT_SPush(N,M,d+2);
    BWT_SPush(Lo,N-1,d);

  }
}

sint32 GRZip_StrongBWT_Encode(uint8 * Input,sint32 Size,uint8 * Output)
{
  uint8     RunOrder[BWT_MaxByte],BigDone[BWT_MaxByte];
  sint32    CopyStart[BWT_MaxByte],CopyEnd[BWT_MaxByte];

  uint8     LB;
  sint32    i,h;
  uint32    W;

  uint32 *  Group=(uint32 *)malloc((Size+1)*sizeof(uint32));
  if (Group==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  sint32 *  Index=(sint32 *)malloc((Size+1)*sizeof(sint32));
  if (Index==NULL) {free(Group);return (GRZ_NOT_ENOUGH_MEMORY);}

  sint32 *  Buckets=(sint32 *)malloc((BWT_MaxWord+1)*sizeof(sint32));
  if (Buckets==NULL) {free(Index);free(Group);return (GRZ_NOT_ENOUGH_MEMORY);}

  sint32 *  BGroups=(sint32 *)malloc((StrongBWT_TSortStackSize+3)*sizeof(sint32));
  if (BGroups==NULL) {free(Buckets);free(Index);free(Group);return (GRZ_NOT_ENOUGH_MEMORY);}

  memset(Buckets,0,(BWT_MaxWord+1)*sizeof(sint32));

  LB=Input[Size-1]; W=LB<<8; Buckets[W]++;
  for (i=Size-2;i>=0;i--) Buckets[(W=(W>>8)|(Input[i]<<8))]++;

  BGroups[0]=Buckets[0];
  for (i=1;i<=BWT_MaxWord;i++) BGroups[i]=(Buckets[i]+=Buckets[i-1]);

  W=LB<<8;
  for (i=Size-2;i>=0;i--)
  {
    W=(W>>8)|(Input[i]<<8);
    Group[i]=BGroups[W]|(Input[i]<<24);
    Index[Buckets[W]--]=i;
  }

  Group[Size]=0;Group[Size-1]=Buckets[LB<<8]|(LB<<24);
  Index[0]=Size;Index[Buckets[LB<<8]--]=Size-1;

  for (i=0;i<BWT_MaxByte;i++){RunOrder[i]=i;BigDone[i]=0;}

  h=364;
  do {
    h /= 3;
    for (i=h;i<BWT_MaxByte;i++)
    {
      uint8  s=RunOrder[i];
      sint32 j=i;
      while (StrongBWT_BFreq(RunOrder[j-h])>StrongBWT_BFreq(s))
      {
        RunOrder[j]=RunOrder[j-h];
        j -= h;
        if (j<h) break;
      }
      RunOrder[j]=s;
    }
  } while (h!=1);

  h=0; while (StrongBWT_BFreq(RunOrder[h])==0) h++;

  while (h<BWT_MaxByte)
  {
    uint8  S=RunOrder[h];
    sint32 j;
    for (j=0;j<BWT_MaxByte;j++)
      if (j!=S)
      {
        sint32 k=(S<<8)|j;
        sint32 Lo=(Buckets[k])+1;
        sint32 Hi=(Buckets[k+1]&StrongBWT_CMask);
        if ((LB<<8)==k) Lo++;
        if ((Lo<StrongBWT_SMask)&&(Hi>=Lo))
	{
          if (Hi-Lo>BWT_MinQSort)
            StrongBWT_TernarySort(Index,Group,Lo,Hi,BGroups);
          else
            StrongBWT_ShellSortDeph2(Index,Group,Lo,Hi);
	}
      }

    for (j=0;j<BWT_MaxByte;j++)
    {
      CopyStart[j]=(Buckets[(j<<8)+S]&StrongBWT_CMask)+1;
      if ((LB<<8)==((j<<8)+S)) CopyStart[j]++;
      Buckets[(j<<8)+S] |= StrongBWT_SMask;
      CopyEnd[j]=(Buckets[(j<<8)+S+1] & StrongBWT_CMask);
    }

    for (j=(Buckets[S<<8]&StrongBWT_CMask)+1;j<CopyStart[S];j++)
    {
      sint32 k=Index[j]-1;
      uint8  c;
      if (k<0) continue;
      c=Group[k]>>24;
      if (!BigDone[c])
      {
        Group[k]=(Group[k]&0xFF000000)|CopyStart[c];
        Index[CopyStart[c]++]=k;
      }
    }

    for (j=(Buckets[(S+1)<<8]&StrongBWT_CMask);j>CopyEnd[S];j--)
    {
      sint32 k=Index[j]-1;
      uint8  c;
      if (k<0) continue;
      c=Group[k]>>24;
      if (!BigDone[c])
      {
        Group[k]=(Group[k]&0xFF000000)|CopyEnd[c];
        Index[CopyEnd[c]--]=k;
      }
    }
    BigDone[S]=1;h++;
  }

  free(Buckets);free(BGroups);

  LB=Group[0]>>24;
  W= Group[0]&0xFFFFFF;
  for (i=1;i<=Size;i++)
  {
    uint32 Ps=(Group[i]&0xFFFFFF);
    uint8   c=Group[i]>>24;
    if (Ps<W)
      Output[Ps]=LB;
    else
      Output[Ps-1]=LB;
    LB=c;
  }
  free(Index);free(Group);
  return W;
}

sint32 GRZip_StrongBWT_Decode(uint8 * Input,sint32 Size,sint32 FBP)
{
  uint32 Count[BWT_MaxByte];
  sint32 i;
  uint32 Sum;

  uint32 * T=(uint32 *)malloc((Size+1)*sizeof(uint32));
  if (T==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  memset(Count,0,BWT_MaxByte*sizeof(uint32));

  for (i=0;i<FBP;i++)
  {
    uint8 c=Input[i];
    T[i]=((Count[c]++)<<8)|c;
  }

  for (i=FBP;i<Size;i++)
  {
    uint8 c=Input[i];
    T[i+1]=((Count[c]++)<<8)|c;
  }

  for (Sum=1,i=0;i<BWT_MaxByte;i++){Sum+=Count[i];Count[i]=Sum-Count[i];}

  for (FBP=0,i=Size-1;i>=0;i--)
  {
    uint32 u=T[FBP];
    uint8  c=u&0xFF;
    FBP=(u>>8)+Count[c];
    Input[i]=c;
  }

  free(T);
  return GRZ_NO_ERROR;
}

INLINE static
sint32 FastBWT_SimpleCmp
    (uint32 * Cmp1,uint32 * Cmp2,sint32 Size,uint32 * Input,sint32 * AML)
{
  do
  {
    if (*Cmp1!=*Cmp2) return (*Cmp1>*Cmp2);
    if (*(Cmp1-1)!=*(Cmp2-1)) return (*(Cmp1-1)>*(Cmp2-1));
    if (*(Cmp1-2)!=*(Cmp2-2)) return (*(Cmp1-2)>*(Cmp2-2));
    if (*(Cmp1-3)!=*(Cmp2-3)) return (*(Cmp1-3)>*(Cmp2-3));
    if (*(Cmp1-4)!=*(Cmp2-4)) return (*(Cmp1-4)>*(Cmp2-4));
    if (*(Cmp1-5)!=*(Cmp2-5)) return (*(Cmp1-5)>*(Cmp2-5));
    if (*(Cmp1-6)!=*(Cmp2-6)) return (*(Cmp1-6)>*(Cmp2-6));
    if (*(Cmp1-7)!=*(Cmp2-7)) return (*(Cmp1-7)>*(Cmp2-7));

    Cmp1-=8;Cmp2-=8;

    if (Cmp1<Input) Cmp1=(uint32 *)(((uint8 *)Cmp1)+Size);
    if (Cmp2<Input) Cmp2=(uint32 *)(((uint8 *)Cmp2)+Size);

    if ((--(*AML))<0) return 0;

  } while (1);
}

static
void FastBWT_ShellSortDeph2(sint32 * Index,uint8 * Input,sint32 Lo,
                                sint32 Hi,sint32 Size,sint32 * AML)
{
  sint32 i,j;
  if (Hi-Lo>13)
    for (i=Lo+13;i<=Hi;i++)
    {
      sint32   Idx=Index[i];
      uint32 * Ptr=(uint32 *)(Input+Idx-4);
      j=i-13;
      while (FastBWT_SimpleCmp(
            (uint32 *)(Input+Index[j]-4),Ptr,Size,(uint32 *)Input,AML))
      {
        Index[j+13]=Index[j];
        if ((j-=13)<Lo) break;
      }
      Index[j+13]=Idx;
      if ((*AML)<0) return ;
    }
  if (Hi-Lo>4)
    for (i=Lo+4;i<=Hi;i++)
    {
      sint32   Idx=Index[i];
      uint32 * Ptr=(uint32 *)(Input+Idx-4);
      j=i-4;
      while (FastBWT_SimpleCmp(
            (uint32 *)(Input+Index[j]-4),Ptr,Size,(uint32 *)Input,AML))
      {
        Index[j+4]=Index[j];
        if ((j-=4)<Lo) break;
      }
      Index[j+4]=Idx;
      if ((*AML)<0) return ;
    }
  for (i=Lo+1;i<=Hi;i++)
  {
    sint32   Idx=Index[i];
    uint32 * Ptr=(uint32 *)(Input+Idx-4);
    j=i-1;
    while (FastBWT_SimpleCmp(
          (uint32 *)(Input+Index[j]-4),Ptr,Size,(uint32 *)Input,AML))
    {
      Index[j+1]=Index[j];
      if ((--j)<Lo) break;
    }
    Index[j+1]=Idx;
    if ((*AML)<0) return ;
  }
}

static
void FastBWT_ShellSort(sint32 * Index,uint8 * Input,sint32 Lo,
                       sint32 Hi,sint32 d,sint32 Size,sint32 * AML)
{
  sint32 i,j,h=1,n=Hi-Lo+1;
  while (h<n) h=h*3+1;
  do {
    h /= 3;
    for (i=Lo+h;i<=Hi;i++)
    {
      sint32   Idx=Index[i];
      uint32 * Ptr=(uint32 *)(Input+Idx+d);
      j=i;
      while (FastBWT_SimpleCmp(
            (uint32 *)(Input+Index[j-h]+d),Ptr,Size,(uint32 *)Input,AML))
      {
        Index[j]=Index[j-h];
        j -= h;
        if (j<h+Lo) break;
      }
      Index[j]=Idx;
      if ((*AML)<0) return ;
    }
  } while (h!=1);
}

static void
FastBWT_TernarySort(sint32 * Index,uint8 * Input,sint32 Lo,
                        sint32 Hi,sint32 Size,sint32 * AML)
{
  sint32 Stack     [FastBWT_QSortStackSize];

  sint32 NextLo    [3];
  sint32 NextHi    [3];
  sint32 NextDepth [3];

  sint32 A,B,C,D,N,M,d,sp=0;

  BWT_SPush(Lo,Hi,-4);
  while (sp)
  {
    BWT_SPop(Lo,Hi,d);

    if (Hi<=Lo) continue;

    if ((Hi-Lo<BWT_MinQSort)||(d<=(-FastBWT_MaxQSortDepth)))
    {
      FastBWT_ShellSort(Index,Input,Lo,Hi,d,Size,AML);
      if ((*AML)<0) return ;
      continue;
    }

    uint32 Med;

    if ((Hi-Lo)<BWT_MedTrh)
      Med=BWT_Med3(*(uint32 *)(Input+Index[(Lo+Hi)>>1]+d),
                   *(uint32 *)(Input+Index[Lo]+d),
                   *(uint32 *)(Input+Index[Hi]+d));
    else
    {
      M=(Hi+Lo)>>1;
      N=(Hi-Lo)>>3;
      uint32 Med1=BWT_Med3(*(uint32 *)(Input+Index[Lo]+d),
                           *(uint32 *)(Input+Index[Lo+N]+d),
                           *(uint32 *)(Input+Index[Lo+N+N]+d));
      uint32 Med2=BWT_Med3(*(uint32 *)(Input+Index[M-N]+d),
                           *(uint32 *)(Input+Index[M]+d),
                           *(uint32 *)(Input+Index[M+N]+d));
      uint32 Med3=BWT_Med3(*(uint32 *)(Input+Index[Hi-N-N]+d),
                           *(uint32 *)(Input+Index[Hi-N]+d),
                           *(uint32 *)(Input+Index[Hi]+d));
      Med=BWT_Med3(Med1,Med2,Med3);
    }

    A=Lo;B=Lo;C=Hi;D=Hi;
    while (1)
    {
      while (1)
      {
        if (B>C) break;
        uint32 T=*(uint32 *)(Input+Index[B]+d);
        if (T==Med)
        {
          BWT_Swap(Index[A],Index[B]);
          A++;B++;
          continue;
        }
        if (T>Med) break;
        B++;
      }
      while (1)
      {
        if (B>C) break;
        uint32 T=*(uint32 *)(Input+Index[C]+d);
        if (T==Med)
        {
          BWT_Swap(Index[C],Index[D]);
          D--;C--;
          continue;
        }
        if (T<Med) break;
        C--;
      }
      if (B>C) break;
      BWT_Swap(Index[B],Index[C]);
      B++;C--;
    }

    if (D<A)
    {
      BWT_SPush(Lo,Hi,d-4);
      continue;
    }

    N=BWT_Min(A-Lo,B-A);BWT_VSwap(Lo,B-N,N);
    M=BWT_Min(Hi-D,D-C);BWT_VSwap(B,Hi-M+1,M);

    N=Lo+B-A-1;
    M=Hi-(D-C)+1;

    NextLo[0]=Lo; NextHi[0]=N;  NextDepth[0]=d;
    NextLo[1]=M;  NextHi[1]=Hi; NextDepth[1]=d;
    NextLo[2]=N+1;NextHi[2]=M-1;NextDepth[2]=d-4;

    if (FastBWT_NSize(0)<FastBWT_NSize(2)) FastBWT_NSwap(0,2);
    if (FastBWT_NSize(0)<FastBWT_NSize(1)) FastBWT_NSwap(0,1);
    if (FastBWT_NSize(1)<FastBWT_NSize(2)) FastBWT_NSwap(1,2);

    BWT_SPush(NextLo[0],NextHi[0],NextDepth[0]);
    BWT_SPush(NextLo[1],NextHi[1],NextDepth[1]);
    BWT_SPush(NextLo[2],NextHi[2],NextDepth[2]);

  }
}

#define FastBWT_Match(Grp0,Grp1,Win)                   \
{                                                      \
  sint32 Ptr0=SmallBucketLo[Grp0];                     \
  sint32 Ptr1=SmallBucketLo[Grp1];                     \
  if (Ptr0>=SmallBucketHi[Grp0])                       \
     Winner[Win]=Grp1;                                 \
  else                                                 \
   if (Ptr1>=SmallBucketHi[Grp1])                      \
      Winner[Win]=Grp0;                                \
   else                                                \
    {                                                  \
      uint32 * i0=PtrHash[Grp0];                       \
      uint32 * i1=PtrHash[Grp1];                       \
      if (FastBWT_SimpleCmp(i1,i0,Size,                \
                              (uint32 *)Input,AML))    \
         Winner[Win]=Grp0;                             \
      else                                             \
         Winner[Win]=Grp1;                             \
    }                                                  \
}

INLINE static
void TreeReBuild(sint32 * SmallBucketLo,
                 sint32 * SmallBucketHi,
                 uint32 ** PtrHash,
                 uint8  * Winner,
                 uint8  * Input,
                 sint32 Size,
                 sint32 * AML)
{
  if (FastBWT_NumGroups==8)
   {
     FastBWT_Match(0,1,0); FastBWT_Match(2,3,1);
     uint8 Win0=Winner[0];
     uint8 Win1=Winner[1];
     FastBWT_Match(Win0,Win1,4);
     FastBWT_Match(4,5,2); FastBWT_Match(6,7,3);
     Win0=Winner[2]; Win1=Winner[3];
     FastBWT_Match(Win0,Win1,5);
     Win0=Winner[4]; Win1=Winner[5];
     FastBWT_Match(Win0,Win1,6);
   }
  else
   {
     sint32 i,j;
     for (i=0,j=FastBWT_NumGroups>>1;i<j;i++) FastBWT_Match(2*i,2*i+1,i);
     for (i=0,j=(FastBWT_NumGroups>>1)-1;i<j;i++)
     {
       uint8 Win0=Winner[2*i];
       uint8 Win1=Winner[2*i+1];
       FastBWT_Match(Win0,Win1,i+(FastBWT_NumGroups>>1));
     }
   }
}

#define FastBWT_MatchFast(Grp0,Grp1,Win)            \
{                                                   \
  uint32 * i0=PtrHash[Grp0];                        \
  uint32 * i1=PtrHash[Grp1];                        \
  if (FastBWT_SimpleCmp(i1,i0,Size,                 \
                          (uint32 *)Input,AML))     \
    Winner[Win]=Grp0;                               \
  else                                              \
    Winner[Win]=Grp1;                               \
}

INLINE static
void TreeUpdateFast(uint32 ** PtrHash,
                    uint8  * Winner,
                    uint8  * Input,
                    sint32 Size,
                    sint32 PredWinner,
                    sint32 * AML)
{
 if (FastBWT_NumGroups==8)
   {
     switch (PredWinner)
     {
       case 0: case 1:
        {
          FastBWT_MatchFast(0,1,0);
          uint8 Win0=Winner[0];
          uint8 Win1=Winner[1];
          FastBWT_MatchFast(Win0,Win1,4);
          Win0=Winner[4]; Win1=Winner[5];
          FastBWT_MatchFast(Win0,Win1,6);
          break;
        }
       case 2: case 3:
        {
          FastBWT_MatchFast(2,3,1);
          uint8 Win0=Winner[0];
          uint8 Win1=Winner[1];
          FastBWT_MatchFast(Win0,Win1,4);
          Win0=Winner[4]; Win1=Winner[5];
          FastBWT_MatchFast(Win0,Win1,6);
          break;
        }
       case 4: case 5:
        {
          FastBWT_MatchFast(4,5,2);
          uint8 Win0=Winner[2];
          uint8 Win1=Winner[3];
          FastBWT_MatchFast(Win0,Win1,5);
          Win0=Winner[4]; Win1=Winner[5];
          FastBWT_MatchFast(Win0,Win1,6);
          break;
        }
       case 6: case 7:
        {
          FastBWT_MatchFast(6,7,3);
          uint8 Win0=Winner[2];
          uint8 Win1=Winner[3];
          FastBWT_MatchFast(Win0,Win1,5);
          Win0=Winner[4]; Win1=Winner[5];
          FastBWT_MatchFast(Win0,Win1,6);
          break;
        }
     }
   }
  else
   {
     uint8 Win0=PredWinner;
     uint8 Win1=PredWinner^(0x1);
     uint8 Win=(PredWinner>>1);
     FastBWT_MatchFast(Win0,Win1,Win);
     do {
       uint8 Win0=Winner[Win];
       uint8 Win1=Winner[Win^0x1];
             Win>>=1;
       FastBWT_MatchFast(Win0,Win1,Win+(FastBWT_NumGroups>>1));
       Win=Win+(FastBWT_NumGroups>>1);
    } while (Win<(FastBWT_NumGroups-2));
  }
}

sint32 GRZip_FastBWT_Encode(uint8 * Input,sint32 Size,uint8 * Output)
{

  sint32   GroupsFreq[FastBWT_NumGroups][BWT_MaxByte];

  sint32   BucketStart[BWT_MaxByte];
  sint32   SmallBucketLo[FastBWT_NumGroups];
  sint32   SmallBucketHi[FastBWT_NumGroups];

  uint8    Winner[FastBWT_NumGroups-1];
  uint32 * PtrHash[FastBWT_NumGroups];

  sint32 * BigBucket=(sint32 *)malloc(BWT_MaxWord*sizeof(sint32));
  if (BigBucket==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  sint32 * Index=(sint32 *)malloc((Size+1)*sizeof(sint32));
  if (Index==NULL) {free(BigBucket);return (GRZ_NOT_ENOUGH_MEMORY);}

  sint32   GroupSize=Size/FastBWT_NumGroups;

  sint32   AML=(sint32)(((float)Size)*FastBWT_RepTreshStep2);

  sint32   i,j,Cum,Lo,Hi;

  memset(GroupsFreq,0,FastBWT_NumGroups*BWT_MaxByte*sizeof(sint32));
  memset(BigBucket,0,BWT_MaxWord*sizeof(sint32));

  for (i=Size-1;i>0;i-=FastBWT_NumGroups)
    {
      for (j=0;j<FastBWT_NumGroups;j++)
        GroupsFreq[j][Input[i-j]]++;
      BigBucket[(Input[i-FastBWT_NumGroups+1]<<8)|Input[i-FastBWT_NumGroups]]++;
    }

  for (Cum=0,i=0;i<FastBWT_NumGroups;i++)
  {
    SmallBucketLo[i]=Cum;
    SmallBucketHi[i]=(Cum+=GroupSize);
  }

  for (Cum=SmallBucketLo[FastBWT_NumGroups-1],i=0;i<BWT_MaxWord;i++)
    Cum+=BigBucket[i],BigBucket[i]=Cum-BigBucket[i];

  for (i=0;i<Size;i+=FastBWT_NumGroups)
  {
    sint32 tmp=(Input[i]<<8)|Input[i-1];
    Index[BigBucket[tmp]++]=i;
  }

  for (Lo=SmallBucketLo[FastBWT_NumGroups-1],i=0;i<BWT_MaxWord;i++)
  {
    Hi=BigBucket[i]-1;
    if (Lo<Hi)
    {
      if (Hi-Lo>BWT_MinQSort)
        FastBWT_TernarySort(Index,Input,Lo,Hi,Size,&AML);
      else
        FastBWT_ShellSortDeph2(Index,Input,Lo,Hi,Size,&AML);
    }
    if (AML<0)
    {
      free(BigBucket);
      free(Index);
      return (GRZ_FAST_BWT_FAILS);
    }
    Lo=Hi+1;
  }

  free(BigBucket);

  for (i=FastBWT_NumGroups-2;i>=0;i--)
  {
    Cum=SmallBucketLo[i];
    for (j=0;j<BWT_MaxByte;j++)
      BucketStart[j]=Cum,Cum+=GroupsFreq[i][j];
    for (Lo=SmallBucketLo[i+1],Hi=SmallBucketHi[i+1];Lo<Hi;Lo++)
    {
      sint32 tmp=Index[Lo];
      uint8  c=Input[tmp+1];
      Index[Lo]=tmp|(c<<24);
      Index[BucketStart[c]++]=tmp+1;
    }
  }

  for (Lo=SmallBucketLo[0],Hi=SmallBucketHi[0];Lo<Hi;Lo++)
  {
    sint32 tmp=Index[Lo];
    Index[Lo]=tmp|(Input[tmp+1]<<24);
  }

  for (Cum=0,i=0;i<BWT_MaxByte;i++)
  {
    BucketStart[i]=Cum;
    for (j=0;j<FastBWT_NumGroups;j++) Cum+=GroupsFreq[j][i];
  }

  memset(Output,0xFF,Size);

  for (i=0;i<FastBWT_NumGroups;i++)
    PtrHash[i]=(uint32 *)(Input+(Index[SmallBucketLo[i]]&0xFFFFFF)-3);

  AML=(sint32)(((float)Size)*FastBWT_RepTreshStep4);

  i=0; while (1)
  {
    sint32 Min=(SmallBucketHi[0]-SmallBucketLo[0]);
    for (j=1;j<FastBWT_NumGroups;j++)
    {
      sint32 tmp=(SmallBucketHi[j]-SmallBucketLo[j]);
      if (tmp<Min) Min=tmp;
    }
    if (Min<=0) break;
    for (j=i+Min;i<j;)
    {
      TreeReBuild(SmallBucketLo,SmallBucketHi,PtrHash,Winner,Input,Size,&AML);
      if (AML<0) {free(Index); return (GRZ_FAST_BWT_FAILS);}
      for (;(i<j)&&(Output[i]==0xFF);i++)
      {
        uint8 GNum = Winner[FastBWT_NumGroups-2];
        uint32 Indx= SmallBucketLo[GNum]++;
        uint32 Ptr = Index[Indx];
        uint8   PB = (Output[i]=(Ptr>>24));
        if ((Ptr&0xFFFFFF)==Size-1) Cum=i;
        PtrHash[GNum] =(uint32 *)(Input+(Index[Indx+1]&0xFFFFFF)-3);

        Ptr=(BucketStart[PB]++);
        if (i<Ptr) Output[Ptr]=(GNum+FastBWT_NumGroups-1)&(FastBWT_NumGroups-1);

        TreeUpdateFast(PtrHash,Winner,Input,Size,GNum,&AML);
        if (AML<0) {free(Index); return (GRZ_FAST_BWT_FAILS);}
      }
      for (;(i<j)&&(Output[i]!=0xFF);i++)
      {
        uint8 GNum = Output[i];
        uint32 Indx= SmallBucketLo[GNum]++;
        uint32 Ptr = Index[Indx];
        uint8   PB = (Output[i]=(Ptr>>24));
        if ((Ptr&0xFFFFFF)==Size-1) Cum=i;
        PtrHash[GNum] =(uint32 *)(Input+(Index[Indx+1]&0xFFFFFF)-3);

        Ptr=(BucketStart[PB]++);
        if (i<Ptr) Output[Ptr]=(GNum+FastBWT_NumGroups-1)&(FastBWT_NumGroups-1);
      }
    }
  }
  for (;i<Size;i++)
    {
      TreeReBuild(SmallBucketLo,SmallBucketHi,PtrHash,Winner,Input,Size,&AML);
      if (AML<0) {free(Index); return (GRZ_FAST_BWT_FAILS);}
      uint8 GNum = Winner[FastBWT_NumGroups-2];
      uint32 Indx= SmallBucketLo[GNum]++;
      uint32 Ptr = Index[Indx];
      Output[i]=(Ptr>>24);
      if ((Ptr&0xFFFFFF)==Size-1) Cum=i;
      PtrHash[GNum] =(uint32 *)(Input+(Index[Indx+1]&0xFFFFFF)-3);
   }
  free(Index);
  if (AML<0) return (GRZ_FAST_BWT_FAILS); else return (Cum);
}

sint32 GRZip_FastBWT_Decode(uint8 * Input,sint32 Size,sint32 FBP)
{
  uint32 Count[BWT_MaxByte];
  sint32 i;
  uint32 Sum;

  uint32 * T=(uint32 *)malloc(Size*sizeof(uint32));
  if (T==NULL) return (GRZ_NOT_ENOUGH_MEMORY);

  memset(Count,0,BWT_MaxByte*sizeof(uint32));

  for (i=0;i<Size;i++)
  {
    uint8 c=Input[i];
    T[i]=((Count[c]++)<<8)|c;
  }

  for (Sum=0,i=0;i<BWT_MaxByte;i++){Sum+=Count[i];Count[i]=Sum-Count[i];}

  for (i=Size-1;i>=0;i--)
  {
    uint32 u=T[FBP];
    uint8  c=u&0xFF;
    FBP=(u>>8)+Count[c];
    Input[i]=c;
  }

  free(T);
  return GRZ_NO_ERROR;
}

void GRZip_BWT_FastBWT_Init(uint8 * Input,sint32 Size)
{
  sint32 i,Mid=(Size+FastBWT_NumOverShoot)>>1;
  for (i=0;i<FastBWT_NumOverShoot;i++) Input[Size+i]=Input[i];
  for (i=0;i<Mid;i++)
  {
    uint8 C0=Input[i];
    uint8 C1=Input[Size+FastBWT_NumOverShoot-i-1];
    Input[i]=C1;
    Input[Size+FastBWT_NumOverShoot-i-1]=C0;
  }
  Input[Size+FastBWT_NumOverShoot]=Input[FastBWT_NumOverShoot];
  Input[Size+FastBWT_NumOverShoot+1]=Input[FastBWT_NumOverShoot+1];
  Input[Size+FastBWT_NumOverShoot+2]=Input[FastBWT_NumOverShoot+2];
  Input[Size+FastBWT_NumOverShoot+3]=Input[FastBWT_NumOverShoot+3];
}

void GRZip_BWT_FastBWT_Done(uint8 * Input,sint32 Size)
{
  sint32 i,Mid=(Size+FastBWT_NumOverShoot)>>1;
  for (i=0;i<Mid;i++)
  {
    uint8 C0=Input[i];
    uint8 C1=Input[Size+FastBWT_NumOverShoot-i-1];
    Input[i]=C1;
    Input[Size+FastBWT_NumOverShoot-i-1]=C0;
  }
}

sint32 GRZip_BWT_Encode(uint8 * Input,sint32 Size,uint8 * Output,sint32 FastMode)
{
  if (!FastMode)
  {
    sint32 Result=GRZip_StrongBWT_Encode(Input,Size,Output);
    if (Result!=GRZ_NOT_ENOUGH_MEMORY) return (Result|StrongBWT_Flag);
    return (GRZ_NOT_ENOUGH_MEMORY);
  }
  if (Input==Output)
  {
    uint8 * Buf=(uint8 *)malloc(Size);
    if (Buf==NULL) return (GRZ_NOT_ENOUGH_MEMORY);
    GRZip_BWT_FastBWT_Init(Input,Size);
    sint32 Result=GRZip_FastBWT_Encode(Input+FastBWT_NumOverShoot,Size,Buf);
    if ((Result!=GRZ_FAST_BWT_FAILS)&&(Result!=GRZ_NOT_ENOUGH_MEMORY))
    {
      memcpy(Output,Buf,Size);
      free(Buf);
      return (Result);
    }
    free(Buf);
    GRZip_BWT_FastBWT_Done(Input,Size);
    if (Result==GRZ_NOT_ENOUGH_MEMORY) return (Result);
    Result=GRZip_StrongBWT_Encode(Input,Size,Output);
    if (Result!=GRZ_NOT_ENOUGH_MEMORY) return (Result|StrongBWT_Flag);
    return (GRZ_NOT_ENOUGH_MEMORY);
  }
  GRZip_BWT_FastBWT_Init(Input,Size);
  sint32 Result=GRZip_FastBWT_Encode(Input+FastBWT_NumOverShoot,Size,Output);
  GRZip_BWT_FastBWT_Done(Input,Size);
  if (Result!=GRZ_FAST_BWT_FAILS) return (Result);
  Result=GRZip_StrongBWT_Encode(Input,Size,Output);
  if (Result!=GRZ_NOT_ENOUGH_MEMORY) return (Result|StrongBWT_Flag);
  return (GRZ_NOT_ENOUGH_MEMORY);
}

sint32 GRZip_BWT_Decode(uint8 * Input,sint32 Size,sint32 FBP)
{
  if ((FBP&StrongBWT_Flag)==0)
    return (GRZip_FastBWT_Decode(Input,Size,FBP));
  else
    return (GRZip_StrongBWT_Decode(Input,Size,FBP&(~StrongBWT_Flag)));
}

#undef BWT_MaxByte
#undef BWT_MaxWord
#undef BWT_MinQSort
#undef BWT_MedTrh
#undef BWT_SPush
#undef BWT_SPop
#undef BWT_Swap
#undef BWT_Min
#undef BWT_VSwap

#undef FastBWT_RepTreshStep2
#undef FastBWT_RepTreshStep4
#undef FastBWT_MaxQSortDepth
#undef FastBWT_QSortStackSize
#undef FastBWT_NumOverShoot
#undef FastBWT_NumGroups
#undef FastBWT_MatchFast
#undef FastBWT_Match

#undef StrongBWT_Flag
#undef StrongBWT_TSortStackSize
#undef StrongBWT_BFreq
#undef StrongBWT_CMask
#undef StrongBWT_SMask

/*-------------------------------------------------*/
/* End                                       BWT.c */
/*-------------------------------------------------*/
