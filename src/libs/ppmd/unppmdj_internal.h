/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001, 2006                                                         *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_
#define NDEBUG
#include <stdio.h>
#include <assert.h>

typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;                // it must be equal to uint8_t
typedef unsigned short WORD;                // it must be equal to uint16_t
typedef unsigned int  DWORD;                // it must be equal to uint32_t
typedef unsigned long QWORD;                // it must be equal to uint64_t
typedef unsigned int   UINT;
                        /* Optimal definitions for processors:              */
#if defined(__LP64__) || (UINTPTR_MAX == 0xffffffffffffffffLL)
#define _64_NORMAL    /* AMD64/EM64T                                      */
#else
#define _32_NORMAL    /* IA-32                                            */
#endif
//#define _32_EXOTIC    /* with request for 32bit alignment for uint32_t    */
//#define _64_EXOTIC    /* some unknown to me processors                    */
#if defined(_32_NORMAL)+defined(_64_NORMAL)+defined(_32_EXOTIC)+defined(_64_EXOTIC) != 1
#error Only one processor type must be defined
#endif /* defined(_32_NORMAL)+defined(_64_NORMAL)+defined(_32_EXOTIC)+defined(_64_EXOTIC) != 1 */

#define _PAD_TO_64(Dummy)
#if defined(_32_NORMAL) || defined(_64_NORMAL)
typedef BYTE  _BYTE;
typedef WORD  _WORD;
//typedef DWORD _DWORD;
#else
#pragma message ("Warning: real memory usage will be twice larger")
typedef WORD  _BYTE;
typedef DWORD _WORD;
#if defined(_32_EXOTIC)
//typedef DWORD _DWORD;
#undef _PAD_TO_64
#define _PAD_TO_64(Dummy) DWORD Dummy;
#else
//typedef QWORD _DWORD;
#endif
#endif /* defined(_32_NORMAL) || defined(_64_NORMAL) */
#include <stdint.h>
typedef intptr_t    _DWORD;

#if !defined(NDEBUG)
BOOL TestCompilation();                     /* for testing our data types   */
#endif /* !defined(NDEBUG) */

#define _FASTCALL
#define _STDCALL

/* _USE_THREAD_KEYWORD macro must be defined at compilation for creation    *
 * of multithreading applications. Some compilers generate correct code     *
 * with the use of standard '__thread' keyword (GNU C), some others use it  *
 * in non-standard way (BorlandC) and some use __declspec(thread) keyword   *
 * (IntelC, VisualC).                                                       */
//#define _USE_THREAD_KEYWORD
#if defined(_USE_THREAD_KEYWORD)
#if defined(_MSC_VER)
#define _THREAD
#define _THREAD1 __declspec(thread)
#elif defined(__GNUC__)
#define _THREAD
#define _THREAD1 __thread
#else /* __BORLANDC__ */
#define _THREAD __thread
#define _THREAD1
#endif /* defined(_MSC_VER) */
#else
#define _THREAD
#define _THREAD1
#endif /* defined(_USE_THREAD_KEYWORD) */

static const DWORD PPMdSignature=0x84ACAF8F;
enum { PROG_VAR='J', MAX_O=16 };            /* maximum allowed model order  */
#define _USE_PREFETCHING                    /* it gives 2-6% speed gain     */

template <class T>
static T CLAMP(const T& X,const T& LoX,const T& HiX) { return (X >= LoX)?((X <= HiX)?(X):(HiX)):(LoX); }
template <class T>
static void SWAP(T& t1,T& t2) { T tmp=t1; t1=t2; t2=tmp; }

/* PPMd module works with file streams via ...GETC/...PUTC macros only      */
typedef unsigned char _PPMD_FILE;
#define _PPMD_E_GETC(fp)   xgetc(fp)
#define _PPMD_E_PUTC(c,fp) xputc((c),fp)
#define _PPMD_D_GETC(fp)   xgetc(fp)
#define _PPMD_D_PUTC(c,fp) xputc((c),fp)
/******************  Example of C++ buffered stream  ************************
class PRIME_STREAM {
public:
enum { BUF_SIZE=64*1024 };
    PRIME_STREAM(): Error(0), StrPos(0), Count(0), p(Buf) {}
    int  get(     ) { return (--Count >= 0)?(*p++    ):( fill( )); }
    int  put(int c) { return (--Count >= 0)?(*p++ = c):(flush(c)); }
    int  getErr() const { return Error; }
    int    tell() const { return StrPos+(p-Buf); }
    BOOL  atEOS() const { return (Count < 0); }
protected:
    int Error, StrPos, Count;
    BYTE* p, Buf[BUF_SIZE];
    virtual int  fill(     ) = 0;           // it must fill Buf[]
    virtual int flush(int c) = 0;           // it must remove (p-Buf) bytes
};
typedef PRIME_STREAM _PPMD_FILE;
#define _PPMD_E_GETC(pps)   (pps)->get()
#define _PPMD_E_PUTC(c,pps) (pps)->put(c)
#define _PPMD_D_GETC(pps)   (pps)->get()
#define _PPMD_D_PUTC(c,pps) (pps)->put(c)
**************************  End of example  *********************************/

#endif /* !defined(_PPMDTYPE_H_) */

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001, 2006                                                         *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/
enum { UNIT_SIZE=12, N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4,
        N_INDEXES=N1+N2+N3+N4 };

static void PrefetchData(void* Addr)
{
#if defined(_USE_PREFETCHING)
    //BYTE PrefetchByte = *(volatile BYTE*)Addr;
#endif /* defined(_USE_PREFETCHING) */
}
static BYTE Indx2Units[N_INDEXES], Units2Indx[128]; // constants
static _THREAD1 UINT _THREAD GlueCount, _THREAD GlueCount1, _THREAD SubAllocatorSize=0;
static _THREAD1 _BYTE* _THREAD HeapStart, * _THREAD pText, * _THREAD UnitsStart;
static _THREAD1 _BYTE* _THREAD LoUnit, * _THREAD HiUnit, * _THREAD AuxUnit;

#if defined(_32_NORMAL) || defined(_64_EXOTIC)
static _DWORD Ptr2Indx(void* p) { return (_DWORD)p; }
static void*  Indx2Ptr(_DWORD indx) { return (void*)indx; }
#else
static _THREAD1 _BYTE* _THREAD HeapNull;
static _DWORD Ptr2Indx(void* p) { return ((_BYTE*)p)-HeapNull; }
static void*  Indx2Ptr(_DWORD indx) { return (void*)(HeapNull+indx); }
#endif /* defined(_32_NORMAL) || defined(_64_EXOTIC) */

#pragma pack(1)
static _THREAD1 struct BLK_NODEj {
    _DWORD Stamp;
    _PAD_TO_64(Dummy1)
    _DWORD NextIndx;
    _PAD_TO_64(Dummy2)
    BLK_NODEj*   getNext() const      { return (BLK_NODEj*)Indx2Ptr(NextIndx); }
    void        setNext(BLK_NODEj* p) { NextIndx=Ptr2Indx(p); }
    BOOL          avail() const      { return (NextIndx != 0); }
    void           link(BLK_NODEj* p) { p->NextIndx=NextIndx; setNext(p); }
    void         unlink()            { NextIndx=getNext()->NextIndx; }
    void* remove();
    void  insert(void* pv,int NU);
} _THREAD BList[N_INDEXES+1];
struct MEM_BLKj: public BLK_NODEj { _DWORD NU; _PAD_TO_64(Dummy3) };
#pragma pack()

void* BLK_NODEj::remove() {
    BLK_NODEj* p=getNext();                  unlink();
    Stamp--;                                return p;
}
void BLK_NODEj::insert(void* pv,int NU) {
    MEM_BLKj* p=(MEM_BLKj*)pv;                link(p);
    p->Stamp=~_DWORD(0);                    p->NU=NU;
    Stamp++;
}
static UINT U2B(UINT NU) { return 8*NU+4*NU; }
static void SplitBlock(void* pv,UINT OldIndx,UINT NewIndx)
{
    UINT i, k, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
    _BYTE* p=((_BYTE*)pv)+U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) {
        k=Indx2Units[--i];                  BList[i].insert(p,k);
        p += U2B(k);                        UDiff -= k;
    }
    BList[Units2Indx[UDiff-1]].insert(p,UDiff);
}
static UINT _STDCALL GetUsedMemory()
{
    UINT i, RetVal=SubAllocatorSize-(HiUnit-LoUnit)-(UnitsStart-pText);
    for (i=0;i < N_INDEXES;i++)
            RetVal -= U2B(Indx2Units[i]*BList[i].Stamp);
    return RetVal;
}
static void _STDCALL StopSubAllocator() {
    if ( SubAllocatorSize ) {
        SubAllocatorSize=0;                 delete HeapStart;
    }
}
static BOOL _STDCALL StartSubAllocator(UINT SASize)
{
    UINT t=SASize << 20U;
    if (SubAllocatorSize == t)              return TRUE;
    StopSubAllocator();
    if ((HeapStart = new _BYTE[t]) == NULL) return FALSE;
    SubAllocatorSize=t;                     return TRUE;
}
static void InitSubAllocator()
{
    memset(BList,0,sizeof(BList));
    HiUnit=(pText=HeapStart)+SubAllocatorSize;
    UINT Diff=U2B(SubAllocatorSize/8/UNIT_SIZE*7);
    LoUnit=UnitsStart=HiUnit-Diff;          GlueCount=GlueCount1=0;
#if !defined(_32_NORMAL) && !defined(_64_EXOTIC)
    HeapNull=HeapStart-1;
#endif /* !defined(_32_NORMAL) && !defined(_64_EXOTIC) */
}
static void GlueFreeBlocks()
{
    UINT i, k, sz;
    MEM_BLKj s0, * p, * p0, * p1;
    if (LoUnit != HiUnit)                   *LoUnit=0;
    for ((p0=&s0)->NextIndx=i=0;i <= N_INDEXES;i++)
            while ( BList[i].avail() ) {
                p=(MEM_BLKj*)BList[i].remove();
                if ( !p->NU )               continue;
                while ((p1=p+p->NU)->Stamp == ~_DWORD(0)) {
                    p->NU += p1->NU;        p1->NU=0;
                }
                p0->link(p);                p0=p;
            }
    while ( s0.avail() ) {
        p=(MEM_BLKj*)s0.remove();            sz=p->NU;
        if ( !sz )                          continue;
        for ( ;sz > 128;sz -= 128, p += 128)
                BList[N_INDEXES-1].insert(p,128);
        if (Indx2Units[i=Units2Indx[sz-1]] != sz) {
            k=sz-Indx2Units[--i];           BList[k-1].insert(p+(sz-k),k);
        }
        BList[i].insert(p,Indx2Units[i]);
    }
    GlueCount=1 << (13+GlueCount1++);
}
static void* _STDCALL AllocUnitsRare(UINT indx)
{
    UINT i=indx;
    do {
        if (++i == N_INDEXES) {
            if ( !GlueCount-- ) {
                GlueFreeBlocks();
                if (BList[i=indx].avail())  return BList[i].remove();
            } else {
                i=U2B(Indx2Units[indx]);
                return (UnitsStart-pText > i)?(UnitsStart -= i):(NULL);
            }
        }
    } while ( !BList[i].avail() );
    void* RetVal=BList[i].remove();         SplitBlock(RetVal,i,indx);
    return RetVal;
}
static void* AllocUnits(UINT NU)
{
    UINT indx=Units2Indx[NU-1];
    if ( BList[indx].avail() )              return BList[indx].remove();
    void* RetVal=LoUnit;                    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit)                   return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);        return AllocUnitsRare(indx);
}
static void* AllocContext()
{
    if (HiUnit != LoUnit)                   return (HiUnit -= UNIT_SIZE);
    return (BList->avail())?(BList->remove()):(AllocUnitsRare(0));
}
static void UnitsCpy(void* Dest,void* Src,UINT NU)
{
#if defined(_32_NORMAL) || defined(_64_NORMAL)
    DWORD* p1=(DWORD*)Dest, * p2=(DWORD*)Src;
    do {
        p1[0]=p2[0];                        p1[1]=p2[1];
        p1[2]=p2[2];
        p1 += 3;                            p2 += 3;
    } while ( --NU );
#else
    MEM_BLKj* p1=(MEM_BLKj*)Dest, * p2=(MEM_BLKj*)Src;
    do { *p1++ = *p2++; } while ( --NU );
#endif /* defined(_32_NORMAL) || defined(_64_NORMAL) */
}
static void* ExpandUnits(void* OldPtr,UINT OldNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           return OldPtr;
    void* ptr=AllocUnits(OldNU+1);
    if (ptr) { UnitsCpy(ptr,OldPtr,OldNU);  BList[i0].insert(OldPtr,OldNU); }
    return ptr;
}
static void* ShrinkUnits(void* OldPtr,UINT OldNU,UINT NewNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
    if (i0 == i1)                           return OldPtr;
    if ( BList[i1].avail() ) {
        void* ptr=BList[i1].remove();       UnitsCpy(ptr,OldPtr,NewNU);
        BList[i0].insert(OldPtr,Indx2Units[i0]);
        return ptr;
    } else { SplitBlock(OldPtr,i0,i1);      return OldPtr; }
}
static void FreeUnits(void* ptr,UINT NU) {
    UINT indx=Units2Indx[NU-1];
    BList[indx].insert(ptr,Indx2Units[indx]);
}
static void FreeUnit(void* ptr)
{
    BList[((_BYTE*)ptr > UnitsStart+128*1024)?(0):(N_INDEXES)].insert(ptr,1);
}
static void* MoveUnitsUp(void* OldPtr,UINT NU)
{
    UINT indx;                              PrefetchData(OldPtr);
    if ((_BYTE*)OldPtr > UnitsStart+128*1024 ||
        (BLK_NODEj*)OldPtr > BList[indx=Units2Indx[NU-1]].getNext())
            return OldPtr;
    void* ptr=BList[indx].remove();         UnitsCpy(ptr,OldPtr,NU);
    BList[N_INDEXES].insert(OldPtr,Indx2Units[indx]);
    return ptr;
}
static void PrepareTextArea()
{
    AuxUnit = (_BYTE*)AllocContext();
    if ( !AuxUnit )                         AuxUnit = UnitsStart;
    else if (AuxUnit == UnitsStart)         AuxUnit = (UnitsStart += UNIT_SIZE);
}
static void ExpandTextArea()
{
    BLK_NODEj* p;
    UINT Count[N_INDEXES], i=0;             memset(Count,0,sizeof(Count));
    if (AuxUnit != UnitsStart) {
        if(*(_DWORD*)AuxUnit != ~_DWORD(0)) UnitsStart += UNIT_SIZE;
        else                                BList->insert(AuxUnit,1);
    }
    while ((p=(BLK_NODEj*)UnitsStart)->Stamp == ~_DWORD(0)) {
        MEM_BLKj* pm=(MEM_BLKj*)p;            UnitsStart=(_BYTE*)(pm+pm->NU);
        Count[Units2Indx[pm->NU-1]]++;      i++;
        pm->Stamp=0;
    }
    if ( !i )                               return;
    for (p=BList+N_INDEXES;p->NextIndx;p=p->getNext()) {
        while (p->NextIndx && !p->getNext()->Stamp) {
            Count[Units2Indx[((MEM_BLKj*)p->getNext())->NU-1]]--;
            p->unlink();                    BList[N_INDEXES].Stamp--;
        }
        if ( !p->NextIndx )                 break;
    }
    for (i=0;i < N_INDEXES;i++)
        for (p=BList+i;Count[i] != 0;p=p->getNext())
            while ( !p->getNext()->Stamp ) {
                p->unlink();                BList[i].Stamp--;
                if ( !--Count[i] )          break;
            }
}

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

enum { TOP=1 << 24, BOT=1 << 15 };
static _THREAD1 struct SUBRANGE { DWORD low, high, scale; } _THREAD Range;
static _THREAD1 DWORD _THREAD low, _THREAD code, _THREAD range;

static void rcInitEncoder() { low=0; range=DWORD(-1); }
#define RC_ENC_NORMALIZE(stream) {                                          \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        _PPMD_E_PUTC(low >> 24,stream);                                     \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
static void rcEncodeSymbol()
{
    low += Range.low*(range/=Range.scale);  range *= Range.high-Range.low;
}
static void rcFlushEncoder(_PPMD_FILE* stream)
{
    for (UINT i=0;i < 4;i++) {
        _PPMD_E_PUTC(low >> 24,stream);     low <<= 8;
    }
}
static void rcInitDecoder(_PPMD_FILE* stream)
{
    low=code=0;                             range=DWORD(-1);
    for (UINT i=0;i < 4;i++)
            code=(code << 8) | _PPMD_D_GETC(stream);
}
#define RC_DEC_NORMALIZE(stream) {                                          \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | _PPMD_D_GETC(stream);                            \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
static UINT rcGetCurrentCount() { return (code-low)/(range /= Range.scale); }
static void rcRemoveSubrange()
{
    low += range*Range.low;                 range *= Range.high-Range.low;
}
static UINT rcBinStart(UINT f0,UINT Shift)  { return f0*(range >>= Shift); }
static UINT rcBinDecode  (UINT tmp)         { return (code-low >= tmp); }
static void rcBinCorrect0(UINT tmp)         { range=tmp; }
static void rcBinCorrect1(UINT tmp,UINT f1) { low += tmp;   range *= f1; }
