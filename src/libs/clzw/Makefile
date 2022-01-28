CC=gcc
CFLAGS =-g -O

lzw-enc: lzw-enc.o
	$(CC) encoder.c $< -o $@

lzw-dec: lzw-dec.o
	$(CC) decoder.c $< -o $@

lzw.a: lzw-enc.o lzw-dec.o
	$(AR) -cq $@ $<

lzw-enc.o lzw-dec.o: %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

lzw-enc.o lzw-dec.o: lzw.h

clean:
	rm -f lzw-enc.o lzw-dec.o lzw.a lzw-enc lzw-dec
