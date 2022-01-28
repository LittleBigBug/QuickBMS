
BriefLZ - small fast Lempel-Ziv
===============================

Version 1.3.0

Copyright (c) 2002-2020 Joergen Ibsen

<http://www.ibsensoftware.com/>

[![Build Status](https://travis-ci.org/jibsen/brieflz.svg?branch=master)](https://travis-ci.org/jibsen/brieflz) [![Build status](https://ci.appveyor.com/api/projects/status/l9vhammx8p8hkrqb/branch/master?svg=true)](https://ci.appveyor.com/project/jibsen/brieflz/branch/master) [![codecov](https://codecov.io/gh/jibsen/brieflz/branch/master/graph/badge.svg)](https://codecov.io/gh/jibsen/brieflz)


About
-----

BriefLZ is a small and fast open source implementation of a Lempel-Ziv
style compression algorithm. The main focus is on speed and code footprint,
but the ratios achieved are quite good compared to similar algorithms.


Why BriefLZ?
------------

Two widely used types of Lempel-Ziv based compression libraries are those
that employ [entropy encoding][entropy] to achieve good ratios (like Brotli,
zlib, Zstd), and those that forgo entropy encoding to favor speed (like LZ4,
LZO).

BriefLZ attempts to place itself somewhere between these two by using a
[universal code][universal], which may improve compression ratios compared to
no entropy encoding, while requiring little extra code.

Not counting the optional lookup tables, the compression function `blz_pack`
is 147 LOC, and the decompression function `blz_depack` is 61 LOC (and can be
implemented in 103 bytes of x86 machine code).

If you do not need the extra speed of libraries without entropy encoding, but
want reasonable compression ratios, and the footprint of the compression or
decompression code is a factor, BriefLZ might be an option.

[entropy]: https://en.wikipedia.org/wiki/Entropy_encoding
[universal]: https://en.wikipedia.org/wiki/Universal_code_(data_compression)


Benchmark
---------

Here are some results from running [lzbench][] on the
[Silesia compression corpus][silesia] on a Core i5-4570 @ 3.2GHz:

| Compressor name         | Compression| Decompress.|  Compr. size  | Ratio |
| ---------------         | -----------| -----------| ------------- | ----- |
| brotli 2019-10-01 -9    |  5.80 MB/s |   432 MB/s |    56,697,015 | 26.75 |
| zstd 1.4.3 -9           |    29 MB/s |   848 MB/s |    60,185,637 | 28.40 |
| **brieflz 1.3.0 -9**    |  1.95 MB/s |   415 MB/s |    63,947,751 | 30.17 |
| zstd 1.4.3 -3           |   199 MB/s |   776 MB/s |    66,681,182 | 31.46 |
| **brieflz 1.3.0 -6**    |    17 MB/s |   400 MB/s |    67,208,420 | 31.71 |
| brotli 2019-10-01 -3    |    95 MB/s |   382 MB/s |    67,369,244 | 31.79 |
| zlib 1.2.11 -9          |    11 MB/s |   320 MB/s |    67,644,548 | 31.92 |
| zlib 1.2.11 -3          |    63 MB/s |   313 MB/s |    72,967,040 | 34.43 |
| brotli 2019-10-01 -1    |   220 MB/s |   338 MB/s |    73,499,222 | 34.68 |
| zstd 1.4.3 -1           |   345 MB/s |   880 MB/s |    73,508,823 | 34.68 |
| **brieflz 1.3.0 -3**    |    96 MB/s |   369 MB/s |    75,550,736 | 35.65 |
| zlib 1.2.11 -1          |    91 MB/s |   295 MB/s |    77,259,029 | 36.45 |
| lz4hc 1.9.2 -9          |    28 MB/s |  3417 MB/s |    77,884,448 | 36.75 |
| **brieflz 1.3.0 -1**    |   164 MB/s |   362 MB/s |    81,138,803 | 38.28 |
| lzo1x 2.10 -1           |   531 MB/s |   702 MB/s |   100,572,537 | 47.45 |
| lz4 1.9.2               |   550 MB/s |  3444 MB/s |   100,880,800 | 47.60 |

Please note that this benchmark is not entirely fair because BriefLZ has no
window size limit.

[lzbench]: https://github.com/inikep/lzbench
[silesia]: http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia


Usage
-----

The include file `include/brieflz.h` contains documentation in the form of
[doxygen][] comments. A configuration file is included, so you can simply run
`doxygen` to generate documentation in HTML format.

If you wish to compile BriefLZ on 16-bit systems, make sure to adjust the
constants `BLZ_HASH_BITS` and `DEFAULT_BLOCK_SIZE`.

When using BriefLZ as a shared library (dll on Windows), define `BLZ_DLL`.
When building BriefLZ as a shared library, define both `BLZ_DLL` and
`BLZ_DLL_EXPORTS`.

The `example` folder contains a simple command-line program, `blzpack`, that
can compress and decompress a file using BriefLZ. For convenience, the example
comes with makefiles for GCC and MSVC.

BriefLZ uses [CMake][] to generate build systems. To create one for the tools
on your platform, and build BriefLZ, use something along the lines of:

~~~sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
~~~

You can also simply compile the source files and link them into your project.
CMake just provides an easy way to build and test across various platforms and
toolsets.

[doxygen]: http://www.doxygen.org/
[CMake]: http://www.cmake.org/


License
-------

This projected is licensed under the [zlib License](LICENSE) (Zlib).
