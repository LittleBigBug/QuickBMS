// http://git.kopf-tisch.de/?p=zyxel-revert;a=blob_plain;f=lzsd.c;hb=HEAD
#include <stdio.h>
#include <stdint.h>

struct zyxel_lzs_state {
	uint8_t *srcblkstart;
	uint8_t *src;
	uint32_t srcsize;

	uint8_t *dstblkstart;
	uint8_t *dst;
	uint32_t dstsize;

	uint32_t bitbuf;
	uint32_t bitcnt;
};

static uint32_t zyxel_get_bits(struct zyxel_lzs_state *state, int num)
{
	while (state->bitcnt < num) {
		state->bitbuf = (state->bitbuf << 8) | *(state->src)++;
		state->bitcnt += 8;
	}

	state->bitcnt -= num;
	return (state->bitbuf >> state->bitcnt) & ((1 << num) -1);
}

static uint32_t zyxel_get_len(struct zyxel_lzs_state *state)
{
	uint32_t bits;
	uint32_t length = 2;

	do {
		bits = zyxel_get_bits(state, 2);
		length += bits;
	} while ((bits == 3) && (length < 8));

	if (length == 8) {
		do {
			bits = zyxel_get_bits(state, 4);
			length += bits;
		} while (bits == 15);
	}

	return length;
}

static int zyxel_get_zyxel_header(struct zyxel_lzs_state *state)
{
	//uint8_t *p = state->srcblkstart;
	//uint32_t a = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

	uint32_t b = ((state->dst - state->dstblkstart) << 16) & 0xFFFF0000;
	b |= ((state->src - state->srcblkstart) & 0xFFFF);

	/* remove own header */
	b -= 4;

	//printf("header of previous block is=0x%08x expected=0x%08x %s\n", a, b, ((a == b) ? "OK" : "ERROR"));
	return 0;
}

/*
 * TODO: check src/dst sizes
 */
uint32_t zyxel_lzs_unpack(uint8_t *srcbuf, uint32_t srcsize, uint8_t *dstbuf, uint32_t dstsize)
{
	struct zyxel_lzs_state state = {
		.srcblkstart = srcbuf,
		.src = srcbuf,
		.srcsize = srcsize,

		.dstblkstart = dstbuf,
		.dst = dstbuf,
		.dstsize = dstsize,

		.bitbuf = 0,
		.bitcnt = 0,
	};

	while (1) {
		/* jump over header */
		if (state.srcblkstart == state.src)
			state.src += 4;

		uint32_t tag = zyxel_get_bits(&state, 1);

		/* Uncompressed byte */
		if (tag == 0) {
			*(state.dst)++ = zyxel_get_bits(&state, 8);
//			printf("uncompressed byte: 0x%02x\n", *(state.dst -1));

		/* Compressed string */
		} else {
			/* read 7 or 11 bit offset */
			tag = zyxel_get_bits(&state, 1);
			uint32_t offset = zyxel_get_bits(&state, (tag == 1) ? 7 : 11);

			/* end condition (7bit offset == 0x00) */
			if (tag == 1 && offset == 0) {
				/* align src to next byte */
				uint32_t cnt = state.bitcnt;
				/*uint32_t tmp =*/ zyxel_get_bits(&state, cnt);

//				printf("=== BLOCK END (align=%d bits=0x%x) === \n", cnt, tmp);
				zyxel_get_zyxel_header(&state);
				state.srcblkstart = state.src;
				state.dstblkstart = state.dst;

				/* all src bytes used? */
				if (state.src >= srcbuf + srcsize)
					break;

				continue;
			}

			uint8_t *dict = state.dst - offset;
			if (dict < dstbuf) {
				printf("lzs_unpack: invalid dict: %p < %p (tag=%d, offset=0x%x)\n",
					dict, dstbuf, tag, offset);
				break;
			}

			uint32_t len = zyxel_get_len(&state);
//			printf("compressed string, offset(%d)=0x%03x len=0x%04x\n", tag, offset, len);

			while (len--)
				*(state.dst)++ = *dict++;
		}
	}

	//printf("lzs_unpack: decompressed %d (%d) bytes to %d (%d) bytes\n",
		//(state.src - srcbuf), srcsize, (state.dst - dstbuf), dstsize);

	return state.dst - dstbuf;
}
