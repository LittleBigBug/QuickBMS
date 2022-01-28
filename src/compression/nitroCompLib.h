// nitroCompLib.h : nitroCompLib.DLL main header file
//

#ifndef __NITROCOMPLIB_H__
#define __NITROCOMPLIB_H__

//===========================================================================================
// include
//===========================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

# ifndef __stdcall
#  define __stdcall __attribute__ ((__stdcall__))
# endif

//===========================================================================================
// Type Definitions
//===========================================================================================
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

//===========================================================================================
// Prototype Declaration
//===========================================================================================
// For C++
#ifdef __cplusplus
  extern "C"
  {
#endif

//BOOL WINAPI DllMain( HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved);
u8     *__stdcall nitroCompMalloc(u32 size);
void __stdcall nitroCompFree(u8 *p);
u32 __stdcall nitroCompress(u8 *srcp, u32 size, u8 *dstp, char *compList, u8 rawHeader);
u32 __stdcall nitroDecompress(u8 *srcp, u32 size, u8 *dstp, s8 depth);
void __stdcall debugMemPrint(FILE * fp, u8 *str, u32 size);
void __stdcall debugMemBitPrint(FILE * fp, u8 *str, u32 size);
int __stdcall matchingCheck(u8 *srcp, u32 srcSize, u8 *dstp, u32 dstSize);

#ifdef __cplusplus
  }
#endif

//===========================================================================================
// How to use the functions
//===========================================================================================
//----------------------------------------------------------------------------------
//  Secure memory region to place compressed data
//    Secure a region that is twice the size the data before it is compressed.
//    Arguments
//      u32  size   Data size before compression
//                     u32 is an unsigned integer type.
//                     Usually unsigned int (depends on process type)
//    Return value
//      u8    *        Not a pointer to the region of data after compression
//                     (This is a generic pointer. Cast it as any type you like.)
//                                              .
//----------------------------------------------------------------------------------
//u8 *nitroCompMalloc(u32 size);

//----------------------------------------------------------------------------------
//  Free the memory region where the data was held after it was compressed.
//    Arguments
//      u8    *p   Pointer to the region for data after compression.
//----------------------------------------------------------------------------------
//void nitroCompFree(u8 *p);

//----------------------------------------------------------------------------------
//  Compress data
//  Use the argument compList to specify compression method and compression sequence.
//    Arguments
//              u8   *srcp      Pointer indicating the data that is targeted for compression.
//              u32    size      Size of the data that is targeted for compression (bytes)
//              u8   *dstp      Pointer indicating the data region for storing data after compression.
//                                                      You must secure a sufficient region.
//                                                      The nitroCompMalloc return value will be sufficient.
//              char *compList    This list holds the compression method, and compression sequence. (C language NULL character string)
//                                                      d8      : 8-bit difference filter
//                                                      d16     : 16-bit difference filter
//                                                      r : Run Length Encoding
//                                                      lx      : LZ77 x is the offset for the start point to search for identical data.
//                                                              : It must be at least 2. Maximum is 255
//                                                      h4      : 4-bit Huffman compression
//                                                      h8      : 8-bit Huffman compression
//              u8    rawHeaderFlag  This flag instructs whether to append header information that indicates that the data is not yet compressed.
//                                                      For 0 do not append. For 1 it will also be appended to
//                                                      data that has been expanded.
//        Return value
//          Data size after compression
//----------------------------------------------------------------------------------
//u32   nitroCompress(u8 *srcp, u32 size, u8 *dstp, char *compList, u8 rawHeader);

//----------------------------------------------------------------------------------
//  Expand data
//  Looks at the data header to determine the expansion method, and expansion sequence.
//    Arguments
//              u8   *srcp      Pointer to the data that is targeted for expansion.
//              u32    size      Size of the data that is targeted for expansion (bytes)
//              u8   *dstp      Pointer to the data region for storing data after it has been expanded.
//                                                      You must secure a sufficient region.
//                                                      The nitroCompMalloc return value will be sufficient.
//              u8    depth      Expansion depth (number of iterations)
//                          If less than 0, continue expanding until you can obtain the header information that indicates data prior to compression.
//        Return value
//          Data size after expansion
//----------------------------------------------------------------------------------
//u32   nitroDecompress(u8 *srcp, u32 size, u8 *dstp, s8 depth);

#endif // __NITROCOMPLIB_H__
