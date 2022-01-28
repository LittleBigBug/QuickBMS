/*

unlzwx 0.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

LZW memory decompressor compatible with the one used in the MIX
file archives of the Milestone games.

The function prototype is the following:

  int unlzwx(void *out, int outsize, void *in, int insize);

which returns the number of bytes written in output


    Copyright 2008 Luigi Auriemma

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

    http://www.gnu.org/licenses/gpl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>



#define UNLZW_BITS      9
#define UNLZW_END       256



#pragma pack(2)
typedef struct {
    uint8_t     *data;                  // data offset
    uint16_t    len;                    // data size, 16 bits: in over half gigabyte
} dict_t;                               // of zeroes this len was minor than 30000
#pragma pack()



static dict_t   *dict;                  // the dictionary
static uint32_t outlen,                 // current output length
                outsize,                // output buffer size
                dictsize,               // offset of the last element in the dictionary
                dictoff,                // offset of the new entry to add to the dictionary
                dictalloc;              // used only for dynamic dictionary allocation
static uint16_t dictlen;                // offset length (like dict_t.len)
static uint8_t  bits,                   // init bits (usually max 16)
                *out;                   // output buffer



static int unlzwx_init(void) {
    bits     = UNLZW_BITS;
    dictsize = UNLZW_END + 2;
    dictoff  = 0;
    dictlen  = 0;

    if(!dict) {                         // allocate memory for a dictionary of UNLZW_BITS bits
        dictalloc = sizeof(dict_t) * (1 << (UNLZW_BITS + 3));
        dict = malloc(dictalloc);       // + 3 is used for avoiding too much realloc() calls
        if(!dict) return(-1);
    }                                   // if dict still exists we use it
    memset((void *)dict, 0, dictalloc); // all lengths set to zero to avoid malicious crashes

    return(0);
}



static void unlzwx_cpy(uint8_t *out, uint8_t *in, int len) {
    int     i;

    for(i = 0; i < len; i++) {
        out[i] = in[i];
    }
}



static int unlzwx_expand(uint32_t code) {
    if(code >= dictsize) return(0);     // invalid so return 0

    if(code >= UNLZW_END) {             // put the data in the dictionary
        if((outlen + dict[code].len) > outsize) return(-1);
        unlzwx_cpy(out + outlen, dict[code].data, dict[code].len);
        return(dict[code].len);
    }
                                        // put the byte
    if((outlen + 1) > outsize) return(-1);
    out[outlen] = code;
    return(1);
}



static int unlzwx_dictionary(void) {     // fill the dictionary
    uint32_t    tmp;

    if(dictlen++) {
        if((dictoff + dictlen) > outsize) {
            dictlen = outsize - dictoff;
        }
        dict[dictsize].data = out + dictoff;
        dict[dictsize].len  = dictlen;
        dictsize++;
        if((dictsize >> bits) && (bits != 12)) {
            bits++;
                                        // dynamic dictionary
            tmp = sizeof(dict_t) * (1 << bits);
            if(tmp > dictalloc) {
                dict = realloc(dict, tmp);
                if(!dict) return(-1);
                memset((void *)dict + dictalloc, 0, tmp - dictalloc);
                dictalloc = tmp;
            }
        }
    }
    return(0);
}



uint32_t unlzwx(uint8_t *outbuff, uint32_t maxsize, uint8_t *in, uint32_t insize) {
    uint32_t    code,                   // current element
                inlen,                  // current input length
                cl,
                dl,
                totbits;
    int         n,                      // bytes written in the output
                i;

    code    = -1;                       // useless
    inlen   = 0;                        // current input length
    outlen  = 0;                        // current output length
    out     = outbuff;                  // needed only for the global var
    outsize = maxsize;                  // needed only for the global var
    totbits = 0;

    dict    = NULL;                     // global var initialization
    if(unlzwx_init() < 0) return(0);

    insize -= 2;
    while(inlen < insize) {
        cl = (in[inlen + 1] << 8) | in[inlen];
        dl = in[inlen + 2];
        for(i = 0; i < (totbits & 7); i++) {
            cl = (cl >> 1) | ((dl & 1) << 15);
            dl >>= 1;
        }
        code = ((1 << bits) - 1) & cl;

        totbits += bits;
        inlen = totbits >> 3;

        if(code == 257) break;
        if(code == UNLZW_END) {         // means that we need to reset everything
            if(unlzwx_init() < 0) break;

            continue;                   // and restart the unpacking
        }

        if(code == dictsize) {          // I think this is used for repeated chars
            if(unlzwx_dictionary()      // fill the dictionary
              < 0) break;

            n = unlzwx_expand(code);    // unpack

        } else {
            n = unlzwx_expand(code);

            if(unlzwx_dictionary()
              < 0) break;
        }
        if(n < 0) break;                // break if unlzwx_expand() failed

        dictoff = outlen;               // increment all the remaining values
        dictlen = n;
        outlen += n;
    }

    free(dict);                         // free memory
    return(outlen);                     // return the output length
}



#undef UNLZW_BITS
#undef UNLZW_END
