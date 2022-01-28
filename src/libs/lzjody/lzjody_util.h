/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#ifndef LZJODY_UTIL_H
#define LZJODY_UTIL_H

#include <lzjody.h>

#define LZJODY_UTIL_VER "0.1"
#define LZJODY_UTIL_VERDATE "2014-12-29"

/* Debugging stuff */
#ifndef DLOG
 #ifdef DEBUG
  #define DLOG(...) fprintf(stderr, __VA_ARGS__)
 #else
  #define DLOG(...)
 #endif
#endif

/* Use POSIX threads in compression utility  (define this in Makefile) */
/* #define THREADED 1 */

struct files_t {
	FILE *in;
	FILE *out;
};

/* Number of LZJODY_BSIZE blocks to process per thread */
#define CHUNK 1024

#ifdef THREADED
/* Per-thread working state */
struct thread_info {
	unsigned char blk[LZJODY_BSIZE * CHUNK];	/* Thread input blocks */
	unsigned char out[(LZJODY_BSIZE + 4) * CHUNK];	/* Thread output blocks */
	char options;	/* Compressor options */
	pthread_t id;	/* Thread ID */
	int block;	/* What block is thread working on? */
	int length;	/* Total bytes in block */
	int o_length;	/* Output length */
	int working;	/* Is thread working (1) or idle (0)? */
};
#endif /* THREADED */

#endif	/* LZJODY_UTIL_H */
