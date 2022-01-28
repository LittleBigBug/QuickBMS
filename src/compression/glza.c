#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "../extra/stdatomic.h"

int glza_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
#if defined(__GNUC__) && !defined(__clang__)
    #include "../libs/glza/GLZAdecode.c"
    size_t  ret_out = 0;
    if(!GLZAdecode(insz, in, &ret_out, out, NULL)) return -1;
    return ret_out;
#else
    return -1;
#endif
}

