/*
  by Luigi Auriemma
*/

// reversed from AIM Racing
int unrlew(u8 *in, int insz, u8 *out, int outsz) {
#define RLEW_GET16  if((i + 2) > insz) break; \
                    c = in[i];          i++; \
                    c |= (in[i] << 8);  i++;
#define RLEW_PUT16  if((o + 2) > outsz) return(-1); \
                    out[o] = c;         o++; \
                    out[o] = c >> 8;    o++;

    int     i,
            o,
            c,
            n,
            mask;

    o = 0;
    mask = in[0] << 8;
    for(i = 2; i < insz;) {
        RLEW_GET16
        if((c & 0xff00) == mask) {
            if(c == (mask | 0xff)) {
                RLEW_GET16
                RLEW_PUT16
            } else {
                n = (c & 0xff) + 3;
                RLEW_GET16
                while(n > 0) {
                    RLEW_PUT16
                    n--;
                }
            }
        } else {
            RLEW_PUT16
        }
    }
    return(o);
}
