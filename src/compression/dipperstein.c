#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../extra/mem2mem.h"

#include "../libs/dipperstein/huffman.h"
#include "../libs/dipperstein/arcode.h"
#include "../libs/dipperstein/delta.h"
#include "../libs/dipperstein/freqsub.h"
#include "../libs/dipperstein/lzss.h"
#include "../libs/dipperstein/lzw.h"
#include "../libs/dipperstein/rice.h"
#include "../libs/dipperstein/rle.h"



int dipperstein_decompress(unsigned char *in, int insz, unsigned char *out, int outsz, int compression) {
    int     ret;

    mem2mem_init(in, insz, out, outsz);

    switch(compression) {
        case 0:  ret = ArDecodeFile(mem2mem_FILE, mem2mem_FILE, MODEL_ADAPTIVE); break;
        case 1:  ret = ArDecodeFile(mem2mem_FILE, mem2mem_FILE, MODEL_STATIC); break;
        case 2:  ret = DeltaDecodeFile(mem2mem_FILE, mem2mem_FILE, 8); break;
        case 3:  ret = FreqDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        case 4:  ret = HuffmanDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        case 5:  ret = CanonicalDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        case 6:  ret = DecodeLZSS(mem2mem_FILE, mem2mem_FILE); break;
        case 7:  ret = LZWDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        case 8:  ret = RiceDecodeFile(mem2mem_FILE, mem2mem_FILE, 7 /* 2 - 7 */); break;
        case 9:  ret = RleDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        case 10: ret = VPackBitsDecodeFile(mem2mem_FILE, mem2mem_FILE); break;
        default: ret = -1; break;
    }

    if(ret < 0) return -1;
    ret = mem2mem_ret();
    return ret;
}

