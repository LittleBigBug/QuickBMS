/* SR3C, a symbol ranking data compressor.
 *
 * This file implements a fast and effective data compressor in a form
 * easily embeddable into C programs.  The compression is slightly
 * faster to gzip -7, but results in ~11% smaller data.  bzip2 -2
 * compresses slightly better than SR3C, but takes almost three times
 * as long.  Furthermore, since bzip2 is based on Burrows-Wheeler
 * block sorting, it can't be used in on-line compression tasks.
 * Memory consumption of SR3C is currently around 4.5 MB per ongoing
 * compression and decompression.
 *
 * Author: Kenneth Oksanen <cessu@iki.fi>, 2008.
 *
 * This code borrows many ideas and some paragraphs of comments from
 * Matt Mahoney's s symbol ranking compression program SR2 and Peter
 * Fenwicks SRANK, but otherwise all code has been implemented from
 * scratch.
 *
 * This file is distributed under the following license:

The MIT License
Copyright (c) 2008 Helsinki University of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */


#ifndef INCL_SR3C
#define INCL_SR3C  1


/* Prior to compression or uncompression the user of this library
   creates a "compression context" of type 'sr3c_context_t' which can
   with certain disciplines be used for both compression and
   uncompression.  The compression context initialization is given a
   function 'output_f' which the compression and uncompression
   routines call for processing the data further.  The compression
   context initialization is also passed a "output context", of type
   'void *', behind which the user of this library can store
   information passed from and to the 'output_f' without resort to
   global variables.

   This library is MT-safe so that any number of concurrent
   compression or uncompression tasks may be ongoing as long as each
   of them is passed a distinct compression context.  By default each
   compression context requires approximately 4.5 MB of memory, but
   see the internal documentation of sr3c.c for further notes on this.

   Compression tasks are generally NOT synchronous in the sense that
   once some data is passed to 'sr3c_compress' the 'output_f's would
   be passed enough compressed data so as to uncompress all of the
   same original data.  When synchronization is required, for example
   at the end of successful compression, the user of this library
   should call 'sr3c_flush'.  The decompressing side will recognize
   the flush from the compressed data.  Only when a compression and
   decompression have flushed in this manner, can the corresponding
   compression contexts be used in reverse direction.
 */


/* The type of the function used to process the compressed or
   uncompressed data further.  The function should return a negative
   value on failure.  The function may not free the memory referred to
   by the 'bytes' argument.  The argument 'flush' is passed a non-zero
   value iff sr3c_flush was called after compressing the data.
   'output_ctx' can be defined in the code using this library to
   contain various data (such as output file descriptors) which
   otherwise would have to be passed in MT-unsafe global variables.
 */
typedef int (*sr3c_output_f_t)(const unsigned char *bytes, size_t n_bytes,
			       int flush, void *output_ctx);


typedef struct sr3c_context_t sr3c_context_t;


/* Reinitialize the given compression context.  If 'output_f' is
   non-NULL, it is assigned to the compression context's new output
   function and 'output_ctx' to its new callback argument.  If, on the
   other hand, output_f is NULL the callback function and context will
   remain as they are.

   This function should be MT-safe if your hardware supports
   simultaneous initialization of some global data with identical
   values - AFAIK this holds everywhere.
 */
void sr3c_reset(sr3c_context_t *context,
		sr3c_output_f_t output_f, void *output_ctx);


/* Allocate and initialize a new compression context.

   This function is MT-safe with the same provisions as sr3c_reset.
 */ 
sr3c_context_t *sr3c_alloc(sr3c_output_f_t output_f, void *output_ctx);


/* Dispose from memory the given compression context.  This does not
   flush any buffered compressed data to 'output_f' - in fact this
   function is guaranteed to not call output_f so as to allow for
   cleaning up memory when something has gone awry.  Neither does this
   function free 'output_ctx'.

   This function is MT-safe.  However, any ctx may be passed to
   sr3c_free only once.
 */
void sr3c_free(sr3c_context_t *ctx);


/* Compress the given bytes, possibly sending some compressed data to
   'output_f'.  Returns zero on success, or whatever 'output_f' returned
   should it have erred.  Once the context has been used for
   compressing, it can't be used for uncompressing before src3_flush
   has been called.

   This function is MT-safe in the sense that simultaneous calls for
   different contexts are allowed and their corresponding output_f's
   can be called MT-safely.
 */
int sr3c_compress(const unsigned char *bytes, size_t n_bytes,
		  sr3c_context_t *context);


/* Uncompress the given bytes, possibly sending some uncompressed data
   to 'output_f'.  Returns zero on success, or whatever 'output_f'
   returned should it have failed.  SR3C includes no checksum so
   corrupted compressed messages will not be detected.  Once the
   context has been used for uncompressing, it can't be used for
   compressing before src3_flush has been issued on the corresponding
   compressing side and the resulting compressed data has been
   uncompressed.

   This function is MT-safe in the sense that simultaneous calls for
   different contexts are allowed and their corresponding output_f's
   can be called MT-safely.
 */
int sr3c_uncompress(const unsigned char *bytes, size_t n_bytes,
		    sr3c_context_t *context);


/* Flush the internal buffered state of the compression context to
   'output_f' so that a uncompression of all the data provided prior
   to the flush becomes possible.  Returns zero on success, or
   whatever 'output_f' returned should it have erred.  It is possible
   to continue compression after flushing the buffers, but each flush
   will cause at least a three byte overhead, probably higher.
   File compressors and archivers typically flush only at the end of
   compression, but message-interchanging programs flush whenever
   real-timeness requires or a response is required to proceed.

   Note that sr3c_flush may not be called for a context currently used
   for uncompression.
 */
int sr3c_flush(sr3c_context_t *context);


/* Return non-zero if the given context is flushed, i.e. if the
   context can now be used for either compression or uncompression.
 */
int sr3c_is_flushed(sr3c_context_t *context);


#endif
