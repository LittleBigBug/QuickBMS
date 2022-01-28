/*
old Bizarre Creations decompression 0.1
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org

compression algorithm used by old games of Bizarre Creations
*/

int old_bizarre(unsigned char *in, int insz, unsigned char *out, int outsz, int skip_size) {
    int     t,
            back,
            amount,
            flags;
    unsigned char   *inl = in + insz,
                    *o = out,
                    *ol;

    if(!skip_size) {
        t = (in[3] << 24) | (in[2] << 16) | (in[1] << 8) | in[0];  // in the real implementation in[3] is ignored
        in += 4;
        if(t > outsz) return -1;
        outsz = t;
    }
    ol = out + outsz;
    for(;;) {
        if((in + 4) > inl) break;
        flags = (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | in[3];
        in += 4;
        int esp4 = 0xe    -  (flags & 3);
        int esp8 = 0x3fff >> (flags & 3);
        int ebp;
        for(ebp = 0; ebp < 0x1e; ebp++) {
            if(flags < 0) {
                if((in + 2) > inl) break;
                t = (in[0] << 8) | in[1];;
                in += 2;
                back   = (t & esp8) + 1;
                amount = (t >> esp4) + 3;
                if((o + amount) > ol) break;
                if((o - back) < out) break;
                while(amount--) {
                    *o = *(o - back);
                    o++;
                }
            } else {
                if((o + 1) > ol) break;
                if((in + 1) > inl) break;
                *o++ = *in++;
            }
            flags += flags; // << 1 is sign dependent
        }
    }
    return o - out;
}
