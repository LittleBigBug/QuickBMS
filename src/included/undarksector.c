/*

undarksector 0.1a
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

reversed from Dark Sector where it's used in ZIP files as type 64
*/

int undarksector(unsigned char *in, int insz, unsigned char *out, int outsz, int use_chunks) {
    unsigned        i,
                    c,
                    n;
    unsigned char   *il,
                    *o,
                    *ol,
                    *p,
                    *next = NULL;

    il = in + insz;
    o  = out;
    ol = out + outsz;

    for(;;) {
        if(use_chunks && (in >= next)) {
            if((in + 4 + 4) > il) break;
            next = in + 4 + 4 + ((in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3]));
            in += 4 + 4;    // output chunk not calculated (default should be 0x4000)
        }
        if(in >= il) break;
        c = *in++;
        if(c < 0x20) {
            c++;
            if((in + c) > il) break;
            if((o + c) > ol) return(-1);
            for(i = 0; i < c; i++) {
                *o++ = *in++;
            }
        } else {
            n = c >> 5;
            if(n == 7) {
                if(in >= il) break;
                n = (*in++) + 7;
            }
            n += 2;
            if(in >= il) break;
            p = ((o - ((c & 0x1f) << 8)) - 1) - (*in++);
            if(p < out) break;
            if((o + n) > ol) return(-1);
            for(i = 0; i < n; i++) {
                *o++ = *p++;
            }
        }
    }
    return(o - out);
}


