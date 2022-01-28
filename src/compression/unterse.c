/*
IBM TERSE decompression (PACK/SPACK) 0.1a
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org

Open source decompression algorithm reverse engineered from tersepc.zip (terse.exe).
It supports both PACK and SPACK.

---
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../extra/mybits.h"    // QuickBMS



typedef struct {
    signed short    n1;
    signed short    n2;
    signed short    n3;
    signed short    n4;
} terse_table_t;

// Not safe for multi-thread.
// total memory: 64kb
static terse_table_t terse_table[0x1000 + 1];
static signed short x43a880[0x1000];
static signed short x43c880[0x1000];
static signed short x43e880[0x1000];
static signed short x440880[0x1000];

static signed short    x414130 = 1;
//static signed short    x414134 = 1 or 0x10;
static signed short    x414138 = 0x80;
static signed short    x41413c = 1;    // 0:PACK 1:SPACK
static signed short    x414140 = 1;
static signed short    x414144 = 1;
//static signed short    x414150 = 1;
static signed short    x414154 = 0x100;
static signed short    x4522f8 = 0;

static const int            x4141b8[] = { 0, 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };
static const unsigned char  x4156dc[] = "\r\n\0";
static const unsigned char  x41a300[] =
    "\x00\x01\x02\x03\xCF\x09\xD3\x7F\xD4\xD5\xC3\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\xC7\xB4\x08\xC9\x18\x19\xCC\xCD\x83\x1D\xD2\x1F"
    "\x81\x82\x1C\x84\x86\x0A\x17\x1B\x89\x91\x92\x95\xA2\x05\x06\x07\xE0\xEE\x16\xE5\xD0\x1E\xEA\x04\x8A\xF6\xC6\xC2\x14\x15\xC1\x1A"
    "\x20\xA6\xE1\x80\xEB\x90\x9F\xE2\xAB\x8B\x9B\x2E\x3C\x28\x2B\x7C\x26\xA9\xAA\x9C\xDB\xA5\x99\xE3\xA8\x9E\x21\x24\x2A\x29\x3B\x5E"
    "\x2D\x2F\xDF\xDC\x9A\xDD\xDE\x98\x9D\xAC\xBA\x2C\x25\x5F\x3E\x3F\xD7\x88\x94\xB0\xB1\xB2\xFC\xD6\xFB\x60\x3A\x23\x40\x27\x3D\x22"
    "\xF8\x61\x62\x63\x64\x65\x66\x67\x68\x69\x96\xA4\xF3\xAF\xAE\xC5\x8C\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x97\x87\xCE\x93\xF1\xFE"
    "\xC8\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xEF\xC0\xDA\x5B\xF2\xF9\xB5\xB6\xFD\xB7\xB8\xB9\xE6\xBB\xBC\xBD\x8D\xD9\xBF\x5D\xD8\xC4"
    "\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\xCB\xCA\xBE\xE8\xEC\xED\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xA1\xAD\xF5\xF4\xA3\x8F"
    "\x5C\xE7\x53\x54\x55\x56\x57\x58\x59\x5A\xA0\x85\x8E\xE9\xE4\xD1\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xB3\xF7\xF0\xFA\xA7\xFF"
    "\x00\x01\x02\x03\x37\x2D\x2E\x2F\x16\x05\x25\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x3C\x3D\x32\x26\x18\x19\x3F\x27\x22\x1D\x35\x1F"
    "\x40\x5A\x7F\x7B\x5B\x6C\x50\x7D\x4D\x5D\x5C\x4E\x6B\x60\x4B\x61\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\x7A\x5E\x4C\x7E\x6E\x6F"
    "\x7C\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xAD\xE0\xBD\x5F\x6D"
    "\x79\x81\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96\x97\x98\x99\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xC0\x4F\xD0\xA1\x07"
    "\x43\x20\x21\x1C\x23\xEB\x24\x9B\x71\x28\x38\x49\x90\xBA\xEC\xDF\x45\x29\x2A\x9D\x72\x2B\x8A\x9A\x67\x56\x64\x4A\x53\x68\x59\x46"
    "\xEA\xDA\x2C\xDE\x8B\x55\x41\xFE\x58\x51\x52\x48\x69\xDB\x8E\x8D\x73\x74\x75\xFA\x15\xB0\xB1\xB3\xB4\xB5\x6A\xB7\xB8\xB9\xCC\xBC"
    "\xAB\x3E\x3B\x0A\xBF\x8F\x3A\x14\xA0\x17\xCB\xCA\x1A\x1B\x9C\x04\x34\xEF\x1E\x06\x08\x09\x77\x70\xBE\xBB\xAC\x54\x63\x65\x66\x62"
    "\x30\x42\x47\x57\xEE\x33\xB6\xE1\xCD\xED\x36\x44\xCE\xCF\x31\xAA\xFC\x9E\xAE\x8C\xDD\xDC\x39\xFB\x80\xAF\xFD\x78\x76\xB2\x9F\xFF";



typedef struct {
    unsigned char *o;
    unsigned char *outl;
} terse_io_t;

static void terse_decompress_dump(terse_io_t *io, int dump) {
    signed short bx;
    for(bx = 8; bx > 0;) {
        if((bx <= 7) || (x414138 != 0x80)) {
            if(x414138 == 1) {
                if(io->o < io->outl) *io->o++ = x4141b8[bx] & dump;
                x414138 = 0x80;
            } else {
                x414138 >>= 1;
            }
            bx--;
        } else {
            if(io->o < io->outl) *io->o++ = dump;
            bx -= 8;
        }
    }
}

static void terse_decompress_out_crlf(terse_io_t *io) {
    int     x;
    for(x = 0; x4156dc[x]; x++) terse_decompress_dump(io, x4156dc[x]);
}

static void terse_decompress_out_code(terse_io_t *io, int code) {
    if(code == 0) {
        if(!x414130) return;
        if(!x414140) return;
        if(!x414144) return;
        terse_decompress_out_crlf(io);
    } else {
        if(!x414130 || !x414140) {
            if(code >= 257) return;
            terse_decompress_dump(io, code - 1);
        } else if(!x414144) {
            terse_decompress_dump(io, x41a300[code - 1]);
            x4522f8++;
            if(x4522f8 == x414154) {
                terse_decompress_out_crlf(io);
                x4522f8 = 0;
            }
        } else {
            if(code == 257) {
                terse_decompress_out_crlf(io);
            } else {
                terse_decompress_dump(io, x41a300[code - 1]);
            }
        }
    }
}

static void terse_decompress_out(terse_io_t *io, int t) {
    signed short    stack[2048];
    int     si = 0;
    for(;;) {
        while(t > 257) {
            if(si < 2048) {
                stack[si] = terse_table[t].n2;
                si++;
            }
            t = terse_table[t].n1;
        }
        terse_decompress_out_code(io, t);
        si--;
        if(si < 0) break;
        t = stack[si];
    }
}


int terse_decompress(
    unsigned char *in,      // input buffer
    int insz,               // size of input buffer
    unsigned char *out,     // output buffer
    int outsz,              // size of output buffer
    int is_raw_pack_spack,  // 0:header/SPACK, -1:PACK, 1:PACK
    int force_binary        // force the binary mode (-b option)
) {
    mybits_ctx_t    ctx;
    mybits_init(&ctx, in, insz, NULL);

    terse_io_t io;
    io.o    = out;
    io.outl = out + outsz;

    int     i;
    //signed short    code = 0;

    int Version_Flag = 5;
    int Fixed_Variable_Block_Flag = 1;
    int Record_Length1 = 0x100;
    int Record_Length2 = 0;
    int Record_Length3 = 0;

    if(is_raw_pack_spack) {
        x41413c = (is_raw_pack_spack < 0) ? 0 : 1;
    } else {
        Version_Flag = mybits_read(&ctx, 8, 1);
        switch(Version_Flag) {
            case 2:
                x41413c = 0;
                break;
            case 5:
                break;
            case 1:
            case 7:
                x414130 = 0;
                x414140 = 0;
                break;
            default:
                break;
        }
        if(x414140) {
            Fixed_Variable_Block_Flag = mybits_read(&ctx, 8, 1);
            Record_Length1 = mybits_read(&ctx, 16, 1);
            /*Filler1 =*/ mybits_read(&ctx, 16, 1);
            /*Filler2 =*/ mybits_read(&ctx, 16, 1);
            Record_Length2 = mybits_read(&ctx, 16, 1);
            Record_Length3 = mybits_read(&ctx, 16, 1);
        } else {
            Fixed_Variable_Block_Flag = mybits_read(&ctx, 8, 1);
            Record_Length1 = mybits_read(&ctx, 8, 1);
            /*Filler1 =*/ mybits_read(&ctx, 8, 1);
        }
    }
    if(force_binary) x414140 = 0;   // -b option

    x414144 = Fixed_Variable_Block_Flag;
    if(Record_Length1) x414154 = Record_Length1;
    else               x414154 = (Record_Length2 << 16) | Record_Length3;

    memset(terse_table, 0, sizeof(terse_table));
    for(i = 0; i < 0x1000; i++) {
        if(i <= 257) {
            terse_table[i].n1 = -1;
            terse_table[i].n2 = i;
            if(i > 0) terse_table[i].n4 = -1;
        } else {
            terse_table[i].n1 = -1;
            terse_table[i].n2 = -1;
            terse_table[i].n4 = i + 1;
        }
    }
    terse_table[i - 1].n4 = -1;
    terse_table[i].n4 = -1;

    signed short _t239;
    if(x41413c) {

        // SPACK
        signed short _v312 = 258;
        signed short _t228 = -1;
        do {
            // read
            if(ctx.eof) break;
            _t239 = mybits_read(&ctx, 12, 1);

            if(_v312 == -1) {
                signed short _t26 = terse_table[0].n4;
                signed short _t28 = terse_table[_t26].n4;
                signed short _t38 = terse_table[_t26].n3;
                terse_table[_t28].n3 = _t38;
                terse_table[_t38].n4 = _t28;
                signed short _t30 = terse_table[_t26].n1;
                signed short _t45 = terse_table[_t30].n4;
                if(_t45 != -1) {
                    terse_table[_t30].n4 = _t45 + 1;
                } else {
                    signed short _t50 = terse_table[0].n3;
                    terse_table[_t30].n4 = 0;
                    terse_table[0].n3 = _t30;
                    terse_table[_t30].n3 = _t50;
                    terse_table[_t50].n4 = _t30;
                }
                signed short _t32 = terse_table[_t26].n2;
                signed short _t47 =  terse_table[_t32].n4;
                if(_t47 != -1) {
                    terse_table[_t32].n4 = _t47 + 1;
                    terse_table[_t26].n4 = _v312;
                    _v312 = _t26;
                } else {
                    signed short _t49 =  terse_table[0].n3;
                    terse_table[_t32].n4 = 0;
                    terse_table[0].n3 = _t32;
                    terse_table[_t32].n3 = _t49;
                    terse_table[_t49].n4 = _t32;
                    terse_table[_t26].n4 = _v312;
                    _v312 = _t26;
                }
            }

            // output
            terse_decompress_out(&io, _t239);

            // update terse_table (from second code)
            if(_t228 >= 0) {
                signed short _t190 = _v312;
                _v312 =  terse_table[_t190].n4;
                terse_table[_t190].n1 = _t228;
                terse_table[_t190].n2 = _t239;
                signed short _t213 = terse_table[_t228].n4;
                if(_t213 >= 0) {
                    terse_table[0].n4 = _t213;
                    terse_table[_t213].n3 = terse_table[_t228].n3;
                    terse_table[_t228].n4 = -1;
                } else {
                    terse_table[_t228].n4 = _t213 - 1;
                }
                signed short _t133 = _t239;
                signed short _t216 = terse_table[_t133].n4;
                if(_t216 >= 0) {
                    signed short _t233 = terse_table[_t133].n3;
                    terse_table[_t233].n4 = _t216;
                    terse_table[_t216].n3 = _t233;
                    terse_table[_t133].n4 = -1;
                } else {
                    terse_table[_t133].n4 = _t216 - 1;
                }
                signed short _t134 = terse_table[0].n3;
                terse_table[_t190].n4 = 0;
                terse_table[0].n3 = _t190;
                terse_table[_t190].n3 = _t134;
                terse_table[_t134].n4 = _t190;
            }
            _t228 = _t239;
        } while(_t239 != 0);

    } else {

        // PACK
        memset(x43a880, 0, sizeof(x43a880));
        memset(x43c880, 0, sizeof(x43c880));
        memset(x43e880, 0, sizeof(x43e880));
        memset(x440880, 0, sizeof(x440880));

        int c = 'A';
        for(i = 258; i < 0x1000; i++) {
            x43c880[i] = c;
        }
        x43e880[258] = c;
        c = 258;
        for(i = 258 + 1; i < 0x1000; i++) {
            x43e880[i] = c++;
        }
        c = 258;
        for(i = 0; c < 0xfff; i++) {
            x43a880[258 + 1 + i] = c;
            c++;
            x440880[258 + i] = c;
        }
        x43a880[0] = 0xfff;
        x440880[0] = 258;

        signed short __esp20 = 0;
        signed short _t209 = mybits_read(&ctx, 12, 1);
        while(_t209) {
            signed short _t153 =  x43a880[0];
            signed short _t216 = _t153;
            signed short _t136 =  x43a880[_t216];
            signed short _t197 = 0;
            x440880[_t136] = 0;
            signed short __esp16 = _t153;
            signed short __esp24 = _t216;
            x43a880[0] = _t136;
            signed short _t169 = _t153;

            while(_t209 > 257) {
                signed short _t147 = _t209;
                signed short _t156 = x43a880[_t147];
                signed short _t189 = x440880[_t147];
                x440880[_t156] = _t189;
                x43a880[_t189] = _t156;
                signed short _t209_tmp = x43e880[_t147];
                x440880[_t147] = _t169;
                x43a880[_t169] = _t209;
                x43e880[_t147] = _t197;
                _t169 = _t209;
                _t197 = _t209;
                _t209 = _t209_tmp;
            }
            _t153 = __esp16;
            _t216 = __esp24;

            signed short _t137 =  x440880[0];
            x440880[_t216] = _t137;
            x440880[0] = _t169;
            x43a880[_t137] = _t153;
            x43a880[_t169] = 0;
            x43c880[__esp20] = _t209;
            terse_decompress_out(&io, _t209);
            __esp20 = _t153;
            while(_t197) {
                __esp16 = x43e880[_t197];
                terse_decompress_out(&io, x43c880[_t197]);
                x43e880[_t197] = _t209;
                _t209 = _t197;
                _t197 = __esp16;
            }

            x43e880[_t216] = _t209;
            if(ctx.eof) break;
            _t209 = mybits_read(&ctx, 12, 1);
        }
    }

    return io.o - out;
}

