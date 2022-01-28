#include "stdafx.h"
#include "IntMath.h"
#include "See.h"

#include "ContextData.h"

void Ppmz2::ContextData::Update(int sym, int order, unsigned long index, Ppmz2::See* see, Context* context)
{
    ContextNode *node,*prev;
    bool escape;

    // <> could track the 'if I had coded' entropy here
	
    if ( _totSymCount >= Context_CharCountScaleDown )
    {
        Halve();
    }

    _lastSym = sym;

    prev = NULL;
    node = _syms;
    while(node)
    {
        if ( node->sym == sym )
        {
            /* Move 'node' to front of linklist to reduce */
            /* average search time in future.  (This cut  */
            /* pzip runtime by 12.5% when I added it.)    */
            if ( prev ) 
            {
	            prev->next = node->next;
	            node->next   = _syms;
	            _syms	 = node;
            }

            if ( node->count <= Context_SymIncNovel )
            {
	            _escapeCount -= Context_EscpInc;
	            node->count += Context_SymInc - Context_SymIncNovel;
	            _totSymCount += Context_SymInc - Context_SymIncNovel;
	            if ( _escapeCount <= 0 )
		            _escapeCount = 1;
            }
            node->count += Context_SymInc;
            _totSymCount += Context_SymInc;

            escape = false;
	
            goto gotNode;
        }
        prev = node;
        node = node->next;
    }

    escape = true;

    node = new ContextNode();

    node->next = _syms;
    _syms = node;

    node->sym = sym;
    node->count = Context_SymIncNovel;
    _totSymCount += Context_SymIncNovel;	

    if ( _escapeCount < Context_Escape_Max )
    {
        _escapeCount += Context_EscpInc;
    }

    _numSyms++;

    gotNode: //---------------

    _largestCount = max(_largestCount, node->count);

    if ( see )
    {
        // note that this may or may not be the same state that we coded from, because
        //	of exclusions & such
        see->AdjustState(_seeState, escape);
        _seeState = see->GetState(_escapeCount, _totSymCount, index, order, _numSyms, context);
    }
    else
    {
        _seeState = NULL;
    }
}