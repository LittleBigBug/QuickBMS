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



        /*****************************************\
        |* THESE FUNCTIONS ARE *NOT* THREAD-SAFE *|
        \*****************************************/

    /*
    everything remains allocated till the closing of the main program
    the only exceptions are the reallocations that require more memory than
    that available in the previous buffer, in which case it must be
    freed and allocated newly.

    11112222333333333333333334444
    |   |   |                |
    |   |   |                PAGE_GUARD
    |   |   |the requested buffer
    |   |   
    |   PAGE_GUARD
    structure with information

    notes: stdout is necessary for debugging
    */

    /*
    problems:
    - if I don't use the index (linked lists) there is the increasing of the
      problem about memory unallocable due to VirtualAlloc that doesn't
      manipolate the memory like malloc
    - a secure usage of the memory caged between two PAGE_GUARDS takes 16kb
      for each allocation (even if you try to allocate 1 byte) wasting tons
      of memory
    - the usage of linked lists make the program slower when there are
      many allocations
    */

#define XDBG_ALLOC_C



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/mman.h>
    #define MEM_COMMIT      (MAP_PRIVATE | MAP_ANONYMOUS)
    #define MEM_RESERVE     0
    #define MEM_RELEASE     0
    #define PAGE_NOACCESS   PROT_NONE
    #define PAGE_READONLY   PROT_READ
    #define PAGE_GUARD      PROT_NONE
    #define PAGE_READWRITE  (PROT_READ | PROT_WRITE)

    typedef uint32_t    DWORD;

    typedef struct {
        DWORD   RegionSize;
    } MEMORY_BASIC_INFORMATION;
#endif



#include "xalloc.h"
#include "uthash_real_alloc.h"
#include "utlist.h"



        // what if the pointer has been allocated by another library that doens't use this system?
        // char *p = mydll_func();
        // free(p); // would return an error!
#define XDBG_ALLOC_ALLOW_UNKNOWN_FREE

//#define XDBG_USE_MALLOC     // an experiment, it can't be used with PAGE_GUARD because
                              // memory gets moved and so the PAGE_GUARDs affect other
                              // buffers



                                           // u8 required        // info            // page_guard
#define XDBG_ALLOC_MEM2PTR(X)   ((void *)(((unsigned char *)X) + (g_xdbg_pagesize + g_xdbg_pagesize)))
#define XDBG_ALLOC_PTR2MEM(X)   ((void *)(((unsigned char *)X) - (g_xdbg_pagesize + g_xdbg_pagesize)))
#define XDBG_ALLOC_MEM2INFO(X)  ((xdbg_alloc_t *)(X))
#define XDBG_ALLOC_PTR2INFO(X)  ((xdbg_alloc_t *)(XDBG_ALLOC_PTR2MEM(X)))
#define XDBG_ALLOC_FREECHR      0xCC            // INT3
#define XDBG_ALLOC_ERROR        xdbg_alloc_error(__FILE__, __LINE__, __FUNCTION__)
#define XDBG_ALLOC_ERROR_RETURN { \
                                    if(xdbg_return_NULL_on_error) { \
                                        return NULL; \
                                    } else { \
                                        XDBG_ALLOC_ERROR; \
                                    } \
                                }



#pragma pack(16)
typedef struct xdbg_alloc_t {
    void    *addr;      // same address of the struct's address
    size_t  original;   // the real size requested by the user
    size_t  size;       // the amount of data really allocated without PAGE_GUARDs
                        // (so only the structure + the memory)
    size_t  xsize;      // everything included the PAGE_GUARDs
    size_t  active;     // in use
    UT_hash_handle hh;  // hash for faster searches
    struct xdbg_alloc_t *next;  // the next allocated memory
    struct xdbg_alloc_t *prev;  // the previous allocated memory
} xdbg_alloc_t;
#pragma pack()



static size_t       g_xdbg_pagesize     = 0;
#ifndef PAGE_GUARD
#define PAGE_GUARD		0x0100
#endif
static int  g_xdbg_page_guard =
#ifdef WIN32
    PAGE_READONLY | PAGE_GUARD  // avoids problems with Xonar and other bugged drivers
#else
    PAGE_NOACCESS   // PROT_NONE
#endif
;
static xdbg_alloc_t *g_xdbg_alloc       = NULL;
static int xdbg_return_NULL_on_error    = 0;    // keep it disabled because we don't trust the main program



#ifndef WIN32
    void *VirtualAlloc(void *lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
        lpAddress = mmap(lpAddress, dwSize, flProtect, flAllocationType, 0, 0);
        if((long)lpAddress == -1) return NULL;
        return(lpAddress);
    }

    size_t VirtualProtect(void *lpAddress, size_t dwSize, DWORD flNewProtect, DWORD *lpflOldProtect) {
        if(mprotect(lpAddress, dwSize, flNewProtect) == -1) return 0;
        return 1;
    }

    size_t VirtualFree(void *lpAddress, size_t dwSize, DWORD dwFreeType) {
        if(!lpAddress) return 1;
        if(!dwSize) dwSize = XDBG_ALLOC_MEM2INFO(lpAddress)->xsize;
        if(munmap(lpAddress, dwSize) == -1) return 0;
        return 1;
    }

    size_t HeapValidate(size_t hHeap, DWORD dwFlags, void *lpMem) {
        return 1;
    }

    size_t GetProcessHeap(void) {
        return 1;
    }

    #define CopyMemory  memcpy
#endif



void xdbg_alloc_error(const char *fname, DWORD line, const char *func) {
    char    tmp[16],
            *message = NULL;

    #ifdef WIN32
    if(!GetLastError()) {
        message = "";
    } else {
        FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          GetLastError(),
          0,
          (char *)&message,
          0,
          NULL);
    }
    #endif
    if(!message) message = strerror(errno);

    fprintf(stderr, "\n- error in %s line %d: %s()\n", fname, (int32_t)line, func);
    fprintf(stderr, "\n"
        "Error: memory allocation problem\n"
        "       %s\n"
        "\n"
        "press ENTER to quit\n",
        message ? message : "unknown error");
    fgets(tmp, sizeof(tmp), stdin);
    exit(1);
}



size_t xdbg_alloc_align(size_t size) {
    if(!g_xdbg_pagesize) {
        #ifdef WIN32
        SYSTEM_INFO sSysInfo;
        GetSystemInfo(&sSysInfo);
        g_xdbg_pagesize = sSysInfo.dwPageSize;
        #else
        g_xdbg_pagesize = getpagesize();
        #endif
        if(!g_xdbg_pagesize) g_xdbg_pagesize = 4096;
        if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# g_xdbg_pagesize %u (called by %p)\n", (uint32_t)g_xdbg_pagesize, __builtin_extract_return_addr(__builtin_return_address(0)));
    }
    size = (size + (g_xdbg_pagesize - 1)) & (~(g_xdbg_pagesize - 1));
    return(size);
}



size_t xdbg_alloc_init(size_t *ret_original, size_t *ret_size, size_t *ret_xsize) {
    static int  init = 0;
    size_t  original,
            size,
            xsize;

    if(!init) {
        init = 1;
        #ifdef WIN32
        OSVERSIONINFO   osver = {0};
        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osver);
        if(osver.dwMajorVersion <= 4) g_xdbg_page_guard = PAGE_NOACCESS;  // Win98
        #endif
    }

    if(!ret_size) XDBG_ALLOC_ERROR;
    size = *ret_size;

    original = size;
    size = xdbg_alloc_align(size);
    if(size < original) XDBG_ALLOC_ERROR;

    *ret_size = size;
    if(ret_original) *ret_original = original;
    if(ret_xsize) {
                //info            //page_guard      //data //page_guard
        xsize = g_xdbg_pagesize + g_xdbg_pagesize + size + g_xdbg_pagesize;
        if(xsize < original) XDBG_ALLOC_ERROR;
        *ret_xsize = xsize;
    }

    return(size);
}



/*
mem: pointer to the beginning of the allocated memory block
*/
int xdbg_alloc_remove_mem_prev_next_ref(void *mem) {
    if(!XDBG_ALLOC_INDEX) return 0;

    xdbg_alloc_t    *xdbg_alloc;

    if(!mem) return -1;
    if(!g_xdbg_alloc) return -1;

    // note that mem is already verified so method 2 is not needed

    // method 1, faster
    xdbg_alloc = XDBG_ALLOC_MEM2INFO(mem);

    // method 2, safe
    //HASH_FIND_INT(g_xdbg_alloc, &mem, xdbg_alloc);
    //if(!xdbg_alloc) return -1;

    HASH_DEL(g_xdbg_alloc, xdbg_alloc);
    CDL_DELETE(g_xdbg_alloc, xdbg_alloc);
    return 0;
}



/*
new_mem: pointer to the beginning of the allocated memory block
*/
int xdbg_alloc_add_mem(void *new_mem) {
    if(!XDBG_ALLOC_INDEX) return 0;

    xdbg_alloc_t    *xdbg_alloc;

    if(!new_mem) return -1;

    xdbg_alloc = XDBG_ALLOC_MEM2INFO(new_mem);
    xdbg_alloc->addr = new_mem; // needed for the hash
    HASH_ADD_INT(g_xdbg_alloc, addr, xdbg_alloc);
    CDL_PREPEND(g_xdbg_alloc, xdbg_alloc);
    return 0;
}



void *xdbg_alloc_reuse(size_t size) {
    if(!XDBG_ALLOC_INDEX) return NULL;

    xdbg_alloc_t    *xdbg_alloc;

    CDL_FOREACH(g_xdbg_alloc, xdbg_alloc) {
        if(!xdbg_alloc->active) {
            if(xdbg_alloc->size >= size) return(xdbg_alloc);
        }
    }
    return NULL;
}



void *xdbg_alloc_check(void *mymem, int check_active) {
    if(!XDBG_ALLOC_INDEX) return((void *)(-1));

    xdbg_alloc_t    *xdbg_alloc;

    HASH_FIND_INT(g_xdbg_alloc, &mymem, xdbg_alloc);
    if(!xdbg_alloc) return NULL;
    if(check_active && !xdbg_alloc->active) return NULL;   // use-after-free
    return(xdbg_alloc);
}



void *xdbg_alloc_check_ptr(void *ptr, int check_active) {
    return(xdbg_alloc_check(XDBG_ALLOC_PTR2MEM(ptr), check_active));
}



size_t xdbg_alloc_size(void *ptr) {
    if(!ptr) return 0;
    xdbg_alloc_check_ptr(ptr, 1);
    return(XDBG_ALLOC_PTR2INFO(ptr)->original);
}



/*
ptr: the buffer visible by the main program
*/
void xdbg_real_free( void *ptr ) {
    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# real_free(%p) (called by %p)\n", ptr, __builtin_extract_return_addr(__builtin_return_address(0)));

    if(!ptr) return;

    if(XDBG_HEAPVALIDATE) {
        if(!HeapValidate(GetProcessHeap(), 0, NULL)) XDBG_ALLOC_ERROR;  // NO ptr!
    }

    if(!XDBG_ALLOC_ACTIVE) {
        if(g_xdbg_alloc) {
            if(xdbg_alloc_check_ptr(ptr, 0)) goto xdbg_real_free_continue;
        }
        real_free(ptr);
        return;
    }

    xdbg_real_free_continue:

    if(!xdbg_alloc_check_ptr(ptr, 0)) {
        #ifdef XDBG_ALLOC_ALLOW_UNKNOWN_FREE
        real_free(ptr);
        return;
        #else
        XDBG_ALLOC_ERROR;
        #endif
    }
    void    *mem;
    mem = XDBG_ALLOC_PTR2MEM(ptr);
    xdbg_alloc_remove_mem_prev_next_ref(mem);

    #ifndef WIN32
    size_t  xsize;  // xsize is a backup of the one contained in the
                    // structure because I clear (memset) it before
                    // the free, and so Linux can't work without it
    xsize = XDBG_ALLOC_MEM2INFO(mem)->xsize;
    #endif

    // may help with use-after-free
    #ifdef XDBG_ALLOC_FREECHR
    memset(ptr, XDBG_ALLOC_FREECHR, XDBG_ALLOC_MEM2INFO(mem)->size);            // data
    memset(XDBG_ALLOC_MEM2INFO(mem), XDBG_ALLOC_FREECHR, sizeof(xdbg_alloc_t)); // info (xdbg_alloc->size is made before)
    #endif

    #ifdef XDBG_USE_MALLOC
    real_free(mem);
    #else
    if(!VirtualFree(mem,
        #ifdef WIN32
        0
        #else
        xsize
        #endif
        , MEM_RELEASE)) XDBG_ALLOC_ERROR;
    #endif
}



/*
ptr: the buffer visible by the main program
*/
void xdbg_free( void *ptr ) {
    xdbg_alloc_t    *xdbg_alloc;

    if(!XDBG_ALLOC_INDEX) {
        xdbg_real_free(ptr);
        return;
    }

    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# free(%p) (called by %p)\n", ptr, __builtin_extract_return_addr(__builtin_return_address(0)));

    if(!ptr) return;

    if(XDBG_HEAPVALIDATE) {
        if(!HeapValidate(GetProcessHeap(), 0, NULL)) XDBG_ALLOC_ERROR;  // NO ptr!
    }

    if(!XDBG_ALLOC_ACTIVE) {
        if(g_xdbg_alloc) {
            xdbg_alloc = xdbg_alloc_check_ptr(ptr, 0);
            if(xdbg_alloc) {
                xdbg_real_free(XDBG_ALLOC_MEM2PTR(xdbg_alloc));
                return;
            }
        }
        real_free(ptr);
        return;
    }

    if(!xdbg_alloc_check_ptr(ptr, 1)) {
        // what if the pointer has been allocated by another library that doesn't use this system?
        // char *p = mydll_func();
        // free(p); // would return an error!
        #ifdef XDBG_ALLOC_ALLOW_UNKNOWN_FREE
        real_free(ptr);
        return;
        #else
        XDBG_ALLOC_ERROR;
        #endif
    }
    xdbg_alloc = XDBG_ALLOC_PTR2INFO(ptr);
    xdbg_alloc->active = 0;
    #ifdef XDBG_ALLOC_FREECHR
    memset(ptr, XDBG_ALLOC_FREECHR, xdbg_alloc->size);  // may help with use-after-free
    #endif
}



void xdbg_freeall(int only_the_not_active) {
    if(!XDBG_ALLOC_INDEX) return;

    xdbg_alloc_t    *xdbg_alloc,
                    *tmp1,
                    *tmp2;

    CDL_FOREACH_SAFE(g_xdbg_alloc, xdbg_alloc, tmp1, tmp2) {
        if(!only_the_not_active || (only_the_not_active && !xdbg_alloc->active)) {
            xdbg_real_free(XDBG_ALLOC_MEM2PTR(xdbg_alloc));
        }
    }
}



void xdbg_toggle(void) {
    if(XDBG_ALLOC_ACTIVE) {

        /*
        xdbg_alloc_t    *xdbg_alloc,
                        *tmp1,
                        *tmp2;
        void    *ptr;

        // create backup copies of the allocations
        // the problem is that pointers linked to the old locations will not work (use-after-free?)

        CDL_FOREACH_SAFE(g_xdbg_alloc, xdbg_alloc, tmp1, tmp2) {
            if(xdbg_alloc->active) {
                ptr = real_malloc(xdbg_alloc->original);    // no need of memset
                if(!ptr) XDBG_ALLOC_ERROR;
                memcpy(ptr, XDBG_ALLOC_MEM2PTR(xdbg_alloc), xdbg_alloc->original);
            }
        }

        xdbg_freeall(0);
        */
        
        // the solution is to set XDBG_ALLOC_ACTIVE to zero and check every
        // free to correctly handle the memory previously allocated

        XDBG_ALLOC_ACTIVE = 0;

    } else {

        // I guess there are problems to enable it at runtime due to previous allocations made with real_*.
        // the choices are:
        // - use real_* for pointer not in the database (impossible to recognize use-after-free bugs)
        // - return error if pointer if not in the database (will give ever error)

        XDBG_ALLOC_ACTIVE = 1;
    }
}



/*
ptr: the buffer visible by the main program
new_size: requested size
*/
void xdbg_realloc_page_guard(void *ptr, size_t new_size) {
    // unfortunately a continous usage of VirtualProtect
    // "kills" the performances in a terrible way so my idea
    // is simply to keep only the last PAGE_GUARD active
    // and leaving the rest as PAGE_READWRITE:
    //   1111233333333333333334444  malloc(16)
    //   11112333333..........4444  realloc(buff, 6)
    //   111123333333333......4444  realloc(buff, 10)
    // what really matters is that the end of the allocated
    // memory is guarded and so secure.
    // even the memset kills time!
    return;

    size_t  total_size;

    total_size = XDBG_ALLOC_PTR2INFO(ptr)->size;

    //total_size = xdbg_alloc_align(total_size);
    new_size = xdbg_alloc_align(new_size);
    if(total_size < new_size) XDBG_ALLOC_ERROR;

    #ifndef XDBG_USE_MALLOC
    DWORD   tmp;
    if(!VirtualProtect(
        ptr,
        new_size,
        PAGE_READWRITE, &tmp)) XDBG_ALLOC_ERROR;                // our memory
    #endif
    #ifdef XDBG_ALLOC_FREECHR
    memset(
        (unsigned char *)ptr + new_size,
        XDBG_ALLOC_FREECHR,
        total_size - new_size);
    #endif
    #ifndef XDBG_USE_MALLOC
    if(!VirtualProtect(
        (unsigned char *)ptr + new_size,
        (total_size - new_size) + g_xdbg_pagesize,
        g_xdbg_page_guard, &tmp)) XDBG_ALLOC_ERROR;    // post PAGE_GUARD
    #endif

    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# realloc_page_guard(%p, %u, %u) (called by %p)\n", ptr, (uint32_t)new_size, (uint32_t)total_size, __builtin_extract_return_addr(__builtin_return_address(0)));
}



void *xdbg_malloc( size_t size ) {
    if(!XDBG_ALLOC_ACTIVE) return(real_calloc(size, 1));

    xdbg_alloc_t    *xdbg_alloc;
    size_t  xsize,
            original;
    unsigned char   *mem;

    xdbg_alloc_init(&original, &size, &xsize);

    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# malloc(%u) -> %u %u (called by %p)\n", (uint32_t)original, (uint32_t)size, (uint32_t)xsize, __builtin_extract_return_addr(__builtin_return_address(0)));

    if(XDBG_HEAPVALIDATE) {
        if(!HeapValidate(GetProcessHeap(), 0, NULL)) XDBG_ALLOC_ERROR;
    }

    mem = xdbg_alloc_reuse(size);
    if(mem) {
        if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# xdbg_alloc_reuse %p (called by %p)\n", mem, __builtin_extract_return_addr(__builtin_return_address(0)));

        xdbg_alloc = XDBG_ALLOC_MEM2INFO(mem);
        xdbg_alloc->original    = original;
        xdbg_alloc->active      = 1;
        memset(XDBG_ALLOC_MEM2PTR(mem), 0, size);   // needed! think to calloc!
        //xdbg_realloc_page_guard(XDBG_ALLOC_MEM2PTR(mem), size);
    } else {
        int     retry;
        for(retry = 0; retry < 2; retry++) {
            #ifdef XDBG_USE_MALLOC
            mem = real_calloc(1, xsize);
            #else
            mem = VirtualAlloc(NULL, xsize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            #endif
            if(mem) break;
            if(!retry) xdbg_freeall(1); // let's try before failing...
        }
        if(!mem) XDBG_ALLOC_ERROR_RETURN;

        //ZeroMemory(mem, xsize);   // VirtualAlloc/real_calloc automatically put zeroes in the memory

        #ifndef XDBG_USE_MALLOC
        DWORD   tmp;
        //if(!VirtualProtect((unsigned char *)mem,                                         g_xdbg_pagesize,  PAGE_READWRITE,             &tmp)) XDBG_ALLOC_ERROR;    // information
        if(!VirtualProtect((unsigned char *)mem + g_xdbg_pagesize,                         g_xdbg_pagesize,  g_xdbg_page_guard, &tmp)) XDBG_ALLOC_ERROR;    // pre PAGE_GUARD
        //if(!VirtualProtect((unsigned char *)mem + g_xdbg_pagesize + g_xdbg_pagesize,         size,           PAGE_READWRITE,             &tmp)) XDBG_ALLOC_ERROR;    // our memory
        if(!VirtualProtect((unsigned char *)mem + g_xdbg_pagesize + g_xdbg_pagesize + size,  g_xdbg_pagesize,  g_xdbg_page_guard, &tmp)) XDBG_ALLOC_ERROR;    // post PAGE_GUARD
        #endif

        xdbg_alloc = XDBG_ALLOC_MEM2INFO(mem);
        xdbg_alloc->addr        = mem;  // set by xdbg_alloc_add_mem
        xdbg_alloc->original    = original;
        xdbg_alloc->size        = size;
        xdbg_alloc->xsize       = xsize;
        xdbg_alloc->active      = 1;
        xdbg_alloc->next        = NULL; // set by xdbg_alloc_add_mem
        xdbg_alloc->prev        = NULL; // set by xdbg_alloc_add_mem
        xdbg_alloc_add_mem(mem);
    }

    if(XDBG_ALLOC_VERBOSE)
        fprintf(stdout, "# %p (%p -> %p) = malloc(%u / %u / %u) (called by %p)\n",
            mem, XDBG_ALLOC_MEM2PTR(mem), (unsigned char *)mem + xsize, (uint32_t)original, (uint32_t)size, (uint32_t)xsize, __builtin_extract_return_addr(__builtin_return_address(0)));
    return(XDBG_ALLOC_MEM2PTR(mem));
}



void *xdbg_realloc( void *ptr, size_t size ) {
    xdbg_alloc_t    *xdbg_alloc;

    if(!XDBG_ALLOC_ACTIVE) {
        if(g_xdbg_alloc) {
            xdbg_alloc = xdbg_alloc_check_ptr(ptr, 0);
            if(xdbg_alloc) {
                ptr = real_calloc(size, 1);
                if(!ptr) XDBG_ALLOC_ERROR_RETURN;
                memcpy(ptr, XDBG_ALLOC_MEM2PTR(xdbg_alloc), (size < xdbg_alloc->original) ? size : xdbg_alloc->original);
                xdbg_real_free(XDBG_ALLOC_MEM2PTR(xdbg_alloc));
                return ptr;
            }
        }
        return(real_realloc(ptr, size));
    }

    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# realloc(%p, %u) (called by %p)\n", ptr, (uint32_t)size, __builtin_extract_return_addr(__builtin_return_address(0)));

    if(!ptr) return(xdbg_malloc(size));

    if(XDBG_HEAPVALIDATE) {
        if(!HeapValidate(GetProcessHeap(), 0, NULL)) XDBG_ALLOC_ERROR_RETURN;
    }

    size_t  original;
    void    *new_ptr;

    xdbg_alloc_init(&original, &size, NULL);
    if(!xdbg_alloc_check_ptr(ptr, 1)) XDBG_ALLOC_ERROR;

    xdbg_alloc = XDBG_ALLOC_PTR2INFO(ptr);
    if(xdbg_alloc->size >= size) {  // aligned_allocated versus aligned_requested
        if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# realloc reused same buffer %p (%u %u) (called by %p)\n", ptr, (uint32_t)size, (uint32_t)xdbg_alloc->size, __builtin_extract_return_addr(__builtin_return_address(0)));
        xdbg_alloc->original = original;
        //memset(ptr, 0, xdbg_alloc->size - original);  // takes really lot of time!
        //xdbg_realloc_page_guard(ptr, size);
        return(ptr);
    }

    // realloc_to_file mode
    int realloc_to_file = 0;
    static FILE *fd = NULL;
    size_t old_size = xdbg_alloc->size;
    int _xdbg_return_NULL_on_error = xdbg_return_NULL_on_error; // return NULL on error
    xdbg_return_NULL_on_error = 1;

    new_ptr = xdbg_malloc(original);

    xdbg_return_NULL_on_error = _xdbg_return_NULL_on_error;     // restore
    if(!new_ptr) {
        realloc_to_file = 1;
        if(!fd) {
            fd = tmpfile(); // tmpfile64 for > 2Gb only (Mingw doesn't link to it)
            if(!fd) XDBG_ALLOC_ERROR;
        }
        fseek(fd, 0, SEEK_SET);
        fwrite(ptr, 1, old_size, fd);
        xdbg_real_free(ptr);
        new_ptr = xdbg_malloc(original);
    }
    if(!new_ptr) XDBG_ALLOC_ERROR_RETURN;

    if(XDBG_ALLOC_VERBOSE) {
        void    *new_mem = XDBG_ALLOC_PTR2MEM(new_ptr);
        void    *old_mem = XDBG_ALLOC_PTR2MEM(ptr); // not a problem for realloc_to_file
        size_t  tmp = XDBG_ALLOC_MEM2INFO(new_mem)->xsize;
        fprintf(stdout, "# %p (%p -> %p) = realloc(%p, %u / %u / %u) (called by %p)\n",
            new_mem, new_ptr, (unsigned char *)new_mem + tmp, old_mem, (uint32_t)original, (uint32_t)size, (uint32_t)tmp, __builtin_extract_return_addr(__builtin_return_address(0)));
    }
    if(realloc_to_file) {
        fseek(fd, 0, SEEK_SET);
        fread(new_ptr, 1, old_size, fd);
    } else {
        CopyMemory(new_ptr, ptr, xdbg_alloc->original);
        xdbg_real_free(ptr);    // real_free because it's not good to leave it allocated wasting space
    }
    return(new_ptr);
}



void *xdbg_calloc( size_t num, size_t size ) {
    if(!XDBG_ALLOC_ACTIVE) return(real_calloc(num, size));

    if(XDBG_ALLOC_VERBOSE) fprintf(stdout, "# calloc(%u, %u) (called by %p)\n", (uint32_t)num, (uint32_t)size, __builtin_extract_return_addr(__builtin_return_address(0)));

    uint64_t    tmp;
    tmp = (uint64_t)num * (uint64_t)size;
    if(tmp > (uint64_t)0xffffffffLL) XDBG_ALLOC_ERROR;
    return(xdbg_malloc(tmp));
}



// replace any call to the original malloc with the new one.
// not needed because almost unused and only for x86
// and would make the XDBG_ALLOC_ACTIVE impossible to use
void xdbg_alloc_extreme(void) {
    #define XDBG_ALLOC_EXTREME_JMP(X) \
        p = (void *)real_##X; \
        if(VirtualProtect(p, 1 + sizeof(void *), PAGE_READWRITE, &tmp)) { \
            p[0] = 0xe9; \
            *((uint32_t *)(p + 1)) = ((void *)X) - (void *)(p + 1 + 4); \
            VirtualProtect(p, 1 + sizeof(void *), tmp, &tmp); \
        }

    #ifdef _X86_
    static int      init = 0;
    DWORD           tmp;
    unsigned char   *p;

    if(!init) {
        XDBG_ALLOC_EXTREME_JMP(malloc)
        XDBG_ALLOC_EXTREME_JMP(calloc)
        XDBG_ALLOC_EXTREME_JMP(realloc)
        XDBG_ALLOC_EXTREME_JMP(free)
        init = 1;
    }
    #endif
}

