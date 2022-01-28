// modified by Luigi Auriemma
/*
 *  dos-cc.c -- .CC file reader
 *  Copyright (C) 2011 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int16_t    sword;
typedef uint16_t   word; /* 16-bit */
typedef uint8_t    byte; /* 8-bit */
typedef uint32_t word24; /* 24-bit :( */


#define MBUFFER_SIZE 1024
#define MBUFFER_EDGE (MBUFFER_SIZE - 3)



/*
 * Returns Number of bytes read, 0 on error 
 */
int KB_funLZW(char *result, unsigned int max, unsigned char *in, int insz) {
	int /*i,*/ n, j;

	unsigned int res_pos = 0;

	/*
	 * The data is kept in BIT-positioned "blocks".
	 * We use several variables to iterate it with some
	 * level of sanity:
	 */
	int pos = 0;	/* Position in bits */

	int byte_pos = 0; /* Position in bytes */
	int bit_pos = 0; /* Extra shift in bits */

	/*
	 * Each "block" can take from 9 to 12 bits of data.
	 * Last 8 bits are "the value" 
	 * First N bits are "the key"
	 */
	int step = 9;	/* Bits per step */

	/* Those masks help us get the relevant bits */ 
	word keyMask[4] = {
		0x01FF, 	// 0001 1111 
		0x03FF,		// 0011 1111
		0x07FF,		// 0111 1111
		0x0FFF,		// 1111 1111
	};

	/*
	 * If the "the key" is unset (00), the value is read as-is.
	 * Otherwise, the whole "block" (N+8 bytes) is treated
	 * as a dictionary key, and the dictionary is queried.
	 * The exceptions are the 0x0100 value, which means
	 * the dictionary must be cleared, and the 0x0101 value,
	 * which marks END OF FILE.
	 */

	/*
	 * The dictionary.
	 * Each entry consists of an index to a previous entry
	 * and a value, forming a tree.
	 */
	word dict_key[768 * 16] = { 0 };
	char dict_val[768 * 16];
	word dict_index = 0x0102; /* Start populating dictionary from 0x0102 */
	word dict_range = 0x0200; /* Allow that much entries before increasing step */

	/* Since data stored this way is backwards, we need a small queue */
	char queue[0xFF];
	int queued = 0;

	/* Read buffer */ 
	char mbuffer[MBUFFER_SIZE];

	/* Data */
	word next_index = 0;	/* block of data we currently examine */

	char last_char  = 0;	/* value from previous iteration */
	word last_index = 0;	/* block from previous iteration */

	sword big_index;	/* temp. variable to safely load and shift 3 bytes */
	word keep_index;	/* temp. variable to keep "next_index" before making it "last_index" */
	int reset_hack=0;	/* HACK -- delay dictionary reset to next iteration */ 

	/* Read first chunk of data */
	//n = KB_fread(mbuffer, sizeof(char), MBUFFER_SIZE, f);
    n = MBUFFER_SIZE; if(n > insz) n = insz;
    memcpy(mbuffer, in, n); in += n; insz -= n;

	/* Decompress */
	while (1)
	{
		/* We need a dictionary reset */
		if (reset_hack) 
		{
			step = 9;
			dict_range = 0x0200;
			dict_index = 0x0102;
		}

		/* Since "pos" is in bits, we get position in bytes + small offset in bits */
		byte_pos = pos / 8;
		bit_pos = pos % 8;

		pos += step;	/* And advance to the next chunk */

		/* Edge of buffer, read more data from file */
		if (byte_pos >= MBUFFER_EDGE)
		{
				int bytes_extra = MBUFFER_SIZE - byte_pos;//~= 3
				int bytes_left = MBUFFER_SIZE - bytes_extra;//~= 1021

				/* Copy leftovers */
				for (j = 0; j < bytes_extra; j++) mbuffer[j] = mbuffer[bytes_left + j];

				/* Read in the rest */				
				//n = KB_fread(&mbuffer[bytes_extra], sizeof(char), bytes_left, f);
                n = bytes_left; if(n > insz) n = insz;
                memcpy(&mbuffer[bytes_extra], in, n); in += n; insz -= n;

				/* Reset cursor */
				pos = bit_pos + step;	/* Add all unused bits */
				byte_pos = 0;
				/* On dictionary reset, use byte offset as bit offset*/
				if (reset_hack) bit_pos = bytes_extra;
		}
#ifdef DEBUG
KB_debuglog(0, "%04d\t0x%04X:%01X\t", pos, byte_pos, bit_pos);
#endif
		/* Read index from position "byte_pos", bit offset "bit_pos" */
		big_index = 
			((mbuffer[byte_pos+2] & 0x00FF) << 16) | 
			((mbuffer[byte_pos+1] & 0x00FF) << 8) | 
			(mbuffer[byte_pos] & 0x00FF);

		big_index >>= bit_pos;
		big_index &= 0x0000FFFF;

		next_index = big_index;
		next_index &= keyMask[ (step - 9) ];

		/* Apply the value as-is, continuing with dictionary reset, C) */
		if (reset_hack) 
		{
			/* Save index */
			last_index = next_index;
			/* Output char value */
			last_char = (next_index & 0x00FF);
			result[res_pos++] = last_char;
			/* We're done with the hack */
			reset_hack = 0;
			continue;
		}

		if (next_index == 0x0101)	/* End Of File */
		{
			/* DONE */
			break;
		}

		if (next_index == 0x0100) 	/* Reset dictionary */
		{
			/* Postpone it into next iteration */
			reset_hack = 1;
			/* Note: this hack avoids code duplication, but	makes the algorithm
			 * harder to follow. Basically, what happens, when the "reset" 
			 * command is being hit, is that 
			 * A) the dictionary is reset
			 * B) one more value is being read (this is the code duplication bit)
			 * C) the value is applied to output as-is
			 * D) we continue as normal
			 */
			continue;
		}

		/* Remember *real* "next_index" */
		keep_index = next_index;

		/* No dictionary entry to query, step back */
		if (next_index >= dict_index)
		{
			next_index = last_index;
			/* Queue 1 char */
			queue[queued++] = last_char;
		}

		/* Quering dictionary? */
		while (next_index > 0x00ff)
		{
			/* Queue 1 char */
			queue[queued++] = dict_val[next_index];
			/* Next query: */
			next_index = dict_key[next_index];
		}

		/* Queue 1 char */
		last_char = (next_index & 0x00FF);
		queue[queued++] = last_char;

		/* Ensure buffer overflow wouldn't happen */
		if (res_pos + queued > max) break;

		/* Unqueue */
		while (queued) 
		{
			result[res_pos++] = queue[--queued];
		}

		/* Save value to the dictionary */
		dict_key[dict_index] = last_index; /* "goto prev entry" */
		dict_val[dict_index] = last_char;  /* the value */
		dict_index++;

		/* Save *real* "next_index" */
		last_index = keep_index;

		/* Edge of dictionary, increase the bit-step, making range twice as large. */
		if (dict_index >= dict_range && step < 12) 
		{
			step += 1;
			dict_range *= 2;
		}
	}

	return res_pos;
}



static char *dst;
static int dst_max;
static int step = 9;	/* Bits per step */
static int err = 0; /* Loop breaker */
static unsigned long total_bits = 0; /* Position in bits */
static	int dst_len = 0; /* Position in bytes / Bytes written */
static	int bit_pos = 0; /* Extra shift in bits */

	static void write_bits(word x) {
		word24 big_index = 0;
//printf("[%d] Wrote value: %08x\n", dst_len, x);
		/* See if buffer is large enough */
		if (dst_len + 3 > dst_max) {
			err = 1;
			dst_len = 0;
			bit_pos = 0;
			/* ^ set everything to 0 */
			return;
		}

		big_index = x << bit_pos;

/* NOTE: this chunk should probably be rewritten with endianess in mind */
		byte a = big_index >> 16;
		byte b = big_index >> 8;
		byte c = big_index >> 0;
/* END NOTE */

		dst[dst_len+0] |= c;
		dst[dst_len+1] |= b;
		dst[dst_len+2] |= a;

		total_bits += step;

		dst_len = total_bits / 8;
		bit_pos = total_bits % 8;
	}



int DOS_LZW(char *_dst, int _dst_max, char *src, int src_len) {

static const int DICT_START =  	0x0102;
static const int DICT_BOUNDRY =	0x0200;
static const int MAX_BITS =	12;
static const int CMD_RESET =	0x0100;
static const int CMD_END = 	0x0101;

#define MAX_DICT_SIZE (sizeof(dict_t) * (DICT_BOUNDRY * 128 + 1))

typedef struct {   
	word next[256];
} dict_t;


	/*
	 * The data is kept in BIT-positioned "blocks".
	 * We use several variables to iterate it with some
	 * level of sanity:
	 */
	total_bits = 0; /* Position in bits */

    dst = _dst;
    dst_max = _dst_max;
	dst_len = 0; /* Position in bytes / Bytes written */
	bit_pos = 0; /* Extra shift in bits */
	/*
	 * Each "block" can take from 9 to 12 bits of data.
	 * Last 8 bits are "the value"
	 * First N bits are "the key"
	 */
	step = 9;	/* Bits per step */

	dict_t *dict;
	word    dict_index = 0x0102; /* Start populating dictionary from 0x0102 */
	word    dict_range = 0x0200; /* Allow that much entries before increasing step */

	word last_value;	/* value from previous iteration */
	//byte next_value;	/* value we currently examine */
	err = 0; /* Loop breaker */

	dict = malloc(MAX_DICT_SIZE);
	memset(dict, 0, MAX_DICT_SIZE);

	write_bits(CMD_RESET);

	last_value = (byte) *(src++);
	src_len--;
	while (src_len-- > 0 && !err) {
		word query = last_value & 0xFFFF; /* cast down from word */
		byte next_value = *(src++);

		/* Query dictionary */
		if (dict[query].next[next_value]) {
			last_value = dict[query].next[next_value];
		} else {
			/* Not found, so we add it (and output as is) */
			write_bits(last_value);
			dict[query].next[next_value] = dict_index++;
			last_value = next_value;
		}

		/* When dictionary is overflown, */
		if (dict_index > dict_range) {

			/* Increase bit step */
			if (step < MAX_BITS) {

				dict_range *= 2;
				step++;

			} else { /* and if we can't */ 

				/* Reset dictionary */
				write_bits(CMD_RESET);

				step = 9;
				dict_range = DICT_BOUNDRY;
				dict_index = DICT_START;

				memset(dict, 0, MAX_DICT_SIZE);

			}
		}
	}

	write_bits(last_value);
	write_bits(CMD_END);

	dst_len += (bit_pos ? 1 : 0); 

	free(dict);
	return dst_len;
}

