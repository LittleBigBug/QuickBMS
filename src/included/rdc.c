/*
 * Listing 2
 *
 * DECOMPRS.C - Ross Data Compression (RDC)
 *              decompress function
 *
 * Written by Ed Ross, 1/92
 *
 * decompress inbuff_len bytes of inbuff into outbuff.
 *
 * return length of outbuff.
 *
*/

#include <string.h>

//typedef unsigned char uchar;    /*  8 bits, unsigned */
//typedef unsigned int  uint;     /* 16 bits, unsigned */

int rdc_decompress(uchar *inbuff, uint inbuff_len,
                   uchar *outbuff)
{
uint    ctrl_bits = 0;
uint    ctrl_mask = 0;
uchar   *inbuff_idx = inbuff;
uchar   *outbuff_idx = outbuff;
uchar   *inbuff_end = inbuff + inbuff_len;
uint    cmd;
uint    cnt;
uint    ofs;
//uint    len;

  /* process each item in inbuff */

  while (inbuff_idx < inbuff_end)
  {
    /* get new load of control bits if needed */

    if ((ctrl_mask >>= 1) == 0)
    {
      ctrl_bits = * (uint *) inbuff_idx;
      inbuff_idx += 2;
      ctrl_mask = 0x8000;
    }

    /* just copy this char if control bit is zero */

    if ((ctrl_bits & ctrl_mask) == 0)
    {
      *outbuff_idx++ = *inbuff_idx++;

      continue;
    }

    /* undo the compression code */

    cmd = (*inbuff_idx >> 4) & 0x0F;
    cnt = *inbuff_idx++ & 0x0F;

    switch (cmd)
    {
    case 0:     /* short rle */
         cnt += 3;
         memset(outbuff_idx, *inbuff_idx++, cnt);
         outbuff_idx += cnt;
         break;

    case 1:     /* long rle */
         cnt += (*inbuff_idx++ << 4);
         cnt += 19;
         memset(outbuff_idx, *inbuff_idx++, cnt);
         outbuff_idx += cnt;
         break;

    case 2:     /* long pattern */
         ofs = cnt + 3;
         ofs += (*inbuff_idx++ << 4);
         cnt = *inbuff_idx++;
         cnt += 16;
         memcpy(outbuff_idx, outbuff_idx - ofs, cnt);
         outbuff_idx += cnt;
         break;

    default:    /* short pattern */
         ofs = cnt + 3;
         ofs += (*inbuff_idx++ << 4);
         memcpy(outbuff_idx, outbuff_idx - ofs, cmd);
         outbuff_idx += cmd;
         break;
    }
  }

  /* return length of decompressed buffer */

  return outbuff_idx - outbuff;
}
