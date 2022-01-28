#include "stdafx.h"
#include "PPMZ2.h"

#include <vector>
#include "PpmDet.h"
#include "ArithInfo.h"
#include "See.h"
#include "Exclude.h"
#include "IntMath.h"
#include "Stopwatch.h"
#include "Probabilities.h"
#include "ContextTrie.h"
#include "Context.h"
#include "ContextData.h"

__declspec(dllexport) Ppmz2::Ppmz* Ppmz2::Ppmz::Create(int order, int trieMegs, int detMegs, CodingOptions options, LocalOrderEstimation::LOEType loeType, void (*loggingCallback)(const std::string&))
{
    // TODO: validate arguments
    Ppmz* ppz = new Ppmz();
	ppz->_loggingCallback = loggingCallback;
	std::vector<unsigned int> alphabets(order);

    alphabets[0] = alphabets[1] = alphabets[2] = alphabets[3] = alphabets[4] = 256;
    alphabets[5] = 256*256*256;
    alphabets[6] = alphabets[7] = UINT_MAX;

	ppz->_contextTrie = new ContextTrie(&alphabets[0], order, trieMegs);	        

    ppz->_arithInfo = new ArithInfo();
    ppz->_order = order;
    ppz->_trieMegs = trieMegs;
    ppz->_detMegs = detMegs;
    ppz->_options = options;

    ppz->_exclude = new Ppmz2::Exclude(256);

    ppz->_see = new Ppmz2::See();

    ppz->_det = Ppmz2::PpmDet::Create(detMegs);
    ppz->_loeType = loeType;

    return ppz;
}

__declspec(dllexport) bool Ppmz2::Ppmz::DecodeArraySub(unsigned char* rawBuf, unsigned int rawLength, unsigned char* compBuf)
{
	CodingMetrics metrics = {0};

    unsigned char* rawPtr = rawBuf;
    unsigned char* rawBufEnd = rawBuf + rawLength;

    memcpy(rawPtr,compBuf,PPMZ2_SeedBytes);
    memset(rawPtr - PPMZ2_MaxContextLen,PPMZ2_SeedByte,PPMZ2_MaxContextLen);
    rawPtr += PPMZ2_SeedBytes;
    compBuf += PPMZ2_SeedBytes;

    _arithInfo->DecodeInitNoStuff(compBuf);

    while(rawPtr < rawBufEnd)
    {
        unsigned long cntx;
        unsigned long tindeces[PPMZ2_Order];
        Context* contexts[PPMZ2_Order+1];
        int sym, LOE_Order, order, codedOrder;
        bool useFull;

        _exclude->Clear();

        // build the forward context

        cntx = getulong(rawPtr-4);

        tindeces[0] = rawPtr[-1];	tindeces[1] = rawPtr[-2];
        tindeces[2] = rawPtr[-3];	tindeces[3] = rawPtr[-4];
        tindeces[4] = rawPtr[-5];
        tindeces[5] = rawPtr[-6] + (rawPtr[-7]<<8) + (rawPtr[-8]<<16);
        tindeces[6] = getulong(rawPtr-12);
        tindeces[7] = getulong(rawPtr-16);

        // get all the contexts from the ContextTrie
        _contextTrie->GetNodes(tindeces, contexts);

        useFull = true;
		
		if (DetDecode(rawPtr, rawBuf, &sym, contexts, &useFull, &(metrics.DetCodingTime)))
		{
			metrics.BytesDetEncoded++;
		}
		else
		{
	        sym = DecodeFromOrder(contexts, cntx, &useFull, &metrics, &codedOrder);
			metrics.UpdatingTime += Update(contexts, sym, cntx, codedOrder);
        }

		metrics.DetUpdatingTime += DetUpdate(rawPtr, rawBuf, sym);

        *rawPtr++ = sym;
    }
    _arithInfo->DecodeDone();

	SafeLog("Decoding", metrics);
    return true;
}
__declspec(dllexport) unsigned int Ppmz2::Ppmz::EncodeArraySub(unsigned char* rawBuf, unsigned int rawLen, unsigned char* compBuf)
{
	CodingMetrics metrics = {0};
	
    unsigned char* rawPtr = rawBuf;
    unsigned char* rawBufEnd = rawBuf + rawLen;

    // seed a preamble
    memcpy(compBuf, rawBuf, PPMZ2_SeedBytes);
    assert( PPMZ2_SeedBytes > 0 );
    memset(rawPtr - PPMZ2_MaxContextLen, PPMZ2_SeedByte, PPMZ2_MaxContextLen);
    rawPtr  += PPMZ2_SeedBytes;

    _arithInfo->EncodeInitNoStuff(compBuf + PPMZ2_SeedBytes);

    while(rawPtr < rawBufEnd)
    {
		std::vector<unsigned long> tindeces(_order);                
        Context* contexts[PPMZ2_Order+1];
        int order, codedOrder;

        // reset exclusions

        _exclude->Clear();

        int sym = *rawPtr;

        // build the forward context

        unsigned long cntx = getulong(rawPtr-4);

        // the context structure is determined by these indeces;
        // we do 1-2-3-4-5-8-12-16 , all with the ContextTrie

        tindeces[0] = rawPtr[-1];	tindeces[1] = rawPtr[-2];
        tindeces[2] = rawPtr[-3];	tindeces[3] = rawPtr[-4];
        tindeces[4] = rawPtr[-5];
        tindeces[5] = rawPtr[-6] + (rawPtr[-7]<<8) + (rawPtr[-8]<<16);
        tindeces[6] = getulong(rawPtr-12);
        tindeces[7] = getulong(rawPtr-16);

        // get all the contexts from the ContextTrie
        //	we must do this before the det, cuz the det points out of
        //	the top context node (we don't need to do this in "follow" mode)

        _contextTrie->GetNodes(&tindeces[0], contexts);

        // notez : including Det in the LOE helps compression on text-like files
        //			(paper1,bib) but hurts on LZ-like files (trans,progs)
        //		since it hurts speed so much, we don't do it

        bool useFull = true;


		if (DetEncode(rawPtr, rawBuf, sym, contexts, &useFull, &(metrics.DetCodingTime)))
		{
			metrics.BytesDetEncoded++;
		}
		else
        {
	        int codedOrder = EncodeFromOrder(contexts, sym, cntx, &useFull, &metrics);
			metrics.UpdatingTime += Update(contexts, sym, cntx, codedOrder);
        }

        metrics.DetUpdatingTime += DetUpdate(rawPtr, rawBuf, sym);

        rawPtr++;

		//for (int i = 0; i < PPMZ2_Order+1; ++i)
		//	delete contexts[i];
    }

    unsigned int compLen = _arithInfo->EncodeDoneMinimal() + PPMZ2_SeedBytes;

    // the arithc has stuffed 1 byte; put it back
    compBuf[PPMZ2_SeedBytes-1] = rawBuf[PPMZ2_SeedBytes-1];            

	SafeLog("Encoding", metrics);
    return compLen;
}


__int64 __declspec(dllexport) Ppmz2::Ppmz::Update(Context** contexts, int sym, unsigned long cntx, int order)
{
	Stopwatch timer;

	if ( ! (_options & /*CodingOptions::*/NoUpdate) )
	{
	    int codedOrder = max(order, 0);

	    for(order = 0; order <= _order; ++order)
	    {					 
            contexts[order]->Update(sym, cntx, _see, codedOrder);				            
	    }
	}
	return timer.Elapsed();
}

__declspec(dllexport) __int64 Ppmz2::Ppmz::EncodeOrderMinusOne(int sym)
{
	Stopwatch timer;
	// encode raw with order -1
	if ( _options & /*CodingOptions::*/TextMode )
		EncodeOrderMinusOneText(sym);
	else
		EncodeOrderMinusOne(sym, 256);
	return timer.Elapsed();
}

__declspec(dllexport) int Ppmz2::Ppmz::DecodeOrderMinusOne(__int64* minusOneOrderTime)
{
	Stopwatch timer;
	// decode raw with order -1
	int sym;
	if ( _options & /*CodingOptions::*/TextMode )
		sym = DecodeOrderMinusOneText();
	else
		sym = DecodeOrderMinusOne(256);
	*minusOneOrderTime += timer.Elapsed();	
	return sym;
}

__declspec(dllexport) int Ppmz2::Ppmz::EncodeFromOrder(Context** contexts, int sym, unsigned long cntx, bool* useFull, CodingMetrics* metrics)
{
	Stopwatch timer;
	// do the initial LOE to pick a start order
    int LOE_Order = LocalOrderEstimation::ChooseOrder(_loeType, contexts, cntx, PPMZ2_Order, _exclude, _see, *useFull);

	// go down the orders

	int order = LOE_Order;
	while (true)
	{
	    // try to coder from order
	    
	    if ( EncodeFromContext(contexts[order], cntx, sym, useFull) )
	    {
	        break;
	    }
    
	    if ( order == 0 )
		{
			metrics->MinusOneOrderTime += EncodeOrderMinusOne(sym);
	        break;
		}
    		
	    // maybe skip down a few :

        order = LocalOrderEstimation::ChooseOrder(_loeType, contexts, cntx, order-1, _exclude, _see, useFull);
	}
	metrics->CodeFromOrderTime += timer.Elapsed();
	return order;
}

__declspec(dllexport) int Ppmz2::Ppmz::DecodeFromOrder(Context** contexts, unsigned long cntx, bool* useFull, CodingMetrics* metrics, int* codedOrder)
{
	Stopwatch timer;
	// do the initial LOE to pick a start order

	int LOE_Order = LocalOrderEstimation::ChooseOrder(_loeType, contexts, cntx, PPMZ2_Order, _exclude, _see, useFull);

	// go down the orders
	int sym;
	for(*codedOrder = LOE_Order;;)
	{
	    // try to coder from order

	    if ( DecodeFromContext(contexts[*codedOrder], cntx, &sym, useFull) )
	    {
	        break;
	    }
    
	    if ( *codedOrder == 0 )
		{
			sym = DecodeOrderMinusOne(&(metrics->MinusOneOrderTime));
	        break;
		}
    		
	    // maybe skip down a few :
        *codedOrder = LocalOrderEstimation::ChooseOrder(_loeType, contexts, cntx, *codedOrder-1, _exclude, _see, useFull);
	}
	metrics->CodeFromOrderTime += timer.Elapsed();
	return sym;
}

__declspec(dllexport) bool Ppmz2::Ppmz::DetEncode(unsigned char* rawPtr, unsigned char* rawBuf, int sym, Context** contexts, bool* useFull, __int64* codingTime)
{
	Stopwatch timer;
	bool retVal = _det->Encode(_arithInfo, rawPtr, rawBuf, sym, _exclude, contexts[PPMZ2_Order], useFull);
	*codingTime += timer.Elapsed();
	return retVal;
}

__declspec(dllexport) bool Ppmz2::Ppmz::DetDecode(unsigned char* rawPtr, unsigned char* rawBuf, int* sym, Context** contexts, bool* useFull, __int64* codingTime)
{
	Stopwatch timer;
	bool retVal = _det->Decode(_arithInfo, rawPtr, rawBuf, sym, _exclude, contexts[PPMZ2_Order], useFull);
	__int64 elapsed = timer.Elapsed();
	*codingTime += elapsed;
	return retVal;
}


__declspec(dllexport) __int64 Ppmz2::Ppmz::DetUpdate(unsigned char* rawPtr, unsigned char* rawBuf, int sym)
{
	Stopwatch timer;
	if ( ! (_options & /*CodingOptions::*/NoUpdate) )
	{
		_det->Update(rawPtr, rawBuf, sym);
	}
	return timer.Elapsed();
}

__declspec(dllexport) unsigned int Ppmz2::Ppmz::DecodeOrderMinusOneText()
{
    unsigned int low,target,sym;

    unsigned int tot = 0;
    for(int i = 0; i < 256; ++i)
    {
        if ( ! _exclude->IsExcluded(i) )
	        tot += textCharCounts[i];
    }
    tot += textCharCounts[256];

    target = _arithInfo->Get(tot);

    sym = low = 0;
    for(;;)
    {
        while( _exclude->IsExcluded(sym) )
	        sym++;
        assert( sym < 257 );
        if ( textCharCounts[sym] )
        {
	        if ( target < textCharCounts[sym] )
		        break;
	        low += textCharCounts[sym];
	        target -= textCharCounts[sym];
        }
        sym++;
        if ( sym == 256 )
	        break;
    }

    assert( sym < 257 );
    assert( textCharCounts[sym] > 0 );
    assert( ! _exclude->IsExcluded(sym) );

    _arithInfo->Decode(low, low + textCharCounts[sym], tot);

    if ( sym == 256 )
    {
        for(int i = 0; i < 256; ++i)
        {
            if ( textCharCounts[i] && ! _exclude->IsExcluded(i) )
                _exclude->Set(i);
        }

        sym = DecodeOrderMinusOne(256);
    }	

    return sym;
}

__declspec(dllexport) void Ppmz2::Ppmz::EncodeOrderMinusOne(unsigned int sym, unsigned int numChars)
{
    assert( ! _exclude->IsExcluded(sym) );

    unsigned int low = 0;
    for(unsigned int i = 0; i < sym; ++i)
    {
        if ( ! _exclude->IsExcluded(i) ) low++;
    }
    unsigned int tot = low+1;
    for(unsigned int i = sym + 1; i < numChars; ++i)
    {
        if ( ! _exclude->IsExcluded(i) ) tot++;
    }

    _arithInfo->Encode(low, low + 1, tot);
}
__declspec(dllexport) void Ppmz2::Ppmz::EncodeOrderMinusOneText(unsigned int sym)
{
    unsigned int low, high, tot, i;

    assert( ! _exclude->IsExcluded(sym) );

    if ( ! textCharCounts[sym] )
    {
        // send an escape
        low = 0;
        for(i=0;i<256;i++)
        {
	        if ( textCharCounts[i] && ! _exclude->IsExcluded(i) )
	        {
		        low += textCharCounts[i];
		        _exclude->Set(i);
	        }
        }
        tot = high = low + textCharCounts[256];

        _arithInfo->Encode(low, high, tot);
        //arithEncode(ari, low,high,tot);

        EncodeOrderMinusOne(sym, 256);
    }
    else
    {
        low = 0;
        for(i=0;i<sym;i++)
        {
	        if ( ! _exclude->IsExcluded(i) )
		        low += textCharCounts[i];
        }
        tot = high = low + textCharCounts[sym];
        for(i = sym+1; i<256; ++i)
        {
	        if ( ! _exclude->IsExcluded(i) )
		        tot += textCharCounts[i];
        }
        tot += textCharCounts[256];

        _arithInfo->Encode(low, high, tot);
    }
}

__declspec(dllexport) unsigned int Ppmz2::Ppmz::DecodeOrderMinusOne(unsigned int numChars)
{
    unsigned int tot = 0;
    for(unsigned int i = 0; i < numChars; ++i)
    {
        if ( ! _exclude->IsExcluded(i) ) 
            tot++;
    }

    unsigned int target = _arithInfo->Get(tot);// arithGet(ari,tot);
    _arithInfo->Decode(target, target + 1, tot);

    unsigned int sym = 0;
    for(;;)
    {
        while( _exclude->IsExcluded(sym) )
	        sym++;
        if ( target == 0 )
	        break;
        target--;
        sym++;
    }

    return sym;
}


__declspec(dllexport) bool Ppmz2::Ppmz::EncodeFromContext(Context* cntx, unsigned long index, int sym, bool* pUseFull)
{
    int low,high,tot;
    int largest,escape;            
    SeeState *ss;

    assert( ! _exclude->IsExcluded(sym) );

    //if ( cntx->full.totSymCount == 0 )
    if ( cntx->_upex._totSymCount == 0 )
    {
        assert( cntx->_upex._totSymCount == 0 );
        return false;
    }

    //if ( *pUseFull && Context_ChooseFull(cntx,exc,see,index) )
    //	cd = &(cntx->full);
    //else
    ContextData* cd = &(cntx->_upex);

    assert( cd->_totSymCount > 0 );

    *pUseFull = false; //@@ do this?
    cd->GetExcludedInfo(_exclude, &tot, &largest, &escape);

    if ( tot == 0 ) // no chars unexcluded
        return false;

    low = high = 0;

    for(ContextNode* n = cd->_syms; n; n = n->next)
    {
        assert( n->count > 0 );
        if ( ! _exclude->IsExcluded(n->sym) )
        {
	        if ( n->sym == sym )
	        {
		        // found it :
		        high = low + n->count;
		        assert( high > 0 );
	        }
	        else if ( high == 0 )
	        {
		        low += n->count;
	        }

	        _exclude->Set(n->sym);
        }
    }

    assert( tot < _arithInfo->_probMax && high <= tot );

    #ifdef DO_USE_LOE_SEESTATE //{
    ss = cd->loeSeeState;
    #else //}{
    if ( escape > tot ) ss = NULL;
    else ss = _see->GetState(escape, tot, index, cntx->_contextOrder, cd->_numSyms, cntx);
    #endif //} DO_USE_LOE_SEESTATE 

    if ( high )
    {
        // found it
        _see->EncodeEscape(_arithInfo, ss, escape, tot, false);
        _arithInfo->Encode(low, high, tot);		        
        return true;
    }
    else
    {
        _see->EncodeEscape(_arithInfo, ss, escape, tot, true);
        return false;
    }
}

__declspec(dllexport) bool Ppmz2::Ppmz::DecodeFromContext(Context* cntx, unsigned long index, int *psym, bool * pUseFull)
{
    int low,got,tot,escape,largest;
    ContextNode * n;
    SeeState *ss;

    //if ( *pUseFull && Context_ChooseFull(cntx,exc,see,index) )
    //	cd = &(cntx->full);
    //else
    ContextData * cd = &(cntx->_upex);

    if ( cd->_totSymCount == 0 )
        return false;

    *pUseFull = false; //@@

    cd->GetExcludedInfo(_exclude, &tot, &largest, &escape);

    if ( tot == 0 ) // no chars unexcluded
        return false;

    #ifdef DO_USE_LOE_SEESTATE //{
    ss = cd->loeSeeState;
    #else //}{
    if ( escape > tot ) ss = NULL;
    else ss = _see->GetState(escape, tot, index, cntx->_contextOrder, cd->_numSyms, cntx);
    #endif //} DO_USE_LOE_SEESTATE 

    if ( _see->DecodeEscape(_arithInfo, ss, escape, tot) )
    {
        n = cd->_syms;
        while(n)
        {
            _exclude->Set(n->sym);
	        n = n->next;
        }
        return false;
    }

    assert( tot < _arithInfo->_probMax );

    got = _arithInfo->Get(tot);
	
    low = 0;

    n = cd->_syms;
    while(n)
    {
    int high;
        assert( got >= low );
        if ( ! _exclude->IsExcluded(n->sym) )
        {
	        high = low + n->count;
	        if ( got < high )
	        {
		        // found it;

                _arithInfo->Decode(low,high,tot);
		        *psym = n->sym;
		        return true;
	        }
	        low = high;
        }
        n = n->next;
    }

    assert(0); // !! should not get here!
	return false;
}

__declspec(dllexport) void Ppmz2::Ppmz::ReInitCoder()
{
    // re-init the arithcoder, but keep the model
    delete _arithInfo;
    _arithInfo = new ArithInfo();
}
