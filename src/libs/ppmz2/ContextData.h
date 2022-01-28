#pragma once
#define __declspec(X)

#include "IntMath.h"
#include "Exclude.h"

namespace Ppmz2
{
    typedef struct SeeState SeeState;
    typedef class See See;
    typedef class Context Context;

    struct ContextNode
    {
	    ContextNode* next;
	    unsigned short sym;
        unsigned short count;
    };        

    class ContextData
    {
    public:
        int _escapeCount;
        int _numSyms;
        int _totSymCount;
        int _lastSym;
        int _largestCount;
        ContextNode* _syms;
        SeeState* _seeState;

        static const int Context_SymIncNovel = 1;
        static const int Context_SymInc	= 1;    // 2 for PPMD , 1 for PPMC
        static const int Context_EscpInc = 1;
        static const unsigned int Context_Excluded_Escape_Shift	= 2;
        static const unsigned int  Context_Excluded_Escape_Init	= 6;
        static const unsigned int  Context_Excluded_Escape_Inc = 4;	// == (1<<Context_Excluded_Escape_Shift)
        static const unsigned int  Context_Excluded_Escape_ExcludedInc = 3;

        static const int Context_Escape_Max = 20;				// never let escapeCount get bigger than this
        static const int Context_CharCountScaleDown = 4096;	// seems to matter very little; even order0 doesn't get hit this much

        ContextData()
            : _escapeCount(0),
            _numSyms(0),
            _totSymCount(0),
            _lastSym(0),
            _largestCount(0),
            _syms(NULL),
            _seeState(NULL)
        {
        }

        void GetExcludedInfo(Exclude* exc, int* pTotCount, int* pLargestCount, int* pEscapeCount)
        {
            int largestCount,totCount,escapeCount;

            if ( exc->IsEmpty() )
            {
	            totCount = _totSymCount;
	            largestCount = _largestCount;
	            escapeCount = _escapeCount;
            }
            else
            {
                ContextNode * n;

	            // escape from un-excluded counts
	            //	also count the excluded escape syms, but not as hard
	            // rig up the counding so that 1 excluded -> 1 final count

	            // helped paper2 2.193 -> 2.188 bpc !!
	            // you can get 0.001 bpc by tweaking all these constants :

	            // these counts make 1 exc -> 1, 2 -> 2, and 3 -> 2 , etc.
	            //	so for low-escape contexts, we get the same counts, and for low-orders we get
	            //	much lower escape counts

	            largestCount = 0;
	            totCount = 0;
	            escapeCount = Context_Excluded_Escape_Init;

	            for(n = _syms; n; n = n->next)
	            {
		            if ( ! exc->IsExcluded(n->sym) )
		            {
			            totCount += n->count;
			            if ( n->count > largestCount )
				            largestCount = n->count;

			            if ( n->count <= Context_SymIncNovel )
				            escapeCount += Context_Excluded_Escape_Inc;
		            }
		            else
		            {
			            if ( n->count <= Context_SymIncNovel )
				            escapeCount += Context_Excluded_Escape_ExcludedInc;
		            }
	            }

	            escapeCount >>= Context_Excluded_Escape_Shift;
            }

            *pTotCount = totCount;
            *pLargestCount = largestCount;
            *pEscapeCount = escapeCount;
        }

        void Halve()
        {
            ContextNode *node,**nodePtr;

            _totSymCount = 0;
            _numSyms = 0;
            _largestCount = 0;

            nodePtr = &(_syms);
            while( (node = *nodePtr) != NULL )
            {
	            node->count = (node->count)>>1;

	            if ( node->count == 0 )
	            {
		            *nodePtr = node->next;
                    delete node;
	            }
	            else
	            {
		            if ( node->count <= Context_SymIncNovel )
			            node->count = Context_SymIncNovel + 1;

		            _totSymCount += node->count;

		            _numSyms++;

		            _largestCount = max(_largestCount, node->count);

		            nodePtr = &(node->next);
	            }
            }
        	
            _escapeCount = ((_escapeCount)>>1) + 1;
        }

        __declspec(dllexport) void Update(int sym, int order, unsigned long index, Ppmz2::See* see, Context* context);            
    };
}