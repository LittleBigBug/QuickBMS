# Copyright (C) powturbo 2015-2020
# nmake "NCODEC1=1" "NCODEC2=1" /f makefile.msc
# or
# nmake "AVX2=1" /f makefile.msc

.SUFFIXES: .c .obj .dllobj

CC = cl /nologo
LD = link /nologo
AR = lib /nologo
CFLAGS = /MD -I.
LDFLAGS = 

LIB_LIB = libtrle.lib
LIB_DLL = trle.dll
LIB_IMP = trle.lib

OBJS	= trled.obj trlec.obj  

!IF "$(AVX2)" == "1"
DEFS = $(DEFS) /D__AVX2__
!endif

!IF "$(NSSE2)" != "1"
DEFS = $(DEFS) /D__SSE__
!endif

DLL_OBJS = $(OBJS:.obj=.dllobj)

all:	$(LIB_LIB) trle.exe 

#$(LIB_DLL) $(LIB_IMP) 

.c.obj:
	$(CC) -c /Fo$@ /O2 $(CFLAGS) $(DEFS) $**

.cc.obj:
	$(CC) -c /Fo$@ /O2 $(CFLAGS) $(DEFS) $**

.c.dllobj:
	$(CC) -c /Fo$@ /O2 $(CFLAGS) $(DEFS) /DLIB_DLL $**

$(LIB_LIB): $(OBJS)
	$(AR) $(ARFLAGS) -out:$@ $(OBJS)

$(LIB_DLL): $(DLL_OBJS)
	$(LD) $(LDFLAGS) -out:$@ -dll -implib:$(LIB_IMP) $(DLL_OBJS)

$(LIB_IMP): $(LIB_DLL)

trle.exe: trle.obj vs/getopt.obj $(LIB_LIB)
	$(LD) $(LDFLAGS) -out:$@ $**

clean:
	-del *.dll *.exe *.exp *.obj *.dllobj *.lib *.manifest 2>nul
