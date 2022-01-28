// modified and fixed by Luigi Auriemma for memory2memory decompression (rev2)
// note that the original code is... horrible and not only not portable but even
// incompatible using some tunes or optimizations (for example -mtune can't be used)
// I hope to have fixed it a bit but it's probably simpler to rewrite everything from scratch...

/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001, 2010                                                         *
 *  Contents: PPMII model description and encoding/decoding routines        *
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>
static unsigned char   *EncodedFile  = NULL;
static unsigned char   *EncodedFilel = NULL;
static unsigned char   *DecodedFile  = NULL;
static unsigned char   *DecodedFilel = NULL;
#include "unppmdi_internal.h"

enum { UP_FREQ=5, INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
    INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124, O_BOUND=9 };

#pragma pack(1)
static struct SEE2i_CONTEXT { // SEE-contexts for PPM-contexts with masked symbols
    WORD Summ;
    BYTE Shift, Count;
    void init(UINT InitVal) { Summ=InitVal << (Shift=PERIOD_BITS-4); Count=7; }
    UINT getMean() {
        UINT RetVal=(Summ >> Shift);        Summ -= RetVal;
        return RetVal+(RetVal == 0);
    }
    void update() {
        if (Shift < PERIOD_BITS && --Count == 0) {
            Summ += Summ;                   Count=3 << Shift++;
        }
    }
} _PACK_ATTR SEE2Cont[24][32], DummySEE2Cont;
static struct PPMi_CONTEXT {                 // Notes:
    BYTE NumStats, Flags;                   // 1. NumStats & NumMasked contain
    WORD SummFreq;                          //  number of symbols minus 1
    struct STATE {                          // 2. sizeof(WORD) > sizeof(BYTE)
        BYTE Symbol, Freq;                  // 3. contexts example:
        PPMi_CONTEXT* Successor;             // MaxOrder:
    } _PACK_ATTR * Stats;                   //  ABCD    context
    PPMi_CONTEXT* Suffix;                    //   BCD    suffix
    void           decodeBinSymbol();//    CD    suffix
    void             decodeSymbol1();//   BCDE   successor
    void             decodeSymbol2();
    void           update1(STATE* p);
    void           update2(STATE* p);
    SEE2i_CONTEXT*     makeEscFreq2();
    void                          rescale();
    void      refresh(int OldNU,BOOL Scale);
    PPMi_CONTEXT*          cutOff(int Order);
    PPMi_CONTEXT*  removeBinConts(int Order);
    STATE& oneState() const { return (STATE&) SummFreq; }
} _PACK_ATTR* MaxContext;
#pragma pack()

static BYTE NS2BSIndx[256], QTable[260];    // constants
static PPMi_CONTEXT::STATE* FoundState;      // found next state transition
static int  InitEsc, OrderFall, RunLength, InitRL, MaxOrder;
static BYTE CharMask[256], NumMasked, PrevSuccess, EscCount/*, PrintCount*/;
static WORD BinSumm[25][64];                // binary SEE-contexts
static MR_METHOD MRMethod;

static void StateCpy(PPMi_CONTEXT::STATE& s1,const PPMi_CONTEXT::STATE& s2)
{
    //(WORD&) s1=(WORD&) s2;                  s1.Successor=s2.Successor;
    s1.Symbol = s2.Symbol;
    s1.Freq   = s2.Freq;
    s1.Successor=s2.Successor;
}
static void SWAP(PPMi_CONTEXT::STATE& s1,PPMi_CONTEXT::STATE& s2)
{
    //WORD t1=(WORD&) s1;                     PPMi_CONTEXT* t2=s1.Successor;
    //(WORD&) s1 = (WORD&) s2;                s1.Successor=s2.Successor;
    //(WORD&) s2 = t1;                        s2.Successor=t2;
    PPMi_CONTEXT::STATE  t1;
    StateCpy(t1, s1);
    StateCpy(s1, s2);
    StateCpy(s2, t1);
}
struct PPMD_STARTUPi { PPMD_STARTUPi(); } PPMd_StartUpi;
PPMD_STARTUPi::PPMD_STARTUPi()         // constants initialization
{
    UINT i, k, m, Step;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
    NS2BSIndx[0]=2*0;                       NS2BSIndx[1]=2*1;
    memset(NS2BSIndx+2,2*2,9);              memset(NS2BSIndx+11,2*3,256-11);
    for (i=0;i < UP_FREQ;i++)               QTable[i]=i;
    for (m=i=UP_FREQ, k=Step=1;i < 260;i++) {
        QTable[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
    //(DWORD&) DummySEE2Cont=PPMdSignature;
    DummySEE2Cont.Summ=(WORD)(PPMdSignature);
    DummySEE2Cont.Shift=(BYTE)(PPMdSignature>>16);
    DummySEE2Cont.Count=(BYTE)(PPMdSignature>>24);
}
static void StartModelRare(int MaxOrder,MR_METHOD MRMethod)
{
    UINT i, k, m;
    memset(CharMask,0,sizeof(CharMask));    EscCount=/*PrintCount=*/1;
    if (MaxOrder < 2) {                     // we are in solid mode
        OrderFall=::MaxOrder;
        for (PPMi_CONTEXT* pc=MaxContext;pc->Suffix != NULL;pc=pc->Suffix)
                OrderFall--;
        return;
    }
    OrderFall=::MaxOrder=MaxOrder;          ::MRMethod=MRMethod;
    InitSubAllocator();
    RunLength=InitRL=-((MaxOrder < 12)?MaxOrder:12)-1;
    MaxContext = (PPMi_CONTEXT*) AllocContext();
    MaxContext->Suffix=NULL;
    MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
    MaxContext->Stats = (PPMi_CONTEXT::STATE*) AllocUnits(256/2);
    for (PrevSuccess=i=0;i < 256;i++) {
        MaxContext->Stats[i].Symbol=i;      MaxContext->Stats[i].Freq=1;
        MaxContext->Stats[i].Successor=NULL;
    }
static const WORD InitBinEsc[]={0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051};
    for (i=m=0;m < 25;m++) {
        while (QTable[i] == m)              i++;
        for (k=0;k < 8;k++)
                BinSumm[m][k]=BIN_SCALE-InitBinEsc[k]/(i+1);
        for (k=8;k < 64;k += 8)
                memcpy(BinSumm[m]+k,BinSumm[m],8*sizeof(WORD));
    }
    for (i=m=0;m < 24;m++) {
        while (QTable[i+3] == m+3)          i++;
        SEE2Cont[m][0].init(2*i+5);
        for (k=1;k < 32;k++)                SEE2Cont[m][k]=SEE2Cont[m][0];
    }
}
void PPMi_CONTEXT::refresh(int OldNU,BOOL Scale)
{
    int i=NumStats, EscFreq;               	Scale |= (SummFreq >= 32768);
    STATE* p = Stats = (STATE*) ShrinkUnits(Stats,OldNU,(i+2) >> 1);
    Flags=(Flags & (0x10+0x04*Scale))+0x08*(p->Symbol >= 0x40);
    EscFreq=SummFreq-p->Freq;
    SummFreq = (p->Freq=(p->Freq+Scale) >> Scale);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Scale) >> Scale);
        Flags |= 0x08*(p->Symbol >= 0x40);
    } while ( --i );
    SummFreq += (EscFreq=(EscFreq+Scale) >> Scale);
}
#define P_CALL(F) ( PrefetchData(p->Successor), \
                    p->Successor=p->Successor->F(Order+1))
PPMi_CONTEXT* PPMi_CONTEXT::cutOff(int Order)
{
    int i, tmp;
    STATE* p;
    if ( !NumStats ) {
        if ((BYTE*) (p=&oneState())->Successor >= UnitsStart) {
            if (Order < MaxOrder)           P_CALL(cutOff);
            else                            p->Successor=NULL;
            if (!p->Successor && Order > O_BOUND)
                    goto REMOVE;
            return this;
        } else {
REMOVE:     SpecialFreeUnit(this);          return NULL;
        }
    }
    PrefetchData(Stats);
    Stats = (STATE*) MoveUnitsUp(Stats,tmp=(NumStats+2) >> 1);
    for (p=Stats+(i=NumStats);p >= Stats;p--)
            if ((BYTE*) p->Successor < UnitsStart) {
                p->Successor=NULL;          SWAP(*p,Stats[i--]);
            } else if (Order < MaxOrder)    P_CALL(cutOff);
            else                            p->Successor=NULL;
    if (i != NumStats && Order) {
        NumStats=i;                         p=Stats;
        if (i < 0) { FreeUnits(p,tmp);      goto REMOVE; }
        else if (i == 0) {
            Flags=(Flags & 0x10)+0x08*(p->Symbol >= 0x40);
            StateCpy(oneState(),*p);        FreeUnits(p,tmp);
            oneState().Freq=(oneState().Freq+11) >> 3;
        } else                              refresh(tmp,SummFreq > 16*i);
    }
    return this;
}
PPMi_CONTEXT* PPMi_CONTEXT::removeBinConts(int Order)
{
    STATE* p;
    if ( !NumStats ) {
        p=&oneState();
        if ((BYTE*) p->Successor >= UnitsStart && Order < MaxOrder)
                P_CALL(removeBinConts);
        else                                p->Successor=NULL;
        if (!p->Successor && (!Suffix->NumStats || Suffix->Flags == 0xFF)) {
            FreeUnits(this,1);              return NULL;
        } else                              return this;
    }
    PrefetchData(Stats);
    for (p=Stats+NumStats;p >= Stats;p--)
            if ((BYTE*) p->Successor >= UnitsStart && Order < MaxOrder)
                    P_CALL(removeBinConts);
            else                            p->Successor=NULL;
    return this;
}
static void RestoreModelRare(PPMi_CONTEXT* pc1,PPMi_CONTEXT* MinContext,
        PPMi_CONTEXT* FSuccessor)
{
    PPMi_CONTEXT* pc;
    PPMi_CONTEXT::STATE* p;
    for (pc=MaxContext, pText=HeapStart;pc != pc1;pc=pc->Suffix)
            if (--(pc->NumStats) == 0) {
                pc->Flags=(pc->Flags & 0x10)+0x08*(pc->Stats->Symbol >= 0x40);
                p=pc->Stats;                StateCpy(pc->oneState(),*p);
                SpecialFreeUnit(p);
                pc->oneState().Freq=(pc->oneState().Freq+11) >> 3;
            } else
                    pc->refresh((pc->NumStats+3) >> 1,FALSE);
    for ( ;pc != MinContext;pc=pc->Suffix)
            if ( !pc->NumStats )
                    pc->oneState().Freq -= pc->oneState().Freq >> 1;
            else if ((pc->SummFreq += 4) > 128+4*pc->NumStats)
                    pc->refresh((pc->NumStats+2) >> 1,TRUE);
    if (MRMethod > MRM_FREEZE) {
        MaxContext=FSuccessor;              GlueCount += !(BList[1].Stamp & 1);
    } else if (MRMethod == MRM_FREEZE) {
        while ( MaxContext->Suffix )        MaxContext=MaxContext->Suffix;
        MaxContext->removeBinConts(0);      MRMethod=MR_METHOD(MRMethod+1);
        GlueCount=0;                        OrderFall=MaxOrder;
    } else if (MRMethod == MRM_RESTART || GetUsedMemory() < (SubAllocatorSize >> 1)) {
        StartModelRare(MaxOrder,MRMethod);
        EscCount=0;                         //PrintCount=0xFF;
    } else {
        while ( MaxContext->Suffix )        MaxContext=MaxContext->Suffix;
        do {
            MaxContext->cutOff(0);          ExpandTextArea();
        } while (GetUsedMemory() > 3*(SubAllocatorSize >> 2));
        GlueCount=0;                        OrderFall=MaxOrder;
    }
}
static PPMi_CONTEXT* CreateSuccessors(BOOL Skip,PPMi_CONTEXT::STATE* p,
        PPMi_CONTEXT* pc);
static PPMi_CONTEXT* ReduceOrder(PPMi_CONTEXT::STATE* p,PPMi_CONTEXT* pc)
{
    PPMi_CONTEXT::STATE* p1,  * ps[MAX_O+1], ** pps=ps;
    PPMi_CONTEXT* pc1=pc, * UpBranch = (PPMi_CONTEXT*) pText;
    BYTE tmp, sym=FoundState->Symbol;
    *pps++ = FoundState;                    FoundState->Successor=UpBranch;
    OrderFall++;
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    for ( ; ; ) {
        if ( !pc->Suffix ) {
            if (MRMethod > MRM_FREEZE) {
FROZEN:         do { (*--pps)->Successor = pc; } while (pps != ps);
                pText=HeapStart+1;          OrderFall=1;
            }
            return pc;
        }
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=2*(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else { p=&(pc->oneState());       p->Freq += (p->Freq < 32); }
LOOP_ENTRY:
        if ( p->Successor )                 break;
        *pps++ = p;                         p->Successor=UpBranch;
        OrderFall++;
    }
    if (MRMethod > MRM_FREEZE) {
        pc = p->Successor;                  goto FROZEN;
    } else if (p->Successor <= UpBranch) {
        p1=FoundState;                      FoundState=p;
        p->Successor=CreateSuccessors(FALSE,NULL,pc);
        FoundState=p1;
    }
    if (OrderFall == 1 && pc1 == MaxContext) {
        FoundState->Successor=p->Successor; pText--;
    }
    return p->Successor;
}
void PPMi_CONTEXT::rescale()
{
    UINT OldNU, Adder, EscFreq, i=NumStats;
    STATE tmp, * p1, * p;
    for (p=FoundState;p != Stats;p--)       SWAP(p[0],p[-1]);
    p->Freq += 4;                           SummFreq += 4;
    EscFreq=SummFreq-p->Freq;
    Adder=(OrderFall != 0 || MRMethod > MRM_FREEZE);
    SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            StateCpy(tmp,*(p1=p));
            do StateCpy(p1[0],p1[-1]); while (tmp.Freq > (--p1)[-1].Freq);
            StateCpy(*p1,tmp);
        }
    } while ( --i );
    if (p->Freq == 0) {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;                       OldNU=(NumStats+2) >> 1;
        if ((NumStats -= i) == 0) {
            StateCpy(tmp,*Stats);
            tmp.Freq=(2*tmp.Freq+EscFreq-1)/EscFreq;
            if (tmp.Freq > MAX_FREQ/3)      tmp.Freq=MAX_FREQ/3;
            FreeUnits(Stats,OldNU);         StateCpy(oneState(),tmp);
            Flags=(Flags & 0x10)+0x08*(tmp.Symbol >= 0x40);
            FoundState=&oneState();         return;
        }
        Stats = (STATE*) ShrinkUnits(Stats,OldNU,(NumStats+2) >> 1);
        Flags &= ~0x08;                     i=NumStats;
        Flags |= 0x08*((p=Stats)->Symbol >= 0x40);
        do { Flags |= 0x08*((++p)->Symbol >= 0x40); } while ( --i );
    }
    SummFreq += (EscFreq -= (EscFreq >> 1));
    Flags |= 0x04;                          FoundState=Stats;
}
static PPMi_CONTEXT* CreateSuccessors(BOOL Skip,PPMi_CONTEXT::STATE* p,
        PPMi_CONTEXT* pc)
{
    PPMi_CONTEXT ct, * UpBranch=FoundState->Successor;
    PPMi_CONTEXT::STATE* ps[MAX_O], ** pps=ps;
    UINT cf, s0;
    BYTE tmp, sym=FoundState->Symbol;
    if ( !Skip ) {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  goto NO_LOOP;
    }
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    do {
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else {
            p=&(pc->oneState());
            p->Freq += (!pc->Suffix->NumStats & (p->Freq < 24));
        }
LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc=p->Successor;                break;
        }
        *pps++ = p;
    } while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          return pc;
    ct.NumStats=0;                          ct.Flags=0x10*(sym >= 0x40);
    ct.oneState().Symbol=sym=*(BYTE*) UpBranch;
    ct.oneState().Successor=(PPMi_CONTEXT*) (((BYTE*) UpBranch)+1);
    ct.Flags |= 0x08*(sym >= 0x40);
    if ( pc->NumStats ) {
        if ((p=pc->Stats)->Symbol != sym)
                do { tmp=p[1].Symbol;       p++; } while (tmp != sym);
        s0=pc->SummFreq-pc->NumStats-(cf=p->Freq-1);
        ct.oneState().Freq=1+((2*cf <= s0)?(5*cf > s0):((cf+2*s0-3)/s0));
    } else
            ct.oneState().Freq=pc->oneState().Freq;
    do {
        PPMi_CONTEXT* pc1 = (PPMi_CONTEXT*) AllocContext();
        if ( !pc1 )                         return NULL;
        //((DWORD*) pc1)[0] = ((DWORD*) &ct)[0];
        //((DWORD*) pc1)[1] = ((DWORD*) &ct)[1];
        pc1->NumStats   = ct.NumStats;
        pc1->Flags      = ct.Flags;
        pc1->SummFreq   = ct.SummFreq;
        //pc1->Stats      = ct.Stats; // this one works too but I prefer memcpy
        memcpy(&pc1->Stats, &ct.Stats, sizeof(struct PPMi_CONTEXT::STATE));
        pc1->Suffix=pc;                     (*--pps)->Successor=pc=pc1;
    } while (pps != ps);
    return pc;
}
static void UpdateModel(PPMi_CONTEXT* MinContext)
{
    PPMi_CONTEXT::STATE* p=NULL;
    PPMi_CONTEXT* Successor, * FSuccessor, * pc, * pc1=MaxContext;
    UINT ns1, ns, cf, sf, s0, FFreq=FoundState->Freq;
    BYTE Flag, sym, FSymbol=FoundState->Symbol;
    FSuccessor=FoundState->Successor;       pc=MinContext->Suffix;
    if (FFreq < MAX_FREQ/4 && pc) {
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != FSymbol) {
                do { sym=p[1].Symbol;       p++; } while (sym != FSymbol);
                if (p[0].Freq >= p[-1].Freq) {
                    SWAP(p[0],p[-1]);       p--;
                }
            }
            cf=2*(p->Freq < MAX_FREQ-9);
            p->Freq += cf;                  pc->SummFreq += cf;
        } else { p=&(pc->oneState());       p->Freq += (p->Freq < 32); }
    }
    if (!OrderFall && FSuccessor) {
        FoundState->Successor=CreateSuccessors(TRUE,p,MinContext);
        if ( !FoundState->Successor )       goto RESTART_MODEL;
        MaxContext=FoundState->Successor;   return;
    }
    *pText++ = FSymbol;                     Successor = (PPMi_CONTEXT*) pText;
    if (pText >= UnitsStart)                goto RESTART_MODEL;
    if ( FSuccessor ) {
        if ((BYTE*) FSuccessor < UnitsStart)
                FSuccessor=CreateSuccessors(FALSE,p,MinContext);
    } else
                FSuccessor=ReduceOrder(p,MinContext);
    if ( !FSuccessor )                      goto RESTART_MODEL;
    if ( !--OrderFall ) {
        Successor=FSuccessor;               pText -= (MaxContext != MinContext);
    } else if (MRMethod > MRM_FREEZE) {
        Successor=FSuccessor;               pText=HeapStart;
        OrderFall=0;
    }
    s0=MinContext->SummFreq-(ns=MinContext->NumStats)-FFreq;
    for (Flag=0x08*(FSymbol >= 0x40);pc1 != MinContext;pc1=pc1->Suffix) {
        if ((ns1=pc1->NumStats) != 0) {
            if ((ns1 & 1) != 0) {
                p=(PPMi_CONTEXT::STATE*) ExpandUnits(pc1->Stats,(ns1+1) >> 1);
                if ( !p )                   goto RESTART_MODEL;
                pc1->Stats=p;
            }
            pc1->SummFreq += (3*ns1+1 < ns);
        } else {
            p=(PPMi_CONTEXT::STATE*) AllocUnits(1);
            if ( !p )                       goto RESTART_MODEL;
            StateCpy(*p,pc1->oneState());   pc1->Stats=p;
            if (p->Freq < MAX_FREQ/4-1)     p->Freq += p->Freq;
            else                            p->Freq  = MAX_FREQ-4;
            pc1->SummFreq=p->Freq+InitEsc+(ns > 2);
        }
        cf=2*FFreq*(pc1->SummFreq+6);       sf=s0+pc1->SummFreq;
        if (cf < 6*sf) {
            cf=1+(cf > sf)+(cf >= 4*sf);
            pc1->SummFreq += 4;
        } else {
            cf=4+(cf > 9*sf)+(cf > 12*sf)+(cf > 15*sf);
            pc1->SummFreq += cf;
        }
        p=pc1->Stats+(++pc1->NumStats);     p->Successor=Successor;
        p->Symbol = FSymbol;                p->Freq = cf;
        pc1->Flags |= Flag;
    }
    MaxContext=FSuccessor;                  return;
RESTART_MODEL:
    RestoreModelRare(pc1,MinContext,FSuccessor);
}
// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))
void PPMi_CONTEXT::decodeBinSymbol()
{
    BYTE indx=NS2BSIndx[Suffix->NumStats]+PrevSuccess+Flags;
    STATE& rs=oneState();
    WORD& bs=BinSumm[QTable[rs.Freq-1]][indx+((RunLength >> 26) & 0x20)];
    if (ariGetCurrentShiftCount(TOT_BITS) < bs) {
        FoundState=&rs;                     rs.Freq += (rs.Freq < 196);
        SubRange.LowCount=0;                SubRange.HighCount=bs;
        bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        PrevSuccess=1;                      RunLength++;
    } else {
        SubRange.LowCount=bs;               bs -= GET_MEAN(bs,PERIOD_BITS,2);
        SubRange.HighCount=BIN_SCALE;       InitEsc=ExpEscape[bs >> 10];
        CharMask[rs.Symbol]=EscCount;
        NumMasked=PrevSuccess=0;            FoundState=NULL;
    }
}
void PPMi_CONTEXT::update1(STATE* p)
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) {
        SWAP(p[0],p[-1]);                   FoundState=--p;
        if (p->Freq > MAX_FREQ)             rescale();
    }
}
void PPMi_CONTEXT::decodeSymbol1()
{
    UINT i, count, HiCnt=Stats->Freq;
    STATE* p=Stats;                         SubRange.scale=SummFreq;
    if ((count=ariGetCurrentCount()) < HiCnt) {
        PrevSuccess=(2*(SubRange.HighCount=HiCnt) >= SubRange.scale);
        (FoundState=p)->Freq=(HiCnt += 4);  SummFreq += 4;
        RunLength += PrevSuccess;
        if (HiCnt > MAX_FREQ)               rescale();
        SubRange.LowCount=0;                return;
    }
    i=NumStats;                             PrevSuccess=0;
    while ((HiCnt += (++p)->Freq) <= count)
        if (--i == 0) {
            if ( Suffix )                   PrefetchData(Suffix);
            SubRange.LowCount=HiCnt;        CharMask[p->Symbol]=EscCount;
            i=NumMasked=NumStats;           FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            SubRange.HighCount=SubRange.scale;
            return;
        }
    SubRange.LowCount=(SubRange.HighCount=HiCnt)-p->Freq;
    update1(p);
}
void PPMi_CONTEXT::update2(STATE* p)
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p->Freq > MAX_FREQ)                 rescale();
    EscCount++;                             RunLength=InitRL;
}
SEE2i_CONTEXT* PPMi_CONTEXT::makeEscFreq2()
{
    BYTE* pb=(BYTE*) Stats;                 UINT t=2*NumStats;
    PrefetchData(pb);                       PrefetchData(pb+t);
    PrefetchData(pb += 2*t);                PrefetchData(pb+t);
    SEE2i_CONTEXT* psee2c;
    if (NumStats != 0xFF) {
        t=Suffix->NumStats;
        psee2c=SEE2Cont[QTable[NumStats+2]-3]+(SummFreq > 11*(NumStats+1));
        psee2c += 2*(2*NumStats < t+NumMasked)+Flags;
        SubRange.scale=psee2c->getMean();
    } else {
        psee2c=&DummySEE2Cont;              SubRange.scale=1;
    }
    return psee2c;
}
void PPMi_CONTEXT::decodeSymbol2()
{
    SEE2i_CONTEXT* psee2c=makeEscFreq2();
    UINT Sym, count, HiCnt=0, i=NumStats-NumMasked;
    STATE* ps[256], ** pps=ps, * p=Stats-1;
    do {
        do { Sym=p[1].Symbol;   p++; } while (CharMask[Sym] == EscCount);
        HiCnt += p->Freq;                   *pps++ = p;
    } while ( --i );
    SubRange.scale += HiCnt;                count=ariGetCurrentCount();
    p=*(pps=ps);
    if (count < HiCnt) {
        HiCnt=0;
        while ((HiCnt += p->Freq) <= count) p=*++pps;
        SubRange.LowCount = (SubRange.HighCount=HiCnt)-p->Freq;
        psee2c->update();                   update2(p);
    } else {
        SubRange.LowCount=HiCnt;            SubRange.HighCount=SubRange.scale;
        i=NumStats-NumMasked;               NumMasked = NumStats;
        do { CharMask[(*pps)->Symbol]=EscCount; pps++; } while ( --i );
        psee2c->Summ += SubRange.scale;
    }
}
static void ClearMask(void)
{
    EscCount=1;                             memset(CharMask,0,sizeof(CharMask));
}

// SaSize   is -mN  (default 10)
// MaxOrder is -oN  (default 4)
// MRMethod is -rN  (default 0)

extern "C" int unppmdi_raw(unsigned char *in, int insz, unsigned char *out, int outsz, int SaSize, int MaxOrder, MR_METHOD MRMethod) {
    EncodedFile  = in;
    EncodedFilel = in + insz;
    DecodedFile  = out;
    DecodedFilel = out + outsz;

    if(SaSize   <= 0) SaSize   = 10;
    if(MaxOrder <= 0) MaxOrder = 4;

    StartSubAllocator(SaSize);

    ARI_INIT_DECODER(EncodedFile);          StartModelRare(MaxOrder,MRMethod);
    PPMi_CONTEXT* MinContext=MaxContext;
    for (BYTE ns=MinContext->NumStats; ; ) {
        ( ns )?(MinContext->decodeSymbol1()):(MinContext->decodeBinSymbol());
        ariRemoveSubrange();
        while ( !FoundState ) {
            ARI_DEC_NORMALIZE(EncodedFile);
            do {
                OrderFall++;                MinContext=MinContext->Suffix;
                if ( !MinContext )          goto STOP_DECODING;
            } while (MinContext->NumStats == NumMasked);
            MinContext->decodeSymbol2();    ariRemoveSubrange();
        }
        _PPMD_D_PUTC(FoundState->Symbol,DecodedFile);
        if (!OrderFall && (BYTE*) FoundState->Successor >= UnitsStart)
                PrefetchData(MaxContext=FoundState->Successor);
        else {
            UpdateModel(MinContext);        PrefetchData(MaxContext);
            if (EscCount == 0)              ClearMask();
        }
        ns=(MinContext=MaxContext)->NumStats;
        ARI_DEC_NORMALIZE(EncodedFile);
    }
STOP_DECODING:
    StopSubAllocator();
    return(DecodedFile - out);
}

extern "C" int unppmdi(unsigned char *in, int insz, unsigned char *out, int outsz) {
    if(insz < 2) return(-1);
    int         parameters = in[0] | (in[1] << 8);
    int         MaxOrder = (parameters & 0x0f) + 1;
    int         SaSize   = ((parameters >> 4) & 0xFF) + 1;
    MR_METHOD   MRMethod = MR_METHOD(parameters >> 12);
    return(unppmdi_raw(in + 2, insz - 2, out, outsz, SaSize, MaxOrder, MRMethod));
}
