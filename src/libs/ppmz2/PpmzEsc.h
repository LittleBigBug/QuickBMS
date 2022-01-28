#pragma once

#include "IntMath.h"
#include "ArithInfo.h"
#include "Context.h"

namespace Ppmz2
{
    class PpmzEsc
    {
    public:
        PpmzEsc()
        {
	        for(int i = 0; i < ORDERS; ++i)
	        {                
		        _esc[i] = new unsigned short[OrderSize(i)];			    
		        _tot[i] = new unsigned short[OrderSize(i)];
	        }

	        /* seed tables, requires some knowledge of the hash */

	        for(int i = 0; i < ORDERS; ++i)
	        {
		        for(unsigned int j = 0; j < OrderSize(i); ++j)
		        {
			        int esc = j & 0x3;
                    int tot = (j>>2) & 0x7;
			        _esc[i][j] = 1 + (ZEsc_INIT_SCALE * esc) + ZEsc_INIT_ESC;
			        _tot[i][j] = 2 + (ZEsc_INIT_SCALE * tot) + ZEsc_INIT_TOT + ZEsc_INIT_ESC;
		        }
	        }
        }

		~PpmzEsc()
		{
			for(int i = 0; i < ORDERS; ++i)
	        {                
		        delete _esc[i];
		        delete _tot[i];
	        }
		}

        void Encode(ArithInfo* ari, unsigned long cntx, int escC, int totSymC, int order, int numParentSyms, bool escape)
        {
            assert( escC >= 1 );

            int escP,totP;
	        GetStats(&escP, &totP, cntx, escC, totSymC, order, numParentSyms);
	        UpdateStats(escape);

            ari->EncodeBit(totP - escP, totP, escape);
        }

        bool Decode(ArithInfo *ari, unsigned long cntx, int escC, int totSymC, int order, int numParentSyms)
        {
	        assert( escC >= 1 );

            int escP,totP;
	        GetStats(&escP, &totP, cntx, escC, totSymC, order, numParentSyms);

	        bool escape = ari->DecodeBit(totP-escP, totP);
            UpdateStats(escape);

            return escape;
        }
        
    private:

        static const int ORDERS = 3;
        static const int ZEsc_INIT_ESC  = 8;
        static const int ZEsc_INIT_TOT  = 12;
        static const int ZEsc_INIT_SCALE = 7;
        static const int ZEsc_ESC_INC   = 17;
        static const int ZEsc_ESCTOT_INC = 1;
        static const int ZEsc_TOT_INC   = 17;        

        unsigned short* _esc[ORDERS];
	    unsigned short* _tot[ORDERS];
        unsigned long _hash[ORDERS];
        bool _hashOk;
        
        static size_t OrderSize(int order)
        {
            static int order_bits[ORDERS] = { 7, 15, 16 };
            return (1 << order_bits[(order)]);
        }

        void GetStats(int* escPptr, int* totPptr, unsigned long cntx, int escC, int totSymC, int PPMorder, int numParentSyms)
        {
	        if ( ! (_hashOk = Hash(_hash, cntx, escC, totSymC, PPMorder, numParentSyms)))
	        {
		        *escPptr = escC;
		        *totPptr = escC + totSymC;
		        return;
	        }

	        int e1,e2,e0,t1,t2,t0,h1,h2,h0;
	        int esc,tot;

		    // blend by entropy

		    e2 = _esc[2][_hash[2]]; t2 = _tot[2][_hash[2]];
		    e1 = _esc[1][_hash[1]]; t1 = _tot[1][_hash[1]];
		    e0 = _esc[0][_hash[0]]; t0 = _tot[0][_hash[0]];

		    h2 = (t2<<12)/(t2 * intlog2r(t2) - e2 * intlog2r(e2) - (t2-e2) * intlog2r(t2-e2) + 1);
		    h1 = (t1<<12)/(t1 * intlog2r(t1) - e1 * intlog2r(e1) - (t1-e1) * intlog2r(t1-e1) + 1);
		    h0 = (t0<<12)/(t0 * intlog2r(t0) - e0 * intlog2r(e0) - (t0-e0) * intlog2r(t0-e0) + 1);

		    tot = (h0 + h1 + h2);
		    esc = (e2*h2/t2 + e1*h1/t1 + e0*h0/t0);

		    while ( tot >= 16000 )
		    {
			    tot >>= 1; esc >>= 1;
		    }

		    if ( esc < 1 ) esc = 1;
		    if ( esc >= tot ) tot = esc + 1;

		    *escPptr = esc;
		    *totPptr = tot;
        }

    public:
        bool Hash(unsigned long* hashPtr, unsigned long cntx, int escC, int totSymC, int PPMorder, int numParentSyms)
        {
            int counts,totP,totC;

	        // <> this is only used by PPMDet, everyone else uses See
	        //  hence we ignore the Order

	        totC = escC + totSymC;
        	
	        assert( escC >= 1 );
	        assert( totC >= 2 );

	        // cap it to 2 bits:
	        if ( numParentSyms > 3 ) numParentSyms = 3;

        /*
	        switch(PPMorder) 
	        { // map it to 2 bits
		        case 0: case 1: PPMorder = 0; break;
		        case 2: case 3: PPMorder = 1; break;
		        case 4: case 5: PPMorder = 2; break;
		        default: PPMorder = 3; break;
	        }
        */

	        switch(escC-1) 
	        {
		        case 0: counts = 0; break;
		        case 1: counts = 1; break;
		        case 2: counts = 2; break;
		        case 3: counts = 3; break;
		        default: return false; // don't handle escapes higher than 3
	        }

	        switch(totC-2)
	        {
		        case 0: totP = 0; break;
		        case 1: totP = 1; break;
		        case 2: totP = 2; break;
		        case 3: case 4: totP = 3; break;
		        case 5: case 6: totP = 4; break;
		        case 7: case 8: case 9: totP = 5; break;
		        case 10: case 11: case 12: totP = 6; break;
		        default: totP = 7; break;
	        }
	        counts += totP << 2; // total of 5 bits

	        /*** this (&0x3) from two next characters is the 'a'/'A'/' '/'\n' flag ***/

        /*	hashPtr[2] = counts + (( (cntx&0x7F) + (((cntx>>13)&0x3)<<7) + (PPMorder<<9) )<<5);

	        hashPtr[1] = counts + (( ((cntx>>5)&0x3) + (((cntx>>13)&0x3)<<2)
				        + (((cntx>>21)&0x3)<<4) + (((cntx>>29)&0x3)<<6) + (PPMorder<<8) )<<5);

	        hashPtr[0] = counts + (PPMorder<<5);
        */

	        hashPtr[2] = counts + (( (cntx&0x7F) + (((cntx>>13)&0x3)<<7) + (numParentSyms<<9) )<<5);

	        hashPtr[1] = counts + (( ((cntx>>5)&0x3) + (((cntx>>13)&0x3)<<2)
				        + (((cntx>>21)&0x3)<<4) + (((cntx>>29)&0x3)<<6) + (numParentSyms<<8) )<<5);

	        hashPtr[0] = counts + (numParentSyms<<5);

            return true;
        }

        void UpdateStats(bool escape)
        {
            if ( ! _hashOk )
		        return;

	        for(int order = ORDERS - 1; order >= 0; --order)
	        {
	            unsigned short *pEsc,*pTot;

		        unsigned long h = _hash[order];
		        pEsc = _esc[order] + h;
		        pTot = _tot[order] + h;

		        if ( escape )
		        {
			        *pEsc += ZEsc_ESC_INC;
			        *pTot += ZEsc_ESC_INC + ZEsc_ESCTOT_INC;
		        }
		        else
		        {
			        *pTot += ZEsc_TOT_INC;
		        }

		        if ( *pTot > 16000 )
		        {
			        *pTot >>= 1;
			        *pEsc >>= 1;
			        if ( *pEsc < 1 )
				        *pEsc = 1;
		        }
	        }
        }
    };
}