// by Luigi Auriemma

#ifndef MEM2MEM_H
#define MEM2MEM_H

#define mem2mem_FILE    ((void *)-0x80000000)

#ifndef MEM2MEM_NO_WRAP
#define fgetc   mem2mem_fgetc
#define fputc   mem2mem_fputc
#define fread   mem2mem_fread
#define fwrite  mem2mem_fwrite
#define ftell   mem2mem_ftell
#define fseek   mem2mem_fseek
#undef feof
#define feof    mem2mem_feof
#undef ferror
#define ferror  mem2mem_ferror
#define fclose  mem2mem_fclose
#define fopen   mem2mem_fopen
#define fflush(X)

#undef getc
#define getc    fgetc
#undef putc
#define putc    fputc
#endif  // MEM2MEM_NO_WRAP

#ifdef __cplusplus
    #define mem2mem_extern  extern "C"
#else
    #define mem2mem_extern
#endif

mem2mem_extern int mem2mem_fgetc(FILE *fd);
mem2mem_extern int mem2mem_fputc(int c, FILE *fd);
mem2mem_extern int mem2mem_fread(unsigned char * ptr, int size, int count, FILE * fd);
mem2mem_extern int mem2mem_fwrite(unsigned char * ptr, int size, int count, FILE * fd);
mem2mem_extern int mem2mem_ftell(FILE *fd);
mem2mem_extern int mem2mem_fseek(FILE * fd, int offset, int origin);
mem2mem_extern int mem2mem_feof(FILE *fd);
mem2mem_extern int mem2mem_ferror(FILE *fd);
mem2mem_extern int mem2mem_fclose(FILE *fd);
mem2mem_extern FILE *mem2mem_fopen(const char * filename, const char * mode );

mem2mem_extern int mem2mem_init(unsigned char *in, int insz, unsigned char *out, int outsz);
mem2mem_extern int mem2mem_ret(void);

#endif
