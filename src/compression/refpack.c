// modified by Luigi Auriemma
// http://wiki.niotso.org/RefPack

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct scanner {
	uint8_t *start;
	uint8_t *ptr;
	uint8_t *end;
	uint8_t overflow;
};
 
static __inline size_t position(const struct scanner *ctx)
{
	return (size_t)(ctx->ptr - ctx->start);
}
 
static __inline size_t remaining(const struct scanner *ctx)
{
	return (size_t)(ctx->end - ctx->ptr);
}
 
static __inline uint8_t overflowed(const struct scanner *ctx)
{
	return ctx->overflow;
}
 
static __inline void scanner_init(struct scanner *ctx, uint8_t *start,
	size_t size)
{
	ctx->start = ctx->ptr = start;
	ctx->end = start + size;
}
 
static uint8_t read_u8(struct scanner *ctx)
{
	if (ctx->end == ctx->ptr) {
		ctx->overflow = 1;
		return 0;
	}
	return *(ctx->ptr++);
}
 
static uint16_t read_u16(struct scanner *ctx)
{
	uint16_t x;
	if (remaining(ctx) < 2) {
		ctx->ptr = ctx->end, ctx->overflow = 1;
		return 0;
	}
	x = (ctx->ptr[0] << 8) | ctx->ptr[1];
	ctx->ptr += 2;
	return x;
}
 
static uint32_t read_u24(struct scanner *ctx)
{
	uint32_t x;
	if (remaining(ctx) < 3) {
		ctx->ptr = ctx->end, ctx->overflow = 1;
		return 0;
	}
	x = (ctx->ptr[0] << 16) | (ctx->ptr[1] << 8) | ctx->ptr[2];
	ctx->ptr += 3;
	return x;
}

static uint32_t read_u32(struct scanner *ctx)
{
	uint32_t x;
	if (remaining(ctx) < 4) {
		ctx->ptr = ctx->end, ctx->overflow = 1;
		return 0;
	}
	x = (ctx->ptr[0] << 24) | (ctx->ptr[1] << 16) | (ctx->ptr[2] << 8) | ctx->ptr[3];
	ctx->ptr += 4;
	return x;
}
 
static void append(struct scanner *out, struct scanner *in, size_t length)
{
	if (!length)
		return;
    // do NOT enable the following!!!
	//else if (length > 4)
		//length = 4;
 
	if (remaining(in) < length || remaining(out) < length) {
		if (remaining(in) < length)
			in->ptr = in->end, in->overflow = 1;
		if (remaining(out) < length)
			out->ptr = out->end, out->overflow = 1;
		return;
	}
 
	memcpy(out->ptr, in->ptr, length);
	out->ptr += length, in->ptr += length;
}
 
static void self_copy(struct scanner *ctx, size_t distance, size_t length)
{
	size_t i;
	uint8_t *in_ptr, *out_ptr;
 
	if (position(ctx) < distance || remaining(ctx) < length) {
		ctx->ptr = ctx->end, ctx->overflow = 1;
		return;
	}
 
	in_ptr = ctx->ptr - distance, out_ptr = ctx->ptr;
	/* neither memcpy nor memmove implement an LZ77 overlapping copy; a
	** one-byte-at-a-time copy is the most efficient thing which does what
	** we want */
	for (i = 0; i < length; i++)
		*out_ptr++ = *in_ptr++;
	ctx->ptr += length;
}
 
/**
 * @brief Decompress a RefPack bitstream
 * @param indata - Pointer to the input RefPack bitstream
 * @param insize - Size of RefPack bitstream
 * @param bytes_read_out - (optional) Pointer to a size_t which will be filled
 *	with the total number of bytes read from the RefPack bitstream; may be
 *	NULL
 * @param outdata - Pointer to the output buffer which will be filled with the
 *	decompressed data
 * @param outsize - Size of output buffer
 * @param bytes_written_out - (optional) Pointer to a size_t which will be
 *	filled with the total number of bytes written to the output buffer; may
 *	be NULL
 * @param compressed_size_out - (optional) Pointer to a uint32_t which will be
 *	set to the "compressed size" field in the RefPack bitstream
 * @param decompressed_size_out - (optional) Pointer to a uint32_t which will be
 *	set to the "decompressed size" field in the RefPack bitstream
 * @return 0 on success; -1 on read overflow of the input buffer; and -2 on
 *	read/write overflow of the output buffer. As an input overflow can
 *	*cause* an output overflow, an input overflow is considered more
 *	significant by this function and takes precedence in the return value;
 *	thus, if both an input overflow and an output overflow occur, this
 *	function returns -1.
 *
 * This is a safe reimplementation of refpack_decompress_unsafe.
 */
int refpack_decompress_safe(const uint8_t *indata, size_t insize,
	size_t *bytes_read_out, uint8_t *outdata, size_t outsize,
	size_t *bytes_written_out, uint32_t *compressed_size_out,
	uint32_t *decompressed_size_out
    , int skip_header)
{
	struct scanner in, out;
	uint16_t signature;
	uint32_t compressed_size, decompressed_size;
	uint8_t byte_0, byte_1, byte_2, byte_3;
	uint32_t proc_len, ref_dis, ref_len;
 
    memset(&in, 0, sizeof(in));
	scanner_init(&in, (uint8_t*)indata, insize);
    memset(&out, 0, sizeof(out));
	scanner_init(&out, outdata, outsize);
 
    uint32_t (*myread)(struct scanner *ctx) = read_u24;

    if(skip_header) {
        signature = 0x10fb;
        compressed_size = insize;
        decompressed_size = outsize;
    } else {

        // Note that "15 FB" is NOT implemented yet!!!
        // this is just a placeholder

        signature = read_u16(&in);
        if(signature & 0x0400) {    // 15 fb 32bit_size without zsize, so doesn't matter if 0x0100
            myread = read_u32;
            compressed_size = 0;
        } else {                    // 11 fb 32bit 32bit ?
            compressed_size = (signature & 0x0100) ? myread(&in) : 0;
        }
        if (compressed_size_out)
            *compressed_size_out = compressed_size;
     
        decompressed_size = myread(&in);
        if (decompressed_size_out)
            *decompressed_size_out = decompressed_size;
    }

	while (!overflowed(&in) && !overflowed(&out)) {
		byte_0 = read_u8(&in);
		if (!(byte_0 & 0x80)) {
			/* 2-byte command: 0DDRRRPP DDDDDDDD */
			byte_1 = read_u8(&in);
 
			proc_len = byte_0 & 0x03;
			append(&out, &in, proc_len);
 
			ref_dis = ((byte_0 & 0x60) << 3) + byte_1 + 1;
			ref_len = ((byte_0 >> 2) & 0x07) + 3;
			self_copy(&out, ref_dis, ref_len);
		} else if(!(byte_0 & 0x40)) {
			/* 3-byte command: 10RRRRRR PPDDDDDD DDDDDDDD */
			byte_1 = read_u8(&in);
			byte_2 = read_u8(&in);
 
			proc_len = byte_1 >> 6;
			append(&out, &in, proc_len);
 
			ref_dis = ((byte_1 & 0x3f) << 8) + byte_2 + 1;
			ref_len = (byte_0 & 0x3f) + 4;
			self_copy(&out, ref_dis, ref_len);
		} else if(!(byte_0 & 0x20)) {
			/* 4-byte command: 110DRRPP DDDDDDDD DDDDDDDD RRRRRRRR*/
			byte_1 = read_u8(&in);
			byte_2 = read_u8(&in);
			byte_3 = read_u8(&in);
 
			proc_len = byte_0 & 0x03;
			append(&out, &in, proc_len);
 
			ref_dis = ((byte_0 & 0x10) << 12)
				+ (byte_1 << 8) + byte_2 + 1;
			ref_len = ((byte_0 & 0x0c) << 6) + byte_3 + 5;
			self_copy(&out, ref_dis, ref_len);
		} else {
			/* 1-byte command */
			proc_len = (byte_0 & 0x1f) * 4 + 4;
			if (proc_len <= 0x70) {
				/* no stop flag */
				append(&out, &in, proc_len);
			} else {
				/* stop flag */
				proc_len = byte_0 & 0x3;
				append(&out, &in, proc_len);
 
				break;
			}
		}
	}
 
	if (bytes_read_out)
		*bytes_read_out = position(&in);
	if (bytes_written_out)
		*bytes_written_out = position(&out);
 
	if (overflowed(&in))
		return -1;
	else if (overflowed(&out))
		return -2;
	else
		return 0;
}
