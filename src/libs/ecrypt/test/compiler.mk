
comp ?= gcc
cc ?= gcc

ifeq ($(comp),msvc)
  CC = $(cc) -nologo
  version = $(cc)

  opt ?= -Ox

  obj = .obj
  exe = .exe
  out = -Fo

  ifeq ($(shell uname),Linux) 
    run = wine
  endif
endif

ifeq ($(comp),icc)
  CC = $(cc) $(std)
  version = $(cc) -v

  opt ?= -O3
endif

ifeq ($(comp),gcc)
  CC = $(cc) -Wall -pedantic $(std)
  version = $(cc) -v

  opt ?= -O2 -fomit-frame-pointer
endif

ifeq ($(comp),mingw)
  CC = $(cc) -Wall -pedantic $(std)
  version = $(cc) -v

  opt ?= -O2 -fomit-frame-pointer

  exe = .exe

  ifeq ($(shell uname),Linux) 
    run = wine
  endif
endif

ifeq ($(comp),cc)
  CC = $(cc)

  ifeq ($(shell uname),HP-UX)
    version = ($(cc) -V -E - < /dev/null > /dev/null)
  else
    version = $(cc) -V
  endif

  opt ?= -fast
endif

obj ?= .o
exe ?=
out ?= -o $(empty)

testobj ?= $(obj)

%$(testobj): %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $(out)$@ $<

%$(testobj): %.S
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $(out)$@ $<

%$(testobj): %.s
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $(out)$@ $<

%$(id)$(exe): %$(testobj)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@

version:
	@$(version) 2>&1

.PHONY: version
