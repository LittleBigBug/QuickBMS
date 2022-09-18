/*
the idea was to have a complete replacement of file functions with memory alternatives
but it's currently not allowed because once "added" it's impossible to replace a symbol
so the following is kept for a possible future usage...
*/

/*
//skeleton functions
__cdecl FILE *  QUICKBMS_fopen (const char *pathname, const char *mode) { return NULL; }
__cdecl FILE *  QUICKBMS_freopen (const char *pathname, const char *mode, FILE *fd) { return NULL; }
__cdecl int     QUICKBMS_fflush (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_fclose (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_remove (const char *filename) { return 0; }
__cdecl int     QUICKBMS_rename (const char *oldname, const char *newname) { return 0; }
__cdecl FILE *  QUICKBMS_tmpfile (void) { return NULL; }
__cdecl char *  QUICKBMS_tmpnam (char *str) { return NULL; }
__cdecl char *  QUICKBMS_tempnam (const char *dir, const char *pfx) { return NULL; }
__cdecl int     QUICKBMS_rmtmp (void) { return 0; }
__cdecl int     QUICKBMS_unlink (const char *filename) { return 0; }
__cdecl int     QUICKBMS_setvbuf (FILE *fd, char *buf, int mode, size_t size) { return 0; }
__cdecl void    QUICKBMS_setbuf (FILE *fd, char *str) { }
__cdecl int     QUICKBMS_fprintf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_printf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_sprintf (char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vfprintf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vprintf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsprintf (char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_snprintf (char *buf, size_t size, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsnprintf (char *buf, size_t size, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vscanf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fscanf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_scanf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_sscanf (const char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fgetc (FILE *fd) { return 0; }
__cdecl char *  QUICKBMS_fgets (char *buf, int size, FILE *fd) { return NULL; }
__cdecl int     QUICKBMS_fputc (int c, FILE *fd) { return 0; }
__cdecl int     QUICKBMS_fputs (const char *buf, FILE *fd) { return 0; }
__cdecl char *  QUICKBMS_gets (char *buf) { return NULL; }
__cdecl int     QUICKBMS_puts (const char *buf) { return 0; }
__cdecl int     QUICKBMS_ungetc (int c, FILE *fd) { return 0; }
__cdecl int     QUICKBMS_getc (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_putc (int c, FILE *fd) { return 0; }
__cdecl int     QUICKBMS_getchar (void) { return 0; }
__cdecl int     QUICKBMS_putchar (int c) { return 0; }
__cdecl size_t  QUICKBMS_fread (void *ptr, size_t size, size_t count, FILE *fd) { return 0; }
__cdecl size_t  QUICKBMS_fwrite (const void *ptr, size_t size, size_t count, FILE *fd) { return 0; }
__cdecl int     QUICKBMS_fseek (FILE *fd, long offset, int origin) { return 0; }
__cdecl long    QUICKBMS_ftell (FILE *fd) { return 0; }
__cdecl void    QUICKBMS_rewind (FILE *fd) { }
__cdecl int     QUICKBMS_fgetpos (FILE *fd, long *offset) { return 0; }
__cdecl int     QUICKBMS_fsetpos (FILE *fd, const long *offset) { return 0; }
__cdecl int     QUICKBMS_feof (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_ferror (FILE *fd) { return 0; }
__cdecl void    QUICKBMS_clearerr (FILE *fd) { }
__cdecl void    QUICKBMS_perror (const char *s) { }
__cdecl int     QUICKBMS_fgetchar (void) { return 0; }
__cdecl int     QUICKBMS_fputchar (int c) { return 0; }
__cdecl FILE *  QUICKBMS_fdopen (int fd, const char *mode) { return NULL; }
__cdecl int     QUICKBMS_fileno (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_fwprintf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_wprintf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vfwprintf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vwprintf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fwscanf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_wscanf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_swscanf (const wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl wint_t  QUICKBMS_fgetwc (FILE *fd) { return 0; }
__cdecl wint_t  QUICKBMS_fputwc (wchar_t c, FILE *fd) { return 0; }
__cdecl wint_t  QUICKBMS_ungetwc (wchar_t c, FILE *fd) { return 0; }
__cdecl int     QUICKBMS_swprintf (wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vswprintf (wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl wchar_t*QUICKBMS_fgetws (wchar_t *buf, int size, FILE *fd) { return NULL; }
__cdecl int     QUICKBMS_fputws (const wchar_t *buf, FILE *fd) { return 0; }
__cdecl wint_t  QUICKBMS_getwc (FILE *fd) { return 0; }
__cdecl wint_t  QUICKBMS_getwchar (void) { return 0; }
__cdecl wint_t  QUICKBMS_putwc (wint_t c, FILE *fd) { return 0; }
__cdecl wint_t  QUICKBMS_putwchar (wint_t c) { return 0; }
__cdecl int     QUICKBMS_snwprintf (wchar_t *buf, size_t size, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsnwprintf (wchar_t *buf, size_t size, const wchar_t *fmt, ...) { return 0; }
__cdecl FILE *  QUICKBMS_wpopen (const wchar_t *pathname, const wchar_t *mode) { return NULL; }
__cdecl wint_t  QUICKBMS_fgetwchar (void) { return 0; }
__cdecl wint_t  QUICKBMS_fputwchar (wint_t c) { return 0; }
__cdecl int     QUICKBMS_getw (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_putw (int c, FILE *fd) { return 0; }
*/



// copied from extra/mem2mem.c since not easy to use here
static unsigned char    *QUICKBMS_mem_in_start = NULL;
static unsigned char    *QUICKBMS_mem_out_start = NULL;
static unsigned char    *QUICKBMS_mem_in = NULL;
static unsigned char    *QUICKBMS_mem_inl = NULL;
static unsigned char    *QUICKBMS_mem_out = NULL;
static unsigned char    *QUICKBMS_mem_outl = NULL;

int QUICKBMS_mem_init(unsigned char *in, int insz, unsigned char *out, int outsz) {
    QUICKBMS_mem_in_start   = in;
    QUICKBMS_mem_out_start  = out;
    QUICKBMS_mem_in         = QUICKBMS_mem_in_start;
    QUICKBMS_mem_inl        = NULL;
    if(in  && (insz  >= 0)) QUICKBMS_mem_inl =  QUICKBMS_mem_in  + insz;
    QUICKBMS_mem_out        = QUICKBMS_mem_out_start;
    QUICKBMS_mem_outl       = NULL;
    if(out && (outsz >= 0)) QUICKBMS_mem_outl = QUICKBMS_mem_out + outsz;
    return 0;
}



u8 *QUICKBMS_FILE_to_mem(FILE *fd) {
    if(!fd) return NULL;

    u8  **mem   = (void *)fd;
    u8  *ret    = NULL;

    if(fd == stdin) {
        ret = QUICKBMS_mem_in;
    } else if(fd == stdout) {
        ret = QUICKBMS_mem_out;
    } else if(fd == stderr) {
        // ??? ret = QUICKBMS_mem_out;
    } else if(mem) {
        // ret = *mem; is the same
        if(*mem == QUICKBMS_mem_in) {
            ret = QUICKBMS_mem_in;
        } else if(*mem == QUICKBMS_mem_out) {
            ret = QUICKBMS_mem_out;
        } else {
            ret = *mem;
        }
    } else {
        if(QUICKBMS_mem_in) {
            ret = QUICKBMS_mem_in;
        } else if(QUICKBMS_mem_out) {
            ret = QUICKBMS_mem_out;
        } else {
            // ???
        }
    }
    return ret;
}

void QUICKBMS_mem_to_FILE(FILE *fd, u8 *buff) {
    if(QUICKBMS_mem_in  && ((buff >= QUICKBMS_mem_in ) && (buff <= QUICKBMS_mem_inl ))) QUICKBMS_mem_in  = buff;
    if(QUICKBMS_mem_out && ((buff >= QUICKBMS_mem_out) && (buff <= QUICKBMS_mem_outl))) QUICKBMS_mem_out = buff;
    if(!fd) return;
    if((fd == stdin) || (fd == stdout) || (fd == stderr)) {
        /* do nothing */
    } else {
        u8      **mem = (u8 **)fd;
        *mem = buff;
    }
}



__cdecl int     _QUICKBMS_feof (FILE *fd) {
    u8      *buff = QUICKBMS_FILE_to_mem(fd);
    if(!buff) return 0;
    if((buff == QUICKBMS_mem_in)  && (buff >= QUICKBMS_mem_inl )) return 1;
    if((buff == QUICKBMS_mem_out) && (buff >= QUICKBMS_mem_outl)) return 1;
    return 0;
}

__cdecl int     _QUICKBMS_fgetc (FILE *fd) {
    u8      *buff = QUICKBMS_FILE_to_mem(fd);
    if(!buff) return -1;
    if((buff == QUICKBMS_mem_in)  && (buff >= QUICKBMS_mem_inl )) return -1;
    if((buff == QUICKBMS_mem_out) && (buff >= QUICKBMS_mem_outl)) return -1;
    int     ret = buff[0];
    buff++;
    QUICKBMS_mem_to_FILE(fd, buff);
    return ret;
}

__cdecl int     _QUICKBMS_fputc (int c, FILE *fd) {
    u8      *buff = QUICKBMS_FILE_to_mem(fd);
    if(!buff) return -1;
    if((buff == QUICKBMS_mem_in)  && (buff >= QUICKBMS_mem_inl )) return -1;
    if((buff == QUICKBMS_mem_out) && (buff >= QUICKBMS_mem_outl)) return -1;
    buff[0] = c;
    buff++;
    QUICKBMS_mem_to_FILE(fd, buff);
    return c;
}

__cdecl char *  _QUICKBMS_fgets (char *buf, int size, FILE *fd) {
    int c, i;
    if(size > 0) buf[0] = 0;
    for(i = 0;;) {
        if(size >= 0) {
            if(i >= size) break;
        }
        c = _QUICKBMS_fgetc(fd ? fd : stdin);
        if(c < 0) return NULL;
        if(c == '\r') continue;
        if(c == '\n') break;
        if(!c) break;
        buf[i++] = c;
    }
    buf[i] = 0;
    return buf;
}

__cdecl int     _QUICKBMS_fputs (const char *buf, FILE *fd) {
    int c, i;
    for(i = 0; (c = buf[i]); i++) {
        c = _QUICKBMS_fputc(c, fd ? fd : stdin);
        if(c < 0) return -1;
    }
    return i;
}

__cdecl size_t  _QUICKBMS_fread (void *ptr, size_t size, size_t count, FILE *fd) {
    u8  *buf = (u8*)ptr;
    int c, i;
    size *= count;
    for(i = 0; i < size; i++) {
        c = _QUICKBMS_fgetc(fd ? fd : stdin);
        if(c < 0) return -1;
        buf[i] = c;
    }
    return i;
}

__cdecl size_t  _QUICKBMS_fwrite (const void *ptr, size_t size, size_t count, FILE *fd) {
    u8  *buf = (u8*)ptr;
    int c, i;
    size *= count;
    for(i = 0; i < size; i++) {
        c = buf[i];
        c = _QUICKBMS_fputc(c, fd ? fd : stdout);
        if(c < 0) return -1;
    }
    return i;
}

__cdecl int     _QUICKBMS_fseek (FILE *fd, long offset, int origin) {
    u8      *buff = QUICKBMS_FILE_to_mem(fd);
    if(!buff) return -1;
    u8      *limit = NULL;
    if(buff == QUICKBMS_mem_in)  limit = QUICKBMS_mem_inl;
    if(buff == QUICKBMS_mem_out) limit = QUICKBMS_mem_outl;
    u8      *start = buff;
    if(buff == QUICKBMS_mem_in)  start = QUICKBMS_mem_in_start;
    if(buff == QUICKBMS_mem_out) start = QUICKBMS_mem_out_start;

    switch(origin) {
        case SEEK_SET: buff = start + offset; break;
        case SEEK_CUR: buff += offset; break;
        case SEEK_END: if(limit) buff = limit + offset; break;
        default: break;
    }
    if(buff < start) buff = start;
    if(limit) {
        if(buff > limit) buff = limit;
    }
    QUICKBMS_mem_to_FILE(fd, buff);
    return 0;
}

__cdecl long    _QUICKBMS_ftell (FILE *fd) {
    u8      *buff = QUICKBMS_FILE_to_mem(fd);
    if(!buff) return -1;
    if(buff == QUICKBMS_mem_in)  return QUICKBMS_mem_in  - QUICKBMS_mem_in_start;
    if(buff == QUICKBMS_mem_out) return QUICKBMS_mem_out - QUICKBMS_mem_out_start;
    return 0;
}



// wint_t has been replaced with size_t to avoid compilation problems on some platforms (wchar.h may not be available?)
__cdecl FILE *  QUICKBMS_fopen (const char *pathname, const char *mode)             { return (FILE*)(mystrchrs((u8*)mode, "wa+") ? &QUICKBMS_mem_out       : &QUICKBMS_mem_in); }
__cdecl FILE *  QUICKBMS_freopen (const char *pathname, const char *mode, FILE *fd) { return (FILE*)(mystrchrs((u8*)mode, "wa+") ? &QUICKBMS_mem_out_start : &QUICKBMS_mem_in_start); }
__cdecl int     QUICKBMS_fflush (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_fclose (FILE *fd) { return 0; }
__cdecl int     QUICKBMS_remove (const char *filename) { return 0; }
__cdecl int     QUICKBMS_rename (const char *oldname, const char *newname) { return 0; }
__cdecl FILE *  QUICKBMS_tmpfile (void) { return NULL; }
__cdecl char *  QUICKBMS_tmpnam (char *str) { return NULL; }
__cdecl char *  QUICKBMS_tempnam (const char *dir, const char *pfx) { return NULL; }
__cdecl int     QUICKBMS_rmtmp (void) { return 0; }
__cdecl int     QUICKBMS_unlink (const char *filename) { return 0; }
__cdecl int     QUICKBMS_setvbuf (FILE *fd, char *buf, int mode, size_t size) { return 0; }
__cdecl void    QUICKBMS_setbuf (FILE *fd, char *str) { }
__cdecl int     QUICKBMS_fprintf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_printf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_sprintf (char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vfprintf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vprintf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsprintf (char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_snprintf (char *buf, size_t size, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsnprintf (char *buf, size_t size, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vscanf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fscanf (FILE *fd, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_scanf (const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_sscanf (const char *buf, const char *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fgetc (FILE *fd) { return _QUICKBMS_fgetc(fd); }
__cdecl char *  QUICKBMS_fgets (char *buf, int size, FILE *fd) { return _QUICKBMS_fgets(buf, size, fd); }
__cdecl int     QUICKBMS_fputc (int c, FILE *fd) { return _QUICKBMS_fputc(c, fd); }
__cdecl int     QUICKBMS_fputs (const char *buf, FILE *fd) { return _QUICKBMS_fputs(buf, fd); }
__cdecl char *  QUICKBMS_gets (char *buf) { return QUICKBMS_fgets(buf, -1, NULL); }
__cdecl int     QUICKBMS_puts (const char *buf) { return QUICKBMS_fputs(buf, stdout); }
__cdecl int     QUICKBMS_ungetc (int c, FILE *fd) { _QUICKBMS_fseek(fd, -1, SEEK_CUR); return QUICKBMS_fputc(c, fd); }
__cdecl int     QUICKBMS_getc (FILE *fd) { return QUICKBMS_fgetc(fd); }
__cdecl int     QUICKBMS_putc (int c, FILE *fd) { return QUICKBMS_fputc(c, fd); }
__cdecl int     QUICKBMS_getchar (void) { return QUICKBMS_fgetc(stdin); }
__cdecl int     QUICKBMS_putchar (int c) { return QUICKBMS_fputc(c, stdout); }
__cdecl size_t  QUICKBMS_fread (void *ptr, size_t size, size_t count, FILE *fd)        { return _QUICKBMS_fread (ptr, size, count, fd); }
__cdecl size_t  QUICKBMS_fwrite (const void *ptr, size_t size, size_t count, FILE *fd) { return _QUICKBMS_fwrite(ptr, size, count, fd); }
__cdecl int     QUICKBMS_fseek (FILE *fd, long offset, int origin) { return _QUICKBMS_fseek(fd, offset, origin); }
__cdecl long    QUICKBMS_ftell (FILE *fd) { return _QUICKBMS_ftell(fd); }
__cdecl void    QUICKBMS_rewind (FILE *fd) { QUICKBMS_fseek(fd, 0, SEEK_SET); }
__cdecl int     QUICKBMS_fgetpos (FILE *fd, long *offset) { if(offset) *offset = QUICKBMS_ftell(fd); return 0; }
__cdecl int     QUICKBMS_fsetpos (FILE *fd, const long *offset) { return QUICKBMS_fseek(fd, *offset, SEEK_SET); }
__cdecl int     QUICKBMS_feof (FILE *fd) { return _QUICKBMS_feof(fd); }
__cdecl int     QUICKBMS_ferror (FILE *fd) { return 0; }
__cdecl void    QUICKBMS_clearerr (FILE *fd) { }
__cdecl void    QUICKBMS_perror (const char *s) { }
__cdecl int     QUICKBMS_fgetchar (void) { return QUICKBMS_fgetc(stdin); }
__cdecl int     QUICKBMS_fputchar (int c) { return QUICKBMS_fputc(c, stdout); }
__cdecl FILE *  QUICKBMS_fdopen (int fd, const char *mode) { if(fd == fileno(stdin)) return stdin; if(fd == fileno(stdout)) return stdout; if(fd == fileno(stderr)) return stderr; return QUICKBMS_fopen("", mode); }
__cdecl int     QUICKBMS_fileno (FILE *fd) { return fileno(stdin); }
__cdecl int     QUICKBMS_fwprintf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_wprintf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vfwprintf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vwprintf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_fwscanf (FILE *fd, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_wscanf (const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_swscanf (const wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl size_t  QUICKBMS_fgetwc (FILE *fd) { return (QUICKBMS_fgetc(fd) | (QUICKBMS_fgetc(fd) << 8)); }
__cdecl size_t  QUICKBMS_fputwc (wchar_t c, FILE *fd) { if(QUICKBMS_fputc(c & 0xff, fd) < 0) return -1; if(QUICKBMS_fputc((c >> 8) & 0xff, fd) < 0) return -1; return c; }
__cdecl size_t  QUICKBMS_ungetwc (wchar_t c, FILE *fd) { QUICKBMS_fseek(fd, -2, SEEK_CUR); return QUICKBMS_fputwc(c, fd); }
__cdecl int     QUICKBMS_swprintf (wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vswprintf (wchar_t *buf, const wchar_t *fmt, ...) { return 0; }
__cdecl wchar_t*QUICKBMS_fgetws (wchar_t *buf, int size, FILE *fd) { int c,i; for(i = 0; i < size; i++) { c = QUICKBMS_fgetwc(fd); if(c < 0) return NULL; if(!c) break; } return buf; }
__cdecl int     QUICKBMS_fputws (const wchar_t *buf, FILE *fd) { int c,i; for(i = 0; (c = buf[i]); i++) { c = QUICKBMS_fputwc(c, fd); if(c < 0) return -1; } return i; }
__cdecl size_t  QUICKBMS_getwc (FILE *fd) { return QUICKBMS_fgetwc(fd); }
__cdecl size_t  QUICKBMS_getwchar (void) { return QUICKBMS_fgetwc(stdin); }
__cdecl size_t  QUICKBMS_putwc (size_t c, FILE *fd) { return QUICKBMS_fputwc(c, fd); }
__cdecl size_t  QUICKBMS_putwchar (size_t c) { return QUICKBMS_fputwc(c, stdout); }
__cdecl int     QUICKBMS_snwprintf (wchar_t *buf, size_t size, const wchar_t *fmt, ...) { return 0; }
__cdecl int     QUICKBMS_vsnwprintf (wchar_t *buf, size_t size, const wchar_t *fmt, ...) { return 0; }
__cdecl FILE *  QUICKBMS_wpopen (const wchar_t *pathname, const wchar_t *mode) { return QUICKBMS_fopen("", "rb"); } // wrong, who cares
__cdecl size_t  QUICKBMS_fgetwchar (void) { return QUICKBMS_getwchar(); }
__cdecl size_t  QUICKBMS_fputwchar (size_t c) { return QUICKBMS_putwchar(c); }
__cdecl int     QUICKBMS_getw (FILE *fd) { return QUICKBMS_fgetwc(fd); }
__cdecl int     QUICKBMS_putw (int c, FILE *fd) { return QUICKBMS_fputwc(c, fd); }

