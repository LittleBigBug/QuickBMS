/* ecrypt-test.c */

/* 
 * API conformance test, test vector generation, and speed measurement (DRAFT)
 *
 * Based on the NESSIE test suite (http://www.cryptonessie.org/)
 */

/* ------------------------------------------------------------------------- */

#define QUOTE(str) QUOTE_HELPER(str)
#define QUOTE_HELPER(str) # str

#include "ecrypt-portable.h"
#include QUOTE(ECRYPT_API)

#if defined(ECRYPT_SSYN) || defined(ECRYPT_SSYN_AE)
#error self-synchronising stream ciphers are not supported yet
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------------- */

int compare_blocks(const u8 *m1, const u8 *m2, int len_bits)
{
  int i;
  const int lenb = (len_bits + 7) >> 3;
  const int mask0 = (1 << (((len_bits - 1) & 7) + 1)) - 1;

  if ((m1[0] & mask0) != (m2[0] & mask0))
    return 1;

  for (i = 1; i < lenb; i++)
    if (m1[i] != m2[i])
      return 1;
  
  return 0;
}

void print_data(FILE *fd, const char *str, const u8 *val, int len)
{
  int i;

  static const char hex[] = "0123456789ABCDEF";

  fprintf(fd, "%28s = ", str);

  for (i = 0; i < len; i++)
    {
      if (i > 0 && (i & 0xF) == 0 && (len > 24))
        fprintf(fd, "\n%28s   ", "");

      putc(hex[(val[i] >> 4) & 0xF], fd);
      putc(hex[(val[i]     ) & 0xF], fd);
    }

  fprintf(fd, "\n");
}

void print_chunk(FILE *fd, const char *str, const u8 *val, int start, int len)
{
  char indexed[80];

  sprintf(indexed, "%s[%d..%d]", str, start, start + len - 1);
  print_data(fd, indexed, val + start, len);
}

void xor_digest(const u8 *stream, int size, u8 *out, int outsize)
{
  int i;
  memset(out, 0, outsize);
  for (i = 0; i < size; i++)
    out[i % outsize] ^= stream[i];
}

/* ------------------------------------------------------------------------- */

double cpu_speed = 0.0;
double test_time = 3.0;

int quiet = 0;
int test_packet = 1;
int test_setup = 1;
int test_agility = 1;
int output_vectors = 0;

int errors = 0;

/* ------------------------------------------------------------------------- */

void check_status()
{
  const double sec = (double)clock() / (double)CLOCKS_PER_SEC;

  if (errors >= 10)
    {
      if (!quiet)
	fprintf(stderr, 
	  "Too many errors (%d errors). Aborting test.\n", errors);

      exit(3);
    }

  if (sec > test_time)
    {
      if (!quiet)
	fprintf(stderr,
	  "Time out (%.2f seconds). Aborting test.\n", sec);

      exit(1);
    }
}

/* ------------------------------------------------------------------------- */

#if defined(ECRYPT_SYNC_AE) || defined(ECRYPT_SSYN_AE)

#define ECRYPT_AE

#define CTX ECRYPT_AE_ctx
#define IVSETUP ECRYPT_AE_ivsetup
#define ENCRYPT_BYTES ECRYPT_AE_encrypt_bytes
#define DECRYPT_BYTES ECRYPT_AE_decrypt_bytes
#define AUTHENTICATE_BYTES ECRYPT_AE_authenticate_bytes
#define ENCRYPT_BLOCKS ECRYPT_AE_encrypt_blocks
#define DECRYPT_BLOCKS ECRYPT_AE_decrypt_blocks
#define KEYSETUP ECRYPT_AE_keysetup
#define ENCRYPT_PACKET ECRYPT_AE_encrypt_packet
#define DECRYPT_PACKET ECRYPT_AE_decrypt_packet
#define FINALIZE ECRYPT_AE_finalize

#else

#define CTX ECRYPT_ctx
#define IVSETUP ECRYPT_ivsetup
#define ENCRYPT_BYTES ECRYPT_encrypt_bytes
#define DECRYPT_BYTES ECRYPT_decrypt_bytes
#define ENCRYPT_BLOCKS ECRYPT_encrypt_blocks
#define DECRYPT_BLOCKS ECRYPT_decrypt_blocks

#define KEYSETUP(ctx, key, keysize, ivsize, macsize)                         \
  ECRYPT_keysetup(ctx, key, keysize, ivsize)

#define ENCRYPT_PACKET(                                                      \
    ctx, iv, aad, aadlen, plaintext, ciphertext, msglen, mac)                \
  ECRYPT_encrypt_packet(ctx, iv, plaintext, ciphertext, msglen)

#define DECRYPT_PACKET(                                                      \
    ctx, iv, aad, aadlen, ciphertext, plaintext, msglen, mac)                \
  ECRYPT_decrypt_packet(ctx, iv, ciphertext, plaintext, msglen)

#define FINALIZE(ctx, checkmac)

#define ECRYPT_MAXMACSIZE 0
#define ECRYPT_MACSIZE(i) (i)

#endif

#define NONZEROSIZE(s) (((s) <= 0) ? 4 : (s))

#define MAXKEYSIZEB NONZEROSIZE((ECRYPT_MAXKEYSIZE + 7) / 8)
#define MAXIVSIZEB NONZEROSIZE((ECRYPT_MAXIVSIZE + 7) / 8)
#define MAXMACSIZEB NONZEROSIZE((ECRYPT_MAXMACSIZE + 7) / 8)

/* ------------------------------------------------------------------------- */

void print_header(FILE *fd)
{
  fprintf(fd, 
	  "****************************************"
	  "****************************************\n");
  fprintf(fd, 
	  "*                          ECRYPT Stream"
	  " Cipher Project                        *\n");
  fprintf(fd, 
	  "****************************************"
	  "****************************************\n");
}

void print_primitive(FILE *fd, int keysize, int ivsize, int macsize)
{
  fprintf(fd, "\n");
  fprintf(fd, "Primitive Name: %s\n", ECRYPT_NAME);
  fprintf(fd, "================%.*s\n", (int)strlen(ECRYPT_NAME),
	  "==========================================");
  fprintf(fd, "Profile: %s\n", ECRYPT_PROFILE);
  fprintf(fd, "Key size: %d bits\n", keysize);
  fprintf(fd, "IV size: %d bits\n", ivsize);
#ifdef ECRYPT_AE
  fprintf(fd, "MAC size: %d bits\n", macsize);
#endif
  fprintf(fd, "\n");
}

/* ------------------------------------------------------------------------- */

#define TEST_STREAM_SIZEB 0x200
#define LONG_TEST_STREAM_SIZEB 0x20000
#define TEST_CHUNK 64

#ifdef ECRYPT_LONG_VECTORS
#define TEST_STEP 1
#else
#define TEST_STEP 9
#endif

typedef struct
{
  ALIGN(u8, key, MAXKEYSIZEB);
  ALIGN(u8, iv, MAXIVSIZEB);

  ALIGN(u8, plaintext, LONG_TEST_STREAM_SIZEB);
  ALIGN(u8, ciphertext, LONG_TEST_STREAM_SIZEB);
  ALIGN(u8, checktext, LONG_TEST_STREAM_SIZEB);

  ALIGN(u8, xored ,TEST_CHUNK);

#ifdef ECRYPT_AE
  ALIGN(u8, aad, TEST_CHUNK);
  ALIGN(u8, mac, MAXMACSIZEB);
  ALIGN(u8, checkmac, MAXMACSIZEB);
#endif

  CTX ctx;

  int keysize;
  int ivsize;
  int msglen;

#ifdef ECRYPT_AE
  int macsize;
  int aadlen;
#endif

  FILE *fd;

  int vector;

} test_struct;

void encrypt_and_check(test_struct* t, void (*print)(test_struct*, int))
{
  u8* plaintext;
  u8* ciphertext;
  u8* checktext;
  int msglen;

  unsigned int i;

  memset(t->ciphertext.b, 0, sizeof(t->ciphertext.b));
#ifdef ECRYPT_AE
  memset(t->mac.b, 0, sizeof(t->mac.b));
#endif

  KEYSETUP(&t->ctx, t->key.b, t->keysize, t->ivsize, t->macsize);
  ENCRYPT_PACKET(&t->ctx, t->iv.b, 
    t->aad.b, t->aadlen, t->plaintext.b, t->ciphertext.b, t->msglen, t->mac.b);

  print(t, 0);

#ifdef ECRYPT_AE
  memset(t->checkmac.b, 0, sizeof(t->checkmac.b));
#endif
  memset(t->checktext.b, 0, sizeof(t->checktext.b));

  KEYSETUP(&t->ctx, t->key.b, t->keysize, t->ivsize, t->macsize);
  DECRYPT_PACKET(&t->ctx, t->iv.b, t->aad.b, t->aadlen,
    t->ciphertext.b, t->checktext.b, t->msglen, t->checkmac.b);

  if (compare_blocks(t->plaintext.b, t->checktext.b, t->msglen * 8) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_packet:\n"
	      "*** decrypted text differs from plaintext:\n");
      print(t, 1);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(t->mac.b, t->checkmac.b, t->macsize) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_packet:\n"
	      "*** decryption MAC differs from encryption MAC:\n");
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8);
    }

  memset(t->checkmac.b, 0, sizeof(t->checkmac.b));
#endif
  memset(t->checktext.b, 0, sizeof(t->checktext.b));

  IVSETUP(&t->ctx, t->iv.b);

#ifdef ECRYPT_SUPPORTS_AAD
  AUTHENTICATE_BYTES(&t->ctx, t->aad.b, t->aadlen);
#endif

  ENCRYPT_BYTES(&t->ctx, t->plaintext.b, t->checktext.b, t->msglen);
  FINALIZE(&t->ctx, t->checkmac.b);

  if (compare_blocks(t->ciphertext.b, t->checktext.b, t->msglen * 8) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> encrypt_bytes:\n"
	      "*** encrypt_bytes generates different ciphertext:\n");
      print(t, 2);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(t->mac.b, t->checkmac.b, t->macsize) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> encrypt_bytes:\n"
	      "*** encrypt_bytes generates different MAC:\n");
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8);
    }

  memset(t->checkmac.b, 0, sizeof(t->checkmac.b));
#endif
  memset(t->checktext.b, 0, sizeof(t->checktext.b));

  IVSETUP(&t->ctx, t->iv.b);

#ifdef ECRYPT_SUPPORTS_AAD
  AUTHENTICATE_BYTES(&t->ctx, t->aad.b, t->aadlen);
#endif

  DECRYPT_BYTES(&t->ctx, t->ciphertext.b, t->checktext.b, t->msglen);
  FINALIZE(&t->ctx, t->checkmac.b);

  if (compare_blocks(t->plaintext.b, t->checktext.b, t->msglen * 8) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_bytes:\n"
	      "*** decrypt_bytes generates different plaintext:\n");
      print(t, 2);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(t->mac.b, t->checkmac.b, t->macsize) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_bytes:\n"
	      "*** decrypt_bytes generates different MAC:\n");
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8);
    }

  memset(t->checkmac.b, 0, sizeof(t->checkmac.b));
#endif
  memset(t->checktext.b, 0, sizeof(t->checktext.b));

  IVSETUP(&t->ctx, t->iv.b);

#ifdef ECRYPT_SUPPORTS_AAD
  AUTHENTICATE_BYTES(&t->ctx, t->aad.b, t->aadlen);
#endif

  plaintext = t->plaintext.b;
  checktext = t->checktext.b;
  msglen = t->msglen;

  for (i = (t->vector + 1) * 1381; msglen >= ECRYPT_BLOCKLENGTH; i *= 1487)
    {
      const int blocks = i % (msglen / ECRYPT_BLOCKLENGTH + 1);
      const int bytes = blocks * ECRYPT_BLOCKLENGTH;

      ENCRYPT_BLOCKS(&t->ctx, plaintext, checktext, blocks);

      plaintext += bytes;
      checktext += bytes;
      msglen -= bytes;

      if (blocks == 0)
	break;
    }

  ENCRYPT_BYTES(&t->ctx, plaintext, checktext, msglen);
  FINALIZE(&t->ctx, t->checkmac.b);

  if (compare_blocks(t->ciphertext.b, t->checktext.b, t->msglen * 8) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> encrypt_blocks/bytes:\n"
	      "*** encrypt_blocks/bytes generates different ciphertext:\n");
      print(t, 2);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(t->mac.b, t->checkmac.b, t->macsize) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> encrypt_blocks/bytes:\n"
	      "*** encrypt_blocks/bytes generates different MAC:\n");
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8);
    }

  memset(t->checkmac.b, 0, sizeof(t->checkmac.b));
#endif
  memset(t->checktext.b, 0, sizeof(t->checktext.b));

  IVSETUP(&t->ctx, t->iv.b);

#ifdef ECRYPT_SUPPORTS_AAD
  AUTHENTICATE_BYTES(&t->ctx, t->aad.b, t->aadlen);
#endif

  ciphertext = t->ciphertext.b;
  checktext = t->checktext.b;
  msglen = t->msglen;

  for (i = (t->vector + 1) * 1381; msglen >= ECRYPT_BLOCKLENGTH; i *= 1487)
    {
      const int blocks = i % (msglen / ECRYPT_BLOCKLENGTH + 1);
      const int bytes = blocks * ECRYPT_BLOCKLENGTH;

      DECRYPT_BLOCKS(&t->ctx, ciphertext, checktext, blocks);

      ciphertext += bytes;
      checktext += bytes;
      msglen -= bytes;

      if (blocks == 0)
	break;
    }

  DECRYPT_BYTES(&t->ctx, ciphertext, checktext, msglen);
  FINALIZE(&t->ctx, t->checkmac.b);

  if (compare_blocks(t->plaintext.b, t->checktext.b, t->msglen * 8) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_blocks/bytes:\n"
	      "*** decrypt_blocks/bytes generates different plaintext:\n");
      print(t, 2);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(t->mac.b, t->checkmac.b, t->macsize) != 0)
    {
      ++errors;
      fprintf(t->fd, 
	      "*** ERROR: encrypt_packet <-> decrypt_blocks/bytes:\n"
	      "*** decrypt_blocks/bytes generates different MAC:\n");
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8);
    }
#endif

  fprintf(t->fd, "\n");

  check_status();
}

void print_stream(test_struct* t, int type)
{
  const int chunk = TEST_CHUNK;

  switch (type)
    {
    case 0:
      print_data(t->fd, "key", t->key.b, (t->keysize + 7) / 8); 
      print_data(t->fd, "IV", t->iv.b, (t->ivsize + 7) / 8);    
      
      print_chunk(t->fd, "stream", t->ciphertext.b, 0, chunk);
      print_chunk(t->fd, "stream", t->ciphertext.b, t->msglen/2-chunk, chunk);
      print_chunk(t->fd, "stream", t->ciphertext.b, t->msglen/2, chunk);
      print_chunk(t->fd, "stream", t->ciphertext.b, t->msglen-chunk, chunk);
      
      xor_digest(t->ciphertext.b, t->msglen, t->xored.b, chunk);
      print_data(t->fd, "xor-digest", t->xored.b, chunk);  
      
#ifdef ECRYPT_AE
      print_data(t->fd, "MAC", t->mac.b, (t->macsize + 7) / 8); 
#endif
      break;
    case 1:
      print_chunk(t->fd, "decryption", t->checktext.b, 0, chunk);
      print_chunk(t->fd, "decryption", t->checktext.b, 
        t->msglen / 2-chunk, chunk);
      print_chunk(t->fd, "decryption", t->checktext.b, t->msglen / 2, chunk);
      print_chunk(t->fd, "decryption", t->checktext.b, t->msglen-chunk, chunk);
      
      xor_digest(t->checktext.b, t->msglen, t->xored.b, chunk);
      print_data(t->fd, "xor-digest", t->xored.b, chunk);  

#ifdef ECRYPT_AE
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8); 
#endif
      break;
    case 2:
      print_chunk(t->fd, "stream", t->checktext.b, 0, chunk);
      print_chunk(t->fd, "stream", t->checktext.b, t->msglen/2-chunk, chunk);
      print_chunk(t->fd, "stream", t->checktext.b, t->msglen/2, chunk);
      print_chunk(t->fd, "stream", t->checktext.b, t->msglen-chunk, chunk);
      
      xor_digest(t->checktext.b, t->msglen, t->xored.b, chunk);
      print_data(t->fd, "xor-digest", t->xored.b, chunk); 
 
#ifdef ECRYPT_AE   
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8); 
#endif
      break;
    }
}

void print_pair(test_struct* t, int type)
{
  switch (type)
    {
    case 0:
      print_data(t->fd, "key", t->key.b, (t->keysize + 7) / 8); 
      print_data(t->fd, "IV", t->iv.b, (t->ivsize + 7) / 8);

#ifdef ECRYPT_SUPPORTS_AAD
      if (t->aadlen)
	print_data(t->fd, "AAD", t->aad.b, t->aadlen);
#endif
      print_data(t->fd, "plaintext", t->plaintext.b, t->msglen);
      print_data(t->fd, "ciphertext", t->ciphertext.b, t->msglen); 
#ifdef ECRYPT_AE
      print_data(t->fd, "MAC", t->mac.b, (t->macsize + 7) / 8);      
#endif
      break;
    case 1:
      print_data(t->fd, "decryption", t->checktext.b, t->msglen); 
#ifdef ECRYPT_AE
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8); 
#endif
      break;
    case 2:
      print_data(t->fd, "ciphertext", t->checktext.b, t->msglen);
#ifdef ECRYPT_AE 
      print_data(t->fd, "MAC", t->checkmac.b, (t->macsize + 7) / 8); 
#endif
      break;
    }
}

void test_vectors(FILE *fd, int keysize, int ivsize, int macsize)
{

#define STREAM_VECTOR(set, vect)                                             \
  do {                                                                       \
    fprintf(fd, "Set %d, vector#%3d:\n", set, t.vector = vect);              \
    encrypt_and_check(&t, print_stream);                                     \
  } while (0)

#define MAC_VECTOR(set, vect)                                                \
  do {                                                                       \
    fprintf(fd, "Set %d, vector#%3d:\n", set, t.vector = vect);              \
    encrypt_and_check(&t, print_pair);                                       \
  } while (0)

#define AAD_VECTOR(set, vect)                                                \
  do {                                                                       \
    fprintf(fd, "Set %d, vector#%3d:\n", set, t.vector = vect);              \
    encrypt_and_check(&t, print_pair);                                       \
  } while (0)

  test_struct t;
  int i, v;

  print_primitive(fd, keysize, ivsize, macsize);

  memset(t.plaintext.b, 0, sizeof(t.plaintext.b));
  memset(t.ciphertext.b, 0, sizeof(t.ciphertext.b));

  /* check key stream */

  t.fd = fd;

  t.keysize = keysize;
  t.ivsize = ivsize;
#ifdef ECRYPT_AE
  t.macsize = macsize;
  t.aadlen = 0;
#endif
  t.msglen = TEST_STREAM_SIZEB;

  fprintf(t.fd, "Test vectors -- set 1\n");
  fprintf(t.fd, "=====================\n\n");
  fprintf(t.fd, "(stream is generated by encrypting %d zero bytes)\n\n", 
	  t.msglen);

  memset(t.iv.b, 0, sizeof(t.iv.b));

  for (v = 0; v < t.keysize; v += TEST_STEP)
    {
      memset(t.key.b, 0, sizeof(t.key.b));
      t.key.b[v >> 3] = 1 << (7 - (v & 7));
      
      STREAM_VECTOR(1, v);
    }

  fprintf(t.fd, "Test vectors -- set 2\n");
  fprintf(t.fd, "=====================\n\n");

  memset(t.iv.b, 0, sizeof(t.iv.b));

  for (v = 0; v < 256; v += TEST_STEP)
  {
    memset(t.key.b, v, sizeof(t.key.b));

    STREAM_VECTOR(2, v);
  }

  fprintf(fd, "Test vectors -- set 3\n");
  fprintf(fd, "=====================\n\n");

  memset(t.iv.b, 0, sizeof(t.iv.b));

  for (v = 0; v < 256; v += TEST_STEP)
  {
    for (i = 0; i < sizeof(t.key.b); i++)
      t.key.b[i] = (i + v) & 0xFF;

    STREAM_VECTOR(3, v);
  }

  t.msglen = LONG_TEST_STREAM_SIZEB;

  fprintf(t.fd, "Test vectors -- set 4\n");
  fprintf(t.fd, "=====================\n\n");

  for (v = 0; v < 4; v++)
  {
    for (i = 0; i< sizeof(t.key.b); i++)
      t.key.b[i] = (i * 0x53 + v * 5) & 0xFF;

    STREAM_VECTOR(4, v);
  }

  t.msglen = TEST_STREAM_SIZEB;

  fprintf(t.fd, "Test vectors -- set 5\n");
  fprintf(t.fd, "=====================\n\n");

  memset(t.key.b, 0, sizeof(t.key.b));

  for (v = 0; v < t.ivsize; v += TEST_STEP)
  {
    memset(t.iv.b, 0, sizeof(t.iv.b));
    t.iv.b[v >> 3] = 1 << (7 - (v & 7));

    STREAM_VECTOR(5, v);
  }

  t.msglen = LONG_TEST_STREAM_SIZEB;

  fprintf(t.fd, "Test vectors -- set 6\n");
  fprintf(t.fd, "=====================\n\n");

  for (v = 0; v < 4; v++)
  {
    for (i = 0; i < sizeof(t.key.b); i++)
      t.key.b[i] = (i * 0x53 + v * 5) & 0xFF;

    for (i = 0; i < sizeof(t.iv.b); i++)
      t.iv.b[i] = (i * 0x67 + v * 9 + 13) & 0xFF;

    STREAM_VECTOR(6, v);
  }

#if defined(ECRYPT_AE) || !defined(ECRYPT_GENERATES_KEYSTREAM)
  /* check MAC */

  t.msglen = TEST_STREAM_SIZEB;

  fprintf(t.fd, "Test vectors -- set 7\n");
  fprintf(t.fd, "=====================\n\n");

  memset(t.key.b, 0, sizeof(t.key.b));
  memset(t.iv.b, 0, sizeof(t.iv.b));
  memset(t.plaintext.b, 0, sizeof(t.plaintext.b));

  for (i = 0; i < sizeof(t.key.b); i++)
    t.key.b[i] = (i * 0x11) & 0xFF;

  for (v = 0; v <= TEST_CHUNK; v += TEST_STEP)
  {
    t.msglen = v;

    MAC_VECTOR(7, v);
  }

  t.msglen = TEST_CHUNK / 2;

  fprintf(t.fd, "Test vectors -- set 8\n");
  fprintf(t.fd, "=====================\n\n");

  memset(t.key.b, 0, sizeof(t.key.b));
  memset(t.iv.b, 0, sizeof(t.iv.b));

  for (v = 0; v < t.msglen * 8; v += TEST_STEP)
  {
    memset(t.plaintext.b, 0, sizeof(t.plaintext.b));
    t.plaintext.b[v >> 3] = 1 << (7 - (v & 7));

    MAC_VECTOR(8, v);
  }

  fprintf(t.fd, "Test vectors -- set 9\n");
  fprintf(t.fd, "=====================\n\n");

  for (v = 0; v < 4; v++)
  {
    for (i = 0; i < sizeof(t.key.b); i++)
      t.key.b[i] = (i * 0x53 + v * 5) & 0xFF;

    for (i = 0; i < sizeof(t.iv.b); i++)
      t.iv.b[i] = (i * 0x67 + v * 9 + 13) & 0xFF;

    for (i = 0; i < t.msglen; i++)
      t.plaintext.b[i] = (i * 0x61 + v * 7 + 109) & 0xFF;

    MAC_VECTOR(9, v);
  }

#ifdef ECRYPT_SUPPORTS_AAD
  /* check AAD */

  t.msglen = TEST_CHUNK / 2;

  fprintf(t.fd, "Test vectors -- set 10\n");
  fprintf(t.fd, "======================\n\n");

  memset(t.key.b, 0, sizeof(t.key.b));
  memset(t.iv.b, 0, sizeof(t.iv.b));
  memset(t.plaintext.b, 0, sizeof(t.plaintext.b));
  memset(t.aad.b, 0, sizeof(t.aad.b));

  for (i = 0; i < sizeof(t.key.b); i++)
    t.key.b[i] = (i * 0x11) & 0xFF;

  for (v = 0; v <= TEST_CHUNK; v += TEST_STEP)
  {
    t.aadlen = v;

    AAD_VECTOR(10, v);
  }

  t.aadlen = TEST_CHUNK / 2;

  fprintf(t.fd, "Test vectors -- set 11\n");
  fprintf(t.fd, "======================\n\n");

  memset(t.key.b, 0, sizeof(t.key.b));
  memset(t.iv.b, 0, sizeof(t.iv.b));
  memset(t.plaintext.b, 0, sizeof(t.plaintext.b));

  for (v = 0; v < t.aadlen * 8; v += TEST_STEP)
  {
    memset(t.aad.b, 0, sizeof(t.aad.b));
    t.aad.b[v >> 3] = 1 << (7 - (v & 7));

    AAD_VECTOR(11, v);
  }

  fprintf(t.fd, "Test vectors -- set 12\n");
  fprintf(t.fd, "======================\n\n");

  for (v = 0; v < 4; v++)
  {
    for (i = 0; i < sizeof(t.key.b); i++)
      t.key.b[i] = (i * 0x53 + v * 5) & 0xFF;

    for (i = 0; i < sizeof(t.iv.b); i++)
      t.iv.b[i] = (i * 0x67 + v * 9 + 13) & 0xFF;

    for (i = 0; i < t.msglen; i++)
      t.plaintext.b[i] = (i * 0x61 + v * 7 + 109) & 0xFF;

    for (i = 0; i < t.aadlen; i++)
      t.aad.b[i] = (i * 0x25 + v * 13 + 11) & 0xFF;

    AAD_VECTOR(12, v);
  }
#endif
#endif

  fprintf(t.fd, "\n\nEnd of test vectors\n");
  fflush(t.fd);
}

/* ------------------------------------------------------------------------- */

void test_if_conform_to_api(FILE *fd, int keysize, int ivsize, int macsize)
{ 
  ALIGN(u8, key[2], MAXKEYSIZEB);
  ALIGN(u8, iv[2], MAXIVSIZEB);
  
  ALIGN(u8, plaintext, TEST_CHUNK + ECRYPT_BLOCKLENGTH);
  ALIGN(u8, ciphertext[3], TEST_CHUNK + ECRYPT_BLOCKLENGTH);
#ifdef ECRYPT_AE
  ALIGN(u8, mac[3], MAXMACSIZEB);
#endif

  CTX ctx[2];
    
  int msglen = TEST_CHUNK;

  int i;

  for(i = 0; i < MAXKEYSIZEB; i++)
    {
      key[0].b[i] = 3 * i + 5;
      key[1].b[i] = 240 - 5 * i;
    }

  for(i = 0; i < MAXIVSIZEB; i++)
    {
      iv[0].b[i] = 9 * i + 25;
      iv[1].b[i] = 11 * i + 17;
    }

  memset(&plaintext, 0, sizeof(plaintext));
  memset(ciphertext, 0, sizeof(ciphertext));

  KEYSETUP(&ctx[0], key[0].b, keysize, ivsize, macsize);

  IVSETUP(&ctx[0], iv[0].b);
  ENCRYPT_BYTES(&ctx[0], plaintext.b, ciphertext[0].b, msglen);
  FINALIZE(&ctx[0], mac[0].b);

  IVSETUP(&ctx[0], iv[0].b);
  ENCRYPT_BYTES(&ctx[0], plaintext.b, ciphertext[1].b, msglen);
  FINALIZE(&ctx[0], mac[1].b);

  if (compare_blocks(ciphertext[0].b, ciphertext[1].b, msglen * 8) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** Two calls to ivsetup produced different results:\n");

      print_data(fd, "K", key[0].b, (keysize + 7) / 8);
      print_data(fd, "IV", iv[0].b, (ivsize + 7) / 8);

      print_data(fd, "P", plaintext.b, msglen);
      print_data(fd, "C after 1st IV setup", ciphertext[0].b, msglen);
      print_data(fd, "C after 2nd IV setup", ciphertext[1].b, msglen);
      fprintf(fd, "\n");
      fflush(fd);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(mac[0].b, mac[1].b, macsize) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** Two calls to ivsetup produced different results:\n");

      print_data(fd, "K", key[0].b, (keysize + 7) / 8);
      print_data(fd, "IV", iv[0].b, (ivsize + 7) / 8);

      print_data(fd, "P", plaintext.b, msglen);
      print_data(fd, "MAC after 1st IV setup", mac[0].b, (macsize + 7) / 8);
      print_data(fd, "MAC after 2nd IV setup", mac[1].b, (macsize + 7) / 8);
      fprintf(fd, "\n");
      fflush(fd);
    }
#endif

  check_status();

  memset(ciphertext, 0, sizeof(ciphertext));

  KEYSETUP(&ctx[0], key[0].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[0], iv[0].b);
  ENCRYPT_BYTES(&ctx[0], plaintext.b, ciphertext[0].b, msglen);
  FINALIZE(&ctx[0], mac[0].b);

  KEYSETUP(&ctx[1], key[1].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[1], iv[1].b);
  ENCRYPT_BYTES(&ctx[1], plaintext.b, ciphertext[1].b, msglen);
  FINALIZE(&ctx[1], mac[1].b);

  IVSETUP(&ctx[0], iv[0].b);

  IVSETUP(&ctx[1], iv[1].b);

  ENCRYPT_BYTES(&ctx[0], plaintext.b, ciphertext[2].b, msglen);
  FINALIZE(&ctx[0], mac[2].b);

  if (compare_blocks(ciphertext[0].b, ciphertext[2].b, msglen * 8) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** code produces inconsistent results when calls with different\n" 
	"*** contexts are interleaved:\n");

      if (compare_blocks(ciphertext[1].b, ciphertext[2].b, msglen * 8) == 0)
	fprintf(fd, 
	  "*** (this is probably due to the use of static state variables)\n");

      print_data(fd, "K1", key[0].b, (keysize + 7) / 8);
      print_data(fd, "K2", key[1].b, (keysize + 7) / 8);
      print_data(fd, "IV1", iv[0].b, (ivsize + 7) / 8);
      print_data(fd, "IV2", iv[0].b, (ivsize + 7) / 8);

      print_data(fd, "P", plaintext.b, msglen);
      print_data(fd, "C by K1", ciphertext[0].b, msglen);
      print_data(fd, "C by K2", ciphertext[1].b, msglen);
      print_data(fd, "C by K1 after IV2 setup", ciphertext[2].b, msglen);
      fprintf(fd, "\n");
      fflush(fd);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(mac[0].b, mac[2].b, macsize) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** code produces inconsistent results when calls with different\n" 
	"*** contexts are interleaved:\n");

      if (compare_blocks(mac[1].b, mac[2].b, macsize) == 0)
	fprintf(fd, 
	  "*** (this is probably due to the use of static state variables)\n");

      print_data(fd, "K1", key[0].b, (keysize + 7) / 8);
      print_data(fd, "K2", key[1].b, (keysize + 7) / 8);
      print_data(fd, "IV1", iv[0].b, (ivsize + 7) / 8);
      print_data(fd, "IV2", iv[0].b, (ivsize + 7) / 8);

      print_data(fd, "P", plaintext.b, msglen);
      print_data(fd, "MAC by K1", mac[0].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K2", mac[1].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K1 after IV2 setup", mac[2].b, (macsize + 7) / 8);
      fprintf(fd, "\n");
      fflush(fd);
    }
#endif

  check_status();

#define B ECRYPT_BLOCKLENGTH

  memset(ciphertext, 0, sizeof(ciphertext));

  KEYSETUP(&ctx[0], key[0].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[0], iv[0].b);
  ENCRYPT_BYTES(&ctx[0], plaintext.b + B, ciphertext[0].b + B, msglen);
  FINALIZE(&ctx[0], mac[0].b);

  KEYSETUP(&ctx[1], key[1].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[1], iv[1].b);
  ENCRYPT_BLOCKS(&ctx[1], plaintext.b, ciphertext[1].b, 1);
  ENCRYPT_BYTES(&ctx[1], plaintext.b + B, ciphertext[1].b + B, msglen);
  FINALIZE(&ctx[1], mac[1].b);

  IVSETUP(&ctx[0], iv[0].b);

  IVSETUP(&ctx[1], iv[1].b);
  ENCRYPT_BLOCKS(&ctx[1], plaintext.b, ciphertext[2].b, 1);

  ENCRYPT_BYTES(&ctx[0], plaintext.b + B, ciphertext[2].b + B, msglen);
  FINALIZE(&ctx[0], mac[2].b);

  if (compare_blocks(ciphertext[0].b + B, ciphertext[2].b + B,
        msglen * 8) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** code produces inconsistent results when calls with different\n" 
	"*** contexts are interleaved:\n");

      if (compare_blocks(ciphertext[1].b, ciphertext[2].b,
            (msglen + B) * 8) == 0)
	fprintf(fd, 
	  "*** (this is probably due to the use of static state variables)\n");

      print_data(fd, "K1", key[0].b, (keysize + 7) / 8);
      print_data(fd, "K2", key[1].b, (keysize + 7) / 8);
      print_data(fd, "IV1", iv[0].b, (ivsize + 7) / 8);
      print_data(fd, "IV2", iv[1].b, (ivsize + 7) / 8);

      print_data(fd, "(last part of) P", plaintext.b + B, msglen);
      print_data(fd, "C by K1", ciphertext[0].b + B, msglen);
      print_data(fd, "last part of C by K2", ciphertext[1].b + B, msglen);
      print_data(fd, "C by K1 after calls K2", ciphertext[2].b + B, msglen);
      fprintf(fd, "\n");
      fflush(fd);
    }
#ifdef ECRYPT_AE
  else if (compare_blocks(mac[0].b, mac[2].b, macsize) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** code produces inconsistent results when calls with different\n" 
	"*** contexts are interleaved:\n");

      if (compare_blocks(mac[1].b, mac[2].b, macsize) == 0)
	fprintf(fd, 
	  "*** (this is probably due to the use of static state variables)\n");

      print_data(fd, "K1", key[0].b, (keysize + 7) / 8);
      print_data(fd, "K2", key[1].b, (keysize + 7) / 8);
      print_data(fd, "IV1", iv[0].b, (ivsize + 7) / 8);
      print_data(fd, "IV2", iv[1].b, (ivsize + 7) / 8);

      print_data(fd, "(last part of) P", plaintext.b, msglen);
      print_data(fd, "MAC by K1", mac[0].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K2", mac[1].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K1 after K2 calls", mac[2].b, (macsize + 7) / 8);
      fprintf(fd, "\n");
      fflush(fd);
    }
#endif

  check_status();

#ifdef ECRYPT_SUPPORTS_AAD

  KEYSETUP(&ctx[0], key[0].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[0], iv[0].b);
  AUTHENTICATE_BYTES(&ctx[0], plaintext.b, msglen);
  FINALIZE(&ctx[0], mac[0].b);

  KEYSETUP(&ctx[1], key[1].b, keysize, ivsize, macsize);
  IVSETUP(&ctx[1], iv[1].b);
  AUTHENTICATE_BYTES(&ctx[1], plaintext.b, msglen);
  FINALIZE(&ctx[1], mac[1].b);

  IVSETUP(&ctx[0], iv[0].b);
  AUTHENTICATE_BYTES(&ctx[0], plaintext.b, msglen);

  IVSETUP(&ctx[1], iv[1].b);
  AUTHENTICATE_BYTES(&ctx[1], plaintext.b, msglen);
  FINALIZE(&ctx[1], mac[2].b);

  FINALIZE(&ctx[0], mac[2].b);

  if (compare_blocks(mac[0].b, mac[2].b, macsize) != 0)
    {
      ++errors;
      fprintf(fd, 
        "*** ERROR: Code does not conform to ECRYPT API:\n"
	"*** code produces inconsistent results when calls with different\n" 
	"*** contexts are interleaved:\n");

      if (compare_blocks(mac[1].b, mac[2].b, macsize) == 0)
	fprintf(fd, 
	  "*** (this is probably due to the use of static state variables)\n");

      print_data(fd, "K1", key[0].b, (keysize + 7) / 8);
      print_data(fd, "K2", key[1].b, (keysize + 7) / 8);
      print_data(fd, "IV1", iv[0].b, (ivsize + 7) / 8);
      print_data(fd, "IV2", iv[1].b, (ivsize + 7) / 8);

      print_data(fd, "AAD", plaintext.b, msglen);
      print_data(fd, "MAC by K1", mac[0].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K2", mac[1].b, (macsize + 7) / 8);
      print_data(fd, "MAC by K1 after K2 calls", mac[2].b, (macsize + 7) / 8);
      fprintf(fd, "\n");
      fflush(fd);
    }

  check_status();
#endif
}

/* ------------------------------------------------------------------------- */

#define PAGESIZE 0x2000

void* aligned_malloc(size_t size)
{
  void* ptr = malloc(size + PAGESIZE);

  if (ptr)
    {
      void* aligned = (void*)(((long)ptr + PAGESIZE) & ~(PAGESIZE - 1));
      ((void**)aligned)[-1] = ptr;

      return aligned;
    }
  else
    return NULL;
}

void aligned_free(void* aligned)
{
  if (aligned)
    {
      void* ptr = ((void**)aligned)[-1];
      free(ptr);
    }
}

/* ------------------------------------------------------------------------- */

#include "timers.h"

/* ------------------------------------------------------------------------- */

#ifdef _MSC_VER
int __cdecl cmp_double(const void* a, const void* b)
#else
int cmp_double(const void* a, const void* b)
#endif
{
  const double x = *(double*)a;
  const double y = *(double*)b;

  if (x > y)
    return 1;
  else if (x < y)
    return -1;
  else
    return 0;
}

/* ------------------------------------------------------------------------- */

#define BYTES_TO_BLOCKS(b) ((b + ECRYPT_BLOCKLENGTH - 1) / ECRYPT_BLOCKLENGTH)

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN_KEYS_TO_TEST 100
#define MAX_KEYS_TO_TEST (0x1000000 / sizeof(CTX))

#ifndef ECRYPT_BUFFERLENGTH
#define ECRYPT_BUFFERLENGTH 0x1000
#endif

#define SMALL_BUFFER BYTES_TO_BLOCKS(0x100)
#define FAST_BUFFER BYTES_TO_BLOCKS(ECRYPT_BUFFERLENGTH)

#define TEST_SPEED(max_keys_to_test, LOOP, TEST)                              \
  do {                                                                        \
    TIMER_VARS;                                                               \
    double trials[TRIALS];                                                    \
    int i, j, t;                                                              \
                                                                              \
    for (keys_to_test = 1; ; keys_to_test *= 10)                              \
      {                                                                       \
        /* First test a few times to let data enter the cache */              \
        for(i = j = 0; i < keys_to_test; i++, j++)                            \
          TEST;                                                               \
                                                                              \
        /* And then compute how many tests can be made */                     \
        REPEAT                                                                \
          {                                                                   \
            LOOP_UNTIL_TIMEOUT(TEST, ++i);                                    \
                                                                              \
            if (i * TARGET_TIME / SHORT_TARGET_TIME >= keys_to_test * 100)    \
              break;                                                          \
	  }                                                                   \
                                                                              \
        if (i * TARGET_TIME / SHORT_TARGET_TIME < keys_to_test * 100)         \
          break;                                                              \
                                                                              \
        if (keys_to_test * 10 > max_keys_to_test)                             \
          break;                                                              \
      }                                                                       \
                                                                              \
    tests = 0;                                                                \
                                                                              \
    REPEAT                                                                    \
      {                                                                       \
        LOOP_UNTIL_TIMEOUT(TEST, i += keys_to_test);                          \
                                                                              \
        if ((i = i * TARGET_TIME / USEC(TICKS())) > tests)                    \
          tests = i;                                                          \
      }                                                                       \
                                                                              \
    /* Now test for about TARGET_TIME usec under keys_to_test keys */         \
                                                                              \
    tests_per_key = tests / keys_to_test;                                     \
                                                                              \
    /* truncate to multiples of 100 keys */                                   \
    if (tests_per_key > 500)                                                  \
      tests_per_key = tests_per_key - tests_per_key % 100;                    \
                                                                              \
    /* truncate to multiples of 10 keys */                                    \
    if (tests_per_key > 50)                                                   \
      tests_per_key = tests_per_key - tests_per_key % 10;                     \
                                                                              \
    /* Perform at least one test per key */                                   \
    if (tests_per_key < 1)                                                    \
      tests_per_key = 1;                                                      \
                                                                              \
    tests = tests_per_key * keys_to_test;                                     \
                                                                              \
    REPEAT                                                                    \
      {                                                                       \
        TIMER_START(); LOOP TEST; TIMER_STOP();                               \
        trials[t] = TICKS();                                                  \
      }                                                                       \
                                                                              \
    qsort(trials, TRIALS, sizeof(double), cmp_double);                        \
                                                                              \
    ticks = trials[(TRIALS - 1) / 2];                                         \
    usec = USEC(ticks) / tests;                                               \
                                                                              \
  } while (0)

#define FOR_I_FOR_J                                                           \
  for(i = 0; i < keys_to_test; i++)                                           \
    for(j = 0; j < tests_per_key; j++)

#define FOR_J_FOR_I                                                           \
  for(j = 0; j < tests_per_key; j++)                                          \
    for(i = 0; i < keys_to_test; i++)

#define LOOP_UNTIL_TIMEOUT(TEST, INC)                                         \
  do {                                                                        \
    TIMER_START(); TIMER_STOP();                                              \
                                                                              \
    for(i = 0; USEC(TICKS()) < SHORT_TARGET_TIME; INC)                        \
      {                                                                       \
        for(j = 0; j < keys_to_test; j++)                                     \
	  TEST;                                                               \
                                                                              \
        TIMER_STOP();                                                         \
      }                                                                       \
  } while (0)

#define REPEAT                                                                \
  for(t = 0; t < TRIALS; ++t)

void test_speed(FILE *fd, int keysize, int ivsize, int macsize)
{
  ALIGN(u8, key[MIN_KEYS_TO_TEST], MAXKEYSIZEB);
  ALIGN(u8, iv[MIN_KEYS_TO_TEST], MAXIVSIZEB);
  ALIGN(u8, *text, MAX(SMALL_BUFFER, FAST_BUFFER) * ECRYPT_BLOCKLENGTH);
#ifdef ECRYPT_AE
  ALIGN(u8, mac, MAXMACSIZEB);
#endif

  CTX* ctx;

  int tests, tests_per_key, keys_to_test, ticks; 
  double usec, usec_enc, size;

  static const int sizes[3] = {40, 576, 1500};
  static const double ratios[3] = {7.0, 4.0, 1.0};
  double usecs[3];

  int i, j, k;

  print_primitive(fd, keysize, ivsize, macsize);

  fprintf(fd, "CPU speed: %.1f MHz\n", cpu_speed);
  fprintf(fd, "Cycles are measured using %s\n\n", TIMER_NAME);

  fprintf(fd, "Testing memory requirements:\n\n");
  fflush(fd);

  fprintf(fd, "Size of %s: %d bytes\n\n", QUOTE(CTX), (int)sizeof(CTX));

  text = aligned_malloc(2 * FAST_BUFFER * ECRYPT_BLOCKLENGTH * sizeof(u8));
  ctx = aligned_malloc(MIN_KEYS_TO_TEST * sizeof(CTX));

  for(i = 0; i < MIN_KEYS_TO_TEST; i++)
    {
      for(j = 0; j < MAXKEYSIZEB; j++)
	key[i].b[j] = U8V(rand());

      for(j = 0; j < MAXIVSIZEB; j++)
	iv[i].b[j] = U8V(rand());

      KEYSETUP(&ctx[i], key[i].b, keysize, ivsize, macsize);
      IVSETUP(&ctx[i], iv[i].b);
    }

  for(i = 0; i < FAST_BUFFER * ECRYPT_BLOCKLENGTH; i++)
    text[0].b[i] = U8V(rand());

  fprintf(fd, "Testing stream encryption speed:\n\n");
  fflush(fd);

  TEST_SPEED(MIN_KEYS_TO_TEST, FOR_I_FOR_J, do
    {
      ENCRYPT_BLOCKS(&ctx[i % MIN_KEYS_TO_TEST],
        text[i % 2].b, text[(i + 1) % 2].b, FAST_BUFFER);

    } while (0));

  fprintf(fd, 
    "Encrypted %d blocks of %d bytes (under %d keys, %d blocks/key)\n",
    tests, FAST_BUFFER * ECRYPT_BLOCKLENGTH, keys_to_test, tests_per_key);

  fprintf(fd, "Total time: %d clock ticks (%.2f usec)\n",
    ticks, USEC(ticks));

  usec /= (double)(FAST_BUFFER * ECRYPT_BLOCKLENGTH);
  usec_enc = usec;

  fprintf(fd, "Encryption speed (cycles/byte): %.2f\n", usec * cpu_speed);
  fprintf(fd, "Encryption speed (Mbps): %.2f\n", 8.0 / usec);

  fprintf(fd, "\n");

  for(i = 0; i < MIN_KEYS_TO_TEST; i++)
    FINALIZE(&ctx[i], mac.b);

  if (test_packet)
    {
      fprintf(fd, "Testing packet encryption speed:\n\n");
      fflush(fd);

      for (k = 0; k < 3; k++)
	{
	  TEST_SPEED(MIN_KEYS_TO_TEST, FOR_I_FOR_J, do
	  {
	    ENCRYPT_PACKET(&ctx[i % MIN_KEYS_TO_TEST],
              iv[j % MIN_KEYS_TO_TEST].b, NULL, 0,
              text[i % 2].b, text[(i + 1) % 2].b, sizes[k], mac.b);
	    
	  } while (0));

	  usecs[k] = usec;
	  
	  fprintf(fd, "Encrypted %d packets of %d bytes "
	    "(under %d keys, %d packets/key)\n",
	    tests, sizes[k], keys_to_test, tests_per_key);

	  fprintf(fd, "Total time: %d clock ticks (%.2f usec)\n",
	    ticks, USEC(ticks));

	  fprintf(fd, "Encryption speed (cycles/packet): %.2f\n", 
            usec * cpu_speed);

	  usec /= (double)sizes[k];

	  fprintf(fd, "Encryption speed (cycles/byte): %.2f\n", 
            usec * cpu_speed);
	  fprintf(fd, "Encryption speed (Mbps): %.2f\n", 8.0 / usec);
	  fprintf(fd, "Overhead: %.1f%%\n", 100.0 * (usec / usec_enc - 1.0));
	  
	  fprintf(fd, "\n");
	  fflush(fd);
	}

      fprintf(fd, "Weighted average (Simple Imix):\n");

      usec = size = 0;

      for (k = 0; k < 3; k++)
	{
	  usec += ratios[k] * usecs[k];
	  size += ratios[k] * sizes[k];
	}

      usec /= size;

      fprintf(fd, "Encryption speed (cycles/byte): %.2f\n", usec * cpu_speed);
      fprintf(fd, "Encryption speed (Mbps): %.2f\n", 8.0 / usec);
      fprintf(fd, "Overhead: %.1f%%\n", 100.0 * (usec / usec_enc - 1.0));
  
      fprintf(fd, "\n");
    }

  if (test_setup)
    {
      fprintf(fd, "Testing key setup speed:\n\n");
      fflush(fd);
      
      TEST_SPEED(MIN_KEYS_TO_TEST, FOR_J_FOR_I, do
      {
	KEYSETUP(&ctx[0], key[i % MIN_KEYS_TO_TEST].b, 
          keysize, ivsize, macsize);
	
      } while (0));

      fprintf(fd, 
        "Did %d key setups (under %d keys, %d setups/key)\n",
        tests, keys_to_test, tests_per_key);

      fprintf(fd, "Total time: %d clock ticks (%.2f usec)\n",
        ticks, USEC(ticks));

      fprintf(fd, "Key setup speed (cycles/setup): %.2f\n", usec * cpu_speed);
      fprintf(fd, "Key setup speed (setups/second): %.2f\n", 1000000.0 / usec);

      fprintf(fd, "\n");

#ifndef ECRYPT_AE
      fprintf(fd, "Testing IV setup speed:\n\n");
#else
      fprintf(fd, "Testing speed of IV setup + finalize:\n\n");
#endif
      fflush(fd);

      TEST_SPEED(MIN_KEYS_TO_TEST, FOR_I_FOR_J, do
      {
	IVSETUP(&ctx[i % MIN_KEYS_TO_TEST], iv[j % MIN_KEYS_TO_TEST].b);
	FINALIZE(&ctx[i % MIN_KEYS_TO_TEST], mac.b);
	
      } while (0));

      fprintf(fd, 
        "Did %d IV setups (under %d keys, %d setups/key)\n",
	tests, keys_to_test, tests_per_key);

      fprintf(fd, "Total time: %d clock ticks (%.2f usec)\n",
        ticks, USEC(ticks));

      fprintf(fd, "IV setup speed (cycles/setup): %.2f\n", usec * cpu_speed);
      fprintf(fd, "IV setup speed (setups/second): %.2f\n", 1000000.0 / usec);

      fprintf(fd, "\n");
      fflush(fd);
    }

  if (test_agility)
    {
      int* order;
      int current = 0;

      aligned_free(ctx);
      ctx = aligned_malloc(MAX_KEYS_TO_TEST * sizeof(CTX));
      order = malloc(MAX_KEYS_TO_TEST * sizeof(int));

      for(i = 0; i < MIN_KEYS_TO_TEST; i++)
	{
	  for(j = 0; j < MAXKEYSIZEB; j++)
	    key[0].b[j] = U8V(rand());
	  
	  for(j = 0; j < MAXIVSIZEB; j++)
	    iv[0].b[j] = U8V(rand());
	  
	  KEYSETUP(&ctx[i], key[0].b, keysize, ivsize, macsize);
	  IVSETUP(&ctx[i], iv[0].b);
	}

      for(i = MIN_KEYS_TO_TEST; i < MAX_KEYS_TO_TEST; i++)
	memcpy(&ctx[i], &ctx[i - MIN_KEYS_TO_TEST], sizeof(CTX));

      for(i = 0; i < MAX_KEYS_TO_TEST; i++)
	order[i] = i;

      for(i = 0; i < MAX_KEYS_TO_TEST; i++)
	{
	  const int j = i + (rand() % (MAX_KEYS_TO_TEST - i));
	  const int tmp = order[i];

	  order[i] = order[j];
	  order[j] = tmp;
	}

      fprintf(fd, "Testing key agility:\n\n");
      fflush(fd);
     
      TEST_SPEED(MAX_KEYS_TO_TEST, FOR_J_FOR_I, do
      {
        ENCRYPT_BLOCKS(&ctx[order[(++current) % MAX_KEYS_TO_TEST]],
          text[i % 2].b, text[(i + 1) % 2].b, SMALL_BUFFER);

      } while (0));

      fprintf(fd, 
        "Encrypted %d blocks of %d bytes (each time switching contexts)\n",
	tests, SMALL_BUFFER * ECRYPT_BLOCKLENGTH);

      fprintf(fd, "Total time: %d clock ticks (%.2f usec)\n",
        ticks, USEC(ticks));

      usec /= (double)(SMALL_BUFFER * ECRYPT_BLOCKLENGTH);

      fprintf(fd, "Encryption speed (cycles/byte): %.2f\n", usec * cpu_speed);
      fprintf(fd, "Encryption speed (Mbps): %.2f\n", 8.0 / usec);
      fprintf(fd, "Overhead: %.1f%%\n", 100.0 * (usec / usec_enc - 1.0));
      
      fprintf(fd, "\n");

      for(i = 0; i < MAX_KEYS_TO_TEST; i++)
	FINALIZE(&ctx[i], mac.b);

      free(order);
    }

  fprintf(fd, "\nEnd of performance measurements\n");
  fflush(fd);

  aligned_free(text);
  aligned_free(ctx);
}

/* ------------------------------------------------------------------------- */

void run_tests(FILE *fd, int keysize, int ivsize, int macsize)
{
  if (output_vectors)
    {
      test_if_conform_to_api(fd, keysize, ivsize, macsize);
      
      if (errors == 0)
	test_vectors(fd, keysize, ivsize, macsize);
    }
  
  if (cpu_speed > 0)
    test_speed(fd, keysize, ivsize, macsize); 
}

/* ------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
  int numtests = 0;

  int keytarget[8];
  int ivtarget[8];
  int mactarget[8];

  int keysize[8];
  int ivsize[8];
  int macsize[8];

  int k, i, m, j;
  int s;

  while(argc > 1)
    {
      char* p = *(++argv);
      argc--;

      switch (*(p++))
	{
	case '-':
	  while (*p)
	    switch (*(p++))
	      {
	      case 'v':
		output_vectors = 1;
		break;
	      case 'c':
		if (!(*p) && (argc > 1))
		  {
		    p = *(++argv);
		    argc--;
		  }

		cpu_speed = strtod(p, &p);
		break;
	      case 't':
		if (!(*p) && (argc > 1))
		  {
		    p = *(++argv);
		    argc--;
		  }

		test_time = strtod(p, &p);
		break;
	      case 'p':
		test_packet = 0;
		break;
	      case 'k':
		test_setup = 0;
		break;
	      case 'a':
		test_agility = 0;
		break;
	      case 's':
		if (!(*p) && (argc > 1))
		  {
		    p = *(++argv);
		    argc--;
		  }

		keytarget[numtests] = strtol(p, &p, 0);

		if (!(*p) && (argc > 1))
		  {
		    p = *(++argv);
		    argc--;
		  }

		ivtarget[numtests] = strtol(p, &p, 0);

		if (!(*p) && (argc > 1))
		  {
		    p = *(++argv);
		    argc--;
		  }

		mactarget[numtests] = strtol(p, &p, 0);

		if (numtests + 1 < 8)
		  numtests++;

		break;
	      case 'q':
		quiet = 1;
		break;
	      default:
		fprintf(stderr, "warning: invalid option '%c'\n", p[-1]);
	      }
	  break;
	default:
	  fprintf(stderr, "warning: invalid argument '%s'\n", argv[0]);
	}
    }

  if (!output_vectors && (cpu_speed <= 0))
    {
      fprintf(stderr,
        "Usage: ecrypt-test [OPTIONS]\n"
	"\n"
	"  -v      generate test vectors\n"
	"  -c MHZ  perform speed measurements assuming the given "
	          "clock frequency\n"
	"  -t SEC  limit the duration of the tests (default: 3 seconds)\n"
	"  -p      do not test packet encryption speed\n"
	"  -k      do not test key and IV setup speed\n"
	"  -a      do not test key agility\n"
	"  -s KEY IV MAC\n"
	"          only perform tests for specified key/IV/MAC length\n"
	"          (this option can be specified more than once)\n"
	"  -q      be quiet\n");

      exit(2);
    }

  print_header(stdout);

  ECRYPT_init();

  if (numtests > 0)
    for (j = 0; j < numtests; ++j)
      {
	keysize[j] = ECRYPT_KEYSIZE(0);
	ivsize[j] = ECRYPT_IVSIZE(0);
	macsize[j] = ECRYPT_MACSIZE(0);
      }
  else
    keysize[0] = ivsize[0] = macsize[0] = -1;

  for (k = 0, j = 0; (s = ECRYPT_KEYSIZE(k)) <= ECRYPT_MAXKEYSIZE; k++)
    {
      if ((k > 0) && (s <= ECRYPT_KEYSIZE(k - 1)))
	{
	  ++errors;
	  fprintf(stdout, 
	    "*** ERROR: ECRYPT_KEYSIZE(i) does not conform to API.\n");
	  break;
	}
      
      if (numtests > 0)
	{
	  for (j = 0; j < numtests; ++j)
	    if (abs(s - keytarget[j]) < abs(keysize[j] - keytarget[j]))
	      keysize[j] = s;
	}
      else
	/* Only powers of 2 or multiples of 80 between 64 and 256 */
	if ((s >= 64) && (s <= 256) && (!(s & (s - 1)) || !(s % 80)))
	    {
	      keysize[j] = s;
	      keysize[++j] = -1;
	    }
    }

  for (i = 0, j = 0; (s = ECRYPT_IVSIZE(i)) <= ECRYPT_MAXIVSIZE; i++)
    {
      if ((i > 0) && (s <= ECRYPT_IVSIZE(i - 1)))
	{
	  ++errors;
	  fprintf(stdout, 
	    "*** ERROR: ECRYPT_IVSIZE(i) does not conform to API.\n");
	  break;
	}

      if (numtests > 0)
	{
	  for (j = 0; j < numtests; ++j)
	    if (abs(s - ivtarget[j]) < abs(ivsize[j] - ivtarget[j]))
	      ivsize[j] = s;
	}
      else
	/* Only powers of 2 larger than 32 or multiples of 80 */
	if ((s <= 256) && (((s >= 32) && !(s & (s - 1))) || !(s % 80)))
	    {
	      ivsize[j] = s;
	      ivsize[++j] = -1;
	    }
    }

  for (m = 0, j = 0; (s = ECRYPT_MACSIZE(m)) <= ECRYPT_MAXMACSIZE; m++)
    {
      if ((m > 0) && (s <= ECRYPT_MACSIZE(m - 1)))
	{
	  ++errors;
	  fprintf(stdout, 
	    "*** ERROR: ECRYPT_MACSIZE(i) does not conform to API.\n");
	  break;
	}

      if (numtests > 0)
	{
	  for (j = 0; j < numtests; ++j)
	    if (abs(s - mactarget[j]) < abs(macsize[j] - mactarget[j]))
	      macsize[j] = s;
	}
      else
	/* Only multiples of 32 smaller than 128 */
	if (!(s % 32) && (s <= 128))
	    {
	      macsize[j] = s;
	      macsize[++j] = -1;
	    }
    }

  check_status();

  if (numtests > 0)
    for (j = 0; j < numtests; j++)
      {
	int duplicate = 0;
	
	for (i = 0; i < j; i++)
	  if ((keysize[i] == keysize[j]) &&
	      (ivsize[i] == ivsize[j]) &&
	      (macsize[i] == macsize[j]))
	    {
	      duplicate = 1;
	      break;
	    }

	if (!duplicate)
	  run_tests(stdout, keysize[j], ivsize[j], macsize[j]); 
      }
  else
    {
      if (keysize[0] < 0)
	{
	  keysize[0] = ECRYPT_KEYSIZE(0);
	  keysize[1] = -1;
	}

      if (ivsize[0] < 0)
	{
	  ivsize[0] = ECRYPT_IVSIZE(0);
	  ivsize[1] = -1;
	}

      if (macsize[0] < 0)
	{
	  macsize[0] = ECRYPT_MACSIZE(0);
	  macsize[1] = -1;
	}

      for (k = 0; keysize[k] >= 0; k++)
	for (i = 0; ivsize[i] >= 0; i++)   
	  for (m = 0; macsize[m] >= 0; m++)
	    run_tests(stdout, keysize[k], ivsize[i], macsize[m]); 
    }

  if (!quiet)
    {
      fprintf(stderr, "Elapsed time: %.2f seconds.\n", 
        (double)clock() / (double)CLOCKS_PER_SEC);
      fprintf(stderr, "There were %d errors.\n", errors);
    }

  if (errors)
    return 3;
  else
    return 0;
}
