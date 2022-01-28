// modified by Luigi Auriemma
/*
http://www.ross.net/compression/download/original/old_lzrw2.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fast_copy(A,B,C)    memmove(B,A,C)
#define FLAG_BYTES 0
//#define FLAG_COMPRESS 0     /* Signals that output was result of compression. */
//#define FLAG_COPY     1     /* Signals that output was simply copied over.    */
#define ULONG_ALIGN_UP(X) ((((ULONG)X)+3)&~3)
#define MAX_CMP_GROUP (2+16*2)
#define START_STRING_18 "123456789012345678"
#define LOCAL
typedef unsigned char   UBYTE;
typedef unsigned short  UCARD;
typedef unsigned short  UWORD;
typedef unsigned int    ULONG;

#define U(X)            ((ULONG) X)
#define SIZE_P_BYTE     (U(sizeof(UBYTE *)))
#define SIZE_WORD       (U(sizeof(UWORD  )))
#define ALIGNMENT_FUDGE (U(100))
#define CMP_MEM_REQ ( U(4096)*(SIZE_P_BYTE+SIZE_WORD) + ALIGNMENT_FUDGE )
#define DEC_MEM_REQ ( U(4096)*(SIZE_P_BYTE          ) + ALIGNMENT_FUDGE )

LOCAL void lzrw2_compress_decompress
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
 /* Byte pointers p_src and p_dst scan through the input and output blocks.   */
 register UBYTE *p_src = p_src_first+FLAG_BYTES;
 register UBYTE *p_dst = p_dst_first;

 /* The following two variables are never modified and are used to control    */
 /* the main loop.                                                            */
 UBYTE *p_src_post  = p_src_first+src_len;
 UBYTE *p_src_max16 = p_src_first+src_len-(MAX_CMP_GROUP-2);
 
 /* The variable 'control' is used to buffer the control bits which appear in */
 /* groups of 16 bits (control words) at the start of each compressed group.  */
 /* When each group is read, bit 16 of the register is set to one. Whenever   */
 /* a new bit is needed, the register is shifted right. When the value of the */
 /* register becomes 1, we know that we have reached the end of a group.      */
 /* Initializing the register to 1 thus instructs the code to follow that it  */
 /* should read a new control word immediately.                               */
 register ULONG control=1;
 
 /* The phrase table is the same as in the compressor. The decompressor does  */
 /* not need to maintain a hash table, only a phrase table.                   */
 /* The phrase table is the only occupant of the working memory.              */
 UBYTE **phrase = (UBYTE **) ULONG_ALIGN_UP(p_wrk_mem);
 
 /* The next variable cycles through the phrase table always containing the   */
 /* index of the next phrase pointer to be overwritten in the phrase table.   */
 /* Optimization note: I tried using a pointer to cycle through the table but */
 /* this went more slowly than using an explicit index.                       */
 register UWORD next=0;
      
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
   
 /* Whereas the compressor needs to maintain a hash table and a phrase table  */
 /* the decompressor needs to maintain only the phrase table. Only the first  */
 /* entry of the phrase table needs initializing as, apart from this entry,   */
 /* the compressor guarantees not to refer to a table entry until the entry   */
 /* has been written.                                                         */
 phrase[0]=(UBYTE *) START_STRING_18;
    
 /* The outer loop processes either 1 or 16 items per iteration depending on  */
 /* how close p_src is to the end of the input block.                         */
 while (p_src!=p_src_post)
   {/* Start of outer loop */
   
    register UWORD unroll;   /* Counts unrolled loop executions.              */
    
    /* When 'control' has the value 1, it means that the 16 buffered control  */
    /* bits that were read in at the start of the current group have all been */
    /* shifted out and that all that is left is the 1 bit that was injected   */
    /* into bit 16 at the start of the current group. When we reach the end   */
    /* of a group, we have to load a new control word and inject a new 1 bit. */
    if (control==1)
      {
       control=0x10000|*p_src++;
       control|=(*p_src++)<<8;
      
       /* Because 4096 (the number of entries in the phrase table) is a       */
       /* multiple of 16 (the loop unrolling), and 'unroll' has the value 1   */
       /* or 16 and never increases its initial value, this wraparound check  */
       /* need only be done once per main loop. In fact it can even reside    */
       /* within this 'if'.                                                   */
       next&=0xFFF;
      }

    /* If it is possible that we are within 16 groups from the end of the     */
    /* input, execute the unrolled loop only once, else process a whole group */
    /* of 16 items by looping 16 times.                                       */
    unroll= p_src<=p_src_max16 ? 16 : 1;

    /* This inner loop processes one phrase (item) per iteration. */
    while (unroll--)
      { /* Begin unrolled inner loop. */
      
       /* Process a literal or copy item depending on the next control bit. */
       if (control&1)
         { /* Copy item. */
          register UWORD lenmt; /* Length of copy item minus three.           */
          register UBYTE *p;    /* Points to history posn from which to copy. */
          
          /* Read and dismantle the copy word. Work out from where to copy.   */
          lenmt=*p_src++;
          p=phrase[((lenmt&0xF0)<<4)|*p_src++];
          lenmt&=0xF;

          /* Update the phrase table. Don't do this before p=phrase[...]. */
          phrase[next++]=p_dst;
          
          /* Now perform the copy using a half unrolled loop. */
          *p_dst++=*p++;
          *p_dst++=*p++;
          *p_dst++=*p++;
          while (lenmt--)
             *p_dst++=*p++;
         }
       else
         { /* Literal item. */
          phrase[next++]=p_dst;  /* Update the phrase table.    */
          *p_dst++=*p_src++;     /* Copy over the literal byte. */
          }
          
       /* Shift the control buffer so the next control bit is in bit 0. */
       control>>=1;
       
      } /* End unrolled inner loop. */
               
   } /* End of outer loop */
   
 /* Write the length of the decompressed data before returning. */
 *p_dst_len=p_dst-p_dst_first;
}

