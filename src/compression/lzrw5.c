// modified by Luigi Auriemma
/*
http://www.ross.net/compression/download/original/old_lzrw5.txt
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

#define MAX_NODES         (1L<<16)
#define MAX_PHRASE_LENGTH (255L)
#define ALPHABET_SIZE     (256L)
#define FLAG_BYTES 0

#define MEM_COMPRESS    786752
#define MEM_DECOMPRESS  328000

LOCAL void lzrw5_compress_decompress
           (p_wrk_mem,p_src_first,src_len,p_dst_first,p_dst_len)
/* Input  : Hand over the required amount of working memory in p_wrk_mem.     */
/* Input  : Specify input block using p_src_first and src_len.                */
/* Input  : Point p_dst_first to the start of the output zone.                */
/* Input  : Point p_dst_len to a ULONG to receive the output length.          */
/* Input  : Input block and output zone must not overlap. User knows          */
/* Input  : upperbound on output block length from earlier compression.       */
/* Input  : In any case, maximum expansion possible is nine times.            */
/* Output : Length of output block written to *p_dst_len.                     */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1].       */
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1].       */
UBYTE *p_wrk_mem;
UBYTE *p_src_first;
ULONG  src_len;
UBYTE *p_dst_first;
ULONG *p_dst_len;
{
 UCARD execute_c_code = 1;

 /* Byte pointers p_src and p_dst scan through the input and output blocks.   */
 /* FOLLOWING TWO WERE REGISTER FOR C CODE. */
 UBYTE *p_src = p_src_first+FLAG_BYTES;
 UBYTE *p_dst = p_dst_first;

 /* The following variable is never modified and are used to control          */
 /* the main loop.                                                            */
 UBYTE *p_src_post = p_src_first+src_len;

 UBYTE **pointer;
 UBYTE *length;
 UBYTE *alphabet;
 UBYTE **p_pointer,**p_post_pointer;
 UBYTE *p_length=NULL;

 /* Check the leading copy flag to see if the compressor chose to use a copy  */
 /* operation instead of a compression operation. If a copy operation was     */
 /* used, then all we need to do is copy the data over, set the output length */
 /* and return.                                                               */
/*
 if (*p_src_first==FLAG_COPY)
   {
    fast_copy(p_src_first+FLAG_BYTES,p_dst_first,src_len-FLAG_BYTES);
    *p_dst_len=src_len-FLAG_BYTES;
    return;
   }
*/

 pointer  = (UBYTE **) ULONG_ALIGN_UP(p_wrk_mem);
 alphabet = (UBYTE *)  &pointer[MAX_NODES];
 length   = (UBYTE *)  &alphabet[ALPHABET_SIZE];

 ass( ((p_src_post-p_src)&1)==0,
      "lzrw5_decompress: Input block is of odd length.");
 ass( (((ULONG)p_src)&1)==0,
      "lzrw5_decompress: Input block is odd aligned.");

 //printf("DECOMPRESSOR CALLED\n");

 {
  UCARD i;
  UBYTE **p_init_pointer  = pointer;
  UBYTE  *p_init_alphabet = alphabet;
  UBYTE  *p_init_length   = length;
  UBYTE length_init_value = execute_c_code ? 0 : 255;
  for (i=0;i<ALPHABET_SIZE;i++)
    {
     *p_init_pointer ++ = p_init_alphabet;
     *p_init_alphabet++ = i;
     *p_init_length  ++ = length_init_value;
    }
 }

 p_pointer = p_post_pointer = &pointer[MAX_NODES];

 while (p_src!=p_src_post)
   {
    ULONG items_to_go   = (p_src_post-p_src)>>1;
    ULONG entries_to_go = p_post_pointer-p_pointer;
    ULONG unroll;  /* WAS REGISTER FOR C CODE. */

    if (entries_to_go==0)
       {
        p_pointer=&pointer[ALPHABET_SIZE];
        p_length =&length [ALPHABET_SIZE];
        entries_to_go=p_post_pointer-p_pointer;
       }

    unroll= (items_to_go<entries_to_go) ? items_to_go : entries_to_go;

    ass(unroll>0,"unroll==0.");

    if (execute_c_code)
       while (unroll--)
         {
          register UBYTE *p;
          register UCARD len;
          register UWORD phrase;

          phrase =*p_src++<<8;
          phrase|=*p_src++;
          p=pointer[phrase];
          len=length[phrase];
          *p_pointer++=p_dst;
          *p_length++=len+1;
          do
                {*p_dst++=*p++;}
              while (len--);
         }

   } /* End outer while loop. */

 /* Write the length of the decompressed data before returning. */
 *p_dst_len=p_dst-p_dst_first;

 //printf("EXITING DECOMPRESSOR.\n");
}

