/*
PKWARE reduce decompression 0.1
by Luigi Auriemma
e-mail: me@aluigi.org
web:    aluigi.org

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



static int reduce_decompress_B(unsigned char X) {
    static const unsigned char mask[] = {
        8,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };
    return mask[X];
}

static int reduce_decompress_L(int X, int factor) {
    return X & ((1 << (8 - factor)) -1);
}

static int reduce_decompress_F(int X, int factor) {
    if(reduce_decompress_L(X, factor) == reduce_decompress_L(-1, factor)) return 2;
    return 3;
}

static int reduce_decompress_D(int X, int Y, int factor) {
    return ((X >> (8 - factor)) * 256) + Y + 1;
}



int reduce_decompress(unsigned char *in, int insz, unsigned char *out, int outsz, int factor) {
    mybits_ctx_t    ctx;
    mybits_init(&ctx, in, insz, NULL);

    unsigned char   *o = out,
                    *outl = out + outsz;

    static const int    DLE = 144;  // 0x90
    unsigned char   *p;
    int     i,
            j;
    int     C       = 0,
            V       = 0,
            I       = 0,
            Len     = 0,
            State   = 0,
            Last_Character = 0;



    unsigned char   N[256],
                    S[256][64];

    memset(S, 0, sizeof(S));
    for(j = 0xff; j >= 0; j--) {
        N[j] = mybits_read(&ctx, 6, 0);
        for(i = 0; i < N[j]; i++) {
            S[j][i] = mybits_read(&ctx, 8, 0);
        }
    }



    while(!ctx.eof && (o < outl)) {

        if(N[Last_Character] == 0) {
            C = mybits_read(&ctx, 8, 0);
        } else {
            if(mybits_read(&ctx, 1, 0)) {
                C = mybits_read(&ctx, 8, 0);
            } else {
                I = mybits_read(&ctx, reduce_decompress_B(N[Last_Character]), 0);
                C = S[Last_Character][I];
            }
        }
        Last_Character = C;



        switch(State) {

            case 0:
                if(C != DLE) {
                    if(o < outl) *o++ = C;
                } else {
                    State = 1;
                }
                break;

            case 1:
                if(C != 0) {
                    V = C;
                    Len = reduce_decompress_L(V, factor);
                    State = reduce_decompress_F(Len, factor);
                } else {
                    if(o < outl) *o++ = DLE;
                    State = 0;
                }
                break;

            case 2:
                Len += C;
                State = 3;
                break;

            case 3:
                p = o - (reduce_decompress_D(V, C, factor) & 0x3fff);   // Winzip uses a 0x3fff mask here
                for(i = 0; i < (Len + 3); i++, p++) {
                    if(o < outl) {
                        if(p < out) *o++ = 0;
                        else        *o++ = *p;
                    }
                }
                State = 0;
                break;

            default: break;
        }
    }

    return o - out;
}


