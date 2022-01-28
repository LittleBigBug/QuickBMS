/*
dblspace_dec.c

DMSDOS CVF-FAT module: [dbl|drv]space cluster read and decompression routines.

******************************************************************************
DMSDOS (compressed MSDOS filesystem support) for Linux
written 1995-1998 by Frank Gockel and Pavel Pisa

    (C) Copyright 1995-1998 by Frank Gockel
    (C) Copyright 1996-1998 by Pavel Pisa

Some code of dmsdos has been copied from the msdos filesystem
so there are the following additional copyrights:

    (C) Copyright 1992,1993 by Werner Almesberger (msdos filesystem)
    (C) Copyright 1994,1995 by Jacques Gelinas (mmap code)
    (C) Copyright 1992-1995 by Linus Torvalds

DMSDOS was inspired by the THS filesystem (a simple doublespace
DS-0-2 compressed read-only filesystem) written 1994 by Thomas Scheuermann.

The DMSDOS code is distributed under the Gnu General Public Licence.
See file COPYING for details.
******************************************************************************

*/

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/locks.h>
#include <linux/fs.h>
#include <linux/malloc.h>
#include <linux/msdos_fs.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/byteorder.h>
#endif

#include "dmsdos.h"

#ifdef __DMSDOS_LIB__
/* some interface hacks */
#include"lib_interface.h"
#include<malloc.h>
#include<string.h>
#include<errno.h>
#endif

#ifdef __GNUC__
#define INLINE static inline
#else
/* non-gnu compilers may not like inline */
#define INLINE static
#endif

/* we always need DS decompression */

#if defined(__GNUC__) && defined(__i386__) && defined(USE_ASM)
#define USE_GNU_ASM_i386

/* copy block, overlaping part is replaced by repeat of previous part */
/* pointers and counter are modified to point after block */
#define M_MOVSB(D,S,C) \
__asm__ /*__volatile__*/(\
	"cld\n\t" \
	"rep\n\t" \
	"movsb\n" \
	:"=D" (D),"=S" (S),"=c" (C) \
	:"0" (D),"1" (S),"2" (C) \
	:"memory")


#else

#ifdef __GNUC__
/* non-gnu compilers may not like warning directive */
//#warning USE_GNU_ASM_I386 not defined, using "C" equivalent
#endif

#define M_MOVSB(D,S,C) for(;(C);(C)--) *((__u8*)(D)++)=*((__u8*)(S)++)

#endif

#if !defined(le16_to_cpu)
    /* for old kernel versions - works only on i386 */
    #define le16_to_cpu(v) (v)
#endif

/* for reading and writting from/to bitstream */
typedef
 struct {
   __u32 buf;	/* bit buffer */
     int pb;	/* already readed bits from buf */
   __u16 *pd;	/* first not readed input data */
   __u16 *pe;	/* after end of data */
 } bits_t;

static
const unsigned dblb_bmsk[]=
   {0x0,0x1,0x3,0x7,0xF,0x1F,0x3F,0x7F,0xFF,
    0x1FF,0x3FF,0x7FF,0xFFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF};

/* read next 16 bits from input */
#define RDN_G16(bits) \
   { \
    (bits).buf>>=16; \
    (bits).pb-=16; \
    if((bits).pd<(bits).pe) \
    { \
     (bits).buf|=((__u32)(le16_to_cpu(*((bits).pd++))))<<16; \
    }; \
   }

/* prepares at least 16 bits for reading */
#define RDN_PR(bits,u) \
   { \
    if((bits).pb>=16) RDN_G16(bits); \
    u=(bits).buf>>(bits).pb; \
   }

/* initializes reading from bitstream */
INLINE void dblb_rdi(bits_t *pbits,void *pin,unsigned lin)
{
  pbits->pb=32;
  pbits->pd=(__u16*)pin;
  pbits->pe=pbits->pd+((lin+1)>>1);
    pbits->buf=0;
}

/* reads n<=16 bits from bitstream *pbits */
INLINE unsigned dblb_rdn(bits_t *pbits,int n)
{
  unsigned u;
  RDN_PR(*pbits,u);
  pbits->pb+=n;
  u&=dblb_bmsk[n];
  return u;
}

INLINE int dblb_rdoffs(bits_t *pbits)
{ unsigned u;
  RDN_PR(*pbits,u);
  switch (u&3)
  {
    case 0: case 2:
      pbits->pb+=1+6;  return 63&(u>>1);
    case 1:
      pbits->pb+=2+8;  return (255&(u>>2))+64;
  }
  pbits->pb+=2+12; return (4095&(u>>2))+320;
}

INLINE int dblb_rdlen(bits_t *pbits)
{ unsigned u;
  RDN_PR(*pbits,u);
  switch (u&15)
  { case  1: case  3: case  5: case  7:
    case  9: case 11: case 13: case 15:
      pbits->pb++;     return 3;
    case  2: case  6:
    case 10: case 14:
      pbits->pb+=2+1;  return (1&(u>>2))+4;
    case  4: case 12:
      pbits->pb+=3+2;  return (3&(u>>3))+6;
    case  8:
      pbits->pb+=4+3;  return (7&(u>>4))+10;
    case  0: ;
  }
  switch ((u>>4)&15)
  { case  1: case  3: case  5: case  7:
    case  9: case 11: case 13: case 15:
      pbits->pb+=5+4;  return (15&(u>>5))+18;
    case  2: case  6:
    case 10: case 14:
      pbits->pb+=6+5;  return (31&(u>>6))+34;
    case  4: case 12:
      pbits->pb+=7+6;  return (63&(u>>7))+66;
    case  8:
      pbits->pb+=8+7;  return (127&(u>>8))+130;
    case  0: ;
  }
  pbits->pb+=9;
  if(u&256) return dblb_rdn(pbits,8)+258;
  return -1;
}

INLINE int dblb_decrep(bits_t *pbits, __u8 **p, void *pout, __u8 *pend,
		 int repoffs, int k, int flg)
{ int replen;
  __u8 *r;

  if(repoffs==0){LOG_DECOMP("DMSDOS: decrb: zero offset ?\n");return -2;}
  if(repoffs==0x113f)
  { 
    int pos=*p-(__u8*)pout;
    LOG_DECOMP("DMSDOS: decrb: 0x113f sync found.\n");
    if((pos%512) && !(flg&0x4000))
    { LOG_DECOMP("DMSDOS: decrb: sync at decompressed pos %d ?\n",pos);
      return -2;
    }
    return 0;
  }
  replen=dblb_rdlen(pbits)+k;

  if(replen<=0)
    {LOG_DECOMP("DMSDOS: decrb: illegal count ?\n");return -2;}
  if((__u8*)pout+repoffs>*p)
    {LOG_DECOMP("DMSDOS: decrb: of>pos ?\n");return -2;}
  if(*p+replen>pend)
    {LOG_DECOMP("DMSDOS: decrb: output overfill ?\n");return -2;}
  r=*p-repoffs;
  M_MOVSB(*p,r,replen);
  return 0;
}

/* DS decompression */
/* flg=0x4000 is used, when called from stacker_dec.c, because of
   stacker does not store original cluster size and it can mean,
   that last cluster in file can be ended by garbage */
int ds_dec(void* pin,int lin, void* pout, int lout, int flg)
{ 
  __u8 *p, *pend;
  unsigned u, repoffs;
  int r;
  bits_t bits;

  dblb_rdi(&bits,pin,lin);
  p=(__u8*)pout;pend=p+lout;
  if((dblb_rdn(&bits,16))!=0x5344) return -1;
    
  u=dblb_rdn(&bits,16);
  LOG_DECOMP("DMSDOS: DS decompression version %d\n",u);
  
  do
  { r=0;
    RDN_PR(bits,u);
    switch(u&3)
    {
      case 0:
	bits.pb+=2+6;
	repoffs=(u>>2)&63;
	r=dblb_decrep(&bits,&p,pout,pend,repoffs,-1,flg);
	break;
      case 1:
	bits.pb+=2+7;
	*(p++)=(u>>2)|128;
	break;
      case 2:
	bits.pb+=2+7;
	*(p++)=(u>>2)&127;
	break;
      case 3:
	if(u&4) {  bits.pb+=3+12; repoffs=((u>>3)&4095)+320; }
	else  {  bits.pb+=3+8;  repoffs=((u>>3)&255)+64; };
	r=dblb_decrep(&bits,&p,pout,pend,repoffs,-1,flg);
	break;
    }
  }while((r==0)&&(p<pend));
  
  if(r<0) return r;

  if(!(flg&0x4000))
  { 
    u=dblb_rdn(&bits,3);if(u==7) u=dblb_rdn(&bits,12)+320;
    if(u!=0x113f)
    { LOG_DECOMP("DMSDOS: decrb: final sync not found?\n");
      return -2;
    }
  }

  return p-(__u8*)pout;
}

/* JM decompression */
int jm_dec(void* pin,int lin, void* pout, int lout, int flg)
{ 
  __u8 *p, *pend;
  unsigned u, repoffs;
  int r;
  bits_t bits;

  dblb_rdi(&bits,pin,lin);
  p=(__u8*)pout;pend=p+lout;
  if((dblb_rdn(&bits,16))!=0x4D4A) return -1;

  u=dblb_rdn(&bits,16);
  LOG_DECOMP("DMSDOS: JM decompression version %d\n",u);

  do
  { r=0;
    RDN_PR(bits,u);
    switch(u&3)
    {
      case 0:
      case 2:
	bits.pb+=8;
	*(p++)=(u>>1)&127;
	break;
      case 1:
	bits.pb+=2;
	repoffs=dblb_rdoffs(&bits);
	r=dblb_decrep(&bits,&p,pout,pend,repoffs,0,flg);
	break;
      case 3:
	bits.pb+=9;
	*(p++)=((u>>2)&127)|128;
	break;
    }
  }
  while((r==0)&&(p<pend));

  if(r<0) return r;

  if(!(flg&0x4000))
  { 
    u=dblb_rdn(&bits,2);if(u==1) u=dblb_rdoffs(&bits);
    if(u!=0x113f)
    { LOG_DECOMP("DMSDOS: decrb: final sync not found?\n");
      return -2;
    }
  }

  return p-(__u8*)pout;
}

