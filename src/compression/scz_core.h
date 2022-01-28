/************************************************************************
 * SCZ_CORE.c - Core routines for SCZ compression methods.		*
 ************************************************************************/

#ifndef SCZ_CORE
#define SCZ_CORE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SCZ_MAX_BUF  16777215
#define SCZFREELSTSZ 2048
#define nil 0
int sczbuflen = 4 * 1048576;

struct scz_item		/* Data structure for holding buffer items. */
 {
   unsigned char ch;
   struct scz_item *nxt;
 } *sczfreelist1=0;

struct scz_amalgam	/* Data structure for holding markers and phrases. */
 {
   unsigned char phrase[2];
   int value;
 };

struct scz_item2		/* Data structure for holding buffer items with index. */
 {
   unsigned char ch;
   int j;
   struct scz_item2 *nxt;
 } *sczphrasefreq[256], *scztmpphrasefreq, *sczfreelist2=0;

struct scz_block_alloc		/* List for holding references to blocks of allocated memory. */
{
 void *mem_block;
 struct scz_block_alloc *next_block;
} *scz_allocated_blocks = 0, *scz_tmpblockalloc;




struct scz_item *new_scz_item()
{
 int j;
 struct scz_item *tmppt;

 if (sczfreelist1==nil)
  {
   sczfreelist1 = (struct scz_item *)malloc(SCZFREELSTSZ * sizeof(struct scz_item));
   tmppt = sczfreelist1;
   for (j=SCZFREELSTSZ-1; j!=0; j--) 
    {
     tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
     tmppt = tmppt->nxt;
    }
   tmppt->nxt = nil;
   /* Record the block allocation for eventual freeing. */
   scz_tmpblockalloc = (struct scz_block_alloc *)malloc(sizeof(struct scz_block_alloc));
   scz_tmpblockalloc->mem_block = sczfreelist1;
   scz_tmpblockalloc->next_block = scz_allocated_blocks;
   scz_allocated_blocks = scz_tmpblockalloc;
  }
 tmppt = sczfreelist1;
 sczfreelist1 = sczfreelist1->nxt;
 return tmppt;
}

void sczfree( struct scz_item *tmppt )
{
 tmppt->nxt = sczfreelist1;
 sczfreelist1 = tmppt;
}


struct scz_item2 *new_scz_item2()
{
 int j;
 struct scz_item2 *tmppt;

 if (sczfreelist2==nil)
  {
   sczfreelist2 = (struct scz_item2 *)malloc(SCZFREELSTSZ * sizeof(struct scz_item2));
   tmppt = sczfreelist2;
   for (j=SCZFREELSTSZ-1; j!=0; j--) 
    {
     tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
     tmppt = tmppt->nxt;
    }
   tmppt->nxt = nil;
   /* Record the block allocation for eventual freeing. */
   scz_tmpblockalloc = (struct scz_block_alloc *)malloc(sizeof(struct scz_block_alloc));
   scz_tmpblockalloc->mem_block = sczfreelist2;
   scz_tmpblockalloc->next_block = scz_allocated_blocks;
   scz_allocated_blocks = scz_tmpblockalloc;
  }
 tmppt = sczfreelist2;
 sczfreelist2 = sczfreelist2->nxt;
 return tmppt;
}

void sczfree2( struct scz_item2 *tmppt )
{
 tmppt->nxt = sczfreelist2;
 sczfreelist2 = tmppt;
}


void scz_cleanup()	/* Call after last SCZ call to free temporarily allocated */
{			/*  memory which will all be on the free-lists now. */
 while (scz_allocated_blocks!=0)
  {
   scz_tmpblockalloc = scz_allocated_blocks;
   scz_allocated_blocks = scz_allocated_blocks->next_block;
   free(scz_tmpblockalloc->mem_block);
   free(scz_tmpblockalloc);
  }
 sczfreelist1 = nil;
 sczfreelist2 = nil;
}


/*-----------------------*/
/* Add item to a buffer. */
/*-----------------------*/
void scz_add_item( struct scz_item **hd, struct scz_item **tl, unsigned char ch )
{
 struct scz_item *tmppt;

 tmppt = new_scz_item();
 tmppt->ch = ch;
 tmppt->nxt = 0;
 if (*hd==0) *hd = tmppt; else (*tl)->nxt = tmppt;
 *tl = tmppt;
}

#endif
