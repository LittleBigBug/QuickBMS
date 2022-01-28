/*
lib_interface.h

DMSDOS library: headers for interface functions, hacks, dummies, and fakes.

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

/* These are copied from the kernel include files in order to avoid
   including those files. They are not 100% identical to the kernel types.
   Most of them needn't be the same as in the kernel.
   This has been done for libc6 support.
*/

/* machine and system dependent hacks */

/* Linux section -- no problems here... :)) */
#ifdef __linux__IGNORE
/* this defines machine-dependent __u8, __s8 etc. types */
#include<asm/types.h>
/* this defines get_unaligned and put_unaligned */
#include<asm/unaligned.h>
/* this defines cpu_to_le16 etc. in 2.1 kernels - a kind of nop for 2.0 */
#include<asm/byteorder.h> 

/* Other systems usually do not have the asm include files */
#else
/* emulate asm/types.h */
typedef unsigned char __u8;
typedef signed char __s8;
typedef unsigned short int __u16;
typedef signed short int __s16;
typedef unsigned int __u32;
typedef signed int __s32;
/* emulate asm/unaligned.h */
/* edit these lines if your system cannot do unaligned access */
#define get_unaligned(ptr) (*(ptr))
#define put_unaligned(val, ptr) ((void)( *(ptr) = (val) ))
/* emulate asm/byteorder.h */
/* edit these lines if your system is non-linux and big endian */
/* the examples are commented out; they are valid for a little endian cpu */
/* #define cpu_to_le16(v) (v) */
/* #define cpu_to_be16(v) ( (((v)&0xff)<<8) | (((v)&0xff00)>>8) ) */
/* #define cpu_to_le32(v) (v) */
/* hack: sometimes NULL is missing */
#ifndef NULL
#define NULL ((void*)0)
#endif
#endif

int printk(const char *fmt, ...);
void panic(const char * fmt, ...);

#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT

#define SECTOR_SIZE 512
#define SECTOR_BITS 9
#define MSDOS_DPB       (MSDOS_DPS) /* dir entries per block */
#define MSDOS_DPB_BITS  4 /* log2(MSDOS_DPB) */
#define MSDOS_DPS       (SECTOR_SIZE/sizeof(struct msdos_dir_entry))
#define MSDOS_DPS_BITS  4 /* log2(MSDOS_DPS) */
#define MSDOS_DIR_BITS  5 /* log2(sizeof(struct msdos_dir_entry)) */

#define MSDOS_SUPER_MAGIC 0x4d44 /* MD */

#define MSDOS_MAX_EXTRA 3 /* tolerate up to that number of clusters which are
                             inaccessible because the FAT is too short */

#define KERN_EMERG      "<0>"   /* system is unusable                   */
#define KERN_ALERT      "<1>"   /* action must be taken immediately     */
#define KERN_CRIT       "<2>"   /* critical conditions                  */
#define KERN_ERR        "<3>"   /* error conditions                     */
#define KERN_WARNING    "<4>"   /* warning conditions                   */
#define KERN_NOTICE     "<5>"   /* normal but significant condition     */
#define KERN_INFO       "<6>"   /* informational                        */
#define KERN_DEBUG      "<7>"   /* debug-level messages                 */

struct buffer_head {
        unsigned long b_blocknr;        /* block number */
        char * b_data;                  /* pointer to data block */
};

#define MS_RDONLY   1      /* Mount read-only */
#define MSDOS_SB(s) (&((s)->u.msdos_sb))

struct msdos_dir_entry {
        __s8    name[8],ext[3]; /* name and extension */
        __u8    attr;           /* attribute bits */
        __u8    lcase;          /* Case for base and extension */
        __u8    ctime_ms;       /* Creation time, milliseconds */
        __u16   ctime;          /* Creation time */
        __u16   cdate;          /* Creation date */
        __u16   adate;          /* Last access date */
        __u16   starthi;        /* High 16 bits of cluster in FAT32 */
        __u16   time,date,start;/* time, date and first cluster */
        __u32   size;           /* file size (in bytes) */
};

struct msdos_sb_info {
        unsigned short cluster_size; /* sectors/cluster */
        unsigned char fats,fat_bits; /* number of FATs, FAT bits (12 or 16) */
        unsigned short fat_start,fat_length; /* FAT start & length (sec.) */
        unsigned short dir_start,dir_entries; /* root dir start & entries */
        unsigned short data_start;   /* first data sector */
        unsigned long clusters;      /* number of clusters */
        unsigned long root_cluster;  /* first cluster of the root directory */
        unsigned long fsinfo_offset; /* FAT32 fsinfo offset from start of disk */
        void *fat_wait;
        int fat_lock;
        int prev_free;               /* previously returned free cluster number */
        int free_clusters;           /* -1 if undefined */
        /*struct fat_mount_options options;*/
        struct nls_table *nls_disk;  /* Codepage used on disk */
        struct nls_table *nls_io;    /* Charset used for input and display */
        struct cvf_format* cvf_format;
        void* private_data;
};

struct super_block {
        int s_dev;
        unsigned long s_blocksize;
        unsigned char s_blocksize_bits;
        unsigned long s_flags;
        unsigned long s_magic;
        unsigned long* directlist;
        unsigned long* directlen;
        unsigned long directsize;
        union {
                struct msdos_sb_info msdos_sb;
        } u;

};

struct fat_boot_sector {
        __s8    ignored[3];     /* Boot strap short or near jump */
        __s8    system_id[8];   /* Name - can be used to special case
                                   partition manager volumes */
        __u8    sector_size[2]; /* bytes per logical sector */
        __u8    cluster_size;   /* sectors/cluster */
        __u16   reserved;       /* reserved sectors */
        __u8    fats;           /* number of FATs */
        __u8    dir_entries[2]; /* root directory entries */
        __u8    sectors[2];     /* number of sectors */
        __u8    media;          /* media code (unused) */
        __u16   fat_length;     /* sectors/FAT */
        __u16   secs_track;     /* sectors per track */
        __u16   heads;          /* number of heads */
        __u32   hidden;         /* hidden sectors (unused) */
        __u32   total_sect;     /* number of sectors (if sectors == 0) */
        /* The following fields are only used by FAT32 */
        __u32   fat32_length;   /* sectors/FAT */
        __u16   flags;          /* bit 8: fat mirroring, low 4: active fat */
        __u8    version[2];     /* major, minor filesystem version */
        __u32   root_cluster;   /* first cluster in root directory */
        __u16   info_sector;    /* filesystem info sector */
        __u16   backup_boot;    /* backup boot sector */
        __u16   reserved2[6];   /* Unused */
};

#define MALLOC malloc
#define FREE free
#define kmalloc(x,y) malloc(x)
#define kfree free
#define CURRENT_TIME time(NULL)
#define vmalloc malloc
#define vfree free

#define MAXFRAGMENT 300
