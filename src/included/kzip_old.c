// by Luigi Auriemma

// the original failsafe method

// http://advsys.net/ken/utils.htm
// kzip.exe is recreated all the times because
// I don't want to leave temporary files on the disk

#ifdef WIN32
#include <process.h>
#include "kzip_exe.h"



u8 *quickbms_tmpname(u8 **fname, u8 *prefix, u8 *ext);
u32 mydump(u8 *fname, u8 *data, u32 size);



int kzip_old(u8 *in, int insz, u8 *out, int outsz, int zlib) {
    int     size    = -1;
    FILE    *fd;
    u32     namesz,
            crc;
    u8      head[0x1e],
            *exe_fname,
            *in_fname,
            *out_fname;

    quickbms_tmpname(&exe_fname, NULL, "exe");
    mydump(exe_fname, (void *)kzip_exe, sizeof(kzip_exe));

    quickbms_tmpname(&in_fname,  NULL, "tmp");
    mydump(in_fname, in, insz);

    quickbms_tmpname(&out_fname, NULL, "zip");

    if(spawnl(
        P_WAIT,
        exe_fname,
        exe_fname,
        "/y",       // force overwrite
        "/q",       // quiet mode
        "/b128",    // 128 seems to be better with most files rather than 256, except already deflated streams
        out_fname,
        in_fname,
        NULL)) goto quit;

    fd = fopen(out_fname, "rb");
    if(!fd) goto quit;
    if((fread(head, 1, sizeof(head), fd) == sizeof(head)) && !memcmp(head, "PK\x03\x04", 4)) {
        crc = (u32)adler32(0, Z_NULL, 0);
        crc = (u32)adler32(crc, in, insz);
        size    = head[0x12] | (head[0x13]<<8) | (head[0x14]<<16) | (head[0x15]<<24);
        namesz  = head[0x1a] | (head[0x1b]<<8) | (head[0x1c]<<16) | (head[0x1d]<<24);
        if(!fseek(fd, namesz, SEEK_CUR)) {
            if(zlib) outsz -= (2 + 4);
            if(size < 0) size = outsz;
            if(size > outsz) size = outsz;
            if(zlib) {
                out[0] = 0x78;  // CM, CINFO
                out[1] = 0x5e;  // FCHECK, FDICT, FLEVEL
                size = fread(out + 2, 1, size, fd);
                out[2 + size]     = crc >> 24;
                out[2 + size + 1] = crc >> 16;
                out[2 + size + 2] = crc >> 8;
                out[2 + size + 3] = crc;
                size += 2 + 4;
            } else {
                size = fread(out, 1, size, fd);
            }
        }
    }
    fclose(fd);

quit:
    unlink(exe_fname);
    unlink(in_fname);
    unlink(out_fname);
    FREE(exe_fname)
    FREE(in_fname)
    FREE(out_fname)
    return(size);
}


#else

int kzip_old(u8 *in, int insz, u8 *out, int outsz, int zlib) {
    return(-1);
}

#endif

