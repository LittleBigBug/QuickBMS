// modified by Luigi Auriemma
#include "LZHLDecoderStat.hpp"
#include "LZHLCompressor.hpp"
#include "LZHLDecompressor.hpp"

extern "C" {

  void *lzhlight_initComp(void) {
    return new LZHLCompressor();
  }

  void lzhlight_delComp(void *comp) {
    delete (LZHLCompressor *)comp;
  }

  size_t lzhlight_compress(void *comp, unsigned char *buf, size_t size, unsigned char *ret) {
    return ((LZHLCompressor *)comp)->compress(ret, buf, size);
  }

  void *lzhlight_initDecomp(void) {
    return new LZHLDecompressor();
  }

  size_t lzhlight_decompress(void *decomp, unsigned char *buf, size_t size, unsigned char *ret, size_t retsize) {
    ((LZHLDecompressor *)decomp)->decompress(ret, &retsize, buf, &size);
    return retsize;
  }

  void lzhlight_delDecomp(void *decomp) {
    delete (LZHLDecompressor *)decomp;
  }

}
