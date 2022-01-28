##
## blzpack - BriefLZ example
##
## GCC Makefile
##

.SUFFIXES:

.PHONY: clean all

CFLAGS = -std=c99 -Wall -Wno-unused-function -msse2 -mfpmath=sse -O3 -flto
CPPFLAGS = -DNDEBUG -I../include

ifeq ($(OS),Windows_NT)
  LDFLAGS += -static
  ifeq ($(CC),cc)
    CC = gcc
  endif
endif

objs = blzpack.o parg.o brieflz.o depack.o depacks.o

target = blzpack

all: $(target)

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o : ../src/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(target): $(objs)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(objs) $(target)

blzpack.o: ../include/brieflz.h parg.h
parg.o: parg.h
brieflz.o: ../include/brieflz.h
depack.o: ../include/brieflz.h
depacks.o: ../include/brieflz.h
