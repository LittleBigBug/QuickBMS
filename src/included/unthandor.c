/*
unthandor 0.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

decompression algorithm reversed from the game Thandor,
I don't know what exact algorithm it is, anyway it gets the
dictionary from the first 256 bytes of the file.

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



int unthandor(uint8_t *in, int insz, uint8_t *out, int outsz) {
    uint32_t    tab[0x100 + 0x400 + 0x400],
                *tab1,  // unused
                *tab2,
                *tab3,
                *tabe,
                a,
                *b,
                d,
                *s,
                *t,
                *p;
    int         i;
    uint8_t     c,
                *o,
                *inl,
                *outl;

    o = out;
    if(insz < 0x100) goto quit;
    inl  = in + insz;
    outl = out + outsz;
    memset(&tab, 0, sizeof(tab));
    tab1 = &tab[0];
    tab2 = &tab[0x100];
    tab3 = &tab[0x100 + 0x400];

    for(i = 0; i < 0x100; i++) {
        tab1[i] = in[i];
        tab2[i * 4] = tab1[i];
    }

    b = tab2;
    t = tab3;
    for(p = tab3; p < &tab3[0x400]; p += 4) {
        a = -1;
        d = -1;
        s = tab2;
        for(i = 0; i < 0x200; i++) {
            if(*s) {
                if(a <= *s) {
                    if(d > *s) {
                        d = *s;
                        t = s;
                    }
                } else {
                    if(d > a) {
                        d = a;
                        t = b;
                    }
                    a = *s;
                    b = s;
                }
            }
            s += 4;
        }
        if((int)d < 0) {
            tabe = p - 4;
            c = 0;
            in += 0x100;
            for(;;) {
                b = tabe;
                a = *(uint32_t *)in >> c;
                c++;
                if(a & 1) {
                    a >>= 1;
                    d = a;
                    a >>= 4;
                    c += 4;
                    do {
                        if(a & 1) {
                            b = (uint32_t *)b[2];
                        } else {
                            b = (uint32_t *)b[1];
                        }
                        a >>= 1;
                        c++;
                    } while(b[1]);
                    while(c >= 8) {
                        in++;
                        if(in >= inl) goto quit;
                        c -= 8;
                    }
                    for(a = (d & 0xf) + 3; a; a--){
                        if(o >= outl) goto quit;
                        *o++ = ((uint8_t *)b - (uint8_t *)tab2) >> 4;
                    }
                } else {
                    a >>= 1;
                    do {
                        if(a & 1) {
                            b = (uint32_t *)b[2];
                        } else {
                            b = (uint32_t *)b[1];
                        }
                        a >>= 1;
                        c++;
                    } while(b[1]);
                    if(o >= outl) goto quit;
                    *o++ = ((uint8_t *)b - (uint8_t *)tab2) >> 4;
                    while(c >= 8) {
                        in++;
                        if(in >= inl) goto quit;
                        c -= 8;
                    }
                }
            }
            break;
        }
        p[0] = a + d;
        p[1] = (uint32_t)b;
        p[2] = (uint32_t)t;
        b[3] = (uint32_t)p;
        t[3] = (uint32_t)p;
        b[0] = 0;
        t[0] = 0;
    }
quit:
    return(o - out);
}


