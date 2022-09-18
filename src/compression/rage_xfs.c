/*
  RA decompression class version 1.0
  (c) 2006 Benjamin Haisch
  email: john_doe@techie.com
  You may use this code freely as long as you give me credit for it :)
*/

#include <stdlib.h>
#include <string.h>

typedef int             bool;
typedef unsigned char   byte;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

static        int bitsLeft;
static        byte bitBuffer;
static        byte *inb, *outb;
static        byte *inl, *outl;
static        byte countTable1[512], countTable2[512];
static        byte lengthTable1[512], lengthTable2[512];
static        byte offsetTable1[32], offsetTable2[32];

static int getBit() {
    if (bitsLeft == 0) {
        if(inb >= inl) return 0;
        bitBuffer = *inb++;
        bitsLeft = 8;
    }
    byte t = (bitBuffer & 0x80) >> 7;
    bitBuffer <<= 1;
    bitsLeft--;
    return t;
}

static int loadBits(int count) {
    int value = 0;
    while (count > 0) {
        value = (value << 1) | getBit();
        count--;
    }
    return value;
}

static void loadTable(byte *table1, byte *table2) {
    if(inb >= inl) return;
    int count = (*inb++) * 2;
    int len1 = loadBits(3) + 1;
    int len2 = loadBits(3) + 1;
    while (count > 0) {
        byte value;
        if (getBit() == 0) {
            value = loadBits(len1);
        } else {
            value = loadBits(len2);
            *table2 = value;
            value = 0;
        }
        *table1++ = value;
        table2++;
        count--;
    }
}

static int loadValue(byte *table1, byte *table2) {
    int index = 0;
    byte value = 0;
    do {
        index = (value << 1) | getBit();
        value = table1[index];
    } while (value != 0);
    return table2[index];
}

static int loadCount() {
    return loadValue(countTable1, countTable2);
}

static int loadLength() {
    int length = loadValue(lengthTable1, lengthTable2);
    if (length == 255) {
        byte len_hi = (getBit() << 1) | getBit();
        if(inb >= inl) return 0;
        byte len_lo = *inb++;
        length = ((len_hi << 8) | len_lo) + 255;
    }
    return length;
}

static int loadOffset() {
    int offset = loadValue(offsetTable1, offsetTable2);
    if (offset >= 2) {
        int bitcount = offset - 1;
        offset = 1;
        while (bitcount > 0) {
            offset = (offset << 1) | getBit();
            bitcount--;
        }
    }
    return ~offset;
}

static void decompressData() {

    bitsLeft = 0;
    bitBuffer = 0;

    memset(countTable1, 0, 512);
    memset(countTable2, 0, 512);
    memset(lengthTable1, 0, 512);
    memset(lengthTable2, 0, 512);
    memset(offsetTable1, 0, 32);
    memset(offsetTable2, 0, 32);

    /* initialize bitbuffer */
    getBit();

    do {
        if((inb + 2) > inl) break;
        uint16 blocksize = *(uint16*)inb;
        inb += 2;

        /* load tables */
        loadTable(lengthTable1, lengthTable2);
        loadTable(countTable1, countTable2);
        loadTable(offsetTable1, offsetTable2);

        /* process the block */
        while (blocksize > 0) {
            int copyCount = loadCount();
            while(copyCount--) {
                if(inb  >= inl) break;
                if(outb >= outl) break;
                *outb++ = *inb++;
            }
            int length = loadLength();
            if (length > 0) {
                int offset = loadOffset();
                /* don't use memcpy here because source and dest can overlap */
                while (length > 0) {
                    if(outb >= outl) break;
                    *outb = *(outb+offset);
                    outb++;
                    length--;
                }
            }
            blocksize--;
        }

    } while (getBit());

}

static bool isRA(byte *inbuffer, uint32 insize) {
    return (insize > 12 && !memcmp(inbuffer, "RA\x00\x02", 4) /* *(uint32*)inbuffer == 0x02004152 */);
}

uint32 rage_xfs_decompress(byte *inbuffer, int insize, byte *outbuffer, int outsize) {
    if (!outbuffer || !inbuffer)
        return 0;

    //uint32 outsize = *(uint32*)&inbuffer[4];
    //*outbuffer = new byte[outsize];

    inl  = inbuffer + insize;
    outl = outbuffer + outsize;

    /* set input to the beginning of the compressed data */
    if(isRA(inbuffer, insize)) {
        inb = inbuffer + 12;    // magic, outsize, ???
    } else {
        inb = inbuffer;         // raw data
    }
    outb = outbuffer;
    decompressData();

    /* return the decompressed size */
    return outb - outbuffer;
}
