#*-------------------------------------------------*#
#* GRZipII/libGRZip compressor            makefile *#
#* GRZipII makefile                                *#
#*-------------------------------------------------*#

# Modified compressor client program par Jean-Pierre Demailly
# (command line options matching more closely those of gzip/bzip2)

#ifndef DESTDIR
DESTDIR=/usr
#endif

GRZIPLIB = -L. -lgrzip

CC       = gcc

WARNING  = -Wall

# OPTFLAGS = -O6 -ffast-math -fomit-frame-pointer -fforce-addr
OPTFLAGS = -O2 -DENABLE_NLS -DLOCALEDIR=\"$(DESTDIR)/share/locale\"

# CPUARCH  = -march=pentium
CPUARCH =
ENDIANNESS = SMALLENDIAN

CFLAGS = $(CPUARCH) $(OPTFLAGS) $(WARNING) -D$(ENDIANNESS)
LDFLAGS = -lm
LDSTATICFLAGS = libgrzip.a -lm

static: grzip_static
	mv -f grzip_static grzip
	@echo ""
	@echo "Type 'make all' to generate shared library"
	@echo ""	
	@echo "grzip-0.3.0 has been tested on x86, x86_64 and alpha processors."
	@echo "It should therefore be 64-bit safe."
	@echo ""	
	@echo "More work is needed for big-endian machines (e.g. LinuxPPC)"
	@echo "since the code does not work on those yet."
	@echo ""		
	@echo "On 32 bit systems, grzip only works with files < 2GB in size."
	@echo "This limitation can be overcome (up to 4 GB on 32-bit systems)"
	@echo "by using grzip with <stdin> and <stdout> rather than directly on files"
	@echo ""	

all: libgrzip.so libgrzip.a grzip potfiles

install: all
	if [ -x grzip ] ; then install -c -s grzip $(DESTDIR)/bin/grzip ; fi
	cd $(DESTDIR)/bin ; ln -s -f grzip grunzip
	cd $(DESTDIR)/bin ; ln -s -f grzip grzcat
	install -c scripts/grztar $(DESTDIR)/bin
	install -c scripts/grzdiff $(DESTDIR)/bin/grzdiff
	install -c scripts/grzgrep $(DESTDIR)/bin/grzgrep
	install -c scripts/grzmore $(DESTDIR)/bin/grzmore
	cd $(DESTDIR)/bin ; ln -s -f grzdiff grzcmp
	cd $(DESTDIR)/bin ; ln -s -f grzmore grzless
	if [ -x libgrzip.so ] ; then install -c -s libgrzip.so $(DESTDIR)/lib ; fi
	if [ -r libgrzip.a ] ; then install -c -s libgrzip.a $(DESTDIR)/lib ; fi	
	cp -f grzip.h $(DESTDIR)/include
	cd po ; make DESTDIR=$(DESTDIR) install

libgrzip.o: libGRZip.h CRC32.c BWT.c LZP.c ST4.c MTF_Ari.c WFC_Ari.c \
  Rec_Flt.c main.c libgrzip.c
	$(CC) $(CFLAGS) -c libgrzip.c -o libgrzip.o
	
libgrzip.so: libgrzip.o
	$(CC) -shared -Wl,-soname,libgrzip.so libgrzip.o -o libgrzip.so

libgrzip.a: libgrzip.o
	ar rc libgrzip.a libgrzip.o
	ranlib libgrzip.a

grzip: libgrzip.so
	$(CC) $(CFLAGS) grzip.c -o grzip $(LDFLAGS) $(GRZIPLIB)

grzip_static: libgrzip.a
	$(CC) $(CFLAGS) grzip.c -o grzip_static $(LDSTATICFLAGS)
	
# Original compressor client program par Ilya Grebnov  = GRZipII
# if still wanted (grzip can replace it)
#
GRZipII: libgrzip.a
	$(CC) $(CFLAGS) GRZipII.c -o GRZipII $(LDSTATICFLAGS)
	
pot:
	xgettext --keyword=_ -o grzip.pot main.c
	mv -f grzip.pot po

potfiles:
	cd po ; make DESTDIR=$(DESTDIR)
	
clean:
	-rm -f core build-stamp *.o *~ libgrzip.a libgrzip.so \
	       grzip grzip_static GRZipII
	cd po ; make clean

distclean: clean

#*-------------------------------------------------*#
#* End                                    makefile *#
#*-------------------------------------------------*#
