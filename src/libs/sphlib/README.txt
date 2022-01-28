sphlib 3.0
==========

Overview
========

Sphlib is a set of implementations of various hash functions, both in C
and in Java. The C code is meant to be easily imported into other
projects, in particular embedded systems. The Java code implements
an API somewhat similar to that of java.security.MessageDigest.

The C source code also provides two standalone tools:
- sphspeed   performs speed tests on various hash functions
- sphsum     computes and verifies checksums over files


*************************************************************************
IMPORTANT NOTE: for users of the previous version (sphlib-2.1)
--------------------------------------------------------------
BLAKE, Groestl, JH, Keccak and Skein have been updated, to match the
"tweaked" specifications published for the third round of the SHA-3
competition. Thus, these function now return distinct values from what
they were producing previously. Also, for Skein with a 224-bit or
256-bit output, the size of the context structure has changed, so
calling code must be recompiled as well.
*************************************************************************


License
=======

Licensing is specified in the LICENSE.txt file. This is an MIT-like,
BSD-like open-source license. Basically, we will get the fame but not
the blame. If you reuse our code in your own projects, and distribute
the result, then you should state that you used our code and that we
always disclaimed any kind of warranty, and will continue to do so in
the foreseeable future, and beyond. You have no other obligation such as
disclosing your own source code. See the LICENSE.txt file for the
details in a lawyer-compatible language.

The authors are the "Projet RNRT SAPHIR", which is a research project
sponsored by the French government; project members are public and
private organizations:
- Cryptolog
- DCSSI
- Ecole Normale Superieure
- France Telecom
- Gemalto
Projet RNRT SAPHIR was continued into Projet RNRT SAPHIR2, with four
new additional members:
- EADS SN
- Sagem Securite
- INRIA
- UVSQ
We use the "Projet RNRT SAPHIR" expression to designate both SAPHIR and
SAPHIR2.

All the actual code has been written by:

   Thomas Pornin <thomas.pornin@cryptolog.com>

to whom technical questions may be addressed. Note that I do not claim
authorship: all writing was done on behalf of the Projet RNRT SAPHIR.


Documentation
=============

The programming interface for both the C code and the Java code can be
found in the doc/ subdirectory. This documentation is in HTML format and
was generated from the comments in the source code with, respectively,
doxygen and javadoc.


Conformance
===========

The hash functions have been implemented with regards to their
published specification. Whenever possible, the correction of the
implementation has been verified with regards to published test
vectors. Some functions have several variants; for instance, there
are three distinct "Whirlpool" which sphlib implements, under the
names "Whirlpool-0", "Whirlpool-1" and "Whirlpool".

For the SHA-3 candidates, sphlib follows the "round 3" specifications,
thus including the "tweaks" that some of the candidates added right
after round 1 and all also the tweaks that the "finalists" added after
round 2. For some of those functions, the officially submitted code and
test vectors turned out to be flawed (non conforming to the
specification), and corrections were published by their authors; sphlib
follows the specification and agrees with those corrected versions.

For two of the second round SHA-3 candidates (Hamsi and SHAvite-3), the
most recently published specifications (as of June 18th, 2010) have some
flaws which do not alter the function robustness or performance, but
still mean that some or all of the published implementations and test
vectors are wrong. The respective designers of those functions are aware
of those flaws and intend to publish corrections at some point. sphlib
anticipates on those corrections and already implements them.


Installation (C code)
=====================

The c/ subdirectory contain the C code. In that directory, there are two
Makefiles and a build shell script. The shell script, named "build.sh",
is for Unix-like systems.

sphlib does not feature a "proper" compilation and configuration system
such as those customarily found in open-source libraries for Unix
systems. This may be corrected in a future version. Right now, I am not
utterly convinced that the autoconf-generated scripts are the "way to
go". Anyway, sphlib is meant for evaluation, research and import into
other projects; a streamlined standalone compilation process is hardly
relevant for those usages.


All systems
-----------

By default, sphlib compiles for "big" architectures, using heavy loop
unrolling. This is what provides the best performance on modern PC,
workstations, servers, and about any architecture where the level-1
cache for instruction (in the CPU) has size 32 kB or more.

However, sphlib also includes variants optimized for architectures with
small level-1 cache. To use them, arrange for the SPH_SMALL_FOOTPRINT
macro to be defined (to a non-zero integer value) during compilation,
e.g. through the arguments passed to the C compiler by the build script.
These variants have been tested on a MIPS-compatible processor with 8 kB
of level-1 cache, and they offer much better performance than the normal
code on those architectures. In some specific situations, you might want
to use these "small footprint" variants on big computers as well; test
and measure speed if unsure.


Unix systems
------------

If you happen to have a Unix-like system (e.g. Linux), you may simply
type:

	c/build.sh

which should:

 - compile the library
 - compile the tools
 - compile the unit tests
 - run the unit tests

The library and tools may be installed with:

	c/build.sh -i

which will install sphspeed and sphsum in /usr/local/bin, libsph.a in
/usr/local/lib, and the header files (all the sph_*.h files) in
/usr/local/include.

The installation directories and the compilation options can be altered
at will with appropriate options. Use:

	c/build.sh --help

to access the list of options.

"build.sh" is only for Unix-like systems such as Linux. This script has
not been thoroughly tested, is very crude, and has only limited
autodetection capabilities. If you are after getting the maximum hashing
speed, or if you want to use the library from a shared object, you will
probably have to specify other compile options. Use "--with-cflags" to
change the compilation options. For instance:

	c/build.sh --with-cflags="-W -Wall -O1 -fPIC -mtune=athlon64"

This selects options for position-independant code, i.e. suitable for a
shared object, and tuned for maximum performance on Ahtlon64-type
processors. It has been noticed that "-O1" provides better performance
than "-O2" with recent versions (4.4.3) of GCC, although "-O2" yields
better code for some of the hash functions.

A realistic example of cross-compilation for a MIPS-compatible
architecture would look like this:

	c/build.sh --with-cc=mipsel-linux-uclibc-gcc \
		--with-clags="-W -Wall -O1 -DSPH_SMALL_FOOTPRINT"

which selects an alternate C compiler, and also defines the
SPH_SMALL_FOOTPRINT macro to use the "small footprint" variants which
offer much better performance on architectures with low L1 cache.

"build.sh" is not mandatory; you may edit and use the Makefile.unix file
directly.

The "sphsum" binary can be used to hash files in a way similar to what
the "md5sum" Linux tool does. The first argument of sphsum must be the
name of a hash function; matching is not case sensitive. Recognized
names are:

  name          function
  ----------------------------------------------------------
  haval128_3    HAVAL, 128-bit output, 3 passes
  haval128_4    HAVAL, 128-bit output, 4 passes
  haval128_5    HAVAL, 128-bit output, 5 passes
  haval160_3    HAVAL, 160-bit output, 3 passes
  haval160_4    HAVAL, 160-bit output, 4 passes
  haval160_5    HAVAL, 160-bit output, 5 passes
  haval192_3    HAVAL, 192-bit output, 3 passes
  haval192_4    HAVAL, 192-bit output, 4 passes
  haval192_5    HAVAL, 192-bit output, 5 passes
  haval224_3    HAVAL, 224-bit output, 3 passes
  haval224_4    HAVAL, 224-bit output, 4 passes
  haval224_5    HAVAL, 224-bit output, 5 passes
  haval256_3    HAVAL, 256-bit output, 3 passes
  haval256_4    HAVAL, 256-bit output, 4 passes
  haval256_5    HAVAL, 256-bit output, 5 passes
  md2           MD2
  md4           MD4
  md5           MD5
  panama        Panama
  radiogatun32  RadioGatun[32]
  radiogatun64  RadioGatun[64]
  ripemd        RIPEMD (original function)
  ripemd128     RIPEMD-128 (revised function, 128-bit output)
  ripemd160     RIPEMD-160 (revised function, 160-bit output)
  rmd           RIPEMD (original function)
  rmd128        RIPEMD-128 (revised function, 128-bit output)
  rmd160        RIPEMD-160 (revised function, 160-bit output)
  sha0          SHA-0 (original SHA, withdrawn)
  sha1          SHA-1
  sha224        SHA-224
  sha256        SHA-256
  sha384        SHA-384
  sha512        SHA-512
  tiger         Tiger
  tiger2        Tiger2 (Tiger with a modified padding)
  whirlpool     Whirlpool (2003, current version)
  whirlpool0    Whirlpool-0 (2000)
  whirlpool1    Whirlpool-1 (2001)

For the implemented "SHA-3 candidates", there are four names for each
function, depending on the hash output size in bits. That size is
appended to the base name; e.g. "shabal384" means "the Shabal hash
function with a 384-bit output". Here are the base names for the
implemented SHA-3 candidates:

  blake         BLAKE
  bmw           Blue Midnight Wish
  cubehash      CubeHash
  echo          ECHO
  fugue         Fugue
  groestl       Groestl
  hamsi         Hamsi
  jh            JH
  keccak        Keccak
  luffa         Luffa
  shabal        Shabal
  shavite       SHAvite-3
  simd          SIMD
  skein         Skein


Alternatively, the "sphsum" executable file can be named after one of
these functions, in which case the function name needs not be specified.
Hence, if you install "sphsum" and create a link (either symbolic or
not) to "sphsum" named "md5sum", then you may use that link as a drop-in
replacement for the standard Linux tool "md5sum". This function name
recognition process ignores the ".exe", "sum" and "sum.exe" suffixes.


Windows
-------

On Windows systems, you may use the Makefile.win32 file. This is meant
for Visual C 2005 or later (command-line compiler). Open a "Visual C
console" from the start menu (this is a standard text console with the
environment set up for using cl.exe). Type:

	nmake /f makefile.win32

which should compile the code, the unit tests and the standalone
binaries. There is no library per se, only a collection of object files.

Other C compilers exist for Windows (e.g. MinGW or the cygwin system).
They should be able to process sphlib code with no worry; but we provide
no build script or makefile for them.


Other systems
-------------

If you wish to include sphlib C code in your own projects, then you must
copy the header and source files which implement the functions you want
to use. Here are the dependency rules:

- sph_types.h: always needed; all other files include it.

- Each function or function family has its own header, e.g. sph_sha2.h
for the SHA-2 family (SHA-224, SHA-256, SHA-384 and SHA-512). The
sph_sha3.h header includes the sph_sha2.h file (for SHA-2) and all the
header files for the implemented SHA-3 candidates.

- Each function or function family is implemented in one or a few C
files. You need to include C files only for the functions that you
actually use. Most of the file names are self-explanatory, but please
note the following:
  * Some functions indirectly use the md_helper.c file. These are MD4,
    MD5, all RIPEMD*, all SHA-*, Tiger, Tiger2 and all Whirpool*. The
    md_helper.c file MUST NOT be compiled by itself: it is a helper
    file which is _included_ by, for instance, md5.c. Just drop it in
    the same directory.
  * Similarly:
    - HAVAL (haval.c) includes haval_helper.c
    - ECHO (echo.c) and SHAvite-3 (shavite.c) include aes_helper.c
    - Hamsi (hamsi.c) includes hamsi_helper.c
  * sha2.c is for SHA-224 and SHA-256. sha2big.c is for SHA-384 and SHA-512.
  * speed.c and hsum.c are the main files for, respectively, the sphspeed
    and sphsum command-line utilities.
  * utest.c, utest.h and the test_*.c files are used for the unit tests,
    which verify that the implementations operate properly. They need
    not be included in your own project.
  * sha3nist.c and sha3nist.h are a wrapper used to transform SHA-2 or
    any of the SHA-3 candidates into functions with the API defined by
    NIST for the SHA-3 competition. You have to modify the sha3nist.h
    file to select the actual candidate (only one at a time, this is
    an artefact of how the NIST API is defined).

Most of the "magic" happens in sph_types.h. This is where one may find
such things as inline assembly for faster little/big-endian word access.


Tuning
------

The C code tries to detect (through predefined macros) the kind of
architecture on which it is supposed to run. This information can be
used to speed up some operations, in particular decoding and encoding of
32-bit and 64-bit words. When the current architecture cannot be
detected, sphlib uses some generic code which always works but is
somewhat slower. The speed gain obtained through architecture specific
code can reach +30% on the fastest functions (less on the slower
functions).

Most of the C macros which govern that behaviour are boolean flags. To
explicitly enable the feature, define the macro to a non-zero integer
value, e.g. with '-DSPH_LITTLE_ENDIAN=1' (for most C compilers, defining
the macro without an explicit content, with '-DSPH_LITTLE_ENDIAN', has
the same effect). To explicitly disable the feature, define the macro to
a zero integer value: '-DSPH_LITTLE_ENDIAN=0'. If a feature is not
explicitly enabled or disabled, then sph_types.h will try to autodetect
its status.

The following flags are defined:

SPH_LITTLE_ENDIAN
   When non-zero, sphlib assumes that its 32-bit-or-more integer type
   (respectively 64-bit-or-more integer type) has size _exactly_ 32 bits
   (respectively 64 bits), and is encoded in RAM with the little-endian
   convention.

SPH_BIG_ENDIAN
   Similar to SPH_LITTLE_ENDIAN, but with the big-endian convention.

SPH_LITTLE_FAST
   When non-zero, little-endian decoding is assumed to be fast: some
   functions will thus omit caching decoded words in local variables.
   This is normally implied by SPH_LITTLE_ENDIAN.

SPH_BIG_FAST
   Similar to SPH_LITTLE_FAST, but with big-endian convention.

SPH_UNALIGNED
   The processor tolerates unaligned 32-bit or 64-bit accesses with
   only a slight timing penalty.

SPH_SPARCV9_GCC_32
   The target architecture is an UltraSPARC-compatible processor, used
   in 32-bit mode, and the compiler is GCC.

SPH_SPARCV9_GCC_64
   The target architecture is an UltraSPARC-compatible processor, used
   in 64-bit mode, and the compiler is GCC.

SPH_SPARCV9_GCC
   The target architecture is an UltraSPARC-compatible processor, used
   in 32-bit or 64-bit mode, and the compiler is GCC.

SPH_I386_GCC
   The target architecture is an x86-compatible processor, used in
   32-bit mode, and the compiler is GCC.

SPH_I386_MSVC
   The target architecture is an x86-compatible processor, used in
   32-bit mode, and the compiler is Microsoft Visual C.

SPH_AMD64_GCC
   The target architecture is an x86-compatible processor, used in
   64-bit mode, and the compiler is GCC.

SPH_AMD64_MSVC
   The target architecture is an x86-compatible processor, used in
   64-bit mode, and the compiler is Microsoft Visual C.

SPH_SMALL_FOOTPRINT
   When non-zero, "small footprint" variants are compiled. The code
   is less unrolled, resulting in more compact binary code, at the
   expense of extra indirections. This macro is never auto-detected.
   You should use it when the target architecture level-1 cache for
   instructions is strictly smaller than 32 kB.

   There are also function-specific "small footprint" flags, which can
   be used to enable or disable "small footprint" variants for each
   function independently (the function-specific flag takes precedence
   over SPH_SMALL_FOOTPRINT when both are defined). These flags are:

     SPH_SMALL_FOOTPRINT_BLAKE      (for BLAKE)
     SPH_SMALL_FOOTPRINT_BMW        (for Blue Midnight Wish)
     SPH_SMALL_FOOTPRINT_CUBEHASH   (for CubeHash)
     SPH_SMALL_FOOTPRINT_ECHO       (for ECHO)
     SPH_SMALL_FOOTPRINT_GROESTL    (for Groestl)
     SPH_SMALL_FOOTPRINT_HAMSI      (for Hamsi)
     SPH_SMALL_FOOTPRINT_HAVAL      (for HAVAL)
     SPH_SMALL_FOOTPRINT_JH         (for JH)
     SPH_SMALL_FOOTPRINT_KECCAK     (for Keccak)
     SPH_SMALL_FOOTPRINT_SHA2       (for SHA-224, SHA-256, SHA-384 and SHA-512)
     SPH_SMALL_FOOTPRINT_SHAVITE    (for SHAvite-3)
     SPH_SMALL_FOOTPRINT_SIMD       (for SIMD)
     SPH_SMALL_FOOTPRINT_SKEIN      (for Skein)
     SPH_SMALL_FOOTPRINT_WHIRLPOOL  (for Whirlpool)

Another additional macro is SPH_UPTR. This is not a boolean flag; when
defined, it must evaluate to an unsigned integer type which has the same
size as a pointer. When casting a C pointer to SPH_UPTR and back, the
original pointer must be recovered, and it must be possible to determine
the pointer alignment by looking at the least significant bits of its
value when cast to a SPH_UPTR. SPH_UPTR cannot be defined unless either
SPH_LITTLE_ENDIAN or SPH_BIG_ENDIAN is also defined (explicitly or
auto-detected). If unsure, leave undefined; it has no influence over
performance of most of the implemented functions.

The "test_types" binary (built as part of the unit tests) prints out,
when executed, a synthetic report on what architecture characteristics
were actually used.


Installation (Java code)
========================

Java code is in the java/ directory. Hash function implementations are
located in the "fr.cryptohash" package; there is one specific class for
each hash function, a common interface called "Digest", and some
non-public helper classes.

The "fr.cryptohash.test" package contains two standalone applications
(classes with a main() method). The "TestDigest" application runs the
unit tests. The "Speed" application runs speed tests, with an output
similar to that provided by the "sphspeed" tool from the C code. Note
that these tests cannot access the CPU usage by the test process;
instead, they use the "wall clock" time. Hence, speed tests should be
performed on an otherwise idle machine.

The Java code should be compatible both with older virtual machines
(e.g. Java 1.1) and with J2ME platforms.

#######################################################################
                          IMPORTANT WARNING

It appears that some versions of the Java virtual machine from Sun (now
Oracle) have a bug, in which the code for ECHO is not properly handled
at runtime. To check whether your VM has the bug, run the
fr.cryptohash.test.TestDigest application, preferably with the '-server'
command-line flag (this is the default on x86_64 but not on i386).

Affected versions include at least 1.6.0_16. However, 1.6.0_19 and
1.6.0_20 seem fine. If unsure then update your JVM to the latest
published version.

Some OpenJDK versions are also affected, including 6b16-1.6.1.
#######################################################################


The NIST SHA-3 API
==================

Internally, sphlib tended to use the name "sha3" for the 64-bit
functions of the SHA-2 family, namely SHA-384 and SHA-512. This is
historical. Here, we talk about the SHA-3 contest which was launched in
2008 by NIST, to define the next family of hash functions which will
become an american standard, as substitutes for the existing SHA-224,
SHA-256, SHA-384 and SHA-512 functions. Many candidate functions have
been submitted so far. The competition has reached its second round, in
which 14 candidates have been kept. sphlib currently implements those
14 candidates.

For the purposes of this competition, the NIST published a C API. All
candidates were asked to provide reference and optimized implementations
fitting in that API.

The basic sphlib API is distinct from the NIST API. However, a
compatibility layer has been added to sphlib-1.1. It consists in the
sha3nist.c and sha3nist.h source files. With these files, you may use
some of the sphlib implementations through an API conforming to the NIST
specification. Namely, you may select either the SHA-2 family, or any of
the implemented SHA-3 candidates.

To use that layer, modify sha3nist.h to designate the hash functions
you wish to use. By default, the SHA-224/... functions are used. To
use Shabal instead, replace the following line:

   #define SPH_NIST   sha

with this:

   #define SPH_NIST   shabal

and add the sha3nist.c file to the list of C files to compile into your
application. Similarly, use "bmw" for Blue Midnight Wish, "jh" for JH,
and so on.


Future work
===========

Future versions of sphlib may feature:
- options for better conditional inclusion (e.g. not compiling RIPEMD if
you only want RIPEMD-160)
- optimized versions for footprint-constrained environments (which should
also help platforms with a small L1 cache)
- a better compilation and installation procedure for the library and
standalone tools
- man pages for the standalone tools
- a building process for sphlib as a shared library


Change log
==========

** new in sphlib-3.0
   - Updated BLAKE, Groestl, JH, Keccak and Skein to SHA-3 round 3 tweaks
   - Fixed suboptimal code in Keccak
   - Fixed a data-management bug in Hamsi

** new in sphlib-2.1
   - Added implementations of CubeHash, Groestl, Hamsi, Keccak and
     SHAvite-3 (C and Java)
   - Added Java implementations for RadioGatun
   - Optimized RadioGatun on small architectures and 32-bit x86
   - Made "size-generic" Java implementation of Shabal (supports all
     output sizes multiple of 32, from 32 to 512 bits)
   - Added macros for explicit architecture feature activation or
     deactivation
   - Renamed SHABAL -> Shabal, and WHIRLPOOL -> Whirlpool
   - Fixed some bugs on exotic architectures

** new in sphlib-2.0
   - Added implementations of BLAKE, Blue Midnight Wish, ECHO, Fugue,
     JH, Luffa, SIMD and Skein (C and Java)
   - Changed default optimization level to -O1 with GCC
   - Moved SHA-384 / SHA-512 headers to sph_sha2.h; sph_sha3.h now
     includes sph_sha2.h and the header files for all SHA-3 candidates
   - Renamed implementation file for SHA-384 / SHA-512 (now sha2big.c)
   - Added support for signed integer types of at least 32 or 64 bits
   - Improved MIPS support (endianness detection)
   - Fixed code with exotic architectures (oversized integers)

** new in sphlib-1.1
   - Fixed bug in Panama implementation (some special padding cases)
   - Added RadioGatun[32] and RadioGatun[64] (C)
   - Added SHABAL-192/224/256/384/512 (C and Java)
   - Added API for fractional bits on some functions (MD5, SHA-0, SHA-1,
     SHA-224/256/384/512 and SHABAL)
   - Added compatibility layer for the NIST SHA-3 competition API
