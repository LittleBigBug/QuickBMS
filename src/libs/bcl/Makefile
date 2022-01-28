##########################################################################
# Makefile for Basic Compression Library.
# Compiler: GNU C compiler
#-------------------------------------------------------------------------
# Copyright (c) 2003-2006 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would
#    be appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not
#    be misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
#    distribution.
#
# Marcus Geelnard
# marcus.geelnard at home.se
##########################################################################

##########################################################################
# Compiler settings
##########################################################################

CC      = gcc
CFLAGS  = -c -Wall -W -ansi -pedantic -O3
LFLAGS  = -s
CFLAGS_EXE = -c -Wall -w -O3 -s


##########################################################################
# Library building settings
##########################################################################

MKLIB    = ar
LIBFLAGS = -rcs
LIBNAME  = libbcl.a


##########################################################################
# Library object files
##########################################################################

LIBOBJS = rle.o shannonfano.o huffman.o rice.o lz.o


##########################################################################
# Default build option
##########################################################################

all: $(LIBNAME) bfc.exe bcltest.exe


##########################################################################
# Library building rule
##########################################################################

$(LIBNAME): $(LIBOBJS)
	$(MKLIB) $(LIBFLAGS) $(LIBNAME) $(LIBOBJS)


##########################################################################
# Executable building rule
##########################################################################

bfc.exe: bfc.o $(LIBNAME)
	$(CC) $(LFLAGS) bfc.o $(LIBNAME) -o bfc.exe

bcltest.exe: bcltest.o systimer.o $(LIBNAME)
	$(CC) $(LFLAGS) bcltest.o systimer.o $(LIBNAME) -o bcltest.exe



##########################################################################
# Library object compiler rules
##########################################################################

rle.o: rle.c rle.h
	$(CC) $(CFLAGS) rle.c

shannonfano.o: shannonfano.c shannonfano.h
	$(CC) $(CFLAGS) shannonfano.c

huffman.o: huffman.c huffman.h
	$(CC) $(CFLAGS) huffman.c

rice.o: rice.c rice.h
	$(CC) $(CFLAGS) rice.c

lz.o: lz.c lz.h
	$(CC) $(CFLAGS) lz.c


##########################################################################
# Executable compiler rules
##########################################################################

bfc.o: bfc.c rle.h shannonfano.h huffman.h rice.h lz.h
	$(CC) $(CFLAGS_EXE) bfc.c

bcltest.o: bcltest.c rle.h shannonfano.h huffman.h rice.h lz.h systimer.h
	$(CC) $(CFLAGS_EXE) bcltest.c

systimer.o: systimer.c
	$(CC) $(CFLAGS_EXE) systimer.c
