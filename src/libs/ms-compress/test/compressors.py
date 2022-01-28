from ctypes import c_size_t, c_int, c_void_p, c_ubyte, c_char_p, c_char, c_bool
from ctypes import create_string_buffer, cast, POINTER, byref, cdll, sizeof, memmove, Structure
from abc import ABCMeta, abstractmethod
from warnings import warn
from itertools import product
import os
import sys

__all__ = ["CompressionFormat", "CompressionEngine", "Compressor", "StreamableCompressor",
           "NoCompression", "LZNT1", "Xpress", "XpressHuffman",
           "Copy", "CopyFast"]

# Determine the system we are running on
# On Windows we can enable the Rtl compressors
# On Windows 8 we can use the native NTDLL instead of an included one since it supports all types
# When running in 64-bit mode the DLL names are sometimes different
# OpenSrc is available if the compiled library can be found
# Copy, and CopyFast are always available
isWindows = os.name == 'nt'
if isWindows:
    from ctypes import windll
    winVers = sys.getwindowsversion()
    isWindows8orNewer = (winVers.major == 6 and winVers.minor >= 2) or winVers.major > 6
is64bit = (sizeof(c_void_p) == 8)

# Utility functions
def _get_buf(buf, default_size=10*1024*1024):
    if buf is None:
        ba = bytearray(default_size)
        #print memoryview(ba)
        return ba
    if isinstance(buf, bytearray):
        #print memoryview(buf)
        return buf
    if isinstance(buf, (int, long)):
        buf = bytearray(buf)
        #print memoryview(buf)
        return buf
    raise TypeError
def _ptr(buf, off=0):
    if isinstance(buf, bytearray): buf = (c_ubyte * (len(buf) - off)).from_buffer(buf, off)
    return cast(buf, c_void_p)


class CompressionFormat(object):
    """The supported compression formats. Not all compressors support all formats."""
    NoCompression = 0x0000
    _Default      = 0x0001
    LZNT1         = 0x0002
    Xpress        = 0x0003
    XpressHuffman = 0x0004

class CompressionEngine(object):
    """The supported compression engines, only used by Rtl."""
    Standard = 0x0000
    Maximum  = 0x0100
    Hiber    = 0x0200

NoCompression = {}
LZNT1 = {}
Xpress = {}
XpressHuffman = {}

class Compressor(object):
    """The base class for all compressors"""
    __metaclass__ = ABCMeta
    @abstractmethod
    def Compress(self, input, output_buf=None):
        """
        Compress and return the input data. If output_buf is provided, it is used to store the
        output data temporarily (otherwise a buffer is allocated). It can also be an integer for a
        suggested output size. If the buffer is not large enough, errors are likely.
        """
        pass
    @abstractmethod
    def Decompress(self, input, output_buf=None):
        """
        Decompress and return the input data. If output_buf is provided, it is used to store the
        output data temporarily (otherwise a buffer is allocated). It can also be an integer for a
        suggested output size. If the buffer is not large enough, errors are likely. Errors may also
        be raised if the data cannot be decompressed due to format.
        """
        pass

class StreamableCompressor(Compressor):
    """
    A compressor that supports streaming - basically takes chunks of data at a time instead of all
    at once.
    """
    __metaclass__ = ABCMeta
    @abstractmethod
    def CompressStream(self, input, output, input_buf=None, output_buf=None):
        """
        Compresses the data from the file-like object input to the file-like object output. The
        input object must support the method readinto() and the output object must support write().
        The input_buf and output_buf are used for temporary storage of data if provided (otherwise
        buffers are allocated). They can also be integers. The default is a 10MB buffer for each.
        """
        pass
    @abstractmethod
    def DecompressStream(self, input, output, input_buf=None, output_buf=None):
        """
        Decompresses the data from the file-like object input to the file-like object output. The
        input object must support the method readinto() and the output object must support write().
        The input_buf and output_buf are used for temporary storage of data if provided (otherwise
        buffers are allocated). They can also be integers. The default is a 10MB buffer for each.
        """
        pass

# Built-in Windows compressors (Rtl)
# Only available on Windows
# Always supports LZNT1, on Windows 8 or with a copy of the Windows 8 ntdll it also supports Xpress and XpressHuffman
# Does not support streaming
if isWindows:
    has_win8_ntdll = True
    if isWindows8orNewer:
        ntdll = windll.ntdll
    else:
        try:
            ntdll = windll['ntdll-8-64' if is64bit else 'ntdll-8-32']
        except WindowsError:
            ntdll = windll.ntdll
            has_win8_ntdll = False
            warn('Rtl compression library is not from Windows 8, only LZNT1 format will be available (Note: if you copy the ntdll.dll from Windows 8 into the working directory and call it ntdll-8-32.dll or ntdll-8-64.dll you can use it on a non-Windows 8 machine)')
    __all__.append("Rtl")
    from ctypes.wintypes import USHORT, ULONG, LONG
    NTSTATUS = LONG
    UCHAR = c_ubyte
    PUCHAR = POINTER(UCHAR)
    PULONG = POINTER(ULONG)
    PVOID = c_void_p

    def _errchk(status, func, args):
        if status != 0 and status != 0x00000117: # STATUS_BUFFER_ALL_ZEROS
            raise Exception('%s failed: %08X'%(func.__name__,(status&0xffffffff)))
        return status
    def _prep(f, args):
        f.restype, f.errcheck, f.argtypes = NTSTATUS, _errchk, args
        return f
    
    class Rtl(Compressor):
        DecompressBufferEx = _prep(ntdll.RtlDecompressBufferEx, [USHORT, PVOID, ULONG, PVOID, ULONG, PULONG, PVOID])
        CompressBuffer     = _prep(ntdll.RtlCompressBuffer, [USHORT, PVOID, ULONG, PVOID, ULONG, ULONG, PULONG, PVOID])
        GetCompressionWorkSpaceSize = _prep(ntdll.RtlGetCompressionWorkSpaceSize, [USHORT, PULONG, PULONG])

        def __init__(self, format, engine=CompressionEngine.Maximum):
            self.format = USHORT(format)
            self.format_engine = USHORT(format | engine)
            CompressBufferWorkSpaceSize = ULONG(0)
            CompressFragmentWorkSpaceSize = ULONG(0)
            Rtl.GetCompressionWorkSpaceSize(self.format_engine, byref(CompressBufferWorkSpaceSize), byref(CompressFragmentWorkSpaceSize))
            self.ws = create_string_buffer(CompressBufferWorkSpaceSize.value)

        def Compress(self, input, output_buf=None):
            len_input, comp_len = len(input), ULONG(0)
            output_buf = _get_buf(output_buf, max(int(len_input * 1.5), len_input + 1024))
            Rtl.CompressBuffer(self.format_engine, _ptr(input), len_input, _ptr(output_buf), len(output_buf), 4096, byref(comp_len), self.ws)
            return output_buf[:comp_len.value]

        def Decompress(self, input, output_buf=None):
            len_input, decomp_len = len(input), ULONG(0)
            output_buf = _get_buf(output_buf, len_input * 4)
            Rtl.DecompressBufferEx(self.format, _ptr(output_buf), len(output_buf), _ptr(input), len_input, byref(decomp_len), self.ws)
            return output_buf[:decomp_len.value]

    Rtl.LZNT1 = Rtl(CompressionFormat.LZNT1)
    Rtl.Standard = CompressionFormat() # dummy class used so we can add attributes
    Rtl.Standard.LZNT1 = Rtl(CompressionFormat.LZNT1, CompressionEngine.Standard)
    LZNT1['Rtl'] = Rtl.LZNT1
    LZNT1['Rtl-Standard'] = Rtl.Standard.LZNT1
    if has_win8_ntdll:
        Rtl.Xpress        = Rtl(CompressionFormat.Xpress)
        Rtl.XpressHuffman = Rtl(CompressionFormat.XpressHuffman)
        Rtl.Standard.Xpress        = Rtl(CompressionFormat.Xpress,        CompressionEngine.Standard)
        Rtl.Standard.XpressHuffman = Rtl(CompressionFormat.XpressHuffman, CompressionEngine.Standard)
        Xpress['Rtl']        = Rtl.Xpress
        XpressHuffman['Rtl'] = Rtl.XpressHuffman
        Xpress['Rtl-Standard']        = Rtl.Standard.Xpress
        XpressHuffman['Rtl-Standard'] = Rtl.Standard.XpressHuffman
else:
    warn('Rtl compression library is unavailable because this isn\'t Windows')


# Custom-made compressors in C/C++
# Supports all methods and streaming
# NOTE: currently only NoCompression and LZNT1 are supported properly, Xpress/XpressHuffman have not yet been updated to the current API
dll = None
mypath = os.path.dirname(os.path.abspath(__file__))
names_to_try = (('MSCompression','MSCompression64') if is64bit else ('MSCompression',)) if isWindows else  ('libMSCompression.so',)
for path,name in product((mypath, os.path.dirname(mypath)), names_to_try):
    try:
        dll = cdll.LoadLibrary(os.path.join(path,name))
        break
    except OSError as ex: continue
if dll is not None:
    __all__.append("OpenSrc")
    def _errchk(status, func, args):
        has_stream = False
        for a in args:
            if hasattr(a, '_obj') and isinstance(a._obj, OpenSrc.stream):
                s, has_stream = a._obj, True
                break
        if has_stream and s.warning:
            warn(s.warning)
            s.warning = ''
        if status < 0:
            if has_stream and s.error:
                raise Exception('%s failed: %d %s'%(func.__name__,status,s.error))
            else: raise Exception('%s failed: %d'%(func.__name__,status))
        return status
    def _prep(f, args):
        f.restype, f.errcheck, f.argtypes = c_int, _errchk, args
        return f
    class OpenSrc(StreamableCompressor):
        class stream(Structure):
            _fields_ = [("format", c_int), ("compressing", c_bool),
                        ("in_", c_void_p), ("in_avail",  c_size_t), ("in_total",  c_size_t),
                        ("out", c_void_p), ("out_avail", c_size_t), ("out_total", c_size_t),
                        ("error", c_char*256), ("warning", c_char*256),
                        ("state", c_void_p)]

        compress     = _prep(dll.ms_compress,   [c_int, c_void_p, c_size_t, c_void_p, POINTER(c_size_t)])
        decompress   = _prep(dll.ms_decompress, [c_int, c_void_p, c_size_t, c_void_p, POINTER(c_size_t)])
        deflate_init = _prep(dll.ms_deflate_init, [c_int, POINTER(stream)])
        deflate      = _prep(dll.ms_deflate,      [POINTER(stream), c_int])
        deflate_end  = _prep(dll.ms_deflate_end,  [POINTER(stream)])
        inflate_init = _prep(dll.ms_inflate_init, [c_int, POINTER(stream)])
        inflate      = _prep(dll.ms_inflate,      [POINTER(stream)])
        inflate_end  = _prep(dll.ms_inflate_end,  [POINTER(stream)])
        max_compressed_size = dll.ms_max_compressed_size
        max_compressed_size.restype = c_size_t
        max_compressed_size.argtypes = [c_int, c_size_t]

        NO_FLUSH = 0
        FLUSH = 2
        FINISH = 4

        def __init__(self, format):
            self.format = c_int(format)

        def MaxCompressedSize(self, input_len):
            val = OpenSrc.max_compressed_size(self.format, input_len)
            if val == (0xFFFFFFFFFFFFFFFF if is64bit else 0xFFFFFFFF): raise ValueError()
            return val

        def Compress(self, input, output_buf=None):
            len_input = len(input)
            output_buf = _get_buf(output_buf, max(int(len_input * 1.5), len_input + 1024))
            comp_len = c_size_t(len(output_buf))
            OpenSrc.compress(self.format, _ptr(input), c_size_t(len_input), _ptr(output_buf), byref(comp_len))
            return output_buf[:comp_len.value]

        def Decompress(self, input, output_buf=None):
            len_input = len(input)
            output_buf = _get_buf(output_buf, len_input * 4)
            decomp_len = c_size_t(len(output_buf))
            OpenSrc.decompress(self.format, _ptr(input), c_size_t(len_input), _ptr(output_buf), byref(decomp_len))
            return output_buf[:decomp_len.value]

        def CompressStream(self, input, output, input_buf=None, output_buf=None):
            input_buf, output_buf = _get_buf(input_buf), _get_buf(output_buf)
            input_ptr, output_ptr, output_len = _ptr(input_buf), _ptr(output_buf), len(output_buf)
            s = OpenSrc.stream()
            s_ptr = byref(s)
            OpenSrc.deflate_init(self.format, byref(s))
            try:
                s.in_avail = input.readinto(input_buf)
                while s.in_avail != 0:
                    s.in_ = input_ptr
                    while s.in_avail != 0:
                        s.out, s.out_avail = output_ptr, output_len
                        OpenSrc.deflate(s_ptr, OpenSrc.NO_FLUSH)
                        output.write(buffer(output_buf, 0, output_len-s.out_avail))
                    s.in_avail = input.readinto(input_buf)
                done = False
                while not done:
                    s.out, s.out_avail = output_ptr, output_len
                    done = OpenSrc.deflate(s_ptr, OpenSrc.FINISH) >= 1
                    output.write(buffer(output_buf, 0, output_len-s.out_avail))
            finally:
                OpenSrc.deflate_end(s_ptr)

        def DecompressStream(self, input, output, input_buf=None, output_buf=None):
            input_buf, output_buf = _get_buf(input_buf), _get_buf(output_buf)
            input_ptr, output_ptr, output_len = _ptr(input_buf), _ptr(output_buf), len(output_buf)
            s = OpenSrc.stream()
            s_ptr = byref(s)
            OpenSrc.inflate_init(self.format, byref(s))
            try:
                s.in_avail = input.readinto(input_buf)
                while s.in_avail != 0:
                    s.in_ = input_ptr
                    while s.in_avail != 0:
                        s.out, s.out_avail = output_ptr, output_len
                        OpenSrc.inflate(s_ptr)
                        output.write(buffer(output_buf, 0, output_len-s.out_avail))
                    s.in_avail = input.readinto(input_buf)
                done = False
                while not done:
                    s.out, s.out_avail = output_ptr, output_len
                    done = OpenSrc.inflate(s_ptr) >= 1
                    output.write(buffer(output_buf, 0, output_len-s.out_avail))
            finally:
                OpenSrc.inflate_end(s_ptr)

    OpenSrc.NoCompression = OpenSrc(CompressionFormat.NoCompression)
    OpenSrc.LZNT1         = OpenSrc(CompressionFormat.LZNT1)
    OpenSrc.Xpress        = OpenSrc(CompressionFormat.Xpress)
    OpenSrc.XpressHuffman = OpenSrc(CompressionFormat.XpressHuffman)

    NoCompression['OpenSrc'] = OpenSrc.NoCompression
    LZNT1['OpenSrc']         = OpenSrc.LZNT1
    Xpress['OpenSrc']        = OpenSrc.Xpress
    XpressHuffman['OpenSrc'] = OpenSrc.XpressHuffman
else:
    warn('OpenSrc compression library is unavailable because the library couldn\'t be found/loaded')


# Simple copy compressor (for no compression)
# Added for speed comparison to determine requirements for allocating memory and simply copying the data
# The MS compressor supports "no compression" natively but is possibly less efficient
# This uses the same logic for allocating memory as the MS compressor so it can be directly compared - see CopyFast for an optimized version
# Only provide output_bufs to sizes that are greater than or equal to len(input)
class Copy(StreamableCompressor):
    def __init__(self, format):
        if format != CompressionFormat.NoCompression:
            raise ArgumentError()
    def Compress(self, input, output_buf=None):
        len_input = len(input)
        output_buf = _get_buf(output_buf, max(int(len_input * 1.5), len_input + 1024))
        memmove(_ptr(output_buf), _ptr(input), len_input)
        return output_buf[:len_input]
    def Decompress(self, input, output_buf=None):
        len_input = len(input)
        output_buf = _get_buf(output_buf, len_input * 4)
        memmove(_ptr(output_buf), _ptr(input), len_input)
        return output_buf[:len_input]
    def CompressStream(self, input, output, input_buf=None, output_buf=None):
        input_buf, output_buf = _get_buf(input_buf), _get_buf(output_buf)
        input_ptr, output_ptr, output_len = _ptr(input_buf).value, _ptr(output_buf), len(output_buf)
        in_avail = input.readinto(input_buf)
        while in_avail != 0:
            in_ptr = input_ptr
            while in_avail > output_len:
                memmove(output_ptr, c_void_p(in_ptr), output_len)
                output.write(output_buf)
                in_ptr += output_len
                in_avail -= output_len
            if in_avail != 0:
                memmove(output_ptr, c_void_p(in_ptr), in_avail)
                output.write(buffer(output_buf, 0, in_avail))
            in_avail = input.readinto(input_buf)
    def DecompressStream(self, input, output, input_buf=None, output_buf=None):
        input_buf, output_buf = _get_buf(input_buf), _get_buf(output_buf)
        input_ptr, output_ptr, output_len = _ptr(input_buf).value, _ptr(output_buf), len(output_buf)
        in_avail = input.readinto(input_buf)
        while in_avail != 0:
            in_ptr = input_ptr
            while in_avail > output_len:
                memmove(output_ptr, c_void_p(in_ptr), output_len)
                output.write(output_buf)
                in_ptr += output_len
                in_avail -= output_len
            if in_avail != 0:
                memmove(output_ptr, c_void_p(in_ptr), in_avail)
                output.write(buffer(output_buf, 0, in_avail))
            in_avail = input.readinto(input_buf)
Copy.NoCompression = Copy(CompressionFormat.NoCompression)
NoCompression['Copy'] = Copy.NoCompression

# Simple copy compressor (for no compression)
# Like above but much smarter with allocating memory so should be faster
# If providing an output_buf, make sure it is exactly equal in length to the input
# TODO: create faster streaming functions
class CopyFast(Copy):
    def Compress(self, input, output_buf=None):
        len_input = len(input)
        if output_buf is None: output_buf = bytearray(len_input)
        elif isinstance(output_buf, (int, long)): output_buf = bytearray(output_buf)
        memmove(_ptr(output_buf), _ptr(input), len_input)
        return output_buf[:len_input]
    def Decompress(self, input, output_buf=None):
        len_input = len(input)
        if output_buf is None: output_buf = bytearray(len_input)
        elif isinstance(output_buf, (int, long)): output_buf = bytearray(output_buf)
        memmove(_ptr(output_buf), _ptr(input), len_input)
        return output_buf[:len_input]
CopyFast.NoCompression = CopyFast(CompressionFormat.NoCompression)
NoCompression['CopyFast'] = CopyFast.NoCompression
