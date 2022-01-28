/*
    Copyright 2013 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/



#ifdef WIN32    // Windows only, Linux maybe via wine
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>



#define UBERFLATE_MIN(X, Y) ((X)<(Y)?(X):(Y))
#define UBERFLATE_CLOSEHANDLE(X)    \
    if(X && (X != INVALID_HANDLE_VALUE)) { \
        CloseHandle(X); \
        X = INVALID_HANDLE_VALUE; \
    }
#define UBERFLATE_FREE(X) \
    if(X) { \
        free(X); \
        X = NULL; \
    }



#ifndef QUICKBMS
    typedef unsigned char   u8;
    typedef unsigned short  u16;
    typedef unsigned int    u32;

    // note that this is only a quick and simple implementation
    static u8 *quickbms_tmpname(u8 **fname, u8 *prefix, u8 *ext) {
        static u8   path[MAX_PATH + 1] = "";
        static int  i = 0;
        HANDLE  fd; // fopen and fstat fail
        u8      tmp[MAX_PATH + 1 + 64],
                *ret;

        if(fname) *fname = NULL;
        if(!path[0]) GetTempPath(sizeof(path), path);
        for(;;) {
            if(prefix) sprintf(tmp, "%s\\", prefix);
            else       strcpy(tmp, path);
            sprintf(tmp + strlen(tmp), "%08x%08x.%s", (int)GetCurrentProcessId(), i, ext);
            i++;
            fd = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
            if(fd == INVALID_HANDLE_VALUE) break;
            CloseHandle(fd);
        }
        ret = strdup(tmp);
        if(fname) *fname = ret;
        return(ret);
    }
#else
    u8 *quickbms_tmpname(u8 **fname, u8 *prefix, u8 *ext);
#endif

    static void *uberflate_memmem(const void *b1, const void *b2, size_t len1, size_t len2) {
        unsigned char *sp  = (unsigned char *) b1;
        unsigned char *pp  = (unsigned char *) b2;
        unsigned char *eos = sp + len1 - len2;

        if(!(b1 && b2 && len1 && len2))
            return NULL;

        while (sp <= eos) {
            if (*sp == *pp)
                if (memcmp(sp, pp, len2) == 0)
                    return sp;
            sp++;
        }
        return NULL;
    }

    static int uberflate_memmove(u8 *dst, u8 *src, int size) {
        int     i;

        if(!dst || !src) return(0);
        if(size < 0) size = strlen(src) + 1;
        if(dst < src) {
            for(i = 0; i < size; i++) {
                dst[i] = src[i];
            }
        } else {
            for(i = size - 1; i >= 0; i--) {
                dst[i] = src[i];
            }
        }
        return(i);
    }

    static u32 uberflate_dump_exe(u8 *fname, u8 *data, u32 size) {
        FILE    *fd;

        fd = fopen(fname, "wb");
        if(!fd) return(0);  // error
        fwrite(data, 1, size, fd);
        fclose(fd);
        return(size);
    }



#include "kzip_exe.c"
#include "deflopt_exe.c"
#include "defluff_exe.c"
#include "uberflate_fixes.c"
#include "uberflate.h"



typedef struct {
    HANDLE  pipe;
    u8      *data;
    int     size;
    u8      *info;
    int     done;
    HANDLE  tid;
} uberflate_pipe_t;

#pragma pack(2)
typedef struct {
    u32     sign;
    u16     ver;
    u16     flag;
    u16     method;
    u16     modtime;
    u16     moddate;
    u32     crc;
    u32     comp_size;
    u32     uncomp_size;
    u16     name_len;
    u16     extra_len;
} uberflate_pkzip_lfh_t;

typedef struct {
    u32     sign;
    u16     ver_made;
    u16     ver_need;
    u16     flag;
    u16     method;
    u16     modtime;
    u16     moddate;
    u32     crc;
    u32     comp_size;
    u32     uncomp_size;
    u16     name_len;
    u16     extra_len;
    u16     comm_len;
    u16     disknum;
    u16     int_attr;
    u32     ext_attr;
    u32     rel_offset;
} uberflate_pkzip_cd_t;

typedef struct {
    u32     sign;
    u16     disk_num;
    u16     disk_start;
    u16     central_entries;
    u16     central_entries2;
    u32     central_size;
    u32     central_offset;
    u16     comm_len;
} uberflate_pkzip_eocd_t;
#pragma pack()



static int uberflate_start_thread(void *func, uberflate_pipe_t *arg, HANDLE pipe, void *data, int size, void *info) {
    DWORD   t;

    if(!func || !arg) return(-1);
    memset(arg, 0, sizeof(uberflate_pipe_t));
    arg->pipe = pipe;
    arg->data = data;
    arg->size = size;
    arg->info = info;
    arg->tid  = CreateThread(NULL, 0, func, (void *)arg, 0, &t);
    if(!arg->tid) return(-1);
    return(0);
}



static u8 *uberflate_read_pipe(HANDLE pipe, int size, u8 *o, u8 *ol) {
    DWORD   t;

    if(size <= 0) return(o);
    if(ol && ((o + size) > ol)) return(NULL);
    if(!ReadFile(pipe, o, size, &t, NULL) && (GetLastError() != ERROR_MORE_DATA)) return(NULL);
    return(o + t);
}



static int uberflate_write_pipe(HANDLE pipe, void *data, int size) {
    DWORD   t;

    if(!size) return(0);
    if(size < 0) return(-1);
    if(!WriteFile(pipe, data, size, &t, NULL)) return(-1);
    FlushFileBuffers(pipe);
    return(t);
}



static DWORD WINAPI uberflate_kzip_pipe_write(void *arg) {
    HANDLE  pipe    = ((uberflate_pipe_t *)arg)->pipe;
    u8      *data   = ((uberflate_pipe_t *)arg)->data;
    int     size    = ((uberflate_pipe_t *)arg)->size;
                      ((uberflate_pipe_t *)arg)->size = -1;
                      ((uberflate_pipe_t *)arg)->done = 0;
    int     t;

    if(ConnectNamedPipe(pipe, NULL)) {
        for(;;) {
            t = uberflate_write_pipe(pipe, data ,size);
            if(t < 0) break;
        }
        DisconnectNamedPipe(pipe);
        ((uberflate_pipe_t *)arg)->size = size;
    }
    ((uberflate_pipe_t *)arg)->done = 1;
    UBERFLATE_CLOSEHANDLE(pipe);
    return(0);
}



static DWORD WINAPI uberflate_kzip_pipe_zip(void *arg) {
    uberflate_pkzip_lfh_t     *pkzip_lfh  = NULL;
    uberflate_pkzip_cd_t      *pkzip_cd   = NULL;
    uberflate_pkzip_eocd_t    *pkzip_eocd = NULL;

    HANDLE  pipe    = ((uberflate_pipe_t *)arg)->pipe;
    u8      *out    = ((uberflate_pipe_t *)arg)->data;
    int     outsz   = ((uberflate_pipe_t *)arg)->size;
                      ((uberflate_pipe_t *)arg)->size = -1;
                      ((uberflate_pipe_t *)arg)->done = 0;
    DWORD   t;
    int     md,
            chunk,
            size    = -1;
    u8      sign[4],
            *o      = NULL,
            *ol     = out + outsz;

    if(ConnectNamedPipe(pipe, NULL)) {

            // why it's so chaotic to read the generated ZIP in streaming?
            // because kzip creates multiple archives till it reaches the best size

        for(;;) {
            // PK header
            if(!uberflate_read_pipe(pipe, 4, sign, sign + 4)) break;
            if(!memcmp(sign, "\0\0\0\0", 4)) break; // lame?

            if(!memcmp(sign, "PK\x03\x04", 4)) {
                o = out;
                memcpy(o, sign, 4);
                o += 4;

                // header
                pkzip_lfh   = (uberflate_pkzip_lfh_t *)(o - 4);
                o = uberflate_read_pipe(pipe, sizeof(uberflate_pkzip_lfh_t) - 4, o, ol);
                if(!o) goto quit;

                // comp_size is 0 at this point

                /* already packed in the 1mb of write cache used by kzip
                // name + extra
                o = uberflate_read_pipe(pipe, pkzip_lfh->name_len + pkzip_lfh->extra_len, o, ol);
                if(!o) goto quit;
                */

                // boring 1mb chunks used by kzip, 0x100000 bytes
                for(chunk = 0; o < ol; chunk++) {
                    SetLastError(0);
                    for(md = 0;; md++) {
                        if(!ReadFile(pipe, o, ol - o, &t, NULL)) goto quit;
                        if(GetLastError() != ERROR_MORE_DATA) break;
                    }
                    if(!md) {
                        if((t == (4 + 4 + 4)) && (chunk > 0)) {
                            break;
                        }
                        o += t;
                    }
                    // if md is non-zero it means that our buffer was too small so skip it
                }

                // crc, compressed and uncompressed size
                //if(!uberflate_read_pipe(pipe, 4 + 4 + 4, (void *)&(pkzip_lfh->crc), ol)) goto quit;
                memcpy(&(pkzip_lfh->crc), o, 4 + 4 + 4);

            } else if(!memcmp(sign, "PK\x01\x02", 4)) {
                memcpy(o, sign, 4);
                o += 4;

                // header
                pkzip_cd    = (uberflate_pkzip_cd_t *)(o - 4);
                o = uberflate_read_pipe(pipe, sizeof(uberflate_pkzip_cd_t) - 4, o, ol);
                if(!o) goto quit;

                // name + extra + comm_len
                o = uberflate_read_pipe(pipe, pkzip_cd->name_len + pkzip_cd->extra_len + pkzip_cd->comm_len, o, ol);
                if(!o) goto quit;

            } else if(!memcmp(sign, "PK\x05\x06", 4)) {
                memcpy(o, sign, 4);
                o += 4;

                // header
                pkzip_eocd  = (uberflate_pkzip_eocd_t *)(o - 4);
                o = uberflate_read_pipe(pipe, sizeof(uberflate_pkzip_eocd_t) - 4, o, ol);
                if(!o) goto quit;

                // name + extra + comm_len
                o = uberflate_read_pipe(pipe, pkzip_eocd->comm_len, o, ol);
                if(!o) goto quit;

            } else {
                //fprintf(stderr, "\nError: kzip unknown PKZIP signature %02x%02x%02x%02x\n", sign[0], sign[1], sign[2], sign[3]);
                goto quit;
            }
        }

        size = o - out;
        ((uberflate_pipe_t *)arg)->size = size;
quit:
        DisconnectNamedPipe(pipe);
    }
    ((uberflate_pipe_t *)arg)->done = 1;
    UBERFLATE_CLOSEHANDLE(pipe);
    return(size);
}



static DWORD WINAPI uberflate_deflopt_pipe_write(void *arg) {
    HANDLE  pipe    = ((uberflate_pipe_t *)arg)->pipe;
    u8      *fname  = ((uberflate_pipe_t *)arg)->info;
    u8      *data   = ((uberflate_pipe_t *)arg)->data;
    int     size    = ((uberflate_pipe_t *)arg)->size;
                      ((uberflate_pipe_t *)arg)->size = -1;
                      ((uberflate_pipe_t *)arg)->done = 0;
    int     len;

    if(!ConnectNamedPipe(pipe, NULL)) goto quit;
    len = 8;
    if(uberflate_write_pipe(pipe, data, UBERFLATE_MIN(len, size)) < 0) goto quit;
    DisconnectNamedPipe(pipe);

    if(!ConnectNamedPipe(pipe, NULL)) goto quit;

    for(len = 0x4000;; len <<= 1) {
        if(uberflate_write_pipe(pipe, data, UBERFLATE_MIN(len, size)) < 0) goto quit;
        if(len >= size) break;
    }

    fname = strrchr(fname, '\\') + 1;
    len = sizeof(uberflate_pkzip_cd_t) + strlen(fname);
    if(uberflate_write_pipe(pipe, data + size - sizeof(uberflate_pkzip_eocd_t) - len, len) < 0) goto quit;
    if(uberflate_write_pipe(pipe, data + size - sizeof(uberflate_pkzip_eocd_t), 4) < 0) goto quit;
    if(uberflate_write_pipe(pipe, data, sizeof(uberflate_pkzip_lfh_t)) < 0) goto quit;
    if(uberflate_write_pipe(pipe, data + sizeof(uberflate_pkzip_lfh_t), strlen(fname)) < 0) goto quit;
    if(uberflate_write_pipe(pipe, data + sizeof(uberflate_pkzip_lfh_t) + strlen(fname), ((uberflate_pkzip_lfh_t *)data)->comp_size) < 0) goto quit;

    ((uberflate_pipe_t *)arg)->size = size;
quit:
    DisconnectNamedPipe(pipe);
    ((uberflate_pipe_t *)arg)->done = 1;
    UBERFLATE_CLOSEHANDLE(pipe);
    return(0);
}



static DWORD WINAPI uberflate_deflopt_pipe_zip(void *arg) {
    HANDLE  pipe    = ((uberflate_pipe_t *)arg)->pipe;
    u8      *out    = ((uberflate_pipe_t *)arg)->data;
    int     outsz   = ((uberflate_pipe_t *)arg)->size;
    u8      *fname  = ((uberflate_pipe_t *)arg)->info;
                      ((uberflate_pipe_t *)arg)->size = -1;
                      ((uberflate_pipe_t *)arg)->done = 0;
    DWORD   t;
    int     size = -1;

    if(ConnectNamedPipe(pipe, NULL)) {
        for(size = 0; size < outsz; size += t) {
            if(!ReadFile(pipe, out + size, outsz - size, &t, NULL)) break;
        }
        DisconnectNamedPipe(pipe);
    }

    fname = strrchr(fname, '\\') + 1;
    t = strlen(fname);
    if(size > (sizeof(uberflate_pkzip_lfh_t) + t + sizeof(uberflate_pkzip_cd_t) + t + sizeof(uberflate_pkzip_eocd_t))) {
        uberflate_pkzip_lfh_t     *pkzip_lfh  = NULL;
        uberflate_pkzip_cd_t      *pkzip_cd   = NULL;
        uberflate_pkzip_eocd_t    *pkzip_eocd = NULL;

        pkzip_eocd = (uberflate_pkzip_eocd_t *)(out + size - sizeof(uberflate_pkzip_eocd_t));
        pkzip_cd   = (uberflate_pkzip_cd_t *)((void *)pkzip_eocd - sizeof(uberflate_pkzip_cd_t) - t);
        pkzip_lfh  = (uberflate_pkzip_lfh_t *)((void *)pkzip_cd - sizeof(uberflate_pkzip_lfh_t) - t);
        memcpy(out, pkzip_lfh, sizeof(uberflate_pkzip_lfh_t));
        t = (u8 *)pkzip_cd - out;
        uberflate_memmove((void *)pkzip_lfh, (void *)pkzip_cd, size - t);
        size = ((u8 *)pkzip_lfh - out) + (size - t);
    } else {
        size = -1;
    }

    ((uberflate_pipe_t *)arg)->size = size;
    ((uberflate_pipe_t *)arg)->done = 1;
    UBERFLATE_CLOSEHANDLE(pipe);
    return(size);
}



static DWORD WINAPI uberflate_defluff_write(void *arg) {
    HANDLE  pipe    = ((uberflate_pipe_t *)arg)->pipe;
    u8      *data   = ((uberflate_pipe_t *)arg)->data;
    int     size    = ((uberflate_pipe_t *)arg)->size;
                      ((uberflate_pipe_t *)arg)->size = -1;
                      ((uberflate_pipe_t *)arg)->done = 0;
    int     t;

    t = uberflate_write_pipe(pipe, data, size);
    if(t < 0) goto quit;
    ((uberflate_pipe_t *)arg)->size = t;
quit:
    ((uberflate_pipe_t *)arg)->done = 1;
    UBERFLATE_CLOSEHANDLE(pipe);
    return(size);
}



static int uberflate_defluff(u8 *cmd, u8 *in, int insz, u8 *out, int outsz, u8 *fname) {
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sec;
    uberflate_pipe_t    uberflate_in_pipe;
    HANDLE  in_read     = INVALID_HANDLE_VALUE,
            in_write    = INVALID_HANDLE_VALUE,
            out_read    = INVALID_HANDLE_VALUE,
            out_write   = INVALID_HANDLE_VALUE,
            err_read    = INVALID_HANDLE_VALUE,
            err_write   = INVALID_HANDLE_VALUE,
            hp          = INVALID_HANDLE_VALUE;
    DWORD   t;
    int     size        = -1,
            detach      = 0;

    struct  _OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionEx(&vi);
    if(vi.dwPlatformId < VER_PLATFORM_WIN32_NT) detach = DETACHED_PROCESS;  // Win9x only

    memset(&sec, 0, sizeof(sec));
    sec.nLength              = sizeof(sec);
    sec.lpSecurityDescriptor = NULL;
    sec.bInheritHandle       = TRUE;
    if(!CreatePipe(&in_read,  &in_write,  &sec, 0)) goto quit;
    if(!CreatePipe(&out_read, &out_write, &sec, 0)) goto quit;
    if(!CreatePipe(&err_read, &err_write, &sec, 0)) goto quit;

    GetStartupInfo(&si);
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdInput   = in_read;
    si.hStdOutput  = out_write;
    si.hStdError   = err_write;
 
    if(!CreateProcess(
        NULL,                               // lpApplicationName
        cmd,                                // lpCommandLine
        &sec,                               // lpProcessAttributes
        &sec,                               // lpThreadAttributes
        TRUE,                               // bInheritHandles
        detach | CREATE_NEW_PROCESS_GROUP,  // dwCreationFlags
        NULL,                               // lpEnvironment
        NULL,                               // lpCurrentDirectory
        &si,                                // lpStartupInfo
        &pi)) {                             // lpProcessInformation
            goto quit;
    }

    hp = pi.hProcess;
    UBERFLATE_CLOSEHANDLE(in_read);
    UBERFLATE_CLOSEHANDLE(out_write);
    UBERFLATE_CLOSEHANDLE(err_write);

    uberflate_start_thread(uberflate_defluff_write, &uberflate_in_pipe, in_write, in, insz, NULL);

    for(size = 0; size < outsz; size += t) {
        if(!ReadFile(out_read, out + size, outsz - size, &t, NULL)) break;
    }

    fname = strrchr(fname, '\\') + 1;
    t = strlen(fname);
    if(size > (sizeof(uberflate_pkzip_lfh_t) + t + sizeof(uberflate_pkzip_cd_t) + t + sizeof(uberflate_pkzip_eocd_t))) {
        u8              *p          = NULL;
        uberflate_pkzip_lfh_t     *pkzip_lfh  = NULL;
        uberflate_pkzip_cd_t      *pkzip_cd   = NULL;
        uberflate_pkzip_eocd_t    *pkzip_eocd = NULL;

        pkzip_lfh  = (uberflate_pkzip_lfh_t *)out;
        if(!pkzip_lfh->crc && !pkzip_lfh->comp_size && !pkzip_lfh->uncomp_size) {
            pkzip_eocd = (uberflate_pkzip_eocd_t *)(out + size - sizeof(uberflate_pkzip_eocd_t));
            pkzip_cd   = (uberflate_pkzip_cd_t *)((void *)pkzip_eocd - sizeof(uberflate_pkzip_cd_t) - t);
            p          = (void *)((void *)pkzip_cd - 0x14);
            memcpy(out + 6, p, 0x14);
            t = (u8 *)pkzip_cd - out;
            uberflate_memmove((void *)p, (void *)pkzip_cd, size - t);
            size = ((u8 *)p - out) + (size - t);
        }
    } else {
        size = -1;
    }

quit:
    UBERFLATE_CLOSEHANDLE(in_write);
    UBERFLATE_CLOSEHANDLE(out_read);
    UBERFLATE_CLOSEHANDLE(err_read);
    TerminateProcess(hp, -1);
    return(size);
}



static u8 *uberflate_create_pipe(u8 **fname, u8 *ext, int size, int msg) {
    HANDLE  pipe;

    *fname = NULL;
    quickbms_tmpname(fname, "\\\\.\\pipe", ext);
    pipe = CreateNamedPipe(
        *fname,
        PIPE_ACCESS_DUPLEX,
        (msg ?
            (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE) :
            (PIPE_TYPE_BYTE    | PIPE_READMODE_BYTE)
        ) | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        size,
        size,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);
    return(pipe);
}



static int uberflate_store(u8 *in, int insz, u8 *out, int outsz) {
    int     t;
    u8      *i  = in,
            *il = in + insz,
            *o  = out,
            *ol = out + outsz;

    for(; i < il; i += t) {
        t = 0xffff; // max deflate store size

        if((ol - o) < 1) return(-1);
        if((il - i) <= t) {   // yeah <= to catch the the last aligned chunk
            t = il - i;
            *o++ = 1;
        } else {
            *o++ = 0;
        }

        if((ol - o) < 2) return(-1);
        *(u16 *)o = t;
        o += 2;

        if((ol - o) < 2) return(-1);
        *(u16 *)o = 0 - (t + 1);
        o += 2;

        if((ol - o) < t) return(-1);
        memcpy(o, i, t);
        o += t;
    }
    return(o - out);
}



static u32 uberflate_adler32(u8 *data, int size) {
    u32     sum,
            crc     = 1;
    int     i;

    sum = (crc >> 16) & 0xffff;
    crc &= 0xffff;
    for(i = 0; i < size; i++) {
        crc += data[i];
        sum += crc;
        crc %= 65521;
        sum %= 65521;
    }
    return(crc | (sum << 16));
}



int uberflate(void *const_in, int const_insz, void *const_out, int const_outsz, int zlib) {
    intptr_t    pid;
    uberflate_pipe_t    uberflate_in_pipe,
                        uberflate_out_pipe;
    HANDLE  in_pipe             = INVALID_HANDLE_VALUE,
            out_pipe            = INVALID_HANDLE_VALUE;
    u32     crc                 = 0;
    int     t,
            insz                = 0,
            outsz               = 0,
            step                = 0,
            size                = -1;
    u8      *kzip_exe_fname     = NULL, // NO static because the exe must be recreated all the times with correct size!
            *deflopt_exe_fname  = NULL, // NO static because the exe must be recreated all the times with correct size!
            *defluff_exe_fname  = NULL, // NO static because the exe must be recreated all the times with correct size!
            *in_fname           = NULL,
            *out_fname          = NULL,
            *in                 = NULL,
            *out                = NULL;

    in   = (u8 *)const_in;
    insz = const_insz;
    if(!in || (insz <= 0)) goto quit;
    if(const_out) {
        out   = (u8 *)const_out;
        outsz = const_outsz;
    } else {
        outsz = UBERFLATE_MAXZIPLEN(insz);
        out = malloc(outsz);
        if(!out) goto quit;
    }
    if(!out || (outsz <= 0)) goto quit;
    if(zlib) {
        outsz -= (1 + 1 + 4);
        if(outsz < 0) goto quit;
        *out++ = 0x78;  // CM, CINFO
        *out++ = 0x5e;  // FCHECK, FDICT, FLEVEL
        crc = uberflate_adler32(in, insz);
    }


        /********\
        |* KZIP *|
        \********/

        // pipes

    in_pipe = uberflate_create_pipe(&in_fname, "tmp", insz, 1);
    if(in_pipe == INVALID_HANDLE_VALUE) goto quit;

    out_pipe = uberflate_create_pipe(&out_fname, "zip", outsz, 1);
    if(out_pipe == INVALID_HANDLE_VALUE) goto quit;

        // exe

    if(uberflate_kzip_exe_patch(insz) < 0) goto quit;
    quickbms_tmpname(&kzip_exe_fname, NULL, "exe");
    uberflate_dump_exe(kzip_exe_fname, (void *)kzip_exe, sizeof(kzip_exe) - 1);

        // input/output

    uberflate_start_thread(uberflate_kzip_pipe_write, &uberflate_in_pipe,  in_pipe,  in,  insz,  in_fname);
    uberflate_start_thread(uberflate_kzip_pipe_zip,   &uberflate_out_pipe, out_pipe, out, outsz, out_fname);

        // exec

    pid = spawnl(
        P_NOWAIT,
        kzip_exe_fname,
        kzip_exe_fname,
        "/y",       // force overwrite
        "/q",       // quiet mode
        "/b128",    // 128 seems to be better with most files rather than 256, except with already deflated streams
        out_fname,
        in_fname,
        NULL);
    if(pid < 0) goto quit;

        // wait

    while(!uberflate_in_pipe.done || !uberflate_out_pipe.done) {
        if( (uberflate_in_pipe.done  && (uberflate_in_pipe.size  < 0))
         || (uberflate_out_pipe.done && (uberflate_out_pipe.size < 0))
        ) {
            size = -1;
            goto quit;
        }
        Sleep(10);
    }
    size = uberflate_out_pipe.size;
    if(size < 0) goto quit;

        // free

    UBERFLATE_CLOSEHANDLE(uberflate_in_pipe.tid);
    UBERFLATE_CLOSEHANDLE(uberflate_out_pipe.tid);
    UBERFLATE_FREE(in_fname)
    UBERFLATE_FREE(out_fname)

    if(((uberflate_pkzip_lfh_t *)out)->method != 8) {
        goto do_store;
    }

        // from now on in and insz are no longer used
        // they will be passed to kzip_old() if uberflate() fails

    step = 0;
redo_deflopt:

        /***********\
        |* DEFLOPT *|
        \***********/

        // pipes

    in_pipe = uberflate_create_pipe(&in_fname, "zip", size, 1);
    if(in_pipe == INVALID_HANDLE_VALUE) goto quit;

    out_pipe = uberflate_create_pipe(&out_fname, "zip", outsz, 1);
    if(out_pipe == INVALID_HANDLE_VALUE) goto quit;

        // exe

    if(uberflate_deflopt_exe_patch(out_fname, size) < 0) goto quit;
    if(!deflopt_exe_fname) quickbms_tmpname(&deflopt_exe_fname, NULL, "exe");   // do not create the name a second time
    uberflate_dump_exe(deflopt_exe_fname, (void *)deflopt_exe, sizeof(deflopt_exe) - 1);

        // input/output

    uberflate_start_thread(uberflate_deflopt_pipe_write, &uberflate_in_pipe,  in_pipe,  out, size,  in_fname);
    uberflate_start_thread(uberflate_deflopt_pipe_zip,   &uberflate_out_pipe, out_pipe, out, outsz, out_fname);

        // exec

    pid = spawnl(
        P_NOWAIT,
        deflopt_exe_fname,
        deflopt_exe_fname,
        "/a",
        "/b",
        "/s",   // do NOT use /v because it's used to contain my patches
        in_fname,
        NULL);
    if(pid < 0) goto quit;

        // wait

    while(!uberflate_in_pipe.done || !uberflate_out_pipe.done) {
        if( (uberflate_in_pipe.done  && (uberflate_in_pipe.size  < 0))
         || (uberflate_out_pipe.done && (uberflate_out_pipe.size < 0))
        ) {
            size = -1;
            goto quit;
        }
        Sleep(10);
    }
    size = uberflate_out_pipe.size;
    if(size < 0) goto quit;

        // free

    UBERFLATE_CLOSEHANDLE(uberflate_in_pipe.tid);
    UBERFLATE_CLOSEHANDLE(uberflate_out_pipe.tid);
    UBERFLATE_FREE(in_fname)
    // UBERFLATE_FREE(out_fname) // do NOT free out_fname because it's used in defluff!

    step++;
    if(step < 2) {

            /***********\
            |* defluff *|
            \***********/

            // exe

        if(uberflate_defluff_exe_patch(size) < 0) goto quit;
        quickbms_tmpname(&defluff_exe_fname, NULL, "exe");
        uberflate_dump_exe(defluff_exe_fname, (void *)defluff_exe, sizeof(defluff_exe) - 1);

            // execute

        size = uberflate_defluff(defluff_exe_fname, out, size, out, outsz, out_fname);
        if(size < 0) goto quit;

            // free

        UBERFLATE_FREE(out_fname)

        goto redo_deflopt;
    }

        // strip deflate data and add zlib header

    uberflate_pkzip_lfh_t     *pkzip_lfh  = NULL;
    pkzip_lfh = (uberflate_pkzip_lfh_t *)out;
    t = sizeof(uberflate_pkzip_lfh_t) + pkzip_lfh->name_len + pkzip_lfh->extra_len;
    size = pkzip_lfh->comp_size;
    uberflate_memmove(out, out + t, size);

    if((size == insz) && !memcmp(in, out, size)) {
do_store:
        if(zlib) out[-1] = 0x01;
        size = uberflate_store(in, insz, out, outsz);
        if(size < 0) goto quit;
    }

    if(zlib) {
        out[size++] = crc >> 24;
        out[size++] = crc >> 16;
        out[size++] = crc >> 8;
        out[size++] = crc;
        size++; // CM, CINFO
        size++; // FCHECK, FDICT, FLEVEL
    }

    if(!const_out) {
        if(size > insz) {
            size = -1;
            goto quit;
        }
        memcpy(in, zlib ? (out - 2) : out, size);
    }

quit:
    if(kzip_exe_fname)    while(unlink(kzip_exe_fname))    Sleep(10);
    if(deflopt_exe_fname) while(unlink(deflopt_exe_fname)) Sleep(10);
    if(defluff_exe_fname) while(unlink(defluff_exe_fname)) Sleep(10);
    UBERFLATE_FREE(kzip_exe_fname)
    UBERFLATE_FREE(deflopt_exe_fname)
    UBERFLATE_FREE(defluff_exe_fname)
    UBERFLATE_FREE(in_fname)
    UBERFLATE_FREE(out_fname)
    UBERFLATE_CLOSEHANDLE(uberflate_in_pipe.tid);
    UBERFLATE_CLOSEHANDLE(uberflate_out_pipe.tid);
    if(!const_out) UBERFLATE_FREE(out)

    if(size < 0) {
#ifdef QUICKBMS
        fprintf(stderr, "- some problems with uberflate, switching to kzip_old\n");
        return(kzip_old(in, insz, out, outsz, zlib));
#endif
    }
    return(size);
}


#else
int uberflate(void *const_in, int const_insz, void *const_out, int const_outsz, int zlib) {
    return(-1);
}
#endif

