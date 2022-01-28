/*
ULZ decompression 0.1a
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

reverse engineered from ULZ 0.06 created by Ilya Muravyov:
http://compressme.net/#downloads
*/

int ulz_decompress(unsigned char *in, int insz, unsigned char *out, int outsz, int check_magic) {
    unsigned char   *inl = in + insz,
                    *o   = out,
                    *ol  = out + outsz;

    static const int    chunk_size  = 0x1000000;

    int     t,
            b,
            d,
            shift;

    if(check_magic && (insz >= 8) && (in[0] == 'U') && (in[1] == 'L') && (in[2] == 'Z') && (in[3] < 0x10)) { // "ULZ\x06"

        in += 4;
        while(in < inl) {
            t = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
            in += 4;
            if((t < 0) || (t > insz)) return -1;
            if(t > (chunk_size + 0x10)) return -1;
            if(t >= chunk_size) {
                while(t--) {
                    *o++ = *in++;
                }
            } else {
                d = ulz_decompress(in, t, o, outsz - (o - out), 0);
                if(d < 0) return -1;
                in += t;
                o  += d;
            }
        }

    } else {

        while(in < inl) {
            b = *in++;
            if(b >= 0x20) {
                t = b >> 5;
                if(t == 7) {
                    for(shift = 0; shift <= 0x15; shift += 7) {
                        if(in >= inl) return -1;
                        d = *in++;
                        t += (d << shift);
                        if(d < 0x80) break;
                    }
                }
                if((in + t) > inl) return -1;
                if((o + t) > ol)  return -1;
                while(t--) {
                    *o++ = *in++;
                }
            }
            if(in >= inl) break;    // this check is mandatory!
            if((in + 2) > inl) return -1;
            d = in[0] | (in[1] << 8);
            in += 2;
            if(b & 0x10) {
                if(in >= inl) return -1;
                d |= (*in++ << 16);
            }
            b &= 0xf;
            if(b == 0xf) {
                if(in >= inl) return -1;
                b = *in++ + 0xf;
            }
            b += 4;
            if((o - d) < out) return -1;
            if((o + b) > ol)  return -1;
            while(b--) {
                *o = o[-d];
                o++;
            }
        }

    }

    return o - out;
}


