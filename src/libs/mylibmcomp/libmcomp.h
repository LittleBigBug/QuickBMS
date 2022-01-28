/*
LibMComp wrapper 0.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

  int LibMComp(         // return the size of the compressed data or -1 if error
    void *input,        // decompressed buffer (input)
    int input_length,   // decompressed size (input size)
    void *output,       // destination buffer for the compressed data (output)
    int output_size,    // size of the output buffer (output size)
    int algo            // -1 for reading the file "as-is" or any value from 0 <= 0x14
  );

The job of this function is:
- launching the executables silently from the %TEMP% folder
- using named pipes for avoiding the usage of the disk
- emulating the access to the input/output files (named pipes are streams)
- patching the original executables
- providing a simple interface for memory input=>output compression
- this function has been tested on Windows XP, 7 and 8
*/ 



int LibMComp(
    void *,
    int,
    void *,
    int,
    int);

