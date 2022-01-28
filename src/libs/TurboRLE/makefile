# powturbo  (c) Copyright 2015-2020
# Linux: "export CC=clang" "export CXX=clang". windows mingw: "set CC=gcc" "set CXX=g++" or uncomment the CC,CXX lines
CC ?= gcc
CXX ?= g++
#CC=clang-8
#CXX=clang++-8

#CC = gcc-8
#CXX = g++-8

#CC=powerpc64le-linux-gnu-gcc
#CXX=powerpc64le-linux-gnu-g++

DEBUG=-DNDEBUG -s
#DEBUG=-g
CFLAGS=$(DEBUG)
#------- OS/ARCH -------------------
ifneq (,$(filter Windows%,$(OS)))
  OS := Windows
  CC=gcc
  CXX=g++
  ARCH=x86_64
# CFLAGS+=-D__int64_t=int64_t
else
  OS := $(shell uname -s)
  ARCH := $(shell uname -m)

ifneq (,$(findstring aarch64,$(CC)))
  ARCH = aarch64
else ifneq (,$(findstring powerpc64le,$(CC)))
  ARCH = ppc64le
endif
endif

ifeq ($(ARCH),ppc64le)
  CFLAGS=-mcpu=power9 -mtune=power9
  MSSE=-D__SSSE3__
else ifeq ($(ARCH),aarch64)
  CFLAGS+=-march=armv8-a 
ifneq (,$(findstring clang, $(CC)))
  CFLAGS+=-march=armv8-a -falign-loops -fomit-frame-pointer
else
  CFLAGS+=-march=armv8-a 
endif
  MSSE=-march=armv8-a
else ifeq ($(ARCH),$(filter $(ARCH),x86_64 ppc64le))
  CFLAGS=-march=native
  MSSE=-mssse3
endif

ifeq (,$(findstring clang, $(CC)))
DEFS+=-falign-loops
endif
#$(info ARCH="$(ARCH)")

ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
LDFLAGS+=-lrt
endif
ifeq ($(STATIC),1)
LDFLAGS+=-static
endif


ifeq ($(AVX2),1)
MARCH+=-mbmi2 -mavx2
else
AVX2=0
endif

CFLAGS+=$(DDEBUG) -w -Wall -std=gnu99 -DUSE_THREADS  -fstrict-aliasing -Iext $(DEFS)
CXXFLAGS+=$(DDEBUG) -w -fpermissive -Wall -fno-rtti -Iext/FastPFor/headers $(DEFS)

all: trle

trle.o: $(TP)trle.c
	$(CC) -O2 $(CFLAGS) $(MARCH) -c $(TP)trle.c -o trle.o

trle: trlec.o trled.o trle.o
	$(CC) trle.o trlec.o trled.o $(LDFLAGS) -o trle
 
.c.o:
	$(CC) -O3 $(CFLAGS) $(MARCH) $< -c -o $@

clean:
	rm  *.o

cleanw:
	del .\*.o

