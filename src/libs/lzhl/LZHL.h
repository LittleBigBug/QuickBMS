#include <stddef.h>

void *initComp();
void delComp(void *comp);
size_t compress(void *comp, unsigned char *buf, size_t size, unsigned char *ret);
void *initDecomp();
void delDecomp(void *decomp);
size_t decompress(void *decomp, unsigned char *buf, size_t size, unsigned char *ret, size_t retsize);
