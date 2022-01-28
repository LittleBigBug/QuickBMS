
#include "LZHL.h"
#include <stdlib.h>
#include <stdio.h>

#define MAXBUFSIZE 65536

int compare(size_t inbufsize, unsigned char* inbuf, size_t outbufsize, unsigned char* outbuf)
{
    size_t i;
    
    if(inbufsize != outbufsize) {
        fprintf(stderr, "Buffer sizes differ after compression/decompression\n");
        return 1;
    }
    
    for(i=0; i<inbufsize; i++) {
        if(inbuf[i] != outbuf[i]) {
            fprintf(stderr, "Byte at offset %lu did not match\n", i);
            return 1;
        }
    }
    
    return 0;
}

int main(int argc, char** argv)
{
    void* compressor;
    void* decompressor;
    unsigned char inbuf[MAXBUFSIZE];
    unsigned char compbuf[MAXBUFSIZE];
    unsigned char outbuf[MAXBUFSIZE];
    size_t count, inbufsize, compbufsize, outbufsize, i;
    int errors = 0;

    compressor = initComp();
    decompressor = initDecomp();

    count = 10;
    while(count--) {
        /* Initialize random buffer size with uniform data */
        inbufsize = rand() % MAXBUFSIZE;
        for(i=0; i<inbufsize; i++) {
            inbuf[i] = 0xff;
        }

        /* Compress & Decompress */
        compbufsize = compress(compressor, inbuf, inbufsize, compbuf);
        outbufsize = decompress(decompressor, compbuf, compbufsize, outbuf, MAXBUFSIZE);
        fprintf(stderr, "Uniform data: %lu bytes input to %lu bytes compressed to %lu bytes uncompressed\n", inbufsize, compbufsize, outbufsize);
        errors += compare(inbufsize, inbuf, outbufsize, outbuf);
    }

    count = 10;
    while(count--) {
        /* Initialize random buffer size with sequential data */
        inbufsize = rand() % MAXBUFSIZE;
        for(i=0; i<inbufsize; i++) {
            inbuf[i] = i%0xff;
        }

        /* Compress & Decompress */
        compbufsize = compress(compressor, inbuf, inbufsize, compbuf);
        outbufsize = decompress(decompressor, compbuf, compbufsize, outbuf, MAXBUFSIZE);
        fprintf(stderr, "Sequential data: %lu bytes input to %lu bytes compressed to %lu bytes uncompressed\n", inbufsize, compbufsize, outbufsize);
        errors += compare(inbufsize, inbuf, outbufsize, outbuf);
    }

    count = 10;
    while(count--) {
        /* Initialize random buffer size with random data */
        inbufsize = rand() % MAXBUFSIZE;
        for(i=0; i<inbufsize; i++) {
            inbuf[i] = rand()%0xff;
        }

        /* Compress & Decompress */
        compbufsize = compress(compressor, inbuf, inbufsize, compbuf);
        outbufsize = decompress(decompressor, compbuf, compbufsize, outbuf, MAXBUFSIZE);
        fprintf(stderr, "Random data: %lu bytes input to %lu bytes compressed to %lu bytes uncompressed\n", inbufsize, compbufsize, outbufsize);
        errors += compare(inbufsize, inbuf, outbufsize, outbuf);
    }

    count = 10;
    while(count--) {
        /* Initialize random buffer size with semi-random data */
        inbufsize = rand() % MAXBUFSIZE;
        for(i=0; i<inbufsize; i++) {
            inbuf[i] = rand()%0x08;
        }

        /* Compress & Decompress */
        compbufsize = compress(compressor, inbuf, inbufsize, compbuf);
        outbufsize = decompress(decompressor, compbuf, compbufsize, outbuf, MAXBUFSIZE);
        fprintf(stderr, "Semi-random data: %lu bytes input to %lu bytes compressed to %lu bytes uncompressed\n", inbufsize, compbufsize, outbufsize);
        errors += compare(inbufsize, inbuf, outbufsize, outbuf);
    }

    delComp(compressor);
    delDecomp(decompressor);
    return EXIT_SUCCESS;
}
