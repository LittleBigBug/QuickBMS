/*
  by Luigi Auriemma

reversed from asmodean's unrrbpe.exe
*/

#include <string.h>

static int xgetc(unsigned char **in, unsigned char *inl) {
    int     ret;
    if(*in >= inl) return(-1);
    ret = **in;
    (*in)++;
    return(ret);
}

int yuke_bpe(unsigned char *in, int insz, unsigned char *out, int outsz, int fill_outsz) {
    unsigned char   stack[512 + 4096];
    int             c,
                    count,
                    i,
                    size,
                    n;

    unsigned char   *inl,
                    *o,
                    *outl;

    inl  = in + insz;
    o    = out;
    outl = out + outsz;

    count = 0;
    for(;;) {
        i = 0;
        do {
            if((c = xgetc(&in, inl)) < 0) break;
            if(c > 127) {
                c -= 127;
                while((c > 0) && (i < 256)) {
                    stack[i * 2] = i;
                    c--;
                    i++;
                }
            }
            c++;
            while((c > 0) && (i < 256)) {
                if((n = xgetc(&in, inl)) < 0) break;
                stack[i * 2] = n;
                if(i != n) {
                    if((n = xgetc(&in, inl)) < 0) break;
                    stack[(i * 2) + 1] = n;
                }
                c--;
                i++;
            }
        } while(i < 256);

        if((n = xgetc(&in, inl)) < 0) break;
        size = n;
        if((n = xgetc(&in, inl)) < 0) break;
        size |= (n << 8);

        while(size || count) {
            if(count) {
                count--;
                n = stack[count + 512];
            } else {
                if((n = xgetc(&in, inl)) < 0) break;
                size--;
            }
            c = stack[n * 2];
            if(n == c) {
                if(o >= outl) return(-1);
                *o++ = n;
            } else {
                if((count + 512 + 2) > sizeof(stack)) return(-1);
                stack[count + 512] = stack[(n * 2) + 1];
                stack[count + 512 + 1] = c;
                count += 2;
            }
        }
    }
    if(fill_outsz) {    // this is what is wanted by the format
        memset(o, 0, outl - o);
        o = outl;
    }
    return(o - out);
}

