
/* Dicky - public domain */

#ifndef __DICKY_H__
#define __DICKY_H__ 1

int dicky_compress(unsigned char ** const target, size_t * const target_size,
                   const char * const source, const size_t source_size);

int dicky_uncompress(char ** const target, size_t * const target_size,
                     const unsigned char * const source,
                     const size_t source_size);

void dicky_free(void * const buffer);

#endif

