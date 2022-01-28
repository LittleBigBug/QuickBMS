/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;

static const DWORD PPMdSignature=0x84ACAF8F, Variant='I';
static const int MAX_O=16;                         /* maximum allowed model order  */

//#define _USE_PREFETCHING                    /* for puzzling mainly          */
#if defined(__GNUC__)
#define _PACK_ATTR __attribute__ ((packed))
#else /* "#pragma pack" is used for other compilers */
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

static int xgetc(void *X) {
    if(EncodedFile >= EncodedFilel) return(-1);
    return(*EncodedFile++);
}
static int xputc(int chr, void *X) {
    if(DecodedFile >= DecodedFilel) return(-1);
    *DecodedFile++ = chr;
    return(chr);
}
#define _PPMD_D_GETC(fp)   xgetc(fp)
#define _PPMD_D_PUTC(c,fp) xputc((c),fp)

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Method of model restoration at memory insufficiency:                     *
 *     MRM_RESTART - restart model from scratch (default)                   *
 *     MRM_CUT_OFF - cut off model (nearly twice slower)                    *
 *     MRM_FREEZE  - freeze context tree (dangerous)                        */
enum MR_METHOD { MRM_RESTART, MRM_CUT_OFF, MRM_FREEZE };

#ifdef  __cplusplus
}
#endif

enum { UNIT_SIZE=12, N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4,
        N_INDEXES=N1+N2+N3+N4 };

#pragma pack(1)
static struct BLK_NODEi {
    DWORD Stamp;
    BLK_NODEi* next;
    BOOL   avail()      const { return (next != NULL); }
    void    link(BLK_NODEi* p) { p->next=next; next=p; }
    void  unlink()            { next=next->next; }
    void* remove()            {
        BLK_NODEi* p=next;                   unlink();
        Stamp--;                            return p;
    }
    void insert(void* pv,int NU);
} BList[N_INDEXES];
struct MEM_BLKi: public BLK_NODEi { DWORD NU; } _PACK_ATTR;
#pragma pack()

static BYTE Indx2Units[N_INDEXES], Units2Indx[128]; // constants
static DWORD GlueCount, SubAllocatorSize=0;
static BYTE* HeapStart, * pText, * UnitsStart, * LoUnit, * HiUnit;

static void PrefetchData(void* Addr)
{
//#if defined(_USE_PREFETCHING)
    //BYTE PrefetchByte = *(volatile BYTE*) Addr;
//#endif /* defined(_USE_PREFETCHING) */
}
void BLK_NODEi::insert(void* pv,int NU) {
    MEM_BLKi* p=(MEM_BLKi*) pv;               link(p);
    p->Stamp=~0UL;                          p->NU=NU;
    Stamp++;
}
static UINT U2B(UINT NU) { return 8*NU+4*NU; }
static void SplitBlock(void* pv,UINT OldIndx,UINT NewIndx)
{
    UINT i, k, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
    BYTE* p=((BYTE*) pv)+U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) {
        k=Indx2Units[--i];                  BList[i].insert(p,k);
        p += U2B(k);                        UDiff -= k;
    }
    BList[Units2Indx[UDiff-1]].insert(p,UDiff);
}
static DWORD GetUsedMemory()
{
    DWORD i, RetVal=SubAllocatorSize-(HiUnit-LoUnit)-(UnitsStart-pText);
    for (i=0;i < N_INDEXES;i++)
            RetVal -= UNIT_SIZE*Indx2Units[i]*BList[i].Stamp;
    return RetVal;
}
static void StopSubAllocator() {
    if ( SubAllocatorSize ) {
        SubAllocatorSize=0;                 free(HeapStart);
    }
}
static BOOL StartSubAllocator(UINT SASize)
{
    DWORD t=SASize << 20U;
    if (SubAllocatorSize == t)              return TRUE;
    StopSubAllocator();
    if ((HeapStart=(BYTE *)calloc(t,1)) == NULL)    return FALSE;
    SubAllocatorSize=t;                     return TRUE;
}
static void InitSubAllocator()
{
    memset(BList,0,sizeof(BList));
    HiUnit=(pText=HeapStart)+SubAllocatorSize;
    UINT Diff=UNIT_SIZE*(SubAllocatorSize/8/UNIT_SIZE*7);
    LoUnit=UnitsStart=HiUnit-Diff;          GlueCount=0;
}
static void GlueFreeBlocks()
{
    UINT i, k, sz;
    MEM_BLKi s0, * p, * p0, * p1;
    if (LoUnit != HiUnit)                   *LoUnit=0;
    for (i=0, (p0=&s0)->next=NULL;i < N_INDEXES;i++)
            while ( BList[i].avail() ) {
                p=(MEM_BLKi*) BList[i].remove();
                if ( !p->NU )               continue;
                while ((p1=p+p->NU)->Stamp == ~0UL) {
                    p->NU += p1->NU;        p1->NU=0;
                }
                p0->link(p);                p0=p;
            }
    while ( s0.avail() ) {
        p=(MEM_BLKi*) s0.remove();           sz=p->NU;
        if ( !sz )                          continue;
        for ( ;sz > 128;sz -= 128, p += 128)
                BList[N_INDEXES-1].insert(p,128);
        if (Indx2Units[i=Units2Indx[sz-1]] != sz) {
            k=sz-Indx2Units[--i];           BList[k-1].insert(p+(sz-k),k);
        }
        BList[i].insert(p,Indx2Units[i]);
    }
    GlueCount=1 << 13;
}
static void* AllocUnitsRare(UINT indx)
{
    UINT i=indx;
    if ( !GlueCount ) {
        GlueFreeBlocks();
        if ( BList[i].avail() )             return BList[i].remove();
    }
    do {
        if (++i == N_INDEXES) {
            GlueCount--;                    i=U2B(Indx2Units[indx]);
            return (UnitsStart-pText > (int)i)?(UnitsStart -= i):(NULL);
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
    else if ( BList->avail() )              return BList->remove();
    else                                    return AllocUnitsRare(0);
}
static void UnitsCpy(void* Dest,void* Src,UINT NU)
{
    DWORD* p1=(DWORD*) Dest, * p2=(DWORD*) Src;
    do {
        p1[0]=p2[0];                        p1[1]=p2[1];
        p1[2]=p2[2];
        p1 += 3;                            p2 += 3;
    } while ( --NU );
}
static void* ExpandUnits(void* OldPtr,UINT OldNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           return OldPtr;
    void* ptr=AllocUnits(OldNU+1);
    if ( ptr ) {
        UnitsCpy(ptr,OldPtr,OldNU);         BList[i0].insert(OldPtr,OldNU);
    }
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
    } else {
        SplitBlock(OldPtr,i0,i1);           return OldPtr;
    }
}
static void FreeUnits(void* ptr,UINT NU) {
    UINT indx=Units2Indx[NU-1];
    BList[indx].insert(ptr,Indx2Units[indx]);
}
static void SpecialFreeUnit(void* ptr)
{
    if ((BYTE*) ptr != UnitsStart)          BList->insert(ptr,1);
    else { *(DWORD*) ptr=~0UL;              UnitsStart += UNIT_SIZE; }
}
static void* MoveUnitsUp(void* OldPtr,UINT NU)
{
    UINT indx=Units2Indx[NU-1];
    if ((BYTE*) OldPtr > UnitsStart+16*1024 || (BLK_NODEi*) OldPtr > BList[indx].next)
            return OldPtr;
    void* ptr=BList[indx].remove();
    UnitsCpy(ptr,OldPtr,NU);                NU=Indx2Units[indx];
    if ((BYTE*) OldPtr != UnitsStart)       BList[indx].insert(OldPtr,NU);
    else                                    UnitsStart += U2B(NU);
    return ptr;
}
static void ExpandTextArea()
{
    BLK_NODEi* p;
    UINT Count[N_INDEXES];                  memset(Count,0,sizeof(Count));
    while ((p=(BLK_NODEi*) UnitsStart)->Stamp == ~0UL) {
        MEM_BLKi* pm=(MEM_BLKi*) p;           UnitsStart=(BYTE*) (pm+pm->NU);
        Count[Units2Indx[pm->NU-1]]++;      pm->Stamp=0;
    }
    for (UINT i=0;i < N_INDEXES;i++)
        for (p=BList+i;Count[i] != 0;p=p->next)
            while ( !p->next->Stamp ) {
                p->unlink();                BList[i].Stamp--;
                if ( !--Count[i] )          break;
            }
}

static struct SUBRANGE {
    DWORD LowCount, HighCount, scale;
} SubRange;
enum { TOP=1 << 24, BOT=1 << 15 };
static DWORD low, code, range;
#define ARI_INIT_DECODER(stream) {                                          \
    low=code=0;                             range=DWORD(-1);                \
    for (UINT i=0;i < 4;i++)                                                \
            code=(code << 8) | _PPMD_D_GETC(stream);                        \
}
#define ARI_DEC_NORMALIZE(stream) {                                         \
    while (((low ^ (low+range)) < TOP) || ((range < BOT) &&                 \
            ((range= -low & (BOT-1)),1))) {                                 \
        code=(code << 8) | _PPMD_D_GETC(stream);                            \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
static UINT ariGetCurrentCount() {
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
