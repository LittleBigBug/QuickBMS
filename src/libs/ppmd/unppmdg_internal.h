/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_

const int MAX_O=16;                         /* maximum allowed model order */

typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
typedef signed long    LONG;

#define _FASTCALL
#define _STDCALL

#if defined(__GNUC__)
#define _PACK_ATTR __attribute__ ((packed))
#else
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

template <class TMP_TYPE>
static TMP_TYPE CLAMP(const TMP_TYPE& X,const TMP_TYPE& LoX,const TMP_TYPE& HiX) { return (X >= LoX)?((X <= HiX)?(X):(HiX)):(LoX); }
template <class TMP_TYPE>
static void SWAP(TMP_TYPE& t1,TMP_TYPE& t2) { TMP_TYPE tmp=t1; t1=t2; t2=tmp; }

#endif /* !defined(_PPMDTYPE_H_) */

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 *  Comments: this implementation is claimed to be a public domain          *
 ****************************************************************************/
/**********************  Original text  *************************************
////////   Carryless rangecoder (c) 1999 by Dmitry Subbotin   ////////

typedef unsigned int  uint;
typedef unsigned char uc;

#define  DO(n)     for (int _=0; _<n; _++)
#define  TOP       (1<<24)
#define  BOT       (1<<16)


class RangeCoder
{
 uint  low, code, range, passed;
 FILE  *f;

 void OutByte (uc c)           { passed++; fputc(c,f); }
 uc   InByte ()                { passed++; return fgetc(f); }

public:

 uint GetPassed ()             { return passed; }
 void StartEncode (FILE *F)    { f=F; passed=low=0;  range= (uint) -1; }
 void FinishEncode ()          { DO(4)  OutByte(low>>24), low<<=8; }
 void StartDecode (FILE *F)    { passed=low=code=0;  range= (uint) -1;
                                 f=F; DO(4) code= code<<8 | InByte();
                               }

 void Encode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq * (range/= totFreq);
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       OutByte(low>>24), range<<=8, low<<=8;
 }

 uint GetFreq (uint totFreq) {
   uint tmp= (code-low) / (range/= totFreq);
   if (tmp >= totFreq)  throw ("Input data corrupt"); // or force it to return
   return tmp;                                         // a valid value :)
 }

 void Decode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq*range;
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       code= code<<8 | InByte(), range<<=8, low<<=8;
 }
};
*****************************************************************************/

static struct SUBRANGE {
    DWORD LowCount, HighCount, scale;
} SubRange;
static const DWORD TOP=1 << 24, BOT=1 << 15;
static DWORD low, code, range;

static void ariInitEncoder(unsigned char* stream)
{
    low=0;                                  range=DWORD(-1);
}
#define ARI_ENC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        xputc(low >> 24,stream);                                             \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
static void ariEncodeSymbol()
{
    low += SubRange.LowCount*(range /= SubRange.scale);
    range *= SubRange.HighCount-SubRange.LowCount;
}
static void ariShiftEncodeSymbol(UINT SHIFT)
{
    low += SubRange.LowCount*(range >>= SHIFT);
    range *= SubRange.HighCount-SubRange.LowCount;
}
#define ARI_FLUSH_ENCODER(stream) {                                         \
    for (int i=0;i < 4;i++) {                                               \
        xputc(low >> 24,stream);             low <<= 8;                      \
    }                                                                       \
}
#define ARI_INIT_DECODER(stream) {                                          \
    low=code=0;                             range=DWORD(-1);                \
    for (int i=0;i < 4;i++)                 code=(code << 8) | xgetc(stream);\
}
#define ARI_DEC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | xgetc(stream);                                    \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
static int ariGetCurrentCount() {
    return (code-low)/(range /= SubRange.scale);
}
static UINT ariGetCurrentShiftCount(UINT SHIFT) {
    return (code-low)/(range >>= SHIFT);
}
static void ariRemoveSubrange()
{
    low += range*SubRange.LowCount;
    range *= SubRange.HighCount-SubRange.LowCount;
}

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>
//#include "PPMd.h"
//#include "SubAlloc.h"

static const UINT N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4;
static const UINT UNIT_SIZE=12, N_INDEXES=N1+N2+N3+N4;

static long SubAllocatorSize=0;
static BYTE Indx2Units[N_INDEXES], Units2Indx[128];
static BYTE* HeapStart, * LoUnit, * HiUnit, * LastBreath;
static struct NODE { NODE* next; } FreeList[N_INDEXES];

static void InsertNode(void* p,int indx) {
    ((NODE*) p)->next=FreeList[indx].next;  FreeList[indx].next=(NODE*) p;
}
static void* RemoveNode(int indx) {
    NODE* RetVal=FreeList[indx].next;       FreeList[indx].next=RetVal->next;
    return RetVal;
}
static UINT I2B(int indx) { return UNIT_SIZE*Indx2Units[indx]; }
static void SplitBlock(void* pv,int OldIndx,int NewIndx)
{
    int i, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
    BYTE* p=((BYTE*) pv)+I2B(NewIndx);
    if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) {
        InsertNode(p,--i);                  p += I2B(i);
        UDiff -= Indx2Units[i];
    }
    InsertNode(p,Units2Indx[UDiff-1]);
}

static DWORD _STDCALL GetUsedMemory()
{
    DWORD i, k, RetVal=SubAllocatorSize-(HiUnit-LoUnit);
    for (k=i=0;i < N_INDEXES;i++, k=0) {
        for (NODE* pn=FreeList+i;(pn=pn->next) != NULL;k++)
                ;
        RetVal -= UNIT_SIZE*Indx2Units[i]*k;
    }
    if ( LastBreath )                       RetVal -= 128*128*UNIT_SIZE;
    return (RetVal >> 2);
}
static void _STDCALL StopSubAllocator() {
    if ( SubAllocatorSize ) {
        SubAllocatorSize=0;                 delete[] HeapStart;
    }
}
static BOOL _STDCALL StartSubAllocator(int SASize)
{
    DWORD t=SASize << 20;
    if (SubAllocatorSize == t)              return TRUE;
    StopSubAllocator();
    if ((HeapStart=new BYTE[t]) == NULL)    return FALSE;
    SubAllocatorSize=t;                     return TRUE;
}
static void InitSubAllocator()
{
    int i, k;
    memset(FreeList,0,sizeof(FreeList));
    HiUnit=(LoUnit=HeapStart)+UNIT_SIZE*(SubAllocatorSize/UNIT_SIZE);
    LastBreath=LoUnit;                      LoUnit += 128*128*UNIT_SIZE;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
}
static void* _FASTCALL AllocUnitsRare(int NU)
{
    int i, indx=Units2Indx[NU-1];
    if ( FreeList[indx].next )              return RemoveNode(indx);
    void* RetVal=LoUnit;                    LoUnit += I2B(indx);
    if (LoUnit <= HiUnit)                   return RetVal;
    if ( LastBreath ) {
        for (i=0;i < 128;i++) {
            InsertNode(LastBreath,N_INDEXES-1);
            LastBreath += 128*UNIT_SIZE;
        }
        LastBreath=NULL;
    }
    LoUnit -= I2B(indx);                    i=indx;
    do {
        if (++i == N_INDEXES)               return NULL;
    } while ( !FreeList[i].next );
    SplitBlock(RetVal=RemoveNode(i),i,indx);
    return RetVal;
}
static void* AllocContext()
{
    if (HiUnit != LoUnit)                   return (HiUnit -= UNIT_SIZE);
    return AllocUnitsRare(1);
}
static void* _FASTCALL ExpandUnits(void* OldPtr,int OldNU)
{
    int i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           return OldPtr;
    void* ptr=AllocUnitsRare(OldNU+1);
    if ( ptr ) {
        memcpy(ptr,OldPtr,I2B(i0));         InsertNode(OldPtr,i0);
    }
    return ptr;
}
static void* _FASTCALL ShrinkUnits(void* OldPtr,int OldNU,int NewNU)
{
    int i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
    if (i0 == i1)                           return OldPtr;
    if ( FreeList[i1].next ) {
        void* ptr=RemoveNode(i1);           memcpy(ptr,OldPtr,I2B(i1));
        InsertNode(OldPtr,i0);              return ptr;
    } else {
        SplitBlock(OldPtr,i0,i1);           return OldPtr;
    }
}
static void _FASTCALL FreeUnits(void* ptr,int OldNU)
{
    InsertNode(ptr,Units2Indx[OldNU-1]);
}
