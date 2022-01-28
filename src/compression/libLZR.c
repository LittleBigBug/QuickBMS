/*
 * libLZR.c
 * 
 * This file is used to de- and encode LZR streams (as used in the Sony PSP firmware)
 * libLZR Version 0.11 by BenHur - http://www.psp-programming.com/benhur
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "libLZR.h"

void LZRFillBuffer(unsigned int *test_mask, unsigned int *mask, unsigned int *buffer, unsigned char **next_in) {
	/* if necessary: fill up in buffer and shift mask */
	if (*test_mask <= 0x00FFFFFFu) {
		(*buffer) = ((*buffer) << 8) + *(*next_in)++;
		*mask = *test_mask << 8;
	}
}

char LZRNextBit(unsigned char *buf_ptr1, int *number, unsigned int *test_mask, unsigned int *mask, unsigned int *buffer, unsigned char **next_in) {
	/* extract and return next bit of information from in stream, update buffer and mask */
	LZRFillBuffer(test_mask, mask, buffer, next_in);
	unsigned int value = (*mask >> 8) * (*buf_ptr1);
	if (test_mask != mask) *test_mask = value;
	*buf_ptr1 -= *buf_ptr1 >> 3;
	if (number) (*number) <<= 1;
	if (*buffer < value) {
		*mask = value;		
		*buf_ptr1 += 31;		
		if (number) (*number)++;
		return 1;
	} else {
		*buffer -= value;
		*mask -= value;
		return 0;
	}
}

int LZRGetNumber(signed char n_bits, unsigned char *buf_ptr, char inc, char *flag, unsigned int *mask, unsigned int *buffer, unsigned char **next_in) {
	/* extract and return a number (consisting of n_bits bits) from in stream */
	int number = 1;
	if (n_bits >= 3) {
		LZRNextBit(buf_ptr+3*inc, &number, mask, mask, buffer, next_in);
		if (n_bits >= 4) {
			LZRNextBit(buf_ptr+3*inc, &number, mask, mask, buffer, next_in);
			if (n_bits >= 5) {
				LZRFillBuffer(mask, mask, buffer, next_in);
				for (; n_bits >= 5; n_bits--) {
					number <<= 1;
					(*mask) >>= 1;
					if (*buffer < *mask) number++; else (*buffer) -= *mask;
				}
			}
		}
	}
	*flag = LZRNextBit(buf_ptr, &number, mask, mask, buffer, next_in);
	if (n_bits >= 1) {
		LZRNextBit(buf_ptr+inc, &number, mask, mask, buffer, next_in);
		if (n_bits >= 2) {
			LZRNextBit(buf_ptr+2*inc, &number, mask, mask, buffer, next_in);
		}
	}	
	return number;
}

int LZRDecompress(void *out, unsigned int out_capacity, void *in, void *in_end) { 
	unsigned char **next_in, *tmp, *next_out, *out_end, *next_seq, *seq_end, *buf_ptr1, *buf_ptr2;
	unsigned char last_char = 0;
	int seq_len, seq_off, n_bits, buf_off = 0, i, j;	
	unsigned int mask = 0xFFFFFFFF, test_mask;
	char flag;
	
	signed char type = *(signed char*)in;
	unsigned int buffer = ((unsigned int)*(unsigned char*)(in+1) << 24) + 
	                      ((unsigned int)*(unsigned char*)(in+2) << 16) + 
	                      ((unsigned int)*(unsigned char*)(in+3) <<  8) + 
	                      ((unsigned int)*(unsigned char*)(in+4)      );	
	next_in = (in_end) ? in_end : &tmp; //use user provided counter if available
	*next_in = in + 5;
	next_out = out;
	out_end = out + out_capacity;

	if (type < 0) { 
		
		/* copy from stream without decompression */

		seq_end = next_out + buffer;
		if (seq_end > out_end) return LZR_ERROR_BUFFER_SIZE;
		while (next_out < seq_end) {
			*next_out++ = *(*next_in)++;
		} 
		(*next_in)++; //skip 1 byte padding
		return next_out - (unsigned char*)out; 

	}

	/* create and init buffer */
	unsigned char *buf = (unsigned char*)malloc(2800);
	if (!buf) return LZR_ERROR_MEM_ALLOC;
	for (i = 0; i < 2800; i++) buf[i] = 0x80;

	while (1) {

		buf_ptr1 = buf + buf_off + 2488;
		if (!LZRNextBit(buf_ptr1, 0, &mask, &mask, &buffer, next_in)) {

			/* single new char */

			if (buf_off > 0) buf_off--;
			if (next_out == out_end) return LZR_ERROR_BUFFER_SIZE;
			buf_ptr1 = buf + (((((((int)(next_out - (unsigned char*)out)) & 0x07) << 8) + last_char) >> type) & 0x07) * 0xFF - 0x01;
			for (j = 1; j <= 0xFF; ) {
				LZRNextBit(buf_ptr1+j, &j, &mask, &mask, &buffer, next_in);
			}
			*next_out++ = j;

		} else {                       

			/* sequence of chars that exists in out stream */

			/* find number of bits of sequence length */			
			test_mask = mask;
			n_bits = -1;
			do {
				buf_ptr1 += 8;
				flag = LZRNextBit(buf_ptr1, 0, &test_mask, &mask, &buffer, next_in);
				n_bits += flag;
			} while ((flag != 0) && (n_bits < 6));
			
			/* find sequence length */
			buf_ptr2 = buf + n_bits + 2033;
			j = 64;
			if ((flag != 0) || (n_bits >= 0)) {
				buf_ptr1 = buf + (n_bits << 5) + (((((int)(next_out - (unsigned char*)out)) << n_bits) & 0x03) << 3) + buf_off + 2552;
				seq_len = LZRGetNumber(n_bits, buf_ptr1, 8, &flag, &mask, &buffer, next_in);
				if (seq_len == 0xFF) return next_out - (unsigned char*)out; //end of data stream
				if ((flag != 0) || (n_bits > 0)) {
					buf_ptr2 += 56;
					j = 352;
				}
			} else {
				seq_len = 1;
			}

			/* find number of bits of sequence offset */			
			i = 1;
			do {
				n_bits = (i << 4) - j;
				flag = LZRNextBit(buf_ptr2 + (i << 3), &i, &mask, &mask, &buffer, next_in);
			} while (n_bits < 0);

			/* find sequence offset */
			if (flag || (n_bits > 0)) {
				if (!flag) n_bits -= 8;
				seq_off = LZRGetNumber(n_bits/8, buf+n_bits+2344, 1, &flag, &mask, &buffer, next_in);
			} else {
				seq_off = 1;
			}

			/* copy sequence */
			next_seq = next_out - seq_off;
			if (next_seq < (unsigned char*)out) return LZR_ERROR_INPUT_STREAM;
			seq_end = next_out + seq_len + 1;
			if (seq_end > out_end) return LZR_ERROR_BUFFER_SIZE;
			buf_off = ((((int)(seq_end - (unsigned char*)out))+1) & 0x01) + 0x06;
			do {
				*next_out++ = *next_seq++;
			} while (next_out < seq_end);

		}
		last_char = *(next_out-1);		
	}
}

int LZRCompress(void *out, unsigned int out_capacity, void *in, unsigned int in_length, char type) {
	unsigned char *next_in, *next_out, *out_end, *seq_end;
	next_in = in;
	next_out = out;
	out_end = out + out_capacity;

	if (type < 0) { 
		
		/* copy from stream without compression */

		seq_end = next_out + in_length + 6;
		if (seq_end > out_end) return LZR_ERROR_BUFFER_SIZE;
		*next_out++ = type;
		*next_out++ = in_length >> 24;
		*next_out++ = in_length >> 16;
		*next_out++ = in_length >>  8;
		*next_out++ = in_length      ;
		while (next_out < seq_end-1) {
			*next_out++ = *next_in++;
		} 
		*next_out++ = 0; //add 1 byte padding
		return next_out - (unsigned char*)out; 

	}

	/* real lzr compression not yet supported */

	return LZR_ERROR_UNSUPPORTED;
}

