/*
Anco Software CMP0 decompression 0.1a
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

Keywords: DecompressDataMem, DecompressDataFile, DecompressDataFile2Mem, UncompressDataMem2Mem, compress.c
*/



int anco_unpack0(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl    = in  + insz;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    static  unsigned char  dict[0x10000];

    unsigned int    d;
    int             x;
    unsigned char   flags,
                    b;

    for(x = 0; x < sizeof(dict); x++) {
        dict[x] = 0;
    }

    d = 0;
    for(;;) {

        if(in >= inl) break;
        flags = *in++;

        for(x = 0; x < 8; x++) {

            if(flags & (1 << x)) {

                b = dict[d & 0xffff];

            } else {

                if(in >= inl) break;
                b = *in++;
                dict[d & 0xffff] = b;

            }

            d = (d << 8) | b;

            if(o >= ol) return -1;
            *o++ = b;
        }
    }

    return o - out;
}



int anco_unpack1(unsigned char *in, int insz, unsigned char *out, int outsz, int skip_outsz2) {
    unsigned char   *inl    = in  + insz;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    unsigned int    dict[256],  // 16bit struct of two elements
                    ax,
                    outsz2,
                    dictsz;
    int             x;
    unsigned char   b;

    outsz2  = outsz;
    if(!skip_outsz2) {  // redundant field
        if((in + 4) > inl) return -1;
        outsz2 = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
        in += 4;
    }

    if((in + 4) > inl) return -2;
    dictsz = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
    in += 4;

    if(dictsz > 256) return -3;

    for(x = 0; x < dictsz; x++) {
        if((in + 4) > inl) return -4;
        dict[x] = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
        in += 4;
    }

    ax = 0;
    for(;;) {
        if(in >= inl) break;
        b = *in++;

        for(x = 0; x < 8; x++) {
            ax = dict[ax];
            ax = (b & (1 << x)) ? (ax >> 16) : (ax & 0xffff);
            if(ax >= 256) {
                if(outsz2 > 0) {    // useless
                    outsz2--;
                    if(o >= ol) return -5;
                    *o++ = ax;
                }
                ax = 0;
            }
        }

    }

    return o - out;
}



int anco_unpack2(unsigned char *in, int insz, unsigned char *out, int outsz, int skip_outsz2) {
    unsigned char   dict[256];
    int             i,
                    x;
    unsigned char   b,
                    c;

    outsz = anco_unpack1(in, insz, out, outsz, skip_outsz2);
    if(outsz < 0) return -1;

    for(x = 0; x < 256; x++) {
        dict[x] = x + 1;
    }

    c = 0;
    for(i = 0; i < outsz; i++) {
        b = out[i];

        if(b) {
            unsigned char  td, tc;

            td = c;
            x = 0;
            do {
                x++;
                tc = td;
                td = dict[td];
            } while(x != b);

            dict[tc] = dict[td];
            dict[td] = c;
            c        = td;
        }

        out[i] = c;
    }

    return i;
}



int anco_unpack3(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl    = in  + insz;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    unsigned char   cmp,
                    b;

    if(in >= inl) return -1;
    cmp = *in++;

    for(;;) {
        if(in >= inl) break;
        b = *in++;

        if(b == cmp) {

            if(in >= inl) break;
            b = *in++;

            if(b) {

                int len = (b >> 2);
                int pos = (b & 3) << 8;

                if(in >= inl) break;
                pos += *in++;

                if((o - pos) < out) return -2;
                if((o + len) > ol) return -3;
                while(len--) {
                    *o = o[-pos];
                    o++;
                }

            } else {

                if(o >= ol) return -4;
                *o++ = cmp;

            }

        } else {

            if(o >= ol) return -5;
            *o++ = b;

        }
    }

    return o - out;
}



int anco_unpack4(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl    = in  + insz;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    int             len;
    unsigned char   abool,
                    b;

    for(;;) {

        if(in >= inl) break;
        abool = *in++;

        if(in >= inl) break;
        b = *in++;

        if(abool) {

            *o++ = b;

        } else {

            if(b) {

                len = b;

            } else {

                if((in + 2) > inl) break;
                len = (in[0] << 8) | in[1];
                in += 2;

            }

            if((o + len) > ol) return -1;
            while(len--) {
                *o++ = 0;
            }

        }
    }

    return o - out;
}



int anco_unpack5(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *inl    = in  + insz;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    unsigned char   cmp,
                    b;

    if(in >= inl) return -1;
    cmp = *in++;

    for(;;) {
        if(in >= inl) break;
        b = *in++;

        if(b == cmp) {

            if(in >= inl) break;
            b = *in++;

            if(b) {

                if(in >= inl) break;
                int len = *in++;

                if(!len) {
                    if((in + 2) > inl) break;
                    len = (in[0] << 8) | in[1];
                    in += 2;
                }

                if((o + len) > ol) return -2;
                while(len--) {
                    *o++ = b;
                }

            } else {

                if(o >= ol) return -3;
                *o++ = cmp;

            }

        } else {

            if(o >= ol) return -4;
            *o++ = b;

        }
    }

    return o - out;
}



int anco_unpack(unsigned char *in, int insz, unsigned char *out, int outsz) {
    if(insz < 0x10) return -1;

    if(*in++ != 'C') return -2;
    if(*in++ != 'M') return -2;
    if(*in++ != 'P') return -2;
    if(*in++ != '0') return -2;

    int     algo  = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
    in += 4;

    int     zsize = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
    in += 4;

    int     size  = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
    in += 4;

    insz -= 0x10;
    if((zsize < 0) || (zsize > insz)) return -3;
    insz  = zsize;

    if((size  < 0) || (size  > outsz)) return -4;
    outsz = size;

    int     ret;
    switch(algo) {
        case 0: ret = anco_unpack0(in, insz, out, outsz);       break;
        case 1: ret = anco_unpack1(in, insz, out, outsz, 0);    break;
        case 2: ret = anco_unpack2(in, insz, out, outsz, 0);    break;
        case 3: ret = anco_unpack3(in, insz, out, outsz);       break;
        case 4: ret = anco_unpack4(in, insz, out, outsz);       break;
        case 5: ret = anco_unpack5(in, insz, out, outsz);       break;
        default: ret = -5;                                      break;
    }

    return ret;
}


