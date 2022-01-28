////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                     Copyright (c) 2016 David Bryant                    //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#ifndef LZWLIB_H_
#define LZWLIB_H_

int lzw_compress (void (*dst)(int), int (*src)(void), int maxbits);
int lzw_decompress (void (*dst)(int), int (*src)(void));

#endif /* LZWLIB_H_ */
