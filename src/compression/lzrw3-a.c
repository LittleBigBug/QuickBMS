// modified by Luigi Auriemma
/*
http://www.ross.net/compression/download/original/old_lzrw3-a.c
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

#define HASH_TABLE_DEPTH_BITS (3)      /* Must be in range [0,12].            */
#define PARTITION_LENGTH_BITS (12-HASH_TABLE_DEPTH_BITS)
#define PARTITION_LENGTH      (1<<PARTITION_LENGTH_BITS)
#define HASH_TABLE_DEPTH      (1<<HASH_TABLE_DEPTH_BITS )
#define HASH_MASK             (PARTITION_LENGTH-1)
#define DEPTH_MASK            (HASH_TABLE_DEPTH-1)
#define FLAG_BYTES 0
#define MAX_CMP_GROUP (2+16*2)
#define START_STRING_18 ((UBYTE *) "123456789012345678")
#define HASH(PTR) \
 ( \
     (((40543*(((*(PTR))<<8)^((*((PTR)+1))<<4)^(*((PTR)+2))))>>4) & HASH_MASK) \
  << HASH_TABLE_DEPTH_BITS \
 )
#define UPDATE_I(I_BASE,NEWPTR) \
{hash[(I_BASE)+cycle++]=(NEWPTR); cycle&=DEPTH_MASK;}
#define U(X)            ((ULONG) X)
#define SIZE_P_BYTE     (U(sizeof(UBYTE *)))
#define ALIGNMENT_FUDGE (U(16))
#define MEM_REQ ( U(4096)*(SIZE_P_BYTE) + ALIGNMENT_FUDGE )

LOCAL void lzrw3a_compress_decompress
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

 /* The hash table is the only resident of the working memory. The hash table */
 /* contains HASH_TABLE_LENGTH=4096 pointers to positions in the history. To  */
 /* keep Macintoshes happy, it is longword aligned.                           */
 UBYTE **hash = (UBYTE **) ULONG_ALIGN_UP(p_wrk_mem);

 /* The variable 'control' is used to buffer the control bits which appear in */
 /* groups of 16 bits (control words) at the start of each compressed group.  */
 /* When each group is read, bit 16 of the register is set to one. Whenever   */
 /* a new bit is needed, the register is shifted right. When the value of the */
 /* register becomes 1, we know that we have reached the end of a group.      */
 /* Initializing the register to 1 thus instructs the code to follow that it  */
 /* should read a new control word immediately.                               */
 register ULONG control=1;

 /* The value of 'literals' is always in the range 0..3. It is the number of  */
 /* consecutive literal items just seen. We have to record this number so as  */
 /* to know when to update the hash table. When literals gets to 3, there     */
 /* have been three consecutive literals and we can update at the position of */
 /* the oldest of the three.                                                  */
 register UCARD literals=0;

 /* The following variable holds the current 'cycle' value. This value cycles */
 /* through the range [0,HASH_TABLE_DEPTH-1], being incremented every time    */
 /* the hash table is updated. The value give the within-partition number of  */
 /* the next pointer to be overwritten. The compressor maintains a cycle      */
 /* value in synchrony.                                                       */
 UCARD cycle=0;

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

 /* Initialize all elements of the hash table to point to a constant string.  */
 /* Use of an unrolled loop speeds this up considerably.                      */
 /* The comment about register declarations above similar code in the         */
 /* compressor applies here too.                                              */
 {UCARD i; UBYTE **p_h=hash;
  #define ZJ *p_h++=START_STRING_18
  for (i=0;i<256;i++)     /* 256=HASH_TABLE_LENGTH/16. */
    {ZJ;ZJ;ZJ;ZJ;
     ZJ;ZJ;ZJ;ZJ;
     ZJ;ZJ;ZJ;ZJ;
     ZJ;ZJ;ZJ;ZJ;}
 }

 /* The outer loop processes either 1 or 16 items per iteration depending on  */
 /* how close p_src is to the end of the input block.                         */
 while (p_src!=p_src_post)
   {/* Start of outer loop */

    register UCARD unroll;   /* Counts unrolled loop executions.              */

    /* When 'control' has the value 1, it means that the 16 buffered control  */
    /* bits that were read in at the start of the current group have all been */
    /* shifted out and that all that is left is the 1 bit that was injected   */
    /* into bit 16 at the start of the current group. When we reach the end   */
    /* of a group, we have to load a new control word and inject a new 1 bit. */
    if (control==1)
      {
       control=0x10000|*p_src++;
       control|=(*p_src++)<<8;
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
         {
          /* Copy item. */

          register UBYTE *p;           /* Points to place from which to copy. */
          register UCARD lenmt;        /* Length of copy item minus three.    */
          register UBYTE *p_ziv=p_dst; /* Pointer to start of current Ziv.    */
          register UCARD index;        /* Index of hash table copy pointer.   */

          /* Read and dismantle the copy word. Work out from where to copy.   */
          lenmt=*p_src++;
          index=((lenmt&0xF0)<<4)|*p_src++;
          p=hash[index];
          lenmt&=0xF;

          /* Now perform the copy using a half unrolled loop. */
          *p_dst++=*p++;
          *p_dst++=*p++;
          *p_dst++=*p++;
          while (lenmt--)
             *p_dst++=*p++;

          /* Because we have just received 3 or more bytes in a copy item     */
          /* (whose bytes we have just installed in the output), we are now   */
          /* in a position to flush all the pending literal hashings that had */
          /* been postponed for lack of bytes.                                */
          if (literals>0)
            {
             register UBYTE *r=p_ziv-literals;;
             UPDATE_I(HASH(r),r);
             if (literals==2)
                {r++; UPDATE_I(HASH(r),r);}
             literals=0;
            }

          /* In any case, we can immediately update the hash table with the   */
          /* current position. We don't need to do a HASH(...) to work out    */
          /* where to put the pointer, as the compressor just told us!!!      */
          UPDATE_I(index&(~DEPTH_MASK),p_ziv);
         }
       else
         {
          /* Literal item. */

          /* Copy over the literal byte. */
          *p_dst++=*p_src++;

          /* If we now have three literals waiting to be hashed into the hash */
          /* table, we can do one of them now (because there are three).      */
          if (++literals == 3)
             {register UBYTE *p=p_dst-3;
              UPDATE_I(HASH(p),p); literals=2;}
         }

       /* Shift the control buffer so the next control bit is in bit 0. */
       control>>=1;

      } /* End unrolled inner loop. */

   } /* End of outer loop */

 /* Write the length of the decompressed data before returning. */
 *p_dst_len=p_dst-p_dst_first;
}
