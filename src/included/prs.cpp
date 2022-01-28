// modified by Luigi Auriemma
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prs.h"

typedef struct {
    unsigned char bitpos;
    unsigned char* controlbyteptr;
    unsigned char* srcptr_orig;
    unsigned char* dstptr_orig;
    unsigned char* srcptr;
    unsigned char* dstptr; } PRS_COMPRESSOR;

void prs_put_control_bit(PRS_COMPRESSOR* pc,unsigned char bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_control_bit_nosave(PRS_COMPRESSOR* pc,unsigned char bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
}

void prs_put_control_save(PRS_COMPRESSOR* pc)
{
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_static_data(PRS_COMPRESSOR* pc,unsigned char data)
{
    *pc->dstptr = data;
    pc->dstptr++;
}

unsigned char prs_get_static_data(PRS_COMPRESSOR* pc)
{
    unsigned char data = *pc->srcptr;
    pc->srcptr++;
    return data;
}

////////////////////////////////////////////////////////////////////////////////

void prs_init(PRS_COMPRESSOR* pc,void* src,void* dst)
{
    pc->bitpos = 0;
    pc->srcptr = (unsigned char*)src;
    pc->srcptr_orig = (unsigned char*)src;
    pc->dstptr = (unsigned char*)dst;
    pc->dstptr_orig = (unsigned char*)dst;
    pc->controlbyteptr = pc->dstptr;
    pc->dstptr++;
}

void prs_finish(PRS_COMPRESSOR* pc)
{
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,1);
    if (pc->bitpos != 0) *pc->controlbyteptr = ((*pc->controlbyteptr << pc->bitpos) >> 8);
    prs_put_static_data(pc,0);
    prs_put_static_data(pc,0);
}

void prs_rawbyte(PRS_COMPRESSOR* pc)
{
    prs_put_control_bit_nosave(pc,1);
    prs_put_static_data(pc,prs_get_static_data(pc));
    prs_put_control_save(pc);
}

void prs_shortcopy(PRS_COMPRESSOR* pc,int offset,unsigned char size)
{
    size -= 2;
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,(size >> 1) & 1);
    prs_put_control_bit_nosave(pc,size & 1);
    prs_put_static_data(pc,offset & 0xFF);
    prs_put_control_save(pc);
}

void prs_longcopy(PRS_COMPRESSOR* pc,int offset,unsigned char size)
{
    if (size <= 9)
    {
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,((offset << 3) & 0xF8) | ((size - 2) & 0x07));
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_control_save(pc);
    } else {
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,(offset << 3) & 0xF8);
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_static_data(pc,size - 1);
        prs_put_control_save(pc);
    }
}

void prs_copy(PRS_COMPRESSOR* pc,int offset,unsigned char size)
{
    if ((offset > -0x100) && (size <= 5)) prs_shortcopy(pc,offset,size);
    else prs_longcopy(pc,offset,size);
    pc->srcptr += size;
}

////////////////////////////////////////////////////////////////////////////////

// compresses data using PRS. dest buffer should be ((9 * source_size) / 8) + 2 bytes in length.
// yes, that is longer than the original length, but it is highly unlikely that the
// compressed data will be larger than the uncompressed data.
unsigned long prs_compress(void* source,void* dest,unsigned long size)
{
    PRS_COMPRESSOR pc;
    unsigned long x,y;
    unsigned long xsize;
    int lsoffset;
    unsigned int lssize;
    prs_init(&pc,source,dest);
    for (x = 0; x < size; x++)
    {
        lsoffset = lssize = xsize = 0;
        for (y = x - 3; (y > 0) && (y > (x - 0x1FF0)) && (xsize < 255); y--)
        {
            if(x < 0) continue;
            if((x + xsize) > size) continue;
            if(y < 0) continue;
            if((y + xsize) > size) continue;

            xsize = 3;
            if (!memcmp((void*)((unsigned long)source + y),(void*)((unsigned long)source + x),xsize))
            {
                do xsize++;
                while (!memcmp((void*)((unsigned long)source + y),
                               (void*)((unsigned long)source + x),
                               xsize) &&
                       (xsize < 256) &&
                       ((y + xsize) < x) &&
                       ((x + xsize) <= size)
                );
                xsize--;
                if (xsize > lssize)
                {
                    lsoffset = -(x - y);
                    lssize = xsize;
                }
            }
        }
        if (lssize == 0) prs_rawbyte(&pc);
        else {
            prs_copy(&pc,lsoffset,lssize);
            x += (lssize - 1);
        }
    }
    prs_finish(&pc);
    return pc.dstptr - pc.dstptr_orig;
}

////////////////////////////////////////////////////////////////////////////////

// decompresses PRS data. it is not possible to tell how large the decompressed data
// will be without running through it (see prs_decompress_size).
unsigned long prs_decompress(void* source,void* dest)
{
    unsigned long r3,r5; // PowerPC registers (used as temp variables in this function) 
    unsigned long bitpos = 9; // this variable tells when to read another byte from the control stream 
    unsigned char* sourceptr = (unsigned char*)source; // pointer to the source stream 
    unsigned char* destptr = (unsigned char*)dest; // pointer to the destination data stream 
    unsigned char* destptr_orig = (unsigned char*)dest; // pointer to the beginning of the destination data stream 
    unsigned char currentbyte; // the current byte in the control stream that is being used 
    int flag; // the current bit in the control stream that is being used 
    int offset; // the offset to read a linked chunk from (for short/long copies) 
    unsigned long x,t; // loop variables 

    currentbyte = sourceptr[0]; // read the first byte from the source stream 
    sourceptr++; // update the source pointer 
    for (;;) // infinite loop - there's only one way to stop decompressing (see below) 
    {
        bitpos--; // advance the control stream by one bit 
        if (bitpos == 0) // if there are not bits left in the current byte... 
        {
            currentbyte = sourceptr[0]; // ...then read anothe byte from the control stream 
            bitpos = 8; // ...reset this variable 
            sourceptr++; // ...and update the source pointer 
        }
        flag = currentbyte & 1; // read a bit from the control stream 
        currentbyte = currentbyte >> 1; // and update the current byte 
        if (flag) // is the bit is a 1...
        {
            destptr[0] = sourceptr[0]; // ...then copy a byte from the source stream to the destination data stream 
            sourceptr++; // ...update the source pointer 
            destptr++; // ...update the destination data pointer 
            continue; // ...and go back to the beginning of the loop 
        }
        // so if execution reaches here, the bit was a 0, not a 1 
        bitpos--; // so advance the control stream by one bit 
        if (bitpos == 0) // if there are not bits left in the current byte... 
        {
            currentbyte = sourceptr[0]; // ...then read anothe byte from the control stream 
            bitpos = 8; // ...reset this variable 
            sourceptr++; // ...and update the source pointer 
        }
        flag = currentbyte & 1; // read a bit from the control stream 
        currentbyte = currentbyte >> 1; // and update the current byte 
        if (flag) // is the bit is a 1, then we'll do a long copy... 
        {
            r3 = sourceptr[0] & 0xFF; // read a little-endian word from the source stream (the next line completes this operation) 
            offset = ((sourceptr[1] & 0xFF) << 8) | r3;
            sourceptr += 2; // and update the source pointer 
            if (offset == 0) return (unsigned long)(destptr - destptr_orig); // is the offset zero? if so, we're done decompressing; return the size 
            r3 = r3 & 0x00000007; // r3 is the size of the data to copy 
            r5 = (offset >> 3) | 0xFFFFE000; // r5 is the offset into the destination data to copy from (relative to the current destination data pointer) 
            if (r3 == 0) // if the size is zero... 
            {
                flag = 0; // ...then clear the flag 
                r3 = sourceptr[0] & 0xFF; // ...read a byte from the source stream for the length 
                sourceptr++; // ...and update the source pointer 
                r3++; // ...and add 1 to the length, since it would be pointless to copy zero bytes 
            } else r3 += 2; // otherwise, add 2 to the size 
            r5 += (unsigned long)destptr; // r5 is now the memory address to copy data to 
            //printf("> > > %08X->%08X ldat %08X %08X %s\n",sourceptr - sourceptr_orig,destptr - destptr_orig,r5 - (DWORD)destptr,r3,flag ? "inline" : "extended");
        } else {
            // this loop basically reads 2 bits from the control stream for the size of this copy 
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    currentbyte = sourceptr[0];
                    bitpos = 8;
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00; // and the offset comes from the source stream 
            r3 += 2;
            sourceptr++;
            r5 = offset + (unsigned long)destptr;
        }
        if (r3 == 0) continue;
        t = r3;
        for (x = 0; x < t; x++)
        {
            destptr[0] = *(unsigned char*)r5;
            r5++;
            r3++;
            destptr++;
        }
    }
}

// this function is exactly the same as prs_decompress, except there is no
// destination stream. it only returns the size.
unsigned long prs_decompress_size(void* source)
{
    unsigned long r3,r5;
    unsigned long bitpos = 9;
    unsigned char* sourceptr = (unsigned char*)source;
    unsigned char* destptr = NULL;
    unsigned char* destptr_orig = NULL;
    unsigned char currentbyte/*,lastbyte*/;
    int flag;
    int offset;
    unsigned long x,t;

    currentbyte = sourceptr[0];
    sourceptr++;
    for (;;)
    {
        bitpos--;
        if (bitpos == 0)
        {
            //lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            sourceptr++;
            destptr++;
            continue;
        }
        bitpos--;
        if (bitpos == 0)
        {
            //lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            r3 = sourceptr[0];
            offset = (sourceptr[1] << 8) | r3;
            sourceptr += 2;
            if (offset == 0) return (unsigned long)(destptr - destptr_orig);
            r3 = r3 & 0x00000007;
            r5 = (offset >> 3) | 0xFFFFE000;
            if (r3 == 0)
            {
                r3 = sourceptr[0];
                sourceptr++;
                r3++;
            } else r3 += 2;
            r5 += (unsigned long)destptr;
        } else {
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    //lastbyte = currentbyte = sourceptr[0];
                    bitpos = 8;
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00;
            r3 += 2;
            sourceptr++;
            r5 = offset + (unsigned long)destptr;
        }
        if (r3 == 0) continue;
        t = r3;
        for (x = 0; x < t; x++)
        {
            r5++;
            r3++;
            destptr++;
        }
    }
}

