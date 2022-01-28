// modified by Luigi Auriemma (this code is just lzss)
// written by Treeki <treeki@gmail.com>
// https://github.com/Treeki/RandomStuff/blob/eeb8c19408ac8a6d4adde6f93dc67b7a2843878e/SegaLZS2Decomp.c

int sega_lzs2(unsigned char *src, int insz, unsigned char **ret_out, int *ret_outsz) {
static unsigned char decBuffer[0x1000];
    memset(decBuffer, 0, sizeof(decBuffer));
    unsigned char *o;

    // try to guess if there is an header
    int     claimed_decompSize,
            claimed_compSize;
    claimed_decompSize = -1;
    if(!memcmp(src, "CM", 2)) { // some rare chances of catching a compressed stream
        src += 2;   insz -= 2;  // CM
        claimed_decompSize = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
        src += 4;   insz -= 4;  // decompSize
        claimed_compSize   = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
        src += 4;   insz -= 4;  // compSize

        // error, restore
        if( (claimed_compSize   < 0) || (claimed_compSize   > insz) ||
            (claimed_decompSize < 0) /*|| (claimed_decompSize > decompSize)*/
        ) {
            src  -= (2 + 4 + 4);
            insz += (2 + 4 + 4);
            claimed_decompSize = -1;
        } else if(claimed_decompSize > *ret_outsz) {
            *ret_outsz = claimed_decompSize;
            *ret_out = realloc(*ret_out, *ret_outsz);
            if(!*ret_out) return -1;
        }
    } else if(!memcmp(src, "lzs2", 4)) {
        src += 4;   insz -= 4;  // lzs2
        claimed_decompSize = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
        src += 4;   insz -= 4;  // decompSize

        // error, restore
        if((claimed_decompSize < 0) /*|| (claimed_decompSize > decompSize)*/) {
            src  -= (4 + 4);
            insz += (4 + 4);
            claimed_decompSize = -1;
        } else if(claimed_decompSize > *ret_outsz) {
            *ret_outsz = claimed_decompSize;
            *ret_out = realloc(*ret_out, *ret_outsz);
            if(!*ret_out) return -1;
        }
    }

    int currentControlBit = 0;
    int controlByte = 0;
    int dbIndex = 0;

    if(claimed_decompSize < 0) claimed_decompSize = *ret_outsz;

    o = *ret_out;
    while(o < ((*ret_out) + claimed_decompSize)) {
        if (currentControlBit == 0) {
            controlByte = *src;
            currentControlBit = 8;
            src++;
        }

        // now process the control bit
        if (controlByte & 0x80) {
            // CONTROL BIT SET

            unsigned short value = (src[0] << 8) | src[1];
            src += 2;

            unsigned char *endCopyAt = o + (value >> 12) + 3;

            if (endCopyAt >= o) {
                int dunno = value & 0xFFF;
                //unsigned char *dontReallyKnow = decBuffer;

                do {
                    unsigned char thisByte = decBuffer[dunno];
                    dunno = (dunno + 1) & 0xFFF;

                    decBuffer[dbIndex] = thisByte;
                    *o = thisByte;

                    o++;
                    dbIndex = (dbIndex + 1) & 0xFFF;
                } while (o < endCopyAt);
            }

        } else {
            unsigned char thisByte = *src;
            thisByte ^= dbIndex;

            decBuffer[dbIndex] = thisByte;

            dbIndex = (dbIndex + 1) & 0xFFF;

            src++;

            *o = thisByte;
            o++;
        }

        controlByte <<= 1;

        currentControlBit--;
    }

    return(o - *ret_out);
}

