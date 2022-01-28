#include "PP20.h"

extern "C"
int unpp20(unsigned char *in, int insz, unsigned char *out, int outsz) {
    PP20    *myclass = new PP20();

    ubyte_ppt   *ret_out = NULL;
    int ret = myclass->decompress(in, insz, &ret_out);
    if(outsz < ret) ret = -1;
    if(ret > 0) {
        memcpy(out, ret_out, ret);
    }
    delete[] ret_out;
    return ret;
}

