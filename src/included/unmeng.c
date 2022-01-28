/*
  by Luigi Auriemma
*/

// reversed from DreamKiller (Mindware engine)
// unpack resource, there are no other details
int unmeng(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned        a;
    unsigned char   *o,
                    *outl,
                    *inl,
                    *slide;

    inl  = in + insz;
    o    = out;
    outl = out + outsz;

    while((in < inl) && (o < outl)) {
        a = in[0];
        if(a < 0x80) {
            slide = (o - (a & 0x0f)) - 1;
            if(slide < out) return(-1);
            a = (a >> 4) + 3;
            in++;
        } else if((a & 0xe0) == 0x80) {
            a = (a & 0x1F) + 1;
            in++;
            slide = in;
            in += a;
        } else if((a & 0xc0) == 0xc0) {
            a = (a & 0x3F) + 4;
            slide = (o - in[1]) - 1;
            if(slide < out) return(-1);
            in += 2;
        } else if((a & 0xf0) == 0xa0) {
            a = (((a & 0x0F) << 8) | in[1]) + 32;
            in += 2;
            slide = in;
            in += a;
        } else {
            a = (((a & 0x0F) << 8) | in[1]) + 6;
            slide = (o - ((in[2] << 8) | in[3])) - 1;
            if(slide < out) return(-1);
            in += 4;
        }
        while(a--) {
            if(o >= outl) break;    // the algorithm is out-size based, so no return(-1)
            *o++ = *slide++;
        }
    }
    return(o - out);
}

