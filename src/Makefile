EXE		= quickbms
# -m32 because QuickBMS has been tested only on 32bit systems and gives problems using 64bit native code
CFLAGS	+= -m32 -s -O2 -fstack-protector-all -fno-unit-at-a-time -fno-omit-frame-pointer -fno-pie -fPIC -w
# Add -DQUICKBMS64 to CDEFS for compiling quickbms_4gb_files
# For static builds on Linux (no MacOSX due to -lcrt0 error) add: -static
# -ldl is in CDEFS to avoid undefined references.
CDEFS	+= -DDISABLE_MCRYPT -DDISABLE_TOMCRYPT -ldl
# additional CDEFS for specific libraries like lzma and zstd
CDEFS	+= -DZSTD_DISABLE_ASM
# -static-libgcc unavailable on MacOS latest gcc
CLIBS	+= -static-libstdc++ -lstdc++ -lm -lpthread
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
SRC		= $(EXE).c

ifeq ($(shell uname -s), Darwin)
CDEFS	+= -DFORCE_SATUR_SUB_128
CFLAGS	+= -Dunix
#CC		= gcc-6
#CXX		= g++-6
# necessary for my build, remove in case you don't have openssl
USE_OPENSSL	= 1
else
CLIBS	+= -static-libgcc
ifeq ($(filter-out %86, $(shell uname -m)),)
EXTRA_TARGETS	= libs/amiga/amiga.s libs/powzix/*.cpp
CFLAGS	+= -msse2
endif
USE_OPENSSL	= 1
endif

# -liconv and -fPIC are necessary on Android
ifeq ($(shell uname -o), Android)
CLIBS	+= -liconv
endif

ifndef USE_OPENSSL
CDEFS	+= -DDISABLE_SSL
else
ifeq ($(shell uname -s), Darwin)
PREFIX_OPENSSL = `brew --prefix openssl`
CFLAGS	+= -I$(PREFIX_OPENSSL)/include
# openssl is 64bit only in brew and we need everything statically linked, so compile quickbms as 64bit
CLIBS	+= -m64 $(PREFIX_OPENSSL)/lib/libssl.a $(PREFIX_OPENSSL)/lib/libcrypto.a
else
CLIBS	+= -lssl -lcrypto
endif
endif

# MacOSX steps:
# - > xcode-select install
# - > /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
# - > brew install gcc
# - > brew install openssl

all:
	$(CC) $(SRC) $(CFLAGS) -o $(EXE) \
	\
	-D_7ZIP_ST -DE_INVALIDARG=-1 \
	\
	-Ilibs/zlib libs/zlib/*.c compression/*.c* encryption/*.c* libs/lzma/LzmaDec.c libs/lzma/Lzma2Dec.c libs/lzma/Bra86.c libs/lzma/LzFind.c libs/lzma/LzmaEnc.c libs/lzma/Lzma2Enc.c libs/mspack/*.c libs/ppmd_7zip/*.c libs/ppmd/*.cpp libs/aplib/src/depacks.c libs/aplib/lib/coff/aplib.lib -Ilibs/brieflz/include libs/brieflz/src/depacks.c libs/brieflz/src/brieflz.c compression/jcalg1_static.lib libs/zziplib/*.c libs/bcl/*.c libs/szip/*.c libs/lzhl/*.cpp libs/tdcb/*.c extra/mem2mem.c libs/libkirk/*.c libs/7z_advancecomp/*.cc libs/iris/*.cpp libs/old_cabextract/lzx.c libs/mrci/*.cpp libs/lz4/*.c libs/snappy/snappy.cc libs/snappy/snappy-c.cc libs/snappy/snappy-stubs-internal.cc libs/snappy/snappy-sinksource.cc libs/mmini/mmini_huffman.c libs/mmini/mmini_lzl.c libs/clzw/lzw-dec.c libs/clzw/lzw-enc.c libs/lzlib/lzlib.c libs/blosc/blosclz.c libs/blosc/fastcopy.c libs/gipfeli/*.cc libs/liblzg/src/lib/decode.c libs/liblzg/src/lib/encode.c libs/liblzg/src/lib/checksum.c libs/doboz/*.cpp libs/sphlib/c/*.c libs/shadowforce/*.cpp libs/zstd_aluigi/common/*.c libs/zstd_aluigi/compress/*.c libs/zstd_aluigi/decompress/*.c libs/zstd_aluigi/dictBuilder/*.c libs/zstd_aluigi/legacy/*.c -Ilibs/zstd_aluigi -Ilibs/zstd_aluigi/common -Ilibs/zstd_aluigi/legacy libs/azo/unAZO.cpp libs/azo/Decoder/MainCodeD.cpp libs/azo/Common/x86Filter.cpp libs/nintendo_ds/*.c libs/ctw/*.c libs/grzip/libgrzip.c libs/heatshrink/heatshrink_decoder.c libs/heatshrink/heatshrink_encoder.c libs/libzling/*.cpp -Ilibs/ecrypt/include -Ilibs/libcsc -D_7Z_TYPES_ libs/libcsc/csc_dec.cpp libs/libcsc/csc_default_alloc.cpp libs/libcsc/csc_filters.cpp libs/libcsc/csc_memio.cpp -DDENSITY_FORCE_INLINE=inline -Drestrict=__restrict__ libs/density/src/*.c libs/density/src/algorithms/*.c libs/density/src/algorithms/chameleon/core/*.c libs/density/src/algorithms/cheetah/core/*.c libs/density/src/algorithms/lion/core/*.c libs/density/src/algorithms/lion/forms/*.c libs/density/src/buffers/*.c libs/density/src/structure/*.c libs/spookyhash/*.c -Ilibs/brotli/include libs/brotli/dec/*.c libs/brotli/enc/*.c libs/brotli/common/*.c libs/libbsc/adler32/adler32.cpp libs/libbsc/bwt/bwt.cpp libs/libbsc/coder/coder.cpp libs/libbsc/coder/qlfc/qlfc.cpp libs/libbsc/coder/qlfc/qlfc_model.cpp libs/libbsc/filters/detectors.cpp libs/libbsc/filters/preprocessing.cpp libs/libbsc/libbsc/libbsc.cpp libs/libbsc/lzp/lzp.cpp libs/libbsc/platform/platform.cpp libs/libbsc/st/st.cpp libs/shoco/shoco.c libs/ms-compress/src/*.cpp libs/lzjody/lzjody.c libs/lzjody/byteplane_xfrm.c disasm/disasm.c disasm/cmdlist.c disasm/assembl/assembl.c -DMYDOWN_SSL -DMYDOWN_GLOBAL_COOKIE libs/mydownlib/mydownlib.c libs/TurboRLE/trlec.c libs/TurboRLE/trled.c libs/TurboRLE/ext/mrle.c libs/lhasa/lib/*_decoder.c libs/lhasa/lib/crc16.c libs/dipperstein/*.c libs/liblzf/lzf_d.c libs/liblzf/lzf_c_best.c libs/zopfli/*.c libs/lzham_codec/lzhamcomp/*.cpp libs/lzham_codec/lzhamdecomp/*.cpp libs/lzham_codec/lzhamlib/*.cpp -Ilibs/lzham_codec/include -Ilibs/lzham_codec/lzhamcomp -Ilibs/lzham_codec/lzhamdecomp -DLZHAM_ANSI_CPLUSPLUS libs/dmsdos/*.c libs/tornado/Tornado.cpp libs/tornado/Common.cpp libs/PKLib/*.c extra/mybits.c libs/lz5/lz5*.c libs/lizard/*.c libs/ppmz2/*.c* libs/libdivsufsort/*.c libs/xxhash/*.c extra/xalloc.c libs/lzfse/src/*.c libs/hsel/myhsel.cpp libs/hsel/HSEL.cpp libs/glza/GLZAmodel.c -Ilibs/lzo/include libs/lzo/src/*.c -Ilibs/ucl -Ilibs/ucl/include libs/ucl/src/n*.c libs/ucl/src/alloc.c libs/liblzs/lzs-compression.c libs/liblzs/lzs-decompression.c libs/bzip2/*.c libs/zlib/contrib/infback9/*.c libs/lzw-ab/lzw-lib.c libs/cryptohash-sha1/*.c -Ilibs/capstone/include libs/capstone/*.c -DCAPSTONE_X86_ATT_DISABLE -DCAPSTONE_USE_SYS_DYN_MEM -DCAPSTONE_HAS_ARM -DCAPSTONE_HAS_ARM64 -DCAPSTONE_HAS_MIPS -DCAPSTONE_HAS_X86 -DCAPSTONE_HAS_POWERPC -DCAPSTONE_HAS_SPARC -DCAPSTONE_HAS_SYSZ -DCAPSTONE_HAS_XCORE libs/capstone/arch/AArch64/*.c libs/capstone/arch/ARM/*.c libs/capstone/arch/Mips/*.c libs/capstone/arch/PowerPC/*.c libs/capstone/arch/Sparc/*.c libs/capstone/arch/SystemZ/*.c libs/capstone/arch/X86/*.c libs/capstone/arch/XCore/*.c libs/lua/src/*.c extra/quickrva.c libs/tiny-regex-c/re.c libs/exodecrunch/*.c \
	\
	$(EXTRA_TARGETS) \
	\
	$(CLIBS) $(CDEFS)

install:
	install -m 755 -d $(BINDIR)
	install -m 755 $(EXE) $(BINDIR)/$(EXE)

clean:
	rm -f quickbms

.PHONY:
	install clean
