/*
    Copyright 2015 Luigi Auriemma

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



#define LibMComp_CLOSEHANDLE(X)    \
    if(X && (X != INVALID_HANDLE_VALUE)) { \
        CloseHandle(X); \
        X = INVALID_HANDLE_VALUE; \
    }
#define LibMComp_FREE(X) \
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
        return ret;
    }
#else
    u8 *quickbms_tmpname(u8 **fname, u8 *prefix, u8 *ext);
#endif

    static u32 LibMComp_dump_exe(u8 *fname, u8 *data, u32 size) {
        FILE    *fd;

        fd = fopen(fname, "wb");
        if(!fd) return 0;  // error
        fwrite(data, 1, size, fd);
        fclose(fd);
        return size;
    }



#include "mcomp_exe.c"
#include "mcomp_x32_exe.c"
#include "libmcomp.h"



typedef struct {
    HANDLE  pipe;
    u8      *data;
    int     size;
    u8      *prefix_data;
    int     prefix_size;
    u8      *suffix_data;
    int     suffix_size;
    u8      *info;
    int     done;
    HANDLE  tid;
} LibMComp_pipe_t;



static int LibMComp_start_thread(void *func, LibMComp_pipe_t *arg, HANDLE pipe, void *data, int size, void *info, void *prefix_data, int prefix_size, void *suffix_data, int suffix_size) {
    DWORD   t   = 0;

    if(!func || !arg) return -1;
    memset(arg, 0, sizeof(LibMComp_pipe_t));
    arg->pipe        = pipe;
    arg->data        = data;
    arg->size        = size;
    arg->info        = info;
    arg->prefix_data = prefix_data;
    arg->prefix_size = prefix_size;
    arg->suffix_data = suffix_data;
    arg->suffix_size = suffix_size;
    arg->tid  = CreateThread(NULL, 0, func, (void *)arg, 0, &t);
    if(!arg->tid) return -1;
    return 0;
}



static u8 *LibMComp_read_pipe(HANDLE pipe, int size, u8 *o, u8 *ol) {
    DWORD   t   = 0;

    if(size <= 0) return(o);
    if(ol && ((o + size) > ol)) return NULL;
    SetLastError(0);
    if(!ReadFile(pipe, o, size, &t, NULL) && (GetLastError() != ERROR_MORE_DATA)) return NULL;
    return o + t;
}



static int LibMComp_write_pipe(HANDLE pipe, void *data, int size) {
    DWORD   t   = 0;

    if(!size) return 0;
    if(size < 0) return -1;
    if(!WriteFile(pipe, data, size, &t, NULL)) return -1;
    FlushFileBuffers(pipe);
    return t;
}



static DWORD WINAPI LibMComp_write_thread(void *arg) {
    HANDLE  pipe    = ((LibMComp_pipe_t *)arg)->pipe;
    u8      *data   = ((LibMComp_pipe_t *)arg)->data;
    int     size    = ((LibMComp_pipe_t *)arg)->size;
                      ((LibMComp_pipe_t *)arg)->size = -1;
                      ((LibMComp_pipe_t *)arg)->done = 0;

    if(ConnectNamedPipe(pipe, NULL)) {
        for(;;) {
            if(LibMComp_write_pipe(pipe, ((LibMComp_pipe_t *)arg)->prefix_data, ((LibMComp_pipe_t *)arg)->prefix_size) < 0) break;
            if(LibMComp_write_pipe(pipe, data, size) < 0) break;
            if(LibMComp_write_pipe(pipe, ((LibMComp_pipe_t *)arg)->suffix_data, ((LibMComp_pipe_t *)arg)->suffix_size) < 0) break;
        }
        DisconnectNamedPipe(pipe);
        ((LibMComp_pipe_t *)arg)->size = size;
    }
    ((LibMComp_pipe_t *)arg)->done = 1;
    LibMComp_CLOSEHANDLE(pipe);
    return 0;
}



static DWORD WINAPI LibMComp_read_thread(void *arg) {
    HANDLE  pipe    = ((LibMComp_pipe_t *)arg)->pipe;
    u8      *out    = ((LibMComp_pipe_t *)arg)->data;
    int     outsz   = ((LibMComp_pipe_t *)arg)->size;
                      ((LibMComp_pipe_t *)arg)->size = -1;
                      ((LibMComp_pipe_t *)arg)->done = 0;
    int     size    = -1;
    u8      *o      = out,
            *ol     = out + outsz;

    if(ConnectNamedPipe(pipe, NULL)) {
        LibMComp_read_pipe(pipe, ((LibMComp_pipe_t *)arg)->prefix_size, ((LibMComp_pipe_t *)arg)->prefix_data, NULL);
        while(o < ol) {
            o = LibMComp_read_pipe(pipe, ol - o, o, NULL);
            if(!o) break;
            size = o - out;
            ((LibMComp_pipe_t *)arg)->size = size;
        }
        LibMComp_read_pipe(pipe, ((LibMComp_pipe_t *)arg)->suffix_size, ((LibMComp_pipe_t *)arg)->suffix_data, NULL);
        DisconnectNamedPipe(pipe);
    }
    ((LibMComp_pipe_t *)arg)->done = 1;
    LibMComp_CLOSEHANDLE(pipe);
    return size;
}



static u8 *LibMComp_create_pipe(u8 **fname, u8 *ext, int size, int msg) {
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



#endif  // WIN32
int LibMComp(void *const_in, int const_insz, void *const_out, int const_outsz, int algo) {
#ifdef WIN32
    LibMComp_pipe_t LibMComp_in_pipe,
                    LibMComp_out_pipe;
    intptr_t    pid;
    HANDLE  in_pipe             = INVALID_HANDLE_VALUE,
            out_pipe            = INVALID_HANDLE_VALUE;
    int     insz                = 0,
            outsz               = 0,
            size                = -1,
            use_LibMComp_x32    = 1,
            header_size         = 0;
    u8      *mcomp_exe_fname    = NULL, // NO static because the exe must be recreated all the times with correct size!
            *in_fname           = NULL,
            *out_fname          = NULL,
            *in                 = NULL,
            *out                = NULL,
            header[0x18];

    in   = (u8 *)const_in;
    insz = const_insz;
    if(!in || (insz <= 0)) goto quit;
    if(const_out) {
        out   = (u8 *)const_out;
        outsz = const_outsz;
    } else {
        if(algo < 0) {
            if(!memcmp(in, "MCMP", 4)) {
                outsz = *(int *)(in + 12);
            } else if(!memcmp(in, "MCM", 3)) {
                outsz = *(int *)(in + 16);
            }
        }
        if(outsz <= 0) {
            outsz = 1*1024*1024*1024;   // 1gb???
        }
        out = calloc(outsz, 1);
        if(!out) goto quit;
    }
    if(!out || (outsz <= 0)) goto quit;

         if(algo <=  0xc) use_LibMComp_x32 = 1;
    else if(algo <= 0x14) use_LibMComp_x32 = 0;
    else goto quit;
    if(algo < 0) {
        if(memcmp(in, "MCM", 3)) goto quit;
        if(in[3] == 'P') use_LibMComp_x32 = 0;
    } else {
        if(use_LibMComp_x32) {
            header_size = 0x18;
            memcpy(header, "MCMQ", 4);
            *(int *)(header + 4)  = 0x04000000;
            *(int *)(header + 8)  = algo;   // & (1<<31)
            *(int *)(header + 12) = 10;
            *(int *)(header + 16) = outsz;
            *(int *)(header + 20) = 0;      // 64bit not supported
        } else {
            header_size = 0x14;
            memcpy(header, "MCMP", 4);
            *(int *)(header + 4)  = 0x04000000;
            *(int *)(header + 8)  = algo;   // & (1<<31)
            *(int *)(header + 12) = outsz;
            *(int *)(header + 16) = 0;      // 64bit not supported
        }
    }


        // pipes

    in_pipe = LibMComp_create_pipe(&in_fname,   "tmp", insz,  1);
    if(in_pipe == INVALID_HANDLE_VALUE) goto quit;

    out_pipe = LibMComp_create_pipe(&out_fname, "tmp", outsz, 1);
    if(out_pipe == INVALID_HANDLE_VALUE) goto quit;

        // exe

    quickbms_tmpname(&mcomp_exe_fname, NULL, "exe");
    if(use_LibMComp_x32) {
        memcpy(mcomp_x32_exe + 0x4ab6, "\x33\xc0\xc3", 3); // disable printf
        LibMComp_dump_exe(mcomp_exe_fname, (void *)mcomp_x32_exe, sizeof(mcomp_x32_exe) - 1);
    } else {
        memcpy(mcomp_exe + 0x26ab9, "\x33\xc0\xc3", 3); // disable printf
        memset(mcomp_exe + 0x1437, 0x90, 0x146c - 0x1437);
        LibMComp_dump_exe(mcomp_exe_fname, (void *)mcomp_exe,     sizeof(mcomp_exe) - 1);
    }

        // input/output

    LibMComp_start_thread(LibMComp_write_thread, &LibMComp_in_pipe,  in_pipe,  in,  insz,  in_fname,  header, header_size, NULL, 0);
    LibMComp_start_thread(LibMComp_read_thread,  &LibMComp_out_pipe, out_pipe, out, outsz, out_fname, NULL, 0, NULL, 0);

        // exec

    pid = spawnl(
        P_NOWAIT,
        mcomp_exe_fname,
        mcomp_exe_fname,
        "-np",      // quiet
        in_fname,
        out_fname,
        NULL);
    if(pid < 0) goto quit;

        // wait

    while(!LibMComp_in_pipe.done || !LibMComp_out_pipe.done) {
        if( (LibMComp_in_pipe.done  && (LibMComp_in_pipe.size  < 0))
         || (LibMComp_out_pipe.done && (LibMComp_out_pipe.size < 0))
        ) {
            size = -1;
            goto quit;
        }
        Sleep(10);
    }
    size = LibMComp_out_pipe.size;
    if(size < 0) goto quit;

        // free

    LibMComp_CLOSEHANDLE(LibMComp_in_pipe.tid);
    LibMComp_CLOSEHANDLE(LibMComp_out_pipe.tid);
    LibMComp_FREE(in_fname)
    LibMComp_FREE(out_fname)

quit:
    if(mcomp_exe_fname)    while(unlink(mcomp_exe_fname))    Sleep(10);
    LibMComp_FREE(mcomp_exe_fname)
    LibMComp_FREE(in_fname)
    LibMComp_FREE(out_fname)
    LibMComp_CLOSEHANDLE(LibMComp_in_pipe.tid);
    LibMComp_CLOSEHANDLE(LibMComp_out_pipe.tid);
    if(!const_out) LibMComp_FREE(out)

    if(size >= 0) return size;
#endif  // WIN32
    return -1;
}

