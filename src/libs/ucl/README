-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1



                 ooooo     ooo   .oooooo.   ooooo
                 `888'     `8'  d8P'  `Y8b  `888'
                  888       8  888           888
                  888       8  888           888
                  888       8  888           888
                  `88.    .8'  `88b    ooo   888       o
                    `YbodP'     `Y8bood8P'  o888ooooood8


                      The UCL Compression Library
                             Version 1.02

       Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
             Markus F.X.J Oberhumer <markus@oberhumer.com>
                       http://www.oberhumer.com


 Abstract
 --------

 UCL is a portable lossless data compression library written in ANSI C.

 UCL implements a number of compression algorithms that achieve an
 excellent compression ratio while allowing *very* fast decompression.
 Decompression requires no additional memory.

 UCL is distributed under the terms of the GNU General Public License (GPL).


 Overview
 --------

 UCL implements a number of algorithms with the following features:

   - Decompression is simple and *very* fast.
   - Requires no memory for decompression.
   - The decompressors can be squeezed into less than 200 bytes of code.
   - Includes compression levels for generating pre-compressed
     data which achieve an excellent compression ratio.
   - Allows you to dial up extra compression at a speed cost in the
     compressor. The speed of the decompressor is not reduced.
   - Algorithm is thread safe.
   - Algorithm is lossless.

 UCL supports in-place decompression.


 Design criteria
 ---------------

 UCL's main design goal was a very high decompression speed while
 achieving an excellent compression ratio. Real-time decompression should
 be possible for virtually any application. The implementation of the
 NRV2B decompressor in optimized i386 assembler code runs about at
 the fifth of the speed of a memcpy() - and even faster for many files.


 Related work
 ------------

 This section describes how UCL compares to some of my other
 compression technologies.


 * LZO
   ---
   http://www.oberhumer.com/opensource/lzo/
   LZO is distributed under the terms of the GNU GPL.

   LZO is a data compression library that focuses on
   *extremly* fast decompression, and also implements some
   pretty fast compression algorithms.


 * NRV
   ---
   http://www.oberhumer.com/nrv
   NRV is not publicly available.

   NRV started life as an experimental compression library and has since
   grown into a meta-generator for compression algorithms of almost any kind.

   NRV supports a virtually unlimited number of different algorithms by
   glueing typical data compression components. By using a combination
   of sophisticated high level abstractions and advanced information
   theory concepts it usually achieves an incredible compression ratio.


 * UCL
   ---
   http://www.oberhumer.com/opensource/ucl/
   UCL is distributed under the terms of the GNU GPL.

   UCL is a re-implementation of some concrete algorithms that have
   proven especially useful during the NRV development.

   As compared to LZO, the UCL algorithms achieve a better compression
   ratio but decompression is somewhat slower. See below.


 * UPX
   ---
   http://www.oberhumer.com/opensource/upx/
   UPX is distributed under the terms of the GNU GPL.

   UPX is a very powerful executable packer that can be configured
   to use either NRV or UCL for actual compression services. It
   currently supports the NRV2B and NRV2D algorithms.


 Portability
 -----------

 UCL's decompressors should work on any system around - they could even
 get ported to 8-bit processors such as the Z-80 or 6502.

 UCL's compressors currently require at least 32-bit integers. While
 porting them to more restricted environments (such as 16-bit DOS)
 should be possible without too much effort this is not considered
 important at this time.


 How fast is fast ?
 ------------------

 Here are some rough timings originally done on an ancient Intel Pentium 133:

   memcpy():                                    ~60 MB/sec
   LZO decompression in optimized assembler:    ~20 MB/sec
   LZO decompression in C:                      ~16 MB/sec
   UCL decompression in optimized assembler:    ~13 MB/sec

 Your mileage may vary and you're encouraged to run your own benchmarks.

 Also note that UCL's C language decompressors are not optimized very
 much yet, so you should use the assembler versions whenever possible.


 Documentation (preliminary)
 ---------------------------

 Currently UCL implements 3 of NRV's algorithms, namely NRV2B,
 NRV2D and NRV2E.

 UCL is a block compressor, i.e. each memory block passed to the
 compressor will get compressed independently.

 The API of UCL is basically identical to that of LZO. Due to current
 lack of documentation you are strongly advised to download LZO
 and study its documentation and example programs first:

     http://www.oberhumer.com/opensource/lzo/

 The API of UCL is actually very simple: there's a compress() function
 that compresses a block of memory, and there's a decompress() function
 that handles decompression. That's more or less all you need to know.

 See the `examples' directory for some demo programs.

 UCL will expand non-compressible data by a little amount. I suggest
 using this formula for a worst-case expansion calculation:

     output_block_size = input_block_size + (input_block_size / 8) + 256


 COPYRIGHT
 ---------

 The UCL library is Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002,
 2003 by Markus Franz Xaver Johannes Oberhumer <markus@oberhumer.com>.

 The UCL library is distributed under the terms of the GNU General Public
 License (GPL). See the file COPYING.

 Special licenses for commercial and other applications which
 are not willing to accept the GNU General Public License
 are available by contacting the author.



-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.2.3 (GNU/Linux)

iD8DBQE/4ZvKTWFXqwsgQ8kRAhF6AJwIR0ILMZeV9+ZCCCJ/roj/69PrLgCgshst
OMn0jebEgHVJ7+Z5Hva70tE=
=+AhB
-----END PGP SIGNATURE-----
