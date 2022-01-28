#define NOJIT
  #include "../libs/libzpaq/libzpaq.cpp"
  #include <stdio.h>
  #include <stdlib.h>

unsigned char   *libzpaq_in    = NULL,
                *libzpaq_inl   = NULL,
                *libzpaq_out   = NULL,
                *libzpaq_outl  = NULL;

  void libzpaq::error(const char* msg) {  // print message and exit
    fprintf(stderr, "zpaq error: %s\n", msg);
    exit(1);
  }

  class libzpaq_In: public libzpaq::Reader {
  public:
    int get() {
        if(libzpaq_in >= libzpaq_inl) return -1;
        return *libzpaq_in++;
    }
  } libzpaq_in_func;

  class libzpaq_Out: public libzpaq::Writer {
  public:
    void put(int c) {
        if(libzpaq_out >= libzpaq_outl) return;   // ???
        *libzpaq_out++ = c;
    }
  } libzpaq_out_func;

extern "C" int zpaq_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    libzpaq_in    = in;
    libzpaq_inl   = in + insz;
    libzpaq_out   = out;
    libzpaq_outl  = out + outsz;
    libzpaq::decompress(&libzpaq_in_func, &libzpaq_out_func);
    return libzpaq_out - out;
}

extern "C" int zpaq_compress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    libzpaq_in    = in;
    libzpaq_inl   = in + insz;
    libzpaq_out   = out;
    libzpaq_outl  = out + outsz;
    libzpaq::compress(&libzpaq_in_func, &libzpaq_out_func, "14,128,0");
    return libzpaq_out - out;
}
