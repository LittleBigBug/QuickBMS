/*
    Copyright 2009-2022 Luigi Auriemma

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

// QuickBMS general functions



static const u8     hex_codes[] = "0123456789abcdef";



// the macro is better for size and debugging (no stack canary and ret)
static inline void set_int3(const void *dllname, const void *hlib, const void *funcname, const void *funcaddr, const void *argc) {
//#define set_int3(dllname, hlib, funcname, funcaddr, argc) {
    if(g_int3) {
#if defined(__i386__) || defined(__x86_64__)
#if __x86_64__
        __asm__ __volatile__ ("movq %0, %%rax" :: "g"(dllname)  :        "rcx", "rdx", "rsi", "rdi");
        __asm__ __volatile__ ("movq %0, %%rcx" :: "g"(hlib)     : "rax",        "rdx", "rsi", "rdi");
        __asm__ __volatile__ ("movq %0, %%rdx" :: "g"(funcname) : "rax", "rcx",        "rsi", "rdi");
        __asm__ __volatile__ ("movq %0, %%rsi" :: "g"(funcaddr) : "rax", "rcx", "rdx",        "rdi");
        __asm__ __volatile__ ("movq %0, %%rdi" :: "g"(argc)     : "rax", "rcx", "rdx", "rsi"       );
#else
        __asm__ __volatile__ ("movl %0, %%eax" :: "g"(dllname)  :        "ecx", "edx", "esi", "edi");
        __asm__ __volatile__ ("movl %0, %%ecx" :: "g"(hlib)     : "eax",        "edx", "esi", "edi");
        __asm__ __volatile__ ("movl %0, %%edx" :: "g"(funcname) : "eax", "ecx",        "esi", "edi");
        __asm__ __volatile__ ("movl %0, %%esi" :: "g"(funcaddr) : "eax", "ecx", "edx",        "edi");
        __asm__ __volatile__ ("movl %0, %%edi" :: "g"(argc)     : "eax", "ecx", "edx", "esi"       );
#endif
        __asm__ __volatile__ ("int3");
        __asm__ __volatile__ ("nop");
#endif
    }
}



#ifndef FOREGROUND_BLUE
    //typedef unsigned int    DWORD;
    //typedef void*           HANDLE;
    #define STD_OUTPUT_HANDLE   stdout
    #define STD_INPUT_HANDLE    stdin
    typedef struct {
        int     Bottom;
        int     Top;
    } SMALL_RECT;
    typedef struct {
      unsigned short    wAttributes;
      SMALL_RECT        srWindow;
    } CONSOLE_SCREEN_BUFFER_INFO;
    int SetConsoleTextAttribute(HANDLE A, int B) { return 0; }
    HANDLE GetStdHandle(FILE *A) { return fileno(A); }
    int GetConsoleScreenBufferInfo(HANDLE A, CONSOLE_SCREEN_BUFFER_INFO *B) {
        memset(B, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
        B->srWindow.Top = 50;
    }
    #define FOREGROUND_BLUE	1
    #define FOREGROUND_GREEN	2
    #define FOREGROUND_RED	4
    #define FOREGROUND_INTENSITY	8
    #define BACKGROUND_BLUE	16
    #define BACKGROUND_GREEN	32
    #define BACKGROUND_RED	64
    #define BACKGROUND_INTENSITY	128
#endif
int _rgb(u8 *str) {
    static int  original = -1;
    if(original < 0) {
        CONSOLE_SCREEN_BUFFER_INFO  csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        original = csbi.wAttributes;
    }

    int     i = 0; // returned
    int     color = 0;
    if(!str || !str[0]) {
        color = original;
    } else {
        for(i = 0; str[i]; i++) {
            switch(str[i]) {
                case 'o':
                case '0': color |= original;                break;
                case 'r': color |= FOREGROUND_RED;          break;
                case 'g': color |= FOREGROUND_GREEN;        break;
                case 'b': color |= FOREGROUND_BLUE;         break;
                case 'i': color |= FOREGROUND_INTENSITY;    break;
                case 'R': color |= BACKGROUND_RED;          break;
                case 'G': color |= BACKGROUND_GREEN;        break;
                case 'B': color |= BACKGROUND_BLUE;         break;
                case 'I': color |= BACKGROUND_INTENSITY;    break;
                case 'x': original = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; color = original; break;
                default:  return -1;                        break;
            }
        }
    }
    if(!color) color = original;    // just in case (already handled by !str[0])
    if(color == FOREGROUND_INTENSITY) color |= original;
    if(color == BACKGROUND_INTENSITY) color |= original;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    //SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), color);   // wrong, do not enable
    return i;
}



int _ansi_color_foreground(u8 chr) {
    u8  *p = strchr(hex_codes, tolower(chr));
    if(!p) return -1;
    int color = 0;
    int code = p - (u8 *)hex_codes;
    if(code & 1) color |= FOREGROUND_RED;
    if(code & 2) color |= FOREGROUND_GREEN;
    if(code & 4) color |= FOREGROUND_BLUE;
    if(code & 8) color |= FOREGROUND_INTENSITY;
    return color;
}

int _ansi_color_background(u8 chr) {
    u8  *p = strchr(hex_codes, tolower(chr));
    if(!p) return -1;
    int color = 0;
    int code = p - (u8 *)hex_codes;
    if(code & 1) color |= BACKGROUND_RED;
    if(code & 2) color |= BACKGROUND_GREEN;
    if(code & 4) color |= BACKGROUND_BLUE;
    if(code & 8) color |= BACKGROUND_INTENSITY;
    return color;
}

int ansi_colors(u8 *str) {
    static int  original = -1;
    if(original < 0) {
        CONSOLE_SCREEN_BUFFER_INFO  csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        original = csbi.wAttributes;
    }

    int     t;
    int     color = 0;
    if(!str || !str[0]) {
        color = original;
    } else {
        if(strlen(str) <= 2) {
            t = _ansi_color_foreground(str[0]);
            if(t < 0) t = original;
            color |= t;
            if(strlen(str) == 2) {
                t = _ansi_color_background(str[1]);
                if(t < 0) t = original;
                color |= t;
            }
        } else {
            if(stristr(str, "Black"))   color = 0;
            if(stristr(str, "Red"))     color = FOREGROUND_RED;
            if(stristr(str, "Green"))   color =                  FOREGROUND_GREEN;
            if(stristr(str, "Yellow"))  color = FOREGROUND_RED | FOREGROUND_GREEN;
            if(stristr(str, "Blue"))    color =                                     FOREGROUND_BLUE;
            if(stristr(str, "Magenta")) color = FOREGROUND_RED                    | FOREGROUND_BLUE;
            if(stristr(str, "Cyan"))    color =                  FOREGROUND_GREEN | FOREGROUND_BLUE;
            if(stristr(str, "White"))   color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            if(stristr(str, "bright"))  color |= FOREGROUND_INTENSITY;
        }
    }
    if(!color) color = original;    // just in case (already handled by !str[0])
    if(color == FOREGROUND_INTENSITY) color |= original;
    if(color == BACKGROUND_INTENSITY) color |= original;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    //SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), color);   // wrong, do not enable
    return 0;
}



u8 *show_dump(int left, u8 *data, int len, FILE *stream) {
    int                 rem;
    u8                  leftbuff[80],
                        buff[67],
                        chr,
                        *bytes,
                        *p,
                        *limit,
                        *glimit = data + len;

    #define show_dump_stream(X,Y) { \
        if(stream) { \
            fwrite(X, 1, Y, stream); \
        } else { \
            out_buffsz += Y; \
            myalloc(&out_buff, out_buffsz + 1, NULL); \
            memcpy(out_buff + out_buffsz - Y, X, Y); \
            out_buff[out_buffsz] = 0; \
        } \
    }
    int     out_buffsz  = 0;
    u8      *out_buff   = NULL;

    if(!stream) {
        out_buff = calloc(1, 1);
        out_buff[0] = 0;
    }

    if(!data) return NULL;
    if(len < 0) return NULL;
    memset(buff + 2, ' ', 48);
    memset(leftbuff, ' ', sizeof(leftbuff));

    while(data < glimit) {
        limit = data + 16;
        if(limit > glimit) {
            limit = glimit;
            memset(buff, ' ', 48);
        }

        p     = buff;
        bytes = p + 50;
        while(data < limit) {
            chr = *data;
            *p++ = hex_codes[chr >> 4];
            *p++ = hex_codes[chr & 15];
            p++;
            *bytes++ = ((chr < ' ') || (chr >= 0x7f)) ? '.' : chr;
            data++;
        }
        *bytes++ = '\n';

        for(rem = left; rem >= sizeof(leftbuff); rem -= sizeof(leftbuff)) {
            show_dump_stream(leftbuff, sizeof(leftbuff))
        }
        if(rem > 0) show_dump_stream(leftbuff, rem)
        show_dump_stream(buff, (bytes - buff))
    }

    return out_buff;
}



void show_hex_chr(unsigned char chr) {
    fputc(hex_codes[chr >> 4], stdout);
    fputc(hex_codes[chr & 15], stdout);
}



void show_hex(u8 *data, int size) {
    int     i;
    if(!data) return;
    for(i = 0; i < size; i++) {
        show_hex_chr(data[i]);
    }
}



int check_extension(u8 *fname, u8 *ext) {
    u8      *p;

    if(!fname || !ext) return 0;
    p = strrchr(fname, '.');
    if(!p) return 0;
    p++;
    if(!stricmp(p, ext)) return 1;
    return 0;
}



int myabs(int n) {
    if(n < 0) {
        n = -n;
        if(n < 0) n = 0;    // 0x80000000
    }
    return n;
}



u8 *mystrcpy(u8 *dst, u8 *src, int max) {
    u8      *p,
            *l;

    if(dst && (max > 0)) {
        if(!src) src = "";
        p = dst;
        l = dst + max - 1;
        while(p < l) {
            if(!*src) break;
            *p++ = *src++;
        }
        *p = 0;
    }
    return(dst);
}



u8 *mystrdup_simple(u8 *str) { // multiplatform compatible
    int     len;
    u8      *o  = NULL;

    if(str) {
        len = strlen(str);
        o = malloc(len + 1);
        if(!o) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memcpy(o, str, len + 1);
    }
    return o;
}



u8 *mystrdup(u8 **old_buff, u8 *str) { // multiplatform compatible
    int     len;
    u8      *o  = NULL;

    if(old_buff) o = *old_buff;
    if(str) {
        len = strlen(str);
        o = realloc(o, len + 1);
        if(!o) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memcpy(o, str, len + 1);
    } else {
        o = NULL;
    }
    if(old_buff) {
        if(!o) {
            FREE(*old_buff)
        } else {
            *old_buff = o;
        }
    }
    return o;
}



u8 *mystrchrs(u8 *str, u8 *chrs) {
    //int     i;
    u8      *p,
            *ret = NULL;

    if(str && chrs) {
        for(p = str; *p; p++) {
            if(strchr(chrs, *p)) return(p);
        }
        /*
        for(i = 0; chrs[i]; i++) {
            p = strchr(str, chrs[i]);
            if(p && (!ret || (p < ret))) {
                ret = p;
            }
        }
        */
    }
    return ret;
}



u8 *mystrrchrs(u8 *str, u8 *chrs) {
    //int     i;
    u8      *p,
            *ret = NULL;

    if(str && chrs) {
        for(p = str + strlen(str) - 1; p >= str; p--) {
            if(strchr(chrs, *p)) return(p);
        }
        /*
        for(i = 0; chrs[i]; i++) {
            p = strrchr(str, chrs[i]);
            if(p) {
                str = p;
                ret = p;
            }
        }
        */
    }
    return ret;
}



#define mystrstr    stristr
#define mystrrstr   strristr

/*
u8 *mystrstr(u8 *str, u8 *s) {
    u8      *p;

    if(str && s) {
        for(p = str; *p; p++) {
            if(!stricmp(p, s)) return(p);
        }
    }
    return NULL;
}

u8 *mystrrstr(u8 *str, u8 *s) {
    int     slen;
    u8      *p;

    if(str && s) {
        slen = strlen(s);
        for(p = str + strlen(str) - slen; p >= str; p--) {
            if(!stricmp(p, s)) return(p);
        }
    }
    return NULL;
}
*/



// avoid NULL pointers
#define check_strcmp_args \
    if(!a && !b) return 0; \
    if(!a) return -1; \
    if(!b) return 1;

i32 mystrcmp(const char *a, const char *b) {
    check_strcmp_args
    return real_strcmp(a, b);
}
i32 mystricmp(const char *a, const char *b) {
    check_strcmp_args
    return real_stricmp(a, b);
}
i32 mystrncmp(const char *a, const char *b, i32 n) {
    check_strcmp_args
    if(n < 0) return 0;
    return real_strncmp(a, b, n);
}
i32 mystrnicmp(const char *a, const char *b, i32 n) {
    check_strcmp_args
    if(n < 0) return 0;
    return real_strnicmp(a, b, n);
}
i32 mymemicmp(const void *a, const void *b, size_t n) {
    check_strcmp_args
    if(n < 0) return 0;
    i32 res;
    unsigned char   *x = (unsigned char *)a,
                    *y = (unsigned char *)b;
    while(n--) {
        res = toupper(*x) - toupper(*y);
        if(res < 0) return -1;
        if(res > 0) return 1;
        x++;
        y++;
    }
    return 0;
}



int strcmpx(u8 *a, u8 *b) {
    if(!a) a = "";
    if(!b) b = "";
    return strnicmp(a, b, strlen(b));
}



int strcmp_end(u8 *a, u8 *b) {
    if(!a) a = "";
    if(!b) b = "";
    int asz = strlen(a);
    int bsz = strlen(b);
    if(asz < bsz) return -1;
    return strnicmp(a + asz - bsz, b, bsz);
}



int get_my_endian(void) {
    int endian = 1;
    if(!*(char *)&endian) return MYBIG_ENDIAN;  // big endian
    return MYLITTLE_ENDIAN;                     // little endian
}



/*
swprintf is horrible...
1) swprintf doesn't have the sizeOfBuffer field on old versions of Windows C library, so it's behaviour in gcc 4 and 7 is different
2) __mingw_swprintf is bugged on gcc 7 so do NOT use it!
3) _swprintf_p is not even linked...
4) 2021: DO NOT USE swprintf! it didn't work on gcc 10 due to the different prototype (for example "rb" became "r")
#define QUICKBMS_SWPRINTF
*/
#ifdef QUICKBMS_SWPRINTF
#ifdef WIN32    // gcc4 vs gcc7
    #if __MSVCRT_VERSION__ >= __MSVCR80_DLL
    #else
        #define myswprintf(X,Y,Z,...)   swprintf(X, Z, __VA_ARGS__)
    #endif
#endif
#ifndef myswprintf
    #define myswprintf(X,Y,Z,...)   swprintf(X, Y, Z, __VA_ARGS__)
#endif
#endif

#ifdef WIN32
#define tmp_strcpy      strcpy
#define tmp_Wstrcpy     mywstrcpy
UINT_PTR CALLBACK OFN_DUMMY_HOOK(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    return 0;
}

// the following is totally useless since it's unused, file opening happens before arguments parsing
void *build_lpstrFilter(void) {
    static void *ret    = NULL;

    if(!g_filter_in_files) return NULL;

    int     i,
            ret_len = 0;

    char    *f;
    for(i = 0; (f = g_filter_in_files[i]); i++) {
        ret_len += 1 + strlen(f) + 1 + 1;   // visible filter
        ret_len += strlen(f) + 1;           // real filter
    }
    ret_len++;
    ret_len++;

    ret_len *= sizeof(wchar_t);
    ret = realloc(ret, ret_len + sizeof(wchar_t));
    if(!ret) STD_ERR(QUICKBMS_ERROR_MEMORY);
    *(wchar_t *)ret = 0;

    wchar_t *fW = ret;
    for(i = 0; (f = g_filter_in_files[i]); i++) {
        #ifdef QUICKBMS_SWPRINTF
        myswprintf(fW, (ret_len - ((void *)fW - (void *)ret)) / sizeof(wchar_t), L"(%s)", native_utf8_to_unicode(f));
        fW += wcslen(fW) + 1;   // correct
        myswprintf(fW, (ret_len - ((void *)fW - (void *)ret)) / sizeof(wchar_t), L"%s",   native_utf8_to_unicode(f));
        fW += wcslen(fW) + 1;   // correct
        #else
        mywstrcpy(fW, L"(");                        fW += wcslen(fW);
        mywstrcpy(fW, native_utf8_to_unicode(f));   fW += wcslen(fW);
        mywstrcpy(fW, L")");                        fW += wcslen(fW);
        fW += 1;    // correct
        mywstrcpy(fW, native_utf8_to_unicode(f));   fW += wcslen(fW);
        fW += 1;    // correct
        #endif
    }
    *fW++ = 0;
    *fW++ = 0;
    return ret;
}

    // Windows 8.1 has a bug that crashes quickbms if there is no hook,
    // here I use dwMinorVersion >= 2 because GetVersionEx reports 2 for
    // both 8 (which is safe) and 8.1.
    // Note: now I use the manifest that returns the correct version (6.3)
#define get_file_work_around(ofn) \
    if( \
        ((g_osver.dwMajorVersion >= 6) && (g_osver.dwMinorVersion >= 3)) \
     || (g_is_gui && !XDBG_ALLOC_ACTIVE)    /* maybe the safe allocation has been disabled by the exception handler when restarted */ \
    ) { \
        ofn.Flags      |= OFN_ENABLEHOOK; \
        ofn.lpfnHook   = OFN_DUMMY_HOOK; \
    } \

char *get_file(char *title, i32 bms, i32 multi) {
    int     maxlen;
    char    *filename;

    if(multi) {
        maxlen = 0x100000;  // was MULTI_PATHSZ: 32k limit ansi, no limit unicode (apparently there is a limit of 0xffff chars, 16bit in lpstrFile)
    } else {
        maxlen = PATHSZ;
    }
    if(g_osver.dwMajorVersion <= 4) {       // ansi
        if(maxlen > 32768) maxlen = 32768;  // mandatory!
    }
    filename = calloc(maxlen + 1, 1);
    if(!filename) STD_ERR(QUICKBMS_ERROR_MEMORY);

    printf("- %s\n", title);

#define _get_file(W,L) \
    filename##W[0] = 0; \
    memset(&ofn##W, 0, sizeof(ofn##W)); \
    ofn##W.lStructSize     = (g_osver.dwMajorVersion <= 4) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(ofn##W); \
    if(bms) { \
        ofn##W.lpstrFilter = \
            L## \
            "script/plugin (bms/txt/wcx)\0"  "*.bms;*.txt;*.wcx\0" \
            /* "WCX plugin\0"  "*.wcx\0" */ \
            "(*.*)\0"       "*.*\0" \
            "\0"            "\0"; \
    } else { \
        ofn##W.lpstrFilter = build_lpstrFilter(); \
        if(!ofn##W.lpstrFilter) \
        ofn##W.lpstrFilter = \
            L## \
            "(*.*)\0"       "*.*\0" \
            "\0"            "\0"; \
    } \
    ofn##W.nFilterIndex = 1; \
    ofn##W.lpstrFile    = filename##W; \
    ofn##W.nMaxFile     = maxlen; \
    ofn##W.lpstrTitle   = title##W; \
    ofn##W.Flags        = OFN_PATHMUSTEXIST | \
                          OFN_FILEMUSTEXIST | \
                          OFN_LONGNAMES     | \
                          OFN_EXPLORER      | \
                          0x10000000 /*OFN_FORCESHOWHIDDEN*/ | \
                          OFN_ENABLESIZING  | \
                          OFN_HIDEREADONLY  | \
                          OFN_NOVALIDATE | \
                          0; \
    \
    if(multi) ofn##W.Flags |= OFN_ALLOWMULTISELECT; \
    get_file_work_around(ofn##W) \
    \
    if(!GetOpenFileName##W(&ofn##W)) exit(1); // terminate immediately

    if(g_osver.dwMajorVersion <= 4) {
        // ANSI version
        OPENFILENAME    ofn;
        _get_file(,)
    } else {
        // UNICODE version
        OPENFILENAMEW   ofnW;
        wchar_t titleW[strlen(title)+1];
        #ifdef QUICKBMS_SWPRINTF
        myswprintf(titleW, sizeof(titleW) / sizeof(wchar_t), L"%s", native_utf8_to_unicode(title));
        #else
        mywstrcpy(titleW, native_utf8_to_unicode(title));
        #endif
        wchar_t *filenameW = calloc(maxlen + 1, sizeof(wchar_t));
        if(!filenameW) STD_ERR(QUICKBMS_ERROR_MEMORY);
        _get_file(W,L)
        if(multi) {
            char    *f  = filename;
            wchar_t *fW = filenameW;
            for(;;) {
                mystrcpy(f, native_unicode_to_utf8(fW), (maxlen - (f - filename)) + 1);
                if(!f[0]) break;
                f  += strlen(f)  + 1;
                fW += wcslen(fW) + 1;   // correct
            }
        } else {
            mystrcpy(filename, native_unicode_to_utf8(filenameW), maxlen + 1);
        }
        free(filenameW);
    }

    return(filename);
}

char *get_folder(char *title) {
    char    *p;
    char    *filename;
    int     maxlen = PATHSZ;

    filename = malloc(maxlen + 1);
    if(!filename) STD_ERR(QUICKBMS_ERROR_MEMORY);

    printf("- %s\n", title);

#define _get_folder(W,L) \
    tmp_##W##strcpy( filename##W,                      L##"enter in the output folder and press Save"); \
    memset(&ofn##W, 0, sizeof(ofn##W)); \
    ofn##W.lStructSize  = (g_osver.dwMajorVersion <= 4) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(ofn##W); \
    ofn##W.lpstrFilter  = L## "(*.*)\0" "*.*\0" "\0" "\0"; \
    ofn##W.nFilterIndex = 1; \
    ofn##W.lpstrFile    = filename##W; \
    ofn##W.nMaxFile     = maxlen; \
    ofn##W.lpstrTitle   = title##W; \
    ofn##W.Flags        = OFN_PATHMUSTEXIST | \
                          /* removed for folders OFN_FILEMUSTEXIST | */ \
                          OFN_LONGNAMES     | \
                          OFN_EXPLORER      | \
                          0x10000000 /*OFN_FORCESHOWHIDDEN*/ | \
                          OFN_ENABLESIZING  | \
                          OFN_HIDEREADONLY  | \
                          OFN_NOVALIDATE | \
                          0; \
    \
    get_file_work_around(ofn##W) \
    \
    if(!GetSaveFileName##W(&ofn##W)) exit(1); // terminate immediately

    if(g_osver.dwMajorVersion <= 4) {
        // ANSI version
        OPENFILENAME    ofn;
        _get_folder(,)
    } else {
        // UNICODE version
        OPENFILENAMEW   ofnW;
        wchar_t titleW[strlen(title)+1];
        #ifdef QUICKBMS_SWPRINTF
        myswprintf(titleW, sizeof(titleW) / sizeof(wchar_t), L"%s", native_utf8_to_unicode(title));
        #else
        mywstrcpy(titleW, native_utf8_to_unicode(title));
        #endif
        wchar_t *filenameW = calloc(maxlen + 1, sizeof(wchar_t));
        if(!filenameW) STD_ERR(QUICKBMS_ERROR_MEMORY);
        _get_folder(W,L)
        mystrcpy(filename, native_unicode_to_utf8(filenameW), maxlen + 1);
        free(filenameW);
    }

    p = mystrrchrs(filename, PATH_DELIMITERS);
    if(p) *p = 0;
    return(filename);
}
#undef tmp_strcpy
#undef tmp_Wstrcpy
#endif



u8 *skip_begin_string(u8 *p) {
    if(p) {
        while(*p) {
            if(*p > ' ') break;
            p++;
        }
    }
    return(p);
}



u8 *skip_end_string(u8 *p) {
    u8      *l;

    if(p) {
        for(l = p + strlen(p) - 1; l >= p; l--) {
            if(*l > ' ') break;
            *l = 0;
        }
        return l + 1;
    }
    return p;
}



u8 *skip_delimit(u8 *p) {
    p = skip_begin_string(p);
    skip_end_string(p);
    return(p);
}



int delimit(u8 *str) {
    u8      *p;
    if(!str) return -1;
    for(p = str; *p && (*p != '\n') && (*p != '\r'); p++);
    *p = 0;
    p = skip_end_string(str);
    if(!p) return 0;
    return p - str;
}



int fgetz(u8 *data, int datalen, FILE *fd, u8 *fmt, ...) {
    va_list ap;
    u8      *p;

    if(datalen <= 0) return -1;
    if(!data) data = alloca(datalen + 1);   // stack
    if(!data) return -1;
    if(fmt) {
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
    data[0] = 0;
    if(!fgets(data, datalen, fd)) {
        if(fd == stdin) myexit(QUICKBMS_ERROR_UNKNOWN);
        else            return -1;
    }
    return delimit(data);
}



u64 readbase(u8 *data, QUICKBMS_int size, QUICKBMS_int *readn) {
    static const u8 table[256] =    // fast performances
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xff\xff\xff\xff\xff\xff"
            "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
            "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
    u64     num     = 0;
    int     sign;
    u8      c,
            *s,
            *hex_fix;

    s = data;
    if(!data || !size || !data[0]) {
        // do nothing (for readn)
    } else {
        // useful in some occasions, for example if the input is external!
        for(; *s; s++) {
            if(!strchr(" \t\r\n", *s)) break;
        }
        if(*s == '-') {
            sign = -1;
            s++;
        } else {
            sign = 0;
        }
        hex_fix = s;
        for(; *s; s++) {
            c = *s;
            //if((c == 'x') || (c == 'X') || (c == '$')) {  // auto base switching
            if(
                (((c == 'h') || (c == 'x') || (c == 'X')) && (s > hex_fix)) // 0x and 100h, NOT x123 or h123
             || (c == '$')                                                  // $1234 or 1234$
            ) {
                size = 16;
                continue;
            }
            c = table[c];
            if(c >= size) break;    // necessary to recognize the invalid chars based on the size
            num = (num * size) + c;
        }
        if(sign) num = -num;
    }
    if(readn) *readn = s - data;
    return(num);
}



int myisalnum(int chr) {
    if((chr >= '0') && (chr <= '9')) return 1;
    if((chr >= 'a') && (chr <= 'z')) return 1;
    if((chr >= 'A') && (chr <= 'Z')) return 1;
    if(chr == '-') return 1;   // negative number
    //if(chr == '+') return 1;   // positive number
    return 0;
}



int myishexdigit(int chr) {
    if((chr >= 'A') && (chr <= 'F')) return 1;
    if((chr >= 'a') && (chr <= 'f')) return 1;
    if((chr >= '0') && (chr <= '9')) return 1; // this is enough because hex start ever with 0x
    if(chr == '-') return 1;   // negative number
    //if(chr == '+') return 1;   // positive number
    //if(chr == '$') return 1;   // delphi/vb hex
    return 0;
}



int myisdigit(int chr) {
    if((chr >= '0') && (chr <= '9')) return 1; // this is enough because hex start ever with 0x
    if(chr == '-') return 1;   // negative number
    //if(chr == '+') return 1;   // positive number
    //if(chr == '$') return 1;   // delphi/vb hex
    return 0;
}



int myisdigitstr(u8 *str) { // only a quick version
    int     i;

    if(!str) return 0;
    if(!myisdigit(str[0])) return 0;
    for(i = 1; str[i]; i++) {
        if(i >= NUMBERSZ) return 0;    // avoid to waste time with long strings
        if(!strchr("0123456789abcdefABCDEFx$", str[i])) return 0;
    }
    return 1;
}



u8 *myitoa(int num) {
    static const u8 table[] = "0123456789abcdef";
    static u8       dstx[MULTISTATIC][3 + NUMBERSZ + 1] = {{""}};
    static int      dsty = 0;
    u_int   unum;
    u8      tmp[NUMBERSZ + 1],  // needed because hex numbers are inverted, I have already done various tests and this is the fastest!
            *p,                 // even faster than using directly dst as output
            *t,
            *dst;

    dst = (u8 *)dstx[dsty++ % MULTISTATIC];

    if(!num) {  // quick way, 0 is used enough often... ok it's probably useless
        dst[0] = '0';
        dst[1] = 0;
        return(dst);
    }

    p = dst;
    if(num < 0) {
        num = -num; //don't use myabs(num)
        *p++ = '-';
    }
    unum = num; // needed for the sign... many troubles

    //if((unum >= 0) && (unum <= 9)) {  // quick solution for numbers under 10, so uses only one char, (unum >= 0) avoids problems with 0x80000000
        //*p++ = table[unum];
        //*p   = 0;
        //return(dst);
    //}
    t = tmp + (NUMBERSZ - 1);   // the -1 is needed (old tests)
    *t = 0;
    t--;
    if(g_decimal_notation) {
        do {   // "unum" MUST be handled at the end of the cycle! example: 0
            *t = table[unum % (u_int)10];
            unum = unum / (u_int)10;
            if(!unum) break;
            t--;
        } while(t >= tmp);
    } else {
        *p++ = '0'; // hex notation is better for debugging
        *p++ = 'x';
        do {   // "unum" MUST be handled at the end of the cycle! example: 0
            *t = table[unum & 15];
            unum = unum >> (u_int)4;
            if(!unum) break;
            t--;
        } while(t >= tmp);
    }
    strcpy(p, t);

    //sprintf(dst, "%"PRId"", unum);  // old "one-instruction-only" solution, mine is better
    return(dst);
}



// it's all binary despite the name
u8 *strdupcpy(u8 *dst, int *dstlen, u8 *src, int srclen) {
    int     tmp;

    if(srclen < 0) {
        if(src) srclen = strlen(src);
        else    srclen = 0;
    }

    /*
    // normal solution
    //if(srclen < STRINGSZ) srclen = STRINGSZ;  // disabled for testing
    if(dstlen) *dstlen = srclen;
    dst = realloc(dst, srclen + 2); // unicode
    if(!dst) STD_ERR(QUICKBMS_ERROR_MEMORY);
    // normal solution
    */

    // optimized solution
    if(!dstlen) {
        dstlen = &tmp;
        *dstlen = -1;
    }
    if(!dst || (*dstlen < srclen) || (*dstlen < 2)) {   // NULL + unicode to avoid srclen 0
        *dstlen = srclen;
        //MAX_ALLOC_CHECK(*dstlen);        // note that dstlen can't be < 0 due to the "srclen < 0" check
        //if(*dstlen == -2) ALLOC_ERR;        // big endian undelimited unicode
        if(*dstlen < STRINGSZ) *dstlen = STRINGSZ;    // better for numbers and common filenames
        dst = realloc(dst, (*dstlen) + 2);  // big endian undelimited unicode (now it's rare but in future it may be more used)
        if(!dst) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }
    // optimized solution

    if(dst) {
        if(src) STR_MEMCPY(dst, src, srclen);
        else    memset    (dst, 0,   srclen);
        dst[srclen]     = 0;
        dst[srclen + 1] = 0;    // big endian undelimited unicode
    }
    return(dst);
}



u8 *re_strdup(u8 **ret_dst, u8 *src, int *retlen) {  // only for NULL delimited strings, NOT bytes!
    u8      *dst;

    if(ret_dst) dst = *ret_dst;
    else        dst = NULL;
    dst = strdupcpy(dst, retlen, src, -1);

    /*
    int     dstlen  = -1;
    // dst && src checked by strdupcpy
    if(retlen) dstlen = *retlen;
    dst = strdupcpy(dst, &dstlen, src, -1);
    if(retlen) *retlen = dstlen;
    */

    if(ret_dst) *ret_dst = dst;
    return(dst);
}



int strdup_replace(u8 **dstp, u8 *src, int src_len, int *dstp_len) {  // should improve a bit the performances
    if(!dstp) return -1;
    *dstp = strdupcpy(*dstp, dstp_len, src, src_len);

    /*
    int     dst_len = -1;
    u8      *dst;

    if(!dstp) return -1;
    dst = *dstp;

    if(!dstp_len && dst) {
        dst_len = strlen(dst);  // or is it better to use "dst_len = 0"?
    } else if(dstp_len) {
        dst_len = *dstp_len;
    }

    dst = strdupcpy(dst, &dst_len, src, src_len);

    *dstp = dst;
    if(dstp_len) *dstp_len = dst_len;
    */
    return 0;
}



int myisprint(int c) {
    if(c <= 0x1f) return 0;
    if(c >= 0x7f) return 0;
    return 1;
}



int myisdechex_string(u8 *str) {
    QUICKBMS_int    len;

    // I have already verified that using a quick test only on the first char doesn't improve the performances if compared to the current full check
    if(!str) return 0;

    // sort of isprint, necessary to avoid that invalid strings are handled as numbers because readbase ignores these bytes
    int     i;
    for(i = 0; str[i]; i++) {
        if(!myisprint(str[i])) return 0;
    }

    readbase(str, 10, &len);    // no need to know the number
    if(len <= 0) return 0;     // FALSE
    if(len != strlen(str)) return 0;    // otherwise there are huge problems with GetArray and If
                                        // the downside is the lost of compatibility with rare things like 123;!
    return 1;                  // TRUE
}



int isempty_string(u8 *str) {
    if(!str) return 1;
    while(*str) {
        if(myisprint(*str)) return 0;
        str++;
    }
    return 1;
}



u16 swap16(u16 n) {
    n = (((n & 0xff00) >> 8) |
         ((n & 0x00ff) << 8));
    return(n);
}



u32 swap24(u32 n) {
    n = (((n & 0xff0000) >> 16) |
         ((n & 0x00ff00)      ) |
         ((n & 0x0000ff) << 16));
    return(n);
}



u32 swap32(u32 n) {
    n = (((n & 0xff000000) >> 24) |
         ((n & 0x00ff0000) >>  8) |
         ((n & 0x0000ff00) <<  8) |
         ((n & 0x000000ff) << 24));
    return(n);
}



u64 swap64(u64 n) {
//#ifdef QUICKBMS64
    n = (((n & (u64)0xFF00000000000000ULL) >> (u64)56) |
         ((n & (u64)0x00FF000000000000ULL) >> (u64)40) |
         ((n & (u64)0x0000FF0000000000ULL) >> (u64)24) |
         ((n & (u64)0x000000FF00000000ULL) >> (u64) 8) |
         ((n & (u64)0x00000000FF000000ULL) << (u64) 8) |
         ((n & (u64)0x0000000000FF0000ULL) << (u64)24) |
         ((n & (u64)0x000000000000FF00ULL) << (u64)40) |
         ((n & (u64)0x00000000000000FFULL) << (u64)56));
//#else
//    n = swap32(n);
//#endif
    return(n);
}



u16 swap16le(u16 n) {
    if(get_my_endian()) return swap16(n);   // be cpu
    return n;                               // le cpu
}
u16 swap16be(u16 n) {
    if(get_my_endian()) return n;           // be cpu
    return swap16(n);                       // le cpu
}



u32 swap32le(u32 n) {
    if(get_my_endian()) return swap32(n);   // be cpu
    return n;                               // le cpu
}
u32 swap32be(u32 n) {
    if(get_my_endian()) return n;           // be cpu
    return swap32(n);                       // le cpu
}



u64 swap64le(u64 n) {
    if(get_my_endian()) return swap64(n);   // be cpu
    return n;                               // le cpu
}
u64 swap64be(u64 n) {
    if(get_my_endian()) return n;           // be cpu
    return swap64(n);                       // le cpu
}



u16 myhtons(u16 n) {
    if(get_my_endian()) return(n);
    return(swap16(n));
}
u16 myntohs(u16 n) {
    if(get_my_endian()) return(n);
    return(swap16(n));
}
u32 myhtonl(u32 n) {
    if(get_my_endian()) return(n);
    return(swap32(n));
}
u32 myntohl(u32 n) {
    if(get_my_endian()) return(n);
    return(swap32(n));
}



 // code derived from ReactOS rot.c
 u32 myrotl(  u32 value, int shift );
 u32 myrotr(  u32 value, int shift );
 u64 mylrotl( u64 value, int shift );
 u64 mylrotr( u64 value, int shift );
 u32 myrotl(  u32 value, int shift )
 {
     int max_bits = sizeof(u32)<<3;
     if ( shift < 0 )
         return myrotr(value,-shift);
 
     if ( shift > max_bits )
         shift = shift % max_bits;
     return (value << shift) | (value >> (max_bits-shift));
 }
 u32 myrotr( u32 value, int shift )
 {
     int max_bits = sizeof(u32)<<3;
     if ( shift < 0 )
         return myrotl(value,-shift);
 
     if ( shift > max_bits )
         shift = shift % max_bits;
     return (value >> shift) | (value <<  (max_bits-shift));
 }
 u64 mylrotl( u64 value, int shift )
 {
     int max_bits = sizeof(u64)<<3;
     if ( shift < 0 )
         return mylrotr(value,-shift);
 
     if ( shift > max_bits )
         shift = shift % max_bits;
     return (value << shift) | (value >> (max_bits-shift));
 }
 u64 mylrotr( u64 value, int shift )
 {
     int max_bits = sizeof(u64)<<3;
     if ( shift < 0 )
         return mylrotl(value,-shift);
 
     if ( shift > max_bits )
         shift = shift % max_bits;
     return (value >> shift) | (value << (max_bits-shift));
 }



int dump128num(u8 *digest, u64 low, u64 high) {
    int     i;
    for(i = 0; i < 8; i++) {
        digest[7 - i] = high;
        high >>= (u64)8;
    }
    for(i = 0; i < 8; i++) {
        digest[15 - i] = low;
        low >>= (u64)8;
    }
    return 16;
}



char *stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;

      if(!String) return 0;
      if(!Pattern) Pattern = "";

      for (start = (char *)String; *start; start++)
      {
            /* find start of pattern in string */
            for ( ; (*start && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if (!*start)
                  return 0;

            pptr = (char *)Pattern;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if (!*pptr)
                        return (start);
            }
      }
      return 0;
}



char *strnistr(const char *String, int String_len, const char *Pattern, int Pattern_len)
{
      char *pptr, *sptr, *start;

      if(!String) return 0;
      if(!Pattern) Pattern = "";
      if(String_len  < 0) String_len  = strlen(String);
      if(Pattern_len < 0) Pattern_len = strlen(Pattern);
      char *end = (char *)String + String_len;

      for (start = (char *)String; start < end; start++)
      {
            /* find start of pattern in string */
            for ( ; ((start < end) && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if (start >= end) //(!*start)
                  return 0;

            pptr = (char *)Pattern;
            sptr = (char *)start;
            char *eptr = (char *)Pattern + Pattern_len;

            while ((sptr < end) && (pptr < eptr) && (toupper(*sptr) == toupper(*pptr)))
            {
                  sptr++;
                  pptr++;
            }
                  /* if end of pattern then pattern was found */
                  if (pptr >= eptr) //(!*pptr)
                        return (start);
      }
      return 0;
}



u8 *strristr(u8 *s1, u8 *s2) {
    int     s1n,
            s2n;
    u8      *p;

    if(!s1 || !s2) return NULL;
    s1n = strlen(s1);
    s2n = strlen(s2);
    if(s2n > s1n) return NULL;
    for(p = s1 + (s1n - s2n); p >= s1; p--) {
        if(!strnicmp(p, s2, s2n)) return(p);
    }
    return NULL;
}



u8 *strrnistr(u8 *s1, int s1n, u8 *s2, int s2n) {
    u8      *p;

    if(!s1 || !s2) return NULL;
    if(s1n < 0) s1n = strlen(s1);
    if(s2n < 0) s2n = strlen(s2);
    if(s2n > s1n) return NULL;
    for(p = s1 + (s1n - s2n); p >= s1; p--) {
        if(!strnicmp(p, s2, s2n)) return(p);
    }
    return NULL;
}



int vspr(u8 **buff, const u8 *fmt, va_list ap) {
    va_list ap_bck;
    va_copy(ap_bck, ap);

    int     len,
            mlen;
    u8      *ret    = NULL;

    // NO, never! if(buff) *buff = NULL;
    if(buff) ret = *buff;
    
    if(!fmt) return 0;
    mlen = strlen(fmt) + 128;
    for(;;) {
        ret = realloc(ret, mlen + 1 + 1);   // snprintf is a mess, better to use an additional +1
        if(!ret) return 0;     // return -1;
        len = vsnprintf(ret, mlen + 1, fmt, ap);
        if((len >= 0) && (len < mlen)) break;
        va_copy(ap, ap_bck);
        if(len > mlen) mlen = len;
        else           mlen += 128;
    }
    ret[len] = 0;
    if(buff) *buff = ret;
    return len;
}



int spr(u8 **buff, const u8 *fmt, ...) {
    va_list ap;
    int     len;

    va_start(ap, fmt);
    len = vspr(buff, fmt, ap);
    va_end(ap);
    return len;
}



u8 *spr2(const u8 *fmt, ...) {
    va_list ap;
    u8      *ret    = NULL;

    va_start(ap, fmt);
    vspr(&ret, fmt, ap);
    va_end(ap);
    return ret;
}



int find_replace_string(u8 **mybuf, int *buflen, u8 *old, int oldlen, u8 *news, int newlen) {
    int     i,
            len,
            //len_bck,
            tlen,
            found,
            ret     = 0;
    u8      *nbuf,
            *buf,
            *p;

    if(!mybuf) return ret;
    buf = *mybuf;
    if(!buf) return ret;
    found  = 0;
    len = -1;
    if(buflen) len = *buflen;
    if(len < 0) len = strlen(buf);
    if(oldlen < 0) {
        oldlen = 0;
        if(old) oldlen = strlen(old);
    }
    if(oldlen <= 0) return ret;
    tlen    = len - oldlen;
    //len_bck = len;

    for(i = 0; i <= tlen; i++) {
        if(!strnicmp(buf + i, old, oldlen)) found++;
    }
    if(!found) return ret;  // nothing to change: return buf or a positive value

    //if(!news) return ret;  // if we want to know only if the searched string has been found, we will get NULL if YES and buf if NOT!!!
    if(newlen < 0) {
        newlen = 0;
        if(news) newlen = strlen(news);
    }

    if(newlen <= oldlen) {  // if the length of new string is equal/minor than the old one don't waste space for another buffer
        nbuf = buf;
    } else {                // allocate the new size
        nbuf = calloc(len + ((newlen - oldlen) * found) + 1, 1);
        if(!nbuf) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }

    p = nbuf;
    for(i = 0; i <= tlen;) {
        if(!strnicmp(buf + i, old, oldlen)) {
            STR_MEMCPY(p, news, newlen);
            p += newlen;
            i += oldlen;
            ret++;
        } else {
            *p++ = buf[i];
            i++;
        }
    }
    while(i < len) {
        *p++ = buf[i];
        i++;
    }
    len = p - nbuf;
    if(buflen) *buflen = len;
    nbuf[len] = 0;  // hope the original input string has the +1 space
    if(nbuf != buf) FREE(buf)
    *mybuf = nbuf;
    return ret;
}



u8 *numbers_to_bytes(u8 *str, int *ret_size, int hex, int is_const) {
    static int  buffsz  = 0;
    static u8   *buff   = NULL;
    u_int   num;
    QUICKBMS_int len;
    int     i,
            t,
            size,
            slash_fix;
    u8      *s;

    if(ret_size) *ret_size = 0;
    if(!str) return NULL;
    if(is_const) goto dump_string;

    // try to guess non numbers, for example: filexor "mypassword"
    for(s = str; *s; s++) {
        if(*s <= ' ') continue;
        // number
            if(hex) {
                if(myishexdigit(*s)) break;
            } else {
                if(myisdigit(*s) || (*s == '$')) break;
            }
        if(*s == '\\') break;       // \x12

dump_string:
        // dump string
        s = str;    // dump also the initial spaces
        size = strlen(s);
        if(size > buffsz) {
            buffsz = size;
            buff = realloc(buff, buffsz + 1);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        strcpy(buff, s);
        goto quit;
    }

    s = str;
    for(i = 0; *s;) {
        if(*s <= ' ') {
            s++;
            continue;
        }

        // yeah so it can handle also \x11\x22\x33
        slash_fix = -1;
        if(*s == '\\') {
            slash_fix = s - str;
            *s = '0';
        }

        //while(*s && !(myisdigit(*s) || (*s == '$'))) s++;  // this one handles also dots, commas and other bad chars

        // this one handles also dots, commas and other bad chars
        while(*s) {
            if(hex) {
                if(myishexdigit(*s)) break;
            } else {
                if(myisdigit(*s) || (*s == '$')) break;
            }
            s++;
        }

        num = readbase(s, hex ? 16 : 10, &len);

        if(slash_fix >= 0) str[slash_fix] = '\\';

        if(len <= 0) break;

        t = 1;
        if(((int)num > 0xff) || ((int)num < -0xff)) t = 4;

        if((i + t) > buffsz) {
            buffsz += t + STRINGSZ;
            buff = realloc(buff, buffsz + 1);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        i += putxx(buff + i, num, t);

        s += len;
    }
    if(!buff) {
        buff = realloc(buff, buffsz + 1);
        if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }
    buff[i] = 0; // useless, only for possible new usages in future //, return ret as NULL
    size = i;
quit:
    if(ret_size) *ret_size = size;
    if(g_verbose > 0) {
        printf("- numbers_to_bytes of %d bytes\n ", (i32)size);
        for(i = 0; i < size; i++) printf(" 0x%02x", buff[i]);
        printf("\n");
    }
    return buff;
}



// use real_* for improving speed avoiding the xalloc stuff
files_t *add_files(u8 *fname, u64 fsize, int *ret_files) {
    static int      filesi  = 0,
                    filesn  = 0;
    static files_t  *files  = NULL;
    files_t         *ret;

    if(ret_files) {
        *ret_files = filesi;
        files = real_realloc(files, sizeof(files_t) * (filesi + 1)); // not needed, but it's ok
        if(!files) STD_ERR(QUICKBMS_ERROR_MEMORY);
        files[filesi].name   = NULL;
        //files[filesi].offset = 0;
        files[filesi].size   = 0;
        ret    = files;
        filesi = 0;
        filesn = 0;
        files  = NULL;
        return ret;
    }

    if(!fname) return NULL;

    if(fname[0] == '.') {   // remove the initial .\ ./
        if((fname[1] == '/') || (fname[1] == '\\')) fname += 2;
    }

    if(check_wildcards(fname, g_filter_in_files) < 0) return NULL;

    if(filesi >= filesn) {
        filesn += 1024;
        files = real_realloc(files, sizeof(files_t) * filesn);
        if(!files) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memset(&files[filesi], 0, sizeof(files_t) * (filesn - filesi));
    }

    //mystrdup_simple(fname);
    files[filesi].name   = real_realloc(files[filesi].name, strlen(fname) + 1); // realloc in case of reusage
    if(!files[filesi].name) STD_ERR(QUICKBMS_ERROR_MEMORY);
    strcpy(files[filesi].name, fname);

    //files[filesi].offset = 0;
    files[filesi].size   = fsize;
    filesi++;
    return NULL;
}



#ifdef WIN32
    // nothing
#else
// just a work-around, it saves some memory by avoiding to collect all the files recursively
static u8 *quick_simple_tmpname_scanner_filter_filedir = NULL;
static int quick_simple_tmpname_scanner_filter(const struct dirent *namelist) {
    if(!strcmp(namelist->d_name, ".") || !strcmp(namelist->d_name, "..")) return 0;
    if(check_wildcard(namelist->d_name, quick_simple_tmpname_scanner_filter_filedir) < 0) return 0;
    return 1;
}
#endif



// copy of recursive_dir without support for unicode since it's useless here
int quick_simple_tmpname_scanner(u8 *filedir, int filedirsz) {
    int     plen,
            namelen,
            ret     = -1;
    u8      *p;

#ifdef WIN32
    static int      winnt = -1;
    OSVERSIONINFO   osver;
    WIN32_FIND_DATA wfd;
    HANDLE          hFind = INVALID_HANDLE_VALUE;

    if(winnt < 0) {
        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osver);
        if(osver.dwPlatformId >= VER_PLATFORM_WIN32_NT) {
            winnt = 1;
        } else {
            winnt = 0;
        }
    }
#else
    struct  stat    xstat;
    struct  dirent  **namelist;
    int     n,
            i;
#endif

    p = strrchr(filedir, '.');
    if(p) {
        strcpy(p, ".*");
    } else {
        // the following is untested, was invalid before this patch
        p = mystrrchrs(filedir, PATH_DELIMITERS);
        if(p) p++;
        else  p = filedir + strlen(filedir);
        sprintf(p, "%"PRIx".*", g_extracted_files);
    }

    plen = 0;
    p = mystrrchrs(filedir, PATH_DELIMITERS);
    if(p) plen = (p + 1) - filedir;

#ifdef WIN32
    if(winnt) { // required to avoid problems with Vista and Windows7!
        hFind = FindFirstFileEx(filedir, FindExInfoStandard, &wfd, FindExSearchNameMatch, NULL, 0);
    } else {
        hFind = FindFirstFile(filedir, &wfd);
    }
    if(hFind == INVALID_HANDLE_VALUE) goto quit;
    do {
        if(!strcmp(wfd.cFileName, ".") || !strcmp(wfd.cFileName, "..")) continue;
        p = wfd.cFileName;
#else
    // scandir works only with directories
    quick_simple_tmpname_scanner_filter_filedir = filedir;
    n = scandir(".", &namelist, &quick_simple_tmpname_scanner_filter, NULL);
    if(n < 0) goto quit;
    for(i = 0; i < n; i++) {
        //if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
        //if(check_wildcard(namelist[i]->d_name, filedir) < 0) continue;
        p = namelist[i]->d_name;
#endif

        namelen = strlen(p);
        if((plen + namelen) >= filedirsz) goto quit;
        strcpy(filedir + plen, p);
        memcpy(filedir + plen, p, namelen);
        filedir[plen + namelen] = 0;

#ifdef WIN32
        if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
        if(stat(filedir, &xstat) < 0) goto quit;
        if(S_ISDIR(xstat.st_mode))
#endif

        {
            // no recursion
        } else {
            // file found!
            ret = 0;
            break;
        }

#ifdef WIN32
    } while(FindNextFile(hFind, &wfd));
quit:
    if(hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
#else
        FREE(namelist[i]);
    }
quit:
    for(; i < n; i++) FREE(namelist[i]);
    FREE(namelist);
#endif
    return ret;
}



#define recursive_dir_skip_path 0
//#define recursive_dir_skip_path 2



#ifdef WIN32

static HANDLE WINAPI (*myFindFirstFileExW)(
  LPCWSTR            lpFileName,
  FINDEX_INFO_LEVELS fInfoLevelId,
  LPVOID             lpFindFileData,
  FINDEX_SEARCH_OPS  fSearchOp,
  LPVOID             lpSearchFilter,
  DWORD              dwAdditionalFlags
) = NULL;

static BOOL WINAPI (*myFindNextFileW)(
  HANDLE             hFindFile,
  LPWIN32_FIND_DATAW lpFindFileData
) = NULL;

int recursive_dirW(wchar_t *filedir, int filedirsz) {
    int     plen,
            namelen,
            ret     = -1;

    WIN32_FIND_DATAW wfd;
    HANDLE          hFind = INVALID_HANDLE_VALUE;

    plen = wcslen(filedir);
    if((plen + 4) >= filedirsz) goto quit;
    mywstrcpy(filedir + plen, L"\\*.*");
    plen++;

    hFind = myFindFirstFileExW(filedir, FindExInfoStandard, &wfd, FindExSearchNameMatch, NULL, 0);
    if(hFind == INVALID_HANDLE_VALUE) goto quit;
    do {
        if((wfd.cFileName[0] == '.') && !wfd.cFileName[1]) continue;    // "."
        if((wfd.cFileName[0] == '.') && (wfd.cFileName[1] == '.') && !wfd.cFileName[2]) continue;   // ".."
        //if(!mywstrcmp(wfd.cFileName, L".") || !mywstrcmp(wfd.cFileName, L"..")) continue;

        namelen = wcslen(wfd.cFileName);
        if((plen + namelen) >= filedirsz) goto quit;
        mywstrcpy(filedir + plen, wfd.cFileName);

        if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            recursive_dirW(filedir, filedirsz);  // NO goto quit
        } else {
            add_files(native_unicode_to_utf8(filedir + recursive_dir_skip_path), ((u64)wfd.nFileSizeHigh << (u64)(sizeof(wfd.nFileSizeLow) * 8)) | (u64)wfd.nFileSizeLow, NULL);
        }
    } while(myFindNextFileW(hFind, &wfd));
    ret = 0;

quit:
    if(hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
    filedir[plen - 1] = 0;
    return ret;
}
#endif



int recursive_dir(u8 *filedir, int filedirsz) {
    int     plen,
            namelen,
            ret     = -1;

    if(!filedir) return ret;
#ifdef WIN32

    static int      winnt = -1;
    OSVERSIONINFO   osver;
    WIN32_FIND_DATA wfd;
    HANDLE          hFind = INVALID_HANDLE_VALUE;

    if(winnt < 0) {
        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osver);
        if(osver.dwPlatformId >= VER_PLATFORM_WIN32_NT) {
            static HMODULE kernel32 = NULL;
            if(!kernel32) kernel32 = GetModuleHandle("kernel32.dll");
            if(kernel32) {
                if(!myFindFirstFileExW) myFindFirstFileExW = (void *)GetProcAddress(kernel32, "FindFirstFileExW");
                if(!myFindNextFileW)    myFindNextFileW    = (void *)GetProcAddress(kernel32, "FindNextFileW");
            }
            winnt = 1;
        } else {
            winnt = 0;
        }
    }

    if(winnt && myFindFirstFileExW && myFindNextFileW) {
        wchar_t *filedirW = calloc(filedirsz + 1, sizeof(*filedirW));
        if(!filedirW) STD_ERR(QUICKBMS_ERROR_MEMORY);
        mywstrcpy(filedirW, native_utf8_to_unicode(filedir));
        ret = recursive_dirW(filedirW, filedirsz);
        FREE(filedirW);
        return ret;
    }

    plen = strlen(filedir);
    if((plen + 4) >= filedirsz) goto quit;
    strcpy(filedir + plen, "\\*.*");
    plen++;

    if(winnt) { // required to avoid problems with Vista and Windows7!
        hFind = FindFirstFileEx(filedir, FindExInfoStandard, &wfd, FindExSearchNameMatch, NULL, 0);
    } else {
        hFind = FindFirstFile(filedir, &wfd);
    }
    if(hFind == INVALID_HANDLE_VALUE) goto quit;
    do {
        if(!strcmp(wfd.cFileName, ".") || !strcmp(wfd.cFileName, "..")) continue;

        namelen = strlen(wfd.cFileName);
        if((plen + namelen) >= filedirsz) goto quit;
        //strcpy(filedir + plen, wfd.cFileName);
        memcpy(filedir + plen, wfd.cFileName, namelen);
        filedir[plen + namelen] = 0;

        if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            recursive_dir(filedir, filedirsz);  // NO goto quit
        } else {
            add_files(filedir + recursive_dir_skip_path, ((u64)wfd.nFileSizeHigh << (u64)(sizeof(wfd.nFileSizeLow) * 8)) | (u64)wfd.nFileSizeLow, NULL);
        }
    } while(FindNextFile(hFind, &wfd));
    ret = 0;

quit:
    if(hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
#else
    struct  stat    xstat;
    struct  dirent  **namelist;
    int     n,
            i;

    n = scandir(filedir, &namelist, NULL, NULL);
    if(n < 0) {
        if(stat(filedir, &xstat) < 0) {
            fprintf(stderr, "**** %s", filedir);
            STD_ERR(QUICKBMS_ERROR_FOLDER);
        }
        add_files(filedir + recursive_dir_skip_path, xstat.st_size, NULL);
        return 0;
    }

    plen = strlen(filedir);
    if((plen + 1) >= filedirsz) goto quit;
    strcpy(filedir + plen, "/");
    plen++;

    for(i = 0; i < n; i++) {
        if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

        namelen = strlen(namelist[i]->d_name);
        if((plen + namelen) >= filedirsz) goto quit;
        //strcpy(filedir + plen, namelist[i]->d_name);
        memcpy(filedir + plen, namelist[i]->d_name, namelen);
        filedir[plen + namelen] = 0;

        if(stat(filedir, &xstat) < 0) {
            fprintf(stderr, "**** %s", filedir);
            STD_ERR(QUICKBMS_ERROR_FOLDER);
        }
        if(S_ISDIR(xstat.st_mode)) {
            recursive_dir(filedir, filedirsz);  // NO goto quit
        } else {
            add_files(filedir + recursive_dir_skip_path, xstat.st_size, NULL);
        }
        FREE(namelist[i]);
    }
    ret = 0;

quit:
    for(; i < n; i++) FREE(namelist[i]);
    FREE(namelist);
#endif
    filedir[plen - 1] = 0;
    return ret;
}



u_int filesize(FILE *fd) {
    struct stat xstat;
    if(!fd) return 0;
    xstat.st_size = 0;
    fstat(fileno(fd), &xstat);
    return(xstat.st_size);
}



typedef struct {
    u8  *datafile;
    u8  *p;
    u8  *limit;
} datafile_t;

int datafile_init(datafile_t *df, u8 *data, int size) {
    if(size < 0) return -1;
    if(!data && (size > 0)) return -1;
    memset(df, 0, sizeof(datafile_t));
    df->datafile = data;
    df->p        = data;
    df->limit    = data + size;
    return 0;
}

int incremental_fread_BOM(FILE *fd, datafile_t *df) {
    int     utf16   = 0;
    if(fd && (fd == stdin)) return utf16;
    if(
        (fd && !ftell(fd))
     || (df && (df->p == df->datafile))
    ) {
        u8  t1,t2,t3;
        if(fd) t1 = fgetc(fd);
        else   t1 = *(df->p)++;
        if(fd) t2 = fgetc(fd);
        else   t2 = *(df->p)++;
        if((t1 == 0xef) && (t2 == 0xbb)) {          // UTF-8 BOM
            if(fd) t3 = fgetc(fd);
            else   t3 = *(df->p)++;
            if(t3 == 0xbf) utf16 = 0;
            else           utf16 = 0;   // ignore the third value and do not reset
        } else if((t1 == 0xff) && (t2 == 0xfe)) {   // UTF-16 BOM LE
            utf16 = -1;
        } else if((t1 == 0xfe) && (t2 == 0xff)) {   // UTF-16 BOM BE
            utf16 = 1;
        } else {        // reset
            if(fd) fseek(fd, 0, SEEK_SET);
            else   df->p = df->datafile;
        }
    }
    return utf16;
}

int incremental_fread_getc(FILE *fd, datafile_t *df, u8 *data, int utf16) {
    int     t   = 0;

    if(fd) {
        if(utf16 < 0) {
            t = fgetc(fd)        | (fgetc(fd) << 8);
        } else if(utf16 > 0) {
            t = (fgetc(fd) << 8) | fgetc(fd);
        } else {
            t = fgetc(fd);
        }
    } else if(data) {
        if(utf16 < 0) {
            t = data[0]        | (data[1] << 8);
        } else if(utf16 > 0) {
            t = (data[0] << 8) | data[1];
        } else {
            t = data[0];
        }
    } else if(df->p < df->limit) {
        if(utf16 < 0) {
            t = df->p[0]        | (df->p[1] << 8);
        } else if(utf16 > 0) {
            t = (df->p[0] << 8) | df->p[1];
        } else {
            t = df->p[0];
        }
    }
    return t;
}

// remember that the output is allocated and not static, free it when you no longer need it
// eol: 0=just_fread, 1=line, -1=line+utf
u8 *incremental_fread(FILE *fd, int *ret_size, int eol, datafile_t *df, int use_static, int utf16) {
    static const int    STDINSZ = 4096;
    static int  _buffsz = 0;
    static u8   *_buff  = NULL;
    int     t,
            len,
            size,
            buffsz  = 0;
    u8      bom[3]  = "",
            *buff   = NULL;

    if(use_static) {
        buff    = _buff;
        buffsz  = _buffsz;
    }

    if(ret_size) *ret_size = 0;

    if(eol < 0) {
        // this is not a real method, it's not used anywhere and may be removed in the future
        // currently it checks the utf16 from the beginning of the file everytime it's invoked (slow)
        // (the previous code was a sort of "go backward N lines" that didn't work)
        if((fd && (fd != stdin)) || df) {
            u_int   offset;
            if(fd)  offset = ftell(fd);
            else    offset = df->p - df->datafile;
            if(fd)  fseek(fd, 0, SEEK_SET);
            else    df->p = df->datafile;
            utf16 = incremental_fread_BOM(fd, df);
            if(fd)  fseek(fd, offset, SEEK_SET);
            else    df->p = df->datafile + offset;
        }
        eol = 1;
    }

    size = 0;
    for(;;) {
        for(;;) {
            if(size >= buffsz) {
                buffsz = size + STDINSZ;
                buff = (u8 *)realloc(buff, buffsz + 1);
                if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
            }
            if(eol) len = utf16 ? 2 : 1;
            else    len = buffsz - size;
            if(fd) {
                len = fread(buff + size, 1, len, fd);
            } else if(df) {
                t = df->limit - df->p;
                if(t < len) len = t;
                memcpy(buff + size, df->p, len);
                df->p += len;
            }
            if(len <= 0) {
                if(!size) {
                    if(!use_static) FREE(buff)
                    buff = NULL;
                    goto quit;
                }
                break;
            }
            size += len;
            if(eol) {
                t = incremental_fread_getc(NULL, NULL, buff + size - len, utf16);
                //if((t == '\r') || (t == '\n')) {    // old
                if(t == '\r') {
                    if(len == (utf16 ? 2 : 1)) {
                        // '\n'?
                        if(fd) {
                            t = incremental_fread_getc(fd, df, NULL, utf16);
                            if(t != '\n') {
                                ungetc(t, fd);
                                if(utf16) ungetc(t, fd);
                            }
                        } else if(df && (df->p < df->limit)) {
                            t = incremental_fread_getc(fd, df, NULL, utf16);
                            if(t == '\n') df->p += (utf16 ? 2 : 1);
                        }
                    }
                    size -= len;
                    break;
                }
                if(t == '\n') {
                    size -= len;
                    break;
                }
            }
        }

        // leave blank lines if eol is 2 or more
        if(size) break;
        if(eol <= 1) break;
    }
    if(buff) buff[size] = 0;
quit:
    if(utf16) {
        int endian_bck = g_endian;
        g_endian = (utf16 < 0) ? MYLITTLE_ENDIAN : MYBIG_ENDIAN;
        u8  *o8 = set_unicode_to_utf8(buff, size, &size);   // static
        g_endian = endian_bck;
        size++;   // NUL delimiter
        if(size > buffsz) {
            buffsz = size + STDINSZ;
            buff = (u8 *)realloc(buff, buffsz + 1);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        memcpy(buff, o8, size);
    }

    if(ret_size) *ret_size = size;
    if(use_static) {
        if(buff) _buff = buff;  // in case of errors
        _buffsz = buffsz;
    }
    return buff;
}



void pathslash_fix(u8 *fname, int cut_start) {
    u8      *p;
    if(fname) {
        if(cut_start) {
            for(p = fname; *p; p++) {
                if(!strchr(PATH_DELIMITERS, *p)) break;
            }
            if(p != fname) mymemmove(fname, p, -1);
        }
        for(p = fname; *p; p++) {
            if(strchr(PATH_DELIMITERS, *p)) *p = PATHSLASH;
        }
    }
}



u8 *xgetcwd(u8 *buff, int buffsz) {
    u8      *ret    = NULL;
#ifdef WIN32
    wchar_t *buffw  = NULL;
    wchar_t *retw   = NULL;
    if(buff) {
        buffw = calloc(buffsz, sizeof(wchar_t));
        if(!buffw) STD_ERR(QUICKBMS_ERROR_MEMORY);
        if(buffsz >= 1) buff[0] = 0;    // just in case
    }
    retw = _wgetcwd(buffw, buffsz);
    if(retw) {
        ret = native_unicode_to_utf8(retw);
        if(buff) mystrcpy(buff, ret, buffsz);
    }
    FREE(buffw)
#endif
    if(!ret) ret = getcwd(buff, buffsz);
    return ret;
}



#ifdef WIN32
u8 *long_name_support(u8 *fname) {
    // this code is used ONLY in case of errors (fd == NULL)
    // so if fname is \\server\path\file.txt then this code should not be called
    #define long_name_support_tmp_fname_size    32768
    static u8   tmp_fname[4 + long_name_support_tmp_fname_size + 16];   // 32768 is the max
    int     len;
    u8      *p;
    if(!fname) fname = "";
    if(!strncmp(fname, "\\\\?\\", 4)) return fname;  // already long name
    // http://msdn.microsoft.com/en-us/library/aa365247%28v=vs.85%29.aspx
    tmp_fname[0] = 0;
    if((fname[0] == '\\') && (fname[1] == '\\')) {
        // \\server\path\file.txt
        mystrcpy(tmp_fname, fname, long_name_support_tmp_fname_size);
    } else {
        if(fname[0] && (fname[1] == ':')) {
            // absolute path, nothing to do
        } else {
            xgetcwd(tmp_fname, long_name_support_tmp_fname_size);
            if(strchr(PATH_DELIMITERS, fname[0])) { // \path
                p = mystrchrs(tmp_fname, PATH_DELIMITERS);
                if(p) *p = 0;
            }
        }
        len = strlen(tmp_fname);
        if(!strchr(PATH_DELIMITERS, tmp_fname[len - 1])) {
            tmp_fname[len++] = PATHSLASH;
        }
        for(p = fname; *p && strchr(PATH_DELIMITERS, *p); p++);
        mystrcpy(tmp_fname + len, p, long_name_support_tmp_fname_size - len);
    }
    if(strncmp(tmp_fname, "\\\\?\\", 4)) {
        mymemmove(tmp_fname + 4, tmp_fname, -1);
        memcpy(tmp_fname, "\\\\?\\", 4);
    }
    u8  *start = tmp_fname + 4;
    u8  *last = NULL;  // this solution is necessary because Windows doesn't like more than a path delimiter like "folder//file"
    for(p = start; *p; p++) {
        if(strchr(PATH_DELIMITERS, *p)) {
            if((last + 1) == p) {
                mymemmove(p, p + 1, -1);
                p--;    // due to next p++
            } else {
                *p = PATHSLASH;
                last = p;
            }
        }
    }
    // remove the "." like c:\. -> c:\ or there are some rare problems with wchar_t functions
    len = strlen(start);
    if((len >= 2) && (start[len - 1] == '.') && strchr(PATH_DELIMITERS, start[len - 2])) {
        start[len - 1] = 0; // remove only the dot
    }
    return tmp_fname;
}
#endif



/*
root directories aren't handled like normal directories:

_wstat and stat:
\\?\c:\.    fail
\\?\c:\     fail
\\?\c:      fail
c:\.        ok
c:\         ok
c:          fail

_wchdir and chdir:
\\?\c:\.    fail
\\?\c:\     ok
\\?\c:      ok
c:\.        ok
c:\         ok
c:          ok
*/



int check_is_dir(u8 *fname) {
    struct stat xstat;

    if(!fname || !fname[0]) return 1;
    if(!strcmp(fname, ".")) return 1;
    if(!strcmp(fname, "..")) return 1;

    // stat(), then _wstat(), better
    if(stat(fname, &xstat) < 0)
#ifdef WIN32
#if defined(_LARGE_FILES) && !defined(NOLFS)
    #define _wstat  _wstati64
#endif
    if(_wstat(native_utf8_to_unicode(fname), &xstat) < 0)                       // try without long name support
    if(_wstat(native_utf8_to_unicode(long_name_support(fname)), &xstat) < 0)    // try with long name support (fails with \\?\c:\)
#endif
        return 0;
    if(!S_ISDIR(xstat.st_mode)) return 0;
    return 1;
}



u8 *get_main_path(u8 *fname, u8 *argv0, u8 *output) {
    static u8   fullname[PATHSZ + 1];
    DWORD   r;
    u8      *p;

    if(!output) output = fullname;
#ifdef WIN32
    r = GetModuleFileName(NULL, output, PATHSZ);
    if(!r || (r >= PATHSZ))
#endif
    sprintf(output, "%.*s", PATHSZ, argv0);

    if(check_is_dir(output)) return(output);
    p = mystrrchrs(output, PATH_DELIMITERS);
    if(fname) {
        if(!p) p = output - 1;
        sprintf(p + 1, "%.*s", (i32)(PATHSZ - (p - output)), fname);
    } else {
        if(p) *p = 0;
    }
    return(output);
}



int copycut_folder(u8 *input, u8 *output) {
    u8      *p;

    if(!output) return -1;
    if(input) mystrcpy(output, input, PATHSZ);
    if(check_is_dir(output)) return 0;
    p = mystrrchrs(output, PATH_DELIMITERS);
    if(!p) {
        if(input) output[0] = 0;
    } else {
        *p = 0;
    }
    if(check_is_dir(output)) return 0;
    return -1;
}



int xchdir(u8 *folder) {
    int     ret = -1;
    if(!folder || !folder[0]) return ret;
#ifdef WIN32
    u8      tmp[16];
    if(strlen(folder) < (sizeof(tmp) - 2)) {
        u8 *p = strchr(folder, ':');
        if(p && !p[1]) {    // like "C:"
            sprintf(tmp, "%s%c", folder, PATHSLASH);
            folder = tmp;
        }
    }
    // try to keep long name support disabled because it creates a lot of problems and all the paths in getcwd will have the \\?\ prefix,
    // till now long_name_support in chdir has never been adopted and nobody reported any problem
    ret = _wchdir(native_utf8_to_unicode(folder));
    if(ret < 0) ret = _wchdir(native_utf8_to_unicode(long_name_support(folder)));   // first time used in quickbms
#endif
    if(ret < 0) ret = chdir(folder);
    return ret;
}



int make_dir(u8 *folder) {
    int     ret;
#ifdef WIN32
    errno = 0;  // useless
    ret = _wmkdir(native_utf8_to_unicode(long_name_support(folder)));                   // unicode + long name support
    if((ret < 0) && (errno != EEXIST)) ret = _wmkdir(native_utf8_to_unicode(folder));   // unicode
    if((ret < 0) && (errno != EEXIST)) ret = mkdir(folder);                             // automatic fall-back
    if((ret < 0) && (errno != EEXIST) && (strlen(folder) > MAX_PATH)) {
        ret = mkdir(long_name_support(folder));                                         // last chance with long name support
    }
#else
    ret = mkdir(folder, 0755);
#endif
    return ret;
}



int g_force_readwrite_mode  = 0;    // easy solution



// do not use fopen in the other code, use ever xfopen
u8 *get_fullpath_from_name(u8 *fname, int folder_only);
FILE *xfopen(u8 *fname, u8 *mode) {
    FILE    *fd = NULL;

    if(g_verbose > 0) {
        printf("- xfopen  %s: %s\n", mode ? mode : (u8*)"", fname ? fname : (u8*)"");
        printf("- xfopen2 %s\n", get_fullpath_from_name(fname, 0));
    }

    if(!fname || !fname[0]) return NULL;

#ifdef WIN32
    // ccs supports only UTF-8, any other codepage gives error
    wchar_t wmode[strlen(mode) + 20];
    // do NOT enable "ccs=UTF-8" or everything will be screwed on Wine or some languages
    // unfortunately I can't remember why it was so important to use ccs and in what situations, but there are no other solutions
    #ifdef QUICKBMS_SWPRINTF
    myswprintf(wmode, sizeof(wmode) / sizeof(wchar_t), L"%s" /*", ccs=UTF-8"*/, native_utf8_to_unicode(mode));
    #else
    mywstrcpy(wmode, native_utf8_to_unicode(mode));
    #endif

    // I have a report about this issue but can't verify it, very rare and affecting only some OS and/or filesystems I guess
    if((strlen(fname) > 2) && (strstr(fname + 2, "\\\\") || strstr(fname + 2, "//"))) {

        // this is mandatory because we CANNOT modify the input otherwise log "data\\file" fails in append mode
        u8      *tmp_fname = alloca(strlen(fname) + 1); // alloca() is on the stack, it's ok since this is rare
        if(!tmp_fname) STD_ERR(QUICKBMS_ERROR_MEMORY);
        strcpy(tmp_fname, fname);
        fname = tmp_fname;

        int     i, j;
        for(i = j = 0; fname[i]; i++, j++) {
            while(strchr(PATH_DELIMITERS, fname[i]) && strchr(PATH_DELIMITERS, fname[i + 1])) i++;
            fname[j] = fname[i];
        }
        fname[j] = 0;
    }

    if(g_force_readwrite_mode) _wchmod(native_utf8_to_unicode(long_name_support(fname)), _S_IREAD | _S_IWRITE);
    fd = _wfopen(native_utf8_to_unicode(long_name_support(fname)), wmode);
    if(!fd) {
        if(g_force_readwrite_mode) _wchmod(native_utf8_to_unicode(fname), _S_IREAD | _S_IWRITE);
        fd = _wfopen(native_utf8_to_unicode(fname), wmode); // try without long name support
    }
#endif
    if(!fd) {
#ifdef WIN32    // avoid ACL issues
        if(g_force_readwrite_mode) chmod(fname, _S_IREAD | _S_IWRITE);
#endif
        fd = fopen(fname, mode);    // fallback for Win98
    }
    if(fd) {
        if((fd != stdin) && (fd != stdout) && (fd != stderr)) {
            setvbuf(fd, NULL, _IOFBF, 64 * 1024);   // default is good too, do NOT use bigger values!
        }
    }
    if(g_force_readwrite_mode) g_force_readwrite_mode = 0;
    return fd;
}



u8 *fdload(u8 *fname, int *fsize) {
    FILE    *fd;
    int     size;
    u8      *buff;

    if(!fname) return NULL;
    fprintf(stderr, "  %s\n", fname);
    if(!strcmp(fname, "-")) {
        return(incremental_fread(stdin, fsize, 0, NULL, 0, 0));
    }
    fd = xfopen(fname, "rb");
    if(!fd) return NULL;
    size = filesize(fd);
    MAX_ALLOC_CHECK(size);
    buff = malloc(size + 1);
    if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
    size = fread(buff, 1, size, fd);
    buff[size] = 0;
    FCLOSE(fd);
    if(fsize) *fsize = size;
    return buff;
}



u8 *string_to_C(u8 *data, int size, int *ret_len, int binary) {
    int     i;
    int     buffsz  = 0;   // NOT static!!!
    static u8   *buff   = NULL; // static to save memory

    if(data) {
        if(size < 0) size = strlen(data);

        if(binary) {

            buff = realloc(buff, (size * 4) + 1);   // pre-allocate
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);

            for(i = 0; i < size; i++) {
                buff[buffsz++] = '\\';
                buff[buffsz++] = 'x';
                buff[buffsz++] = hex_codes[data[i] >> 4];
                buff[buffsz++] = hex_codes[data[i] & 15];
            }

        } else {

            buff = realloc(buff, (size * 2) + 1);   // pre-allocate
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);

            for(i = 0; i < size; i++) {
                // if(data[i] < ' ') ???
                if(!data[i] || strchr("\n\r\\", data[i])) {
                    buff[buffsz++] = '\\';
                    switch(data[i]) {
                        case '\0': buff[buffsz++] = '0';     break;
                        case '\n': buff[buffsz++] = 'n';     break;
                        case '\r': buff[buffsz++] = 'r';     break;
                        default:   buff[buffsz++] = data[i]; break;
                    }
                } else {
                    buff[buffsz++] = data[i];
                }
            }
        }

    } else {
        buff = realloc(buff, buffsz + 1);
        if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }
    if(ret_len) *ret_len = buffsz;
    buff[buffsz++] = 0;
    return buff;
}



int cstring(u8 *input, u8 *output, int maxchars, int *inlen, u32 *mask) {
    i32     n,
            len;
    u8      *p,
            *o;

    if(mask) *mask = 0;
    if(!input || !output) {
        if(inlen) *inlen = 0;
        return 0;
    }

    p = input;
    o = output;
    while(*p) {
        if(maxchars >= 0) {
            if((o - output) >= maxchars) break;
        }
        if(*p == '\\') {
            n = 0;  // useless
            p++;
            switch(*p) {
                case 0:  goto quit; break;
                //case '0':  n = '\0'; break;
                case 'a':  n = '\a'; break;
                case 'b':  n = '\b'; break;
                case 'e':  n = '\e'; break;
                case 'f':  n = '\f'; break;
                case 'n':  n = '\n'; break;
                case 'r':  n = '\r'; break;
                case 't':  n = '\t'; break;
                case 'v':  n = '\v'; break;
                case '\"': n = '\"'; break;
                case '\'': n = '\''; break;
                case '\\': n = '\\'; break;
                case '?':  n = '\?'; break;
                case '.':  n = '.';  break;
                case ' ':  n = ' ';  break;
                case '/':  n = '/';  break;
                case 'u': {
                    if(sscanf(p + 1, "%04x%n", &n, &len) != 1) goto quit;
                    if(len > 4) len = 4;
                    p += len;
                    *o++ = n;   n = (u32)n >> 8;    // this is NOT a real unicode->utf8 conversion! maybe in the next versions
                    break;
                }
                case 'U': {
                    if(sscanf(p + 1, "%08x%n", &n, &len) != 1) goto quit;
                    if(len > 8) len = 8;
                    p += len;
                    *o++ = n;   n = (u32)n >> 8;    // this is NOT a real unicode->utf8 conversion! maybe in the next versions
                    *o++ = n;   n = (u32)n >> 8;
                    *o++ = n;   n = (u32)n >> 8;
                    break;
                }
                case 'x': {
                    //n = readbase(p + 1, 16, &len);
                    //if(len <= 0) goto quit;
                    if(sscanf(p + 1, "%02x%n", &n, &len) != 1) {
                        if(mask && ((p[1] == '?') || (p[1] == '*') || (p[2] == '?') || (p[2] == '*'))) {
                            *mask = *mask | (1 << (o - output));
                            n = 0;
                            len = 1;
                            if((p[2] == '?') || (p[2] == '*')) len = 2;
                        } else {
                            goto quit;
                        }
                    }
                    if(len > 2) len = 2;
                    p += len;
                    break;
                }

                // external, this is probably useless or wrong because our string is already parsed but programmers may be used to it
                case '%':  n = *p;   break; // some languages???
                case '[':  n = *p;   break; // ANSI / some languages???
                case ']':  n = *p;   break; // ANSI / some languages???
                case '{':  n = *p;   break; // some languages???
                case '}':  n = *p;   break; // some languages???
                case '*':  n = *p;   break; // shell?
                case '&':  n = *p;   break; // Windows?
                case '|':  n = *p;   break; // Windows?
                case '<':  n = *p;   break; // Windows?
                case '>':  n = *p;   break; // Windows?
                case '(':  n = *p;   break; // Windows?
                case ')':  n = *p;   break; // Windows?
                case '`':  n = *p;   break; // PowerShell?
                // external

                default: {
                    //n = readbase(p, 8, &len);
                    //if(len <= 0) goto quit;
                    if(sscanf(p, "%3o%n", &n, &len) != 1) goto quit;
                    if(len > 3) len = 3;
                    p += (len - 1); // work-around for the subsequent p++;
                    break;
                }
            }
            *o++ = n;
        } else {
            *o++ = *p;
        }
        p++;
    }
    *o = 0;
    len = o - output;
    if(inlen) *inlen = p - input;
    return len;
quit:
    fprintf(stderr, "Error: cstring() failure, your input string has some wrong escape sequences or it's not a valid escaped string\n");
    myexit(QUICKBMS_ERROR_BMS);
    return -1;
}



// alternative to sscanf so it's possible to use also commas and hex numbers
// do NOT reset the parameters because they could have default values different than 0!
#define MACRO_get_parameter_numbers(TYPE) \
    va_list ap; \
    TYPE    i, \
            *par; \
    \
    if(!s || !s[0]) return 0; \
    va_start(ap, s); \
    for(i = 0;; i++) { \
        par = va_arg(ap, TYPE *); \
        if(!par) break; \
        \
        while(*s && !myisalnum(*s)) s++; \
        if(!*s) break; \
        *par = myatoi(s); \
        while(*s && myisalnum(*s)) s++; \
        if(!*s) break; \
    } \
    va_end(ap); \
    return i;

i32 get_parameter_numbers_int(u8 *s, ...) {
    MACRO_get_parameter_numbers(int)
}

i32 get_parameter_numbers_i32(u8 *s, ...) {   // the compression code has int->32, quickbms_4gb_files has int->64 in all the other places
    MACRO_get_parameter_numbers(i32)
}



int check_wildcard(u8 *fname, u8 *wildcard) {
    u8      *f      = fname,
            *w      = wildcard,
            *last_w = NULL,
            *last_f = NULL;

    if(!fname) return -1;
    if(!wildcard) return -1;
    while(*f || *w) {
        if(!*w && !last_w) return -1;
        if(*w == '?') {
            if(!*f) break;
            w++;
            f++;
        } else if(*w == '*') {
            w++;
            last_w = w;
            last_f = f;
        } else {
            if(!*f) break;
            if(((*f == '\\') || (*f == '/')) && ((*w == '\\') || (*w == '/'))) {
                f++;
                w++;
            } else if(tolower(*f) != tolower(*w)) {
                if(!last_w) return -1;
                w = last_w;
                if(last_f) f = last_f;
                f++;
                if(last_f) last_f = f;
            } else {
                f++;
                w++;
            }
        }
    }
    if(*f || *w) return -1;
    return 0;
}



int check_wildcard_filenum(u8 *cmp) {
    int     t;
    t = myatoi(cmp);
    if(g_reimport) {
        if(g_reimported_files == t) return 0;
    } else {
        if(g_extracted_files2  == t) return 0;
    }
    return -1;
}



int check_wildcards(u8 *fname, u8 **list) {
    int     i,
            t,
            fok     = 0,
            fnot    = 0,
            ret     = -1;
    u8      *cmp;

    // no wildcards to check = ok
    if(!list) return 0;
    for(i = 0; (cmp = list[i]); i++) {
        if(cmp[0] == '!') {
            cmp++;
            fnot++;
            if((list == g_filter_files) && (cmp[0] == '#')) {
                cmp++;
                if(!check_wildcard_filenum(cmp)) return -1;
            } else {
                if(!check_wildcard(fname,  cmp)) return -1;
            }
        } else {
            fok++;
            if((list == g_filter_files) && (cmp[0] == '#')) {
                cmp++;
                if(!check_wildcard_filenum(cmp)) ret = 0;
            } else {
                if(!check_wildcard(fname,  cmp)) ret = 0;
            }
        }
    }
    if(!fok) return 0;     // -f "!*.mp3" with txt files
    return ret;
}



int file_exists(u8 *fname) {
    FILE    *fd;

    if(!fname) fname = "";

    // stdin/stdout ???
    if(!strcmp(fname, "-")) return 1;

    // needed for symlinks to folders
    if(check_is_dir(fname)) return 0;

    fd = xfopen(fname, "rb");
    if(!fd) return 0;
    FCLOSE(fd);
    return 1;
}



// mdir creates the folder
// cdir goes in the folder
// is_path allows to create folders with the name of the archive
u8 *create_dir(u8 *fname, int mdir, int cdir, int is_path, int filter_bad) {
    static u8   root_path[1+1] = { PATHSLASH, 0x00 };
    int     i;
    u8      *tmp    = NULL,
            *p,
            *l;

    if(!fname) return NULL;

    if(g_quickiso && g_quickiso->fd) return fname;
    if(g_quickzip && g_quickzip->fd) return fname;

    if(filter_bad) {
        p = strchr(fname, ':'); // unused
        if(p) {
            *p = '_';
            fname = p + 1;
        }
        for(p = fname; *p && strchr(PATH_DELIMITERS ". \t:", *p); p++) *p = '_';
        fname = p;
    }

    // do not use "continue"
    for(p = fname;; p = l + 1) {
        for(l = p; *l && (*l != '\\') && (*l != '/'); l++);
        if(!*l) {
            if(!is_path) break;
            l = NULL;
        }
        if(l) *l = 0;

        if(!p[0]) {
            if(p != fname) goto continue_loop;
            p = root_path;
        }

        if(filter_bad) {
            if(!strcmp(p, "..")) {
                p[0] = '_';
                p[1] = '_';
            }
        }

        if(cdir) {
            if(p == root_path) {
                //if(mdir) make_dir(p);
                if(xchdir(p) < 0) goto quit_error;

            } else if(p[0] && (p[strlen(p) - 1] == ':')) {  // we need c:\, not c:
                //if(mdir) make_dir(p);
                if(xchdir(p) < 0) goto quit_error;   // partition
                xchdir(root_path);                   // root

            } else {
                if(file_exists(p)) {
                    tmp = malloc(strlen(p) + 32 /*"extract" + num*/ + 1);
                    if(!tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);
                    sprintf(tmp, "%s_extract", p);
                    for(i = 0; file_exists(tmp); i++) {
                        sprintf(tmp, "%s_extract%d", p, (i32)i);
                    }
                    p = tmp;
                }
                if(mdir) make_dir(p);
                if(xchdir(p) < 0) goto quit_error;
                if(p == tmp) {
                    FREE(tmp);
                    p = NULL;
                }
            }
        } else {
            if(mdir) make_dir(fname);
        }

        continue_loop:
        if(!l) break;
        *l = PATHSLASH;
    }
    return(fname);

quit_error:
    fprintf(stderr, "\nError: impossible to create/enter in folder %s\n", p);
    STD_ERR(QUICKBMS_ERROR_FOLDER);
    return NULL;
}



int get_yesno(u8 *data) {
    u8      tmp[16];

    if(!data) {
        if(g_yes) return('y');  // because data means that we have probably other choices (and a previous fgetz)

        if(fgetz(tmp, sizeof(tmp), stdin, NULL) < 0) return 0;
        data = tmp;
    }
    return(tolower(data[0]));
}



int check_overwrite(u8 *fname, int check_if_present_only) {
    int     c;

    /*
    0   overwrite
    -1  do NOT overwrite / skip
    -2  rename or skip
    */

    if(g_force_overwrite > 0) return 0;
    if(!fname) return 0;
    if(!file_exists(fname)) return 0;
    if(check_if_present_only) return -1;
    if(g_force_rename) return -2;
    if(g_force_overwrite < 0) return -1;
    printf(
        "\n"
        "- The following output file already exists:\n"
        "  %s\n"
        "  Do you want to overwrite it?\n"
        "    y = overwrite (you can use also the 'o' key)\n"
        "    n = skip (default, just press ENTER)\n"
        "    a = overwrite all the files without asking\n"
        "    r = automatically rename the files with the same name\n"
        "    s = skip all the existent files without asking\n"
        "  \n",
        fname);
    if(g_append_mode) printf("\n"
        "  (remember that you are in append mode so be sure that the output folder was\n"
        "  empty otherwise the new data will be appended to the existent files!) ");
    c = get_yesno(NULL);
    if(c == 'y') return 0;
    if(c == 'o') return 0; // Overwrite
    if(c == 'a') {
        g_force_overwrite = 1;
        return 0;
    }
    if(c == 'r') {
        g_force_rename = 1;
        return -2;
    }
    if((c == 's') || (c == '0')) {  // O and 0 (zero) are too similar
        g_force_overwrite = -1;
        return -1;
    }
    return -1;
}



static int myalloc_return_NULL_on_error    = 0;



u8 *myalloc(u8 **data, QUICKBMS_int wantsize, QUICKBMS_int *currsize) {
    QUICKBMS_int    ows;    // original wantsize
    u8      *old_data;      // allocate it at any cost

    if(wantsize < 0) {
        fprintf(stderr, "\nError: the requested amount of bytes to allocate is negative (0x%"PRIx")\n", wantsize);
        myexit(QUICKBMS_ERROR_MEMORY);
    }
    //removed in quickbms 0.11 (problems with MEMORY_FILE and lack of real advantages) //if(!wantsize) return NULL;
    if(!data) return NULL;

    ows = wantsize;
    wantsize += MYALLOC_ZEROES; // needed by XMemDecompress

    // quick secure way that uses the advantages of xdbg_alloc
    if(XDBG_ALLOC_ACTIVE) {

        //if((wantsize < 0) || (wantsize < ows)) {    // due to integer rounding
        if(wantsize < 0) {
            fprintf(stderr, "\nError: the rounded amount of bytes to allocate is negative or too big (0x%"PRIx")\n", wantsize);
            myexit(QUICKBMS_ERROR_MEMORY);
        }

    } else {    // -9 option

        wantsize = (wantsize + 4095) & (~4095);     // not bad as fault-safe and fast alloc solution: padding (4096 is usually the default size of a memory page)
        if((wantsize < 0) || (wantsize < ows)) {    // due to integer rounding
            fprintf(stderr, "\nError: the rounded amount of bytes to allocate is negative or too big (0x%"PRIx")\n", wantsize);
            myexit(QUICKBMS_ERROR_MEMORY);
            //wantsize = ows;   // remember memset MYALLOC_ZEROES
        }
        if(currsize && (wantsize <= *currsize)) {   // wantsize is rounded
            if(*currsize > 0) goto quit;
        }
    }

    if(currsize && (ows <= *currsize)) {
        if(!*data) {
            // we need to allocate something, even if wantsize is zero, so continue
        } else {
            // too expensive: memset((*data) + ows, 0, *currsize - ows);
            if(*currsize > 0) goto quit; //return(*data);
        }
    }

    old_data = *data;
    int _xdbg_return_NULL_on_error = xdbg_return_NULL_on_error;
    xdbg_return_NULL_on_error = 1;
    *data = realloc(*data, wantsize);
    xdbg_return_NULL_on_error = _xdbg_return_NULL_on_error;
    if(!*data) {    // STD_ERR(QUICKBMS_ERROR_MEMORY);
        // this method works only if the caller uses myalloc as malloc()
        // it will corrupt the data if it's used as realloc with existent data to be keep in the buffer
        FREE(old_data); // because realloc requires more memory: old+new
        xdbg_return_NULL_on_error = myalloc_return_NULL_on_error;
        *data = calloc(wantsize, 1);
        xdbg_return_NULL_on_error = _xdbg_return_NULL_on_error;
        if(!*data) {
            if(myalloc_return_NULL_on_error) {
                *data     = NULL;
                *currsize = 0;
                return *data;
            }
            fprintf(stderr, "- try allocating %"PRIu" bytes\n", wantsize);
            STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
    }

    if(currsize) *currsize = ows;
    if(*data) {
        memset((*data) + ows, 0, wantsize - ows); // ows is the original wantsize, useful in some cases like XMemDecompress
    }
quit:
    return(*data);
}



u8 *myalloc_ret(u8 **data, QUICKBMS_int wantsize, QUICKBMS_int *currsize) {
    u8  *ret;
    myalloc_return_NULL_on_error = 1;
    ret = myalloc(data, wantsize, currsize);
    myalloc_return_NULL_on_error = 0;
    return ret;
}



void alloc_err(const char *fname, i32 line, const char *func) {
    fprintf(stderr, "\n- error in %s line %d: %s()\n", fname, (i32)line, func);
    fprintf(stderr, "Error: tentative of allocating -1 bytes\n");
    myexit(QUICKBMS_ERROR_MEMORY);
}



void std_err(const char *fname, i32 line, const char *func, signed char error) {    // char avoids problems with int on 64bit
    fprintf(stderr, "\n- error in %s line %d: %s()\n", fname, (i32)line, func);
    perror("Error");
    if(error < 0) error = QUICKBMS_ERROR_UNKNOWN;
    myexit(error);
}



void winerr(DWORD error, char *msg) {
#ifdef WIN32
    char    *message = NULL;

    if(!error) error = GetLastError();
    if(!msg) msg = "";

    if(error) {
        FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          error,
          0,
          (char *)&message,
          0,
          NULL);
    }
    if(message) {
        fprintf(stderr, "\nError: %s - %s\n", msg, message);
        LocalFree(message);
    } else {
        fprintf(stderr, "\nError: %s - unknown Windows error\n", msg);
    }
    myexit(QUICKBMS_ERROR_UNKNOWN);
#else
    STD_ERR(QUICKBMS_ERROR_UNKNOWN);
#endif
}



QUICKBMS_int get_var_from_name(u8 *name, QUICKBMS_int namelen);
QUICKBMS_int get_var32(QUICKBMS_int idx);
u8 *get_var(QUICKBMS_int idx);
int add_var(int idx, u8 *name, u8 *val, int val32, int valsz);



void myexit_last_script_line(int cmd) {
    if(CMD.debug_line) {

        fprintf(stderr, "\nLast script line before the error or that produced the error:\n  %s\n", CMD.debug_line);

        if(CMD.type == CMD_Log) {

            fprintf(stderr,
                "\n"
                "- OFFSET       0x%"PRIx"\n"
                "- SIZE         0x%"PRIx"\n",
                VAR32(1), VAR32(2));

        } else if(CMD.type == CMD_CLog) {

            fprintf(stderr,
                "\n"
                "- OFFSET       0x%"PRIx"\n"
                "- ZSIZE        0x%"PRIx"\n"
                "- SIZE         0x%"PRIx"\n",
                VAR32(1), VAR32(2), VAR32(5));

        }
    }

    // -v or -V ?
    if(g_verbose > 0) {
        int     idx;
        for(idx = 0; g_variable[idx].name; idx++) {

            if(g_variable[idx].constant) continue;

            fprintf(stderr, "\n"
                "- Variable %-4d %s\n"
                "    value:      %s\n"
                "    value32:    0x%"PRIx"\n"
                //"    float64:    %f\n"
                "    size:       0x%"PRIx" / 0x%"PRIx"\n"
                //"    flags:      %d %d %d %d %p\n"
            ,
            (i32)idx,
            g_variable[idx].name,
            g_variable[idx].value ? g_variable[idx].value : (u8 *)"",
            g_variable[idx].value32,
            //g_variable[idx].float64,
            g_variable[idx].size, g_variable[idx].real_size
            //g_variable[idx].isnum, g_variable[idx].constant, g_variable[idx].binary, g_variable[idx].reserved, g_variable[idx].sub_var
            );
        }
    }
}



void bms_finish(void);
void myexit(int ret) {

    // yes, close it at the end of everything
    if(g_quickiso) {
        quickiso_close(g_quickiso);
        FREE(g_quickiso)
    }
    if(g_quickzip) {
        quickzip_close(g_quickzip);
        FREE(g_quickzip)
    }

    if(g_ipc_web_api) {
        if(ret >= 0) return;    // ignore exit
        // the main program invoked the exit
    }

    if(!ret && g_quick_gui_exit) {
        // nothing to do
    } else {
        if(ret == (u_int)-1LL) {
            fprintf(stderr, "\n"
                "Note that if both the scripts and your files are correct then it's possible\n"
                "that the script needs a newer version of QuickBMS, in which case download it:\n"
                "\n"
                "  http://quickbms.com\n"
                "\n");
        }

        if(ret != QUICKBMS_OK) {
            if(g_bms_line_number >= 0) {
                int cmd;
                for(cmd = 0; CMD.type != CMD_NONE; cmd++) {
                    if(CMD.bms_line_number == g_bms_line_number) {
                        myexit_last_script_line(cmd);
                        break;
                    }
                }
            }

            bms_finish();   // for coverage and so on
        }

#ifdef WIN32
        u8      ans[16];
        if(g_is_gui) {
            fgetz(ans, sizeof(ans), stdin,
                "\nPress ENTER or close the window to quit");
        }
#endif
    }

    exit(ret);  // main->return and exit() automatically call _fcloseall
}



// not necessary, myexit does the same
void myexit_cmd(int cmd, int ret) {
    if(cmd < 0) cmd = g_last_cmd;
    if(cmd >= 0) {
        myexit_last_script_line(cmd);
        g_bms_line_number = -1; // avoids double message
    }
    myexit(ret);
}



u_int rol(u_int n1, u_int n2, u_int bits) {
    if(!bits || (bits > INTSZ)) bits = INTSZ;
    u_int   bck     = n1;
    u_int   mask    = ((u_int)1 << bits) - 1;
    if(bits < INTSZ) n1 &= mask;
    u_int   ret = (n1 << n2) | (n1 >> (bits - n2));
    if(bits < INTSZ) {
        ret = (ret & mask) | (bck & ~mask);
    }
    return ret;
}



u_int ror(u_int n1, u_int n2, u_int bits) {
    if(!bits || (bits > INTSZ)) bits = INTSZ;
    u_int   bck     = n1;
    u_int   mask    = ((u_int)1 << bits) - 1;
    if(bits < INTSZ) n1 &= mask;
    u_int   ret = (n1 >> n2) | (n1 << (bits - n2));
    if(bits < INTSZ) {
        ret = (ret & mask) | (bck & ~mask);
    }
    return ret;
}



u_int bitswap(u_int n1, u_int n2) {
    u_int   out,
            rem = 0;

    if(n2 < INTSZ) {
        rem = n1 & (((int)-1) ^ (((int)1 << n2) - (int)1));
    }

    for(out = 0; n2; n2--) {
        out = (out << (int)1) | (n1 & (int)1);
        n1 >>= (u_int)1;
    }
    return(out | rem);
}



u_int byteswap(u_int n1, u_int n2) {
    u_int   out,
            rem = 0;

    if(n2 < (INTSZ >> 3)) {
        rem = n1 & (((int)-1) ^ (((int)1 << (n2 << (int)3)) - (int)1));
    }

    for(out = 0; n2; n2--) {
        out = (out << (int)8) | (n1 & (int)0xff);
        n1 >>= (u_int)8;
    }
    return(out | rem);
}



int power(int n1, int n2) {
    int     out = 1;

    for(;;) {
        if(n2 & 1) out *= n1;
        n2 >>= (int)1;
        if(!n2) break;
        n1 *= n1;
    }
    return(out);
}



int mysqrt(int num) {
    int    ret    = 0,
           ret_sq = 0,
           b;
    int    s;

    for(s = (INTSZ >> 1) - 1; s >= 0; s--) {
        b = ret_sq + ((int)1 << (s << (int)1)) + ((ret << s) << (int)1);
        if(b <= num) {
            ret_sq = b;
            ret += (int)1 << s;
        }
    }
    return ret;
}



int radix(int n1, int n2) {
    int     i,
            olds,    // due to the
            news;    // lack of bits

    if(!n1 || !n2) return 0;

    if(n2 == 2) return(mysqrt(n1)); // fast way

    for(i = olds = 1; ; i <<= 1) {   // faster???
        news = power(i, n2);
        if((news > n1) || (news < olds)) break;
        olds = news;
    }

    for(i >>= 1; ; i++) {
        news = power(i, n2);
        if((news > n1) || (news < olds)) break;
        olds = news;
    }
    return(i - 1);
}



u32 str2ip(u8 *data) {
    unsigned    a, b, c, d;

    if(!data[0]) return 0;
    sscanf(data, "%u.%u.%u.%u", &a, &b, &c, &d);
    return((a & 0xff) | ((b & 0xff) << 8) | ((c & 0xff) << 16) | ((d & 0xff) << 24));
}



u8 *ip2str(u32 ip) {
    static u8  data[16];

    sprintf(data, "%u.%u.%u.%u",
        (ip & 0xff), ((ip >> 8) & 0xff), ((ip >> 16) & 0xff), ((ip >> 24) & 0xff));
    return(data);
}



// I don't trust memmove, it gave me problems in the past
int mymemmove(void *dstx, void *srcx, int size) {
    int     i;

    u8  *dst = dstx;
    u8  *src = srcx;
    if(!dst || !src) return 0;
    if(dst == src) return 0;
    if(size < 0) size = strlen(src) + 1;
    if(dst < src) {
        for(i = 0; i < size; i++) {
            dst[i] = src[i];
        }
    } else {
        for(i = size - 1; i >= 0; i--) {
            dst[i] = src[i];
        }
    }
    return size;
}



u32 myrand(void) {
    static  u32 rnd = 0;
    // rng_crypt(&rnd, sizeof(rnd));    // real randomization
    if(!rnd) rnd = time(NULL);
    rnd = ((rnd * 0x343FD) + 0x269EC3);
    return(rnd & 0x7fffffff);
}



u8 *quickbms_tmpname(u8 **fname, u8 *prefix, u8 *ext) {
    static i32  cnt = 0;

    if(!prefix) prefix = g_temp_folder;
    for(;;) {
        spr(fname, "%s%cquickbms_%08x%08x%08x%08x.%s",
            prefix,
            PATHSLASH,
#ifdef WIN32
            (i32)GetCurrentProcessId(),
#else
            (i32)getpid(),
#endif
            cnt++,
            (i32)myrand(), (i32)myrand(),
            ext);
        if(!file_exists(*fname)) return(*fname);
    }
    return NULL;
}



u32 mydump(u8 *fname, u8 *data, u32 size) {
    FILE    *fd;

    fd = xfopen(fname, "wb");
    if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
    fwrite(data, 1, size, fd);
    FCLOSE(fd);
    return size;
}



// from NetBSD
void *mymemmem(const void *b1, const void *b2, i32 len1, i32 len2) {
    if(!b1 || !b2) return NULL;
    if(len1 < 0) len1 = strlen(b1);
    if(len2 < 0) len2 = strlen(b2);

    unsigned char *sp  = (unsigned char *) b1;
    unsigned char *pp  = (unsigned char *) b2;
    unsigned char *eos = sp + len1 - len2;

    if(!(b1 && b2 && len1 && len2))
        return NULL;

    while (sp <= eos) {
        if (*sp == *pp)
            if (memcmp(sp, pp, len2) == 0)
                return sp;
        sp++;
    }
    return NULL;
}



void *mymemimem(const void *b1, const void *b2, i32 len1, i32 len2) {
    if(!b1 || !b2) return NULL;
    if(len1 < 0) len1 = strlen(b1);
    if(len2 < 0) len2 = strlen(b2);

    unsigned char *sp  = (unsigned char *) b1;
    unsigned char *pp  = (unsigned char *) b2;
    unsigned char *eos = sp + len1 - len2;

    if(!(b1 && b2 && len1 && len2))
        return NULL;

    while (sp <= eos) {
        if (*sp == *pp)
            if (mymemicmp(sp, pp, len2) == 0)
                return sp;
        sp++;
    }
    return NULL;
}



void *mymemrmem(const void *b1, const void *b2, i32 len1, i32 len2) {
    void    *p,
            *last;

    if(!b1 || !b2) return NULL;
    last = NULL;
    for(p = (void *)b1; p = mymemmem(p, b2, len1 - (p - b1), len2); p = (void *)((unsigned char *)p + len2)) {
        last = p;
    }
    return last;
}



int invalid_chars_to_spaces(u8 *s) {
    if(!s) return -1;
    for(; *s; s++) {
        if(*s < ' ') *s = ' ';
    }
    return 0;
}



u8 *quickbms_path_open(u8 *fname) {
    int     i;
    u8      *new_fname,
            *mypath,
            *p;

    if(!fname) fname = "";
    p = mystrrchrs(fname, PATH_DELIMITERS);
    if(p) fname = p + 1;

    new_fname = NULL;
    for(i = 0; ; i++) {
        switch(i) {
            case 0:  mypath = g_bms_folder;     break;
            case 1:  mypath = g_exe_folder;     break;
            case 2:  mypath = g_file_folder;    break;
            case 3:  mypath = g_current_folder; break;
            case 4:  mypath = g_output_folder;  break;
            case 5:  mypath = ".";              break;
            default: mypath = NULL;             break;
        }
        if(!mypath) {
            FREE(new_fname)
            break;
        }
        spr(&new_fname, "%s%c%s", mypath, PATHSLASH, fname);

        if(file_exists(new_fname)) break;
    }
    return(new_fname);
}



u8 *get_extension(u8 *fname) {
    u8      *p;

    if(fname) {
        p = strrchr(fname, '.');
        if(p) return(p + 1);
        return(fname + strlen(fname));
    }
    return(fname);
}



u8 *get_filename(u8 *fname) {
    u8      *p;

    if(fname) {
        p = mystrrchrs(fname, PATH_DELIMITERS);
        if(p) return(p + 1);
    }
    return(fname);
}



u8 *get_basename(u8 *fname) {
    u8      *p,
            *l;

    p = get_filename(fname);
    if(p) {
        l = strrchr(p, '.');
        if(l) *l = 0;
        return p;
    }
    return(fname);
}



u8 *get_fullpath_from_name(u8 *fname, int folder_only) {
    static u8   tmp[PATHSZ + 1];
    static u8   *out = NULL;
    int     t;
    u8      *p;

    if(!fname) fname = "";
    xgetcwd(tmp, PATHSZ);
    p = mystrrchrs(fname, PATH_DELIMITERS);
    if(p) {
        *p++ = 0;
        out = realloc(out, PATHSZ + 1 + strlen(p) + 1);
        if(!out) STD_ERR(QUICKBMS_ERROR_MEMORY);
        out[0] = 0;

        if(xchdir(fname) < 0) {
            strcpy(out, tmp);
        } else {
            xgetcwd(out, PATHSZ);
            xchdir(tmp);
        }
        // avoids the double backslash in C:\\ and at the same time allows UNC paths
        t = strlen(out);
        if((t >= 2) && (out[t - 2] == ':') && (out[t - 1] == '\\')) t--;
        if(folder_only) out[t] = 0;
        else            sprintf(out + t, "%c%s", PATHSLASH, p);
        p[-1] = PATHSLASH;

    } else {
        out = realloc(out, PATHSZ + 1 + strlen(fname) + 1);
        if(!out) STD_ERR(QUICKBMS_ERROR_MEMORY);
        if(folder_only) strcpy(out, tmp);
        else            sprintf(out, "%s%c%s", tmp, PATHSLASH, fname);
    }
    return out;
}



// necessary to avoid that Windows handles the format... even if delimited by quotes
u8 **build_filter(u8 ***ret_filter, u8 *filter) {
    int     i,
            len,
            ret_n;
    u8      *tmp_filter = NULL,
            *p,
            *l,
            **ret       = NULL;

    if(!filter || !filter[0]) return NULL;

    if(!stricmp(get_extension(filter), "txt") && !strchr(filter, '{') && !strchr(filter, '*') && !strchr(filter, ',') && !strchr(filter, ';')) {
        FILE *fd = xfopen(filter, "rb");
        if(fd) {
            fprintf(stderr, "- filter file: \"%s\"\n", filter);
            int utf16 = incremental_fread_BOM(fd, NULL);
            tmp_filter = incremental_fread(fd, &len, 0, NULL, 0, utf16);
            FCLOSE(fd);
        }
    }
    if(!tmp_filter) {
        fprintf(stderr, "- filter string: \"%s\"\n", filter);
        tmp_filter = mystrdup_simple(filter);
    }

    if(ret_filter) ret = *ret_filter;
    ret_n = 0;
    if(ret) {
        for(ret_n = 0; ret[ret_n]; ret_n++);
    }
    for(p = tmp_filter; p && *p; p = l) {
        for(     ; *p &&  strchr(" \t\r\n", *p); p++);
        if(!*p) break;

        for(l = p; *l && !strchr(",;|\r\n", *l); l++);
        if(!*l) l = NULL;
        else *l++ = 0;

        p = skip_delimit(p);
        if(!p[0]) continue;

        // "{}.exe" (/bin/find like)
        find_replace_string(&p, NULL, "{}", -1, "*", -1);   // output is shorter so no realloc

        // "\"*.exe\""
        len = strlen(p);
        if((p[0] == '\"') && (p[len - 1] == '\"')) {
            len -= 2;
            mymemmove(p, p + 1, len);
            p[len] = 0;
        }

        ret = realloc(ret, (ret_n + 1) * sizeof(u8 *));
        if(!ret) STD_ERR(QUICKBMS_ERROR_MEMORY);
        ret[ret_n] = mystrdup_simple(p);
        ret_n++;
    }

    if(ret) {
        ret = realloc(ret, (ret_n + 1) * sizeof(u8 *));
        if(!ret) STD_ERR(QUICKBMS_ERROR_MEMORY);
        ret[ret_n] = NULL;

        for(i = 0; ret[i]; i++) {
            fprintf(stderr, "- filter %3d: %s\n", (i32)(i + 1), ret[i]);
        }
    }
    FREE(tmp_filter)
    if(ret_filter) *ret_filter = ret;
    return ret;
}



void dump_cmdline(int argc, char **argv) {
    int     i;

    printf("- command-line arguments:\n");
    for(i = 0; i < argc; i++) {
        printf("  %s\n", argv[i]);
    }
}



void *malloc_copy(void **output, void *input, int size) {
    void    *ret;

    if(!input || (size < 0)) return NULL;
    MAX_ALLOC_CHECK(size);
    ret = realloc(output ? *output : NULL, size + 1);
    if(!ret) STD_ERR(QUICKBMS_ERROR_MEMORY);
    if(output) *output = ret;
    if(input) memcpy(ret, input, size);
    else      memset(ret, 0x00,  size);
    ((u8 *)ret)[size] = 0;
    return ret;
}



u8 *append_list(u8 **ret_dst, u8 *src) {
    int     dstsz   = 0,
            srcsz   = 0;
    u8      *dst    = NULL;

    if(ret_dst) dst = *ret_dst;

    if(dst) dstsz = strlen(dst);
    if(src) srcsz = strlen(src);

    if(!dstsz) {
        dst = realloc(dst, srcsz + 1);
        if(!dst) STD_ERR(QUICKBMS_ERROR_MEMORY);
    } else {
        dst = realloc(dst, dstsz + 1 + srcsz + 1);
        if(!dst) STD_ERR(QUICKBMS_ERROR_MEMORY);
        dst[dstsz] = ';';
        dstsz++;
    }
    memcpy(dst + dstsz, src, srcsz);
    dst[dstsz + srcsz] = 0;

    if(ret_dst) *ret_dst = dst;
    return(dst);
}



#ifdef WIN32
static PVOID WINAPI (*_AddVectoredContinueHandler)(ULONG FirstHandler, PVECTORED_EXCEPTION_HANDLER VectoredHandler) = NULL;
static PVOID WINAPI (*_AddVectoredExceptionHandler)(ULONG FirstHandler, PVECTORED_EXCEPTION_HANDLER VectoredHandler) = NULL;
int winapi_missing(void) {
    static HMODULE kernel32 = NULL;
    if(!kernel32) kernel32 = GetModuleHandle("kernel32.dll");   // LoadLibrary may be dangerous
    if(kernel32) {
        if(!_AddVectoredContinueHandler)
            _AddVectoredContinueHandler = (void *)GetProcAddress(kernel32, "AddVectoredContinueHandler");
        if(!_AddVectoredExceptionHandler)
            _AddVectoredExceptionHandler = (void *)GetProcAddress(kernel32, "AddVectoredExceptionHandler");
        return 0;
    }
    return -1;
}
#endif



void fix_my_d_option(u8 *fname, u8 *fdir) {
    static u8   tmp[PATHSZ + 1];
    int     i,
            eofdir = 0;
    u8      *p,
            *s,
            a,
            b;

    if(!fname) return;
    if(!fdir) {
        tmp[0] = 0;
        xgetcwd(tmp, PATHSZ);
        fdir = tmp;
    }

    for(p = fname, s = fdir; *p && *s; p++, s++) {

        // ./
        while((p[0] == '.') && strchr(PATH_DELIMITERS, p[1])) p += 2;
        while((s[0] == '.') && strchr(PATH_DELIMITERS, s[1])) s += 2;

        a = tolower(*p);
        b = tolower(*s);

        // \/
        if(strchr(PATH_DELIMITERS, a) && strchr(PATH_DELIMITERS, b)) continue;

        // different
        if(a != b) break;
    }
    if(!*p && !*s) eofdir = 1;

    // skip the next \/
    while(*p && strchr(PATH_DELIMITERS, *p)) p++;

    // going back till the previous \/ or fname (in case the previous check fails)
    if(!eofdir) {
        for(--p; p >= fname; p--) {
            if(strchr(PATH_DELIMITERS, *p)) break;
        }
        p++;
    }

    if(p > fname) {
        for(i = 0; p[i]; i++) {
            fname[i] = p[i];
        }
        fname[i] = 0;
    }
}



u32 mycrc(u8 *data, int datasz) {
    u32     crc;
    crc = adler32(0L, Z_NULL, 0);
    if(data) {
        if(datasz < 0) datasz = strlen(data);
        crc = adler32(crc, data, datasz);
    }
    return crc;
}



void mytolower(u8 *str) {
    u8      *s;
    for(s = str; *s; s++) {
        *s = tolower(*s);
    }
}



void mytoupper(u8 *str) {
    u8      *s;
    for(s = str; *s; s++) {
        *s = toupper(*s);
    }
}



u8 *mystrnchr(u8 *p, u8 c, int n) {
	if(p) {
        while(n--) {
            if(*p == c) return p;
            p++;
        }
    }
	return NULL;
}



u8 *clean_filename(u8 *fname, int *wildcard_extension) {
    static const u8 clean_filename_chars[] = "?%*:|\"<>";
    u8      *p,
            *l,
            *s,
            *ext,
            *wild_ext;

    if(!fname || !fname[0]) return fname;
    if(fname[1] == ':') fname += 2; // fname[0] check performed by !fname[0]

    for(p = fname; *p && (*p != '\n') && (*p != '\r'); p++);
    *p = 0;

    if(wildcard_extension) {
        *wildcard_extension = -1;
        ext = strrchr(fname, '.');
        if(ext && (!ext[1] || ext[1] == '*')) {
            if(*wildcard_extension < 0) {
                *wildcard_extension = ext - fname;
            }
        }
        ext = strrchr(fname, '*');
        if(ext && !ext[1]) {
            if(*wildcard_extension < 0) {
                while(((ext - 1) >= fname) && (ext[-1] == '.')) ext--;
                *wildcard_extension = ext - fname;
            }
        }
    }

    // remove final spaces and dots
    for(p = fname + strlen(fname); p >= fname; p--) {
        if(!strchr(clean_filename_chars, *p)) {
            if((*p != ' ') && (*p != '.')) break;
        }
        *p = 0;
    }

    for(p = fname; *p; p++) {
        if(strchr(clean_filename_chars, *p)) {    // invalid filename chars not supported by the most used file systems
            *p = '_';
        }
    }
    *p = 0;

    // remove spaces at the end of the folders (they are not supported by some OS)
    for(p = fname; *p; p = l + 1) {
        l = mystrchrs(p, PATH_DELIMITERS);
        if(!l) break;
        for(s = l - 1; s >= p; s--) {
            if(*s > ' ') break;
        }
        s++;
        mymemmove(s, l, -1);
        l = s;
    }

    // remove final spaces and dots
    for(p = fname + strlen(fname); p >= fname; p--) {
        if(!strchr(clean_filename_chars, *p)) {
            if((*p != ' ') && (*p != '.')) break;
        }
        *p = 0;
    }
    wild_ext = p + 1;

    if(wildcard_extension && (*wildcard_extension >= 0)) {
        if(*wildcard_extension > (wild_ext - fname)) {
            *wildcard_extension = wild_ext - fname;
        }
    }
    return(fname);
}



int debug_privileges(void)  {
#ifdef WIN32
    TOKEN_PRIVILEGES tp;
    HANDLE  hp;

    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hp)) return -1;
    memset(&tp, 0, sizeof(tp));
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) return FALSE;
    if(!AdjustTokenPrivileges(hp, FALSE, &tp, sizeof(tp), NULL, NULL)) return -1;
    CloseHandle(hp);
#endif
    return 0;
}



void get_temp_path(u8 *output, int outputsz) {
    u8      *p;

    output[0] = 0;
#ifdef WIN32
    static DWORD WINAPI (*_GetTempPathW)(DWORD nBufferLength, LPWSTR lpBuffer) = NULL;
    if(!_GetTempPathW) {
        _GetTempPathW = (void *)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetTempPathW");
    }
    if(_GetTempPathW) {
        wchar_t *outputW = (wchar_t *)calloc(outputsz + 1, sizeof(wchar_t));
        if(!outputW) STD_ERR(QUICKBMS_ERROR_MEMORY);
        outputW[0] = 0;
        _GetTempPathW(outputsz, outputW);
        mystrcpy(output, native_unicode_to_utf8(outputW), outputsz);
        FREE(outputW)
    } else {
        GetTempPath(outputsz, output);
    }
#endif
    if(!output[0]) {
        p = getenv ("TMP");
        if(!p) p = getenv ("TEMP");
        if(!p) p = getenv ("TMPDIR");
        if(!p) p = getenv ("TEMPDIR");
#ifdef WIN32
        if(!p) {
            p = getenv ("LOCALAPPDATA");
            if(p) p = spr2("%s\\%s", p, "temp");
        }
        if(!p) {
            p = getenv ("USERPROFILE");
            if(p) p = spr2("%s\\%s", p, "AppData\\Local\\Temp");
        }
        if(!p) {
            p = getenv ("WINDIR");
            if(p) p = spr2("%s\\%s", p, "temp");
        }
        if(!p) p = "c:\\windows\\temp"; // it's wrong but it should never happen
#else
        if(!p) p = "/tmp";
#endif
        mystrcpy(output, p, outputsz);

        // TMP="z:\new temp folder\aaa bbb ccc"
        while(*p && ((*p <= ' ') || (*p == '\"'))) p++;
        if(*p) {
            mymemmove(output, p, -1);
            for(p = output + strlen(output) - 1; (p >= output) && ((*p <= ' ') || (*p == '\"')); p--);
            *p = 0;
        }
    }
}



int need_quote_delimiters(u8 *p) {
    if(!p || !p[0]) return 1;   // ""
    if(p[0] == '\"') return 0;  // already quoted
    if(mystrchrs(p,
        "^&|<>() \t,;=\xff%"
    )) return 1;
    return 0;
}



#define quickbms_archive_output_open(X) \
        if(g_quick##X && !g_quick##X->fd) { \
            p = fname; \
            if(files) p = g_file_folder; \
            for(i = 0;; i++) { \
                if(!i) sprintf(iso_fname, "%.*s.%s",    (i32)(sizeof(iso_fname) - 20), get_filename(p), #X); \
                else   sprintf(iso_fname, "%.*s_%d.%s", (i32)(sizeof(iso_fname) - 20), get_filename(p), (i32)i, #X); \
                printf("- generating %s %s in the output folder\n", #X, iso_fname); \
                t = check_overwrite(iso_fname, 0); \
                if(!t) break; \
                if(t == -2) continue; \
                myexit(QUICKBMS_ERROR_FILE_WRITE); \
            } \
            if(quick##X##_open(g_quick##X, iso_fname) < 0) { \
                fprintf(stderr, "\nError: impossible to create the %s file\n", #X); \
                myexit(QUICKBMS_ERROR_FILE_WRITE); \
            } \
        }

#define quickbms_archive_output_write(X, WRITE_BUFF) \
                    if(g_quick##X) { \
                        if(g_append_mode != APPEND_MODE_NONE) { \
                            fprintf(stderr, \
                                "\nError: the %s mode doesn't support direct append mode on the output files.\n", #X); \
                            myexit(QUICKBMS_ERROR_FILE_WRITE); \
                        } \
                        fd = g_quick##X->fd; \
                        if(!fd) { \
                            fprintf(stderr, \
                                "\nError: the %s file has not been created and so it's not possible to extract the files.\n", #X); \
                            myexit(QUICKBMS_ERROR_FILE_WRITE); \
                        } \
                        quick##X##_add_entry(g_quick##X, fname, WRITE_BUFF, size); \
                        if(WRITE_BUFF) fd = NULL; \
                        break; \
                    }



TCCState *tcc_compiler(u8 *input) {
    TCCState *tccstate;
    tccstate = tcc_new();
    if(!tccstate) myexit(QUICKBMS_ERROR_BMS);
    tcc_set_output_type(tccstate, TCC_OUTPUT_MEMORY);
    /*
    // this boring tcc wants lib\libtcc1.a or will not work.
    // anyway -nostdlib allows to bypass this annoyance but
    // you can't use the stdlib API (printf, strcmp and so on).
    // unfortunately the following code that forces tcc to load
    // our libtcc1.a is not enough and it still requires lib\libtcc1.a
    static u8   *tmp_fname = NULL;
    if(!tmp_fname) {
        quickbms_tmpname(&tmp_fname,  NULL, "libtcc1_a");
        mydump(tmp_fname, libtcc1_a, sizeof(libtcc1_a) - 1);
    }
    tcc_add_file(tccstate, tmp_fname);
    */
    tcc_set_options(tccstate, "-nostdlib -DFILE=void");
    if(tcc_compile_string(tccstate, input) < 0) myexit(QUICKBMS_ERROR_BMS);
    return tccstate;
}



#include "tcc_symbols.c"



void external_executable_prompt(int cmd, u8 *dllname, int is_exe) {
    if(!enable_calldll) {
        fprintf(stderr, "\n"
            "- the script needs to load a pre-compiled function from the dll or code\n"
            "  %s\n"
            "%s"
            "  do you want to continue with this and any other requested dll/code (y/N)?\n"
            "  ",
            dllname, is_exe ? "- also note that it's an executable so its working is not guarantee\n" : "");
        if(get_yesno(NULL) != 'y') myexit_cmd(cmd, QUICKBMS_ERROR_USER);
        enable_calldll = 1; // needed to avoid to bug the user N times with the same question
    }
}



u8 *de_html_putc(u8 *start, u8 *s, int chr) {
    if(chr < 0) {
        if((s == start) || (s[-1] == '\n')) return s;
        return NULL;
    }
    if(chr == 0) return s;
    if(chr == '\r') return s;
    if(chr == '\t') chr = ' ';
    if(chr <= ' ') {    // line-feed, space, tab and so on
        if((s == start) || (s[-1] == '\n')) return s;
    }
    if(!chr || (chr == '\n')) {
        u8  *l;
        for(l = s - 1; (l >= start) && (*l <= ' '); l--);
        s = l + 1;
    }
    int t = utf16_to_utf8_chr(chr, s, max_utf8_char_size, 0, g_codepage);
    if(t < 0) *s++ = '.';
    else      s += t;
    return s;
}



int html_to_text(u8 *uri, u8 **only_one_char) {
    typedef struct {
        char    *code;
        int     chr;
    } html_codes_t;
    static const html_codes_t html_codes[] = {
        { "exclamation", 0x00021 },
        { "quot", 0x00021 },
        { "percent", 0x00025 },
        { "amp", 0x0026 },
        { "apos", 0x0026 },
        { "add", 0x0002B },
        { "lt", 0x003C },
        { "equal", 0x0003D },
        { "gt", 0x003E },
        { "nbsp", 0x00A0 },
        { "iexcl", 0x00A1 },
        { "cent", 0x00A2 },
        { "pound", 0x00A3 },
        { "curren", 0x00A4 },
        { "yen", 0x00A5 },
        { "brvbar", 0x00A6 },
        { "sect", 0x00A7 },
        { "uml", 0x00A8 },
        { "copy", 0x00A9 },
        { "ordf", 0x00AA },
        { "laquo", 0x00AB },
        { "not", 0x00AC },
        { "shy", 0x00AD },
        { "reg", 0x00AE },
        { "macr", 0x00AF },
        { "deg", 0x00B0 },
        { "plusmn", 0x00B1 },
        { "sup2", 0x00B2 },
        { "sup3", 0x00B3 },
        { "acute", 0x00B4 },
        { "micro", 0x00B5 },
        { "para", 0x00B6 },
        { "middot", 0x00B7 },
        { "cedil", 0x00B8 },
        { "sup1", 0x00B9 },
        { "ordm", 0x00BA },
        { "raquo", 0x00BB },
        { "frac14", 0x00BC },
        { "frac12", 0x00BD },
        { "frac34", 0x00BE },
        { "iquest", 0x00BF },
        { "Agrave", 0x00C0 },
        { "Aacute", 0x00C1 },
        { "Acirc", 0x00C2 },
        { "Atilde", 0x00C3 },
        { "Auml", 0x00C4 },
        { "Aring", 0x00C5 },
        { "AElig", 0x00C6 },
        { "Ccedil", 0x00C7 },
        { "Egrave", 0x00C8 },
        { "Eacute", 0x00C9 },
        { "Ecirc", 0x00CA },
        { "Euml", 0x00CB },
        { "Igrave", 0x00CC },
        { "Iacute", 0x00CD },
        { "Icirc", 0x00CE },
        { "Iuml", 0x00CF },
        { "ETH", 0x00D0 },
        { "Ntilde", 0x00D1 },
        { "Ograve", 0x00D2 },
        { "Oacute", 0x00D3 },
        { "Ocirc", 0x00D4 },
        { "Otilde", 0x00D5 },
        { "Ouml", 0x00D6 },
        { "times", 0x00D7 },
        { "Oslash", 0x00D8 },
        { "Ugrave", 0x00D9 },
        { "Uacute", 0x00DA },
        { "Ucirc", 0x00DB },
        { "Uuml", 0x00DC },
        { "Yacute", 0x00DD },
        { "THORN", 0x00DE },
        { "szlig", 0x00DF },
        { "agrave", 0x00E0 },
        { "aacute", 0x00E1 },
        { "acirc", 0x00E2 },
        { "atilde", 0x00E3 },
        { "auml", 0x00E4 },
        { "aring", 0x00E5 },
        { "aelig", 0x00E6 },
        { "ccedil", 0x00E7 },
        { "egrave", 0x00E8 },
        { "eacute", 0x00E9 },
        { "ecirc", 0x00EA },
        { "euml", 0x00EB },
        { "igrave", 0x00EC },
        { "iacute", 0x00ED },
        { "icirc", 0x00EE },
        { "iuml", 0x00EF },
        { "eth", 0x00F0 },
        { "ntilde", 0x00F1 },
        { "ograve", 0x00F2 },
        { "oacute", 0x00F3 },
        { "ocirc", 0x00F4 },
        { "otilde", 0x00F5 },
        { "ouml", 0x00F6 },
        { "divide", 0x00F7 },
        { "oslash", 0x00F8 },
        { "ugrave", 0x00F9 },
        { "uacute", 0x00FA },
        { "ucirc", 0x00FB },
        { "uuml", 0x00FC },
        { "yacute", 0x00FD },
        { "thorn", 0x00FE },
        { "yuml", 0x00FF },
        { "OElig", 0x0152 },
        { "oelig", 0x0153 },
        { "Scaron", 0x0160 },
        { "scaron", 0x0161 },
        { "Yuml", 0x0178 },
        { "fnof", 0x0192 },
        { "circ", 0x02C6 },
        { "tilde", 0x02DC },
        { "Alpha", 0x0391 },
        { "Beta", 0x0392 },
        { "Gamma", 0x0393 },
        { "Delta", 0x0394 },
        { "Epsilon", 0x0395 },
        { "Zeta", 0x0396 },
        { "Eta", 0x0397 },
        { "Theta", 0x0398 },
        { "Iota", 0x0399 },
        { "Kappa", 0x039A },
        { "Lambda", 0x039B },
        { "Mu", 0x039C },
        { "Nu", 0x039D },
        { "Xi", 0x039E },
        { "Omicron", 0x039F },
        { "Pi", 0x03A0 },
        { "Rho", 0x03A1 },
        { "Sigma", 0x03A3 },
        { "Tau", 0x03A4 },
        { "Upsilon", 0x03A5 },
        { "Phi", 0x03A6 },
        { "Chi", 0x03A7 },
        { "Psi", 0x03A8 },
        { "Omega", 0x03A9 },
        { "alpha", 0x03B1 },
        { "beta", 0x03B2 },
        { "gamma", 0x03B3 },
        { "delta", 0x03B4 },
        { "epsilon", 0x03B5 },
        { "zeta", 0x03B6 },
        { "eta", 0x03B7 },
        { "theta", 0x03B8 },
        { "iota", 0x03B9 },
        { "kappa", 0x03BA },
        { "lambda", 0x03BB },
        { "mu", 0x03BC },
        { "nu", 0x03BD },
        { "xi", 0x03BE },
        { "omicron", 0x03BF },
        { "pi", 0x03C0 },
        { "rho", 0x03C1 },
        { "sigmaf", 0x03C2 },
        { "sigma", 0x03C3 },
        { "tau", 0x03C4 },
        { "upsilon", 0x03C5 },
        { "phi", 0x03C6 },
        { "chi", 0x03C7 },
        { "psi", 0x03C8 },
        { "omega", 0x03C9 },
        { "thetasym", 0x03D1 },
        { "upsih", 0x03D2 },
        { "piv", 0x03D6 },
        { "ensp", 0x2002 },
        { "emsp", 0x2003 },
        { "thinsp", 0x2009 },
        { "zwnj", 0x200C },
        { "zwj", 0x200D },
        { "lrm", 0x200E },
        { "rlm", 0x200F },
        { "ndash", 0x2013 },
        { "mdash", 0x2014 },
        { "horbar", 0x2015 },
        { "lsquo", 0x2018 },
        { "rsquo", 0x2019 },
        { "sbquo", 0x201A },
        { "ldquo", 0x201C },
        { "rdquo", 0x201D },
        { "bdquo", 0x201E },
        { "dagger", 0x2020 },
        { "Dagger", 0x2021 },
        { "bull", 0x2022 },
        { "hellip", 0x2026 },
        { "permil", 0x2030 },
        { "prime", 0x2032 },
        { "Prime", 0x2033 },
        { "lsaquo", 0x2039 },
        { "rsaquo", 0x203A },
        { "oline", 0x203E },
        { "frasl", 0x2044 },
        { "euro", 0x20AC },
        { "image", 0x2111 },
        { "weierp", 0x2118 },
        { "real", 0x211C },
        { "trade", 0x2122 },
        { "alefsym", 0x2135 },
        { "larr", 0x2190 },
        { "uarr", 0x2191 },
        { "rarr", 0x2192 },
        { "darr", 0x2193 },
        { "harr", 0x2194 },
        { "crarr", 0x21B5 },
        { "lArr", 0x21D0 },
        { "uArr", 0x21D1 },
        { "rArr", 0x21D2 },
        { "dArr", 0x21D3 },
        { "hArr", 0x21D4 },
        { "forall", 0x2200 },
        { "part", 0x2202 },
        { "exist", 0x2203 },
        { "empty", 0x2205 },
        { "nabla", 0x2207 },
        { "isin", 0x2208 },
        { "notin", 0x2209 },
        { "ni", 0x220B },
        { "prod", 0x220F },
        { "sum", 0x2211 },
        { "minus", 0x2212 },
        { "lowast", 0x2217 },
        { "radic", 0x221A },
        { "prop", 0x221D },
        { "infin", 0x221E },
        { "ang", 0x2220 },
        { "and", 0x2227 },
        { "or", 0x2228 },
        { "cap", 0x2229 },
        { "cup", 0x222A },
        { "int", 0x222B },
        { "there4", 0x2234 },
        { "sim", 0x223C },
        { "cong", 0x2245 },
        { "asymp", 0x2248 },
        { "ne", 0x2260 },
        { "equiv", 0x2261 },
        { "le", 0x2264 },
        { "ge", 0x2265 },
        { "sub", 0x2282 },
        { "sup", 0x2283 },
        { "nsub", 0x2284 },
        { "sube", 0x2286 },
        { "supe", 0x2287 },
        { "oplus", 0x2295 },
        { "otimes", 0x2297 },
        { "perp", 0x22A5 },
        { "sdot", 0x22C5 },
        { "lceil", 0x2308 },
        { "rceil", 0x2309 },
        { "lfloor", 0x230A },
        { "rfloor", 0x230B },
        { "lang", 0x2329 },
        { "rang", 0x232A },
        { "loz", 0x25CA },
        { "spades", 0x2660 },
        { "clubs", 0x2663 },
        { "hearts", 0x2665 },
        { "diams", 0x2666 },
        { NULL, -1 }
    };

    i32     t,
            n;
    u8      *ret,
            *p,
            *l;

    if(!uri) return 0;
    //u8 *uril = uri + strlen(uri); // do NOT use strlen() or the processing will be very slow!
    ret = uri;
    p = ret;
    while(*uri) {
        t = 0;
        if(*uri == '&') {
            uri++;
            for(l = uri; *l && (*l != ';'); l++);
            int     bck = *l;
            *l = 0;
            if(*uri == '#') {
                uri++;
                if(tolower(*uri) == 'x') {
                    uri++;
                    if(sscanf(uri, "%x", &t) != 1) t = *uri;
                } else {
                    if(sscanf(uri, "%d", &t) != 1) t = *uri;
                }
            } else {
                int     i;
                for(i = 0; html_codes[i].code; i++) {
                    if(!stricmp(uri, html_codes[i].code)) {
                        t = html_codes[i].chr;
                        break;
                    }
                }
            }
            *l = bck;
            uri = l;    // points to ';' or 0, need++ as next instruction
            if(*uri) uri++;
        } else {
            wchar_t wc = 0;
            n = utf8_to_utf16_chr(uri, /* do not use uril or -1, strlen is expensive */ max_utf8_char_size, &wc, 1, g_codepage);
            if(n > 0) {
                t = wc;
                uri += n;
            } else {
                t = *uri++;
            }
        }
        if(!t) t = ' ';

        if(only_one_char) { *only_one_char = uri; return t; }
        p = de_html_putc(p, p, t);
    }
    if(only_one_char) { *only_one_char = uri; return 0; }
    *p = 0;
    return p - ret;
}



int de_html(u8 *in, int insz, u8 *out, int outsz) {
    int     skip        = 0;
    u8      *inl,
            *outl,
            *p,
            *s,
            *last_p;

    if(insz  < 0) insz  = strlen(in);
    if(outsz < 0) outsz = insz; // who cares
    inl  = in  + insz;
    outl = out + outsz;
    p = in;
    s = out;
    while((p < inl) && (s < outl)) {
        if(!strnicmp(p, "<br>", 4) || !strnicmp(p, "</br>", 5) || !strnicmp(p, "<br/>", 5)) {
            s = de_html_putc(out, s, '\n');
            p = strchr(p, '>') + 1;
            //if(!p) break;
        } else if(*p == '<') {
            p++;
            if(skip) skip = 0;
            if(!strnicmp(p, "script", 5)) skip = 1;
            if((p[0] == '/') && ((tolower(p[1]) == 'p') || (tolower(p[1]) == 'h'))) {
                s = de_html_putc(out, s, '\n');    // paragraph and header
            }
            while(*p && (*p != '>')) p++;
            p++;
        } else if(skip) {
            p++;
        } else {
            last_p = p;
            s = de_html_putc(out, s, html_to_text(p, &p));
            if(p == last_p) p++;    // it happens with NUL bytes
        }
    }
    *s = 0;
    return s - out;

    // do not activate the following, it will cause an overflow because chars >=0x80 & <=0xff are re-encoded as 2 bytes: 1 -> 2 bytes
    // it was used in the past for some sample that I currently don't remember
    //return html_to_text(out, NULL);
}



int html_easy(u8 *in, int insz, u8 *out, int outsz) {
    int     t;
    u8      *inl,
            *outl,
            *p,
            *s,
            *last_p;

    if(insz  < 0) insz  = strlen(in);
    if(outsz < 0) outsz = insz; // who cares
    // s should be double buff, don't care

    inl  = in  + insz;
    outl = out + outsz;
    p = in;
    s = out;
    while(p < inl) {
        t = p[0];
        if(s >= outl) break;
        if(t == '<') s = de_html_putc(out, s, '\n');

        if(s >= outl) break;
        last_p = p;
        s = de_html_putc(out, s, html_to_text(p, &p));  // p is incremented here!
        if(p == last_p) p++;    // it happens with NUL bytes

        if(s >= outl) break;
        if(t == '>') s = de_html_putc(out, s, '\n');
    }
    *s = 0;
    return s - out;
}



int quick_utf8_to_ascii(u8 *str) {
    static const u8 ascii_table[] =
    "\x00\x20\x20\x20\x20\x20\x20\x20\x20\x09\x0a\x20\x20\x0d\x20\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
    "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
    "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
    "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
    "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x27\x27\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x21\x63\x24\x27\x59\x7c\x24\x22\x40\x40\x3c\x2d\x2d\x40\x2d"
    "\x27\x2b\x32\x33\x27\x75\x7c\x2d\x2c\x31\x27\x3e\x2f\x2f\x2f\x3f"
    "\x41\x41\x41\x41\x41\x41\x41\x43\x45\x45\x45\x45\x49\x49\x49\x49"
    "\x44\x4e\x4f\x4f\x4f\x4f\x4f\x78\x30\x55\x55\x55\x55\x59\x62\x42"
    "\x61\x61\x61\x61\x61\x61\x61\x63\x65\x65\x65\x65\x69\x69\x69\x69"
    "\x6f\x6e\x6f\x6f\x6f\x6f\x6f\x2d\x6f\x75\x75\x75\x75\x79\x62\x79";
    u8      *p, *o;
    int     n, x;

    if(!str) return 0;
    p = str;
    o = str;
    while(*p) {
        n = 0;
        if(*p & 0x80) {
            for(x = 0; x < 6; x++){
                if((*p & (1 << (7 - x))) && !(*p & (1 << (7 - (x + 1))))) break;
            }
            n = *p & ((1 << (7 - (x + 2))) - 1);
            p++;
            while(x--) {
                if(!*p) break;
                n <<= 6;
                n |= (*p & ((1 << 6) - 1));
                p++;
            }
        } else {
            n = *p++ & 0x7f;
        }
        *o++ = ascii_table[n & 0xff];
    }
    *o = 0;
    return o - str;
}



u8 *json_viewer(u8 *str) {
    i32     quotes  = 0,
            crlf    = 0,
            spaces  = 0,
            len,
            i,
            n;
    u8      *p,
            *limit;

    static u8  *out = NULL;
    out = realloc(out, (strlen(str) * 4) + 1);
    if(!out) STD_ERR(QUICKBMS_ERROR_MEMORY);
    u8  *o = out;
    limit = str + strlen(str);
    for(p = str; p < limit; p++) {
        if(!quotes) {
            if(strchr("\r\n", *p)) continue;
            if(strchr("}]", *p)) {
                spaces -= 2;
                crlf = 1;
            }
            if(crlf) {
                o += sprintf(o, "\n%*s", (i32)spaces, "");
                crlf = 0;
            }
            if(!strchr(",:\"", *p)) {
                *o++ = *p;
            }
        }
        if(*p == '\\') {
            p++;
            n = -1;
            len = 0;
            switch(*p) {
                case 0:  /*return(-1);*/ n = -1; break;
                //case '0':  n = '\0'; break;
                case 'a':  n = '\a'; break;
                case 'b':  n = '\b'; break;
                case 'e':  n = '\e'; break;
                case 'f':  n = '\f'; break;
                case 'n':  n = '\n'; break;
                case 'r':  n = '\r'; break;
                case 't':  n = '\t'; break;
                case 'v':  n = '\v'; break;
                case '\"': n = '\"'; break;
                case '\'': n = '\''; break;
                case '\\': n = '\\'; break;
                case '?':  n = '\?'; break;
                case '.':  n = '.';  break;
                case ' ':  n = ' ';  break;
                case 'u': {
                    if(sscanf(p + 1, "%04x%n", &n, &len) != 1) n = -1; break; /*return(-1);*/
                    if(len > 4) len = 4;
                    p += len;
                    *o++ = n;   n = (u32)n >> 8;    // this is NOT a real unicode->utf8 conversion! maybe in the next versions
                    break;
                }
                case 'U': {
                    if(sscanf(p + 1, "%08x%n", &n, &len) != 1) n = -1; break; /*return(-1);*/
                    if(len > 8) len = 8;
                    p += len;
                    *o++ = n;   n = (u32)n >> 8;    // this is NOT a real unicode->utf8 conversion! maybe in the next versions
                    *o++ = n;   n = (u32)n >> 8;
                    *o++ = n;   n = (u32)n >> 8;
                    break;
                }
                case 'x': {
                    //n = readbase(p + 1, 16, &len);
                    //if(len <= 0) return(-1);
                    if(sscanf(p + 1, "%02x%n", &n, &len) != 1) n = -1; break; /*return(-1);*/
                    if(len > 2) len = 2;
                    p += len;
                    break;
                }
                default: {
                    //n = readbase(p, 8, &len);
                    //if(len <= 0) return(-1);
                    if(sscanf(p, "%3o%n", &n, &len) != 1) n = -1; break; /*return(-1);*/
                    if(len > 3) len = 3;
                    p += (len - 1); // work-around for the subsequent p++;
                    break;
                }
            }
            if(n >= 0) *o++ = n;
            continue;
        }
        if(quotes) {
            if(*p == '\"') {
                quotes = 0;
            } else {
                *o++ = *p;
            }
        } else {
            switch(*p) {
                case '[':
                case '{': spaces += 2; crlf = 1; break;
                //case ']':
                //case '}': spaces -= 2; crlf = 1; break;
                case '\"': quotes = 1; break;
                case ':': o += sprintf(o, "   "); break;
                case ',': crlf = 1; break;
                //case '/': currently the comments are not supported and may break compatibility
                default: break;
            }
        }
    }
    if((o > out) && (o[-1] != '\n')) *o++ = '\n';
    *o = 0;

    len = quick_utf8_to_ascii(out);
    for(i = 0; i < len; i++) {
        if(!out[i]) out[i] = ' ';
    }

    return out;
}



typedef struct {
    u8      *names; // list of comma-separated names of the parameters: ",tag,par1,par2,par3,"
    int     names_size;
    u8      *par;   // parameter
    u8      *val;   // value
    int     pv;     // par/val selector

    int     tagi;   // work-around
    u8      *tag;   // work-around
} xml_json_parser_ctx_t;



typedef struct xml_json_parser_names_t {
    u32     crc;
    int     num;
    UT_hash_handle hh;
    struct xml_json_parser_names_t *next;
    struct xml_json_parser_names_t *prev;
} xml_json_parser_names_t;
xml_json_parser_names_t *g_xml_json_parser_names_var    = NULL,
                        *g_xml_json_parser_names_list   = NULL;



void xml_json_parser_free(xml_json_parser_ctx_t *ctx) {
    if(ctx) {
        FREE(ctx->names)
        FREE(ctx->par)
        FREE(ctx->val)
        FREE(ctx->tag)
    }
}



static xml_json_parser_names_t *xml_json_parser_names(xml_json_parser_names_t **X, u8 *Y, int *ret_next) {
    xml_json_parser_names_t *ef;
    u32     crc;

    if(ret_next) *ret_next = -1;
    crc = mycrc(Y, -1);
    HASH_FIND_INT(*X, &crc, ef);
    if(!ef) {
        ef = real_calloc(1, sizeof(xml_json_parser_names_t));
        if(!ef) STD_ERR(QUICKBMS_ERROR_MEMORY);
        ef->crc = crc;
        ef->num = 1;
        HASH_ADD_INT(*X, crc, ef);
    } else {
        if(ret_next) *ret_next = 0;
    }
    return ef;
}



// there is no real improvement so keep it off
//#define xml_json_parser_names_var_optimization  1



static void xml_json_parser_names_var(int i_idx, u8 *Y, u8 *Z) {
    xml_json_parser_names_t *ef;

    if(!Y) return;
    // in theory Z can be NULL if we are adding a tag

    ef = xml_json_parser_names(&g_xml_json_parser_names_var, Y, NULL);

#ifdef xml_json_parser_names_var_optimization
    // ef->num can't be zero, first element is 1 (going to index 0)
    if(ef->num > 1)
#endif
    {
        int     idx;
        int     Ysz = strlen(Y);
        u8      *name_tmp = alloca(Ysz + 3 + 1);    // alloca is on the stack
        if(!name_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memcpy(name_tmp, Y, Ysz);

        // we need to initialize the variable first if not existent, this is a "core thing" of quickbms
        // (in short if you just use 'print "%VAR[i]%"' it fails because VAR[i] must be used somewhere first)
        strcpy(name_tmp + Ysz, "[i]");
        idx = get_var_from_name(name_tmp, -1);
        if(idx < 0) idx = add_var(0, name_tmp, NULL, 0, -2);    // -2 is the magic value for initialization like in bms.c

#ifdef xml_json_parser_names_var_optimization
        // this work-around creates a VAR[0] variable by copying the content of the last VAR variable
        // the idea is using "String X" even for many one-line instructions without creating arrays
        if(ef->num == 2) {
            // VAR[0]
            add_var(i_idx, NULL, NULL, 0, sizeof(int));

            // VAR[0] = VAR
            add_var(idx, NULL, get_var(get_var_from_name(Y, -1)), 0, -1);
        }
#endif

        // VAR[i]
        add_var(i_idx, NULL, NULL, ef->num - 1, sizeof(int));

        // VAR[i] = value
        add_var(idx, NULL, Z, 0, -1);

        // VAR[] = elements
        strcpy(name_tmp + Ysz, "[]");
        add_var(0, name_tmp, NULL, ef->num,     sizeof(int));
    }

    // VAR = value
    add_var(0, Y, Z, 0, -1);

    if(Z) ef->num++;
}



u8 *xml_json_parser(u8 *buff, u8 *limit, int start, int end, int level, xml_json_parser_ctx_t *ctx, u8 *key) {
    /*
    "var"           content (fail-safe)
    "var[]"         elements
    "var[i]"        content
    // key/path is not implemented, it's a waste of time
    for i = 0 < var[]
        print "%i%: %var[i]%"
    next i
    */

    static const u8 escapes[] = "<>{}[]()\"\'`:=;&?|!,/";
    int     elements    = 0,
            strsz       = 0,
            c,
            t;
    u8      *p          = NULL,
            *str        = NULL,
            *last_p     = NULL;

    // long story short: currently the multi-dimensional array work in a weird way where var[i] and var[0] are 2 different things
    int     bck_level   = level;
    int     i_idx       = get_var_from_name("i", -1);
    if(i_idx < 0) i_idx = add_var(0, "i", NULL, 0, sizeof(int));
    int     i_bck       = get_var32(i_idx);

    if(!ctx) goto quit;
    if(!buff) goto quit;
    if(!limit) limit = buff + strlen(buff);

    p = buff;
    while(p) {
        if((end < 0) || (start != end)) {

            for(;;) {
                if(p >= limit) goto quit;
                last_p = p;
                c = *p++;
                if(!c || (c > ' ')) break;
            }
            if(!c) continue;
            if(c == end) break;

            switch(c) { // escapes
                case '<':   ctx->tagi = 1;
                            ctx->pv = 0;    p = xml_json_parser(p, limit, c, '>', level + 1, ctx, key);
                            ctx->tagi = 2;                                                                  break;  // html and xml
                case '{':   ctx->pv = 0;    p = xml_json_parser(p, limit, c, '}', level + 1, ctx, key);     break;  // json
                case '[':   ctx->pv = 0;    p = xml_json_parser(p, limit, c, ']', level,     ctx, NULL);    break;  // json
                case '(':   ctx->pv = 0;    p = xml_json_parser(p, limit, c, ')', level,     ctx, NULL);    break;  // json
                case '\"':                  p = xml_json_parser(p, limit, c, c,   level,     ctx, NULL);    break;  // strings
                case '\'':                  p = xml_json_parser(p, limit, c, c,   level,     ctx, NULL);    break;  // strings and chars
                case '`':                   p = xml_json_parser(p, limit, c, c,   level,     ctx, NULL);    break;  // strings
                case ':':
                case '=':   ctx->pv = 1;                                                                    break;  // values
                case ';':
                case '&':
                case '?':
                case '|':
                case '!':
                case ',':   ctx->pv = 0;                                                                    break;  // separators
                case '/':   ctx->tagi = 4;                                                                  break;  // html
                default:    p--;            c = (ctx->tagi == 2) ? '<' : 0;
                                            p = xml_json_parser(p, limit, c, c,   level,     ctx, NULL);    break;  // strings
            }
            if(!p) break;                   // allow forced termination
            if(p <= last_p) p = last_p + 1; // safe check necessary to avoid endless loops with invalid data!
            continue;
        }

        strsz = 0;
        str = realloc(str, strsz + 1);
        if(!str) STD_ERR(QUICKBMS_ERROR_MEMORY);
        str[strsz] = 0;

        for(;;) {
            if(p >= limit) break;
            c = *p++;
            if(!c) c = ' '; //continue;
            str = realloc(str, strsz + 1 + 1 + 1);  // c + \0 + possible escape after '\'
            if(!str) STD_ERR(QUICKBMS_ERROR_MEMORY);
            str[strsz++] = c;
            str[strsz]   = 0;

            if(c == '\\') {    // simple escape handler, just one char, so can't support things like \x00 but it's ok
                str[strsz++] = *p++;
                str[strsz]   = 0;
                continue;
            }
            if((c == end) || (c == start)) {
                str[--strsz] = 0;
                break;
            }
            if(!end || (start != end)) {    // structs and not "strings"
                if((c <= ' ') || strchr(escapes, c)) {
                    str[--strsz] = 0;
                    p--;
                    break;
                }
            }
        }

        if(strsz >= 0) {
            html_to_text(str, NULL);
            cstring(str, str, -1, NULL, NULL);  // html_text removes the non-text chars (like line-feed)
            quick_utf8_to_ascii(str);
            if(level < 0) level = 0;
            if(!ctx->pv) {
                mystrdup(&ctx->par, str);
            } else {
                mystrdup(&ctx->val, str);
            }
            if(ctx->tagi == 1) {
                ctx->tagi = 0;
                mystrdup(&ctx->tag, ctx->par);
            }
            if(ctx->tagi == 2) {
                ctx->tagi = 3;
                if(ctx->tag && ctx->tag[0]) {
                    xml_json_parser_names_var(i_idx, ctx->tag, str);
                    FREE(ctx->tag)
                    FREE(ctx->par)  // avoid duplicates
                }
            }
            if(ctx->tagi >= 4) {
                ctx->tagi = 0;
            } else {
                if(ctx->pv &&   // added in Quickbms 0.11
                    (ctx->par && ctx->par[0])   // necessary also due to some tag bug
                ) {
                    xml_json_parser_names_var(i_idx, ctx->par, ctx->val);

                    if(!ctx->pv) {
                        xml_json_parser_names(&g_xml_json_parser_names_list, str, &t);
                        if(t < 0) {
                            if(!ctx->names) mystrdup(&ctx->names, ",");
                            int t = strlen(str);    // the previous functions changed the length of str
                            ctx->names = realloc(ctx->names, strlen(ctx->names) + t + 1 + 1);
                            if(!(ctx->names)) STD_ERR(QUICKBMS_ERROR_MEMORY);
                            /*
                            strcat(ctx->names, str);
                            strcat(ctx->names, ",");
                            */
                            // not faster, totally useless but that's ok
                            memcpy(ctx->names + ctx->names_size, str, t);
                            ctx->names_size += t;
                            ctx->names[ctx->names_size++] = ',';
                            ctx->names[ctx->names_size] = 0;
                        }
                    }
                }
            }
            if(ctx->pv) {
                FREE(ctx->par)
                FREE(ctx->val)
                ctx->pv = 0;
            }
        }
        break;
    }
quit:
    if(bck_level < 0) {
        xml_json_parser_names_t *ef         = NULL,
                                *ef_next    = NULL;
        HASH_ITER(hh, g_xml_json_parser_names_var, ef, ef_next) {
            HASH_DEL(g_xml_json_parser_names_var, ef);
            real_free(ef);
        }
        HASH_ITER(hh, g_xml_json_parser_names_list, ef, ef_next) {
            HASH_DEL(g_xml_json_parser_names_list, ef);
            real_free(ef);
        }
    }
    add_var(i_idx, NULL, NULL, i_bck, sizeof(int));
    FREE(str)
    return p;
}



double calculate_entropy(unsigned char *data, int size, unsigned char *most_recurring_byte) {
    int     frequencies[256];
    double  ret,
            freq;
    int     i;

    if(most_recurring_byte) *most_recurring_byte = 0;

    ret = 0.0;
    if(size > 0) {
        memset(frequencies, 0, sizeof(frequencies));    // memset is ok because it's "int"
        for(i = 0; i < size; i++) {
            frequencies[data[i]]++;
        }
        for(i = 0; i < 256; i++) {
            if(!frequencies[i]) continue;
            freq = (double)frequencies[i] / (double)size;
            freq *= log2(freq); // log2(1) is zero
            ret -= freq;
        }
        if(most_recurring_byte) {
            int n = 0;
            for(i = 0; i < 256; i++) {
                if(!frequencies[i]) continue;
                if(frequencies[i] > n) {
                    *most_recurring_byte = i;
                    n = frequencies[i];
                }
            }
        }
    }
    return ret;
}



u8 *get_clipboard(int *ret_size, int text_only) {
#ifdef WIN32
    HANDLE  h,
            mylock;
    int     fmt,
            size;
    u8      *ret    = NULL;

    if(ret_size) *ret_size = 0;

    if(!OpenClipboard(NULL)) goto quit;

    fmt = 0;
    if(text_only) fmt = CF_TEXT;
    fmt = EnumClipboardFormats(fmt);
    if(!fmt) goto quit;
    h = GetClipboardData(fmt);
    if(!h) goto quit;

    mylock = GlobalLock(h);
    if(!mylock) mylock = h;

    size = GlobalSize(mylock);
    if(!size) goto quit;

    ret = calloc(1, size + 1);
    if(!ret) goto quit;
    memcpy(ret, mylock, size);
    ret[size] = 0;

    if(h != mylock) GlobalUnlock(h);

    if(ret_size) *ret_size = size;
quit:
    CloseClipboard();
    return ret;
#else
    return NULL;
#endif
}



int paths_compare(u8 *path1, int path1_len, u8 *path2, int path2_len) {
    if(!path1) path1 = "";
    if(path1_len < 0) path1_len = strlen(path1);
    if(!path2) path2 = "";
    if(path2_len < 0) path2_len = strlen(path2);

    u8  *p1     = path1;
    u8  *p1l    = path1 + path1_len;
    u8  *p2     = path2;
    u8  *p2l    = path2 + path2_len;
    u8  c1      = '/';  // root
    u8  c2      = '/';  // root
    while((p1 < p1l) && (p2 < p2l)) {
        char was_delimiter1 = (c1 == '/');
        char was_delimiter2 = (c2 == '/');
        c1 = tolower(*p1++);
        c2 = tolower(*p2++);
        if((c1 == '\\') || (c1 == '/')) c1 = '/';
        if((c2 == '\\') || (c2 == '/')) c2 = '/';
        if((c1 == '/') || (c2 == '/')) {
            if(c1 == c2) continue;
            if((c1 != '/') && !was_delimiter1) break;
            if((c2 != '/') && !was_delimiter2) break;
            if(!was_delimiter1 && !was_delimiter2) {
                break;
            } else {
                if(c1 == '/') { p2--; c2 = '/'; }   // don't read from *p2
                else          { p1--; c1 = '/'; }   // don't read from *p1
                continue;
            }
        }
        if(c1 != c2) break;
    }

    if((p1 >= p1l) && (p2 >= p2l)) {    // "path" and "path"
        return 0;
    } else if(p1 >= p1l) {              // "path" and "path2" or totally different
        return -1;
    } else if(p2 >= p2l) {              // "path2" and "path" or totally different
        return 1;
    }
    return QUICKBMS_MAX_INT(int);       // just something else, better if positive
}


