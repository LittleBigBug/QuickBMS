#include "../libs/iris/decompress.h"
#include "../libs/iris/huffman.h"
#include "../libs/iris/uo_huffman.h"



extern "C" int iris_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    cDecompressor decompressor;
    int in_size = insz;
    outsz = decompressor.Decompress(in, out, in_size, outsz);
    return(outsz);
}



extern "C" int iris_huffman(char *in, int insz, char *out, int outsz) {
    DecompressingCopier decompressor;
    int in_size = insz,
        out_size = outsz;
    decompressor.Decode (out, in, in_size, out_size);
    return(out_size);
}



extern "C" int iris_uo_huffman(char *in, int insz, char *out, int outsz) {
    uo_DecompressingCopier decompressor;
    int in_size = insz,
        out_size = outsz;
    decompressor.initialise();
    decompressor (out, in, in_size, out_size);
    return(out_size);
}


