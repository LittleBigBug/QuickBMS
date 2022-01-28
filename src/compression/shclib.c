/*
 *  shcodec ;) version 1.0.1
 *  Copyright (C) 1998-2002 Alexander Simakov
 *  April 2002
 *
 *  This software may be used freely for any purpose. However, when
 *  distributed, the original source must be clearly stated, and,
 *  when the source code is distributed, the copyright notice must
 *  be retained and any alterations in the code must be clearly marked.
 *  No warranty is given regarding the quality of this software.
 *
 *  internet: http://www.webcenter.ru/~xander
 *  e-mail: xander@online.ru
 */

#include "shc.h"

int sh_EncodeBlock(uchar *iBlock, uchar *oBlock, int bSize)
{
    int   freq[SH_MAX_ALPHA];
    uchar symb[SH_MAX_ALPHA];
    uchar *len, *code, *aux;
    int i, j, n, treeSize, streamSize, bits;
    uint bitbuf; uint *pBlock;

    if (!bSize) return (0);

    /* Save block size */
    *((int *)oBlock)=bSize;

    /* Get symbol freqs */
    sh_GetFreq(freq, iBlock, bSize);

    /* Sort symbols */
    n=sh_SortFreq(freq, symb);

    /* Calculate code lengths */
    sh_CalcLen(freq, symb, len=(uchar *)freq, n, 31);

    /* Sort symbols */
    sh_SortLen(len, symb, n);

    /* Calculate huffman codes */
    code=((uchar *)freq)+SH_MAX_ALPHA;
    sh_CalcCode(len, symb, code, n);

    /* Pack code tree */
    aux=((uchar *)freq)+SH_MAX_ALPHA*2;
    treeSize=sh_PackTree(len, symb, aux, (uint *)(oBlock+5), n);
    oBlock[4]=(uchar)treeSize;
    streamSize=5+treeSize;

    bits=31; bitbuf=0;
    pBlock=(uint *)(oBlock+streamSize);

    /* Encode a stream */
    for (i=j=0; i<bSize; i++)
    ENCODE_SYMB(len, code, iBlock[i], pBlock, j, bits, bitbuf);

    /* Flush remaining data */
    bitbuf<<=bits;
    pBlock[j++]=bitbuf;

    /* Return encoded stream size(in bytes) */
    return(streamSize+(j<<2));
}

int sh_DecodeBlock(uchar *iBlock, uchar *oBlock, int bSize)
{
    uchar len [SH_MAX_ALPHA];
    uchar symb[SH_MAX_ALPHA];
    uchar base[SH_MAX_CLEN];
    uchar offs[SH_MAX_CLEN];
    uchar *cache;
    int i, j, n, streamSize, treeSize, bits, symbol;
    uint bitbuf, *pBlock;

    if (!bSize) return (0);

    /* Read stream size */
    streamSize=*((int *)iBlock);

    /* Read packed tree size */
    treeSize=iBlock[4];

    /* Expand packed tree */
    n=sh_ExpandTree(len, symb, (uint *)(iBlock+5));

    /* Sort symbols */
    sh_SortLen(len, symb, n);

    /* Calculate decode tables */
    sh_CalcDecode(len, symb, base, offs, cache=len, n);

    pBlock=(uint *)(iBlock+treeSize+5);
    bits=31; bitbuf=pBlock[0];

    /* Decode a stream */
    for (i=0, j=1; i<streamSize; i++) {
        DECODE_SYMB(base, offs, cache, symb, pBlock, j, bits, bitbuf, symbol);
        oBlock[i]=(uchar)symbol;
    }

    /* Return decoded stream size */
    return(streamSize);
}
