#pragma once

#include "CrbList.h"
#include "ArithInfo.h"
#include "Context.h"

namespace Ppmz2
{
    struct SeeState
    {
	    LinkNode LN;
	    SeeState * parent;
	    SeeState * child;
	    unsigned int hash,seen;
	    unsigned int esc,tot;
    };

    class See
    {
    public:

        static const unsigned int See_Inc					= 17;
        static const unsigned int See_EscTot_ExtraInc		= 1;
        static const unsigned int See_ScaleDown			    = 8000;
        static const unsigned int See_Esc_ScaleDown		    = 500;
        static const unsigned int PPMZ2_IntProb_Shift       = 16;	// making this small hurts compression !!
		static const unsigned int PPMZ2_IntProb_One	 = (1UL << PPMZ2_IntProb_Shift);
        static const unsigned int See_Init_Scale	        = 7;
        static const unsigned int See_Init_Esc		        = 8;
        static const unsigned int See_Init_Tot		        = 18;
        static const unsigned int MAX_SEE_ESCC              = 3;
        static const unsigned int MAX_SEE_TOTC              = 64;

        static const unsigned int order0_bits = (9);		// <> these cutoffs & the hashes could all be tuned
        static const unsigned int order1_bits = (16);
        static const unsigned int order2_bits = (23);

        static const unsigned int order0_size = (1<<order0_bits);
        static const unsigned int order1_size = (1<<order1_bits);
        static const unsigned int order2_size = (1<<order2_bits);

        See()
        {
            memset(_order0, 0, sizeof(SeeState) * order0_size);
            memset(_order1, 0, sizeof(SeeState) * order1_size);            
	        Precondition();
        }

        void EncodeEscape(ArithInfo* ari, SeeState* ss, unsigned int escapeCount, unsigned int totSymCount, bool escape)
        {
            if ( ss )
	        {
	            unsigned int esc, tot;
		        GetStats(ss, &esc, &tot, escapeCount, escapeCount + totSymCount);
                ari->EncodeBit(esc, tot, !escape);
		        AdjustState(ss, escape);
	        }
	        else
	        {
                ari->EncodeBit(totSymCount, escapeCount + totSymCount, escape);
	        }
        }

        bool DecodeEscape(ArithInfo* ari, SeeState * ss, unsigned int escapeCount, unsigned int totSymCount)
        {
	        if ( ss )
	        {
	            bool escape;
	            unsigned int esc,tot;
                GetStats(ss, &esc, &tot, escapeCount, escapeCount + totSymCount);
		        escape = ari->DecodeBit(esc,tot);
		        escape = ! escape;
		        AdjustState(ss, escape);
		        return escape;
	        }
	        else
	        {
		        return ari->DecodeBit(totSymCount, escapeCount + totSymCount);
	        }
        }

        SeeState* GetState(unsigned int escapeCount, unsigned int totSymCount, unsigned long cntx, int order, int numSyms, const Context* context)
        {
            unsigned int h,o1;
            unsigned int z;

            SeeState *ss,*o1ss;

	        // do the hash;
	        //	order
	        //	escC,
	        //	totC,
	        //	index
	        // use MPS count ?

	        unsigned int escC = escapeCount;
	        unsigned int totC = totSymCount;

	        if ( totC == 0 )
		        return NULL;

	        assert( numSyms >= 1 );
	        assert( escC >= 1 && totC >= escC );
	        totC -= escC;
	        escC --;

	        if ( escC > MAX_SEE_ESCC || totC >= MAX_SEE_TOTC )
		        return NULL;

	        /***
        	
	        <> tune the see hash!
        	
	        we could make a much fancier hash; right now there are lots
	        of redundant bits; 
        	
	        eg. when order = 0 we dont use any bits for index, so we could
			        use more for esc & tot
		        conversely when esc is large, we should use fewer bits for order & cntx

	        this all makes it much messier to do the precondition..

	        ****/

	        // fill up 15 bits of hash
	        h = 0;

	        // 2 bits for the escC <= 3
	        assert( escC <= 3 );
	        h <<= 2;
	        h |= escC;
        	
	        // 3 bits for totC
	        h <<= 3;
		         if ( totC <= 0 ) h |= 0;
	        else if ( totC <= 1 ) h |= 1;
	        else if ( totC <= 2 ) h |= 2;
	        else if ( totC <= 4 ) h |= 3;
	        else if ( totC <= 6 ) h |= 4;
	        else if ( totC <= 9 ) h |= 5;
	        else if ( totC <= 13) h |= 6;
	        else				  h |= 7;
        	
	        // 2 bits for the order :
	        h <<= 2;
	        if ( escC >= 1 )
	        {
		        if ( order >= 3 )
			        h |= 1;
	        }
	        else
	        {
		        z = order >> 1;
		        if ( z > 3 ) z = 3;
		        h |= z;
	        }

	        // 2 bits for num chars in parent
	        // this was Malcolm's idea, and it helps *huge* (meaning about 0.02 bpc)
	        //	paper2 -> 2.196 and trans -> 1.229 !!!
	        // maybe I should use the actual-coded-parent by LOE instead of the direct parent?
	        //	there is a problem there : the LOE decision depends on this!
	        h <<= 2;
	        if ( context && context->_parent )
	        {
		        // @@ use full or upex for numsyms?
                z = context->_parent->_upex._numSyms;
		        if ( z > 3 ) z = 3;
		        h |= z;
	        }

	        // isdet bool ?
	        //  helps a tiny bit (0.001) on files with lots of dets (trans,bib)
	        //	doesn't affect others
	        h <<= 1;
	        if ( numSyms == 1 )
		        h |= 1;

	        // 8 bits from index : 2 bits from each the last 4 bytes :
	        if ( order > 0 ) { h <<= 2; h |= ((cntx>>5)&0x3); }
	        if ( order > 1 ) { h <<= 2; h |= ((cntx>>13)&0x3); }
	        if ( escC <= 1 )
	        {
		        if ( order > 2 ) { h <<= 2; h |= ((cntx>>21)&0x3); }
		        if ( order > 3 ) { h <<= 2; h |= ((cntx>>29)&0x3); }
	        }

	        // the bottom 5 bits of index[0] :
	        h <<= 5;
	        h |= cntx & 31;

	        assert( h < order2_size );

	        o1 = (h >> (order2_bits - order1_bits));
	        assert( o1 < order1_size );
	        o1ss = &(_order1[o1]);
	        ss = o1ss->child;
	        if ( ss )
	        {
		        for(;;)
		        {
			        if ( ss->hash == h )
			        {
				        // MTF
				        // this is a rotate-to-front :
				        //o1ss->child = ss;
				        //*
				        if ( o1ss->child != ss )
				        {
					        LN_Cut(ss);
					        LN_AddTail(o1ss->child,ss);
				        }
				        /**/
				        o1ss->child = ss;
				        return ss;
			        }
			        ss = (SeeState*) LN_Next(ss);
			        if ( ss == o1ss->child )
				        break;
		        }
	        }

	        ss = new SeeState();
	        ss->parent = o1ss;
	        ss->hash = h;
	        LN_Null(ss);
	        if ( o1ss->child )
		        LN_AddHead(o1ss->child,ss);
	        o1ss->child = ss;

	        #if 0 //{
	        // inheret

	        ss->esc = o1ss->esc;
	        ss->tot = o1ss->tot;

	        #else //}{
	        // a fresh init seems better

	        StatsFromHash(ss, h >> (order2_bits - 5));

	        #endif //}

            return ss;
        }

        unsigned int GetEscapeP(SeeState *ss, unsigned int escapeCount, unsigned int totSymCount)
        {
            if ( ss )
	        {
	            unsigned int esc,tot;
                GetStats(ss, &esc, &tot, escapeCount, escapeCount + totSymCount);
		        return (esc << PPMZ2_IntProb_Shift)/ tot;
	        }
	        else
	        {
		        return (escapeCount << PPMZ2_IntProb_Shift) / (escapeCount + totSymCount);
	        }
        }

        void GetStats(SeeState *ss, unsigned int* pEscC, unsigned int* pTotC, unsigned int inEsc, unsigned int inTot)
        {
            unsigned int e1,e2,e0,t1,t2,t0,h1,h2,h0,s2,s1,s0;
            unsigned int tot,esc;

	        e2 = ss->esc; t2 = ss->tot; s2 = ss->seen; ss = ss->parent; assert(ss);
	        e1 = ss->esc; t1 = ss->tot; s1 = ss->seen; ss = ss->parent; assert(ss);
	        e0 = ss->esc; t0 = ss->tot; s0 = ss->seen;

	        /*
	        the entropy-based weighting is a heuristic that favors contexts we are confident in.  You start with 
	        SEE that predicts a 50/50 escape or no-escape, which is a high entropy (1 bit).  The SEE contexts that 
	        become more deterministic (eg. highly predictable) have lower entropies ; we want to use those ones, so
	        we divide by the entropy.
	        */

	        // spooky : ilog2round (accurate) gives 0.003 bpp over intlog2r (inaccurate for values > 256)
	        // note : ilog2round is quite slow, some table-based lookup would be much faster

	        h2 = (t2<<12)/(t2 * ilog2round(t2) - e2 * ilog2round(e2) - (t2-e2) * ilog2round(t2-e2) + 1);
	        h1 = (t1<<12)/(t1 * ilog2round(t1) - e1 * ilog2round(e1) - (t1-e1) * ilog2round(t1-e1) + 1);
	        h0 = (t0<<12)/(t0 * ilog2round(t0) - e0 * ilog2round(e0) - (t0-e0) * ilog2round(t0-e0) + 1);

	        // give less weight to contexts with only the preconditioned stats
	        // this helps a bit; *2,3, or 4 seems the best multiple :
	        if ( s0 ) h0 <<= 2;
	        if ( s1 ) h1 <<= 2;
	        if ( s2 ) h2 <<= 2;

        #if 0 // {
	        tot = (h0 + h1 + h2);
	        esc = (e2*h2/t2 + e1*h1/t1 + e0*h0/t0);
        #else // }{
        {
	        unsigned int ei,ti,hi;

	        // also weight in the esc/tot from the context
	        //	helps about 0.001 bpc on most files

	        ei = inEsc; ti = inTot;
	        hi = (ti<<12)/(ti * ilog2round(ti) - ei * ilog2round(ei) - (ti-ei) * ilog2round(ti-ei) + 1);

	        tot = (h0 + h1 + h2 + hi);
	        esc = (e2*h2/t2 + e1*h1/t1 + e0*h0/t0 + ei*hi/ti);
        }
        #endif //}

	        while ( tot >= 16000 )
	        {
		        tot >>= 1;
		        esc >>= 1;
	        }
	        if ( esc < 1 ) esc = 1;
	        if ( esc >= tot ) tot = esc + 1;

	        *pEscC = esc;
	        *pTotC = tot;
        }

        void AdjustState(SeeState* ss, bool escape)
        {
	        while(ss)
	        {
		        ss->seen ++;

		        if ( escape )
		        {
			        ss->esc += See_Inc;
			        ss->tot += See_Inc + See_EscTot_ExtraInc;
		        }
		        else
		        {
			        // forget escapes very fast
			        if ( ss->esc >= See_Esc_ScaleDown )
			        {
				        ss->esc = (ss->esc >> 1) + 1;
				        ss->tot = (ss->tot >> 1) + 2;
			        }
        			
			        ss->tot += See_Inc;
		        }

		        if ( ss->tot >= See_ScaleDown )
		        {
			        ss->esc = (ss->esc >> 1) + 1;
			        ss->tot = (ss->tot >> 1) + 2;
			        assert( ss->tot < See_ScaleDown );
		        }

		        ss = ss->parent;
	        }
        }

    
    private:
        SeeState _order0[order0_size];
        SeeState _order1[order1_size];

        void StatsFromHash(SeeState* ss, unsigned int FiveBits)
        {
            unsigned int escapeCount,totSymCount;
            unsigned int seedEsc,seedTot,totC;

	        unsigned int escC = FiveBits >> 3;
	        unsigned int totCH = FiveBits & 7;

	        switch(totCH)
	        {
		        case 0:
		        case 1:
		        case 2:
			        totC = totCH;
			        break;
		        case 3:
			        totC = 3;
			        break;
		        case 4:
			        totC = 5;
			        break;
		        case 5:
			        totC = 8;
			        break;
		        case 6:
			        totC = 11;
			        break;
		        case 7:
			        totC = 20;
			        break;
	        }

	        escapeCount = escC + 1;
	        totSymCount = totC + escapeCount;

	        seedEsc = escapeCount * See_Init_Scale + See_Init_Esc;
	        seedTot = (escapeCount + totSymCount) * See_Init_Scale + See_Init_Tot;
        	
	        ss->esc = seedEsc;
	        ss->tot = seedTot;
        }

        void Precondition()
        {
	        for(int escC = 0; escC <= 3 ; escC ++)
	        {
		        unsigned int escapeCount = escC + 1;
		        for(int totCH = 0; totCH <= 7; totCH ++)
		        {
		            unsigned int h, h_hi, h_lo;
		            unsigned int seedEsc, seedTot, totC;
		            unsigned int shift;

			        switch(totCH)
			        {
				        case 0:
				        case 1:
				        case 2:
					        totC = totCH;
					        break;
				        case 3:
					        totC = 3;
					        break;
				        case 4:
					        totC = 5;
					        break;
				        case 5:
					        totC = 8;
					        break;
				        case 6:
					        totC = 11;
					        break;
				        case 7:
					        totC = 20;
					        break;
			        }

			        int totSymCount = totC + escapeCount;

			        // the 5 bit esc/totC
			        h = (escC<<3) + totCH;
        			
			        h_hi = h;

			        seedEsc = escapeCount * See_Init_Scale + See_Init_Esc;
			        seedTot = (escapeCount + totSymCount) * See_Init_Scale + See_Init_Tot;

			        shift = order1_bits - 5;

			        for(h_lo = 0; h_lo < (1UL<<shift); ++h_lo)
			        {
			            SeeState * ss;

				        h = (h_hi<<shift) | h_lo;

				        assert( h < order1_size );

				        ss = &(_order1[h]);

				        ss->esc = seedEsc;
				        ss->tot = seedTot;
				        ss->hash = h;
        				
				        ss->parent = &(_order0[ (h >> (order1_bits - order0_bits)) ]);
				        ss = ss->parent;
				        ss->hash = (h >> (order1_bits - order0_bits));
				        ss->esc = seedEsc;
				        ss->tot = seedTot;
			        }
		        }
	        }
        }
    };
}