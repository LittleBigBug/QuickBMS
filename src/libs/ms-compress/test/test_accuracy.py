#!/usr/bin/env python
"""
test_accuracy.py format directory

This tests the accuracy of the various compressors against each other. You select a single format
(one of None, LZNT1, Xpress, and Xpress-Huffman) and a directory of files and this will read each
file, send it to each possible compressor and decompressor combination to make sure we get the right
data back out in all cases. This checks both one-shot and streaming (if the compressor/decompressor
supports it). No news is good news! Only errors and minimal status messages are reported.
"""

import sys
import os
import io
from time import clock

from compressors import *

compressors = {
    'none': NoCompression,
    'lznt1': LZNT1,
    'xpress': Xpress,
    'xpress-huffman': XpressHuffman,
    }

if len(sys.argv) != 3:
    print >> sys.stderr, 'Error: arguments'
    sys.exit()

compressors = compressors.get(sys.argv[1].lower(), None)
path = sys.argv[2]
if compressors is None or not os.path.isdir(path):
    print >> sys.stderr, 'Error: arguments'
    sys.exit()

print "The available compressors are: "+", ".join(compressors)

def decompress(fullpath, data, compressed, name1, compressor, name2):
    try:
        decomp = compressor.Decompress(compressed, len(data))
        if len(data) != len(decomp):
            print >> sys.stderr, 'Error: %s failed to %s decompress %s compressed data (length %d != %d)' % (fullpath, name2, name1, len(data), len(decomp))
        elif data != decomp:
            print >> sys.stderr, 'Error: %s failed to %s decompress %s compressed data (content mismatch)' % (fullpath, name2, name1)
        del decomp
    except Exception as ex:
        if len(ex.args) <= 0: raise
        print >> sys.stderr, 'Error: %s failed to %s decompress %s compressed data (%s)' % (fullpath, name2, name1, ex.args[0])

def decompress_stream(fullpath, data, compressed, name1, compressor, name2):
    try:
        decomp = io.BytesIO()
        compressor.DecompressStream(io.BytesIO(compressed), decomp, 100*1024+1, 100*1024+1)
        decomp = decomp.getvalue()
        if len(data) != len(decomp):
            print >> sys.stderr, 'Error: %s failed to %s stream-decompress %s compressed data (length %d != %d)' % (fullpath, name2, name1, len(data), len(decomp))
        elif data != decomp:
            print >> sys.stderr, 'Error: %s failed to %s stream-decompress %s compressed data (content mismatch)' % (fullpath, name2, name1)
    except Exception as ex:
        if len(ex.args) <= 0: raise
        print >> sys.stderr, 'Error: %s failed to %s stream-decompress %s compressed data (%s)' % (fullpath, name2, name1, ex.args[0])

start_time = clock()
for root, dirs, files in os.walk(path):
    print '%8.2f Folder: %s' % (clock() - start_time, root)
    sys.stderr.flush()
    for name in files:
        fullpath = os.path.join(root, name)
        try:
            with io.open(fullpath, 'rb') as f: data = f.read()
        except: continue
        if len(data) == 0: continue

        for name1, compressor1 in compressors.iteritems():
            try:
                compressed = compressor1.Compress(data)
                for name2, compressor2 in compressors.iteritems():
                    decompress(fullpath, data, compressed, name1, compressor2, name2)
##                    if isinstance(compressor2, StreamableCompressor):
##                        decompress_stream(fullpath, data, compressed, name1, compressor2, name2)
                del compressed
            except Exception as ex:
                if len(ex.args) <= 0: raise
                print >> sys.stderr, 'Error: %s failed to %s compress (%s)' % (fullpath, name1, ex.args[0])
##            if isinstance(compressor1, StreamableCompressor):
##                try:
##                    compressed = io.BytesIO()
##                    compressor1.CompressStream(io.BytesIO(data), compressed, 100*1024+1, 100*1024+1)
##                    compressed = compressed.getvalue()
##                    for name2, compressor2 in compressors.iteritems():
##                        decompress(fullpath, data, compressed, name1, compressor2, name2)
##                        if isinstance(compressor2, StreamableCompressor):
##                            decompress_stream(fullpath, data, compressed, name1, compressor2, name2)
##                except Exception as ex:
##                    if len(ex.args) <= 0: raise
##                    print >> sys.stderr, 'Error: %s failed to %s stream-compress (%s)' % (fullpath, name1, ex.args[0])
        del data
print '%8.2f Done' % (clock() - start_time)
