OBJS=huffman.o lzl.o
CC=gcc
CFLAGS=-Wall -O6
#CFLAGS=-Wall -O0 -g

all:	libmmini.a mmini_test mminizip

libmmini.a: $(OBJS)
	rm -f libmmini.a
	ar rcs libmmini.a $(OBJS)
	ranlib libmmini.a

mmini_test: $(OBJS)
	$(CC) -DBENCH_AGAINST_ZLIB -o mmini_test mmini_test.c $(OBJS) -lz

mminizip: $(OBJS)
	$(CC) -o mminizip mminizip.c $(OBJS)

clean:
	rm -f $(OBJS) libmmini.a mmini_test mminizip

test:	mmini_test
	./mmini_test

samples:	mminizip
	rm -f samples/1/lzl-huffman samples/1/huffman samples/1/lzl samples/1/lzo1x-1 samples/1/lzo1x-9 samples/1/gzip-1 samples/1/gzip-9
	./mminizip c samples/1/original samples/1/lzl-huffman
	./mminizip H samples/1/original samples/1/huffman
	./mminizip L samples/1/original samples/1/lzl
	cat samples/1/original | gzip -1 >samples/1/gzip-1
	cat samples/1/original | gzip -9 >samples/1/gzip-9
	cat samples/1/original | lzop -1 >samples/1/lzo1x-1
	cat samples/1/original | lzop -9 >samples/1/lzo1x-9
	ls -l samples/1
	rm -f samples/2/lzl-huffman samples/2/huffman samples/2/lzl samples/2/lzo1x-1 samples/2/lzo1x-9 samples/2/gzip-1 samples/2/gzip-9
	./mminizip c samples/2/original samples/2/lzl-huffman
	./mminizip H samples/2/original samples/2/huffman
	./mminizip L samples/2/original samples/2/lzl
	cat samples/2/original | gzip -1 >samples/2/gzip-1
	cat samples/2/original | gzip -9 >samples/2/gzip-9
	cat samples/2/original | lzop -1 >samples/2/lzo1x-1
	cat samples/2/original | lzop -9 >samples/2/lzo1x-9
	ls -l samples/2
