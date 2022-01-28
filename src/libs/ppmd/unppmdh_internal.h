static unsigned char   *EncodedFile  = NULL;
static unsigned char   *EncodedFilel = NULL;
static unsigned char   *DecodedFile  = NULL;
static unsigned char   *DecodedFilel = NULL;
static int xgetc(void *X) {
    if(EncodedFile >= EncodedFilel) return(-1);
    return(*EncodedFile++);
}
static int xputc(int chr, void *X) {
    if(DecodedFile >= DecodedFilel) return(-1);
    *DecodedFile++ = chr;
    return(chr);
}

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_

#include <stdio.h>

const int MAX_O=16;                         /* maximum allowed model order */

typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
#define _FASTCALL
#define _STDCALL
#define _PACK_ATTR

// PPMd module works with file streams via ...GETC/...PUTC macros only
typedef unsigned char _PPMD_FILE;
#define _PPMD_E_GETC(fp)   xgetc(fp)
#define _PPMD_E_PUTC(c,fp) xputc((c),fp)
#define _PPMD_D_GETC(fp)   xgetc(fp)
#define _PPMD_D_PUTC(c,fp) xputc((c),fp)

template <class T>
static void _PPMD_SWAP(T& t1,T& t2) { T tmp=t1; t1=t2; t2=tmp; }

#endif /* !defined(_PPMDTYPE_H_) */


/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: interface to encoding/decoding routines                       *
 *  Comments: this file can be used as an interface to PPMd module          *
 *  (consisting of Model.cpp) from external program           				*
 ****************************************************************************/
#if !defined(_PPMD_H_)
#define _PPMD_H_

//#include "PPMdType.h"

static BOOL  _STDCALL StartSubAllocator(int SubAllocatorSize);
static void  _STDCALL StopSubAllocator();          // it can be called once
static DWORD _STDCALL GetUsedMemory();             // for information only

// (MaxOrder == 1) parameter value has special meaning, it does not restart
// model and can be used for solid mode archives;
// Call sequence:
//	StartSubAllocator(SubAllocatorSize);
//  EncodeFile(SolidArcFile,File1,MaxOrder);
//  EncodeFile(SolidArcFile,File2,1);
//  ...
//  EncodeFile(SolidArcFile,FileN,1);
//  StopSubAllocator();
//void _STDCALL EncodeFile(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile,int MaxOrder);
//void _STDCALL DecodeFile(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile,int MaxOrder);

// imported function
//void _STDCALL  PrintInfo(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile);

#endif /* !defined(_PPMD_H_) */


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

static void ariInitEncoder()
{
    low=0;                                  range=DWORD(-1);
}
#define ARI_ENC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        _PPMD_E_PUTC(low >> 24,stream);                                     \
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
        _PPMD_E_PUTC(low >> 24,stream); 				low <<= 8;                      \
    }                                                                       \
}
#define ARI_INIT_DECODER(stream) {                                          \
    low=code=0;                             range=DWORD(-1);                \
    for (int i=0;i < 4;i++)                                                 \
            code=(code << 8) | _PPMD_D_GETC(stream);                        \
}
#define ARI_DEC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | _PPMD_D_GETC(stream);                            \
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
 *  1999-2001                                                               *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/

static const UINT N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4;
static const UINT UNIT_SIZE=12, N_INDEXES=N1+N2+N3+N4;

#pragma pack(1)
struct MEM_BLKh {
    WORD Stamp, NU;
    MEM_BLKh* next, * prev;
    void insertAt(MEM_BLKh* p) {
        next=(prev=p)->next;                p->next=next->prev=this;
    }
    void remove() { prev->next=next;        next->prev=prev; }
} _PACK_ATTR;
#pragma pack()
static long SubAllocatorSize=0;
static BYTE Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
static BYTE* HeapStart, * pText, * UnitsStart, * LoUnit, * HiUnit;
static struct NODE { NODE* next; } FreeList[N_INDEXES];

static void InsertNode(void* p,int indx) {
    ((NODE*) p)->next=FreeList[indx].next;  FreeList[indx].next=(NODE*) p;
}
static void* RemoveNode(int indx) {
    NODE* RetVal=FreeList[indx].next;       FreeList[indx].next=RetVal->next;
    return RetVal;
}
static UINT U2B(int NU) { return 8*NU+4*NU; }
static void SplitBlock(void* pv,int OldIndx,int NewIndx)
{
    int i, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
    BYTE* p=((BYTE*) pv)+U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) {
        InsertNode(p,--i);                  p += U2B(i=Indx2Units[i]);
        UDiff -= i;
    }
    InsertNode(p,Units2Indx[UDiff-1]);
}

static DWORD _STDCALL GetUsedMemory()
{
    DWORD i, k, RetVal=SubAllocatorSize-(HiUnit-LoUnit)-(UnitsStart-pText);
    for (k=i=0;i < N_INDEXES;i++, k=0) {
        for (NODE* pn=FreeList+i;(pn=pn->next) != NULL;k++)
                ;
        RetVal -= UNIT_SIZE*Indx2Units[i]*k;
    }
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
    HiUnit=(pText=HeapStart)+SubAllocatorSize;
    UINT Diff=UNIT_SIZE*(SubAllocatorSize/8/UNIT_SIZE*7);
    LoUnit=UnitsStart=HiUnit-Diff;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (GlueCount=k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
}
static void GlueFreeBlocks()
{
    MEM_BLKh s0, * p, * p1;
    int i, k, sz;
    if (LoUnit != HiUnit)                   *LoUnit=0;
    for (i=0, s0.next=s0.prev=&s0;i < N_INDEXES;i++)
            while ( FreeList[i].next ) {
                p=(MEM_BLKh*) RemoveNode(i); p->insertAt(&s0);
                p->Stamp=0xFFFF;            p->NU=Indx2Units[i];
            }
    for (p=s0.next;p != &s0;p=p->next)
        while ((p1=p+p->NU)->Stamp == 0xFFFF && int(p->NU)+p1->NU < 0x10000) {
            p1->remove();                   p->NU += p1->NU;
        }
    while ((p=s0.next) != &s0) {
        for (p->remove(), sz=p->NU;sz > 128;sz -= 128, p += 128)
                InsertNode(p,N_INDEXES-1);
        if (Indx2Units[i=Units2Indx[sz-1]] != sz) {
            k=sz-Indx2Units[--i];           InsertNode(p+(sz-k),k-1);
        }
        InsertNode(p,i);
    }
}
static void* AllocUnitsRare(int indx)
{
    if ( !GlueCount ) {
        GlueCount = 255;                    GlueFreeBlocks();
        if ( FreeList[indx].next )          return RemoveNode(indx);
    }
    int i=indx;
    do {
        if (++i == N_INDEXES) {
            GlueCount--;                    i=U2B(Indx2Units[indx]);
            return (UnitsStart-pText > i)?(UnitsStart -= i):(NULL);
        }
    } while ( !FreeList[i].next );
    void* RetVal=RemoveNode(i);             SplitBlock(RetVal,i,indx);
    return RetVal;
}
static void* AllocUnits(int NU)
{
    int indx=Units2Indx[NU-1];
    if ( FreeList[indx].next )              return RemoveNode(indx);
    void* RetVal=LoUnit;                    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit)                   return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);        return AllocUnitsRare(indx);
}
static void* AllocContext()
{
    if (HiUnit != LoUnit)                   return (HiUnit -= UNIT_SIZE);
    if ( FreeList->next )                   return RemoveNode(0);
    return AllocUnitsRare(0);
}
static void* ExpandUnits(void* OldPtr,int OldNU)
{
    int i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           return OldPtr;
    void* ptr=AllocUnits(OldNU+1);
    if ( ptr ) {
        memcpy(ptr,OldPtr,U2B(OldNU));      InsertNode(OldPtr,i0);
    }
    return ptr;
}
static void* ShrinkUnits(void* OldPtr,int OldNU,int NewNU)
{
    int i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
    if (i0 == i1)                           return OldPtr;
    if ( FreeList[i1].next ) {
        void* ptr=RemoveNode(i1);           memcpy(ptr,OldPtr,U2B(NewNU));
        InsertNode(OldPtr,i0);              return ptr;
    } else {
        SplitBlock(OldPtr,i0,i1);           return OldPtr;
    }
}
static void FreeUnits(void* ptr,int OldNU)
{
    InsertNode(ptr,Units2Indx[OldNU-1]);
}
