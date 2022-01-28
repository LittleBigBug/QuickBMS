/*  FastAri 0.3 - A Fast Block-Based Bitwise Arithmetic Compressor
Copyright (C) 2013  David Catt */

/*#define FA_UNSAFE_DECODE*/
/*#define FA_USE_EOF*/
#include <stdlib.h>
#define ORDER 17
#define ADAPT 4
const int mask = (1 << ORDER) - 1;
int fa_compress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen) {
	unsigned int low=0, mid, high=0xFFFFFFFF;
	size_t ipos, opos=0;
	int bp, bv;
	int ctx;
	/* Initialize model */
	unsigned short* mdl = malloc((mask + 1) * sizeof(unsigned short));
	if(!mdl) return 1;
	for(ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Walk through input */
	for(ipos = 0; ipos < ilen; ++ipos) {
#ifdef FA_USE_EOF
		/* Encode not EOF */
		++low;
		if((high ^ low) < 0x1000000) {
			obuf[opos] = high >> 24;
			++opos;
			high = (high << 8) | 255;
			low <<= 8;
		}
#endif
		/* Encode byte */
		bv = ibuf[ipos];
		for(bp = 7; bp >= 0; --bp) {
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if(bv&1) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				ctx = ((ctx << 1) | 1) & mask;
			} else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			bv >>= 1;
			while((high ^ low) < 0x1000000) {
				obuf[opos] = high >> 24;
				++opos;
				high = (high << 8) | 0xFF;
				low <<= 8;
			}
		}
	}
	/* Flush coder */
	obuf[opos] = (low >> 24); ++opos;
	obuf[opos] = (low >> 16) & 0xFF; ++opos;
	obuf[opos] = (low >> 8) & 0xFF; ++opos;
	obuf[opos] = low & 0xFF; ++opos;
#ifdef FA_USE_EOF
	obuf[opos] = 0; ++opos;
#endif
	/* Cleanup */
	free(mdl);
	*olen = opos;
	return 0;
}
int fa_decompress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen) {
	unsigned int low = 0, mid, high = 0xFFFFFFFF, cur = 0, c;
	size_t opos = 0;
#ifdef FA_UNSAFE_DECODE
	size_t len = olen;
#endif
	int bp, bv;
	int ctx;
	const unsigned char* ibuf_end = ibuf + ilen;
	/* Initialize model */
	unsigned short* mdl = malloc((mask + 1) * sizeof(unsigned short));
	if(!mdl) return 1;
	for(ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Initialize coder */
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	/* Walk through input */
#ifdef FA_UNSAFE_DECODE
	while(len) {
#else
	while(1) {
#endif
#ifdef FA_USE_EOF
		if(cur <= low) {
			free(mdl);
			*olen = opos;
			return 0;
		}
		++low;
		if((high ^ low) < 0x1000000) {
			high = (high << 8) | 0xFF;
			low <<= 8;
			c = *ibuf; ++ibuf;
			cur = (cur << 8) | c;
#ifndef FA_UNSAFE_DECODE
			if(ibuf >= ibuf_end) {
				free(mdl);
				*olen = opos;
				return 0;
			}
#endif
		}
#endif
		if(opos >= *olen) break;
		bv = 0;
		for(bp = 0; bp < 8; ++bp) {
			/* Decode bit */
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if(cur <= mid) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				bv |= (1 << bp);
				ctx = ((ctx << 1) | 1) & mask;
			} else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			while((high ^ low) < 0x1000000) {
				high = (high << 8) | 0xFF;
				low <<= 8;
				c = *ibuf; ++ibuf;
				cur = (cur << 8) | c;
#ifndef FA_UNSAFE_DECODE
				if(ibuf >= ibuf_end) {
					free(mdl);
					*olen = opos;
					return 0;
				}
#endif
			}
		}
		*obuf = bv;
		++obuf;
#ifdef FA_UNSAFE_DECODE
		--len;
#else
		++opos;
#endif
	}
	/* Cleanup */
	free(mdl);
#ifdef FA_UNSAFE_DECODE
	return 0;
#else
	if(ibuf_end == ibuf) {
		*olen = opos;
		return 0;
	} else {
		return 2;
	}
#endif
}
