/*
QuickISO library 0.1a
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org

Usage:
    quickiso_ctx_t ctx;
    quickiso_open(&ctx, "output.iso");

    quickiso_add_entry(&ctx, "file.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "sub_folder/file.txt", "this is a content", 17);

    quickiso_close(&ctx);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef QUICKISO_TEST
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

#define real_realloc    realloc
#define real_calloc     calloc
#define real_malloc     malloc
#define real_free       free
#define xfopen          fopen
#else
FILE *xfopen(u8 *fname, u8 *mode);
#endif



#define quickiso_maxnamelen    (0xff - 34)
#define quickiso_isfolder(X)   (X & 2)

typedef struct {
    u8      name[quickiso_maxnamelen + 1];
    u32     sector;
    u32     size;
    u16     id;
    u8      flags;
    int     childs;
    void    *child;
} quickiso_entry_t;

typedef struct {
    FILE    *fd;
    u8      buff[2048];
    u8      root_record[34];
    quickiso_entry_t *root;
    u16     current_id;
} quickiso_ctx_t;



static int quickiso_putxx(u8 *dst, u32 num, int bits, int endian) {
    u8      *p = dst;

    if(!dst) return 0;

    if(endian <= 0) {
        *p++ = num;
        if(bits >= 16) *p++ = num >> 8;
        if(bits >= 32) {
            *p++ = num >> 16;
            *p++ = num >> 24;
        }
    }
    if(endian >= 0) {
        if(bits >= 32) {
            *p++ = num >> 24;
            *p++ = num >> 16;
        }
        if(bits >= 16) *p++ = num >> 8;
        *p++ = num;
    }
    return p - dst;
}



static int quickiso_putss(u8 *dst, u8 *src, int size) {
    u8      *p = dst;

    if(!dst) return 0;
    if(!src) src = "";
    if(size < 0) size = strlen(src);
    strncpy(p, src, size);
    p += size;
    return p - dst;
}



static int quickiso_puttime(u8 *dst, int mode) {
    struct  tm  *tmx;
    time_t  datex;
    u8      *p = dst;

    if(!dst) return 0;

    time(&datex);
    tmx = gmtime(&datex);

    if(!mode) {
        p += sprintf(p, "%04d%02d%02d%02d%02d%02d%02d%c",
            1900 + tmx->tm_year, tmx->tm_mon + 1, tmx->tm_mday,
            tmx->tm_hour, tmx->tm_min, tmx->tm_sec, 0,
            0);
    } else {
        *p++ = tmx->tm_year;
        *p++ = tmx->tm_mon + 1;
        *p++ = tmx->tm_mday;
        *p++ = tmx->tm_hour;
        *p++ = tmx->tm_min;
        *p++ = tmx->tm_sec;
        *p++ = 0;
    }
    return p - dst;
}



static int quickiso_getnamelen(u8 *name) {
    int     len;

    if(!name) name = "";    // root
    len = strlen(name);
    if(!len) len = 1;       // root
    if(len > quickiso_maxnamelen) len = quickiso_maxnamelen;
    return len;
}



static int quickiso_padding(quickiso_ctx_t *ctx) {
    u32     diff;

    if(!ctx || !ctx->fd) return -1;

    diff = (u64)ftell(ctx->fd) % (u64)2048;
    if(diff) {
        diff = 2048 - diff;
        memset(ctx->buff, 0, diff);
        fwrite(ctx->buff, 1, diff, ctx->fd);
    }
    return 0;
}



static int quickiso_write_directory_raw(quickiso_ctx_t *ctx, quickiso_entry_t *entry, u8 *name) {
    int     len;
    u8      *p;

    if(!ctx || !ctx->fd) return -1;
    if(!entry) return -1;

    p = ctx->buff;
    *p++ = 0;   // placeholder
    *p++ = 0;
    p += quickiso_putxx(p, entry->sector, 32, 0);
    p += quickiso_putxx(p, entry->size, 32, 0);
    p += quickiso_puttime(p, 1);
    *p++ = entry->flags;
    *p++ = 0;
    *p++ = 0;
    p += quickiso_putxx(p, 1, 16, 0);
    len = quickiso_getnamelen(name);
    *p++ = len;
    p += quickiso_putss(p, name, len);
    if((p - ctx->buff) & 1) *p++ = 0;
    ctx->buff[0] = p - ctx->buff;
    fwrite(ctx->buff, 1, p - ctx->buff, ctx->fd);
    return 0;
}



static int quickiso_write_directory(quickiso_ctx_t *ctx, quickiso_entry_t *entry, quickiso_entry_t *parent) {
    quickiso_entry_t **child;
    u64     backup_offset;
    int     i;

    if(!ctx || !ctx->fd) return -1;
    if(!entry) return -1;
    if(!quickiso_isfolder(entry->flags)) return 0;

    child = (quickiso_entry_t **)entry->child;

    for(i = 0; i < entry->childs; i++) {
        if(quickiso_isfolder(child[i]->flags)) {
            quickiso_write_directory(ctx, child[i], entry);
        }
    }

    backup_offset = (u64)ftell(ctx->fd);
    entry->sector = backup_offset / (u64)2048;

    memset(ctx->buff, 0, 34);           // placeholder fixed by quickiso_fix_directories
    fwrite(ctx->buff, 1, 34, ctx->fd);  // quickiso_write_directory_raw(ctx, entry, "");
    fwrite(ctx->buff, 1, 34, ctx->fd);  // quickiso_write_directory_raw(ctx, parent, "\1");

    for(i = 0; i < entry->childs; i++) {
        quickiso_write_directory_raw(ctx, child[i], child[i]->name);
    }

    quickiso_padding(ctx);

    entry->size = (u64)ftell(ctx->fd) - backup_offset;

    return 0;
}



static int quickiso_fix_directories(quickiso_ctx_t *ctx, quickiso_entry_t *entry, quickiso_entry_t *parent) {
    quickiso_entry_t **child;
    int     i;

    if(!ctx || !ctx->fd) return -1;
    if(!entry) return -1;
    if(!quickiso_isfolder(entry->flags)) return 0;

    fseek(ctx->fd, (u64)entry->sector * (u64)2048, SEEK_SET);
    quickiso_write_directory_raw(ctx, entry, "");
    if(entry == ctx->root) memcpy(ctx->root_record, ctx->buff, 34);
    quickiso_write_directory_raw(ctx, parent, "\1");

    child = (quickiso_entry_t **)entry->child;

    for(i = 0; i < entry->childs; i++) {
        if(quickiso_isfolder(child[i]->flags)) {
            quickiso_fix_directories(ctx, child[i], entry);
        }
    }

    return 0;
}



static int quickiso_write_table_raw(quickiso_ctx_t *ctx, quickiso_entry_t *entry, u16 id, int endian) {
    int     len;
    u8      *p;

    if(!ctx || !ctx->fd) return -1;
    if(!entry) return -1;

    p = ctx->buff;
    len = quickiso_getnamelen(entry->name);
    *p++ = len;
    *p++ = 0;
    p += quickiso_putxx(p, entry->sector, 32, endian);
    p += quickiso_putxx(p, id, 16, endian);
    p += quickiso_putss(p, entry->name, len);
    if((p - ctx->buff) & 1) *p++ = 0;
    fwrite(ctx->buff, 1, p - ctx->buff, ctx->fd);
    return 0;
}



static int quickiso_write_table(quickiso_ctx_t *ctx, quickiso_entry_t *entry, u16 id, int endian) {
    quickiso_entry_t **child;
    int     i;

    if(!ctx || !ctx->fd) return -1;

    if(!entry) {    // root
        quickiso_write_table_raw(ctx, ctx->root, 0, endian);
        quickiso_write_table(ctx, ctx->root, 0, endian);
        return 0;
    }

    child = (quickiso_entry_t **)entry->child;

    for(i = 0; i < entry->childs; i++) {
        if(quickiso_isfolder(child[i]->flags)) {
            quickiso_write_table_raw(ctx, child[i], entry->id, endian);
        }
    }

    for(i = 0; i < entry->childs; i++) {
        if(quickiso_isfolder(child[i]->flags)) {
            quickiso_write_table(ctx, child[i], entry->id, endian);
        }
    }

    return 0;
}



static quickiso_entry_t *quickiso_add_entry_raw(quickiso_ctx_t *ctx, quickiso_entry_t *parent, u8 *name, int name_len, int flags, u32 size) {
    quickiso_entry_t *entry;
    quickiso_entry_t **child;

    if(!ctx || !ctx->fd) return NULL;

    if(parent) {
        parent->child = real_realloc(parent->child, (parent->childs + 1) * sizeof(quickiso_entry_t *));
        if(!parent->child) return NULL;
        child = (quickiso_entry_t **)parent->child;
    }
    entry = real_calloc(1, sizeof(quickiso_entry_t));   // common to both parent and !parent, use calloc to avoid memset
    if(!entry) return NULL;
    if(parent) child[parent->childs] = entry;

    if(!name) name = "";
    if(name_len < 0) name_len = strlen(name);
    if(name_len > quickiso_maxnamelen) name_len = quickiso_maxnamelen;
    strncpy(entry->name, name, name_len);
    entry->name[name_len] = 0;
    if(quickiso_isfolder(flags)) {
        entry->id = ++ctx->current_id;
    } else {
        entry->sector = (u64)ftell(ctx->fd) / (u64)2048;
        entry->size = size;
    }
    entry->flags = flags;

    if(parent) parent->childs++;
    return entry;
}



static int quickiso_free_entry(quickiso_entry_t *entry) {
    quickiso_entry_t **child;
    int     i;

    if(!entry) return -1;

    child = (quickiso_entry_t **)entry->child;
    if(child) {
        for(i = 0; i < entry->childs; i++) {
            if(child[i]) {
                quickiso_free_entry(child[i]);
                real_free(child[i]);
                child[i] = NULL;
            }
        }
        real_free(entry->child);
        entry->child = NULL;
    }
    entry->childs = 0;
    //real_free(entry); // NO!
    return 0;
}



    // public API



int quickiso_add_entry(quickiso_ctx_t *ctx, u8 *name, u8 *buff, u32 size) {
    quickiso_entry_t *entry;
    quickiso_entry_t **child;
    int     i;
    u8      *p,
            *l;

    if(!ctx || !ctx->fd) return -1;

    if(!ctx->root) {
        ctx->root = quickiso_add_entry_raw(ctx, NULL, "", -1, 2, 0);
    }
    if(!name) return 0; // root

    entry = ctx->root;

    for(l = p = name; *p; p = l + 1) {
        while(*p && ((*p == '/') || (*p == '\\'))) p++;
        if(!*p) break;  // invalid
        l = p;
        while(*l && ((*l != '/') && (*l != '\\'))) l++;
        if(!*l) break;  // this is a file

        // this is a folder
        child = (quickiso_entry_t **)entry->child;
        for(i = 0; i < entry->childs; i++) {
            if((strlen(child[i]->name) == (l - p)) && !strnicmp(child[i]->name, p, l - p)) {
                break;
            }
        }
        if(i < entry->childs) {
            entry = child[i];
        } else {
            entry = quickiso_add_entry_raw(ctx, entry, p, l - p, 2, 0);
        }
    }

    if(*p) {
        quickiso_add_entry_raw(ctx, entry, p, l - p, 0, size);

        if(buff && size) {
            fwrite(buff, 1, size, ctx->fd);
            quickiso_padding(ctx);
        }
    }

    return 0;
}



int quickiso_open(quickiso_ctx_t *ctx, u8 *name) {
    int     i;

    if(!ctx) return -1;
    memset(ctx, 0, sizeof(quickiso_ctx_t));
    if(!name) return -1;
    ctx->fd = xfopen(name, "wb");
    if(!ctx->fd) return -1;

    quickiso_add_entry(ctx, NULL, NULL, 0);

    memset(ctx->buff, 0, 2048);
    for(i = 0; i < 16; i++) {
        fwrite(ctx->buff, 1, 2048, ctx->fd);
    }

    // placeholders
    fwrite(ctx->buff, 1, 2048, ctx->fd);  // primary
    fwrite(ctx->buff, 1, 2048, ctx->fd);  // terminator
    fwrite(ctx->buff, 1, 2048, ctx->fd);  // padding

    return 0;
}



int quickiso_close(quickiso_ctx_t *ctx) {
    u32     sector1,
            sector2,
            sector3,
            table_size;
    int     i;
    u8      *p;

    if(!ctx || !ctx->fd) return -1;

    quickiso_write_directory(ctx, ctx->root, ctx->root);
    quickiso_padding(ctx);

    sector1 = (u64)ftell(ctx->fd) / (u64)2048;
    quickiso_write_table(ctx, NULL, 0, -1);
    table_size = (u64)ftell(ctx->fd) - ((u64)sector1 * (u64)2048);
    quickiso_padding(ctx);

    sector2 = (u64)ftell(ctx->fd) / (u64)2048;
    quickiso_write_table(ctx, NULL, 0, 1);
    quickiso_padding(ctx);

    sector3 = (u64)ftell(ctx->fd) / (u64)2048;

    // necessary because . and .. don't have the correct sector and size
    quickiso_fix_directories(ctx, ctx->root, ctx->root);

    fseek(ctx->fd, 16 * 2048, SEEK_SET);

    // Primary Volume Descriptor
    memset(ctx->buff, 0, 2048);
    p = ctx->buff;
    *p++ = 1;
    p += quickiso_putss(p, "CD001", 5);
    *p++ = 1;
    *p++ = 0;
    p += quickiso_putss(p, "Win32", 32);
    p += quickiso_putss(p, "CDROM", 32);
    for(i = 0; i < 8;  i++) *p++ = 0;
    p += quickiso_putxx(p, sector3, 32, 0);
    for(i = 0; i < 32; i++) *p++ = 0;
    p += quickiso_putxx(p, 1, 16, 0);
    p += quickiso_putxx(p, 1, 16, 0);
    p += quickiso_putxx(p, 2048, 16, 0);
    p += quickiso_putxx(p, table_size, 32, 0);
    p += quickiso_putxx(p, sector1, 32, -1);
    p += quickiso_putxx(p, 0, 32, -1);
    p += quickiso_putxx(p, sector2, 32, 1);
    p += quickiso_putxx(p, 0, 32, 1);
    memcpy(p, ctx->root_record, 34); p += 34;
    p += quickiso_putss(p, "", 128);
    p += quickiso_putss(p, "", 128);
    p += quickiso_putss(p, "", 128);
    p += quickiso_putss(p, "", 128);
    p += quickiso_putss(p, "", 37);
    p += quickiso_putss(p, "", 37);
    p += quickiso_putss(p, "", 37);
    p += quickiso_puttime(p, 0);
    p += quickiso_puttime(p, 0);
    p += quickiso_puttime(p, 0);
    p += quickiso_puttime(p, 0);
    *p++ = 1;
    *p++ = 0;
    fwrite(ctx->buff, 1, 2048, ctx->fd);

    // Primary Volume Descriptor Terminator
    memset(ctx->buff, 0, 2048);
    p = ctx->buff;
    *p++ = 0xff;
    p += quickiso_putss(p, "CD001", 5);
    *p++ = 1;
    fwrite(ctx->buff, 1, 2048, ctx->fd);

    // empty sector
    memset(ctx->buff, 0, 2048);
    fwrite(ctx->buff, 1, 2048, ctx->fd);

    // close & free
    fclose(ctx->fd);
    quickiso_free_entry(ctx->root);
    if(ctx->root) real_free(ctx->root);

    return 0;
}



#ifdef QUICKISO_TEST
int main(int argc, char *argv[]) {
    quickiso_ctx_t   ctx;

    if(argc < 2) {
        printf("\nUsage: %s <ISO>\n", argv[0]);
        exit(1);
    }

    quickiso_open(&ctx, argv[1]);

    quickiso_add_entry(&ctx, "FILE1.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "sub_folder1/FILE1.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "SUB_FOLDER2/FILE2.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "SUB_FOLDER2/SUB_SUB_FOLDER1/FILE21.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "SUB_FOLDER2/SUB_SUB_FOLDER2/FILE22.txt", "this is a content", 17);
    quickiso_add_entry(&ctx, "SUB_FOLDER3/FILE3.txt", "this is a content", 17);

    quickiso_close(&ctx);

    printf("- done\n");
    return 0;
}
#endif
