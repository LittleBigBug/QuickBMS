/* ctw-header.c : contains functions for reading and writing the CTW header
                  NOTE: all settings are checked before write_header is used */

#include <stdio.h>
#include "ctw-settings.h"


#define HEADERSIZE       12 /* HEADERSIZE is fixed at the moment, may be variable in future */
#define CTWFILE_VERSION  0

/* flags: specify bit positions of different CTW flags */
#define F_STRICTPRUNING  32
#define F_ROOTWEIGHTING  64
#define F_ZEROREDUNDANCY 128


/* put_bytes: local function that writes cnt bytes from value to out. Makes sure the value
   is always stored little-endian: first the most significant byte, last the least significant */
void put_bytes(FILE *out, unsigned int value, byte cnt)
{
  while (cnt--)
    putc((value >> (cnt * 8)) & 0xff, out);
}

/* get_bytes: get cnt files from in, little-endian */
unsigned int get_bytes(FILE *in, byte cnt)
{
  unsigned int value = 0;
  while (cnt--)
    value = (value << 8) | getc(in);
  return value;
}

/* calc_log2: calculates the 2-logarithm of b.
   Returns the correct answer only if b is a power of 2 */
byte calc_log2(unsigned int b)
{
  byte cnt = 0;
  while ((b & 1) == 0 && cnt <= 32)
    { cnt++; b >>= 1; }
  return cnt;
}
  

void write_header (FILE *out, unsigned int filesize)
{
  byte tmp;

  put_bytes(out, HEADERSIZE, 2);  /* header size (2 bytes) */
  putc(CTWFILE_VERSION, out);     /* CTW file format version (1 byte) */
  putc(0, out);                   /* file flags (always zero in this version) (1 byte) */
  put_bytes(out, filesize, 4);    /* uncoded filesize (4 bytes) */
  putc(settings.treedepth, out);  /* tree depth (1 byte) */
  putc(settings.maxnrtries, out); /* max nr tries (1 byte) */
  tmp = calc_log2(settings.maxnrnodes);
  if (settings.strictpruning) tmp |= F_STRICTPRUNING;
  if (settings.rootweighting) tmp |= F_ROOTWEIGHTING;
  if (settings.use_zeroredundancy) tmp |= F_ZEROREDUNDANCY;
  putc(tmp, out);                 /* nr of nodes and flags (1 byte) */
  tmp = calc_log2(settings.maxfilebufsize) - 9;
  tmp |= (calc_log2(settings.maxlogbeta) << 4);
  putc(tmp, out);                 /* maxlogbeta and filebufsize (1 byte) */
}


boolean read_header (FILE *in, unsigned int *filesize)
{
  unsigned int headersize;
  byte tmp;
  
  headersize = get_bytes(in, 2);   /* get header size (2 bytes) */
  if (getc(in) != CTWFILE_VERSION) /* get CTW file version (1 byte) */
  {
    fprintf(stderr, "Error: invalid CTW file version number\n");
    return false;
  }
  if (headersize < HEADERSIZE)
  {
    fprintf(stderr, "Error: invalid header size\n");
    return false;
  }
  getc(in);                        /* get file flags (ignored at the moment) (1 byte) */
  *filesize = get_bytes(in, 4);    /* get uncompressed file size (4 bytes) */
  settings.treedepth = getc(in);   /* tree depth (1 byte) */
  settings.maxnrtries = getc(in);  /* max nr tries (1 byte) */
  tmp = getc(in);
  settings.maxnrnodes = 1 << (tmp & 31);
  settings.strictpruning = (tmp & F_STRICTPRUNING) != 0;
  settings.rootweighting = (tmp & F_ROOTWEIGHTING) != 0;
  settings.use_zeroredundancy = (tmp & F_ZEROREDUNDANCY) != 0;
  tmp = getc(in);
  settings.maxfilebufsize = 1 << ((tmp & 15) + 9);
  if (settings.maxfilebufsize > *filesize)
    settings.filebufsize = *filesize;
  else
    settings.filebufsize = settings.maxfilebufsize;
  settings.maxlogbeta = 1 << (tmp >> 4);
  if (headersize > HEADERSIZE)
  {
    fprintf(stderr, "Warning: header is too long, unknown header fields are ignored\n");
    fseek(in, headersize, SEEK_SET);
  }
  return true;
}
