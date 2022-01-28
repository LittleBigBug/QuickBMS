// modified by Luigi Auriemma

/**
 * compress.c - Compressed attribute handling code.  Part of the Linux-NTFS
 *		project.
 *
 * Copyright (c) 2004-2005 Anton Altaparmakov
 * Copyright (c)      2005 Yura Pakhuchiy
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
#define __le16_to_cpu(x) bswap_16((u16)(x))
#else
#define __le16_to_cpu(x) ((u16)(x))
#endif
#define le16_to_cpup(x)		(u16)__le16_to_cpu(*(u16*)(x))

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/*
#include "attrib.h"
#include "debug.h"
#include "volume.h"
#include "types.h"
#include "layout.h"
#include "runlist.h"
#include "compress.h"
#include "logging.h"
*/

/**
 * enum ntfs_compression_constants - constants used in the compression code
 */
typedef enum {
	/* Token types and access mask. */
	NTFS_SYMBOL_TOKEN	=	0,
	NTFS_PHRASE_TOKEN	=	1,
	NTFS_TOKEN_MASK		=	1,

	/* Compression sub-block constants. */
	NTFS_SB_SIZE_MASK	=	0x0fff,
	NTFS_SB_SIZE		=	0x1000,
	NTFS_SB_IS_COMPRESSED	=	0x8000,
} ntfs_compression_constants;

/**
 * ntfs_decompress - decompress a compression block into an array of pages
 * @dest:	buffer to which to write the decompressed data
 * @dest_size:	size of buffer @dest in bytes
 * @cb_start:	compression block to decompress
 * @cb_size:	size of compression block @cb_start in bytes
 *
 * This decompresses the compression block @cb_start into the destination
 * buffer @dest.
 *
 * @cb_start is a pointer to the compression block which needs decompressing
 * and @cb_size is the size of @cb_start in bytes (8-64kiB).
 *
 * Return 0 if success or -EOVERFLOW on error in the compressed stream.
 */
static int ntfs_decompress(u8 *dest, const u32 dest_size,
		u8 *const cb_start, const u32 cb_size)
{
	/*
	 * Pointers into the compressed data, i.e. the compression block (cb),
	 * and the therein contained sub-blocks (sb).
	 */
	u8 *cb_end = cb_start + cb_size; /* End of cb. */
	u8 *cb = cb_start;	/* Current position in cb. */
	u8 *cb_sb_start = cb;	/* Beginning of the current sb in the cb. */
	u8 *cb_sb_end;		/* End of current sb / beginning of next sb. */
	/* Variables for uncompressed data / destination. */
	u8 *dest_end = dest + dest_size;	/* End of dest buffer. */
	u8 *dest_sb_start;	/* Start of current sub-block in dest. */
	u8 *dest_sb_end;	/* End of current sb in dest. */
	/* Variables for tag and token parsing. */
	u8 tag;			/* Current tag. */
	int token;		/* Loop counter for the eight tokens in tag. */

	//ntfs_log_trace("Entering, cb_size = 0x%x.\n", (unsigned)cb_size);
do_next_sb:
	//ntfs_log_debug("Beginning sub-block at offset = 0x%x in the cb.\n",
			//cb - cb_start);
	/*
	 * Have we reached the end of the compression block or the end of the
	 * decompressed data?  The latter can happen for example if the current
	 * position in the compression block is one byte before its end so the
	 * first two checks do not detect it.
	 */
	if (cb == cb_end || !le16_to_cpup((u16*)cb) || dest == dest_end) {
		//ntfs_log_debug("Completed. Returning success (0).\n");
		return 0;
	}
	/* Setup offset for the current sub-block destination. */
	dest_sb_start = dest;
	dest_sb_end = dest + NTFS_SB_SIZE;
	/* Check that we are still within allowed boundaries. */
	if (dest_sb_end > dest_end)
		goto return_overflow;
	/* Does the minimum size of a compressed sb overflow valid range? */
	if (cb + 6 > cb_end)
		goto return_overflow;
	/* Setup the current sub-block source pointers and validate range. */
	cb_sb_start = cb;
	cb_sb_end = cb_sb_start + (le16_to_cpup((u16*)cb) & NTFS_SB_SIZE_MASK)
			+ 3;
	if (cb_sb_end > cb_end)
		goto return_overflow;
	/* Now, we are ready to process the current sub-block (sb). */
	if (!(le16_to_cpup((u16*)cb) & NTFS_SB_IS_COMPRESSED)) {
		//ntfs_log_debug("Found uncompressed sub-block.\n");
		/* This sb is not compressed, just copy it into destination. */
		/* Advance source position to first data byte. */
		cb += 2;
		/* An uncompressed sb must be full size. */
		if (cb_sb_end - cb != NTFS_SB_SIZE)
			goto return_overflow;
		/* Copy the block and advance the source position. */
		memcpy(dest, cb, NTFS_SB_SIZE);
		cb += NTFS_SB_SIZE;
		/* Advance destination position to next sub-block. */
		dest += NTFS_SB_SIZE;
		goto do_next_sb;
	}
	//ntfs_log_debug("Found compressed sub-block.\n");
	/* This sb is compressed, decompress it into destination. */
	/* Forward to the first tag in the sub-block. */
	cb += 2;
do_next_tag:
	if (cb == cb_sb_end) {
		/* Check if the decompressed sub-block was not full-length. */
		if (dest < dest_sb_end) {
			int nr_bytes = dest_sb_end - dest;

			//ntfs_log_debug("Filling incomplete sub-block with zeroes.\n");
			/* Zero remainder and update destination position. */
			memset(dest, 0, nr_bytes);
			dest += nr_bytes;
		}
		/* We have finished the current sub-block. */
		goto do_next_sb;
	}
	/* Check we are still in range. */
	if (cb > cb_sb_end || dest > dest_sb_end)
		goto return_overflow;
	/* Get the next tag and advance to first token. */
	tag = *cb++;
	/* Parse the eight tokens described by the tag. */
	for (token = 0; token < 8; token++, tag >>= 1) {
		u16 lg, pt, length, max_non_overlap;
		register u16 i;
		u8 *dest_back_addr;

		/* Check if we are done / still in range. */
		if (cb >= cb_sb_end || dest > dest_sb_end)
			break;
		/* Determine token type and parse appropriately.*/
		if ((tag & NTFS_TOKEN_MASK) == NTFS_SYMBOL_TOKEN) {
			/*
			 * We have a symbol token, copy the symbol across, and
			 * advance the source and destination positions.
			 */
			*dest++ = *cb++;
			/* Continue with the next token. */
			continue;
		}
		/*
		 * We have a phrase token. Make sure it is not the first tag in
		 * the sb as this is illegal and would confuse the code below.
		 */
		if (dest == dest_sb_start)
			goto return_overflow;
		/*
		 * Determine the number of bytes to go back (p) and the number
		 * of bytes to copy (l). We use an optimized algorithm in which
		 * we first calculate log2(current destination position in sb),
		 * which allows determination of l and p in O(1) rather than
		 * O(n). We just need an arch-optimized log2() function now.
		 */
		lg = 0;
		for (i = dest - dest_sb_start - 1; i >= 0x10; i >>= 1)
			lg++;
		/* Get the phrase token into i. */
		pt = le16_to_cpup((u16*)cb);
		/*
		 * Calculate starting position of the byte sequence in
		 * the destination using the fact that p = (pt >> (12 - lg)) + 1
		 * and make sure we don't go too far back.
		 */
		dest_back_addr = dest - (pt >> (12 - lg)) - 1;
		if (dest_back_addr < dest_sb_start)
			goto return_overflow;
		/* Now calculate the length of the byte sequence. */
		length = (pt & (0xfff >> lg)) + 3;
		/* Verify destination is in range. */
		if (dest + length > dest_sb_end)
			goto return_overflow;
		/* The number of non-overlapping bytes. */
		max_non_overlap = dest - dest_back_addr;
		if (length <= max_non_overlap) {
			/* The byte sequence doesn't overlap, just copy it. */
			memcpy(dest, dest_back_addr, length);
			/* Advance destination pointer. */
			dest += length;
		} else {
			/*
			 * The byte sequence does overlap, copy non-overlapping
			 * part and then do a slow byte by byte copy for the
			 * overlapping part. Also, advance the destination
			 * pointer.
			 */
			memcpy(dest, dest_back_addr, max_non_overlap);
			dest += max_non_overlap;
			dest_back_addr += max_non_overlap;
			length -= max_non_overlap;
			while (length--)
				*dest++ = *dest_back_addr++;
		}
		/* Advance source position and continue with the next token. */
		cb += 2;
	}
	/* No tokens left in the current tag. Continue with the next tag. */
	goto do_next_tag;
return_overflow:
	//ntfs_log_debug("Failed. Returning -EOVERFLOW.\n");
	errno = -1; //EOVERFLOW;
	return -1;
}

