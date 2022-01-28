/*
Asura huffboh 0.1a
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

this is the decompression algorithm I have reversed from the game Sniper Elite
where it's used in a file->memory implementation for handling its compressed
files of the Asura engine (AsuraCmp) used also in other games.

although the algorithm which comes from that game uses a couple of custom
fields (like the first 8 bytes before the dictionary) I think it can work
also with other streams compressed with the same algorithm without problems.

I do NOT know what is the exact name of this algorithm anyway some "keywords"
for identifying this algorithm are 0xffe, (0xb) 11, 0x1f/0x20, 0xffffffe0 (-20),
300 (0x130/0x134) for the dictionary size, the reading of 4 bytes at time from
the input stream and the usage of the main memory buffer which is at least
0x2000 bytes long.

- usage for file->file decompression:
    FILE *fd  = fopen(..., "rb");
    FILE *fdo = fopen(..., "wb");
    length = huffboh_unpack_file2file(fd, fdo);
    fclose(fd);
    fclose(fdo);

- usage for memory->memory decompression:
    length = huffboh_unpack_mem2mem(in, insz, out, outsz);

if the return value (length) is minor than zero (-1) means that an error
happened, for example the input file/buffer incomplete.

---
    Copyright 2009 Luigi Auriemma

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



uint8_t     huffboh_buff[0x2158+4]  = {0};  // the last 4 have been added by me to recover the decompression
FILE        *huffboh_fd             = NULL; // use this file descriptor to enable the FILE->memory unpacking!
uint8_t     *huffboh_mem            = NULL, // use this and the following to enable the memory->memory unpacking!
            *huffboh_mem_end        = NULL;




int huffboh_read(uint32_t a, uint8_t *b, uint32_t c) {
    int     len = 0;

    if(huffboh_fd) {
        len = fread(b, 1, c, huffboh_fd);
    } else if(huffboh_mem) {
        len = c;
        if((huffboh_mem + c) > huffboh_mem_end) len = huffboh_mem_end - huffboh_mem;
        if(len) {
            memcpy(b, huffboh_mem, len);
            huffboh_mem += len;
        }
    }
    return(len);
}



void huffboh_init2(uint8_t *arg1, uint32_t arg2, uint8_t *arg3, uint8_t *arg4) {
    uint32_t    i,
                n,
                c,
                e10 = 0,
                e14 = 0,
                e18 = 4,
                e1c = 1,
                e24 = 0;
    uint8_t     *p,
                *arg3p;

    arg2 = 1 << arg2;
    arg3p = arg3 + 0xff;
    for(;;) {
        do {
            c = *(uint32_t *)arg1;
            arg1 += 4;
            e24++;
            e18 <<= 1;
            e1c <<= 1;
            arg2 >>= 1;
        } while(!c);
        e14 = e1c << 2;
        do {
            if(arg3p >= arg3) {
                n = (*arg3p << 8) | e24;
                arg3p--;
            } else {
                n = 0xff;
            }
            p = arg4 + ((e10 >> 2) * 4);
            i = arg2;
            do {
                *(uint32_t *)p = n;
                p += e14;
                i--;
            } while(i);
            i = e18;
            do {
                i >>= 1;
                if(i & 3) return;
                e10 ^= i;
            } while(!(e10 & i));
            c--;
        } while(c);
    }
}



int huffboh_init(int n) {
    uint8_t     *s;

    memset(huffboh_buff, 0, sizeof(huffboh_buff));
    s = huffboh_buff + (n * sizeof(huffboh_buff));
    *(uint32_t *)s = n;

#ifdef ASURACMP
    // this is the one made by Asura, the first 8 bytes are the size of the compressed stream (dictionary excluded) and the uncompressed size
    if(huffboh_read(n, s + 4, 0x134) != 0x134) return(-1);
#else
    // my modification (max integer input size, 2gb)
    if(huffboh_read(n, s + 0xc, 300) != 300) return(-1);
    *(uint32_t *)(s + 4) = 0x7fffffff;      
#endif

    huffboh_init2(s + 0xc, 11, s + 0x38, s + 0x138);
    return(0);
}



int huffboh_unpack(uint8_t *out, int outsz) {
    uint32_t    a,
                b,
                c;
    uint8_t     *o,
                *s;

    o = out;
    if(*(uint32_t *)(huffboh_buff + 0x2158)) {
        *(uint32_t *)(huffboh_buff + 0x2158) = 0;
        c = 0;
        goto restore;
    }
    if(!*(huffboh_buff + 0x2154)) *(huffboh_buff + 0x2154) = 1; // mah
    do {
        *(uint32_t *)(huffboh_buff + 0x2138) += *(uint32_t *)(huffboh_buff + 0x2140);
        c = *(uint32_t *)(huffboh_buff + 0x2138);
        b = 0x1f - c;
        *(uint32_t *)(huffboh_buff + 0x214c) = b;
        *(uint32_t *)(huffboh_buff + 0x2144) <<= b;
        a = *(uint32_t *)(huffboh_buff + 0x213c);
        if(b > a) {
            *(uint32_t *)(huffboh_buff + 0x214c) = b - a;
            *(uint32_t *)(huffboh_buff + 0x2138) = a + c;
            if(a) {
                *(uint32_t *)(huffboh_buff + 0x2144) = (*(uint32_t *)(huffboh_buff + 0x2148) << (0x20 - a)) | (*(uint32_t *)(huffboh_buff + 0x2144) >> a);
            }
            if(*(uint32_t *)(huffboh_buff + 0x2150) < *(uint32_t *)(huffboh_buff + 4)) {
                void *tmp = (void *)huffboh_buff;   // useless, avoids only a stupid warning
                if(huffboh_read(*(uint32_t *)tmp, huffboh_buff + 0x2148, 4) != 4) goto quit;
            } else {
                *(uint32_t *)(huffboh_buff + 0x2148) = 0;
            }
            *(uint32_t *)(huffboh_buff + 0x2150) += 4;
            *(uint32_t *)(huffboh_buff + 0x213c) = 0x20;
        }
        b = *(uint32_t *)(huffboh_buff + 0x214c);
        a = *(uint32_t *)(huffboh_buff + 0x2148);
        *(uint32_t *)(huffboh_buff + 0x2138) = 0x1f;
        *(uint32_t *)(huffboh_buff + 0x2148) = a >> b;
        *(uint32_t *)(huffboh_buff + 0x2144) = (a << (0x20 - b)) | (*(uint32_t *)(huffboh_buff + 0x2144) >> b);
        *(uint32_t *)(huffboh_buff + 0x213c) -= b;
restore:
        c = o - out;
        while(c != outsz) {
            s = huffboh_buff + ((*(uint32_t *)(huffboh_buff + 0x2144) & 0xffe) * 2) + 0x138;
            b = *s;
            *(uint32_t *)(huffboh_buff + 0x2140) = b;
            *(uint32_t *)(huffboh_buff + 0x2138) -= b;
            if(*(int *)(huffboh_buff + 0x2138) < 0) break;
            *(uint32_t *)(huffboh_buff + 0x2144) >>= *(uint32_t *)(huffboh_buff + 0x2140);
            *o++ = *(s + 1);
            c++;
            if(c == outsz) {
                *(uint32_t *)(huffboh_buff + 0x2158) = 1;   // restore later if the buffer is too small
                goto quit;
            }
        }
    } while(*(int *)(huffboh_buff + 0x2138) > -20);
quit:
    return(o - out);
}



int huffboh_unpack_file2file(FILE *fd_input, FILE *fd_output) {
    int         len,
                tot;
    uint8_t     buff[4096];

    huffboh_fd      = fd_input;
    huffboh_mem     = NULL;
    huffboh_mem_end = NULL;

    if(huffboh_init(0) < 0) return(-1);
    for(tot = 0; (len = huffboh_unpack(buff, sizeof(buff))); tot += len) {
        if(fwrite(buff, 1, len, fd_output) != len) break;
    }
    return(tot);
}



int huffboh_unpack_mem2mem(uint8_t *in, uint32_t insz, uint8_t *out, uint32_t outsz) {
    huffboh_fd       = NULL;
    huffboh_mem      = in;
    huffboh_mem_end  = in + insz;

    if(huffboh_init(0) < 0) return(-1);
    return(huffboh_unpack(out, outsz));
}


