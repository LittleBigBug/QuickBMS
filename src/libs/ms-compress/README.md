ms-compress
===========
Open source implementations of Microsoft compression algorithms. The progress is listed below.
"RTL" refers to the native RtlCompressBuffer and RtlUncompressBuffer functions from Windows 8's
ntdll.dll. Comparisons are made against the max compression engine for RTL functions.

The quoted speeds are when running on Windows with a Core i7-3720QM 2.6 GHz CPU compiled using
MinGW-W64/GCC v4.9.2 using profile-guided optimizations. A total of 54 files were compressed,
totaling 269 MB. Each file was done 10 times. The files came from the following collections:
* [Calgary Corpus](http://corpus.canterbury.ac.nz/descriptions/#calgary)
* [Canterbury Corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry)
* [Canterbury Large Corpus](http://corpus.canterbury.ac.nz/descriptions/#large)
* [Maximum Compression's single file tests](http://www.maximumcompression.com)
* [Silesia Corpus](http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia)

For comparison, the library includes a "no compression" engine and it operates at ~2000 MB/s in the
same testing environment.

Other Microsoft compression algorithms not included (at least not yet):
 * [LZSS](http://www.cabextract.org.uk/libmspack/doc/szdd_kwaj_format.html) - used by COMPRESS.EXE and in HLP files, decompressor available as part of [libmspack](http://sourceforge.net/p/libmspack/code/HEAD/tree/libmspack/trunk/mspack/lzxd.c)
 * [LZSS+Huffman](http://www.cabextract.org.uk/libmspack/doc/szdd_kwaj_format.html) - used by COMPRESS.EXE, decompressor available as part of [libmspack](http://sourceforge.net/p/libmspack/code/HEAD/tree/libmspack/trunk/mspack/kwajd.c)
 * [MSZIP](https://msdn.microsoft.com/library/bb417343.aspx#microsoftmszipdatacompressionformat) - essentially "deflate" ([zlib](http://zlib.net)) algorithm used in CAB files
 * [Quantum](http://en.wikipedia.org/wiki/Quantum_compression) - used in some very rare CAB files, not mentioned in the MSDN at all, decompressor available as part of [libmspack](http://sourceforge.net/p/libmspack/code/HEAD/tree/libmspack/trunk/mspack/qtmd.c)
 * [MS-OXRTFCP](https://msdn.microsoft.com/library/cc463890%28v=exchg.80%29.aspx) - RTF Compression Algorithm used in the Exchange server - possibly similar to LZNT1
 * [Delta/LZXD](https://msdn.microsoft.com/library/bb417345.aspx) - delta version of LZX used in Windows Updates, decompressor available as part of [libmspack](http://sourceforge.net/p/libmspack/code/HEAD/tree/libmspack/trunk/mspack/oabd.c)
 * [RDC](https://msdn.microsoft.com/en-us/library/windows/desktop/aa372948.aspx) - similar to RSYNC
 * LZMS - recently introduced compression format used in WIM files, available in [wimlib](http://wimlib.sourceforge.net/)


LZNT1
-----
Used for NTFS file compression, Windows 2000 hibernation file, Active Directory, File Replication Service, Windows Vista SuperFetch Files, and Windows Vista/7 bootmgr.

MSDN article [MS-XCA]: https://msdn.microsoft.com/library/hh554002.aspx  
Which includes the [algorithm](https://msdn.microsoft.com/library/jj665697.aspx)
and an [example](https://msdn.microsoft.com/library/jj711990.aspx)

_Status: fully mature_ - no more significant changes likely

* Compression:       37 MB/s, 49% CR
  * Much faster than RTL (average ~45x)
  * Slightly better compression ratio (only when last chunk is better off uncompressed, otherwise identical, 374 bytes in 269 MB)
  * Uses much more memory than RTL (average ~2 MB total and theoretically up to 2 GB)
* Compression (SA):  15 MB/s, 49% CR
  * Alternate compression algorithm favoring a speed/memory balance
  * Much faster than RTL (average ~19x), but slower than the default (~0.42x)
  * Identical compression ratio to the default
  * Uses slightly less memory than RTL (~41 KB total), and much less than the default
* Decompression:    540 MB/s
  * Faster than RTL (average ~1.4x)
  * Gets faster with better compressed data (RTL is reversed)

Xpress
------
Used for Windows XP and newer hibernation file, Directory Replication Service (LDAP/RPC/AD), Exchange Server HTML compression, Windows Update Services, and Windows CE.

MSDN article [MS-XCA]: https://msdn.microsoft.com/library/hh554002.aspx  
Which includes [compression](https://msdn.microsoft.com/library/hh554053.aspx)
and [decompression](https://msdn.microsoft.com/library/hh536411.aspx)
pseudo-code along with an [example](https://msdn.microsoft.com/library/hh553843.aspx). 

_Status: working_ - decompression is fully mature but compression needs speed improvements and does not support streaming

* Compression:    90 MB/s, 40% CR
  * Slower than RTL (average ~0.81x)
  * Has a marginally better compression ratio than RTL
  * Uses about the same amount of memory as RTL
  * RTL bugs:
    * cannot compress inputs of 7 bytes or less
    * requires at least 24 extra bytes in the compression output buffer
* Decompression: 725 MB/s
  * Essentially the same speed as RTL

Xpress Huffman
--------------
Xpress algorithm with Huffman encoding, used in WIM files, Distributed File System Replication, Windows 7 SuperFetch, and Windows 8 bootmgr.

MSDN article [MS-XCA]: https://msdn.microsoft.com/library/hh554002.aspx  
Which includes [compression](https://msdn.microsoft.com/library/hh554076.aspx)
and [decompression](https://msdn.microsoft.com/library/hh536379.aspx)
details along with an [example](https://msdn.microsoft.com/library/hh536484.aspx). 

Additionally, a mostly complete pseudo-code decompression implementation is given at: https://msdn.microsoft.com/library/dd644740.aspx

_Status: working_ - needs major speed improvements, does not create optional chunk boundary spanning matches, and does not support streaming for compression or decompression

* Compression:    55 MB/s, 33% CR
  * Much slower than RTL (average ~0.67)
  * Has a marginally better compression ratio than RTL
  * Uses about the same amount of memory as RTL
  * RTL bug: requires at least 24 extra bytes in the compression buffer
* Decompression: 310 MB/s
  * Slower than RTL (average ~0.78x)
  * RTL bug: does not allow the output buffer to be anything besides the exact size of the uncompressed data

LZX
---
LZX compression used in WIM and CAB files with some minor differences between them.

NOTE: this code is currently removed from the repository due to stability issues.

Microsoft document about the CAB LZX format: http://msdn.microsoft.com/en-us/library/bb417343.aspx#lzxdatacompressionformat

Untested against native Windows functions (not part of RTL, need to test against CABINET.DLL (FCI/FDI) and WIMGAPI.DLL).

_Status: in development_ - works in some cases, but fails frequently

* Compression:
 * WIM: Mostly works
 * CAB: Potentially has issues with non-32k window size, does not seem to create length/offset pairs, memory issues abound
 * BOTH: Speed untested
 * BOTH: Does not support creation of aligned offset blocks but all other features implemented
* Decompression based on [7-zip](http://www.7-zip.org/) code
 * WIM: Mostly works
 * CAB: Should work for all valid compressed data <= 0x8000 in size (needs a refit like the compression functions to work on larger)
 * BOTH: Speed untested
