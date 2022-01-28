############################################################################
#
# Makefile for huffman encode/decode programs
#
############################################################################

CC = gcc
LD = gcc
CFLAGS = -O3 -Wall -Wextra -ansi -pedantic -c
LDFLAGS = -O3 -o

# libraries
LIBS = -L. -lhuffman -loptlist

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
	OS = Windows
endif

ifeq ($(OS),Windows)
	EXE = .exe
	DEL = del
else	#assume Linux/Unix
	EXE =
	DEL = rm
endif

all:		sample$(EXE)

sample$(EXE):	sample.o libhuffman.a liboptlist.a
		$(LD) $^ $(LIBS) $(LDFLAGS) $@

sample.o:	sample.c huffman.h optlist.h
		$(CC) $(CFLAGS) $<

libhuffman.a:	huffman.o canonical.o huflocal.o bitarray.o bitfile.o
		ar crv libhuffman.a huffman.o canonical.o huflocal.o\
		bitarray.o bitfile.o
		ranlib libhuffman.a

huffman.o:	huffman.c huflocal.h bitarray.h bitfile.h
		$(CC) $(CFLAGS) $<

canonical.o:	canonical.c huflocal.h bitarray.h bitfile.h
		$(CC) $(CFLAGS) $<

huflocal.o:	huflocal.c huflocal.h
		$(CC) $(CFLAGS) $<

bitarray.o:	bitarray.c bitarray.h
		$(CC) $(CFLAGS) $<

bitfile.o:	bitfile.c bitfile.h
		$(CC) $(CFLAGS) $<

liboptlist.a:	optlist.o
		ar crv liboptlist.a optlist.o
		ranlib liboptlist.a

optlist.o:	optlist.c optlist.h
		$(CC) $(CFLAGS) $<

clean:
		$(DEL) *.o
		$(DEL) *.a
		$(DEL) sample$(EXE)
