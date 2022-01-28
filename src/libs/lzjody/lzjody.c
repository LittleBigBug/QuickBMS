/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014, 2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#include <stdio.h>
#include <stdint.h>
#include "byteplane_xfrm.h"
#include "lzjody.h"

/* Debugging stuff */
#ifndef DLOG
 #ifdef DEBUG
  #define DLOG(...) fprintf(stderr, __VA_ARGS__)
 #else
  #define DLOG(...)
 #endif
#endif

/* Top 3 bits of a control byte */
#define P_SHORT	0x80	/* Compact control byte form */
#define P_LZ	0x60	/* LZ (dictionary) compression */
#define P_RLE	0x40	/* RLE compression */
#define P_LIT	0x20	/* Literal values */
#define P_LZL	0x10	/* LZ match flag: size > 255 */
#define P_EXT	0x00	/* Extended algorithms (ignore 0x10 and P_SHORT) */
#define P_PLANE 0x04	/* Byte plane transform */
#define P_SEQ32	0x03	/* Sequential 32-bit values */
#define P_SEQ16	0x02	/* Sequential 16-bit values */
#define P_SEQ8	0x01	/* Sequential 8-bit values */

/* Control bits masking value */
#define P_MASK	0x60	/* LZ, RLE, literal (no short) */
#define P_XMASK 0x0f	/* Extended command */
#define P_SMASK 0x03	/* Sequence compression commands */

/* Maximum length of a short element */
#define P_SHORT_MAX 0x0f
#define P_SHORT_XMAX 0xff

/* Minimum sizes for compression
 * These sizes are roughly calculated as follows:
 * control byte(s) + data byte(s) + other control byte(s)
 * This avoids data expansion cause by interrupting a stream of literals
 * (which triggers up to 2 more control bytes). Algorithms adjust these if
 * the literal count to flush is going to trigger an additional control byte.
 *
 * WARNING: Changing these values too low will cause the compression
 * algorithms to expand data and fail in some cases!
 */
#define MIN_LZ_MATCH 3
#define MAX_LZ_MATCH 4095
#define MIN_RLE_LENGTH 3
/* Sequence lengths are not byte counts, they are word counts! */
#define MIN_SEQ32_LENGTH 2
#define MIN_SEQ16_LENGTH 3
#define MIN_SEQ8_LENGTH 4
#define MIN_PLANE_LENGTH 8

/* If a byte occurs more times than this in a block, use linear scanning */
#ifndef MAX_LZ_BYTE_SCANS
 #define MAX_LZ_BYTE_SCANS 0x800
#endif

struct comp_data_t {
	const unsigned char *in;
	unsigned char *out;
	unsigned int ipos;
	unsigned int opos;
	unsigned int literals;
	unsigned int literal_start;
	unsigned int length;	/* Length of input data */
	int options;	/* 0=exhaustive search, 1=stop at first match */
};

struct lz_index_t {
	uint16_t byte[256][MAX_LZ_BYTE_SCANS];	/* Lists of locations of each byte value */
	uint16_t bytecnt[256];	/* How many offsets exist per byte */
};

static inline int lzjody_find_lz(struct comp_data_t * const restrict data,
		const struct lz_index_t * const restrict idx);
static inline int lzjody_find_rle(struct comp_data_t * const restrict data);
static inline int lzjody_find_seq32(struct comp_data_t * const restrict data);
static inline int lzjody_find_seq16(struct comp_data_t * const restrict data);
static inline int lzjody_find_seq8(struct comp_data_t * const restrict data);

static int compress_scan(struct comp_data_t * const restrict data,
		const struct lz_index_t * const restrict idx)
{
	int err;

	while (data->ipos < data->length) {
		/* Scan for compressible items
		 * Try each compressor in sequence; if none works,
		 * just add the byte to the literal stream */
		DLOG("[c_scan] ipos: 0x%x, opos: 0x%x\n", data->ipos, data->opos);

		err = lzjody_find_rle(data);
		if (err < 0) return err;
		if (err > 0) continue;

		err = lzjody_find_seq8(data);
		if (err < 0) return err;
		if (err > 0) continue;
		err = lzjody_find_seq16(data);
		if (err < 0) return err;
		if (err > 0) continue;
		err = lzjody_find_seq32(data);
		if (err < 0) return err;
		if (err > 0) continue;

		err = lzjody_find_lz(data, idx);
		if (err < 0) return err;
		if (err > 0) continue;

		/* Nothing compressed; add to literal bytes */
		if (data->literals == 0) data->literal_start = data->ipos;
		data->literals++;
		data->ipos++;
	}
	return 0;
}

/* Build an array of byte values for faster LZ matching */
static int index_bytes(const struct comp_data_t * const restrict data,
		struct lz_index_t * const restrict idx)
{
	unsigned int pos = 0;
	unsigned char c;

	/* Clear any existing index */
    int i;
	for (i = 0; i < 256; i++) idx->bytecnt[i] = 0;

	/* Read each byte and add its offset to its list */
	if (data->length < MIN_LZ_MATCH) goto error_index;
	while (pos < (data->length - MIN_LZ_MATCH)) {
		c = *(data->in + pos);
		idx->byte[c][idx->bytecnt[c]] = pos;
		idx->bytecnt[c]++;
/*		DLOG("pos 0x%x, len 0x%x, byte 0x%x, cnt 0x%x\n",
				pos, data->length, c,
				idx->bytecnt[c]); */
		pos++;
		if (idx->bytecnt[c] == MAX_LZ_BYTE_SCANS) break;
	}
	return 0;

error_index:
	fprintf(stderr, "liblzjody: internal error: index_bytes data block length too short\n");
	return -1;
}

/* Write the control byte(s) that define data
 * type is the P_xxx value that determines the type of the control byte */
static int lzjody_write_control(struct comp_data_t * const restrict data,
		const unsigned char type,
		const uint16_t value)
{
	if (value > 0x1000) goto error_value_too_large;
	DLOG("control: (i 0x%x, o 0x%x) t 0x%x, val 0x%x: ",
			data->ipos, data->opos, type, value);
	/* Extended control bytes */
	if ((type & P_MASK) == P_EXT) {
		if (value > P_SHORT_XMAX) {
			/* Full size control bytes */
			*(data->out + data->opos) = type;
			data->opos++;
			DLOG("t0x%x ", type);
			*(data->out + data->opos) = (value >> 8);
			data->opos++;
			DLOG("vH 0x%x ", (uint8_t)(value >> 8));
			*(data->out + data->opos) = value;
			data->opos++;
			DLOG("vL 0x%x\n", (uint8_t)value);
		} else {
			/* For P_SHORT_XMAX or less, use compact form */
			*(data->out + data->opos) = (type | P_SHORT);
			data->opos++;
			DLOG("t 0x%x ", type);
			*(data->out + data->opos) = (uint8_t)value;
			data->opos++;
			DLOG("v 0x%x\n", (uint8_t)value);
		}
		return 0;
	}
	/* Standard control bytes */
	else if (value > P_SHORT_MAX) {
		DLOG("t: %x, ", type | (unsigned char)(value >> 8));
		*(unsigned char *)(data->out + data->opos) = (type | (unsigned char)(value >> 8));
		data->opos++;
		DLOG("t+vH 0x%x, vL 0x%x\n", *(data->out + data->opos - 1), (unsigned char)value);
		*(unsigned char *)(data->out + data->opos) = (unsigned char)value;
		data->opos++;
	} else {
		/* For P_SHORT_MAX or less chars, use compact form */
		*(unsigned char *)(data->out + data->opos) = type | P_SHORT | value;
		data->opos++;
		DLOG("t+v 0x%x\n", data->opos - 1);
	}
	return 0;

error_value_too_large:
	fprintf(stderr, "error: lzjody_write_control: value 0x%x > 0x1000\n", value);
	return -1;
}

/* Write out all pending literals without further processing */
static int lzjody_really_flush_literals(struct comp_data_t * const restrict data)
{
	unsigned int i = 0;
	int err;

	if (data->literals == 0) return 0;
	DLOG("really_flush_literals: 0x%x (opos 0x%x)\n", data->literals, data->opos);
	if ((data->opos + data->literals) > (LZJODY_BSIZE + 4)) goto error_opos;
	/* First write the control byte... */
	err = lzjody_write_control(data, P_LIT, data->literals);
	if (err < 0) return err;
	/* ...then the literal bytes. */
	while (i < data->literals) {
		*(data->out + data->opos) = *(data->in + data->literal_start + i);
		data->opos++;
		i++;
	}
	/* Reset literal counter*/
	DLOG("flushed; new opos: 0x%x\n\n", data->opos);
	data->literals = 0;
	return 0;

error_opos:
	fprintf(stderr, "error: final output position will overflow: 0x%x > 0x%x\n",
			data->opos + data->literals, LZJODY_BSIZE + 4);
	return -1;
}

/* Intercept a stream of literals and try byte plane transformation */
static int lzjody_flush_literals(struct comp_data_t * const restrict data)
{
	static unsigned char lit_in[LZJODY_BSIZE];
	static unsigned char lit_out[LZJODY_BSIZE + 4];
	unsigned int i;
	int err;
	static struct comp_data_t d2;
	static struct lz_index_t idx;

	/* For zero literals we'll just do nothing. */
	if (data->literals == 0) return 0;

	/* Handle blocking of recursive calls or very short literal runs */
	if ((data->literals < MIN_PLANE_LENGTH)
			|| (data->options & O_REALFLUSH)) {
		err = lzjody_really_flush_literals(data);
		if (err < 0) return err;
		return 0;
	}


	d2.in = lit_in;
	d2.out = lit_out;
	d2.ipos = 0;
	d2.opos = 0;
	d2.literals = 0;
	d2.literal_start = 0;
	d2.length = data->literals;
	/* Don't allow recursive passes or compressed data size prefix */
	d2.options = (data->options | O_REALFLUSH | O_NOPREFIX);

	DLOG("flush_literals: 0x%x\n", data->literals);

	/* Try to compress a literal run further */
	DLOG("compress further: 0x%x @ 0x%x\n", data->literals, data->literal_start);
	/* Make a transformed copy of the data */
	err = byteplane_transform((data->in + data->literal_start),
			lit_in, data->literals, 4);
	if (err < 0) return err;

	/* Load arrays for match speedup */
	err = index_bytes(&d2, &idx);
	if (err < 0) return err;

	/* Try to compress the data again */
	err = compress_scan(&d2, &idx);
	if (err < 0) return err;
	err = lzjody_really_flush_literals(&d2);
	if (err < 0) return err;

	/* If there was not enough of a size improvement, give up */
	if ((d2.opos + 2) >= d2.length) {
		DLOG("[bp] No improvement, skipping (0x%x >= 0x%x)\n",
				d2.opos,
				d2.length);
		err = lzjody_really_flush_literals(data);
		if (err < 0) return err;
		return 0;
	}

	/* Dump the newly compressed data as a literal stream */
	DLOG("Improvement: 0x%x -> 0x%x\n", d2.length, d2.opos);
	err = lzjody_write_control(data, P_PLANE, d2.opos);
	if (err < 0) return err;

	i = 0;
	while (i < d2.opos) {
		*(data->out + data->opos) = *(d2.out + i);
		data->opos++;
		i++;
	}
	/* Reset literal counter*/
	data->literals = 0;
	return 0;
}

/* Find best LZ data match for current input position */
static inline int lzjody_find_lz(struct comp_data_t * const restrict data,
		const struct lz_index_t * const restrict idx)
{
	unsigned int scan = 0;
	const unsigned char *m0, *m1, *m2;	/* pointers for matches */
	unsigned int length;	/* match length */
	const unsigned int in_remain = data->length - data->ipos;
	unsigned int remain;	/* remaining matches possible */
	int done = 0;	/* Used to terminate matching */
	unsigned int best_lz = 0;
	int best_lz_start = 0;
	unsigned int total_scans;
	unsigned int offset;
	unsigned int min_lz_match = MIN_LZ_MATCH;
	int err;

	/* If literal count > short form constraints, avoid data expansion */
	if (data->literals > P_SHORT_MAX) min_lz_match++;

	if (data->ipos >= (data->length - min_lz_match)) return 0;

	m0 = data->in + data->ipos;
	total_scans = idx->bytecnt[*m0];

	/* If the byte value does not exist anywhere, give up */
	if (!total_scans) return 0;

	/* Use linear matches if a byte happens too frequently */
	if (total_scans >= MAX_LZ_BYTE_SCANS) goto lz_linear_match;

	while (scan < total_scans) {
		/* Get offset of next byte */
		length = 0;
		m1 = m0;
		offset = idx->byte[*m1][scan];

		/* Don't use offsets higher than input position */
		if (offset >= data->ipos) {
			scan = total_scans;
			goto end_lz_jump_match;
		}

		remain = data->length - data->ipos;
		/* Handle underflow */
		if (remain > LZJODY_BSIZE) goto err_remain_underflow;

/*		DLOG("LZ remain 0x%x at offset 0x%x ipos 0x%x\n", remain, offset, data->ipos); */

		/* If we can't possibly hit the minimum match, give up immediately */
		if (remain < min_lz_match) goto end_lz_jump_match;

		m2 = data->in + offset;
/*		DLOG("LZ: offset 0x%x, remain 0x%x, scan 0x%x, total_scans 0x%x\n",
			offset, remain, scan, total_scans); */

		/* Try to reject the match quickly */
		if (*(m1 + min_lz_match - 1) != *(m2 + min_lz_match - 1)) goto end_lz_matches;

		while (*m1 == *m2) {
/*			DLOG("LZ: m1 0x%lx == m2 0x%lx (remain %x)\n",
					(long)((uintptr_t)m1 - (uintptr_t)(data->in)),
					(long)((uintptr_t)m2 - (uintptr_t)(data->in)),
					remain
					); */
			length++;
			m1++; m2++;
			remain--;
			if (!remain) {
				DLOG("LZ: hit end of data\n");
				done = 1;
				break;
			}
			if (length >= MAX_LZ_MATCH) {
				DLOG("LZ: maximum length reached\n");
				done = 1;
				break;
			}
		}
end_lz_jump_match:
		/* If this run was the longest match, record it */
		if ((length >= min_lz_match) && (length > best_lz)) {
			/* LZ can't use 4-bit offsets after 0x0f bytes */
			if ((length == min_lz_match) && (offset > 0x0f)) {
				scan++;
				continue;
			}
			DLOG("LZ match: 0x%x : 0x%x (j)\n", offset, length);
			best_lz_start = offset;
			best_lz = length;
			if (data->options & O_FAST_LZ) break;	/* Accept first LZ match */
			if (done) break;
			if (length >= MAX_LZ_MATCH) break;
		}
		scan++;
	}
	goto end_lz_matches;

lz_linear_match:
	while (scan < data->ipos) {
		m1 = data->in + scan;
		m2 = data->in + data->ipos;
		length = 0;

		remain = (in_remain - length);
		/* If we can't possibly hit the minimum match, give up immediately */
		if (remain < min_lz_match) goto end_lz_linear_match;

		/* Try to reject the match quickly */
		if (*(m1 + min_lz_match - 1) != *(m2 + min_lz_match - 1)) goto end_lz_matches;

		if (remain) {
			while (*m1 == *m2) {
/*			DLOG("LZ: m1 0x%lx == m2 0x%lx (remain %x)\n",
					(long)((uintptr_t)m1 - (uintptr_t)(data->in)),
					(long)((uintptr_t)m2 - (uintptr_t)(data->in)),
					remain); */
				length++;
				m1++; m2++;
				remain--;
				if (!remain) {
					DLOG("LZ: hit end of data\n");
					done = 1;
					goto end_lz_linear_match;
				}
				if (length >= MAX_LZ_MATCH) {
					DLOG("LZ: maximum length reached\n");
					done = 1;
					goto end_lz_linear_match;
				}
			}
		}
end_lz_linear_match:
		/* If this run was the longest match, record it */
		if ((length >= min_lz_match) && (length > best_lz)) {
			/* LZ can't use 4-bit offsets after 0x0f bytes */
			if ((length == min_lz_match) && (scan > 0x0f)) {
				scan++;
				continue;
			}
			DLOG("LZ match: 0x%x : 0x%x (l)\n", scan, length);
			best_lz_start = scan;
			best_lz = length;
			if (data->options & O_FAST_LZ) break;	/* Accept first LZ match */
			if (done) break;
			if (length >= MAX_LZ_MATCH) break;
		}
		scan++;
	}

end_lz_matches:
	/* Write out the best LZ match, if any */
	if (best_lz) {
		DLOG("LZ compressed %x:%x bytes\n", best_lz_start, best_lz);
		err = lzjody_flush_literals(data);
		if (err < 0) return err;
		if (best_lz < 256) {
			err = lzjody_write_control(data, P_LZ, best_lz_start);
			if (err < 0) return err;
		} else {
			err = lzjody_write_control(data, (P_LZ | P_LZL), best_lz_start);
			if (err < 0) return err;
			*(data->out + data->opos) = best_lz >> 8;
			data->opos++;
		}
		/* Write LZ match length low byte */
		*(data->out + data->opos) = (unsigned char)(best_lz & 0xff);
		data->opos++;
		/* Skip matched input */
		data->ipos += best_lz;
		return 1;
	}
	return 0;

err_remain_underflow:
	fprintf(stderr, "liblzjody: internal error: LZ 'remain' underflowed\n");
	return -1;
}

/* Find best RLE data match for current input position */
static inline int lzjody_find_rle(struct comp_data_t * const restrict data)
{
	const unsigned char c = *(data->in + data->ipos);
	unsigned int length = 0;
	unsigned int big_literals = 0;
	int err;

	/* If literal count > short form constraints, avoid data expansion */
	if (data->literals > P_SHORT_MAX) big_literals = 1;
	while (((length + data->ipos) < data->length) && (*(data->in + data->ipos + length) == c)) {
		length++;
	}
	if (length >= (MIN_RLE_LENGTH + big_literals)) {
		DLOG("RLE: 0x%02x of 0x%02x at i %x, o %x\n",
				length, c, data->ipos, data->opos);
		err = lzjody_flush_literals(data);
		if (err < 0) return err;
		err = lzjody_write_control(data, P_RLE, length);
		if (err < 0) return err;
		/* Write repeated byte */
		*(data->out + data->opos) = c;
		data->opos++;
		/* Skip matched input */
		data->ipos += length;
		return 1;
	}
	return 0;
}

/* Find sequential 32-bit values for compression */
static inline int lzjody_find_seq32(struct comp_data_t * const restrict data)
{
	uint32_t num32;
	uint32_t *m32 = (uint32_t *)((uintptr_t)data->in + (uintptr_t)data->ipos);
	const uint32_t num_orig32 = *m32;
	unsigned int seqcnt;
	unsigned int big_literals = 0;
	int err;

	/* If literal count > short form constraints, avoid data expansion */
	if (data->literals > P_SHORT_MAX) big_literals = 1;

	/* 32-bit sequences */
	seqcnt = 0;
	num32 = *m32;
	/* Loop bounds check compensates for bit width of data elements */
	while (*m32 == num32) {
		if ((data->ipos + seqcnt + 3) >= data->length) break;
		seqcnt += 4;
		num32++;
		m32++;
	}
	seqcnt >>= 2;

	if (seqcnt >= (MIN_SEQ32_LENGTH + big_literals)) {
		DLOG("Seq(32): start 0x%x, 0x%x items\n", num_orig32, seqcnt);
		err = lzjody_flush_literals(data);
		if (err < 0) return err;
		err = lzjody_write_control(data, P_SEQ32, seqcnt);
		if (err < 0) return err;
		*(uint32_t *)((uintptr_t)data->out + (uintptr_t)data->opos) = num_orig32;
		data->opos += sizeof(uint32_t);
		data->ipos += (seqcnt << 2);
		return 1;
	}

	return 0;
}

/* Find sequential 16-bit values for compression */
static inline int lzjody_find_seq16(struct comp_data_t * const restrict data)
{
	uint16_t num16;
	uint16_t *m16 = (uint16_t *)((uintptr_t)data->in + (uintptr_t)data->ipos);
	const uint16_t num_orig16 = *m16;
	unsigned int seqcnt;
	unsigned int big_literals = 0;
	int err;

	/* If literal count > short form constraints, avoid data expansion */
	if (data->literals > P_SHORT_MAX) big_literals = 1;

	seqcnt = 0;
	num16 = *m16;
	/* Loop bounds check compensates for bit width of data elements */
	while (*m16 == num16) {
		if ((data->ipos + (seqcnt << 1) + 1) >= data->length) break;
		seqcnt++;
		num16++;
		m16++;
	}

	if (seqcnt >= (MIN_SEQ16_LENGTH + big_literals)) {
		DLOG("Seq(16): start 0x%x, 0x%x items\n", num_orig16, seqcnt);
		err = lzjody_flush_literals(data);
		if (err < 0) return err;
		err = lzjody_write_control(data, P_SEQ16, seqcnt);
		if (err < 0) return err;
		*(uint16_t *)((uintptr_t)data->out + (uintptr_t)data->opos) = num_orig16;
		data->opos += sizeof(uint16_t);
		data->ipos += (seqcnt << 1);
		return 1;
	}

	return 0;
}

/* Find sequential 8-bit values for compression */
static inline int lzjody_find_seq8(struct comp_data_t * const restrict data)
{
	uint8_t num8;
	uint8_t *m8 = (uint8_t *)((uintptr_t)data->in + (uintptr_t)data->ipos);
	const uint8_t num_orig8 = *m8;
	unsigned int seqcnt;
	unsigned int big_literals = 0;
	int err;

	/* If literal count > short form constraints, avoid data expansion */
	if (data->literals > P_SHORT_MAX) big_literals = 1;

	seqcnt = 0;
	num8 = *m8;
	while (*m8 == num8) {
		if ((data->ipos + seqcnt) >= data->length) break;
		seqcnt++;
		num8++;
		m8++;
	}

	if (seqcnt >= (MIN_SEQ8_LENGTH + big_literals)) {
		DLOG("Seq(8): start 0x%x, 0x%x items\n", num_orig8, seqcnt);
		err = lzjody_flush_literals(data);
		if (err < 0) return err;
		err = lzjody_write_control(data, P_SEQ8, seqcnt);
		if (err < 0) return err;
		*(uint8_t *)((uintptr_t)data->out + (uintptr_t)data->opos) = num_orig8;
		data->opos += sizeof(uint8_t);
		data->ipos += seqcnt;
		return 1;
	}
	return 0;
}

/* Lempel-Ziv compressor by Jody Bruchon (LZJODY)
 * Compresses "blk" data and puts result in "out"
 * out must be at least 2 bytes larger than blk in case
 * the data is not compressible at all.
 * Returns the size of "out" data or returns -1 if the
 * compressed data is not smaller than the original data.
 */
extern int lzjody_compress(const unsigned char * const blk_in,
		unsigned char * const blk_out,
		const unsigned int options,
		const unsigned int length)
{
	int err;

	/* Initialize compression data structure */
	static struct comp_data_t data;
	static struct lz_index_t idx;

	DLOG("Comp: blk len 0x%x\n", length);

	data.in = blk_in;
	data.out = blk_out;
	data.ipos = 0;
	data.opos = 2;
	data.literals = 0;
	data.literal_start = 0;
	data.length = length;
	data.options = options;

	if (options & O_NOPREFIX) data.opos = 0;

	/* Perform sanity checks on data length */
	if (length == 0) goto error_zero_length;
	if (length > LZJODY_BSIZE) goto error_large_length;

	/* Nothing under 3 bytes long will compress */
	if (length < 3) {
		data.literals = length;
		goto compress_short;
	}

	/* Load arrays for match speedup */
	err = index_bytes(&data, &idx);
	if (err < 0) return err;

	/* Scan through entire block looking for compressible items */
	err = compress_scan(&data, &idx);
	if (err < 0) return err;

compress_short:
	/* Flush any remaining literals */
	err = lzjody_flush_literals(&data);
	if (err < 0) return err;

	/* Write the total length to the data block unless asked not to */
	if (!(options & O_NOPREFIX)) {
/* This uncompressed block part isn't working yet */
#if 0
		if (data.opos >= length) {
			/* Flag incompressible data for possible faster decompression */
			*(unsigned char *)(data.out) =
				(unsigned char)((((data.opos - 2) & 0x1f00) >> 8) | O_NOCOMPRESS);
			DLOG("### Incompressible: %x -> %x\n",
				(unsigned char)(((data.opos - 2) & 0x1f00) >> 8),
				(unsigned char)(((data.opos - 2) & 0x1f00) >> 8) | O_NOCOMPRESS);
		} else {
#endif
			*(unsigned char *)(data.out) = (unsigned char)(((data.opos - 2) & 0x1f00) >> 8);
//		}
		*(unsigned char *)(data.out + 1) = (unsigned char)(data.opos - 2);
	}

	DLOG("compressed length: %x\n\n", data.opos);
	return data.opos;

error_large_length:
	fprintf(stderr, "liblzjody: error: block length %d larger than maximum of %d\n",
			length, LZJODY_BSIZE);
	return -1;
error_zero_length:
	fprintf(stderr, "liblzjody: error: cannot compress a zero-length block\n");
	return -1;
}

/* LZJODY decompressor */
extern int lzjody_decompress(const unsigned char * const in,
		unsigned char * const out,
		const unsigned int size,
		const unsigned int options)
{
	unsigned int mode;
	register unsigned int ipos = 0;
	register unsigned int opos = 0;
	unsigned int offset;
	register unsigned int length = 0;
	unsigned int sl;	/* short/long */
	unsigned int control = 0;
	unsigned char c;
	const unsigned char *mem1;
	unsigned char *mem2;
	/* FIXME: volatile to prevent vectorization (-fno-tree-loop-vectorize)
	 * Should probably find another way to prevent unaligned vector access */
	union {
		uint32_t *m32;
		volatile uint16_t *m16;
		uint8_t *m8;
	} mem;
	union {
		uint32_t num32;
		uint16_t num16;
		uint8_t num8;
	} num;
	unsigned int seqbits = 0;
	unsigned char *bp_out;
	unsigned int bp_length;
	unsigned char bp_temp[LZJODY_BSIZE];
	int err;

	/* Cannot decompress a zero-length block */
	if (size == 0) return -1;

	while (ipos < size) {
		c = *(in + ipos);
		DLOG("Command 0x%x\n", c);
		mode = c & P_MASK;
		sl = c & P_SHORT;
		ipos++;
		/* Extended commands don't advance input here */
		if (mode == 0) {
			/* Change mode to the extended command instead */
			mode = c & P_XMASK;
			DLOG("X-mode: %x\n", mode);
			/* Initializer for sequence/byteplane commands */
			if (mode & (P_SMASK | P_PLANE)) {
				length = *(in + ipos);
#ifdef DEBUG
				if (mode & P_SMASK) { DLOG("Seq length: %x\n", length); }
				if (mode & P_PLANE) { DLOG("Byte plane length: %x\n", length); }
#endif /* DLOG */
				ipos++;
				/* Long form has a high byte */
				if (!sl) {
					length <<= 8;
					length += (uint16_t)*(in + ipos);
					DLOG("length modifier: 0x%x (0x%x)\n",
						*(in + ipos),
						(uint16_t)*(in + ipos) << 8);
					ipos++;
				}
				if (length > LZJODY_BSIZE) goto error_length;
			}
		}
		/* Handle short/long standard commands */
		else if (sl) {
			control = c & P_SHORT_MAX;
			DLOG("Short control: 0x%x\n", control);
		} else {
			if (c & (P_RLE | P_LZL)) 
				control = (unsigned int)(c & (P_LZL | P_SHORT_MAX)) << 8;
			else control = (unsigned int)(c & P_SHORT_MAX) << 8;
			control += *(in + ipos);
			DLOG("Long control: 0x%x\n", control);
			ipos++;
		}

		/* Based on the command, select a decompressor */
		switch (mode) {
			case P_PLANE:
				/* Byte plane transformation handler */
				DLOG("%04x:%04x:  Byte plane c_len 0x%x\n", ipos, opos, length);
				bp_out = out + opos;
				bp_length = lzjody_decompress((in + ipos), bp_out, length, 0);

				err = byteplane_transform(bp_out, bp_temp, bp_length, -4);
				if (err < 0) return err;

				DLOG("Byte plane transform len 0x%x done\n", bp_length);
				ipos += length;
				opos += bp_length;
				if (opos > LZJODY_BSIZE) goto error_bp_length;
				length = 0;
				/* memcpy sucks, we can do it ourselves */
				while(length < bp_length) {
					*(bp_out + length) = *(bp_temp + length);
					length++;
				}
				break;
			case P_LZ:
				/* LZ (dictionary-based) compression */
				offset = control & 0xfff;
				length = *(in + ipos);
				ipos++;
				if (c & P_LZL) {
					length <<= 8;
					length += *(in + ipos);
					ipos++;
				}
				DLOG("%04x:%04x: LZ block (%x:%x)\n",
						ipos, opos, offset, length);
				/* memcpy/memmove do not handle the overlap
				 * correctly when it happens, so we copy the
				 * data manually.
				 */
				if (offset >= opos) goto error_lz_offset;
				mem1 = out + offset;
				mem2 = out + opos;
				opos += length;
				if (opos > LZJODY_BSIZE) goto error_lz_length;
				while (length != 0) {
					*mem2 = *mem1;
					mem1++; mem2++;
					length--;
				}
				break;

			case P_RLE:
				/* Run-length encoding */
				length = control;
				c = *(in + ipos);
				ipos++;
				DLOG("%04x:%04x: RLE run 0x%x\n", ipos, opos, length);
				if (opos + length > LZJODY_BSIZE) goto error_rle_length;
				while (length > 0) {
					*(out + opos) = c;
					opos++;
					length--;
				}
				break;

			case P_LIT:
				/* Literal byte sequence */
				DLOG("%04x:%04x: 0x%x literal bytes\n", ipos, opos, control);
				length = control;
				mem1 = (const unsigned char *)(in + ipos);
				mem2 = (unsigned char *)(out + opos);
				while (length != 0) {
					*mem2 = *mem1;
					mem1++; mem2++;
					length--;
				}
				ipos += control;
				opos += control;
				if (opos > LZJODY_BSIZE) goto error_lit_length;
				break;

			case P_SEQ32:
				seqbits = 32;
				/* Sequential increment compression (32-bit) */
				DLOG("%04x:%04x: Seq(32) 0x%x\n", ipos, opos, length);
				/* Get sequence start number */
				num.num32 = *(uint32_t *)((uintptr_t)in + (uintptr_t)ipos);
				ipos += sizeof(uint32_t);
				/* Get sequence start position */
				mem.m32 = (uint32_t *)((uintptr_t)out + (uintptr_t)opos);
				opos += (length << 2);
				if (opos > LZJODY_BSIZE) goto error_seq;
				DLOG("opos = 0x%x, length = 0x%x\n", opos, length);
				while (length > 0) {
					*mem.m32 = num.num32;
					mem.m32++; num.num32++;
					length--;
				}
				break;

			case P_SEQ16:
				seqbits = 16;
				/* Sequential increment compression (16-bit) */
				DLOG("%04x:%04x: Seq(16) 0x%x\n", ipos, opos, length);
				/* Get sequence start number */
				num.num16 = *(uint16_t *)((uintptr_t)in + (uintptr_t)ipos);
				ipos += sizeof(uint16_t);
				/* Get sequence start position */
				mem.m16 = (uint16_t *)((uintptr_t)out + (uintptr_t)opos);
				DLOG("opos = 0x%x, length = 0x%x\n", opos, length);
				opos += (length << 1);
				if (opos > LZJODY_BSIZE) goto error_seq;
				while (length > 0) {
					*mem.m16 = num.num16;
					mem.m16++; num.num16++;
					length--;
				}
				break;

			case P_SEQ8:
				seqbits = 8;
				/* Sequential increment compression (8-bit) */
				DLOG("%04x:%04x: Seq(8) 0x%x\n", ipos, opos, length);
				/* Get sequence start number */
				num.num8 = *(uint8_t *)((uintptr_t)in + (uintptr_t)ipos);
				ipos += sizeof(uint8_t);
				/* Get sequence start position */
				mem.m8 = (uint8_t *)((uintptr_t)out + (uintptr_t)opos);
				opos += length;
				if (opos > LZJODY_BSIZE) goto error_seq;
				while (length > 0) {
					*mem.m8 = num.num8;
					mem.m8++; num.num8++;
					length--;
				}
				break;

			default:
				goto error_mode;
		}
	}

	if (opos > LZJODY_BSIZE) goto error_opos;
	return opos;

error_opos:
	fprintf(stderr, "liblzjody: error: output pos %d higher than maximum %d)\n", opos, LZJODY_BSIZE);
	return -1;
error_bp_length:
	fprintf(stderr, "liblzjody: error: byte plane length overflows output pos (%d > %d)\n",
			opos, LZJODY_BSIZE);
	return -1;
error_rle_length:
	fprintf(stderr, "liblzjody: error: RLE length overflows output pos (%d > %d)\n",
			opos + length, LZJODY_BSIZE);
	return -1;
error_lit_length:
	fprintf(stderr, "liblzjody: error: literal length overflows output pos (%d > %d)\n",
			opos, LZJODY_BSIZE);
	return -1;
error_lz_length:
	fprintf(stderr, "liblzjody: error: LZ length overflows output pos (%d > %d)\n",
			opos, LZJODY_BSIZE);
	return -1;
error_lz_offset:
	fprintf(stderr, "liblzjody: data error: LZ offset 0x%x >= output pos 0x%x)\n", offset, opos);
	return -1;
error_seq:
	fprintf(stderr, "liblzjody: data error: seq%d overflow (length 0x%x)\n", seqbits, length);
	return -1;
error_length:
	fprintf(stderr, "liblzjody: data error: length 0x%x greater than maximum 0x%x @ 0x%x\n",
			length, LZJODY_BSIZE, ipos - 1);
	return -1;
error_mode:
	fprintf(stderr, "liblzjody: error: invalid decompressor mode 0x%x at 0x%x\n", mode, ipos);
	return -1;
}

