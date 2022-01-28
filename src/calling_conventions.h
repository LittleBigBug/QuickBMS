/*
    Copyright 2010-2021 Luigi Auriemma

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

/*
  x86 32bit calling conventions 0.2.2b
  ------------------------------------
  quick library for calling the 32bit functions (mainly dumped functions)
  that use the following calling conventions:
  cdecl, stdcall, thiscall, msfastcall, borland, pascal, watcom, safecall,
  syscall, optlink and clarion.

  compatible with gcc and mingw only which grants its working on both
  Windows and *nix x86 systems.

  the functions have been written in this "easy-to-use" way just
  for being used in any project other than QuickBMS for which were
  created and with the maximum compatibility in mind.
  everything done here is necessary to preserve the stack and
  allowing the correct assignment of the arguments to the registers.
  note that all these functions have not been tested at 100%, my
  only doubt is about the preserved registers at return in some
  of them (like syscall).

  how to use:
    #include "calling_conventions.h"
    ret = cdecl_call(original_32bit_function, 3, "arg1", arg2, 0x3);

  limitations:
  compatible with gcc and mingw only, some doubts about values
  different than the return type (I choosed long because usually it
  automatically fits the size of the system) and about arguments (for
  example a 64bit argument using the 32bit calling convention will
  not work)
*/



#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>



// 32bit registers
// this is a project for reverse engineering stuff so I want to support
// the 32bit dumped code on both x86 and x86_64
#if defined(i386) || defined(IA64)

    #define CALLCONV_INIT   va_list ap; \
                            long    i, \
                                    ret, \
                                    argv[argc]; \
                            \
                            va_start(ap, argc); \
                            for(i = 0; i < argc; i++) { \
                                argv[i] = va_arg(ap, long); \
                            } \
                            va_end(ap);
    #define CALLCONV_ASM    __asm__ __volatile__ 

    #define CALLCONV_EBX    ,"%ebx" // clobbered
    #define CALLCONV_EDI    ,"%edi" // constraint
    #define CALLCONV_ESI    ,"%esi" // constraint
    #define CALLCONV_CONSTRAINTS
    #if __PIC__ // both -fpic and -fPIC
        #define CALLCONV_EBX  // "error: PIC register clobbered by '%ebx' in 'asm'"
        #ifndef WIN32   // it works on Linux only with -O0
            #define CALLCONV_EDI
            #define CALLCONV_ESI
            #undef  CALLCONV_CONSTRAINTS
        #endif
    #endif


    // too complex, not implemented yet:
    // http://blogs.msdn.com/b/vcblog/archive/2013/07/12/introducing-vector-calling-convention.aspx
    // long vectorcall_call(void *func, long argc, ...);



    // this is just an experiment
    // the first argument is the register to return, the next 6 arguments are assigned to the registers
    // and then there are the stack arguments. so remember that argc does not correspond to the real
    // number of arguments, if you have only one stack argument you must have argc set to 1 + 6 + 1 and 1 + 6
    // NULL arguments before your one
    long usercall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        long ret_type = -1;  // from 0 to 5 for covering the 6 registers, -1 for void

        if(argc > 0) ret_type = argv[0];

        int tmp_argc = argc - (1 + 6);
        for(i = tmp_argc - 1; i >= 0; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[1 + 6 + i]));
        }

        // eax ecx edx ebx esi edi
#ifdef CALLCONV_CONSTRAINTS
        if(argc > 6) CALLCONV_ASM("movl %0, %%edi" :: "g"(argv[6]) : "%eax", "%ecx", "%edx" CALLCONV_EBX CALLCONV_ESI             );
        if(argc > 5) CALLCONV_ASM("movl %0, %%esi" :: "g"(argv[5]) : "%eax", "%ecx", "%edx" CALLCONV_EBX              CALLCONV_EDI);
#endif
        if(argc > 4) CALLCONV_ASM("movl %0, %%ebx" :: "g"(argv[4]) : "%eax", "%ecx", "%edx"              CALLCONV_ESI CALLCONV_EDI);
        if(argc > 3) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[3]) : "%eax", "%ecx"         CALLCONV_EBX CALLCONV_ESI CALLCONV_EDI);
        if(argc > 2) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[2]) : "%eax",         "%edx" CALLCONV_EBX CALLCONV_ESI CALLCONV_EDI);
        if(argc > 1) CALLCONV_ASM("movl %0, %%eax" :: "g"(argv[1]) :         "%ecx", "%edx" CALLCONV_EBX CALLCONV_ESI CALLCONV_EDI);

                     CALLCONV_ASM("call *%0"       :: "g"(func)    : "%eax", "%ecx", "%edx" CALLCONV_EBX CALLCONV_ESI CALLCONV_EDI);

        switch(ret_type) {
            case 0: CALLCONV_ASM("movl %%eax, %0" : "=g"(ret)); break;
            case 1: CALLCONV_ASM("movl %%ecx, %0" : "=g"(ret)); break;
            case 2: CALLCONV_ASM("movl %%edx, %0" : "=g"(ret)); break;
            case 3: CALLCONV_ASM("movl %%ebx, %0" : "=g"(ret)); break;
            case 4: CALLCONV_ASM("movl %%esi, %0" : "=g"(ret)); break;
            case 5: CALLCONV_ASM("movl %%edi, %0" : "=g"(ret)); break;
            default: ret = 0; break;
        }
        return(ret);
    }



    long cdecl_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 0; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        CALLCONV_ASM("call *%0" :: "g"(func));
        CALLCONV_ASM("add %0, %%esp" :: "g"(argc * 4));
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long stdcall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 0; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        CALLCONV_ASM("call *%0" :: "g"(func));
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long thiscall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 1; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }
        if(argc > 0) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[0]));

        CALLCONV_ASM("call *%0" :: "g"(func) : "%ecx");
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long msfastcall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 2; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }
        if(argc > 1) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[1]) : "%ecx"       );
        if(argc > 0) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[0]) :        "%edx");

        CALLCONV_ASM("call *%0" :: "g"(func) : "%ecx","%edx");
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    // borland, delphi, register
    long borland_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = 3; i < argc; i++) { // left->right
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }
        if(argc > 2) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[2]) : "%eax","%edx"       );
        if(argc > 1) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[1]) : "%eax",       "%ecx");
        if(argc > 0) CALLCONV_ASM("movl %0, %%eax" :: "g"(argv[0]) :        "%edx","%ecx");

        CALLCONV_ASM("call *%0" :: "g"(func) : "%eax","%edx","%ecx");
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long pascal_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = 0; i < argc; i++) { // left->right
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        CALLCONV_ASM("call *%0" :: "g"(func));
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long watcom_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 4; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        if(argc > 3) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[3]) : "%eax","%edx"        CALLCONV_EBX);
        if(argc > 2) CALLCONV_ASM("movl %0, %%ebx" :: "g"(argv[2]) : "%eax","%edx","%ecx"             );
        if(argc > 1) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[1]) : "%eax"       ,"%ecx" CALLCONV_EBX);
        if(argc > 0) CALLCONV_ASM("movl %0, %%eax" :: "g"(argv[0]) :        "%edx","%ecx" CALLCONV_EBX);

        CALLCONV_ASM("call *%0" :: "g"(func) : "%eax","%edx","%ecx" CALLCONV_EBX);
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    long safecall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        ret = 0;
        CALLCONV_ASM("push %0" :: "g"(&ret));

        for(i = argc - 1; i >= 0; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        CALLCONV_ASM("call *%0" :: "g"(func));
        // eax is 0 if all ok or another value in case of exceptions
        return(ret);
    }



    long syscall_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 0; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        CALLCONV_ASM("movl %0, %%eax" :: "g"(argc)); // from Wikipedia but it looks false
        CALLCONV_ASM("call *%0" :: "g"(func) : "%eax");  // note that is not clear the thing of
        CALLCONV_ASM("add %0, %%esp" :: "g"(argc * 4));
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));      // preservation but should be ok like in cdecl
        return(ret);
    }



    // optlink, visualage
    long optlink_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = argc - 1; i >= 3; i--) {    // right->left
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }
        if(argc > 2) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[2]) : "%eax","%edx"       );
        if(argc > 1) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[1]) : "%eax",       "%ecx");
        if(argc > 0) CALLCONV_ASM("movl %0, %%eax" :: "g"(argv[0]) :        "%edx","%ecx");

        CALLCONV_ASM("call *%0" :: "g"(func) : "%eax","%edx","%ecx");
        CALLCONV_ASM("add %0, %%esp" :: "g"(argc * 4));
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        return(ret);
    }



    // clarion, TopSpeed, JPI
    long clarion_call(void *func, long argc, ...) {
        CALLCONV_INIT

        for(i = 4; i < argc; i++) { // left->right
            CALLCONV_ASM("push %0" :: "g"(argv[i]));
        }

        if(argc > 3) CALLCONV_ASM("movl %0, %%edx" :: "g"(argv[3]) : "%eax","%ecx"        CALLCONV_EBX);
        if(argc > 2) CALLCONV_ASM("movl %0, %%ecx" :: "g"(argv[2]) : "%eax",       "%edx" CALLCONV_EBX);
        if(argc > 1) CALLCONV_ASM("movl %0, %%ebx" :: "g"(argv[1]) : "%eax","%ecx","%edx"             );
        if(argc > 0) CALLCONV_ASM("movl %0, %%eax" :: "g"(argv[0]) :        "%ecx","%edx" CALLCONV_EBX);

        CALLCONV_ASM("call *%0" :: "g"(func) : "%eax","%ecx","%edx" CALLCONV_EBX);
        CALLCONV_ASM("movl %%eax, %0" : "=g"(ret));
        //CALLCONV_ASM("movl %%edx, %0" : "=g"(ret));  // for pointers
        return(ret);
    }



#else

    #define usercall_call(FUNC, ARGC, ...)   FUNC(__VA_ARGS__) // wrong
    #define cdecl_call(FUNC, ARGC, ...)      (__cdecl   FUNC)(__VA_ARGS__)
    #ifdef WIN32    // don't use #ifdef __stdcall here
    #define stdcall_call(FUNC, ARGC, ...)    (__stdcall FUNC)(__VA_ARGS__)
    #else
    #define stdcall_call(FUNC, ARGC, ...)    (          FUNC)(__VA_ARGS__)
    #endif
    #define thiscall_call(FUNC, ARGC, ...)   FUNC(__VA_ARGS__)
    #define msfastcall_call(FUNC, ARGC, ...) FUNC(__VA_ARGS__)
    #define borland_call(FUNC, ARGC, ...)    FUNC(__VA_ARGS__)
    #define pascal_call(FUNC, ARGC, ...)     FUNC(__VA_ARGS__)
    #define watcom_call(FUNC, ARGC, ...)     FUNC(__VA_ARGS__)
    #define safecall_call(FUNC, ARGC, ...)   FUNC(__VA_ARGS__)
    #define syscall_call(FUNC, ARGC, ...)    FUNC(__VA_ARGS__)
    #define optlink_call(FUNC, ARGC, ...)    FUNC(__VA_ARGS__)
    #define clarion_call(FUNC, ARGC, ...)    FUNC(__VA_ARGS__)

#endif

