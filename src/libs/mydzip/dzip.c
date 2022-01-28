/*
    Copyright 2016 Luigi Auriemma

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
#include <malloc.h>



#define dzip_CLOSEHANDLE(X)    \
    if(X && (X != INVALID_HANDLE_VALUE)) { \
        CloseHandle(X); \
        X = INVALID_HANDLE_VALUE; \
    }
#define dzip_FREE(X) \
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

    static u32 dzip_dump_exe(u8 *fname, u8 *data, u32 size) {
        FILE    *fd;

        fd = fopen(fname, "wb");
        if(!fd) return 0;  // error
        fwrite(data, 1, size, fd);
        fclose(fd);
        return size;
    }



#include "dzip_exe.c"
//#include "dzip.h"



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
} dzip_pipe_t;



static int dzip_start_thread(void *func, dzip_pipe_t *arg, HANDLE pipe, void *data, int size, void *info, void *prefix_data, int prefix_size, void *suffix_data, int suffix_size) {
    DWORD   t   = 0;

    if(!func || !arg) return -1;
    memset(arg, 0, sizeof(dzip_pipe_t));
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



static u8 *dzip_read_pipe(HANDLE pipe, int size, u8 *o, u8 *ol) {
    DWORD   t   = 0;

    if(size <= 0) return(o);
    if(ol && ((o + size) > ol)) return NULL;
    SetLastError(0);
    if(!ReadFile(pipe, o, size, &t, NULL) && (GetLastError() != ERROR_MORE_DATA)) return NULL;
    return o + t;
}



static int dzip_write_pipe(HANDLE pipe, void *data, int size) {
    DWORD   t   = 0;

    if(!size) return 0;
    if(size < 0) return -1;
    if(!WriteFile(pipe, data, size, &t, NULL)) return -1;
    FlushFileBuffers(pipe);
    return t;
}



static int dzip_write_workaround[10] = {-1};



static DWORD WINAPI dzip_write_thread(void *arg) {
    HANDLE  pipe    = ((dzip_pipe_t *)arg)->pipe;
    u8      *data   = ((dzip_pipe_t *)arg)->data;
    int     size    = ((dzip_pipe_t *)arg)->size;
                      ((dzip_pipe_t *)arg)->size = -1;
                      ((dzip_pipe_t *)arg)->done = 0;

    if(ConnectNamedPipe(pipe, NULL)) {
        for(;;) {
            if(dzip_write_pipe(pipe, ((dzip_pipe_t *)arg)->prefix_data, ((dzip_pipe_t *)arg)->prefix_size) < 0) break;
            int i;
            for(i = 0; dzip_write_workaround[i] >= 0; i++) {
                if(dzip_write_pipe(pipe, ((dzip_pipe_t *)arg)->prefix_data + dzip_write_workaround[i], ((dzip_pipe_t *)arg)->prefix_size - dzip_write_workaround[i]) < 0) break;
            }
            if(dzip_write_pipe(pipe, data, size) < 0) break;
            if(dzip_write_pipe(pipe, ((dzip_pipe_t *)arg)->suffix_data, ((dzip_pipe_t *)arg)->suffix_size) < 0) break;
        }
        DisconnectNamedPipe(pipe);
        ((dzip_pipe_t *)arg)->size = size;
    }
    ((dzip_pipe_t *)arg)->done = 1;
    dzip_CLOSEHANDLE(pipe);
    return 0;
}



static DWORD WINAPI dzip_read_thread(void *arg) {
    HANDLE  pipe    = ((dzip_pipe_t *)arg)->pipe;
    u8      *out    = ((dzip_pipe_t *)arg)->data;
    int     outsz   = ((dzip_pipe_t *)arg)->size;
                      ((dzip_pipe_t *)arg)->size = -1;
                      ((dzip_pipe_t *)arg)->done = 0;
    int     size    = -1;
    u8      *o      = out,
            *ol     = out + outsz;

    if(ConnectNamedPipe(pipe, NULL)) {
        dzip_read_pipe(pipe, ((dzip_pipe_t *)arg)->prefix_size, ((dzip_pipe_t *)arg)->prefix_data, NULL);
        while(o < ol) {
            o = dzip_read_pipe(pipe, ol - o, o, NULL);
            if(!o) break;
            size = o - out;
            ((dzip_pipe_t *)arg)->size = size;
        }
        dzip_read_pipe(pipe, ((dzip_pipe_t *)arg)->suffix_size, ((dzip_pipe_t *)arg)->suffix_data, NULL);
        DisconnectNamedPipe(pipe);
    }
    ((dzip_pipe_t *)arg)->done = 1;
    dzip_CLOSEHANDLE(pipe);
    return size;
}



static u8 *dzip_create_pipe(u8 **fname, u8 *ext, int size, int msg) {
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
int dzip_decompress(void *in, int insz, void *out, int outsz, int algo) {
#ifdef WIN32
    dzip_pipe_t dzip_in_pipe,
                    dzip_out_pipe;
    intptr_t    pid;
    HANDLE  in_pipe             = INVALID_HANDLE_VALUE,
            out_pipe            = INVALID_HANDLE_VALUE;
    int     size                = -1,
            header_size         = 0;
    u8      *dzip_exe_fname     = NULL, // NO static because the exe must be recreated all the times with correct size!
            *in_fname           = NULL,
            *out_fname          = NULL,
            *header,
            *p;

    if(!in  || (insz  <= 0)) goto quit;
    if(!out || (outsz <= 0)) goto quit;

        // pipes

    in_pipe = dzip_create_pipe(&in_fname,   "tmp", insz,  1);
    if(in_pipe == INVALID_HANDLE_VALUE) goto quit;

    out_pipe = dzip_create_pipe(&out_fname, "tmp", outsz, 1);
    if(out_pipe == INVALID_HANDLE_VALUE) goto quit;

        // generate header

    int wwx = 0;
    header_size = 9 + strlen(out_fname)+1 + 0x24;
    header = alloca(header_size);  // gives a warning about fstack-protector but avoid "jump into scope of identifier with variably modified type" error
    memset(header, 0, header_size);
    p = header;
    p += sprintf(p, "DTRZ");
    *(u16 *)p = 1;  p += 2;
    *(u16 *)p = 1;  p += 2;
    *p++ = 0x00;
    dzip_write_workaround[wwx++] = p - header;
    p += sprintf(p, "%s", out_fname) + 1;
    dzip_write_workaround[wwx++] = p - header;
    *(u16 *)p = 0;  p += 2;
    *(u16 *)p = 0;  p += 2;
    *(u16 *)p = -1; p += 2;

    *(u16 *)p = 1;  p += 2;
    *(u16 *)p = 1;  p += 2;

    *(u32 *)p = header_size;  p += 4;
    *(u32 *)p = insz;   p += 4;
    *(u32 *)p = outsz;  p += 4;
    if(!algo || (algo == 4)) {  // dz
        *(u32 *)p = 4;  p += 4;
    } else {                    // combuf/range
        *(u32 *)p = 5;  p += 4;
    }
    *p++ = 0x10;
    *p++ = 0x01;
    *p++ = 0x08;
    *p++ = 0x03;
    *p++ = 0x03;
    *p++ = 0x07;
    *p++ = 0x01;
    *p++ = 0x07;
    *p++ = 0x03;
    *p++ = 0x0f;
    if((p - header) != header_size) goto quit;
    dzip_write_workaround[wwx++] = -1;

        // exe

    quickbms_tmpname(&dzip_exe_fname, NULL, "exe");
    dzip_exe[0x00000F39] = 0x90;
    dzip_exe[0x00000F3A] = 0x90;
    dzip_exe[0x00000F3B] = 0x90;
    dzip_exe[0x00000F3C] = 0x90;
    dzip_exe[0x00000F3D] = 0x90;
    dzip_exe[0x00000F3E] = 0x90;
    dzip_exe[0x00000FDC] = 0x31;
    dzip_exe[0x00000FDD] = 0xC0;
    dzip_exe[0x00000FDE] = 0x90;
    dzip_exe[0x00000FDF] = 0x90;
    dzip_exe[0x00000FE0] = 0x90;
    dzip_exe[0x00011803] = 0xC6;
    dzip_exe[0x00011804] = 0x02;
    dzip_exe[0x00011805] = 0x00;
    dzip_exe[0x00011806] = 0x90;
    dzip_exe[0x00011807] = 0x90;
    dzip_exe[0x00011808] = 0x90;
    dzip_exe[0x00011809] = 0x90;
    dzip_exe[0x0001180A] = 0x90;
    dzip_exe[0x0001180B] = 0x90;
    dzip_exe[0x0001180C] = 0x90;
    dzip_exe[0x0001180D] = 0x90;
    dzip_exe[0x0001180E] = 0x90;
    dzip_exe[0x0001180F] = 0x90;
    dzip_exe[0x00011810] = 0x90;
    dzip_exe[0x00011849] = 0x90;
    dzip_exe[0x0001184A] = 0x90;
    dzip_exe[0x0001184B] = 0x90;
    dzip_exe[0x0001184C] = 0x90;
    dzip_exe[0x00011853] = 0x90;
    dzip_exe[0x00011854] = 0x90;
    dzip_exe[0x0001185E] = 0xeb;
    dzip_exe[0x0001185F] = 0x04;
    dzip_exe[0x00011943] = 0x90;
    dzip_exe[0x00011944] = 0x90;
    dzip_exe[0x00011945] = 0x90;
    dzip_exe[0x00011946] = 0x90;
    dzip_exe[0x00011947] = 0x90;
    dzip_exe[0x00015E7D] = 0x90;
    dzip_exe[0x00015E7E] = 0x90;
    dzip_exe[0x00015E7F] = 0x90;
    dzip_exe[0x0001615B] = 0x90;
    dzip_exe[0x0001615C] = 0x90;
    dzip_exe[0x0001615D] = 0x90;
    dzip_exe[0x0001656B] = 0x90;
    dzip_exe[0x0001656C] = 0x90;
    dzip_exe[0x0001656D] = 0x90;
    dzip_exe[0x0001766B] = 0x90;
    dzip_exe[0x0001766C] = 0x90;
    dzip_exe[0x0001766D] = 0x90;
    dzip_exe[0x00017674] = 0x33;
    dzip_exe[0x00017675] = 0xc0;
    dzip_exe[0x00017676] = 0x90;
    dzip_exe[0x00017677] = 0x90;
    dzip_exe[0x00017678] = 0x90;
    dzip_exe[0x00031BC6] = 0x25;
    dzip_exe[0x00031BC7] = 0x73;
    dzip_exe[0x00031BC8] = 0x00;
    dzip_dump_exe(dzip_exe_fname, (void *)dzip_exe, sizeof(dzip_exe) - 1);

        // input/output

    dzip_start_thread(dzip_write_thread, &dzip_in_pipe,  in_pipe,  in,  insz,  in_fname,  header, header_size, NULL, 0);
    dzip_start_thread(dzip_read_thread,  &dzip_out_pipe, out_pipe, out, outsz, out_fname, NULL, 0, NULL, 0);

        // exec

    pid = spawnl(
        P_NOWAIT,
        dzip_exe_fname,
        dzip_exe_fname,
        "-q",   // quiet
        "-d",   // decompress
        in_fname,
        NULL);
    if(pid < 0) goto quit;

        // wait

    while(!dzip_in_pipe.done || !dzip_out_pipe.done) {
        if( (dzip_in_pipe.done  && (dzip_in_pipe.size  < 0))
         || (dzip_out_pipe.done && (dzip_out_pipe.size < 0))
        ) {
            size = -1;
            goto quit;
        }
        Sleep(10);
    }
    size = dzip_out_pipe.size;
    if(size < 0) goto quit;

        // free

    dzip_CLOSEHANDLE(dzip_in_pipe.tid);
    dzip_CLOSEHANDLE(dzip_out_pipe.tid);
    dzip_FREE(in_fname)
    dzip_FREE(out_fname)

quit:
    if(dzip_exe_fname)    while(unlink(dzip_exe_fname))    Sleep(10);
    dzip_FREE(dzip_exe_fname)
    dzip_FREE(in_fname)
    dzip_FREE(out_fname)
    dzip_CLOSEHANDLE(dzip_in_pipe.tid);
    dzip_CLOSEHANDLE(dzip_out_pipe.tid);

    if(size >= 0) return size;
#endif  // WIN32
    return -1;
}


