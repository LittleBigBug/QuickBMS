#include "stdafx.h"
#include "LocalOrderEstimation.h"
#include "Exclude.h"
#include "Context.h"
#include "ContextData.h"
#include "See.h"

__declspec(dllexport) int Ppmz2::LocalOrderEstimation::ChooseOrder(Ppmz2::LocalOrderEstimation::LOEType loeType, Ppmz2::Context ** contexts, unsigned long cntx, int maxOrder, Ppmz2::Exclude* exc, Ppmz2::See* see, bool useFull)
{
    int rating;

    if ( loeType == LOETYPE_NONE )
        return maxOrder;

    int choseOrder  = 0;
    int choseRating = 0;

    for(int order = maxOrder; order >= 0; --order)
    {
        Context * c;
        ContextData* cd;
        int largestCount,totCount,escapeCount;
        SeeState * ss;

        if ( order == 0 && choseRating == 0 )
	        return 0; //early out; this is our only choice

        c = contexts[order];

        //if ( ! c || c->full.totSymCount == 0 )
        if ( ! c || c->_upex._totSymCount == 0 )
        {
	        assert( c->_upex._totSymCount == 0 );
	        continue;
        }

        //if ( useFull && Context_ChooseFull(c,exc,see,cntx) )
        //	cd = &(c->full);
        //else
	    cd = &(c->_upex);
        cd->GetExcludedInfo(exc, &totCount, &largestCount, &escapeCount);
    			        
        if ( totCount == 0 )
	        continue;

        assert( largestCount >= 0 );


        #ifdef DO_LOE_PENALIZE_NONDET
        // ask if its det before exclusion or after?
        //  doesn't seem to make much difference
        if ( cd->_numSyms > 1 ) // this is before
        //if ( largestCount != totCount ) // this is after
        {
	        totCount += escapeCount;
        }
        // note : this makes us a use a different seeState for LOE than we do for coding!
        #endif

        if ( totCount < escapeCount )
	        ss = NULL;
        else
	        ss = see->GetState(escapeCount, totCount, cntx, order, cd->_numSyms, c);

        //cd->loeSeeState = ss; //(disabled, see codecntx.c)

        if ( loeType == LOETYPE_MPS )
        {
			rating = ((See::PPMZ2_IntProb_One - see->GetEscapeP(ss, escapeCount, totCount))
			            * largestCount ) / totCount;
        }
        else 
        {
        int MPSP,MPSB;

	        // a little pseudo-entropy
	        // better on some files

	        // <> todo : track performance of various LOE schemes & make a weighted rating :
	        //	full_rating = Sum[i] weight(i) * rating(i)

			MPSP = ((See::PPMZ2_IntProb_One - see->GetEscapeP(ss, escapeCount, totCount))
			            * largestCount ) / totCount;

			assert( MPSP > 0 && MPSP < See::PPMZ2_IntProb_One );

			MPSB = ((See::PPMZ2_IntProb_Shift<<4) - ilog2x16(MPSP)); // = 16 * log(1/MPSP)
			rating = MPSP * MPSB + ( See::PPMZ2_IntProb_One - MPSP ) * 128;
	        rating = INT_MAX - rating;
        }

        if ( rating > choseRating )
        {
	        choseRating = rating;
	        choseOrder = order;
        }
    }

    return choseOrder;
}