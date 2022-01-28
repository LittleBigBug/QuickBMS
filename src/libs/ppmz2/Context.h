#pragma once

#include "CrbList.h"
#include "Exclude.h"
#include "ContextData.h"

namespace Ppmz2
{
    typedef struct SeeState SeeState;
    typedef class See See;

    class Context
    {

    public:
        LinkNode _sisters; //must be at head of struct
	    unsigned int _index; // to get to me
	    int _contextOrder;
	    Context * _parent;
	    Context * _child;

	    ContextData _upex;

	    LinkNode _lru;
        int _ppmzOrder;

        Context(Context* parent, unsigned int index, unsigned int ppmzOrder)
            : _index(0),
            _contextOrder(0),
            _parent(NULL),
            _child(NULL),
            _ppmzOrder(0)
        {         
            _ppmzOrder = ppmzOrder;
            memset(&_upex, 0, sizeof(ContextData));

	        LN_Null( &_sisters );
	        LN_Null(&(_lru));

	        _parent = parent;
	        _index = index;

	        if ( parent )
            {
		        _contextOrder = parent->_contextOrder + 1;

		        if ( parent->_child )
		        {
			        LN_AddHead(parent->_child, &_sisters);
		        }
		        else
		        {
			        parent->_child = (Context*) &_sisters;
		        }
	        }
        }

        Context(Context* parent,unsigned int index)
            : _index(index),
            _contextOrder(0),
            _parent(parent),
            _child(NULL),
            _ppmzOrder(0)
        {
            memset(&_upex, 0, sizeof(ContextData));

	        LN_Null( &_sisters );
	        LN_Null(&(_lru));

	        if ( parent )
            {
		        _contextOrder = parent->_contextOrder + 1;

		        if ( parent->_child )
		        {
			        LN_AddHead(parent->_child, this);
		        }
		        else
		        {
			        parent->_child = this;
		        }
            }
        }

        ~Context()
        {
	        LN_Cut( &_sisters );
	        LN_Cut(&(_lru));
	        
	        ContextNode* syms = _upex._syms;
            ContextNode* next = NULL;
	        while(syms)
	        {
		        next = syms->next;
                delete syms;
		        syms = next;
	        }
        }

        void Update(int sym, unsigned long index, Ppmz2::See* see, int codedOrder)
        {
			assert( ! _parent || _parent->_contextOrder == (_contextOrder - 1) );
			assert( _contextOrder == _ppmzOrder || ! _child  || _child->_contextOrder  == (_contextOrder + 1) );
	        assert( ((Context *)LN_Next(&_sisters))->_contextOrder == _contextOrder );

	        // fulls don't do See Updates
	        //ContextData_Update(&(cntx->full),sym,cntx->order,index,NULL,cntx);

	        if ( _contextOrder >= codedOrder )
                _upex.Update(sym, _contextOrder, index, see, this);

	        // not necessarily true cuz of halvings
	        //assert( cntx->full.totSymCount >= cntx->upex.totSymCount || cntx->full.totSymCount >= ((Context_CharCountScaleDown>>1)-256) );
	        //assert( cntx->full.numSyms >= cntx->upex.numSyms || cntx->full.totSymCount >= ((Context_CharCountScaleDown>>1)-256));
        }

        Context* Find(unsigned long index)
        {
            Context* base = _child;
            if ( base )
            {
                Context* c = base;
	            for(;;)
	            {
		            if ( c->_index == index )
		            {
			            // found it, do MTF
			            // this is really easy, cuz we have a circular list,
			            //	we just change the start point!
			            //parent->child = c;
			            //*
			            if ( _child != c )
			            {
				            LN_Cut(c);
				            LN_AddTail(_child, c);
			            }
			            /**/
			            _child = c;
			            return c;
		            }
		            c = (Context*) LN_Next(c);
		            if ( c == base )
			            break;
	            }
            }

        return NULL;
        }
    };
}