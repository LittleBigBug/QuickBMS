#pragma once

#include "CrbList.h"
#include "Context.h"

namespace Ppmz2
{
    class ContextTrie
    {
    private:
	    Context* _order0;
	    Context** _order1;
	    unsigned int _numNodes;
	    unsigned int _order;
	    unsigned int* _alphabets;

	    LinkNode _lru;
	    unsigned int _numLRUContexts;
        unsigned int _maxLRUContexts;

    public:
        ContextTrie(unsigned int* alphabets, int order, int trieMegs)
            : _order0(NULL),
               _order1(NULL),
               _numNodes(0),
               _order(0),
               _alphabets(NULL),
               _numLRUContexts(0),
               _maxLRUContexts(0)
        {
	        _order = order;
	        _alphabets = new unsigned int[order];
	        memcpy(_alphabets, alphabets, order * sizeof(unsigned int));

	        _order0 = new Context(NULL, 0);

	        _order1 = (Context**) calloc(1, sizeof(void*) * alphabets[0]);
	        for(unsigned int i = 0; i < alphabets[0]; ++i)
	        {
		        _order1[i] = new Context(_order0, i);
	        }

	        LN_Null(&(_lru));

	        _numLRUContexts = 0;
	        _maxLRUContexts = (trieMegs << 20) / sizeof(Context);
        }

        ~ContextTrie()
        {
			for(unsigned int i = 0; i < _alphabets[0]; ++i)
	        {
				DeleteContext(_order1[i]);
	        }
	        delete[] _alphabets;
			delete _order0;
			free(_order1);
        }

        void GetNodes(unsigned long* pindex, Context** pinto)
        {
            Context * c;

	        *pinto++ = c = _order0;
	        *pinto++ = c = _order1[*pindex++];
	        for(unsigned int i = 2; i <= _order; ++i)
	        {
		        c = FindOrCreate(c, *pindex++);

                assert( ! c->_parent || c->_parent->_contextOrder == (c->_contextOrder - 1) );
                assert( c->_contextOrder == _order || ! c->_child || c->_child->_contextOrder  == (c->_contextOrder + 1) );
		        assert( ((Context*)LN_Next(c))->_contextOrder == c->_contextOrder );

		        *pinto++ = c;
	        }
        }

        Context* FindOrCreate(Context* c, unsigned long index)
        {
            Context * next = c->Find(index);
	        if ( next )
	        {
		        c = next;
		        LRUOldContext(c);
	        }
	        else
	        {
				// TODO: make sure this is deleted appropriately
		        c = new Context(c, index, _order);
		        LRUNewContext(c);
	        }
            return c;
        }

        void LRUOldContext(Context* c)
        {
            
	        LN_Cut( &(c->_lru) );
	        LN_AddHead( &(_lru), &(c->_lru) );
        }

        void LRUNewContext(Context* c)
        {
	        LN_AddHead( &(_lru), &(c->_lru) );

	        _numLRUContexts ++;

	        if ( _numLRUContexts >= _maxLRUContexts )
	        {
	            LinkNode* pLast;
        	
		        // delete one at tail

		        pLast = LN_CutTail(&(_lru));
		        assert(pLast);
		        c = (Context*)( (intptr_t) pLast - (intptr_t)(&(((Context*)0)->_lru)) );

		        assert( ! c->_parent || c->_parent->_contextOrder == (c->_contextOrder - 1) );
		        assert( c->_contextOrder == _order || ! c->_child  || c->_child->_contextOrder  == (c->_contextOrder + 1) );
		        assert( ((Context*)LN_Next(c))->_contextOrder == c->_contextOrder );
		        assert( c->_child != c->_parent );

		        DeleteContext(c);
	        }
        }

        void DeleteContext(Context *c)
        {
	        assert( ! c->_parent || c->_parent->_contextOrder == (c->_contextOrder - 1) );
	        assert( c->_contextOrder == _order || ! c->_child  || c->_child->_contextOrder  == (c->_contextOrder + 1) );
	        assert( ((Context *)LN_Next(c))->_contextOrder == c->_contextOrder );

	        if ( c->_child )
	        {
		        if ( c->_contextOrder == _order )
		        {
			        // c->child is a PPMDet !
			        //PPMDet_DeleteNodes(
			        c->_child = NULL; // just let the det LRU clean it up later
		        }
		        else
		        {
			        do
			        {
				        assert(c->_child->_contextOrder == (c->_contextOrder + 1) );
				        assert(c->_child->_parent == (Context*) c);
				        DeleteContext((Context*) c->_child);
			        } while(c->_child);
		        }
	        }

	        if ( c->_parent )
	        {
		        assert(c->_parent->_contextOrder == (c->_contextOrder - 1) );
		        if ( c->_parent->_child == (Context*) c )
		        {
			        // parent points to me; try to get a sibling
			        c->_parent->_child = (Context*) LN_Next(c);
			        if ( c->_parent->_child == (Context*) c ) // no siblings, so nullify :
				        c->_parent->_child = NULL;
		        }
	        }

	        assert( ! c->_parent || c->_parent->_contextOrder == (c->_contextOrder - 1) );
	        assert( ! c->_child );
	        assert( ((Context *)LN_Next(c))->_contextOrder == c->_contextOrder );

	        delete c;
	        _numLRUContexts --;
        }
    };
}