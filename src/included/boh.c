// untested

typedef struct {
        unsigned char  **fp;
        unsigned short  buf;
        unsigned char  count;
    } unboh_bitstream;

#define unboh_getw(X) ((*X)[0] | ((*X)[1] << 8))

/* get compress information bit by bit */
void unboh_initbits(unboh_bitstream *p,unsigned char **filep){
    p->fp=filep;
    p->count=0x10;
    p->buf=unboh_getw(filep);
    /* printf("%04x ",p->buf); */
}

int unboh_getbit(unboh_bitstream *p) {
    int b;
    b = p->buf & 1;
    if(--p->count == 0){
        (p->buf)=unboh_getw(p->fp);
        /* printf("%04x ",p->buf); */
        p->count= 0x10;
    }else
        p->buf >>= 1;
    
    return b;
}

int unboh_unpack(unsigned char *ifile, int insz, unsigned char *ofile, int outsz){
    int len;
    int span;
    unboh_bitstream bits;
    static unsigned char data[0x4500], *p=data;

    unsigned char   *il = ifile + insz,
                    *ol = ofile + outsz,
                    *obackup = ofile;

    unboh_initbits(&bits,&ifile);
    for(;;){
        if(ifile >= il) return(-1);
        if(ofile >= ol) return(-1);
        if(p-data>0x4000){
            memcpy(ofile, data, 0x2000); ofile += 0x2000;
            p-=0x2000;
            memcpy(data,data+0x2000,p-data);
        }
        if(unboh_getbit(&bits)) {
            *p++=*ifile++;
            continue;
        }
        if(!unboh_getbit(&bits)) {
            len=unboh_getbit(&bits)<<1;
            len |= unboh_getbit(&bits);
            len += 2;
            span=*ifile++ | 0xff00;
        } else {
            span=(unsigned char)*ifile++;
            len=*ifile++;
            span |= ((len & ~0x07)<<5) | 0xe000;
            len = (len & 0x07)+2; 
            if (len==2) {
                len=*ifile++;

                if(len==0)
                    break;    /* end mark of compreesed load module */

                if(len==1)
                    continue; /* segment change */
                else
                    len++;
            }
        }
        for( ;len>0;len--,p++){
            *p=*(p+span);
        }
    }
    if(p!=data) {
        memcpy(ofile, data,p-data); ofile += (p - data);
    }
    return ofile - obackup;
}

