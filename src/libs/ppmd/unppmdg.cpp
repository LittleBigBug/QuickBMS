// modified by Luigi Auriemma (mem2mem decompression only)
/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: model description and encoding/decoding routines              *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define _FASTCALL
#define _STDCALL
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
//static void PrintInfo(void *a, void *b, int c) {}
#include "unppmdg_internal.h"


//#include "PPMd.h"
//#include "SubAlloc.h"
//#pragma hdrstop
//#include "Coder.hpp"

const int INT_BITS=7, PERIOD_BITS=7, MAX_FREQ=124;
const int INTERVAL=1 << INT_BITS, BIN_SCALE=INTERVAL << PERIOD_BITS;

#pragma pack(1)
static struct SEE2g_CONTEXT { // SEE-contexts for PPM-contexts with masked symbols
    WORD Summ;
    BYTE Shift, Count;
    void init(int InitVal) { Summ=InitVal << (Shift=PERIOD_BITS-4); Count=3; }
    UINT getMean() {
        UINT RetVal=Summ >> Shift;          Summ -= RetVal;
        RetVal &= 0x03FF;                   return RetVal+(RetVal == 0);
    }
    void update() {
        if (Shift < PERIOD_BITS && --Count == 0) {
            Summ += Summ;                   Count=3 << Shift++;
        }
    }
} _PACK_ATTR SEE2Cont[44][8];
static struct PPMg_CONTEXT {
    WORD NumStats,SummFreq;                     // sizeof(WORD) > sizeof(BYTE)
    struct STATE { BYTE Symbol, Freq; PPMg_CONTEXT* Successor; } _PACK_ATTR * Stats;
    PPMg_CONTEXT* Suffix;
    PPMg_CONTEXT(STATE* pStats, PPMg_CONTEXT* ShorterContext);
    PPMg_CONTEXT();
    void encodeBinSymbol(int symbol);    // MaxOrder:
    void   encodeSymbol1(int symbol);    //  ABCD    context
    void   encodeSymbol2(int symbol);    //   BCD    suffix
    void           decodeBinSymbol();    //   BCDE   successor
    void             decodeSymbol1();    // other orders:
    void             decodeSymbol2();    //   BCD    context
    void           update1(STATE* p);    //    CD    suffix
    void           update2(STATE* p);    //   BCDE   successor
    void                          rescale();
    SEE2g_CONTEXT* makeEscFreq2(int Diff);
    void* operator new(size_t ) { return AllocContext(); }
    STATE& oneState() const { return (STATE&) SummFreq; }
} _PACK_ATTR * MinContext, * MedContext, * MaxContext;
#pragma pack()

static PPMg_CONTEXT::STATE* FoundState;      // found next state transition
static int NumMasked, InitEsc, OrderFall, MaxOrder;
static BYTE CharMask[256], NS2Indx[256], NS2BSIndx[256];
static BYTE EscCount, /*PrintCount,*/ PrevSuccess;
static WORD BinSumm[128][16];               // binary SEE-contexts

PPMg_CONTEXT::PPMg_CONTEXT(STATE* pStats,PPMg_CONTEXT* ShorterContext):
        NumStats(1), Suffix(ShorterContext) { pStats->Successor=this; }
PPMg_CONTEXT::PPMg_CONTEXT(): NumStats(0) {}
static clock_t StartModel()
{
    int i, k;
    InitSubAllocator();
    MaxContext = new PPMg_CONTEXT();         MaxContext->Suffix=NULL;
    MaxContext->SummFreq=(MaxContext->NumStats=256)+1;
    MaxContext->Stats = (PPMg_CONTEXT::STATE*) AllocUnitsRare(256/2);
    for (PrevSuccess=i=0;i < 256;i++) {
        MaxContext->Stats[i].Symbol=i;      MaxContext->Stats[i].Freq=1;
        MaxContext->Stats[i].Successor=NULL;
    }
    PPMg_CONTEXT::STATE* p=MaxContext->Stats;
    for (OrderFall=i=1; ;i++) {
        MaxContext = new PPMg_CONTEXT(p,MaxContext);
        if (i == MaxOrder)                  break;
        p=&(MaxContext->oneState());
        p->Symbol = 0;                      p->Freq = 1;
    }
    MaxContext->NumStats=0;                 MedContext=MinContext=MaxContext->Suffix;
static const WORD InitBinEsc[16] = {
                0x3CDD,0x1F3F,0x59BF,0x48F3,0x5FFB,0x5545,0x63D1,0x5D9D,
                0x64A1,0x5ABC,0x6632,0x6051,0x68F6,0x549B,0x6BCA,0x3AB0, };
    for (i=0;i < 128;i++)
        for (k=0;k < 16;k++)
            BinSumm[i][k]=BIN_SCALE-InitBinEsc[k]/(i+2);
    for (i=0;i <   6;i++)                   NS2BSIndx[i]=2*i;
    for (   ;i <  50;i++)                   NS2BSIndx[i]=12;
    for (   ;i < 256;i++)                   NS2BSIndx[i]=14;
    for (i=0;i < 43;i++)
            for (k=0;k < 8;k++)             SEE2Cont[i][k].init(4*i+10);
    SEE2Cont[i][0].Shift=PERIOD_BITS;
    for (i=0;i < 4;i++)                     NS2Indx[i]=i;
    for ( ;i < 4+8;i++)                     NS2Indx[i]=4+((i-4) >> 1);
    for ( ;i < 4+8+32;i++)                  NS2Indx[i]=4+4+((i-4-8) >> 2);
    for ( ;i < 256;i++)                     NS2Indx[i]=4+4+8+((i-4-8-32) >> 3);
    memset(CharMask,0,sizeof(CharMask));    EscCount=/*PrintCount=*/1;
    return clock();
}
static void StopModel() {}
void PPMg_CONTEXT::rescale()
{
    int OldNS=NumStats, i=NumStats-1, Adder, EscFreq;
    STATE* p1, * p;
    for (p=FoundState;p != Stats;p--)       SWAP(p[0],p[-1]);
    Stats->Freq += 4;                       SummFreq += 4;
    EscFreq=SummFreq-p->Freq;               Adder=(OrderFall != 0);
    SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            STATE tmp=*(p1=p);
            do { p1[0]=p1[-1]; } while (--p1 != Stats && tmp.Freq > p1[-1].Freq);
            *p1=tmp;
        }
    } while ( --i );
    if (p->Freq == 0) {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;
        if ((NumStats -= i) == 1) {
            STATE tmp=*Stats;
            do { tmp.Freq-=(tmp.Freq >> 1); EscFreq>>=1; } while (EscFreq > 1);
            FreeUnits(Stats,(OldNS+1) >> 1);
            *(FoundState=&oneState())=tmp;  return;
        }
    }
    SummFreq += (EscFreq -= (EscFreq >> 1));
    int n0=(OldNS+1) >> 1, n1=(NumStats+1) >> 1;
    if (n0 != n1)
            Stats=(STATE*)ShrinkUnits(Stats,n0,n1);
    FoundState=Stats;
}
static BOOL MakeRoot(UINT SkipCount,PPMg_CONTEXT::STATE* p1)
{
    PPMg_CONTEXT* pc=MinContext, * UpBranch=FoundState->Successor;
    PPMg_CONTEXT::STATE* p, * ps[MAX_O], ** pps=ps;
    if (SkipCount == 0) {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  goto NO_LOOP;
    } else if (SkipCount == 2)              pc=pc->Suffix;
    if ( p1 ) {
        p=p1;                               pc=pc->Suffix;
        goto LOOP_ENTRY;
    }
    do {
        pc=pc->Suffix;
        if (pc->NumStats != 1) {
            if ((p=pc->Stats)->Symbol != FoundState->Symbol)
                do { p++; } while (p->Symbol != FoundState->Symbol);
        } else                              p=&(pc->oneState());
LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc=p->Successor;                break;
        }
        *pps++ = p;
    } while ( pc->Suffix );
NO_LOOP:
    PPMg_CONTEXT::STATE& UpState=UpBranch->oneState();
    if (pc->NumStats != 1) {
        UINT cf=UpState.Symbol;
        if ((p=pc->Stats)->Symbol != cf)
                do { p++; } while (p->Symbol != cf);
        UINT s0=pc->SummFreq-pc->NumStats-(cf=p->Freq-1);
        UpState.Freq=1+((2*cf <= s0)?(5*cf > s0):((2*cf+3*s0-1)/(2*s0)));
    } else                                  UpState.Freq=pc->oneState().Freq;
    while (--pps >= ps) {
        pc = new PPMg_CONTEXT(*pps,pc);
        if ( !pc )                          return FALSE;
        pc->oneState()=UpState;
    }
    if ( !OrderFall ) {
        UpBranch->NumStats=1;               UpBranch->Suffix=pc;
    }
    return TRUE;
}
static void UpdateModel()
{
    PPMg_CONTEXT::STATE fs=*FoundState, * p, *p1=NULL;
    PPMg_CONTEXT* pc, * Successor;
    UINT ns1, ns, cf, sf, s0, SkipCount=0;
    if (fs.Freq < MAX_FREQ/4 && (pc=MinContext->Suffix) != NULL) {
        if (pc->NumStats != 1) {
            if ((p1=pc->Stats)->Symbol != fs.Symbol) {
                do { p1++; } while (p1->Symbol != fs.Symbol);
                if (p1[0].Freq >= p1[-1].Freq) {
                    SWAP(p1[0],p1[-1]);     p1--;
                }
            }
            if (p1->Freq < 7*MAX_FREQ/8) {
                p1->Freq += 2;              pc->SummFreq += 2;
            }
        } else {
            p1=&(pc->oneState());           p1->Freq += (p1->Freq < 32);
        }
    }
    if (OrderFall == 0) {
        if ( !MakeRoot(2,NULL) )            goto RESTART_MODEL;
        MinContext=MedContext=fs.Successor; return;
    } else if (--OrderFall == 0) {
        Successor=fs.Successor;             SkipCount=1;
    } else if ((Successor = new PPMg_CONTEXT()) == NULL)
            goto RESTART_MODEL;
    if ( !MaxContext->NumStats ) {
        MaxContext->oneState().Symbol = fs.Symbol;
        MaxContext->oneState().Successor = Successor;
    }
    s0=MinContext->SummFreq-(ns=MinContext->NumStats)-(fs.Freq-1);
    for (pc=MedContext;pc != MinContext;pc=pc->Suffix) {
        if ((ns1=pc->NumStats) != 1) {
            if ((ns1 & 1) == 0) {
                pc->Stats=(PPMg_CONTEXT::STATE*) ExpandUnits(pc->Stats,ns1 >> 1);
                if ( !pc->Stats )           goto RESTART_MODEL;
            }
            pc->SummFreq += (2*ns1 < ns)+2*((4*ns1 <= ns) &
                    (pc->SummFreq <= 8*ns1));
        } else {
            p=(PPMg_CONTEXT::STATE*) AllocUnitsRare(1);
            if ( !p )                       goto RESTART_MODEL;
            *p=pc->oneState();              pc->Stats=p;
            if (p->Freq < MAX_FREQ/4-1)     p->Freq += p->Freq;
            else                            p->Freq  = MAX_FREQ-4;
            pc->SummFreq=p->Freq+InitEsc+(ns > 3);
        }
        cf=2*fs.Freq*(pc->SummFreq+6);      sf=s0+pc->SummFreq;
        if (cf < 6*sf) {
            cf=1+(cf > sf)+(cf >= 4*sf);
            pc->SummFreq += 3;
        } else {
            cf=4+(cf >= 9*sf)+(cf >= 12*sf)+(cf >= 15*sf);
            pc->SummFreq += cf;
        }
        p=pc->Stats+ns1;                    p->Successor=Successor;
        p->Symbol = fs.Symbol;              p->Freq = cf;
        pc->NumStats=++ns1;
    }
    if ( fs.Successor ) {
        if (!fs.Successor->NumStats && !MakeRoot(SkipCount,p1))
                goto RESTART_MODEL;
        MinContext=FoundState->Successor;
    } else {
        FoundState->Successor=Successor;    OrderFall++;
    }
    MedContext=MinContext;                  MaxContext=Successor;
    return;
RESTART_MODEL:
    StartModel();
    EscCount=0;                             //PrintCount=0xFF;
}
// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))
void PPMg_CONTEXT::encodeBinSymbol(int symbol)
{
    STATE& rs=oneState();
    WORD& bs=BinSumm[rs.Freq-1][PrevSuccess+NS2BSIndx[Suffix->NumStats-1]];
    if (rs.Symbol == symbol) {
        FoundState=&rs;                     rs.Freq += (rs.Freq < 128);
        SubRange.LowCount=0;                SubRange.HighCount=bs;
        bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        PrevSuccess=1;
    } else {
        SubRange.LowCount=bs;               bs -= GET_MEAN(bs,PERIOD_BITS,2);
        SubRange.HighCount=BIN_SCALE;       InitEsc=ExpEscape[bs >> 10];
        NumMasked=1;                        CharMask[rs.Symbol]=EscCount;
        PrevSuccess=0;                      FoundState=NULL;
    }
}
void PPMg_CONTEXT::decodeBinSymbol()
{
    STATE& rs=oneState();
    WORD& bs=BinSumm[rs.Freq-1][PrevSuccess+NS2BSIndx[Suffix->NumStats-1]];
    if (ariGetCurrentShiftCount(INT_BITS+PERIOD_BITS) < bs) {
        FoundState=&rs;                     rs.Freq += (rs.Freq < 128);
        SubRange.LowCount=0;                SubRange.HighCount=bs;
        bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        PrevSuccess=1;
    } else {
        SubRange.LowCount=bs;               bs -= GET_MEAN(bs,PERIOD_BITS,2);
        SubRange.HighCount=BIN_SCALE;       InitEsc=ExpEscape[bs >> 10];
        NumMasked=1;                        CharMask[rs.Symbol]=EscCount;
        PrevSuccess=0;                      FoundState=NULL;
    }
}
void PPMg_CONTEXT::update1(STATE* p)
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) {
        SWAP(p[0],p[-1]);                   FoundState=--p;
        if (p->Freq > MAX_FREQ)             rescale();
    }
}
void PPMg_CONTEXT::encodeSymbol1(int symbol)
{
    SubRange.scale=SummFreq;
    STATE* p=Stats;
    if (p->Symbol == symbol) {
        PrevSuccess=(2*(SubRange.HighCount=p->Freq) > SubRange.scale);
        (FoundState=p)->Freq += 4;          SummFreq += 4;
        if (p->Freq > MAX_FREQ)             rescale();
        SubRange.LowCount=0;                return;
    }
    PrevSuccess=0;
    int LoCnt=p->Freq, i=NumStats-1;
    while ((++p)->Symbol != symbol) {
        LoCnt += p->Freq;
        if (--i == 0) {
            SubRange.LowCount=LoCnt;        CharMask[p->Symbol]=EscCount;
            i=(NumMasked=NumStats)-1;       FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            SubRange.HighCount=SubRange.scale;
            return;
        }
    }
    SubRange.HighCount=(SubRange.LowCount=LoCnt)+p->Freq;
    update1(p);
}
void PPMg_CONTEXT::decodeSymbol1()
{
    SubRange.scale=SummFreq;
    STATE* p=Stats;
    int i, count, HiCnt;
    if ((count=ariGetCurrentCount()) < (HiCnt=p->Freq)) {
        PrevSuccess=(2*(SubRange.HighCount=HiCnt) > SubRange.scale);
        (FoundState=p)->Freq=(HiCnt += 4);  SummFreq += 4;
        if (HiCnt > MAX_FREQ)               rescale();
        SubRange.LowCount=0;                return;
    }
    PrevSuccess=0;                          i=NumStats-1;
    while ((HiCnt += (++p)->Freq) <= count)
            if (--i == 0) {
                SubRange.LowCount=HiCnt;    CharMask[p->Symbol]=EscCount;
                i=(NumMasked=NumStats)-1;   FoundState=NULL;
                do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
                SubRange.HighCount=SubRange.scale;
                return;
            }
    SubRange.LowCount=(SubRange.HighCount=HiCnt)-p->Freq;
    update1(p);
}
void PPMg_CONTEXT::update2(STATE* p)
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p->Freq > MAX_FREQ)                 rescale();
    EscCount++;
}
SEE2g_CONTEXT* PPMg_CONTEXT::makeEscFreq2(int Diff)
{
    SEE2g_CONTEXT* psee2c;
    if (NumStats != 256) {
        psee2c=SEE2Cont[NS2Indx[Diff-1]]+(Diff < Suffix->NumStats-NumStats)+
                2*(SummFreq < 11*NumStats)+4*(NumMasked > Diff);
        SubRange.scale=psee2c->getMean();
    } else {
        psee2c=SEE2Cont[43];                SubRange.scale=1;
    }
    return psee2c;
}
void PPMg_CONTEXT::encodeSymbol2(int symbol)
{
    int HiCnt, i=NumStats-NumMasked;
    SEE2g_CONTEXT* psee2c=makeEscFreq2(i);
    STATE* p=Stats-1;                       HiCnt=0;
    do {
        do { p++; } while (CharMask[p->Symbol] == EscCount);
        HiCnt += p->Freq;
        if (p->Symbol == symbol)            goto SYMBOL_FOUND;
        CharMask[p->Symbol]=EscCount;
    } while ( --i );
    SubRange.HighCount=(SubRange.scale += (SubRange.LowCount=HiCnt));
    psee2c->Summ += SubRange.scale;         NumMasked = NumStats;
    return;
SYMBOL_FOUND:
    SubRange.LowCount = (SubRange.HighCount=HiCnt)-p->Freq;
    if ( --i ) {
        STATE* p1=p;
        do {
            do { p1++; } while (CharMask[p1->Symbol] == EscCount);
            HiCnt += p1->Freq;
        } while ( --i );
    }
    SubRange.scale += HiCnt;
    psee2c->update();                       update2(p);
}
void PPMg_CONTEXT::decodeSymbol2()
{
    int count, HiCnt, i=NumStats-NumMasked;
    SEE2g_CONTEXT* psee2c=makeEscFreq2(i);
    STATE* ps[256], ** pps=ps, * p=Stats-1;
    HiCnt=0;
    do {
        do { p++; } while (CharMask[p->Symbol] == EscCount);
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
        i=NumStats-NumMasked;               pps--;
        do { CharMask[(*++pps)->Symbol]=EscCount; } while ( --i );
        psee2c->Summ += SubRange.scale;     NumMasked = NumStats;
    }
}
static void ClearMask(unsigned char* EncodedFile,unsigned char* DecodedFile,clock_t StartClock)
{
    EscCount=1;                             memset(CharMask,0,sizeof(CharMask));
    //if (++PrintCount == 0)
        //PrintInfo(DecodedFile,EncodedFile,((clock()-StartClock) << 10)/int(CLK_TCK));
}


extern "C" int unppmdg_raw(unsigned char *in, int insz, unsigned char *out, int outsz, int SaSize, int MaxOrder) {
    EncodedFile  = in;
    EncodedFilel = in + insz;
    DecodedFile  = out;
    DecodedFilel = out + outsz;

    if(SaSize   <= 0) SaSize   = 10;
    if(MaxOrder <= 0) MaxOrder = 4;

    StartSubAllocator(SaSize);

    ARI_INIT_DECODER(EncodedFile);          ::MaxOrder=MaxOrder;
    clock_t StartClock=StartModel();
    for ( ; ; ) {
        if (MinContext->NumStats != 1)      MinContext->decodeSymbol1();
        else                                MinContext->decodeBinSymbol();
        ariRemoveSubrange();
        while ( !FoundState ) {
            ARI_DEC_NORMALIZE(EncodedFile);
            do {
                OrderFall++;                MinContext=MinContext->Suffix;
                if ( !MinContext )          goto STOP_DECODING;
            } while (MinContext->NumStats == NumMasked);
            MinContext->decodeSymbol2();    ariRemoveSubrange();
        }
        xputc(FoundState->Symbol,DecodedFile);
        if (!OrderFall && FoundState->Successor->NumStats)
                MinContext=MedContext=FoundState->Successor;
        else {
            UpdateModel();
            if (EscCount == 0)
                    ClearMask(EncodedFile,DecodedFile,StartClock);
        }
        ARI_DEC_NORMALIZE(EncodedFile);
    }
STOP_DECODING:
    StopModel();
    StopSubAllocator();
    //PrintInfo(DecodedFile,EncodedFile,((clock()-StartClock) << 10)/int(CLK_TCK));
    return(DecodedFile - out);
}



extern "C" int unppmdg(unsigned char *in, int insz, unsigned char *out, int outsz) {
    if(insz < 2) return(-1);
    int         parameters = in[0] | (in[1] << 8);
    int         MaxOrder = (parameters & 0x0f) + 1;
    int         SaSize   = ((parameters >> 4) & 0xFF) + 1;
    return(unppmdg_raw(in + 2, insz - 2, out, outsz, SaSize, MaxOrder));
}

