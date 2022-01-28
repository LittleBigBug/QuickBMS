// http://www.cs.tut.fi/~albert/Dev/pucrunch/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "pucrunch.h"


#define DELTA
#define DELTA_OP +


/* Pucrunch ©1997-2008 by Pasi 'Albert' Ojala, a1bert@iki.fi */
/* Pucrunch is now under LGPL: see the doc for details. */


/* #define BIG */
/*
    Define BIG for >64k files.
    It will use even more *huge* amounts of memory.

    Note:
    Although this version uses memory proportionally to the file length,
    it is possible to use fixed-size buffers. The LZ77 history buffer
    (and backSkip) needs to be as long as is needed, the other buffers
    minimally need to be about three times the length of the maximum
    LZ77 match. Writing the compressor this way would probably make it a
    little slower, and automatic selection of e.g. escape bits might not be
    practical.

    Adjusting the number of escape bits to adapt to local
    changes in the data would be worth investigating.

    Also, the memory needed for rle/elr tables could probably be reduced
    by using a sparse table implementation. Because of the RLE property
    only the starting and ending points (or lengths) need be saved. The
    speed should not decrease too much, because the tables are used in
    LZ77 string match also.... Wait! Actually no, because the RLE/LZ77
    optimize needs to change the RLE lengths inside RLE's...

    The elr array can be reduced to half by storing only the byte that
    is before a run of bytes if we have the full backSkip table..

    Because the lzlen maximum value is 256, we could reduce the table
    from unsigned short to unsigned char by encoding 0->0, 2->1, .. 256->255.
    lzlen of the value 1 is never used anyway..

 */

#define ENABLE_VERBOSE      /* -v outputs the lz77/rle data to stdout */
#define HASH_STAT	    /* gives statistics about the hash compares */
#define BACKSKIP_FULL       /* full backSkip table - enables RESCAN. If */
                            /* not defined, backSkip only uses max 128kB */
#define RESCAN		    /* rescans LZ77 matches for a closer match. */
#define HASH_COMPARE    /* Use a 3-to-1 hash to skip impossible matches */
/* takes "inbytes" bytes, reduces string compares from 16% to 8% */



//const char version[] = "\0$VER: pucrunch 1.14 22-Nov-2008\n";



static int maxGamma = 7/*, reservedBytes = 2*/;
static int escBits = 2/*, escMask = 0xc0*/;
static int extraLZPosBits = 0, rleUsed = 15;

static int memConfig = 0x37, intConfig = 0x58; /* cli */



static const unsigned char *up_Data;
static int up_Mask, up_Byte;



#ifdef ENABLE_VERBOSE
#define F_VERBOSE (1<<0)
#endif
#define F_STATS   (1<<1)
#define F_AUTO    (1<<2)
#define F_NOOPT   (1<<3)
#define F_AUTOEX  (1<<4)
#define F_SKIP    (1<<5)
#define F_2MHZ    (1<<6)
#define F_AVOID   (1<<7)
#define F_DELTA   (1<<8)

#define F_NORLE   (1<<9)

#define F_UNPACK  (1<<14)
#define F_ERROR   (1<<15)

#ifndef min
#define min(a,b) ((a<b)?(a):(b))
#endif


#define LRANGE		(((2<<maxGamma)-3)*256)	/* 0..125, 126 -> 1..127 */
#define MAXLZLEN	(2<<maxGamma)
#define MAXRLELEN	(((2<<maxGamma)-2)*256)	/* 0..126 -> 1..127 */
#define DEFAULT_LZLEN	LRANGE

static int lrange, maxlzlen, maxrlelen;



#ifdef BIG
#define OUT_SIZE 2000000
#else
#define OUT_SIZE 65536
#endif /* BIG */
static unsigned char outBuffer[OUT_SIZE];
static int outPointer = 0;
//static int bitMask = 0x80;



static const unsigned char *up_Data;
static int up_Mask, up_Byte;
void up_SetInput(const unsigned char *data) {
    up_Data = data;
    up_Mask = 0x80;
    up_Byte = 0;
}
int up_GetBits(int bits) {
    int val = 0;

    while (bits--) {
	val <<= 1;
	if ((*up_Data & up_Mask))
	   val |= 1;
	up_Mask >>= 1;
	if (!up_Mask) {
	    up_Mask = 0x80;
	    up_Data++;
	    up_Byte++;
	}
    }
    return val;
}
int up_GetValue(void) {
    int i = 0;

    while (i<maxGamma) {
	if (!up_GetBits(1))
	    break;
	i++;
    }
    return (1<<i) | up_GetBits(i);
}



int pucrunch_UnPack(int loadAddr, const unsigned char *data, unsigned char *file,
	   int flags) {
    long size, startEsc, endAddr, execAddr, headerSize;
    long startAddr, error = 0;
    //FILE *fp;
    int i, overlap;
    //long timeused = clock();
    const char *byteCodeVec;
#define MAXCODES 20
    int mismatch[MAXCODES], collect[ftEnd];
    struct FixStruct *dc;

    if(loadAddr < 0) {
        loadAddr = data[0] + 256*data[1];
        data += 2;
    }
    if (loadAddr < 0) loadAddr = 0x258;

    /* Search for the right code */
    if (data[0] == 'p' && data[1] == 'u') {
	/* was saved without decompressor */
	int cnt = 2;

	endAddr = (data[cnt] | (data[cnt+1]<<8)) + 0x100;
	cnt += 2;

	startEsc = data[cnt++];
	startAddr = data[cnt] | (data[cnt+1] << 8);
	cnt += 2;

	escBits = data[cnt++];
	if (escBits < 0 || escBits > 8) {
	    fprintf(stderr, "Error: Broken archive, escBits %d.\n",
		    escBits);
	    return -1; //20;
	}
	maxGamma = data[cnt++] - 1;
	if (data[cnt++] != (1<<maxGamma) ||
	   maxGamma < 5 || maxGamma > 7) {
	    fprintf(stderr, "Error: Broken archive, maxGamma %d.\n",
		    maxGamma);
	    return -1; //20;
	}
    lrange = LRANGE;
    maxlzlen = MAXLZLEN;
    maxrlelen = MAXRLELEN;

	extraLZPosBits = data[cnt++];
	if (extraLZPosBits < 0 || extraLZPosBits > 4) {
	    fprintf(stderr, "Error: Broken archive, extraLZPosBits %d.\n",
		    extraLZPosBits);
	    return -1; //20;
	}

	execAddr = data[cnt] | (data[cnt+1]<<8);
	cnt += 2;

	rleUsed = data[cnt++];
	byteCodeVec = &data[cnt - 1];

	overlap = 0;
	memConfig = memConfig;
	intConfig = intConfig;

	size = endAddr-startAddr;
	headerSize = cnt + rleUsed;

	endAddr = loadAddr + size;

    } else {

	for (i=0; fixStruct[i].code && i < MAXCODES; i++) {
	    int j, maxDiff = 0;

	    if (fixStruct[i].code[1] != (loadAddr>>8))
		maxDiff = 5;
	    for (j=0; fixStruct[i].fixes[j].type != ftEnd; j++) {
		maxDiff++;
	    }
	    mismatch[i] = 0;
	    for (j=2; j<fixStruct[i].codeSize-15; j++) {
		if (fixStruct[i].code[j] != data[j-2])
		    mismatch[i]++;
	    }
	    if (mismatch[i] <= maxDiff) {
		fprintf(stderr, "Detected %s (%d <= %d)\n",
			fixStruct[i].name, mismatch[i], maxDiff);
		break;
	    }
	    fprintf(stderr, "Not %s (%d > %d)\n",
		    fixStruct[i].name, mismatch[i], maxDiff);
	}
	dc = &fixStruct[i];
	if (!dc->code) {
	    fprintf(stderr,
		    "Error: The file is not compressed with this program.\n");
	    return -1; //20;
	}

	if ((loadAddr & 0xff) != 1) {
	    fprintf(stderr, "Error: Misaligned basic start address 0x%04x\n",
		    loadAddr);
	    return -1; //20;
	}
	/* TODO: check that the decrunch code and load address match. */

	error = 0;

	for (i=0; i<ftEnd; i++) {
	    collect[i] = 0;
	}
	collect[ftMemConfig] = memConfig;
	collect[ftCli] = intConfig;
	for (i=0; dc->fixes[i].type!=ftEnd; i++) {
	    collect[dc->fixes[i].type] = data[dc->fixes[i].offset-2];
	}

	overlap = collect[ftOverlap];
	/* TODO: check overlap LO/HI and WrapCount */
	maxGamma = collect[ftMaxGamma] - 1;
	if (maxGamma < 5 || maxGamma > 7) {
	    fprintf(stderr, "Error: Broken archive, maxGamma %d.\n",
		    maxGamma);
	    return -1; //20;
	}
    lrange = LRANGE;
    maxlzlen = MAXLZLEN;
    maxrlelen = MAXRLELEN;

	if (collect[ft1MaxGamma] != (1<<maxGamma) ||
	    collect[ft8MaxGamma] != (8-maxGamma) ||
	    collect[ft2MaxGamma] != (2<<maxGamma)-1) {
	    fprintf(stderr,
		    "Error: Broken archive, maxGamma (%d) mismatch.\n",
		    maxGamma);
	    return -1; //20;
	}

	startEsc = collect[ftEscValue];
	startAddr = collect[ftOutposLo] | (collect[ftOutposHi]<<8);
	escBits = collect[ftEscBits];
	if (escBits < 0 || escBits > 8) {
	    fprintf(stderr, "Error: Broken archive, escBits %d.\n",
		    escBits);
	    return -1; //20;
	}

	if (collect[ftEsc8Bits] != 8-escBits) {
	    fprintf(stderr, "Error: Broken archive, escBits (%d) mismatch.\n",
		    escBits);
	    return -1; //20;
	}

	extraLZPosBits = collect[ftExtraBits];
	if (extraLZPosBits < 0 || extraLZPosBits > 4) {
	    fprintf(stderr, "Error: Broken archive, extraLZPosBits %d.\n",
		    extraLZPosBits);
	    return -1; //20;
	}
	endAddr = 0x100 + (collect[ftEndLo] | (collect[ftEndHi]<<8));
	size    = endAddr - (collect[ftInposLo] | (collect[ftInposHi]<<8));
	headerSize = ((collect[ftSizeLo] | (collect[ftSizeHi]<<8))
			+ 0x100 - size - loadAddr) & 0xffff;
	execAddr = collect[ftExecLo] | (collect[ftExecHi]<<8);

	memConfig = collect[ftMemConfig];
	intConfig = collect[ftCli];
	byteCodeVec = &data[dc->codeSize - 32 -2];

	rleUsed = 15 - dc->codeSize +2 + headerSize;
    }


    if ((flags & F_STATS)) {
	fprintf(stderr,
		"Load 0x%04x, Start 0x%04lx, exec 0x%04lx, %s%s$01=$%02x\n",
		loadAddr, startAddr, execAddr,
		(intConfig==0x58)?"cli, ":"", (intConfig==0x78)?"sei, ":"",
		memConfig);
	fprintf(stderr, "Escape bits %d, starting escape 0x%02lx\n",
		escBits, (startEsc<<(8-escBits)));
	fprintf(stderr,
		"Decompressor size %ld, max length %d, LZPOS LO bits %d\n",
		headerSize+2, (2<<maxGamma), extraLZPosBits+8);
	fprintf(stderr, "rleUsed: %d\n", rleUsed);
    }

    if (rleUsed > 15) {
	fprintf(stderr, "Error: Old archive, rleUsed %d > 15.\n", rleUsed);
	return -1; //20;
    }

    outPointer = 0;
    up_SetInput(data + headerSize);
    while (!error) {
	int sel = startEsc;

#ifndef BIG
	if (startAddr + outPointer >= up_Byte + endAddr - size) {
	    if (!error)
		fprintf(stderr, "Error: Target %5ld exceeds source %5ld..\n",
			startAddr + outPointer, up_Byte + endAddr - size);
	    error++;
	}
	if (up_Byte > size + overlap) {
	    fprintf(stderr, "Error: No EOF symbol found (%d > %d).\n",
		    (int)up_Byte, (int)(size + overlap));
	    error++;
	}
#endif /* BIG */

	if (escBits)
	    sel = up_GetBits(escBits);
	if (sel == startEsc) {
	    int lzPos, lzLen = up_GetValue(), i;
#ifdef DELTA
	    int add = 0;
#endif

	    if (lzLen != 1) {
		int lzPosHi = up_GetValue()-1, lzPosLo;

		if (lzPosHi == (2<<maxGamma)-2) {
#ifdef DELTA
/* asm: 25 bytes longer */
		    if (lzLen > 2) {
			add = up_GetBits(8);
			lzPos = up_GetBits(8) ^ 0xff;
		    } else
#endif
		    break; /* EOF */
		} else {
		    if (extraLZPosBits) {
			lzPosHi = (lzPosHi<<extraLZPosBits) |
				    up_GetBits(extraLZPosBits);
		    }
		    lzPosLo = up_GetBits(8) ^ 0xff;
		    lzPos = (lzPosHi<<8) | lzPosLo;
		}
	    } else {
		if (up_GetBits(1)) {
		    int rleLen, byteCode, byte;

		    if (!up_GetBits(1)) {
			int newEsc = up_GetBits(escBits);

			outBuffer[outPointer++] =
			    (startEsc<<(8-escBits)) | up_GetBits(8-escBits);
/*fprintf(stdout, "%5ld %5ld  *%02x\n",
	outPointer-1, up_Byte, outBuffer[outPointer-1]);*/
			startEsc = newEsc;
			if (outPointer >= OUT_SIZE) {
			    fprintf(stderr, "Error: Broken archive, "
				    "output buffer overrun at %d.\n",
				    outPointer);
			    return -1; //20;
			}
			continue;
		    }
		    rleLen = up_GetValue();
		    if (rleLen >= (1<<maxGamma)) {
			rleLen = ((rleLen-(1<<maxGamma))<<(8-maxGamma)) |
			    up_GetBits(8-maxGamma);
			rleLen |= ((up_GetValue()-1)<<8);
		    }
		    byteCode = up_GetValue();
		    if (byteCode < 16/*32*/) {
			byte = byteCodeVec[byteCode];
		    } else {
			byte = ((byteCode-16/*32*/)<<4/*3*/) | up_GetBits(4/*3*/);
		    }

/*fprintf(stdout, "%5ld %5ld RLE %5d 0x%02x\n", outPointer, up_Byte, rleLen+1,
	byte);*/
		    if (outPointer + rleLen + 1 >= OUT_SIZE) {
			fprintf(stderr, "Error: Broken archive, "
				"output buffer overrun at %d.\n",
				OUT_SIZE);
			return -1; //20;
		    }
		    for (i=0; i<=rleLen; i++) {
			outBuffer[outPointer++] = byte;
		    }
		    continue;
		}
		lzPos = up_GetBits(8) ^ 0xff;
	    }
/*fprintf(stdout, "%5ld %5ld LZ %3d 0x%04x\n",
	outPointer, up_Byte, lzLen+1, lzPos+1);*/

	    /* outPointer increases in the loop, thus its minimum is here */
	    if (outPointer - lzPos -1 < 0) {
		fprintf(stderr, "Error: Broken archive, "
			"LZ copy position underrun at %d (%d). "
			"lzLen %d.\n",
			outPointer, lzPos+1, lzLen+1);
		return -1; //20;
	    }
	    if (outPointer + lzLen + 1 >= OUT_SIZE) {
		fprintf(stderr, "Error: Broken archive, "
			"output buffer overrun at %d.\n",
			OUT_SIZE);
		return -1; //20;
	    }
	    for (i=0; i<=lzLen; i++) {
		outBuffer[outPointer] = outBuffer[outPointer - lzPos - 1]
#ifdef DELTA
					DELTA_OP add;
#else
					;
#endif
		outPointer++;
	    }
	} else {
	    int byte = (sel<<(8-escBits)) | up_GetBits(8-escBits);
/*fprintf(stdout, "%5ld %5ld  %02x\n",
	outPointer, up_Byte, byte);*/
	    outBuffer[outPointer++] = byte;
	    if (outPointer >= OUT_SIZE) {
		fprintf(stderr, "Error: Broken archive, "
			"output buffer overrun at %d.\n",
			outPointer);
		return -1; //20;
	    }
	}
    }
    if (error)
	fprintf(stderr, "Error: Target exceeded source %5ld times.\n",
		error);

	unsigned char tmp[2];
	tmp[0] = startAddr & 0xff;
	tmp[1] = (startAddr >> 8);

    memcpy(file, tmp, 2);
    memcpy(file + 2, outBuffer, outPointer);
    return 2 + outPointer;
}


