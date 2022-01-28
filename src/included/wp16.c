// modified by Luigi Auriemma

// http://romxhack.esforos.com/compresion-de-final-fantasy-1-de-psp-la-famosa-wp16-t44

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

int wp16_decompress(unsigned char *in, unsigned char *out, size_t compressed_size, size_t decompressed_size) {
    size_t  real_decompressed_size = 0;
    if(!memcmp(in, "Wp16", 4)) {
        in += 4;
        real_decompressed_size = in[0] | (in[1]<<8) | (in[2]<<16) | (in[3]<<24);
        if(real_decompressed_size < decompressed_size) decompressed_size = real_decompressed_size;
        in += 4;
    }
    int x;

    unsigned char *inl = in + compressed_size;
    unsigned char *o = out;
    unsigned char *ol = out + decompressed_size;

  #define wp16_CONTEXT 0x800
  #define wp16_BIT(n, b) (((n) >> (b)) & 1)

  u16 history[wp16_CONTEXT];
  memset(history, 0, sizeof(u16) * wp16_CONTEXT);
  size_t i = 0, written = 0;
  //size_t start = ftell(in);

  #define wp16_WRITE(x) {                          \
    history[i] = (x);                         \
    /*fwrite(&history[i], sizeof(u16), 1, out);*/ \
    if((o + 2) > ol) goto quit; \
    *o++ = history[i]; \
    *o++ = history[i] >> 8; \
    written++;                                \
    i = (i + 1) & (wp16_CONTEXT - 1);              \
  }
  #define wp16_HISTORY(delta) history[(i - (delta)) & (wp16_CONTEXT - 1)]

  //while (!feof(in) && ftell(in) < start + compressed_size) {
  while(in < inl) {
    u8 flags[4];
    //fread(&flags, sizeof(u8), 4, in);
    for(x = 0; x < 4; x++) flags[x] = *in++;

    u16 vs[32];
    //fread(vs, sizeof(u16), 32, in);
    for(x = 0; x < 32; x++) {
        vs[x] = in[0] | (in[1]<<8);
        in += 2;
    }

    size_t b;
    for ( b = 0; b < 32; b++) {
      if (wp16_BIT(flags[b >> 3], b & 0x7)) { // Copy verbatim
        wp16_WRITE(vs[b]);

      } else { // Copy from history
        size_t count = (vs[b] & 0x1F) + 2,
               dist  = (vs[b] >> 5);
        size_t k;
        for ( k = 0; k < count; k++) wp16_WRITE(wp16_HISTORY(dist))
       }
    }
  }
quit:
  #undef wp16_CONTEXT
  #undef wp16_BIT
  #undef wp16_WRITE
  #undef wp16_HISTORY
    return o - out;
}

/*
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[1]);
    return 1;
  }

  FILE *f = fopen(argv[1], "rb");
  if (f == NULL) {
    fprintf(stderr, "Couldn't open '%s' for reading.\n", argv[1]);
    return 1;
  }

  // Header
  char magic[4];
  fread(magic, sizeof(char), 4, f);
  assert(strncmp(magic, "Wp16", 4) == 0);

  u32 filesize;
  fread(&filesize, sizeof(u32), 1, f);


  char *name = malloc(strlen(argv[1]) + 32);
  sprintf(name, "%s.unpack", argv[1]);
  FILE *fo = fopen(name, "wb");

  // Compressed data
  decompress(f, fo, filesize - 8);

  fclose(f);
  fclose(fo);
  return 0;
}
*/
