// modified by Luigi Auriemma
/* File PC1DEC.c */
/* written in Borland Turbo C 2.0 on PC */
/* PC1 Cipher Algorithm ( Pukall Cipher 1 ) */
/* By Alexander PUKALL 1991 */
/* free code no restriction to use */
/* please include the name of the Author in the final software */
/* the Key is 128 bits */
/* Tested with Turbo C 2.0 for DOS and Microsoft Visual C++ 5.0 for Win 32 */

/* Note that PC1COD.c is the encryption routine */
/* PC1DEC.c is the decryption routine */
/* Only the K zone change in the two routines */
/* You can create a single routine with the two parts in it */

#include <stdio.h>
#include <string.h>


int pc1_128(unsigned char *cle, unsigned char *data, int size, int decenc) {

unsigned short ax, bx, cx, dx, si, tmp, x1a2, x1a0[8], res, i, inter, cfc,
    cfd, compte;
short c;

#define /*inline void*/ pc1_128_code/*(void)*/ \
{ \
    dx = x1a2 + i; \
    ax = x1a0[i]; \
    cx = 0x015a; \
    bx = 0x4e35; \
    \
    tmp = ax; \
    ax = si; \
    si = tmp; \
    \
    tmp = ax; \
    ax = dx; \
    dx = tmp; \
    \
    if (ax != 0) { \
        ax = ax * bx; \
    } \
    \
    tmp = ax; \
    ax = cx; \
    cx = tmp; \
    \
    if (ax != 0) { \
        ax = ax * si; \
        cx = ax + cx; \
    } \
    \
    tmp = ax; \
    ax = si; \
    si = tmp; \
    ax = ax * bx; \
    dx = cx + dx; \
    \
    ax = ax + 1; \
    \
    x1a2 = dx; \
    x1a0[i] = ax; \
    \
    res = ax ^ dx; \
    i = i + 1; \
}

    si = 0;
    x1a2 = 0;
    i = 0;

    int cnt;

    for(cnt = 0; cnt < size; cnt++) {
        c = data[cnt];
        //pc1_128_assemble();

//inline void pc1_128_assemble(void)
//{

    x1a0[0] = (cle[0] * 256) + cle[1];
    pc1_128_code/*();*/
    inter = res;

    x1a0[1] = x1a0[0] ^ ((cle[2] * 256) + cle[3]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[2] = x1a0[1] ^ ((cle[4] * 256) + cle[5]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[3] = x1a0[2] ^ ((cle[6] * 256) + cle[7]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[4] = x1a0[3] ^ ((cle[8] * 256) + cle[9]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[5] = x1a0[4] ^ ((cle[10] * 256) + cle[11]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[6] = x1a0[5] ^ ((cle[12] * 256) + cle[13]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    x1a0[7] = x1a0[6] ^ ((cle[14] * 256) + cle[15]);
    pc1_128_code/*();*/
    inter = inter ^ res;

    i = 0;
//}

        cfc = inter >> 8;
        cfd = inter & 255;        /* cfc^cfd = random byte */

/* K ZONE !!!!!!!!!!!!! */
/* here the mix of c and cle[compte] is after the decryption of c */

        if(!decenc) c = c ^ (cfc ^ cfd);

        for (compte = 0; compte <= 15; compte++) {
/* we mix the plaintext byte with the key */
            cle[compte] = cle[compte] ^ c;
        }

        if(decenc) c = c ^ (cfc ^ cfd);

        data[cnt] = c;
    }
    return(size);
}



int pc1_256(unsigned char *cle, unsigned char *data, int size, int decenc) {

unsigned short ax, bx, cx, dx, si, tmp, x1a2, x1a0[16], res, i, inter, cfc,
    cfd, compte;
short c;

#define /*inline void*/ pc1_256_code/*(void)*/ \
{ \
    dx = x1a2 + i; \
    ax = x1a0[i]; \
    cx = 0x015a; \
    bx = 0x4e35; \
    \
    tmp = ax; \
    ax = si; \
    si = tmp; \
    \
    tmp = ax; \
    ax = dx; \
    dx = tmp; \
    \
    if (ax != 0) { \
        ax = ax * bx; \
    } \
    \
    tmp = ax; \
    ax = cx; \
    cx = tmp; \
    \
    if (ax != 0) { \
        ax = ax * si; \
        cx = ax + cx; \
    } \
    \
    tmp = ax; \
    ax = si; \
    si = tmp; \
    ax = ax * bx; \
    dx = cx + dx; \
    \
    ax = ax + 1; \
    \
    x1a2 = dx; \
    x1a0[i] = ax; \
    \
    res = ax ^ dx; \
    i = i + 1; \
}

    si = 0;
    x1a2 = 0;
    i = 0;

    int cnt;
    for(cnt = 0; cnt < size; cnt++) {
        c = data[cnt];
        //pc1_256_assemble();

//inline void pc1_256_assemble(void)
//{

    x1a0[0] = (cle[0] * 256) + cle[1];
    pc1_256_code/*();*/
    inter = res;

    x1a0[1] = x1a0[0] ^ ((cle[2] * 256) + cle[3]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[2] = x1a0[1] ^ ((cle[4] * 256) + cle[5]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[3] = x1a0[2] ^ ((cle[6] * 256) + cle[7]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[4] = x1a0[3] ^ ((cle[8] * 256) + cle[9]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[5] = x1a0[4] ^ ((cle[10] * 256) + cle[11]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[6] = x1a0[5] ^ ((cle[12] * 256) + cle[13]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[7] = x1a0[6] ^ ((cle[14] * 256) + cle[15]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[8] = x1a0[7] ^ ((cle[16] * 256) + cle[17]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[9] = x1a0[8] ^ ((cle[18] * 256) + cle[19]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[10] = x1a0[9] ^ ((cle[20] * 256) + cle[21]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[11] = x1a0[10] ^ ((cle[22] * 256) + cle[23]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[12] = x1a0[11] ^ ((cle[24] * 256) + cle[25]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[13] = x1a0[12] ^ ((cle[26] * 256) + cle[27]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[14] = x1a0[13] ^ ((cle[28] * 256) + cle[29]);
    pc1_256_code/*();*/
    inter = inter ^ res;

    x1a0[15] = x1a0[14] ^ ((cle[30] * 256) + cle[31]);
    pc1_256_code/*();*/
    inter = inter ^ res;


    i = 0;
//}


        cfc = inter >> 8;
        cfd = inter & 255;        /* cfc^cfd = random byte */

/* K ZONE !!!!!!!!!!!!! */
/* here the mix of c and cle[compte] is after the decryption of c */

        if(!decenc) c = c ^ (cfc ^ cfd);

        for (compte = 0; compte <= 31; compte++) {
/* we mix the plaintext byte with the key */

            cle[compte] = cle[compte] ^ c;
        }

        if(decenc) c = c ^ (cfc ^ cfd);

        data[cnt] = c;
    }
    return(size);
}


