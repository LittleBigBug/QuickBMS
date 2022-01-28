/*
by Luigi Auriemma
C version of the huffman compression used in Neptunia from my old dump of the function.
*/

typedef struct {
    unsigned char   *in;
    unsigned char   *inl;
    unsigned char   flags;
    unsigned char   fbits;
    unsigned short  idx;
    unsigned short  tree1[2048];    // was 1024
    unsigned short  tree2[2048];
} neptunia_context;



int neptunia_check_flags(neptunia_context *ctx) {
    if(!ctx->fbits) {
        ctx->fbits = 8;
        if(ctx->in >= ctx->inl) return 0;
        ctx->flags = *ctx->in++;
    }
    ctx->fbits--;
    return (1 << ctx->fbits) & ctx->flags;
}



int neptunia_tree(neptunia_context *ctx) {
    int     ret;
    if(neptunia_check_flags(ctx)) {
        ret = ctx->idx;
        ctx->idx++;
        if(ret >= (sizeof(ctx->tree1)/sizeof(ctx->tree1[0]))) return -1;
        ctx->tree1[ret] = neptunia_tree(ctx);
        ctx->tree2[ret] = neptunia_tree(ctx);
    } else {
        unsigned char   old_flags   = ctx->flags;
        if(ctx->in >= ctx->inl) return 0;
        ctx->flags = *ctx->in++;
        ret = (((1 << (8 - ctx->fbits)) - 1) & (ctx->flags >> ctx->fbits)) | ((((1 << ctx->fbits) - 1) & old_flags) << (8 - ctx->fbits));
    }
    return ret;
}



int neptunia_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    int     code,
            i;

    neptunia_context    ctx;
    ctx.in      = in;
    ctx.inl     = in + insz;
    ctx.flags   = 0;
    ctx.fbits   = 0;
    ctx.idx     = 256;

    code = neptunia_tree(&ctx);
    if(code < 0) return -1;

    for(i = 0; i < outsz; i++) {
        int t = code;
        while(t >= 256) {
            if(neptunia_check_flags(&ctx)) {
                t = ctx.tree2[t];
            } else {
                t = ctx.tree1[t];
            }
        }
        if(t < 0) return -1;
        out[i] = t;
    }
    return i;
}



int neptunia(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *data   = in;
    unsigned char   *o      = out;
    unsigned char   *ol     = out + outsz;

    int sign        = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;    // 0x1234
    int chunks      = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;    // at least 1
    int chunk_xsize = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;    // 0x20000
    int chunks_off  = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;    // 0x1c minimum

    if(sign != 0x1234) {
        //return -1;    // better to automatically switch to neptunia0
        return neptunia_decompress(data, insz, out, outsz);
    }
    if(chunks       <  0) return -1;
    if(!chunks) return 0;
    if(chunk_xsize  <= 0) return -1;
    if(chunks_off   <  0x1c) return -1;
    if(chunks_off   >  insz) return -1;

    data += chunks_off;

    while(chunks--) {
        int chunk_size  = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;
        int chunk_zsize = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;
        int chunk_off   = in[0]|(in[1]<<8)|(in[2]<<16)|(in[3]<<24); in += 4;

        if(chunk_size  < 0) return -1;
        if(chunk_zsize < 0) return -1;
        if(chunk_off   < 0) return -1;

        int t = ol - o;
        if(t > chunk_size) t = chunk_size;

        t = neptunia_decompress(data + chunk_off, chunk_zsize, o, t);
        if(t < 0) return -1;
        o += t;
    }

    return o - out;
}


