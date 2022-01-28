// http://bellard.org/tcc/
// modified by Luigi Auriemma

#ifndef LIBTCC_H
#define LIBTCC_H

#ifndef LIBTCCAPI
# define LIBTCCAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct TCCState;

typedef struct TCCState TCCState;

/* create a new TCC compilation context */
LIBTCCAPI TCCState *(*tcc_new)(void) = NULL;

/* free a TCC compilation context */
LIBTCCAPI void (*tcc_delete)(TCCState *s) = NULL;

/* set CONFIG_TCCDIR at runtime */
LIBTCCAPI void (*tcc_set_lib_path)(TCCState *s, const char *path) = NULL;

/* set error/warning display callback */
LIBTCCAPI void (*tcc_set_error_func)(TCCState *s, void *error_opaque,
    void (*error_func)(void *opaque, const char *msg)) = NULL;

/* set options as from command line (multiple supported) */
LIBTCCAPI void (*tcc_set_options)(TCCState *s, const char *str) = NULL;

/*****************************/
/* preprocessor */

/* add include path */
LIBTCCAPI int (*tcc_add_include_path)(TCCState *s, const char *pathname) = NULL;

/* add in system include path */
LIBTCCAPI int (*tcc_add_sysinclude_path)(TCCState *s, const char *pathname) = NULL;

/* define preprocessor symbol 'sym'. Can put optional value */
LIBTCCAPI void (*tcc_define_symbol)(TCCState *s, const char *sym, const char *value) = NULL;

/* undefine preprocess symbol 'sym' */
LIBTCCAPI void (*tcc_undefine_symbol)(TCCState *s, const char *sym) = NULL;

/*****************************/
/* compiling */

/* add a file (C file, dll, object, library, ld script). Return -1 if error. */
LIBTCCAPI int (*tcc_add_file)(TCCState *s, const char *filename) = NULL;

/* compile a string containing a C source. Return -1 if error. */
LIBTCCAPI int (*tcc_compile_string)(TCCState *s, const char *buf) = NULL;

/*****************************/
/* linking commands */

/* set output type. MUST BE CALLED before any compilation */
LIBTCCAPI int (*tcc_set_output_type)(TCCState *s, int output_type) = NULL;
#define TCC_OUTPUT_MEMORY   1 /* output will be run in memory (default) */
#define TCC_OUTPUT_EXE      2 /* executable file */
#define TCC_OUTPUT_DLL      3 /* dynamic library */
#define TCC_OUTPUT_OBJ      4 /* object file */
#define TCC_OUTPUT_PREPROCESS 5 /* only preprocess (used internally) */

/* equivalent to -Lpath option */
LIBTCCAPI int (*tcc_add_library_path)(TCCState *s, const char *pathname) = NULL;

/* the library name is the same as the argument of the '-l' option */
LIBTCCAPI int (*tcc_add_library)(TCCState *s, const char *libraryname) = NULL;

/* add a symbol to the compiled program */
LIBTCCAPI int (*tcc_add_symbol)(TCCState *s, const char *name, const void *val) = NULL;

/* output an executable, library or object file. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int (*tcc_output_file)(TCCState *s, const char *filename) = NULL;

/* link and run main() function and return its value. DO NOT call
   tcc_relocate() before. */
LIBTCCAPI int (*tcc_run)(TCCState *s, int argc, char **argv) = NULL;

/* do all relocations (needed before using tcc_get_symbol()) */
LIBTCCAPI int (*tcc_relocate)(TCCState *s1, void *ptr) = NULL;
/* possible values for 'ptr':
   - TCC_RELOCATE_AUTO : Allocate and manage memory internally
   - NULL              : return required memory size for the step below
   - memory address    : copy code to memory passed by the caller
   returns -1 if error. */
#define TCC_RELOCATE_AUTO (void*)1

/* return symbol value or NULL if not found */
LIBTCCAPI void *(*tcc_get_symbol)(TCCState *s, const char *name) = NULL;

#ifdef __cplusplus
}
#endif

#endif



#include "libtcc_dll.h"



int TCC_libtcc_init(void) {
#ifdef WIN32
    static HMODULE hlib = NULL;
    if(hlib) return 0;
    hlib = (void *)MemoryLoadLibrary((void *)libtcc_dll, sizeof(libtcc_dll) - 1);
    if(!hlib) return -1;
    #define TCC_GetProcAddress(X) { \
        if(!X) { \
            X = (void *)MemoryGetProcAddress(hlib, #X); \
            /* if(!X) return -1; */ \
        } \
    }
    TCC_GetProcAddress(tcc_new)
    TCC_GetProcAddress(tcc_delete)
    TCC_GetProcAddress(tcc_set_lib_path)
    TCC_GetProcAddress(tcc_set_error_func)
    TCC_GetProcAddress(tcc_set_options)
    TCC_GetProcAddress(tcc_add_include_path)
    TCC_GetProcAddress(tcc_add_sysinclude_path)
    TCC_GetProcAddress(tcc_define_symbol)
    TCC_GetProcAddress(tcc_undefine_symbol)
    TCC_GetProcAddress(tcc_add_file)
    TCC_GetProcAddress(tcc_compile_string)
    TCC_GetProcAddress(tcc_set_output_type)
    TCC_GetProcAddress(tcc_add_library_path)
    TCC_GetProcAddress(tcc_add_library)
    TCC_GetProcAddress(tcc_add_symbol)
    TCC_GetProcAddress(tcc_output_file)
    TCC_GetProcAddress(tcc_run)
    TCC_GetProcAddress(tcc_relocate)
    TCC_GetProcAddress(tcc_get_symbol)
    return 0;
#else
    return -1;
#endif
}
