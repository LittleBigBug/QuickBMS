/*
    Copyright 2012-2016 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

#ifdef XDBG_ALLOC_DISABLED
#define XDBG_ALLOC_INCLUDE
#endif

#ifndef XDBG_ALLOC_INCLUDE
#define XDBG_ALLOC_INCLUDE



#ifdef XDBG_ALLOC_C
    void *(*real_malloc)(size_t size)                   = (void *)malloc;
    void *(*real_calloc)(size_t nelem, size_t elsize)   = (void *)calloc;
    void *(*real_realloc)(void *mem, size_t size)       = (void *)realloc;
    void (*real_free)(void *mem)                        = (void *)free;

    /*static*/ int XDBG_ALLOC_ACTIVE    = 1;    // the alternative is using the classical malloc
    /*static*/ int XDBG_ALLOC_INDEX     = 1;    // use linked lists (must be enabled!)
    /*static*/ int XDBG_ALLOC_VERBOSE   = 0;    // debug information
    /*static*/ int XDBG_HEAPVALIDATE    = 0;    // time consuming!
    /*extern*/ int xdbg_return_NULL_on_error = 0;
#else
    extern void *(*real_malloc)(size_t size);
    extern void *(*real_calloc)(size_t nelem, size_t elsize);
    extern void *(*real_realloc)(void *mem, size_t size);
    extern void (*real_free)(void *mem);

    extern int XDBG_ALLOC_ACTIVE;           // the alternative is using the classical malloc
    extern int XDBG_ALLOC_INDEX;            // use linked lists (must be enabled!)
    extern int XDBG_ALLOC_VERBOSE;          // debug information
    extern int XDBG_HEAPVALIDATE;           // time consuming!
    extern int xdbg_return_NULL_on_error;
#endif



#define xbdg_alloc_dontuse      "Error: do NOT use malloc/calloc/realloc/free!"



#define malloc                  xdbg_malloc     //xbdg_alloc_dontuse
#define calloc                  xdbg_calloc     //xbdg_alloc_dontuse
#define realloc                 xdbg_realloc    //xbdg_alloc_dontuse
#define free                    xdbg_free       //xbdg_alloc_dontuse



void xdbg_free( void *ptr );
void xdbg_freeall(int only_the_not_active);
void xdbg_toggle(void);
void *xdbg_malloc( size_t size );
void *xdbg_realloc( void *ptr, size_t size );
void *xdbg_calloc( size_t num, size_t size );
void xdbg_alloc_extreme(void);



#endif

