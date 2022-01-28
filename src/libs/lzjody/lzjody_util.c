/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "lzjody.h"
#include "lzjody_util.h"

/* Detect Windows and modify as needed */
#if defined _WIN32 || defined __CYGWIN__
 #define ON_WINDOWS 1
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <io.h>
#endif

#ifdef THREADED
#include <pthread.h>
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;	/* pthreads change condition */
static int thread_error;	/* nonzero if any thread fails */
#endif

/* Debugging stuff */
#ifndef DLOG
 #ifdef DEBUG
  #define DLOG(...) fprintf(stderr, __VA_ARGS__)
 #else
  #define DLOG(...)
 #endif
#endif

struct files_t files;

#ifdef THREADED
static void *compress_thread(void *arg)
{
	struct thread_info * const thr = arg;
	const unsigned char *ipos = thr->blk;	/* Uncompressed input pointer */
	unsigned char *opos = thr->out;	/* Compressed output pointer */
	int i;
	int bsize = LZJODY_BSIZE;	/* Compressor block size */
	int remain = thr->length;	/* Remaining input bytes */

	while (remain) {
		if (remain < LZJODY_BSIZE) bsize = remain;
		i = lzjody_compress(ipos, opos, thr->options, bsize);
		if (i < 0) {
			thread_error = 1;
			pthread_cond_signal(&cond);
		}
		ipos += bsize;
		opos += i;
		remain -= bsize;
	}
	thr->o_length = opos - thr->out;
	thr->working = 0;
	pthread_cond_signal(&cond);

	return 0;
}
#endif /* THREADED */

int main(int argc, char **argv)
{
	static unsigned char blk[LZJODY_BSIZE];
	static unsigned char out[LZJODY_BSIZE + 4];
	int i;
	int length = 0;	/* Incoming data block length counter */
	int c_length;   /* Compressed block length temp variable */
	int blocknum = 0;	/* Current block number */
	unsigned char options = 0;	/* Compressor options */
#ifdef THREADED
	struct thread_info *thr;
	int nprocs = 1;		/* Number of processors */
	int eof = 0;	/* End of file? */
	char running = 0;	/* Number of threads running */
#endif /* THREADED */

	if (argc < 2) goto usage;

	/* Windows requires that data streams be put into binary mode */
#ifdef ON_WINDOWS
	setmode(STDIN_FILENO, _O_BINARY);
	setmode(STDOUT_FILENO, _O_BINARY);
#endif /* ON_WINDOWS */

	files.in = stdin;
	files.out = stdout;

	if (!strncmp(argv[1], "-c", 2)) {
#ifndef THREADED
		/* Non-threaded compression */
		/* fprintf(stderr, "blk %p, blkend %p, files %p\n",
				blk, blk + LZJODY_BSIZE - 1, files); */
		while((length = fread(blk, 1, LZJODY_BSIZE, files.in))) {
			if (ferror(files.in)) goto error_read;
			DLOG("\n--- Compressing block %d\n", blocknum);
			i = lzjody_compress(blk, out, options, length);
			if (i < 0) goto error_compression;
			DLOG("c_size %d bytes\n", i);
			i = fwrite(out, i, 1, files.out);
			if (!i) goto error_write;
			blocknum++;
		}

#else /* Using POSIX threads */

 #ifdef _SC_NPROCESSORS_ONLN
		/* Get number of online processors for pthreads */
		nprocs = (int)sysconf(_SC_NPROCESSORS_ONLN);
		if (nprocs < 1) {
			fprintf(stderr, "warning: system returned bad number of processors: %d\n", nprocs);
			nprocs = 1;
		}
 #endif /* _SC_NPROCESSORS_ONLN */
		/* Run two threads per processor */
		nprocs <<= 1;
		fprintf(stderr, "lzjody: compressing with %d worker threads\n", nprocs);

		/* Allocate per-thread input/output memory and control blocks */
		thr = (struct thread_info *)calloc(nprocs, sizeof(struct thread_info));
		if (!thr) goto oom;

		/* Set compressor options */
		for (i = 0; i < nprocs; i++) (thr + i)->options = options;

		thread_error = 0;
		while (1) {
			struct thread_info *cur = NULL;
			uint32_t min_blk;	/* Minimum block number */
			unsigned int min_thread;	/* Thread for min_blk */
			int thread;	/* Temporary thread scan counter */
			int open_thr;	/* Next open thread */

			/* See if lowest block number is finished */
			while (1) {
				min_blk = 0xffffffff;
				min_thread = 0;
				/* Scan threads for smallest block number */
				pthread_mutex_lock(&mtx);
				for (thread = 0; thread < nprocs; thread++) {
					unsigned int j;

				fprintf(stderr, ":thr %p, thread %d\n",
						(void *)thr, thread);
					if (thread_error != 0) goto error_compression;
					j = (thr + thread)->block;
					if (j > 0 && j < min_blk) {
						min_blk = j;
						min_thread = thread;
				fprintf(stderr, ":j%d:%d thr %p, cur %p, min_thread %d\n",
						j, min_blk, (void *)thr, (void *)cur, min_thread);
					}
				}
				pthread_mutex_unlock(&mtx);

				cur = thr + min_thread;
				fprintf(stderr, "thr %p, cur %p, min_thread %d\n",
						(void *)thr, (void *)cur, min_thread);
				if (cur->working == 0 && cur->length > 0) {
					pthread_detach(cur->id);
					/* flush finished block */
					i = fwrite(cur->out, cur->o_length, 1, files.out);
					if (!i) goto error_write;
					cur->block = 0;
					cur->length = 0;
					DLOG("Thread %d done\n", min_thread);
					running--;
				} else break;
			}

			/* Terminate when all blocks are written */
			if (eof && (running == 0)) break;

			/* Start threads */
			if (running < nprocs) {
				/* Don't read any more if EOF reached */
				if (!eof) {
					/* Find next open thread */
					cur = thr;
					for (open_thr = 0; open_thr < nprocs; open_thr++) {
						if (cur->working == 0 && cur->block == 0) break;
						cur++;
					}

					/* If no threads are available, wait for one */
					if (open_thr == nprocs) {
						pthread_mutex_lock(&mtx);
						pthread_cond_wait(&cond, &mtx);
						pthread_mutex_unlock(&mtx);
						continue;
					}

					/* Read next block */
					length = fread(cur->blk, 1, (LZJODY_BSIZE * CHUNK), files.in);
					if (ferror(files.in)) goto error_read;
					if (length < (LZJODY_BSIZE * CHUNK)) eof = 1;
					if (length > 0) {
						blocknum++;

						/* Set up thread */
						cur->working = 1;
						cur->block = blocknum;
						cur->length = length;
						cur->o_length = 0;
						running++;
						DLOG("Thread %d start\n", open_thr);

						/* Start thread */
						pthread_create(&(cur->id), NULL,
								compress_thread,
								(void *)cur);
					} else eof = 1;
				} else if (running > 0) {
					/* EOF but threads still running */
					pthread_mutex_lock(&mtx);
					pthread_cond_wait(&cond, &mtx);
					pthread_mutex_unlock(&mtx);
				}
			}
		}
		free(thr);
#endif /* THREADED */
	}

	/* Decompress */
	if (!strncmp(argv[1], "-d", 2)) {
		while(fread(blk, 1, 2, files.in)) {
			/* Get block-level decompression options */
			options = *blk & 0xc0;

			/* Read the length of the compressed data */
			length = *(blk + 1);
			length |= ((*blk & 0x1f) << 8);
			if (length > (LZJODY_BSIZE + 4)) goto error_blocksize_d_prefix;

			i = fread(blk, 1, length, files.in);
			if (ferror(files.in)) goto error_read;
			if (i != length) goto error_shortread;

			if (options & O_NOCOMPRESS) {
				c_length = *(blk + 1);
				c_length |= ((*blk & 0x1f) << 8);
				DLOG("--- Writing uncompressed block %d (%d bytes)\n", blocknum, c_length);
				if (c_length > LZJODY_BSIZE) goto error_unc_length;
				i = fwrite((blk + 2), 1, c_length, files.out);
				if (i != c_length) {
					length = c_length;
					goto error_write;
				}
			} else {
				DLOG("--- Decompressing block %d\n", blocknum);
				length = lzjody_decompress(blk, out, i, options);
				if (length < 0) goto error_decompress;
				if (length > LZJODY_BSIZE) goto error_blocksize_decomp;
				i = fwrite(out, 1, length, files.out);
				if (i != length) goto error_write;
 /*			     DLOG("Wrote %d bytes\n", i); */
			}


			blocknum++;
		}
	}

	exit(EXIT_SUCCESS);

error_compression:
	fprintf(stderr, "Fatal error during compression, aborting.\n");
	exit(EXIT_FAILURE);
error_read:
	fprintf(stderr, "Error reading file %s\n", "stdin");
	exit(EXIT_FAILURE);
error_write:
	fprintf(stderr, "Error writing file %s (%d of %d written)\n", "stdout",
			i, length);
	exit(EXIT_FAILURE);
error_shortread:
	fprintf(stderr, "Error: short read: %d < %d (eof %d, error %d)\n",
			i, length, feof(files.in), ferror(files.in));
	exit(EXIT_FAILURE);
error_unc_length:
	fprintf(stderr, "Error: uncompressed length too large (%d > %d)\n",
			c_length, LZJODY_BSIZE);
	exit(EXIT_FAILURE);
error_blocksize_d_prefix:
	fprintf(stderr, "Error: decompressor prefix too large (%d > %d)\n",
			length, (LZJODY_BSIZE + 4));
	exit(EXIT_FAILURE);
error_blocksize_decomp:
	fprintf(stderr, "Error: decompressor overflow (%d > %d)\n",
			length, LZJODY_BSIZE);
	exit(EXIT_FAILURE);
error_decompress:
	fprintf(stderr, "Error: cannot decompress block %d\n", blocknum);
	exit(EXIT_FAILURE);
#ifdef THREADED
oom:
	fprintf(stderr, "Error: out of memory\n");
	exit(EXIT_FAILURE);
#endif
usage:
	fprintf(stderr, "lzjody %s, a compression utility by Jody Bruchon (%s)\n",
			LZJODY_UTIL_VER, LZJODY_UTIL_VERDATE);
	fprintf(stderr, "\nlzjody -c   compress stdin to stdout\n");
	fprintf(stderr, "\nlzjody -d   decompress stdin to stdout\n");
	exit(EXIT_FAILURE);
}
