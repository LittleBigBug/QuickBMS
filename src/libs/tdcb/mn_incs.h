// common functions and defines for algorithms created by Mark Nelson
// made by Luigi Auriemma

// it's not important if the following work or not
//#define putchar(...) do_nothing()
//#define printf(...) do_nothing()
//#define fprintf(...) do_nothing()
//#define getc(...)   return_eof()    //REMOVE THIS FUNCTION
//#define putc(...)   return_eof()    //REMOVE THIS FUNCTION
//#define read(...)   return_eof()    //REMOVE THIS FUNCTION
//#define write(...)  return_eof()    //REMOVE THIS FUNCTION
#define QUICK_EXPAND(X) \
int X##_ExpandMemory(unsigned char *in, int insz, unsigned char *out, int outsz) { \
    int     argc = 0; \
    char    **argv = NULL; \
    BIT_FILE *input; \
    memory_file output; \
    input        = OpenInputBitFile(in, in + insz); \
    output.data  = out; \
    output.datal = out + outsz;

typedef struct {
    unsigned char *datas;
    unsigned char *data;
    unsigned char *datal;
} memory_file;
typedef struct {
    memory_file file;
    unsigned char mask;
    int rack;
} BIT_FILE;

extern int mn_getc(memory_file *x);
extern int mn_putc(int chr, memory_file *x);
extern void fatal_error(char *msg, ...);
extern void do_nothing(void);
extern int return_eof(void);

extern BIT_FILE *OpenOutputBitFile( unsigned char *out, unsigned char *outl );
extern BIT_FILE *OpenInputBitFile( unsigned char *in, unsigned char *inl );
extern void CloseOutputBitFile( BIT_FILE * );
extern void CloseInputBitFile( BIT_FILE * );
extern void OutputBit( BIT_FILE *, int );
extern void OutputBits( BIT_FILE *, unsigned long, int );
extern int InputBit( BIT_FILE * );
extern unsigned long InputBits( BIT_FILE *, int );
extern void FilePrintBinary( void *, unsigned int, int );

