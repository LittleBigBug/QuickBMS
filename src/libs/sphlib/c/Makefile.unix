# $Id: Makefile.unix 223 2010-06-09 13:22:59Z tp $
#
# ==========================(LICENSE BEGIN)============================
# 
# Copyright (c) 2007-2010  Projet RNRT SAPHIR
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# ===========================(LICENSE END)=============================
#
# @author   Thomas Pornin <thomas.pornin@cryptolog.com>
#
# TODO: update the "install" procedure: raw "cp" is quite primitive.
#

#
# Installation directories:
#   PREFIX       base directory, used only to make the other directories
#   INSTALLLIB   installation directory for the compiled library (static)
#   INSTALLINC   installation directory for the include files
#

PREFIX = /usr/local
INSTALLBIN = $(PREFIX)/bin
INSTALLLIB = $(PREFIX)/lib
INSTALLINC = $(PREFIX)/include

#
# Compiler and flags.
#

# Default options: we assume gcc.
CC = gcc
CFLAGS = -W -Wall -O1 -fomit-frame-pointer

# Athlon XP with gcc 4.1.2
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -mtune=athlon-xp -march=athlon-xp

# Athlon AMD64 with gcc 4.1.2
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -mtune=athlon64 -march=athlon64

# Pentium III-based Celeron with gcc 4.0
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -mtune=pentium3 -march=pentium3

# UltraSPARC IIi, 32-bit mode, gcc 3.4.3
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -mcpu=ultrasparc -DSPH_SPARCV9_GCC

# UltraSPARC IIi, 64-bit mode, gcc 3.4.3
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -m64 -mcpu=ultrasparc

# XScale-PXA270 (ARMv5TE) processor with gcc 4.1.2
#CC = gcc
#CFLAGS = -W -Wall -O1 -fomit-frame-pointer -mtune=xscale -march=armv5te

#
# Linker: used to build the static library.
#
AR = ar -rcs

# ==========================================================================

OBJ = blake.o bmw.o cubehash.o echo.o fugue.o groestl.o hamsi.o haval.o jh.o keccak.o luffa.o md2.o md4.o md5.o panama.o radiogatun.o ripemd.o sha0.o sha1.o sha2.o sha2big.o shabal.o shavite.o simd.o skein.o tiger.o whirlpool.o
LIB = libsph.a
TESTTYPES = test_types
TESTBLAKE = test_blake
TESTBMW = test_bmw
TESTCUBEHASH = test_cubehash
TESTECHO = test_echo
TESTFUGUE = test_fugue
TESTGROESTL = test_groestl
TESTHAMSI = test_hamsi
TESTHAVAL = test_haval
TESTJH = test_jh
TESTKECCAK = test_keccak
TESTLUFFA = test_luffa
TESTMD2 = test_md2
TESTMD4 = test_md4
TESTMD5 = test_md5
TESTPANAMA = test_panama
TESTRADIOGATUN = test_radiogatun
TESTRIPEMD = test_ripemd
TESTSHA0 = test_sha0
TESTSHA1 = test_sha1
TESTSHA2 = test_sha2
TESTSHA3 = test_sha2big
TESTSHABAL = test_shabal
TESTSHAVITE = test_shavite
TESTSIMD = test_simd
TESTSKEIN = test_skein
TESTTIGER = test_tiger
TESTWHIRLPOOL = test_whirlpool
TESTS = $(TESTTYPES) $(TESTBLAKE) $(TESTBMW) $(TESTCUBEHASH) $(TESTECHO) $(TESTFUGUE) $(TESTGROESTL) $(TESTHAMSI) $(TESTHAVAL) $(TESTJH) $(TESTKECCAK) $(TESTLUFFA) $(TESTMD2) $(TESTMD4) $(TESTMD5) $(TESTPANAMA) $(TESTRADIOGATUN) $(TESTRIPEMD) $(TESTSHA0) $(TESTSHA1) $(TESTSHA2) $(TESTSHA3) $(TESTSHABAL) $(TESTSHAVITE) $(TESTSIMD) $(TESTSKEIN) $(TESTTIGER) $(TESTWHIRLPOOL)
TESTTYPESOBJ = test_types.o utest.o
TESTBLAKEOBJ = test_blake.o blake.o utest.o
TESTBMWOBJ = test_bmw.o bmw.o utest.o
TESTCUBEHASHOBJ = test_cubehash.o cubehash.o utest.o
TESTECHOOBJ = test_echo.o echo.o utest.o
TESTFUGUEOBJ = test_fugue.o fugue.o utest.o
TESTGROESTLOBJ = test_groestl.o groestl.o utest.o
TESTHAMSIOBJ = test_hamsi.o hamsi.o utest.o
TESTHAVALOBJ = test_haval.o haval.o utest.o
TESTJHOBJ = test_jh.o jh.o utest.o
TESTKECCAKOBJ = test_keccak.o keccak.o utest.o
TESTLUFFAOBJ = test_luffa.o luffa.o utest.o
TESTMD2OBJ = test_md2.o md2.o utest.o
TESTMD4OBJ = test_md4.o md4.o utest.o
TESTMD5OBJ = test_md5.o md5.o utest.o
TESTPANAMAOBJ = test_panama.o panama.o utest.o
TESTRADIOGATUNOBJ = test_radiogatun.o radiogatun.o utest.o
TESTRIPEMDOBJ = test_ripemd.o ripemd.o utest.o
TESTSHA0OBJ = test_sha0.o sha0.o utest.o
TESTSHA1OBJ = test_sha1.o sha1.o utest.o
TESTSHA2OBJ = test_sha2.o sha2.o utest.o
TESTSHA3OBJ = test_sha2big.o sha2big.o utest.o
TESTSHABALOBJ = test_shabal.o shabal.o utest.o
TESTSHAVITEOBJ = test_shavite.o shavite.o utest.o
TESTSIMDOBJ = test_simd.o simd.o utest.o
TESTSKEINOBJ = test_skein.o skein.o utest.o
TESTTIGEROBJ = test_tiger.o tiger.o utest.o
TESTWHIRLPOOLOBJ = test_whirlpool.o whirlpool.o utest.o
SPEED = sphspeed
SPEEDOBJ = speed.o
HSUM = sphsum
HSUMOBJ = hsum.o
HEADERS = sph_blake.h sph_bmw.h sph_cubehash.h sph_echo.h sph_fugue.h sph_groestl.h sph_hamsi.h sph_haval.h sph_jh.h sph_keccak.h sph_luffa.h sph_md2.h sph_md4.h sph_md5.h sph_panama.h sph_radiogatun.h sph_ripemd.h sph_sha0.h sph_sha1.h sph_sha2.h sph_sha3.h sph_shabal.h sph_shavite.h sph_simd.h sph_skein.h sph_tiger.h sph_types.h sph_whirlpool.h

.POSIX:

all: compile tests

compile: $(LIB) $(SPEED) $(HSUM)

build-tests: $(TESTS)

run-tests: build-tests
	./$(TESTTYPES)
	./$(TESTBLAKE)
	./$(TESTBMW)
	./$(TESTCUBEHASH)
	./$(TESTECHO)
	./$(TESTFUGUE)
	./$(TESTGROESTL)
	./$(TESTHAMSI)
	./$(TESTHAVAL)
	./$(TESTJH)
	./$(TESTKECCAK)
	./$(TESTLUFFA)
	./$(TESTMD2)
	./$(TESTMD4)
	./$(TESTMD5)
	./$(TESTPANAMA)
	./$(TESTRADIOGATUN)
	./$(TESTRIPEMD)
	./$(TESTSHA0)
	./$(TESTSHA1)
	./$(TESTSHA2)
	./$(TESTSHA3)
	./$(TESTSHABAL)
	./$(TESTSHAVITE)
	./$(TESTSIMD)
	./$(TESTSKEIN)
	./$(TESTTIGER)
	./$(TESTWHIRLPOOL)

tests: build-tests run-tests

clean:
	-rm -f $(OBJ) $(LIB) $(TESTS) $(TESTTYPESOBJ) $(TESTBLAKEOBJ) $(TESTBMWOBJ) $(TESTCUBEHASHOBJ) $(TESTECHOOBJ) $(TESTFUGUEOBJ) $(TESTGROESTLOBJ) $(TESTHAMSIOBJ) $(TESTHAVALOBJ) $(TESTJHOBJ) $(TESTKECCAKOBJ) $(TESTLUFFAOBJ) $(TESTMD2OBJ) $(TESTMD4OBJ) $(TESTMD5OBJ) $(TESTPANAMAOBJ) $(TESTRADIOGATUNOBJ) $(TESTRIPEMDOBJ) $(TESTSHA0OBJ) $(TESTSHA1OBJ) $(TESTSHA2OBJ) $(TESTSHA3OBJ) $(TESTSHABALOBJ) $(TESTSHAVITEOBJ) $(TESTSIMDOBJ) $(TESTSKEINOBJ) $(TESTTIGEROBJ) $(TESTWHIRLPOOLOBJ) $(SPEED) $(SPEEDOBJ) $(HSUM) $(HSUMOBJ)

$(TESTTYPES): $(TESTTYPESOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTTYPESOBJ)

$(TESTBLAKE): $(TESTBLAKEOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTBLAKEOBJ)

$(TESTBMW): $(TESTBMWOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTBMWOBJ)

$(TESTCUBEHASH): $(TESTCUBEHASHOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTCUBEHASHOBJ)

$(TESTECHO): $(TESTECHOOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTECHOOBJ)

$(TESTFUGUE): $(TESTFUGUEOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTFUGUEOBJ)

$(TESTGROESTL): $(TESTGROESTLOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTGROESTLOBJ)

$(TESTHAMSI): $(TESTHAMSIOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTHAMSIOBJ)

$(TESTHAVAL): $(TESTHAVALOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTHAVALOBJ)

$(TESTJH): $(TESTJHOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTJHOBJ)

$(TESTKECCAK): $(TESTKECCAKOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTKECCAKOBJ)

$(TESTLUFFA): $(TESTLUFFAOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTLUFFAOBJ)

$(TESTMD2): $(TESTMD2OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTMD2OBJ)

$(TESTMD4): $(TESTMD4OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTMD4OBJ)

$(TESTMD5): $(TESTMD5OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTMD5OBJ)

$(TESTPANAMA): $(TESTPANAMAOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTPANAMAOBJ)

$(TESTRADIOGATUN): $(TESTRADIOGATUNOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTRADIOGATUNOBJ)

$(TESTRIPEMD): $(TESTRIPEMDOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTRIPEMDOBJ)

$(TESTSHA0): $(TESTSHA0OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHA0OBJ)

$(TESTSHA1): $(TESTSHA1OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHA1OBJ)

$(TESTSHA2): $(TESTSHA2OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHA2OBJ)

$(TESTSHA3): $(TESTSHA3OBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHA3OBJ)

$(TESTSHABAL): $(TESTSHABALOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHABALOBJ)

$(TESTSHAVITE): $(TESTSHAVITEOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSHAVITEOBJ)

$(TESTSIMD): $(TESTSIMDOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSIMDOBJ)

$(TESTSKEIN): $(TESTSKEINOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTSKEINOBJ)

$(TESTTIGER): $(TESTTIGEROBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTTIGEROBJ)

$(TESTWHIRLPOOL): $(TESTWHIRLPOOLOBJ)
	$(CC) $(CFLAGS) -o $@ $(TESTWHIRLPOOLOBJ)

$(LIB): $(OBJ)
	$(AR) $(LIB) $(OBJ)

$(SPEED): $(SPEEDOBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(SPEEDOBJ) $(LIB)

$(HSUM): $(HSUMOBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(HSUMOBJ) $(LIB)

install: $(LIB) $(HEADERS) $(SPEED) $(HSUM)
	mkdir -p "$(INSTALLBIN)"
	mkdir -p "$(INSTALLINC)"
	mkdir -p "$(INSTALLLIB)"
	cp "$(HSUM)" "$(INSTALLBIN)"
	cp "$(SPEED)" "$(INSTALLBIN)"
	cp $(HEADERS) "$(INSTALLINC)"
	cp "$(LIB)" "$(INSTALLLIB)"

SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

# =========================================================================
# Dependencies.
#

blake.o: sph_blake.h sph_types.h

bmw.o: sph_bmw.h sph_types.h

cubehash.o: sph_cubehash.h sph_types.h

echo.o: sph_echo.h sph_types.h aes_helper.c

fugue.o: sph_fugue.h sph_types.h

groestl.o: sph_groestl.h sph_types.h

hamsi.o: sph_hamsi.h sph_types.h hamsi_helper.c

haval.o: sph_haval.h sph_types.h haval_helper.c

hsum.o: sph_blake.h sph_bmw.h sph_cubehash.h sph_echo.h sph_fugue.h sph_groestl.h sph_hamsi.h sph_haval.h sph_jh.h sph_keccak.h sph_luffa.h sph_md2.h sph_md4.h sph_md5.h sph_panama.h sph_radiogatun.h sph_ripemd.h sph_sha0.h sph_sha1.h sph_sha2.h sph_shabal.h sph_shavite.h sph_simd.h sph_skein.h sph_tiger.h sph_types.h sph_whirlpool.h

jh.o: sph_jh.h sph_types.h

keccak.o: sph_keccak.h sph_types.h

luffa.o: sph_luffa.h sph_types.h

md2.o: sph_md2.h sph_types.h

md4.o: sph_md4.h sph_types.h md_helper.c

md5.o: sph_md5.h sph_types.h md_helper.c

panama.o: sph_panama.h sph_types.h

radiogatun.o: sph_radiogatun.h sph_types.h

ripemd.o: sph_ripemd.h sph_types.h md_helper.c

sha0.o: sph_sha0.h sph_types.h md_helper.c

sha1.o: sph_sha1.h sph_types.h md_helper.c

sha2.o: sph_sha2.h sph_types.h md_helper.c

sha2big.o: sph_sha2.h sph_types.h md_helper.c

shabal.o: sph_shabal.h sph_types.h

shavite.o: sph_shavite.h sph_types.h aes_helper.c

simd.o: sph_simd.h sph_types.h

skein.o: sph_skein.h sph_types.h

speed.o: sph_blake.h sph_bmw.h sph_cubehash.h sph_echo.h sph_fugue.h sph_groestl.h sph_hamsi.h sph_haval.h sph_jh.h sph_keccak.h sph_luffa.h sph_md2.h sph_md4.h sph_md5.h sph_panama.h sph_radiogatun.h sph_ripemd.h sph_sha0.h sph_sha1.h sph_sha2.h sph_shabal.h sph_shavite.h sph_simd.h sph_skein.h sph_tiger.h sph_types.h sph_whirlpool.h

test_blake.o: sph_blake.h sph_types.h test_digest_helper.c utest.h

test_bmw.o: sph_bmw.h sph_types.h test_digest_helper.c utest.h

test_cubehash.o: sph_cubehash.h sph_types.h test_digest_helper.c utest.h

test_echo.o: sph_echo.h sph_types.h test_digest_helper.c utest.h

test_fugue.o: sph_fugue.h sph_types.h test_digest_helper.c utest.h

test_groestl.o: sph_groestl.h sph_types.h test_digest_helper.c utest.h

test_hamsi.o: sph_hamsi.h sph_types.h test_digest_helper.c utest.h

test_haval.o: sph_haval.h sph_types.h test_digest_helper.c utest.h

test_keccak.o: sph_keccak.h sph_types.h test_digest_helper.c utest.h

test_luffa.o: sph_luffa.h sph_types.h test_digest_helper.c utest.h

test_jh.o: sph_jh.h sph_types.h test_digest_helper.c utest.h

test_md2.o: sph_md2.h sph_types.h test_digest_helper.c utest.h

test_md4.o: sph_md4.h sph_types.h test_digest_helper.c utest.h

test_md5.o: sph_md5.h sph_types.h test_digest_helper.c utest.h

test_panama.o: sph_panama.h sph_types.h test_digest_helper.c utest.h

test_radiogatun.o: sph_radiogatun.h sph_types.h test_digest_helper.c utest.h

test_shabal.o: sph_shabal.h sph_types.h test_digest_helper.c utest.h

test_shavite.o: sph_shavite.h sph_types.h test_digest_helper.c utest.h

test_ripemd.o: sph_ripemd.h sph_types.h test_digest_helper.c utest.h

test_sha0.o: sph_sha0.h sph_types.h test_digest_helper.c utest.h

test_sha1.o: sph_sha1.h sph_types.h test_digest_helper.c utest.h

test_sha2.o: sph_sha2.h sph_types.h test_digest_helper.c utest.h

test_sha2big.o: sph_sha2.h sph_types.h test_digest_helper.c utest.h

test_simd.o: sph_simd.h sph_types.h test_digest_helper.c utest.h

test_skein.o: sph_skein.h sph_types.h test_digest_helper.c utest.h

test_tiger.o: sph_tiger.h sph_types.h test_digest_helper.c utest.h

test_types.o: sph_types.h utest.h

test_whirlpool.o: sph_whirlpool.h sph_types.h test_digest_helper.c utest.h

tiger.o: sph_tiger.h sph_types.h md_helper.c

utest.o: utest.h

whirlpool.o: sph_whirlpool.h sph_types.h md_helper.c
