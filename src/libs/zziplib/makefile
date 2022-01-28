#-----------------------------
# Zzip/Zzlib Makefile
#-----------------------------

CC			= gcc
LD			= gcc
STRIP		= strip

WARNING		= -W -Wall -Wshadow -Wpointer-arith -Winline
# Optionnal:
# -fprofile-arcs -fbranch-probabilities
OPTFLAGS	= -O3 -ffast-math -fforce-addr -fstrict-aliasing -fomit-frame-pointer -foptimize-sibling-calls
I386		= -mcpu=pentium2 -malign-double -maccumulate-outgoing-args
SPARC		= -mcpu=v8
WINDOWS		= -DWIN32
UNIX		= -DUNIX

CFLAGS		= $(I386) $(OPTFLAGS) $(WINDOWS) $(WARNING) -c
LFLAGS		= $(I386)

OBJS		= coding.o struct_model0.o struct_model1.o
OBJS_SFX	= zzipstub.o coding-sfx.o struct_model0-sfx.o struct_model1-sfx.o block-sfx.o
OBJS_ZZIP	= $(OBJS) zzip.o block.o bwt.o
OBJS_ZZLIB	= $(OBJS) bwt-dll.o block-dll.o

ZZIP_EXE	= zzip.exe
MAKE_H_EXE	= make_h.exe
SFX_EXE		= zzip-sfx.exe
ZZLIB_DLL	= zzlib.dll

all : $(ZZIP_EXE)

sfx : $(SFX_EXE)

dll : $(ZZLIB_DLL)

clean :
	del *.o zzipstub.sfx sfx_code.h $(ZZIP_EXE) $(MAKE_H_EXE) $(SFX_EXE) $(ZZLIB_DLL) libzzlib.a

$(MAKE_H_EXE) : make_h.c
	$(LD) $(LFLAGS) -o $(MAKE_H_EXE) make_h.c
	$(STRIP) $(MAKE_H_EXE)

sfx_code.h : $(MAKE_H_EXE)
	$(MAKE_H_EXE) zzipstub.sfx > sfx_code.h

$(SFX_EXE) : zzip-sfx.c global.h zzipstub.sfx
	$(LD) $(LFLAGS) -o $(SFX_EXE) zzip-sfx.c
	$(STRIP) $(SFX_EXE)

$(ZZIP_EXE) : $(OBJS_ZZIP)
	$(LD) $(LFLAGS) -o $(ZZIP_EXE) $(OBJS_ZZIP)
	$(STRIP) $(ZZIP_EXE)

zzipstub.sfx : $(OBJS_SFX)
	$(LD) $(LFLAGS) -o zzipstub.sfx $(OBJS_SFX)
	$(STRIP) zzipstub.sfx
	$(MAKE_H_EXE) zzipstub.sfx > sfx_code.h
	$(CC) $(CFLAGS) -DSFX -Os -o block-sfx.o block.c
	$(LD) $(LFLAGS) -o zzipstub.sfx $(OBJS_SFX)
	$(STRIP) zzipstub.sfx
	$(MAKE_H_EXE) zzipstub.sfx > sfx_code.h

$(ZZLIB_DLL) : $(OBJS_ZZLIB)
	$(LD) $(LFLAGS) -shared -Wl,--out-implib,libzzlib.a,--kill-at -o $(ZZLIB_DLL) $(OBJS_ZZLIB)
	$(STRIP) $(ZZLIB_DLL)

$(OBJS_SFX) $(OBJS_ZZLIB) $(OBJS_ZZIP) : global.h zzip.h

zzip.o : zzip.c
	$(CC) $(CFLAGS) -DZZIP zzip.c

block.o : block.c
	$(CC) $(CFLAGS) -DZZIP block.c

bwt.o : bwt.c
	$(CC) $(CFLAGS) -DZZIP bwt.c

coding.o : coding.c
	$(CC) $(CFLAGS) -funroll-loops coding.c

struct_model0.o : struct_model0.c ac-common.h
	$(CC) $(CFLAGS) struct_model0.c

struct_model1.o : struct_model1.c ac-common.h
	$(CC) $(CFLAGS) struct_model1.c

block-dll.o : block.c
	$(CC) $(CFLAGS) -DZZLIB -o block-dll.o block.c

bwt-dll.o : bwt.c
	$(CC) $(CFLAGS) -DZZLIB -o bwt-dll.o bwt.c

zzipstub.o : zzip.c
	$(CC) $(CFLAGS) -DSFX -Os -o zzipstub.o zzip.c

block-sfx.o : block.c sfx_code.h
	$(CC) $(CFLAGS) -DSFX -Os -o block-sfx.o block.c

coding-sfx.o : coding.c
	$(CC) $(CFLAGS) -DSFX -o coding-sfx.o coding.c

struct_model0-sfx.o : struct_model0.c ac-common.h
	$(CC) $(CFLAGS) -DSFX -o struct_model0-sfx.o struct_model0.c

struct_model1-sfx.o : struct_model1.c ac-common.h
	$(CC) $(CFLAGS) -DSFX -o struct_model1-sfx.o struct_model1.c
