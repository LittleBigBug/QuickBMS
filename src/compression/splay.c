/*  
    this program is a straight pascal to C translation of a program that 
    compresses and decompresses files using 'Splay' trees.  This is the 
    original comment block: 
         
{***************************************************************************** 
  Compress and decompress files using the "splay tree" technique. 
  Based on an article by Douglas W. Jones, "Application of Splay Trees to 
  Data Compression", in Communications of the ACM, August 1988, page 996. 
 
  This is a method somewhat similar to Huffman encoding (SQZ), but which is 
  locally adaptive. It therefore requires only a single pass over the 
  uncompressed file, and does not require storage of a code tree with the 
  compressed file. It is characterized by code simplicity and low data 
  overhead. Compression efficiency is not as good as recent ARC 
  implementations, especially for large files. However, for small files, the 
  efficiency of SPLAY approaches that of ARC's squashing technique. 
 
  Usage: 
    SPLAY [/X] Infile Outfile 
 
    when /X is not specified, Infile is compressed and written to OutFile. 
    when /X is specified, InFile must be a file previously compressed by 
    SPLAY, and OutFile will contain the expanded text. 
 
    SPLAY will prompt for input if none is given on the command line. 
 
  Caution! This program has very little error checking. It is primarily 
  intended as a demonstration of the technique. In particular, SPLAY will 
  overwrite OutFile without warning. Speed of SPLAY could be improved 
  enormously by writing the inner level bit-processing loops in assembler. 
 
  Implemented on the IBM PC by 
    Kim Kokkonen 
    TurboPower Software 
    [72457,2131] 
    8/16/88 
*****************************************************************************} 
 
    This program is a _translation_ not a rewrite!!! It indexes arrays 
    pascal style (1..X) instead of C style (0..X-1), and a few other 
    non-C conventions.  This should compile under any C compiler, it only 
    uses about 4 or 5 standard library functions.  It was compiled using 
    Turbo C++ 1.0 in the small memory model.  I have included the 
    original Turbo Pascal source code for reference purposes.  Also, the 
    program usage is slightly different, no '/' is required before the 
    'X' parameter.  It has been tested with files up to approximately 
    450K and also successfully compressed and uncompressed ZIP files 
    (although the compressed version was bigger!).  I retained almost all 
    the original program comments for clarity.  I will, hopefully, 
    translate this to normal looking C sometime soon.  If you have any 
    questions, comments, or complaints, let me know. 
         
    Sean O'Connor 
    [74017,2501] 
    8-26-90 
         
    An aside, Douglas Jones was a professor I had while I was attending the 
    University of Iowa, so this program holds a special interest for me. 
*/ 
 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h> 
#include "../extra/mem2mem.h"
 
typedef unsigned int word; 
typedef unsigned char byte; 
typedef unsigned char bool; 
#define true  1 
#define false 0 
 
#define BufSize     16384       /* size of input and output buffers */ 
#define Sig         0xff02aa55L /* arbitrary signature denotes compressed file */ 
#define MaxChar     256         /* ordinal of highest character */ 
#define EofChar     256         /* used to mark end of compressed file */ 
#define PredMax     255         /* MaxChar - 1 */ 
#define TwiceMax    512         /* 2 * MaxChar */ 
#define Root        0           /* index of root node */ 
 
/* Used to unpack bits and bytes */ 
static byte BitMask[8]={1, 2, 4, 8, 16, 32, 64, 128}; 

typedef byte BufferArray[BufSize + 1]; 
typedef word CodeType;  /* 0..MaxChar */ 
typedef byte UpIndex;   /* 0..PredMax */ 
typedef word DownIndex; /* 0..TwiceMax */ 
typedef DownIndex TreeDownArray[PredMax + 1];  /* UpIndex */ 
typedef UpIndex TreeUpArray[TwiceMax + 1];     /* DownIndex */ 
 
static BufferArray InBuffer;           /* input file buffer */ 
static BufferArray OutBuffer;          /* output file buffer */ 
static char InName[80];                /* input file name */ 
static char OutName[80];               /* output file name */ 
static char CompStr[4];                /* response from Expand? prompt */ 
static FILE *InF;                      /* input file */ 
static FILE *OutF;                     /* output file */ 
 
static TreeDownArray Left, Right;      /* child branches of code tree */ 
static TreeUpArray Up;                 /* Parent branches of code tree */ 
static bool CompressFlag;              /* true to compress file */ 
static byte BitPos;                    /* current bit in byte */ 
static CodeType InByte;                /* current input byte */ 
static CodeType OutByte;               /* current output byte */ 
static word InSize;                    /* current chars in input buffer */ 
static word OutSize;                   /* Current chars in output buffer */ 
static word Index;                     /* general purpose index */ 
 
/* function prototypes */ 
static void InitializeSplay(void); 
static void Splay(CodeType Plain); 
static void FlushOutBuffer(void); 
static void WriteByte(void); 
static void Compress(CodeType Plain); 
static void ReadHeader(void); 
static byte GetByte(void); 
static CodeType Expand(void); 
static void ExpandFile(void); 
 
/* initialize the splay tree - as a balanced tree */ 
static void InitializeSplay(void) 
{ 
    DownIndex I; 
    int /*UpIndex*/ J; 
    DownIndex K; 
     
    for (I = 1; I <= TwiceMax; I++) 
        Up[I] = (I - 1) >> 1; 
    for (J = 0; J <= PredMax; J++) 
    { 
        K = ((byte)J + 1) << 1; 
        Left[J] = K - 1; 
        Right[J] = K; 
    } 
} 
 
/* rearrange the splay tree for each succeeding character */ 
static void Splay(CodeType Plain) 
{ 
    DownIndex A, B; 
    UpIndex C, D; 
     
    A = Plain + MaxChar; 
     
    do 
    { 
        /* walk up the tree semi-rotating pairs */ 
        C = Up[A]; 
        if (C != Root) 
        { 
            /* a pair remains */ 
            D = Up[C]; 
             
            /* exchange children of pair */ 
            B = Left[D]; 
            if (C == B) 
            { 
                B = Right[D]; 
                Right[D] = A; 
            } 
            else 
                Left[D] = A; 
             
            if (A == Left[C]) 
                Left[C] = B; 
            else 
                Right[C] = B; 
             
            Up[A] = D; 
            Up[B] = C; 
            A = D; 
        } 
        else 
            A = C; 
    } while (A != Root); 
} 
 
/* flush output buffer and reset */ 
static void FlushOutBuffer(void) 
{ 
    if (OutSize > 0) 
    { 
        fwrite(OutBuffer+1, sizeof(byte), OutSize, OutF); 
        OutSize = 0; 
    } 
} 
 
/* output byte in OutByte */ 
static void WriteByte(void) 
{ 
    if (OutSize == BufSize) 
        FlushOutBuffer(); 
    OutSize++; 
    OutBuffer[OutSize] = OutByte; 
} 
 
 
/* compress a single char */ 
static void Compress(CodeType Plain) 
{ 
    DownIndex A; 
    UpIndex U; 
    word Sp; 
    bool Stack[PredMax+1]; 
     
    A = Plain + MaxChar; 
    Sp = 0; 
     
    /* walk up the tree pushing bits onto stack */ 
    do 
    { 
        U = Up[A]; 
        Stack[Sp] = (Right[U] == A); 
        Sp++; 
        A = U; 
    } while (A != Root); 
     
    /* unstack to transmit bits in correct order */ 
    do 
    { 
        Sp--; 
        if (Stack[Sp]) 
            OutByte |= BitMask[BitPos]; 
        if (BitPos == 7) 
        { 
            /* byte filled with bits, write it out */ 
            WriteByte(); 
            BitPos = 0; 
            OutByte = 0; 
        } 
        else 
            BitPos++; 
    } while (Sp != 0); 
     
    /* update the tree */ 
    Splay(Plain); 
} 
 
/* compress input file, writing to outfile */ 
static void CompressFile(void) 
{ 
    /* compress file */ 
    OutSize = 0; 
    BitPos = 0; 
    OutByte = 0; 
    do 
    { 
        InSize = fread(InBuffer+1, sizeof(byte), BufSize, InF); 
        for (Index = 1; Index <= InSize; Index++) 
            Compress(InBuffer[Index]); 
    } while (InSize >= BufSize); 
     
    /* Mark end of file */ 
    Compress(EofChar); 
     
    /* Flush buffers */ 
    if (BitPos != 0) 
        WriteByte(); 
    FlushOutBuffer(); 
} 
 
/* return next byte from compressed input */ 
static byte GetByte(void) 
{ 
    Index++; 
    if (Index > InSize) 
    { 
        /* reload file buffer */ 
        InSize = fread(InBuffer+1, sizeof(byte), BufSize, InF); 
        Index = 1; 
        /* end of file handled by special marker in compressed file */ 
    } 
     
    /* get next byte from buffer */ 
    return InBuffer[Index]; 
} 
 
/* return next char from compressed input */ 
static CodeType Expand(void) 
{ 
    DownIndex A; 
     
    /* scan the tree to a leaf, which determines the character */ 
    A = Root; 
    do 
    { 
        if (BitPos == 7) 
        { 
            /* used up bits in current byte, get another */ 
            InByte = GetByte(); 
            BitPos = 0; 
        } 
        else 
            BitPos++; 
         
        if ((InByte & BitMask[BitPos]) == 0) 
            A = Left[A]; 
        else 
            A = Right[A]; 
    } while (A <= PredMax); 
     
    /* Update the code tree */ 
    A -= MaxChar; 
    Splay(A); 
     
    /* return the character */ 
    return A; 
} 
 
/* uncompress the file and write output */ 
static void ExpandFile(void) 
{ 
    /* force buffer load first time */ 
    Index = 0; 
    InSize = 0; 
    /* nothing in output buffer */ 
    OutSize = 0; 
    /* force bit buffer load first time */ 
    BitPos = 7; 
     
    /* read and expand the compressed input */ 
    OutByte = Expand(); 
    while (OutByte != EofChar) 
    { 
        WriteByte(); 
        OutByte = Expand(); 
    } 
     
    /* flush the output buffer */ 
    FlushOutBuffer(); 
} 
 
int splay_trees(unsigned char *in, int insz, unsigned char *out, int outsz, int do_compress) {

    mem2mem_init(in, insz, out, outsz);

    InitializeSplay(); 
    if(do_compress) { 
        CompressFile(); 
    } else {
        ExpandFile(); 
    }

    return mem2mem_ret(); 
} 

