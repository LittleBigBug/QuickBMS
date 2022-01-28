#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "lzw.h"

void lzw_writebuf(void *stream, char *buf, unsigned size)
{
	fwrite(buf, size, 1, (FILE*)stream);
}

unsigned lzw_readbuf(void *stream, char *buf, unsigned size)
{
	return fread(buf, 1, size, (FILE*)stream);
}

// global object
lzw_enc_t lzw;

/******************************************************************************
**  main
**  --------------------------------------------------------------------------
**  Encodes input byte stream into LZW code stream.
**  
**  Arguments:
**      argv[1] - input file name;
**      argv[2] - output file name;
**
**  Return: error code
******************************************************************************/
int main (int argc, char* argv[])
{
	FILE       *fin;
	FILE       *fout;
	lzw_enc_t  *ctx = &lzw;
	unsigned   len;
	char       buf[256];

	if (argc < 3) {
		printf("Usage: lzw-enc <input file> <output file>\n");
		return -1;
	}

	if (!(fin = fopen(argv[1], "rb"))) {
		fprintf(stderr, "Cannot open %s\n", argv[1]);
		return -2;
	}

	if (!(fout = fopen(argv[2], "w+b"))) {
		fprintf(stderr, "Cannot open %s\n", argv[2]);
		return -3;
	}

	lzw_enc_init(ctx, fout);

	while (len = lzw_readbuf(fin, buf, sizeof(buf)))
	{
		lzw_encode(ctx, buf, len);
	}

	lzw_enc_end(ctx);

	fclose(fin);
	fclose(fout);

	return 0;
}