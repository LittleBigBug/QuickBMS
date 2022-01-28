/*
dmsdos.h

DMSDOS CVF-FAT module: declaration of dmsdos functions and structures.

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

#ifndef _DMSDOS_H
#define _DMSDOS_H

/* version number hacks */
#define LVC(x,y,z) ((x)*65536+(y)*256+(z))

#ifdef __KERNEL__

#ifndef LINUX_VERSION_CODE
 #include<linux/version.h>
#endif
#if LINUX_VERSION_CODE == LVC(2,2,1)
 /* this works around a bug in Linux 2.2.1 */
 #include<asm/page.h>
#endif
#include<asm/semaphore.h>
#include<linux/fs.h>
#if LINUX_VERSION_CODE < LVC(2,3,3)
 #define init_MUTEX(sem) (*sem=MUTEX)
 #define DECLARE_MUTEX(name) struct semaphore name=MUTEX
 #define DECLARE_WAIT_QUEUE_HEAD(name) \
  	struct wait_queue * name=NULL
#endif

#if LINUX_VERSION_CODE >= LVC(2,1,78)
 #define __FOR_KERNEL_2_1_80
 #if LINUX_VERSION_CODE >= LVC(2,1,80)
  #define FAT_GET_CLUSTER
 #else
  #define READPAGE_DENTRY
 #endif
#endif
#if (LINUX_VERSION_CODE >= LVC(2,1,0)) && (LINUX_VERSION_CODE < LVC(2,1,78))
 #error dmsdos 0.9.x needs kernel >= 2.1.80 or use 2.0.33
#endif

#if LINUX_VERSION_CODE >= LVC(2,3,99)
#define __FOR_KERNEL_2_3_99
#define HAS_SB_CLUSTER_BITS
#endif

#ifdef FAT_GET_CLUSTER
#define get_cluster fat_get_cluster
#endif

#endif /* __KERNEL__*/

#include "dmsdos-config.h"
/* hacks for new Configure */
#ifndef DMSDOS_EXPERT
#define USE_VMALLOC
#define LISTSIZE 1024
#define MDFATCACHESIZE 40
#define DFATCACHESIZE 20
#define BITFATCACHESIZE 10
#define MAX_CACHE_TIME 60
#define CCACHESIZE 64
#define MAX_CCACHE_TIME 240
#define MAX_READA 64
#define USE_READA_LIST
#define READA_LIST_SIZE 256
#define READA_THRESHOLD 4095
#define DEFAULT_LOGLEVEL 0
#define DEFAULT_CF 11
#define DMSDOS_USE_READPAGE
#define IDMSDOSD_TIME 30
#define SP_BIT0 /* never compress dir */
#define SP_BIT1 /* never compress EMD */
#define SP_BIT4 /* write-back caching */
#define SP_BIT5 /* read-ahead */
#define SP_BIT7 /* daemon compresses */
#endif /* DMSDOS_EXPERT */

#ifndef SP_BIT0
#define SP_BIT0 0
#else
#undef SP_BIT0
#define SP_BIT0 1
#endif

#ifndef SP_BIT1
#define SP_BIT1 0
#else
#undef SP_BIT1
#define SP_BIT1 1
#endif

#ifndef SP_BIT2
#define SP_BIT2 0
#else
#undef SP_BIT2
#define SP_BIT2 1
#endif

#ifndef SP_BIT3
#define SP_BIT3 0
#else
#undef SP_BIT3
#define SP_BIT3 1
#endif

#ifndef SP_BIT4
#define SP_BIT4 0
#else
#undef SP_BIT4
#define SP_BIT4 1
#endif

#ifndef SP_BIT5
#define SP_BIT5 0
#else
#undef SP_BIT5
#define SP_BIT5 1
#endif

#ifndef SP_BIT6
#define SP_BIT6 0
#else
#undef SP_BIT6
#define SP_BIT6 1
#endif

#ifndef SP_BIT7
#define SP_BIT7 0
#else
#undef SP_BIT7
#define SP_BIT7 1
#endif

#ifndef SP_BIT8
#define SP_BIT8 0
#else
#undef SP_BIT8
#define SP_BIT8 1
#endif

#ifndef DEFAULT_SPEEDUP
#define DEFAULT_SPEEDUP ((SP_BIT8<<8)|(SP_BIT7<<7)|(SP_BIT6<<6)|(SP_BIT5<<5)|(SP_BIT4<<4)|(SP_BIT3<<3)|(SP_BIT2<<2)|(SP_BIT1<<1)|(SP_BIT0))
#endif
#ifndef DEFAULT_COMP
#define DEFAULT_COMP GUESS
#endif
#ifndef LISTSIZE
#define LISTSIZE 1024
#endif
#ifndef MDFATCACHESIZE
#define MDFATCACHESIZE 40
#endif
#ifndef DFATCACHESIZE
#define DFATCACHESIZE 20
#endif
#ifndef BITFATCACHESIZE
#define BITFATCACHESIZE 10
#endif
#ifndef MAX_CACHE_TIME
#define MAX_CACHE_TIME 60
#endif
#ifndef CCACHESIZE
#define CCACHESIZE 64
#endif
#ifndef MAX_CCACHE_TIME
#define MAX_CCACHE_TIME 240
#endif
#ifndef MAX_READA
#define MAX_READA 64
#endif
#ifndef READA_LIST_SIZE
#define READA_LIST_SIZE 256
#endif
#ifndef READA_THRESHOLD
#define READA_THRESHOLD 4095
#endif
#ifndef DEFAULT_LOGLEVEL
#define DEFAULT_LOGLEVEL 0
#endif
#ifndef DEFAULT_CF
#define DEFAULT_CF 11
#endif
#ifndef IDMSDOSD_TIME
#define IDMSDOSD_TIME 30
#endif


#define DMSDOS_MAJOR      0
#define DMSDOS_MINOR      9
#define DMSDOS_ACT_REL    2
#define DMSDOS_COMP_REL   2
#define DMSDOS_PL         "3"
#define DMSDOS_EXTRA      "-pre2(alpha test)"

#define DMSDOS_VERSION ((DMSDOS_MAJOR<<16)|(DMSDOS_MINOR<<8)|DMSDOS_ACT_REL)
#define DMSDOS_LOWEST_COMPATIBLE_VERSION ((DMSDOS_MAJOR<<16)|(DMSDOS_MINOR<<8)|DMSDOS_COMP_REL)
#define DMSDOS_VLT "pl" DMSDOS_PL DMSDOS_EXTRA

/* config hacks */
#if (defined(DMSDOS_CONFIG_DBLSP_DRVSP) || defined(DMSDOS_CONFIG_DRVSP3))
#define DMSDOS_CONFIG_DBL
#endif
#if (defined(DMSDOS_CONFIG_STAC3) || defined(DMSDOS_CONFIG_STAC4))
#define DMSDOS_CONFIG_STAC
#endif
#if (!defined(DMSDOS_CONFIG_DBL) && !defined(DMSDOS_CONFIG_STAC))
#error configuration: no CVF format to compile in !!!
#endif

/* known compression methods */
#define DS_0_0 0x00005344
#define DS_0_1 0x01005344
#define DS_0_2 0x02005344
#define JM_0_0 0x00004D4A
/* drivespace 3 high compression */ 
#define JM_0_1 0x01004D4A
/* drivespace 3 ultra compression */
#define SQ_0_0 0x00005153
/* stacker 3 compression (no header) */
#define SD_3 0x00000000
/* stacker 4 compression */
#define SD_4 0x00000081

/* other defines for options */
#define READ_ONLY -1
#define UNCOMPRESSED -2
#define GUESS -3

#define MIN_FREE_SECTORS ( (dblsb->s_cvf_version==DRVSP3              \
                            &&(dmsdos_speedup&SP_NO_FRAG_WRITE)==0)  \
                           ?dblsb->s_sectperclust+1                   \
                           :10*dblsb->s_sectperclust )

typedef struct
{ unsigned long free_sectors;
  unsigned long used_sectors;
  unsigned long max_hole;
  unsigned long free_clusters;
  unsigned long used_clusters;
  unsigned long lost_clusters;
  unsigned long sectors_lo;
  unsigned long sectors_hi;
  unsigned long compressed_clusters;
  unsigned long uncompressed_clusters;
} Dblstat;                                                            

typedef struct
{ long sector_minus_1;
  short size_lo_minus_1;
  short size_hi_minus_1;
  short unknown; /* some bits I don't know to handle.... */
  short flags; /* 00...0uc - u=used, c=compressed */ 
} Mdfat_entry;

/* flag values */
#define D_EMPTY       0
#define D_VALID       1    /* entry is valid -> cluster to be compressed */
#define D_IN_D_ACTION 2    /* is being compressed by daemon */
#define D_OVERWRITTEN 3    /* has been overwritten by dmsdos while daemon
                            is compressing it -> throw away the result from
                            the daemon */
#ifdef __KERNEL__
# ifdef USE_XMALLOC
   void* xmalloc(unsigned long);
   void xfree(void*);
#  define MALLOC(x) xmalloc(x)
#  define FREE(x) xfree(x)
# else
#  ifdef USE_VMALLOC
#   include<linux/mm.h>
#   ifdef __FOR_KERNEL_2_1_80
#    include<linux/vmalloc.h>
#   endif
#   define MALLOC(x) vmalloc(x)
#   define FREE(x) vfree(x)
#  else
#   include<linux/malloc.h>
#   define MALLOC(x) kmalloc(x,GFP_KERNEL)
#   define FREE(x) kfree(x)
#  endif
# endif
#endif

/* this must be known outside the kernel too */
typedef struct {
  int s_dcluster;/*[45-46]*/
  int s_mdfatstart;/*[36-37]+1*/
  int s_fatstart;/*[39-40]+[14-15]*/ 
  int s_rootdir;/*[41-42]+[39-40]*/
  int s_rootdirentries;
  int s_sectperclust;
  int s_spc_bits;
  int s_16bitfat;
  int s_datastart;
  int s_dataend;
  int s_comp;
  int s_bootblock;/*[39-40]*/
  int s_cfaktor;
  int s_full;
  int s_max_cluster;
  int s_max_cluster2;
  int s_cvf_version;   /* dblsp/drvsp/drvsp3/stac3/stac4 */
  int s_2nd_fat_offset;
  int s_lastnear;
  int s_lastbig;
  int s_free_sectors;
  void * mdfat_alloc_semp;
} Dblsb;

#define DBLSP   0
#define DRVSP   1
#define DRVSP3  2
#define STAC3   3
#define STAC4   4

#define UC_NORMAL 0
#define UC_UNCOMPR 1
#define UC_TEST 2
#define UC_DIRECT 3

/* cvf version capabilities - boolean values */
#define DIR_MAY_BE_SHORT(dblsb)       (dblsb->s_cvf_version==DRVSP3)
#define DIR_MAY_BE_COMPRESSED(dblsb)  (dblsb->s_cvf_version>=DRVSP3&&(dmsdos_speedup&SP_NO_DIR_COMPR)==0)
#define UMOUNT_UCFLAG ((dmsdos_speedup&SP_NO_UNMOUNT_COMPR)?UC_UNCOMPR:UC_DIRECT)

typedef struct {
  struct buffer_head * a_buffer;
  unsigned int a_area;
  unsigned long a_time;
  struct super_block* a_sb;
  unsigned int a_acc;
} Acache;

#define C_FREE 0                /* cluster cache entry is free */
#define C_VALID 1               /* data points to valid cluster data */
#define C_DIRTY_NORMAL 2        /* like VALID but data need to be written */
#define C_DIRTY_UNCOMPR 3       /* like VALID but data need to be written */
#define C_DELETED 4             /* like VALID but last request was a delete */
#define C_INVALID 5             /* data are junk but valid memory adress */
#define C_NOT_MALLOCD 6         /* data pointer is not a valid address */ 

#define C_KEEP_LOCK 1
#define C_NO_READ 2

#ifdef __KERNEL__
typedef struct {
  unsigned int c_time;
  unsigned int c_flags;
  unsigned int c_count;
  unsigned int c_length;
  unsigned int c_clusternr;
  struct super_block* c_sb;
  unsigned char* c_data;
  struct semaphore c_sem;
  unsigned int c_errors;
} Cluster_head;
#endif /*__KERNEL__*/

int dbl_mdfat_value(struct super_block*sb, int clusternr,
                     Mdfat_entry*new,Mdfat_entry*mde);
int dbl_fat_nextcluster(struct super_block*sb,int clusternr,int*);
int dbl_bitfat_value(struct super_block*sb,int sektornr,int*);
void exit_dbl(struct super_block*sb);
int find_free_bitfat(struct super_block*sb, int sektornr, int size);
int dbl_replace_existing_cluster(struct super_block*sb, int cluster, 
                             int near_sector,
                             Mdfat_entry*,unsigned char*);
int stac_replace_existing_cluster(struct super_block*sb, int cluster, 
                             int near_sector,
                             Mdfat_entry*);
int dbl_compress(unsigned char* clusterk, unsigned char* clusterd, 
                      int size, int method,int);
#if 0
int stac_compress(void* pin,int lin, void* pout, int lout,
                  int method, int cfaktor);
#else
int stac_compress(unsigned char* pin,int lin, unsigned char* pout, int lout,
                  int method, int cfaktor);
#endif
int sq_comp(void* pin,int lin, void* pout, int lout, int flg);
int dbl_decompress(unsigned char*clusterd, unsigned char*clusterk,
               Mdfat_entry*mde);
int dmsdos_write_cluster(struct super_block*sb,
                         unsigned char* clusterd, int length, int clusternr,
                         int near_sector, int ucflag);

#define CHS(i) ( (unsigned short)i[0]|(unsigned short)i[1]<<8 )
#define CHL(i) ( (unsigned long)i[0]|(unsigned long)i[1]<<8| \
                 (unsigned long)i[2]<<16|(unsigned long)i[3]<<24 )

int dbl_mdfat_cluster2sector(struct super_block*sb,int clusternr);
int simple_check(struct super_block*sb,int repair);
void do_spc_init(void);
void do_spc_exit(void);
void lock_mdfat_alloc(Dblsb*);
void unlock_mdfat_alloc(Dblsb*);
void free_cluster_sectors(struct super_block*sb, int clusternr);
void stac_special_free(struct super_block*sb, int clusternr);
int stac_write_cluster(struct super_block*sb,
                       unsigned char* clusterd, int length, int clusternr,
                       int near_sector, int ucflag);
int stac_read_cluster(struct super_block*sb,unsigned char*clusterd,
                      int clusternr);
void free_idle_cache(void);
void free_idle_ccache(void);
void ccache_init(void);
void free_ccache_dev(struct super_block*sb);
#ifdef __KERNEL__
Cluster_head* ch_read(struct super_block*sb,int clusternr,int flag);
Cluster_head* find_in_ccache(struct super_block*sb,
                             int clusternr,Cluster_head**lastfree,
                             Cluster_head**oldest);
int ch_dirty(Cluster_head*,int near,int ucflag);
int ch_dirty_locked(Cluster_head*,int near,int ucflag);
void lock_ch(Cluster_head*);
void unlock_ch(Cluster_head*);
void ch_dirty_retry_until_success(Cluster_head*,int near,int ucflag);
void ch_free(Cluster_head*);
#endif
void sync_cluster_cache(int allow_daemon);
void delete_cache_cluster(struct super_block*sb, int clusternr);
void log_list_statistics(void);
void log_ccache_statistics(void);
void log_found_statistics(void);
int sq_dec(void* pin,int lin, void* pout, int lout, int flg);
                                               
/* Stacker cluster allocation types access */

#if defined(__KERNEL__)||defined(__DMSDOS_LIB__)
/* structure for walking/working with each sector of cluster */
typedef struct {
  struct super_block*sb;
  int clusternr;
  int start_sect;
  int start_len;
  int flags;
  int sect_cnt;
  int compressed;
  int bytes_in_last;
  int bytes_in_clust;
  struct buffer_head *fbh; /* first sector of fragmented clust */
  /* changes during fragments reads */
  int fcnt;	/* count of unreaded fragments */
  int flen;	/* rest sectors in fragment */
  int sect;	/* actual sector */
  int offset;	/* byte offset in sector */
  int bytes;	/* number of data bytes in sector */
  unsigned char* finfo;	/* points to actual field in fbh */
} Stac_cwalk;

int stac_cwalk_init(Stac_cwalk *cw,struct super_block*sb,
		 int clusternr,int flg);
int stac_cwalk_sector(Stac_cwalk *cw);
void stac_cwalk_done(Stac_cwalk *cw);
#endif /* __KERNEL__||__DMSDOS_LIB__*/

/* loglevel defines */

//extern unsigned long loglevel;

#ifdef SEQLOG
int log_prseq(void);
#define LOGCMD if(log_prseq())printk
#else
#define LOGCMD printk
#endif

#ifndef NOLOG
#define LOG_FS      if(loglevel&0x00000001)LOGCMD
#define LOG_CLUST   if(loglevel&0x00000002)LOGCMD
#define LOG_LLRW    if(loglevel&0x00000008)LOGCMD
#define LOG_DFAT    if(loglevel&0x00000010)LOGCMD
#define LOG_MDFAT   if(loglevel&0x00000020)LOGCMD
#define LOG_BITFAT  if(loglevel&0x00000040)LOGCMD
#define LOG_DECOMP  if(loglevel&0x00000080)LOGCMD
#define LOG_COMP    if(loglevel&0x00000100)LOGCMD
#define LOG_ALLOC   if(loglevel&0x00000200)LOGCMD
#define LOG_DAEMON  if(loglevel&0x00000400)LOGCMD
#define LOG_CCACHE  if(loglevel&0x00000800)LOGCMD
#define LOG_ACACHE  if(loglevel&0x00001000)LOGCMD
#define LOG_REST    if(loglevel&0x80000000)LOGCMD
#else
#define LOG_FS(x,args...)
#define LOG_CLUST(x,args...)
#define LOG_LLRW(x,args...)
#define LOG_DFAT(x,args...)
#define LOG_MDFAT(x,args...)
#define LOG_BITFAT(x,args...)
#define LOG_DECOMP(x,args...)
#define LOG_COMP(x,args...)
#define LOG_ALLOC(x,args...)
#define LOG_DAEMON(x,args...)
#define LOG_CCACHE(x,args...)
#define LOG_ACACHE(x,args...)
#define LOG_REST(x,args...)
#endif

#ifdef __FOR_KERNEL_2_1_80
/* some hacks since the memcpy_from/tofs functions have changed here */
#include  <asm/uaccess.h>
#define memcpy_fromfs copy_from_user
#define memcpy_tofs copy_to_user
#endif

struct buffer_head *raw_bread (
        struct super_block *sb,
        int block);
struct buffer_head *raw_getblk (
        struct super_block *sb,
        int block);
void raw_brelse (
        struct super_block *sb,
        struct buffer_head *bh);
void raw_mark_buffer_dirty (
        struct super_block *sb,
        struct buffer_head *bh,
        int dirty_val);
void raw_set_uptodate (
        struct super_block *sb,
        struct buffer_head *bh,
        int val);
int raw_is_uptodate (
        struct super_block *sb,
        struct buffer_head *bh);
void raw_ll_rw_block (
        struct super_block *sb,
        int opr,
        int nbreq,
        struct buffer_head *bh[32]);

#define FAKED_ROOT_DIR_OFFSET 1
#define FAKED_DATA_START_OFFSET 1000
int dmsdos_read_cluster(struct super_block*sb,
                        unsigned char*clusterd, int clusternr);

struct buffer_head* dblspace_bread(struct super_block*sb,int vsector);
struct buffer_head *dblspace_getblk (
        struct super_block *sb,
        int block);
void dblspace_brelse(struct super_block* sb,struct buffer_head*bh);
void dblspace_mark_buffer_dirty(struct super_block*sb,struct buffer_head*bh,
                                int dirty_val);
void dblspace_set_uptodate (
        struct super_block *sb,
        struct buffer_head *bh,
        int val);
int dblspace_is_uptodate (
        struct super_block *sb,
        struct buffer_head *bh);
void dblspace_ll_rw_block (
        struct super_block *sb,
        int opr,
        int nbreq,
        struct buffer_head *bh[32]);
int stac_bitfat_state(struct super_block*sb,int new_state);
int dblspace_fat_access(struct super_block*sb, int clusternr,int newval);
#ifdef __KERNEL__
int dblspace_bmap(struct inode*inode, int block);
int dblspace_smap(struct inode*inode, int block);
#endif
int ds_dec(void* pin,int lin, void* pout, int lout, int flg);
#ifdef __KERNEL__
void dblspace_zero_new_cluster(struct inode*,int clusternr);
#ifdef __FOR_KERNEL_2_1_80
ssize_t dblspace_file_read(struct file *filp,char *buf,size_t count,
                           loff_t *ppos);
ssize_t dblspace_file_write(struct file *filp,const char *buf,size_t count,
                           loff_t *ppos);
int dblspace_mmap(struct file*file,
                  struct vm_area_struct*vma);
#else
int dblspace_file_read(struct inode *inode,struct file *filp,char *buf,
                                int count);
int dblspace_file_write(struct inode *inode,struct file *filp,const char *buf,
                                int count);
int dblspace_mmap(struct inode*inode,struct file*file,
                  struct vm_area_struct*vma);
#endif
#ifdef READPAGE_DENTRY
int dblspace_readpage(struct dentry*dentry, struct page *page);
#else
int dblspace_readpage(struct inode *inode, struct page *page);
#endif
int dmsdos_ioctl_dir(struct inode *dir,struct file *filp,
                     unsigned int cmd, unsigned long data);
#endif /* __KERNEL__ */
int try_daemon(struct super_block*sb,int clusternr, int length, int method);
void remove_from_daemon_list(struct super_block*sb,int clusternr);
void force_exit_daemon(void);
void dblspace_reada(struct super_block*sb, int sector,int count);
void init_reada_list(void);
void kill_reada_list_dev(int dev);
int daemon_write_cluster(struct super_block*sb,unsigned char*data,
                         int len, int clusternr, int rawlen);
void check_free_sectors(struct super_block*sb);
void get_memory_usage_acache(int*, int*max);
void get_memory_usage_ccache(int*, int*max);
int mount_dblspace(struct super_block*sb,char*options);
int mount_stacker(struct super_block*sb,char*options);
int detect_dblspace(struct super_block*sb);
int detect_stacker(struct super_block*sb);
int unmount_dblspace(struct super_block*sb);

typedef struct
{ int clusternr;
  struct super_block*sb;
  int length;   /* in bytes */
  char flag;
  int method;
} Rwlist;

void init_daemon(void);
void exit_daemon(void);
void clear_list_dev(struct super_block*sb);

/* speedup bits */
#define SP_NO_DIR_COMPR 0x0001
#define SP_NO_EMD_COMPR 0x0002
#define SP_NO_EXACT_SEARCH 0x0004
#define SP_NO_UNMOUNT_COMPR 0x0008
#define SP_USE_WRITE_BACK 0x0010
#define SP_USE_READ_AHEAD 0x0020
#define SP_FAST_BITFAT_ALLOC 0x0040
#define SP_USE_DAEMON 0x0080
#define SP_NO_FRAG_WRITE 0x0100

typedef struct
{ int ccachebytes;
  int max_ccachebytes;
  int acachebytes;
  int max_acachebytes;
} Memuse;


#define DMSDOS_IOCTL_MIN 0x2000
#define DMSDOS_IOCTL_MAX 0x201F
#define DMSDOS_GET_DBLSB 0x2000
#define DMSDOS_EXTRA_STATFS 0x2001
#define DMSDOS_READ_BLOCK 0x2002
#define DMSDOS_WRITE_BLOCK 0x2003
#define DMSDOS_READ_DIRENTRY 0x2004  /* obsolete */
#define DMSDOS_WRITE_DIRENTRY 0x2005 /* obsolete */
#define DMSDOS_READ_BITFAT 0x2006
#define DMSDOS_WRITE_BITFAT 0x2007
#define DMSDOS_READ_MDFAT 0x2008
#define DMSDOS_WRITE_MDFAT 0x2009
#define DMSDOS_READ_DFAT 0x200a
#define DMSDOS_WRITE_DFAT 0x200b
#define DMSDOS_SET_COMP 0x200c
#define DMSDOS_SET_CF 0x200d
#define DMSDOS_SIMPLE_CHECK 0x200e
#define DMSDOS_DUMPCACHE 0x200f
#define DMSDOS_D_ASK 0x2010
#define DMSDOS_D_READ 0x2011
#define DMSDOS_D_WRITE 0x2012
#define DMSDOS_D_EXIT 0x2013
#define DMSDOS_MOVEBACK 0x2014       /* obsolete */
#define DMSDOS_SET_MAXCLUSTER 0x2015 /* currently not supported */
#define DMSDOS_READ_CLUSTER 0x2016
#define DMSDOS_FREE_IDLE_CACHE 0x2017
#define DMSDOS_SET_LOGLEVEL 0x2018
#define DMSDOS_SYNC_CCACHE 0x2019
#define DMSDOS_LOG_STATISTICS 0x201a
#define DMSDOS_SET_SPEEDUP 0x201b
#define DMSDOS_RECOMPRESS 0x201c     /* obsolete */
#define DMSDOS_REPORT_MEMORY 0x201d
#define IS_DMSDOS_IOCTL(cmd) ((cmd)>=DMSDOS_IOCTL_MIN&&(cmd)<=DMSDOS_IOCTL_MAX)

/* dmsdos library interface */
struct super_block* open_cvf(char*filename,int rwflag);
void close_cvf(struct super_block*sb);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
typedef uint8_t     __u8;
typedef uint16_t    __u16;
typedef uint32_t    __u32;
#include "lib_interface.h"
#define MALLOC  malloc
#define FREE    free
#define printk  printf
#define loglevel DEFAULT_LOGLEVEL

#endif
