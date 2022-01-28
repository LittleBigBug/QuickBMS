#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yalz77.h"



extern "C"
int yalz77_compress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    std::string uncompressed( reinterpret_cast<char const*>(in), insz ) ;
    lz77::compress_t compress;
    std::string compressed = compress.feed(uncompressed);
    if((compressed.size() < 0) || (compressed.size() > outsz)) return -1;
    outsz = compressed.size();
    memcpy(out, compressed.data(), outsz);
    compressed.clear();
    return outsz;
}



extern "C"
int yalz77_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    std::string compressed( reinterpret_cast<char const*>(in), insz ) ;
    lz77::decompress_t decompress;
    std::string temp;
    decompress.feed(compressed, temp);
    std::string& uncompressed = decompress.result();
    if((uncompressed.size() < 0) || (uncompressed.size() > outsz)) return -1;
    outsz = uncompressed.size();
    memcpy(out, uncompressed.data(), outsz);
    uncompressed.clear();
    return outsz;
}

