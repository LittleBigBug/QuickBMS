#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int unlzssx(unsigned char *src, int srclen, unsigned char *dst, int dstlen, u8 *parameters) {
    int EI = 12;    /* typically 10..13 */
    int EJ = 4;     /* typically 4..5 */
    int P  = 2;     /* If match length <= P then output one character */
    int N;
    int F;
    int rless = P;  // in some rare implementations it could be 0
    int init_chr = ' ';

    static int slide_winsz = 0;
    static unsigned char *slide_win = NULL;
    unsigned char *dststart = dst;
    unsigned char *srcend = src + srclen;
    unsigned char *dstend = dst + dstlen;
    int  length, offset, k, r, c;
    unsigned flags;

    if(parameters) {
        //sscanf(parameters, "%d %d %d %d %d",
        get_parameter_numbers(parameters,
            &EI, &EJ, &P, &rless, &init_chr, NULL);
    }
    N = 1 << EI;
    F = 1 << EJ;

    if(N > slide_winsz) {
        slide_win   = realloc(slide_win, N);
        if(!slide_win) return -1;
        slide_winsz = N;
    }
    lzss_set_window(slide_win, N, init_chr);

    dst = dststart;
    srcend = src + srclen;
    r = (N - F) - rless;
    N--;
    F--;

    for(flags = 0;; flags >>= 1) {
        if(!(flags & 0x100)) {
            if(src >= srcend) break;
            flags = *src++;
            flags |= 0xff00;
        }
        if(flags & 1) {
            if(src >= srcend) break;
            c = *src++;
            if(dst >= dstend) goto quit; //return -1; better?
            *dst++ = c;
            slide_win[r] = c;
            r = (r + 1) & N;
        } else {
            if(src >= srcend) break;
            length = *src++;
            if(src >= srcend) break;
            offset = *src++;
            offset = r - ((offset << EJ) | (length >> EJ));
            length = (length & F) + P;
            for(k = 0; k <= length; k++) {
                c = slide_win[(offset + k) & N];
                if(dst >= dstend) goto quit; //return -1; better?
                *dst++ = c;
                slide_win[r] = c;
                r = (r + 1) & N;
            }
        }
    }
quit:
    return(dst - dststart);
}
