/*

unlzw 0.1.3a
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

Very simple and basic LZW memory decompressor (compatible with
gunzip) written by me during the reversing of the
COM_LZW_Decompress function (LZW_lib) used in the Vietcong game.
As already said this code has the simplicity in mind and I hope
my comments help a bit to understand of how the things work.
My first implementation used only one function and many #defines
but I have thought that is better to use a function for each
repeated sequence of operations and the needed global variables.
This code works on both little and big endian systems and some
variables have been optimized for speed and size (like dictlen).
In case of compatibility problems try to limit "bits" to 12 in
unlzw_dictionary.

The function prototype is the following:

  int unlzw(void *out, int outsize, void *in, int insize);

which returns the number of bytes written in output


    Copyright 2007-2015 Luigi Auriemma

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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>



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
                dictalloc,              // used only for dynamic dictionary allocation
                end_code;
static uint16_t dictlen;                // offset length (like dict_t.len)
static uint8_t  bits,                   // init bits (usually max 16)
                ibits,                  // inverted bits for catching the element
                *out;                   // output buffer



static int unlzw_init(int UNLZW_BITS, int UNLZW_END) {
    if(UNLZW_BITS <= 0) UNLZW_BITS = 9;
    if(UNLZW_END  <= 0) UNLZW_END  = 256;
    bits     = UNLZW_BITS;
    ibits    = 0;
    end_code = UNLZW_END;
    dictsize = UNLZW_END + 1;
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



static void unlzw_cpy(uint8_t *xout, uint8_t *in, int len) {
    int     i;

    for(i = 0; i < len; i++) {
        xout[i] = in[i];
    }
}



static int unlzw_expand(uint32_t code) {
    if(code >= dictsize) return(0);     // invalid so return 0

    if(code >= end_code) {             // put the data in the dictionary
        if((outlen + dict[code].len) > outsize) return(-1);
        unlzw_cpy(out + outlen, dict[code].data, dict[code].len);
        return(dict[code].len);
    }
                                        // put the byte
    if((outlen + 1) > outsize) return(-1);
    out[outlen] = code;
    return(1);
}



static int unlzw_dictionary(void) {     // fill the dictionary
    uint32_t    tmp;

    if(dictlen++) {
        if((dictoff + dictlen) > outsize) {
            dictlen = outsize - dictoff;
        }
        dict[dictsize].data = out + dictoff;
        dict[dictsize].len  = dictlen;
        dictsize++;
        if((dictsize + 1) >> bits) {
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



uint32_t unlzw(uint8_t *outbuff, uint32_t maxsize, uint8_t *in, uint32_t insize) {
    uint32_t    code,                   // current element
                inlen;                  // current input length
    int         n;                      // bytes written in the output

    code    = -1;                       // useless
    inlen   = 0;                        // current input length
    outlen  = 0;                        // current output length
    out     = outbuff;                  // needed only for the global var
    outsize = maxsize;                  // needed only for the global var

    dict    = NULL;                     // global var initialization
    if(unlzw_init(-1, -1) < 0) return(0);

    while(inlen < insize) {
        code = in[inlen];               // read at least 24 bits (16 + 7) and cut them
        if((insize - inlen) > 1) code |= in[inlen + 1] << 8;
        if((insize - inlen) > 2) code |= in[inlen + 2] << 16;
        code = (code >> ibits) & ((1 << bits) - 1);

        inlen += (bits + ibits) >> 3;   // increment the input length
        ibits  = (bits + ibits) &  7;   // adjust the inverted bits

        if(code == end_code) {         // means that we need to reset everything
            if(ibits) inlen++;

            if(unlzw_init(-1, -1) < 0) break;

            continue;                   // and restart the unpacking
        }

        if(code == dictsize) {          // I think this is used for repeated chars
            if(unlzw_dictionary()       // fill the dictionary
              < 0) break;

            n = unlzw_expand(code);     // unpack

        } else {
            n = unlzw_expand(code);

            if(unlzw_dictionary()
              < 0) break;
        }
        if(n < 0) break;                // break if unlzw_expand() failed

        dictoff = outlen;               // increment all the remaining values
        dictlen = n;
        outlen += n;
    }

    free(dict);                         // free memory
    return(outlen);                     // return the output length
}

