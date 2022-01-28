// original code from cabd_memory.c of mspack
// the only changes made to lzxd.c and system.h are the #includes from <> to ""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/mspack/system.h"

#include "../libs/mspack/lzx.h"
#include "../libs/mspack/mszip.h"
#include "../libs/mspack/qtm.h"
#include "../libs/mspack/lzss.h"
#include "../libs/mspack/mslzh.h"

struct mem_buf {
  void *data;
  size_t length;
};

struct mem_file {
  unsigned char *data;
  size_t length, posn;
};

static void *mem_alloc(struct mspack_system *this, size_t bytes) {
  return malloc(bytes);
}

static void mem_free(void *buffer) {
  free(buffer);
}

static void mem_copy(void *src, void *dest, size_t bytes) {
  memcpy(dest, src, bytes);
}

static void mem_msg(struct mem_file *file, unsigned char *format, ...) {
}

static struct mem_file *mem_open(struct mspack_system *this,
				 struct mem_buf *fn, int mode)
{
  struct mem_file *fh;
  if (!fn || !fn->data || !fn->length) return NULL;
  if ((fh = mem_alloc(this, sizeof(struct mem_file)))) {
    fh->data   = fn->data;
    fh->length = fn->length;
    fh->posn   = (mode == MSPACK_SYS_OPEN_APPEND) ? fn->length : 0;
  }
  return fh;
}

static void mem_close(struct mem_file *fh) {
  if (fh) mem_free(fh);
}

static int mem_read(struct mem_file *fh, void *buffer, int bytes) {
  int todo;
  if (!fh) return -1;
  todo = fh->length - fh->posn;
  if (todo > bytes) todo = bytes;
  if (todo > 0) mem_copy(&fh->data[fh->posn], buffer, (size_t) todo);
  fh->posn += todo; return todo;
}

static int mem_write(struct mem_file *fh, void *buffer, int bytes) {
  int todo;
  if (!fh) return -1;
  todo = fh->length - fh->posn;
  if (todo > bytes) todo = bytes;
  if (todo > 0) mem_copy(buffer, &fh->data[fh->posn], (size_t) todo);
  fh->posn += todo; return todo;
}

static int mem_seek(struct mem_file *fh, off_t offset, int mode) {
  if (!fh) return 1;
  switch (mode) {
  case MSPACK_SYS_SEEK_START: break;
  case MSPACK_SYS_SEEK_CUR:   offset += (off_t) fh->posn; break;
  case MSPACK_SYS_SEEK_END:   offset += (off_t) fh->length; break;
  default: return 1;
  }
  if ((offset < 0) || (offset > (off_t) fh->length)) return 1;
  fh->posn = (size_t) offset;
  return 0;
}

static off_t mem_tell(struct mem_file *fh) {
  return (fh) ? (off_t) fh->posn : -1;
}

static struct mspack_system mem_system = {
  (struct mspack_file * (*)()) &mem_open,
  (void (*)())  &mem_close,
  (int (*)())   &mem_read, 
  (int (*)())   &mem_write,
  (int (*)())   &mem_seek, 
  (off_t (*)()) &mem_tell,
  (void (*)())  &mem_msg,
  &mem_alloc,
  &mem_free,
  &mem_copy,
  NULL
};

int unmspack(unsigned char *in, int insz, unsigned char *out, int outsz, int window_bits, int interval_mode, int reset_interval, int algo) {
    struct lzxd_stream      *lzx_state = NULL;
    struct mszipd_stream    *mszip_state = NULL;
    struct qtmd_stream      *qtm_state = NULL;
    struct mslzhd_stream    *mslzh_state = NULL;
    struct mspack_file *fd = NULL,
                       *fdo = NULL;
    struct mem_buf source = { in, insz };
    struct mem_buf dest   = { out, outsz };
    int     err,
            ret    = -1;

    fd = mem_system.open(&mem_system, (unsigned char *)&source, 0);
    if(!fd) goto quit;
    fdo = mem_system.open(&mem_system, (unsigned char *)&dest, 0);
    if(!fdo) goto quit;

    if(reset_interval < 0) reset_interval = 4096;
    if(algo < 0) algo = 0;

    if(algo == 0) {
        // LZX
        lzx_state = lzxd_init(&mem_system, fd, fdo, window_bits, interval_mode, reset_interval, 0, 0);
        if(!lzx_state) goto quit;
        err = lzxd_decompress(lzx_state, outsz);

    } else if(algo == 1) {
        // MSZIP
        mszip_state = mszipd_init(&mem_system, fd, fdo, reset_interval, 1);
        if(!mszip_state) goto quit;
        err = mszipd_decompress(mszip_state, outsz);

    } else if(algo == 2) {
        // QTM
        qtm_state = qtmd_init(&mem_system, fd, fdo, window_bits, reset_interval);
        if(!qtm_state) goto quit;
        err = qtmd_decompress(qtm_state, outsz);

    } else if(algo == 3) {
        // LZSS
        err = lzss_decompress(&mem_system, fd, fdo, reset_interval, interval_mode);

    } else if(algo == 4) {
        // KWAJ/LZH
        mslzh_state = mslzh_init(&mem_system, fd, fdo);
        if(!mslzh_state) goto quit;
        err = mslzh_decompress(mslzh_state);

    } else if(algo == 5) {
        // LZX delta
        lzx_state = lzxd_init(&mem_system, fd, fdo, window_bits, interval_mode, reset_interval, 0, 1);
        if(!lzx_state) goto quit;
        err = lzxd_decompress(lzx_state, outsz);

    } else {
        goto quit;
    }

    if(err != MSPACK_ERR_OK) {
        //goto quit;   // not checked, I guess I did some tests years ago
    }

    ret = mem_system.tell(fdo);

quit:
    if(fd)  mem_system.close(fd);
    if(fdo) mem_system.close(fdo);
    if(lzx_state)   lzxd_free(lzx_state);
    if(mszip_state) mszipd_free(mszip_state);
    if(qtm_state)   qtmd_free(qtm_state);
    if(mslzh_state) mslzh_free(mslzh_state);
    return(ret);
}



/**/



// https://github.com/gildor2/UModel/blob/master/Unreal/UnCoreCompression.cpp
typedef unsigned char byte;
struct appDecompressLZX_file
{
	byte*		buf;
	int			bufSize;
	int			pos;
	int			rest;
};

static int appDecompressLZX_read(struct appDecompressLZX_file *file, void *buffer, int bytes)
{
	//guard(mspack_read);

	if (!file->rest)
	{
		// read block header
		if (file->buf[file->pos] == 0xFF)
		{
			// [0]   = FF
			// [1,2] = uncompressed block size
			// [3,4] = compressed block size
			file->rest = (file->buf[file->pos+3] << 8) | file->buf[file->pos+4];
			file->pos += 5;
		}
		else
		{
			// [0,1] = compressed size
			file->rest = (file->buf[file->pos+0] << 8) | file->buf[file->pos+1];
			file->pos += 2;
		}
		if (file->rest > file->bufSize - file->pos)
			file->rest = file->bufSize - file->pos;
	}
	if (bytes > file->rest) bytes = file->rest;
	if (!bytes) return 0;

	// copy block data
	memcpy(buffer, file->buf + file->pos, bytes);
	file->pos  += bytes;
	file->rest -= bytes;

	return bytes;
	//unguard;
}

static int appDecompressLZX_write(struct appDecompressLZX_file *file, void *buffer, int bytes)
{
	//guard(mspack_write);
	//assert(file->pos + bytes <= file->bufSize);
	memcpy(file->buf + file->pos, buffer, bytes);
	file->pos += bytes;
	return bytes;
	//unguard;
}

static void *appDecompressLZX_alloc(struct mspack_system *self, size_t bytes)
{
	return /*appMalloc*/malloc(bytes);
}

static void appDecompressLZX_free(void *ptr)
{
	/*appFree*/free(ptr);
}

static void appDecompressLZX_copy(void *src, void *dst, size_t bytes)
{
	memcpy(dst, src, bytes);
}

static struct mspack_system lzxSys =
{
	NULL,				// open
	NULL,				// close
	(int (*)())&appDecompressLZX_read,
	(int (*)())&appDecompressLZX_write,
	NULL,				// seek
	NULL,				// tell
	NULL,				// message
	&appDecompressLZX_alloc,
	&appDecompressLZX_free,
	&appDecompressLZX_copy
};

/*static*/ int appDecompressLZX(byte *CompressedBuffer, int CompressedSize, byte *UncompressedBuffer, int UncompressedSize, int WindowSize, int CompressionPartitionSize)
{
	//guard(appDecompressLZX);

	// setup streams
	struct appDecompressLZX_file src, dst;
	src.buf     = CompressedBuffer;
	src.bufSize = CompressedSize;
	src.pos     = 0;
	src.rest    = 0;
	dst.buf     = UncompressedBuffer;
	dst.bufSize = UncompressedSize;
	dst.pos     = 0;
	// prepare decompressor
    if(WindowSize >= 32) {  // max is 21 or 25 (delta)
        int t = WindowSize;
        for(WindowSize = 0; t >>= 1; WindowSize++);
    }
    if(WindowSize <= 0) WindowSize = 17;
    if(CompressionPartitionSize <= 0) CompressionPartitionSize = 256*1024;
	struct lzxd_stream *lzxd = lzxd_init(&lzxSys, (void *)&src, (void *)&dst, WindowSize, 0, CompressionPartitionSize, UncompressedSize, 0);
	//assert(lzxd);
	// decompress
	int r = lzxd_decompress(lzxd, UncompressedSize);
	if (r != MSPACK_ERR_OK)
		return -1; //appError("lzxd_decompress(%d,%d) returned %d", CompressedSize, UncompressedSize, r);
    int ret = mem_system.tell((void *)&src);
	// free resources
	lzxd_free(lzxd);

	//unguard;
    return ret;
}


