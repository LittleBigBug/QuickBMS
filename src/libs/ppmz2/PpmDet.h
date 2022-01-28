#pragma once

#include "CrbList.h"
#include "Context.h"
#include "ArithInfo.h"
#include "PpmzEsc.h"
#include "Exclude.h"

struct PPMDetNode
{
	LinkNode LN; // must be at head
	unsigned short detMinLen;
    unsigned short sym;
	unsigned char* backPtrMinus13;
};

struct PPMDetContext
{
	LinkNode Nodes;
	unsigned int match;
    unsigned int escape;
};

namespace Ppmz2
{
    class PpmDet
    {
    private:
        static const unsigned int NodeArraySize	= 1<<18; // a 256k window
        static const unsigned int PPMDet_MinLen_Inc = 2;
        static const unsigned int PPMDet_MinOrder	 = 24;
        static const unsigned int PPMDet_MaxMatchLen = 1024;
        static const unsigned int PPMDet_MaxNodeWalk = 100;

    public:
        static PpmDet* Create(unsigned int megaBytes)
        {            
            PpmDet* toReturn = new PpmDet();

            toReturn->_zesc = new Ppmz2::PpmzEsc();

	        for(int i = 0; i < NodeArraySize; ++i)
	        {
		        LN_Null( &(toReturn->_nodeArray[i]) );
	        }

            return toReturn;            
        }

        ~PpmDet()
        {
            delete _zesc;
        }

        void Update(unsigned char* backPtr, unsigned char* backBase, int sym)
        {
	        _nextNode = NULL;

	        // remembered from the last encode/decode call:
	        PPMDetNode* n = _gotNode;

	        assert( _gotContext );

	        if ( n )
	        {
		        assert(_gotDContext);

		        if ( n->sym == sym )
		        {
		            unsigned int  i;

			        _gotDContext->match ++;

			        i = ((intptr_t)n) - (intptr_t)(_nodeArray);
			        if ( i == (NodeArraySize-1)*sizeof(PPMDetNode) )
				        _nextNode = _nodeArray;
			        else
				        _nextNode = n + 1;

			        // nextNode may not be filled out yet until we do AddNode below :
		        }
		        else
		        {
			        _gotDContext->escape++;

			        //assert( _gotML >= n->detMinLen );
			        n->detMinLen = _gotML + PPMDet_MinLen_Inc;
		        }
	        }
        	
	        AddNodeToContext(sym, backPtr, _longestML + 1);
        }

        bool Encode(ArithInfo* ari, unsigned char* backPtr, unsigned char* backBase, int sym,
								Exclude* exc, Context* context, bool* pUseFull)
        {
	        assert( *pUseFull == true );

	        DoMatch(backPtr, backBase, context);

	        if ( ! _gotNode )
		        return false;

	        int count = _gotDContext->match;
	        int pred = 	_gotNode->sym;
	        if ( _gotML >= 64 )
		        count = 99999;

	        assert( exc->IsEmpty() );

	        bool match = ( sym == pred ) ? true : false;

	        *pUseFull = false; // @@ do this?

	        // @@ use upex or full for numsyms ?
            _zesc->Encode(ari, getulong(backPtr-4), 1, count, 8, context->_upex._numSyms, !match);
	        
            exc->Set(pred);        

            return match;
        }

        bool Decode(ArithInfo* ari, unsigned char* backPtr, unsigned char* backBase, int* psym,
								Exclude* exc, Context* context, bool* pUseFull)
        {
	        assert( *pUseFull == true );
            DoMatch(backPtr, backBase, context);

	        if ( ! _gotNode )
		        return false;

	        int count = _gotDContext->match;
	        int pred = 	_gotNode->sym;
	        if ( _gotML >= 64 )
		        count = 99999;

	        assert( exc->IsEmpty() );

	        *pUseFull = false; // @@ do this?

            bool match = ! _zesc->Decode(ari, getulong(backPtr-4), 1, count, 8, context->_upex._numSyms);

            exc->Set(pred);

	        *psym = pred;

            return match;
        }

    private:           
            Ppmz2::PpmzEsc* _zesc;

	        PPMDetNode _nodeArray[ NodeArraySize + 1 ];
	        unsigned int _nodeArrayI;

	        Context* _gotContext;
	        PPMDetContext* _gotDContext;
	        PPMDetNode* _gotNode;
            PPMDetNode* _nextNode;
	        unsigned int _gotML;
            unsigned int _longestML;

            PpmDet()
                : _nodeArrayI(0)
            {
            }

            PPMDetNode * AddNodeToContext(int sym, unsigned char* backPtr, unsigned int minLen)
            {
                PPMDetNode * n;
                PPMDetContext *dc;

	                if ( _gotContext->_child )
	                {
		                dc = (PPMDetContext *)(_gotContext->_child);
	                }
	                else
	                {
		                dc = new PPMDetContext();
		                dc->escape = dc->match = 1;
		                LN_Null(&(dc->Nodes));
		                _gotContext->_child = (Context*) (void *)dc;
	                }

	                n = &(_nodeArray[_nodeArrayI++]);
	                if ( _nodeArrayI == NodeArraySize )
		                _nodeArrayI = 0;
	                LN_Cut(n);

	                if ( minLen < PPMDet_MinOrder )
		                minLen = PPMDet_MinOrder;

	                n->sym = sym;
	                n->detMinLen = minLen;
	                n->backPtrMinus13 = backPtr - 13;

	                LN_Null(n);
	                LN_AddHead(&(dc->Nodes),n);

                return n;
            }

            void DoMatch(unsigned char* backPtr, unsigned char* backBase, Context *context)
            {
	            _gotContext = context;
	            if ( _nextNode )
	            {
		            _gotDContext = (PPMDetContext *)(_gotContext->_child);
		            if ( _gotDContext )
		            {
			            _gotNode = _nextNode;
			            _gotML ++;
			            _longestML = max(_longestML,_gotML);
            			
			            if ( _gotML >= 64 )
			            {
				            // force it to accept this match
				            _gotNode->detMinLen = min(_gotNode->detMinLen, _gotML);
			            }
			            else
			            {
				            if ( _gotML < _gotNode->detMinLen )
				            {
					            GetNode((PPMDetContext *)(_gotContext->_child), backPtr, backBase);
				            }
			            }
		            }
		            else
		            {
			            GetNode((PPMDetContext *)(_gotContext->_child), backPtr, backBase);
		            }
	            }
	            else
	            {
		            _gotDContext = NULL;
		            _gotNode = NULL;
		            GetNode((PPMDetContext *)(_gotContext->_child), backPtr, backBase);
	            }
            }

            PPMDetNode* GetNode(PPMDetContext* dc, unsigned char* backPtr, unsigned char* backBase)
            {
                PPMDetNode *bestNode;
                unsigned int len, bestLen, longestLen;
                unsigned char* bm13;
                LinkNode * pList;
                unsigned int walk;

	            if ( ! dc )
	            {
		            // @@ cbloom bug fix 5-14-02
		            _gotDContext = NULL;
		            _gotNode = NULL;
		            _longestML = 0;
		            _gotML = 0;
		            return NULL;
	            }

	            if ( (unsigned int)(backPtr - backBase) < PPMDet_MinOrder ) return NULL;

	            bm13 = backPtr - 13;
	            bestNode = NULL;
	            bestLen = longestLen = 0;

	            pList = &(dc->Nodes);
	            walk = 0;
	            for(PPMDetNode* n = (PPMDetNode*) LN_Next(pList); n != (PPMDetNode*) pList; n = (PPMDetNode*) LN_Next(n))
	            {
		            len = matchBack(bm13,n->backPtrMinus13,backBase) + 12;
		            longestLen = max(longestLen, len);
		            if ( len >= n->detMinLen && len > bestLen )
		            {
			            bestLen = len;
			            bestNode = n;
		            }
		            if ( ++walk == PPMDet_MaxNodeWalk )
			            break;
	            }

	            _gotDContext = dc;
	            _gotNode = bestNode;
	            _longestML = longestLen;
	            _gotML = bestLen;

	            assert( bestLen >= PPMDet_MinOrder || ! bestNode );

                return bestNode;
            }

            int matchBack(unsigned char* p1,unsigned char* p2, unsigned char* backBase)
            {
	            int len = 0;
	            int maxLen = min( (p1 - backBase), (p2 - backBase) );
	            maxLen = min( maxLen, PPMDet_MaxMatchLen);
	            while( *p1-- == *p2-- )
	            {
		            len++;
		            if ( len >= maxLen )
			            break;
	            }
                return len;
            }
    };
}