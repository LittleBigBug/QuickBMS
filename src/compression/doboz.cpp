#include "../libs/doboz/Decompressor.h"
#include "../libs/doboz/Compressor.h"
#include <string.h>

extern "C" int doboz_decompress(void *in, int insz, void *out, int outsz) {
    doboz::Decompressor z;
    doboz::CompressionInfo z_info;
    if(z.getCompressionInfo(in, insz, z_info)) return -1;
    if(z_info.uncompressedSize > outsz) return -2;
    if(z.decompress(in, insz, out, outsz)) return -3;
    return z_info.uncompressedSize;
}


extern "C" int doboz_compress(void *in, int insz, void *out, int outsz) {
    size_t  size;
    doboz::Compressor z;
    if(z.compress(in, insz, out, outsz, size)) return -1;
    return size;
}

