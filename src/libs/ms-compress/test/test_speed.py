#!/usr/bin/env python
"""
test_speed.py compressor decompressor format times directory

Runs compression speed tests. The compressor and decompressor must be one of rtl/rtl-max, rtl-std,
opensrc, dotnet/.net, copy, or copy-fast. The format must be one of none, lznt1, xpress, or
xpress-huffman and must be supported by both the compressor and decompressor. times is the number of
times to run each compression/decompression. The directory is the folder filled with files to use
for data.

This does not check accuracy of results at all, but instead times each compression and
decompression along with recording compression ratios. This does not support the streaming
compressors/decompressors.
"""
import sys
import os
import gc
from time import clock
import itertools

Rtl, OpenSrc, Net = None, None, None
from compressors import *

if len(sys.argv) != 6:
    print >> sys.stderr, 'Error: arguments'
    sys.exit()

compressors = {
    'rtl': Rtl, 'rtl-max': Rtl,
    'rtl-std': Rtl.Standard if Rtl is not None else None,
    'opensrc': OpenSrc,
    'dotnet': Net, '.net': Net,
    'copy': Copy, 'copy-fast': CopyFast,
    }
formats = {
    'none': 'NoCompression',
    'lznt1': 'LZNT1',
    'xpress': 'Xpress',
    'xpress-huffman': 'XpressHuffman',
    }

compressor = compressors.get(sys.argv[1].lower(), None)
decompressor = compressors.get(sys.argv[2].lower(), None)
format = formats.get(sys.argv[3].lower(), None)
path = sys.argv[5]
if (compressor is None or decompressor is None or format is None or
    not hasattr(compressor, format) or not hasattr(decompressor, format) or
    not os.path.isdir(path)):
    print >> sys.stderr, 'Error: arguments'
    sys.exit()
compressor = getattr(compressor, format)
decompressor = getattr(decompressor, format)
try:
    repeats = [int(t) for t in sys.argv[4].split(',')]
except:
    print >> sys.stderr, 'Error: arguments'
    sys.exit()
if len(repeats) == 1: repeats *= 2
elif len(repeats) > 2 or any(t <= 0 for t in repeats):
    print >> sys.stderr, 'Error: arguments'
    sys.exit()
comp_repeats, decomp_repeats = repeats

# Lock to a single core (reduces context switches, picks highest affinity bit)
# Only available if the affinity module has been installed
try:
    import affinity
    mask, i = affinity.get_process_affinity_mask(os.getpid()), -1
    while mask: mask >>= 1; i += 1
    affinity.set_process_affinity_mask(os.getpid(), 1 << (i-(i&1)))
except: pass

# Attempt to increase the priority to very high
try:
    import win32api, win32process
    win32process.SetPriorityClass(-1, win32process.HIGH_PRIORITY_CLASS)
    win32process.SetPriorityClass(-1, win32process.REALTIME_PRIORITY_CLASS)
except:
    try:
        while True: os.nice(-1)
    except: pass

nfiles = 0
full_size = 0
compressed_size = 0
start_time = clock()
comp_time = 0
decomp_time = 0
gc.disable() # don't want this to interfere with times
for root, dirs, files in os.walk(path):
    ##mb_size = full_size / (1024.0 * 1024.0)
    ##if nfiles == 0:
    ##    data = (0, 100, 0, 0)
    ##else:
    ##    data = (nfiles, compressed_size * 100.0 / full_size, mb_size / comp_time, mb_size / decomp_time)
    ##print '%8.2f Folder: %s' % (clock() - start_time, root)
    ##print '         Files: %6d   CR: %.2f%%   Speed: %.2f MB/s   %.2f MB/s' % data
    for name in files:
        fullpath = os.path.join(root, name)
        try:
            if os.path.getsize(fullpath) == 0: continue
            with open(fullpath, 'rb') as f:
                input = f.read()
        except: continue
        try:
            size = len(input)
            compressed_buf = bytearray(max(int(size * 1.5), size + 1024))
            decompressed_buf = bytearray(size)
            
            a = clock()
            for _ in xrange(comp_repeats):
                compressed = compressor.Compress(input, compressed_buf)
            b = clock()
            comp_time += b - a
            
            a = clock()
            for _ in xrange(decomp_repeats):
                decompressed = decompressor.Decompress(compressed, decompressed_buf)
            b = clock()
            decomp_time += b - a

            nfiles += 1
            full_size += size
            compressed_size += len(compressed)
            
            ##if len(input) != len(decompressed):
            ##    print >> sys.stderr, 'Error: %s failed to compress/decompress (length %d != %d)' % (fullpath,len(input),len(decompressed))
            ##elif input != decompressed:
            ##    print >> sys.stderr, 'Error: %s failed to compress/decompress (content mismatch)' % fullpath
        except Exception as ex:
            print >> sys.stderr, 'Error: %s failed to compress/decompress (%s)' % (fullpath,ex.args[0])
        gc.collect() # manual collection when not timing Compress or Decompress
#print >> sys.stderr, '%8.3f Done!' % (clock() - start_time)
mb_size = full_size / (1024.0 * 1024.0)
print >> sys.stderr, '    Files: %5d   CR:%7.3f%%' % (nfiles, 100 if full_size == 0 else (compressed_size * 100.0 / full_size))
print >> sys.stderr, '    Compressed to   %10d bytes in %7.3f secs - %7.3f MB/s' % (compressed_size*comp_repeats, comp_time, 0 if comp_time == 0 else mb_size*comp_repeats / comp_time)
print >> sys.stderr, '    Decompressed to %10d bytes in %7.3f secs - %7.3f MB/s' % (full_size*decomp_repeats, decomp_time, 0 if decomp_time == 0 else mb_size*decomp_repeats / decomp_time)
