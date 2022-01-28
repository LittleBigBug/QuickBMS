/*
 * test_brieflz - BriefLZ unit test
 *
 * Copyright (c) 2002-2016 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

#include "brieflz.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "greatest.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static const unsigned char data_numbers[] = {
	1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
	16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
	31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,
	46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
	61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,
	76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
	91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105,
	106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
	121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
	151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165,
	166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
	181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
	196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
	211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
	226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248
};

static const unsigned char data_zeroes[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const unsigned char data_alternate[] = {
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

struct packed_data {
	unsigned long src_size;
	unsigned long depacked_size;
	const unsigned char data[32];
};

static const struct packed_data data_errors[] = {
	/* unable to copy first literal from input */
	{ 0, 1, { 0x42 } },
	/* unable to read first byte of first tag */
	{ 1, 2, { 0x42 } },
	/* unable to read second byte of first tag */
	{ 2, 2, { 0x42, 0x00 } },
	/* unable to copy literal from input */
	{ 3, 2, { 0x42, 0x00, 0x00 } },
	/* unable to read offs byte */
	{ 3, 5, { 0x42, 0x00, 0x80 } },
	/* unable to read first bit in offs gamma */
	{ 4, 5, { 0x42, 0x55, 0x45, 0x42 } },
	/* unable to read second bit in offs gamma */
	{ 3, 5, { 0x42, 0xAA, 0x8A } },
	/* match reading one byte before input */
	{ 4, 5, { 0x42, 0x00, 0x80, 0x01 } },
	/* match overflowing g2 encoding */
	{12, 3, { 0x42, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00 } },
	/* match overflowing offs when adding byte */
	{10, 5, { 0x42, 0xAA, 0x8A, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0xC0, 0xFF } },
	/* match one past end of output */
	{ 4, 4, { 0x42, 0x00, 0x80, 0x00 } },
};

static void *workmem = NULL;

static unsigned char buffer1[4093];
static unsigned char buffer2[8191];
static unsigned char buffer3[4093];

static void generate_random(unsigned char *p, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i) {
		p[i] = (unsigned char) rand();
	}
}

/* Test compressing a zero length buffer */
TEST pack_nothing(void *arg)
{
	unsigned long res;
	const int level = *(int *) arg;

	/* try to compress 0 bytes, passing read only memory */
	res = blz_pack_level(data_numbers, (char *) data_numbers, 0,
	               (char *) data_numbers, level);

	ASSERT_EQ(0, res);
	PASS();
}

/* Test compression and decompression of a buffer of zero bytes */
TEST pack_zeroes(void *arg)
{
	unsigned long res;
	size_t i;
	const int level = *(int *) arg;

	for (i = 1; i < ARRAY_SIZE(data_zeroes); ++i) {
		/* compress first i bytes of data_zeroes[] */
		res = blz_pack_level(data_zeroes, buffer1, (unsigned long) i, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) i));

		/* decompress */
		res = blz_depack_safe(buffer1, res, buffer2, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_zeroes, buffer2, i) == 0);

		/* decompress */
		res = blz_depack(buffer1, buffer3, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_zeroes, buffer3, i) == 0);
	}

	PASS();
}

/* Test compression and all decompressors on a buffer of increasing byte
   values (incompressible) */
TEST pack_numbers(void *arg)
{
	unsigned long res;
	unsigned long size;
	size_t i;
	const int level = *(int *) arg;

	for (i = 1; i < ARRAY_SIZE(data_numbers); ++i) {
		/* compress first i bytes of data_numbers[] */
		size = blz_pack_level(data_numbers, buffer1, (unsigned long) i, workmem, level);

		ASSERT(size != BLZ_ERROR);
		ASSERT(size <= blz_max_packed_size((unsigned long) i));

		/* decompress */
		res = blz_depack_safe(buffer1, size, buffer2, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_numbers, buffer2, i) == 0);

		/* decompress */
		res = blz_depack(buffer1, buffer3, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_numbers, buffer3, i) == 0);
	}

	PASS();
}

/* Test compression and decompression of a buffer of compressible data
   consisting of zero and non-zero bytes */
TEST pack_alternate(void *arg)
{
	unsigned long res;
	unsigned long size;
	size_t i;
	const int level = *(int *) arg;

	for (i = 1; i < ARRAY_SIZE(data_alternate); ++i) {
		/* compress first i bytes of data_alternate[] */
		size = blz_pack_level(data_alternate, buffer1, (unsigned long) i, workmem, level);

		ASSERT(size != BLZ_ERROR);
		ASSERT(size <= blz_max_packed_size((unsigned long) i));

		/* decompress */
		res = blz_depack_safe(buffer1, size, buffer2, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_alternate, buffer2, i) == 0);

		/* decompress */
		res = blz_depack(buffer1, buffer3, (unsigned long) i);

		ASSERT(res == i);
		ASSERT(memcmp(data_alternate, buffer3, i) == 0);
	}

	PASS();
}

/* Test compression and decompression of a buffer of random data with an
   expanding area of matching bytes between the front and back */
TEST pack_random(void *arg)
{
	unsigned char *p = NULL;
	const size_t size = ARRAY_SIZE(buffer1);
	size_t i, j;
	unsigned long res;
	const int level = *(int *) arg;

	srand(42);
	generate_random(buffer1, size);

	for (i = 0; i < size / 2; ++i) {

		/* copy first i bytes of buffer1 to end of buffer1 */
		for (j = 0; j < i; ++j) {
			buffer1[size - i + j] = buffer1[j];
		}

		/* compress */
		res = blz_pack_level(buffer1, buffer2, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		p = malloc(res);

		assert(p != NULL);

		/* compress again, to buffer of exact size */
		res = blz_pack_level(buffer1, p, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		/* decompress */
		res = blz_depack_safe(p, res, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		/* decompress */
		res = blz_depack(p, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		free(p);
		p = NULL;
	}

	PASS();
}

/* Test compression and decompression of a buffer of random data with an
   expanding area of identical bytes at the back */
TEST pack_random_start(void *arg)
{
	unsigned char *p = NULL;
	const size_t size = ARRAY_SIZE(buffer1);
	size_t i, j;
	unsigned long res;
	const int level = *(int *) arg;

	srand(42);
	generate_random(buffer1, size);

	for (i = 0; i < size / 2; ++i) {
		/* generate compressible data at end */
		for (j = 0; j < i; ++j) {
			buffer1[size - i + j] = 0xFF;
		}

		/* compress */
		res = blz_pack_level(buffer1, buffer2, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		p = malloc(res);

		assert(p != NULL);

		/* compress again, to buffer of exact size */
		res = blz_pack_level(buffer1, p, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		/* decompress */
		res = blz_depack_safe(p, res, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		/* decompress */
		res = blz_depack(p, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		free(p);
		p = NULL;
	}

	PASS();
}

/* Test compression and decompression of a buffer of random data with an
   expanding area of identical bytes at the front */
TEST pack_random_end(void *arg)
{
	unsigned char *p = NULL;
	const size_t size = ARRAY_SIZE(buffer1);
	size_t i, j;
	unsigned long res;
	const int level = *(int *) arg;

	srand(42);
	generate_random(buffer1, size);

	for (i = 0; i < size / 2; ++i) {
		/* generate compressible data at start */
		for (j = 0; j < i; ++j) {
			buffer1[j] = 0xFF;
		}

		/* compress */
		res = blz_pack_level(buffer1, buffer2, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		p = malloc(res);

		assert(p != NULL);

		/* compress again, to buffer of exact size */
		res = blz_pack_level(buffer1, p, (unsigned long) size, workmem, level);

		ASSERT(res != BLZ_ERROR);
		ASSERT(res <= blz_max_packed_size((unsigned long) size));

		/* decompress */
		res = blz_depack_safe(p, res, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		/* decompress */
		res = blz_depack(p, buffer3, (unsigned long) size);

		ASSERT(res == size);
		ASSERT(memcmp(buffer1, buffer3, size) == 0);

		free(p);
		p = NULL;
	}

	PASS();
}

/* Test decompressing a zero length buffer */
TEST depack_nothing(void)
{
	unsigned long res;

	res = blz_depack_safe(data_numbers, 0, (char *) data_numbers, 0);

	ASSERT_EQ(0, res);

	res = blz_depack(data_numbers, (char *) data_numbers, 0);

	ASSERT_EQ(0, res);

	PASS();
}

/* Test blz_depack_safe on compressed data with errors */
TEST depack_safe_errors(void)
{
	unsigned long res;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(data_errors); ++i) {
		res = blz_depack_safe(data_errors[i].data, data_errors[i].src_size,
		                      buffer1, data_errors[i].depacked_size);

		ASSERT(res == BLZ_ERROR);
	}

	PASS();
}

/* Test blz_depack_safe on random data */
TEST depack_safe_random(void)
{
	const size_t size = ARRAY_SIZE(buffer1) / 2;
	size_t i, j;

	for (i = 0; i < 1024; ++i) {
		generate_random(buffer1, size);

		for (j = 0; j < size / 2; ++j) {
			blz_depack_safe(&buffer1[j], (unsigned long) (size - j),
			                buffer3, (unsigned long) ARRAY_SIZE(buffer3));
		}
	}

	PASS();
}

SUITE(BriefLZ)
{
	int level;

	for (level = 1; level <= 10; ++level) {
		workmem = malloc(blz_workmem_size_level((unsigned long) ARRAY_SIZE(buffer1), level));

		assert(workmem != NULL);

		RUN_TEST1(pack_nothing, &level);

		RUN_TEST1(pack_numbers, &level);
		RUN_TEST1(pack_alternate, &level);
		RUN_TEST1(pack_random, &level);

		if (level < 10) {
			RUN_TEST1(pack_random_start, &level);
			RUN_TEST1(pack_random_end, &level);
			RUN_TEST1(pack_zeroes, &level);
		}

		free(workmem);
	}

	RUN_TEST(depack_nothing);
	RUN_TEST(depack_safe_errors);
	RUN_TEST(depack_safe_random);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[])
{
	GREATEST_MAIN_BEGIN();
	RUN_SUITE(BriefLZ);
	GREATEST_MAIN_END();
}
