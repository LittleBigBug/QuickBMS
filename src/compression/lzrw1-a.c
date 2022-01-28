// modified by Luigi Auriemma
/*
http://www.ross.net/compression/download/original/old_lzrw1.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ass(A,B)
#define fast_copy(A,B,C)    memmove(B,A,C)  // just to be sure that it's ok
//#define FLAG_COMPRESS 0     /* Signals that output was result of compression. */
//#define FLAG_COPY     1     /* Signals that output was simply copied over.    */
#define ULONG_ALIGN_UP(X) ((((ULONG)X)+3)&~3)
#define LOCAL
typedef unsigned char   UBYTE;
typedef unsigned short  UCARD;
typedef unsigned short  UWORD;
typedef unsigned int    ULONG;

#define U(X) ((ULONG) X)

// U(U(4)*U(4096)+U(3)),                    /* Working memory (bytes) to alg.   */

#define FLAG_BYTES    0     /* How many bytes does the flag use up?           */

LOCAL void lzrw1a_compress_decompress(wrk_mem,p_src_first,src_len,p_dst_first,p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone.          */
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */
/* Input  : Input block and output zone must not overlap. User knows    */
/* Input  : upperbound on output block length from earlier compression. */
/* Input  : In any case, maximum expansion possible is nine times.      */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
UBYTE *p_src_first, *p_dst_first; ULONG src_len, *p_dst_len; UBYTE *wrk_mem;
{register UBYTE *p_src=p_src_first+FLAG_BYTES, *p_dst=p_dst_first;
 UBYTE *p_src_post=p_src_first+src_len;
 UBYTE *p_src_max16=p_src_first+src_len-(16*2);
 register ULONG control=1;
 //if (*p_src_first==FLAG_COPY)
 //  {fast_copy(p_src_first+FLAG_BYTES,p_dst_first,src_len-FLAG_BYTES);
 //   *p_dst_len=src_len-FLAG_BYTES; return;}
 while (p_src!=p_src_post)
   {register UWORD unroll;
    if (control==1) {control=0x10000|*p_src++; control|=(*p_src++)<<8;}
    unroll= p_src<=p_src_max16 ? 16 : 1;
    while (unroll--)
      {if (control&1)
         {register UWORD lenmt; register UBYTE *p;
          lenmt=*p_src++; p=p_dst-(((lenmt&0xF0)<<4)|*p_src++);
          *p_dst++=*p++; *p_dst++=*p++; *p_dst++=*p++;
          lenmt&=0xF; while (lenmt--) *p_dst++=*p++;}
       else
          *p_dst++=*p_src++;
       control>>=1;
      }
   }
 *p_dst_len=p_dst-p_dst_first;
}
