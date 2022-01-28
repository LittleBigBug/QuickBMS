/*
uberflate compression library 0.1
    kzip+deflopt+defluff+deflopt wrapper
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

  int uberflate(        // return the size of the compressed data or -1 if error
    void *input,        // decompressed buffer (input)
    int input_length,   // decompressed size (input size)
    void *output,       // destination buffer for the compressed data (output)
    int output_size,    // size of the output buffer (output size)
    int zlib            // 0 for deflate, 1 for zlib
  );

This function/wrapper allows to use the awesome compression performed by
the kzip [1] tool written by Ken Silverman plus the Huffman optimizations
of deflopt [2] and defluff [3] for removing additional bytes:

  kzip(/b128) + deflopt + defluff + deflopt = extreme deflate compression

[1] http://advsys.net/ken/utils.htm
[2] http://www.walbeehm.com/download/
[3] http://encode.ru/threads/1214-defluff-a-deflate-huffman-optimizer

The last "zlib" argument must be 0 for the deflate compression (RFC1591)
or 1 for the zlib format (RFC1590: header + deflate + crc).

The output buffer can be allocated using the following macro:

  outsz = UBERFLATE_MAXZIPLEN(insz);
  out   = malloc(outsz);

If "output" is NULL then the compressed output will be copied in the
input buffer:

  outsz = uberflate(in_out, insz, NULL, 0, 0);

Uberflate uses some internal functions available in QuickBMS
http://quickbms.aluigi.org src/utils.h, but I have implemented some
stand-alone versions here too.

The job of this function is:
- launching the executables silently from the %TEMP% folder
- using named pipes for avoiding the usage of the disk
- emulating the access to the input/output files (named pipes are streams)
- patching the original executables
- providing a simple interface for memory input=>output compression
- this function has been tested on Windows XP, 7 and 8
*/ 



#define UBERFLATE_MAXZIPLEN(n) ((n)+(((n)/100)+1)+256)  // original: ((n)+(((n)/1000)+1)+12)



int uberflate(
    void *,
    int,
    void *,
    int,
    int);

