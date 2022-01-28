// by Luigi Auriemma

#include <stdio.h>
#include <stdlib.h>
#include "mem2mem.h"

static unsigned char    *mem2mem_in_start = NULL;
static unsigned char    *mem2mem_out_start = NULL;
static unsigned char    *mem2mem_in = NULL;
static unsigned char    *mem2mem_inl = NULL;
static unsigned char    *mem2mem_out = NULL;
static unsigned char    *mem2mem_outl = NULL;

int mem2mem_init(unsigned char *in, int insz, unsigned char *out, int outsz) {
    mem2mem_in_start = in;
    mem2mem_out_start = out;
    mem2mem_in = in;
    mem2mem_inl = in + insz;
    mem2mem_out = out;
    mem2mem_outl = out + outsz;
    return 0;
}

int mem2mem_ret(void) {
    return mem2mem_out - mem2mem_out_start;
}

int mem2mem_fgetc(FILE *fd) {
    if(mem2mem_in >= mem2mem_inl) return EOF; //-1;
    int ret = *mem2mem_in++;
    return ret;
}

int mem2mem_fputc(int c, FILE *fd) {
    if(mem2mem_out >= mem2mem_outl) return EOF; //-1;
    *mem2mem_out++ = c;
    return c;
}

int mem2mem_fread(unsigned char * ptr, int size, int count, FILE * fd) {
    int     i,
            c;
    size *= count;
    for(i = 0; i < size; i++) {
        c = mem2mem_fgetc(fd);
        if(c < 0) break;
        ptr[i] = c;
    }
    return i;
}

int mem2mem_fwrite(unsigned char * ptr, int size, int count, FILE * fd) {
    int     i;
    size *= count;
    for(i = 0; i < size; i++) {
        if(mem2mem_fputc(ptr[i], fd) < 0) break;
    }
    return i;
}

int mem2mem_ftell(FILE *fd) {
    return mem2mem_in - mem2mem_in_start;
}

// unsafe
int mem2mem_fseek(FILE * fd, int offset, int origin) {
    switch(origin) {
        case SEEK_SET: mem2mem_in = mem2mem_in_start + offset; break;
        case SEEK_CUR: mem2mem_in += offset; break;
        case SEEK_END: mem2mem_in = mem2mem_inl + offset; break;
        default: break;
    }
    return 0;
}

int mem2mem_feof(FILE *fd) {
    if(mem2mem_in >= mem2mem_inl) return -1;
    return 0;
}

int mem2mem_ferror(FILE *fd) {
    if(mem2mem_in >= mem2mem_inl) return -1;
    return 0;
}

int mem2mem_fclose(FILE *fd) {
    return 0;
}

FILE *mem2mem_fopen(const char * filename, const char * mode ) {
    return mem2mem_FILE;
}
