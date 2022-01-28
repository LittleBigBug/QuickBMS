/*
QuickZIP library 0.1
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org

Usage:
    quickzip_ctx_t ctx;
    quickzip_open(&ctx, "output.zip");

    quickzip_add_entry(&ctx, "file.txt", "this is a content", 17);
    quickzip_add_entry(&ctx, "sub_folder/file.txt", "this is a content", 17);

    quickzip_close(&ctx);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <zlib.h>

#ifdef QUICKZIP_TEST
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



#pragma pack(2)
typedef struct {
    u32 sign;
    u16 ver;
    u16 flag;
    u16 method;
    u32 timedate;
    u32 crc;
    u32 comp_size;
    u32 uncomp_size;
    u16 name_len;
    u16 extra_len;
    /* filename */
    /* extra */
} quickzip_local_dir_t;

typedef struct {
    u32 sign;
    u16 ver_made;
    u16 ver_ext;
    u16 flag;
    u16 method;
    u32 timedate;
    u32 crc;
    u32 comp_size;
    u32 uncomp_size;
    u16 name_len;
    u16 extra_len;
    u16 comm_len;
    u16 disk_start;
    u16 int_attr;
    u32 ext_attr;
    u32 offset;
    /* filename */
    /* extra */
    /* comment */
} quickzip_central_dir_t;

typedef struct {
    u32 sign;
    u16 disk_num;
    u16 disk_num_start;
    u16 tot_ent_disk;
    u16 tot_ent_dir;
    u32 dir_size;
    u32 offset;
    u16 comm_len;
    /* comment */
} quickzip_end_central_dir_t;
#pragma pack()



typedef struct {
    u8      *name;
    u32     crc;
    u32     compressed_size;
    u32     uncompressed_size;
    u64     offset;
    void    *next;
} quickzip_entry_t;

typedef struct {
    FILE    *fd;
    quickzip_entry_t    *root;
    z_stream    z;
    u8      buff[4096];
} quickzip_ctx_t;



    // public API



int quickzip_add_entry(quickzip_ctx_t *ctx, u8 *name, u8 *buff, u32 size) {
    quickzip_local_dir_t    local_dir;
    quickzip_entry_t    *entry,
                        *new_entry;
    u64     backup_off;
    int     res,
            name_len;

    if(!ctx || !ctx->fd) return -1;

    if(!name) name = "";
    if(!buff) return -1;

    new_entry = real_calloc(1, sizeof(quickzip_entry_t));
    if(!new_entry) return -1;

    for(entry = ctx->root; entry && entry->next; entry = (quickzip_entry_t *)entry->next);
    if(!entry) {
        ctx->root = new_entry;
    } else {
        entry->next = new_entry;
    }
    entry = new_entry;

    name_len = strlen(name);
    entry->name = real_malloc(name_len + 1);
    memcpy(entry->name, name, name_len + 1);
    entry->crc = crc32(0L, Z_NULL, 0);
    entry->crc = crc32(entry->crc, buff, size);
    entry->compressed_size = 0; // will be fixed later
    entry->uncompressed_size = size;
    entry->offset = ftell(ctx->fd);

    local_dir.sign = 0x04034b50;
    local_dir.ver = 20;
    local_dir.flag = 0;
    local_dir.method = 8;
    local_dir.timedate = 0;
    local_dir.crc = entry->crc;
    local_dir.comp_size = entry->compressed_size;
    local_dir.uncomp_size = entry->uncompressed_size;
    local_dir.name_len = name_len;
    local_dir.extra_len = 0;

    fwrite(&local_dir, 1, sizeof(local_dir), ctx->fd);
    fwrite(name, 1, name_len, ctx->fd);

    deflateReset(&ctx->z);
    ctx->z.next_in   = buff;
    ctx->z.avail_in  = size;
    do {
        ctx->z.next_out  = ctx->buff;
        ctx->z.avail_out = sizeof(ctx->buff);
        res = deflate(&ctx->z, Z_FINISH);
        fwrite(ctx->buff, 1, sizeof(ctx->buff) - ctx->z.avail_out, ctx->fd);
    } while(res != Z_STREAM_END);

    // fix the compressed size field
    entry->compressed_size = ctx->z.total_out;
    local_dir.comp_size = entry->compressed_size;

    backup_off = ftell(ctx->fd);
    fseek(ctx->fd, entry->offset, SEEK_SET);
    fwrite(&local_dir, 1, sizeof(local_dir), ctx->fd);
    fseek(ctx->fd, backup_off, SEEK_SET);

    return 0;
}



int quickzip_open(quickzip_ctx_t *ctx, u8 *name) {
    if(!ctx) return -1;
    memset(ctx, 0, sizeof(quickzip_ctx_t));
    if(!name) return -1;
    ctx->fd = xfopen(name, "wb");
    if(!ctx->fd) return -1;

    if(deflateInit2(&ctx->z,
        /*Z_BEST_COMPRESSION*/ /*Z_DEFAULT_COMPRESSION*/ Z_BEST_SPEED
        , Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY)) {
        return -1;
    }

    return 0;
}



int quickzip_close(quickzip_ctx_t *ctx) {
    quickzip_end_central_dir_t  end_central_dir;
    quickzip_central_dir_t  central_dir;
    quickzip_entry_t    *entry,
                        *next_entry;
    u64     offset;
    int     entries;

    if(!ctx || !ctx->fd) return -1;

    offset = ftell(ctx->fd);

    memset(&central_dir, 0, sizeof(central_dir));
    central_dir.sign = 0x02014b50;
    central_dir.ver_made = 20;
    central_dir.ver_ext = 20;
    central_dir.method = 8;

    entries = 0;
    for(entry = ctx->root; entry; entry = (quickzip_entry_t *)entry->next) {
        central_dir.crc = entry->crc;
        central_dir.comp_size = entry->compressed_size;
        central_dir.uncomp_size = entry->uncompressed_size;
        central_dir.name_len = strlen(entry->name);
        central_dir.offset = entry->offset;

        fwrite(&central_dir, 1, sizeof(central_dir), ctx->fd);
        fwrite(entry->name, 1, strlen(entry->name), ctx->fd);
        entries++;
    }

    end_central_dir.sign = 0x06054b50;
    end_central_dir.disk_num = 0;
    end_central_dir.disk_num_start = 0;
    end_central_dir.tot_ent_disk = entries;
    end_central_dir.tot_ent_dir = entries;
    end_central_dir.dir_size = (u64)ftell(ctx->fd) - offset;
    end_central_dir.offset = offset;
    end_central_dir.comm_len = 0;
    fwrite(&end_central_dir, 1, sizeof(end_central_dir), ctx->fd);


    // close & free
    fclose(ctx->fd);

    for(entry = ctx->root; entry; entry = next_entry) {
        next_entry = (quickzip_entry_t *)entry->next;
        real_free(entry->name);
        real_free(entry);
    }
    deflateEnd(&ctx->z);

    return 0;
}



#ifdef QUICKZIP_TEST
int main(int argc, char *argv[]) {
    quickzip_ctx_t   ctx;

    if(argc < 2) {
        printf("\nUsage: %s <ZIP>\n", argv[0]);
        exit(1);
    }

    quickzip_open(&ctx, argv[1]);

    quickzip_add_entry(&ctx, "FILE1.txt", "this is a contentAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 117);
    quickzip_add_entry(&ctx, "sub_folder1/FILE1.txt", "this is a contentBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 117);
    quickzip_add_entry(&ctx, "SUB_FOLDER2/FILE2.txt", "this is a contentCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 117);
    quickzip_add_entry(&ctx, "SUB_FOLDER2/SUB_SUB_FOLDER1/FILE21.txt", "this is a contentDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD", 117);
    quickzip_add_entry(&ctx, "SUB_FOLDER2/SUB_SUB_FOLDER2/FILE22.txt", "this is a contentEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE", 117);
    quickzip_add_entry(&ctx, "SUB_FOLDER3/FILE3.txt", "this is a content", 17);

    quickzip_close(&ctx);

    printf("- done\n");
    return 0;
}
#endif
