// experimental, probably doesn't work

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KOFdecompress_history(X) \
    { \
        if(o >= ol) break; \
        if((X) < 0) break; \
        if((X) >= (o - Ubuffer)) break; \
        *o++ = Ubuffer[X]; \
    }

int KOFdecompress(unsigned char *fstream, int fstreamsz, unsigned char *Ubuffer, int Usize, int method) {
  int ctrlBits;
    unsigned char   *o  = Ubuffer;
    unsigned char   *ol = Ubuffer + Usize;
    unsigned char   *fstreaml = fstream + fstreamsz;
    int j, jj;
 
    while (o < ol) { //(o - Ubuffer) < Usize) {
        if(fstream >= fstreaml) break;
        ctrlBits = 0xff00 | *fstream++;
        for(;;) {
            if(!(ctrlBits & 0xff00)) break;
            if(ctrlBits & 1) {
                ctrlBits >>= 1;
                if(o >= ol) break;
                if(fstream >= fstreaml) break;
                *o++ = *fstream++;
            } else {
                ctrlBits >>= 1;
                int len = 0;
                int ofs = 0;
                if(!(ctrlBits & 0xff00)) break;
                if(!(ctrlBits & 1)) {
                    ctrlBits >>= 1;
                    if(!(ctrlBits & 0xff00)) break;
                    len = (ctrlBits & 1) * 2 + (ctrlBits & 2) + 2;
                    ctrlBits >>= 2;
                    if(fstream >= fstreaml) break;
                    ofs = *fstream++ - 256;
                } else {
                    ctrlBits >>= 1;
                    if(fstream >= fstreaml) break;
                    int ctrlH  = *fstream++;
                    if(fstream >= fstreaml) break;
                    int ctrlL  = *fstream++;
                    len = ctrlL % 32;
                    if(!len) {
                        if(fstream >= fstreaml) break;
                        len = 1 + *fstream++;
                    } else len += 2;
                    if(!method) {
                        ofs = ((ctrlH % 32)*8+(ctrlL/32))*256 + 0xF8 + (ctrlH/32);
                        if(ofs > 0x7FFF) ofs -= 65536;
                    } else {
                        ofs = ((ctrlH % 32)*8+(ctrlL/32)) + (0xF8 + (ctrlH/32))*256 - 65536;
                    }
                }

              int loc = ofs + (o - Ubuffer);
              ofs = abs(ofs);
              if(ofs >= len) {
               for(j = 1; j <= len; j++) KOFdecompress_history(loc+j)
              } else {
               for(j = 1; j <= len/ofs; j++) {
                  for(jj = 1; jj <= ofs; jj++) KOFdecompress_history(loc+jj)
               }
               for(j = 1; j <= (len % ofs); j++) KOFdecompress_history(loc+j)
              }
            }
        }
    }
    return o - Ubuffer;
}


int _KOFdecompress(unsigned char *fstream, int fstreamsz, unsigned char *Ubuffer, int Usize, int method) {
    #define KOFdecompress_ctrlBits_test(X)  (ctrlBits & (1 << (X-1)))
    int ctrlBits;
    unsigned char   *o  = Ubuffer;
    unsigned char   *ol = Ubuffer + Usize;
    unsigned char   *fstreaml = fstream + fstreamsz;
    int j, jj;
 
    do {
        if(fstream >= fstreaml) break;
        ctrlBits = *fstream++;
        int ctrlBits_count = 8;
        do {
            if(ctrlBits_count < 1) break;
            if(KOFdecompress_ctrlBits_test(1) == 1) {
                if(o >= ol) break;
                if(fstream >= fstreaml) break;
                *o++ = *fstream++;
                ctrlBits >>= 1;
                ctrlBits_count -= 1;
            } else {
                int len = 0;
                int ofs = 0;
                if(ctrlBits_count < 2) break;
                if(KOFdecompress_ctrlBits_test(2) == 0) {
                    if(ctrlBits_count < 4) break;
                    len = KOFdecompress_ctrlBits_test(3)*2 + KOFdecompress_ctrlBits_test(4) + 2;
                    if(fstream >= fstreaml) break;
                    ofs = *fstream++ - 256;
                    ctrlBits >>= 4;
                    ctrlBits_count -= 4;
                } else {
                    if(fstream >= fstreaml) break;
                    int ctrlH  = *fstream++;
                    if(fstream >= fstreaml) break;
                    int ctrlL  = *fstream++;
                    len = ctrlL % 32;
                    if(len == 0) {
                        if(fstream >= fstreaml) break;
                        len = 1 + *fstream++;
                    } else len += 2;
                    if(!method) {
                        ofs = ((ctrlH % 32)*8+(ctrlL/32))*256 + 0xF8 + (ctrlH/32);
                    } else {
                        ofs = ((ctrlH % 32)*8+(ctrlL/32)) + ((0xF8 + (ctrlH/32))*256); //- 65536;
                    }
                    if(ofs > 0x7FFF) ofs -= 65536;
                    ctrlBits >>= 2;
                    ctrlBits_count -= 2;
                }

                int loc = ofs + (o - Ubuffer);
                ofs = abs(ofs);
                if(ofs >= len) {
                    for(j = 1; j <= len; j++) KOFdecompress_history(loc+j)
                } else {
                    for(j = 1; j <= len/ofs; j++) {
                        for(jj = 1; jj <= ofs; jj++) KOFdecompress_history(loc+jj)
                    }
                    for(j = 1; j <= (len % ofs); j++) KOFdecompress_history(loc+j)
                }
            }
            if(o >= ol) break;
            //if ((ctrlBits_count == 0)) break;
            //if ((ctrlBits_count==1)&&(KOFdecompress_ctrlBits_test(1)==0)) break;
            //if ((ctrlBits_count <= 3)&&(KOFdecompress_ctrlBits_test(1)==0)&&(KOFdecompress_ctrlBits_test(2)==0)) break;
        } while (1==1);
    } while (o < ol);
  return o - Ubuffer;
}

