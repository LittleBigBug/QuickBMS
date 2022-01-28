/*\
 *
 * ProVEST-4, ProVEST-16 and ProVEST-32 ciphers
 * Designed by Sean O'Neil
 * Copyright (c) CB Capital Management
 *
\*/

#include <stdlib.h>
#include <memory.h>
#include "ecrypt-sync-ae.h"

#define VEST_VARIANT 1
#define VEST_WORDS 20

#if (VEST_VARIANT == 1)
#define INCLUDE_VEST_4
#elif (VEST_VARIANT == 2)
#define INCLUDE_VEST_16
#elif (VEST_VARIANT == 3)
#define INCLUDE_VEST_32
#endif

#define vest_4_bits				83
#define vest_16_bits			331
#define vest_32_bits			587
#define vest_input_bits			10
#define vest_sealing_rounds		31
#define vest_counters			16
#define hashing_counters		8
#define	vest_f_width			5
#define	rns_f_width				5
#define bit(x,n)				(((x)>>(n))&1)
#define bit8(x,n)				((((x)[(n)>>3])>>((n)&7))&1)	/* Intel processors perform ((n) & 7) automatically; remove & 7 to increase speed */
#define bit32(x,n)				((((x)[(n)>>5])>>((n)&31))&1)	/* Intel processors perform ((n) & 31) automatically; remove & 31 to increase speed */
#define f5(x,a,b,c,d,e)			(bit32(x,a)|(bit32(x,b)<<1)|(bit32(x,c)<<2)|(bit32(x,d)<<3)|(bit32(x,e)<<4))
#define ob(i)					bit32 (vest_state + vest_counters + 1, i)
#define vest_rotate(x,n,w)		((((x)<<(n))^((x)>>((w)-(n))))&((1<<(w))-1))
#define vest_swap32(x,y)		((t)=(x),(x)=(y),(y)=(t))
#define vest_f_bits				(1 << vest_f_width)
#define vest_f_words			((vest_f_bits+31) >> 5)
#define rns_f_bits				(1 << rns_f_width)
#define rns_f_words				((rns_f_bits+31) >> 5)
#define vest_4_core_words		((vest_4_bits+31)/32)
#define vest_4_words			(vest_counters+1+vest_4_core_words)
#define vest_4_family_words		(vest_counters + (vest_4_bits-vest_f_width)*2)
#define vest_16_core_words		((vest_16_bits+31)/32)
#define vest_16_words			(vest_counters+1+vest_16_core_words)
#define vest_16_family_words	(vest_counters + (vest_16_bits-vest_f_width)*2)
#define vest_32_core_words		((vest_32_bits+31)/32)
#define vest_32_words			(vest_counters+1+vest_32_core_words)
#define vest_32_family_words	(vest_counters + (vest_32_bits-vest_f_width)*2)

/*\
 *
 * total vest_state size in 32-bit words =
 * 16 (counters) + 1 (diffusor) + vest_W_words {3|11|19} (core state)
 *
 * VEST-4:	20 words
 * VEST-16:	28 words
 * VEST-32:	36 words
 *
 * total vest_family size in 32-bit words =
 * 16 (counters) + vest_W_words {3|11|19} (vest_f indexes) + vest_W_words {3|11|19} (p5 indexes)
 *
 * VEST-4:	22 words
 * VEST-16:	38 words
 * VEST-32: 54 words
 *
\*/

typedef u32			vest_core_inputs[vest_f_width+1];	/* the output bit index is included in the list */

/* RNS counters widths */

static const u32	rns_w[32] = {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};

/* RNS counters feedback functions */

static const u32	rns_f[32][rns_f_words] =
{
	{0xA1705DB9},	/* [ 0]: 1009, 1039 */
	{0x947A15CB},	/* [ 1]:  997, 1051 */
	{0x85A1BDC3},	/* [ 2]:  919, 1129 */
	{0xD0C274F5},	/* [ 3]:  877, 1171 */
	{0xEA4308DF},	/* [ 4]:  811, 1237 */
	{0xF031C87D},	/* [ 5]:  769, 1279 */
	{0xB8CA56E1},	/* [ 6]:  757, 1291 */
	{0xD384CB59},	/* [ 7]:  751, 1297 */
	{0xC479F503},	/* [ 8]:  727, 1321 */
	{0xA6945DE1},	/* [ 9]:  619, 1429 */
	{0xBA8E0397},	/* [10]:  601, 1447 */
	{0xE378A90D},	/* [11]:  577, 1471 */
	{0xFA612475},	/* [12]:  499, 1549 */
	{0xD18453AF},	/* [13]:  463,  491, 523, 571 */
	{0xC5A026F7},	/* [14]:  443,  449, 557, 599 */
	{0x86E4127F},	/* [15]:  439, 1609 */
	{0xF4AC89E1},	/* [16]:  503,  521 */
	{0x963E41A7},	/* [17]:  467,  557 */
	{0xBA043D8F},	/* [18]:  461,  563 */
	{0xA1FE08D3},	/* [19]:  431,  593 */
	{0xC4D835E9},	/* [20]:  383,  641 */
	{0xF90E8C63},	/* [21]:  347,  677 */
	{0xE8F463A1},	/* [22]:  281,  743 */
	{0xBF2C980D},	/* [23]:  263,  761 */
	{0x971D843B},	/* [24]:  251,  773 */
	{0xC1B0F547},	/* [25]:  227,  797 */
	{0x8AF17435},	/* [26]:  197,  827 */
	{0xE6247CB1},	/* [30]:  191,  239, 241, 353 */
	{0xD270F389},	/* [27]:  167,  857 */
	{0xF7C43681},	/* [28]:  163,  211, 313, 337 */
	{0xD34C0FA9},	/* [29]:  151,  223, 283, 367 */
	{0xE6AD9C81}	/* [31]:  149,  233, 293, 349 */
};

static void update_rns_counters (u32 *rns_counter, const u32 *rns_family)
{
	u32				i = vest_counters, *c = rns_counter;
	const u32		*r = rns_family;
	
	for (; i; i--, r++, c++)
	{
		*c = vest_rotate (*c, 1, rns_w[*r]) ^ bit32 (rns_f[*r], f5 (c, 0, 1, 2, 6, 7));
	}
}

/* list of input bit permutations; slightly extended for faster keying */

static const u32	p5[128][5] =
{
	{0,1,2,3,4}, {1,0,2,3,4}, {0,2,1,3,4}, {2,0,1,3,4}, {1,2,0,3,4}, {2,1,0,3,4},
	{0,1,3,2,4}, {1,0,3,2,4}, {0,3,1,2,4}, {3,0,1,2,4}, {1,3,0,2,4}, {3,1,0,2,4},
	{0,2,3,1,4}, {2,0,3,1,4}, {0,3,2,1,4}, {3,0,2,1,4}, {2,3,0,1,4}, {3,2,0,1,4},
	{1,2,3,0,4}, {2,1,3,0,4}, {1,3,2,0,4}, {3,1,2,0,4}, {2,3,1,0,4}, {3,2,1,0,4},
	{0,1,2,4,3}, {1,0,2,4,3}, {0,2,1,4,3}, {2,0,1,4,3}, {1,2,0,4,3}, {2,1,0,4,3},
	{0,1,4,2,3}, {1,0,4,2,3}, {0,4,1,2,3}, {4,0,1,2,3}, {1,4,0,2,3}, {4,1,0,2,3},
	{0,2,4,1,3}, {2,0,4,1,3}, {0,4,2,1,3}, {4,0,2,1,3}, {2,4,0,1,3}, {4,2,0,1,3},
	{1,2,4,0,3}, {2,1,4,0,3}, {1,4,2,0,3}, {4,1,2,0,3}, {2,4,1,0,3}, {4,2,1,0,3},
	{0,1,3,4,2}, {1,0,3,4,2}, {0,3,1,4,2}, {3,0,1,4,2}, {1,3,0,4,2}, {3,1,0,4,2},
	{0,1,4,3,2}, {1,0,4,3,2}, {0,4,1,3,2}, {4,0,1,3,2}, {1,4,0,3,2}, {4,1,0,3,2},
	{0,3,4,1,2}, {3,0,4,1,2}, {0,4,3,1,2}, {4,0,3,1,2}, {3,4,0,1,2}, {4,3,0,1,2},
	{1,3,4,0,2}, {3,1,4,0,2}, {1,4,3,0,2}, {4,1,3,0,2}, {3,4,1,0,2}, {4,3,1,0,2},
	{0,2,3,4,1}, {2,0,3,4,1}, {0,3,2,4,1}, {3,0,2,4,1}, {2,3,0,4,1}, {3,2,0,4,1},
	{0,2,4,3,1}, {2,0,4,3,1}, {0,4,2,3,1}, {4,0,2,3,1}, {2,4,0,3,1}, {4,2,0,3,1},
	{0,3,4,2,1}, {3,0,4,2,1}, {0,4,3,2,1}, {4,0,3,2,1}, {3,4,0,2,1}, {4,3,0,2,1},
	{2,3,4,0,1}, {3,2,4,0,1}, {2,4,3,0,1}, {4,2,3,0,1}, {3,4,2,0,1}, {4,3,2,0,1},
	{1,2,3,4,0}, {2,1,3,4,0}, {1,3,2,4,0}, {3,1,2,4,0}, {2,3,1,4,0}, {3,2,1,4,0},
	{1,2,4,3,0}, {2,1,4,3,0}, {1,4,2,3,0}, {4,1,2,3,0}, {2,4,1,3,0}, {4,2,1,3,0},
	{1,3,4,2,0}, {3,1,4,2,0}, {1,4,3,2,0}, {4,1,3,2,0}, {3,4,1,2,0}, {4,3,1,2,0},
	{2,3,4,1,0}, {3,2,4,1,0}, {2,4,3,1,0}, {4,2,3,1,0}, {3,4,2,1,0}, {4,3,2,1,0},
	{3,4,0,1,2}, {2,4,1,0,3}, {1,4,2,0,3}, {4,3,2,1,0}, {3,2,1,0,4}, {2,1,0,4,3},
	{1,0,4,3,2}, {0,4,3,2,1}
};

/* 5x5 s-box */

static const u32	vest_sbox[vest_f_width][vest_f_words] =
{
	{0x37D25607}, {0x520AF73A}, {0x872CC65D}, {0xD7C105DA}, {0x348C1FF4}
};

/* VEST core accumulator feedback functions */

#include "vest_f.h"

/* note: the preceding 32-bit word stores input bits, hence no 'input' parameter */

static void update_vest_core (u32 *v, const u32 *vest_core_family, const vest_core_inputs *vest_structure, const u32 core_bits)
{
	u32				i, x[vest_32_core_words];
	const u32		*f = vest_core_family, *p = vest_core_family + core_bits - vest_f_width, *b = vest_structure[0], *j;
	
	memset (x, 0, ((core_bits + 31) & ~31) >> 3);
	for (i = 0; i < vest_f_width; i++, b += vest_f_width+1)
	{
		x[b[5] >> 5] |= (bit32 (vest_sbox[i], v[0] & 31) ^ bit32 (v-1, i)) << (b[5] & 31);
	}
	for (; i < vest_input_bits; i++, f++, p++, b += vest_f_width+1)
	{
		j = p5[*p];
		x[b[5] >> 5] |= (bit32 (vest_f[*f], f5 (v, b[j[0]], b[j[1]], b[j[2]], b[j[3]], b[j[4]])) ^ bit32 (v, i) ^ bit32 (v-1, i)) << (b[5] & 31);
	}
	for (; i < core_bits; i++, f++, p++, b += vest_f_width+1)
	{
		j = p5[*p];
		x[b[5] >> 5] |= (bit32 (vest_f[*f], f5 (v, b[j[0]], b[j[1]], b[j[2]], b[j[3]], b[j[4]])) ^ bit32 (v, i)) << (b[5] & 31);
	}
	memcpy (v, x, ((core_bits + 31) & ~31) >> 3);
}

static void update_diffusor (u32 *v)
{
	u32				x;
	
/* #if (vest_counters = 16) */
	x = (bit(v[16],1) ^ bit(v[ 1],1) ^ bit(v[ 4],1) ^ bit(v[ 5],1) ^ bit(v[11],1) ^ bit(v[13],1) ^ 1);
	x^= (bit(v[16],2) ^ bit(v[ 0],1) ^ bit(v[ 2],1) ^ bit(v[ 6],1) ^ bit(v[ 8],1) ^ bit(v[14],1)    ) << 1;
	x^= (bit(v[16],3) ^ bit(v[ 3],1) ^ bit(v[ 4],1) ^ bit(v[ 7],1) ^ bit(v[10],1) ^ bit(v[15],1)    ) << 2;
	x^= (bit(v[16],4) ^ bit(v[ 0],1) ^ bit(v[ 3],1) ^ bit(v[ 5],1) ^ bit(v[ 9],1) ^ bit(v[12],1)    ) << 3;
	x^= (bit(v[16],5) ^ bit(v[ 1],1) ^ bit(v[ 4],1) ^ bit(v[ 6],1) ^ bit(v[12],1) ^ bit(v[15],1) ^ 1) << 4;
	x^= (bit(v[16],6) ^ bit(v[ 0],1) ^ bit(v[ 7],1) ^ bit(v[ 9],1) ^ bit(v[13],1) ^ bit(v[14],1)    ) << 5;
	x^= (bit(v[16],7) ^ bit(v[ 1],1) ^ bit(v[ 8],1) ^ bit(v[11],1) ^ bit(v[14],1) ^ bit(v[15],1)    ) << 6;
	x^= (bit(v[16],8) ^ bit(v[ 2],1) ^ bit(v[ 5],1) ^ bit(v[ 6],1) ^ bit(v[10],1) ^ bit(v[12],1) ^ 1) << 7;
	x^= (bit(v[16],0) ^ bit(v[ 0],1) ^ bit(v[ 3],1) ^ bit(v[ 7],1) ^ bit(v[ 8],1) ^ bit(v[ 9],1) ^ 1) << 8;
	x^= (bit(v[16],9) ^ bit(v[ 8],1) ^ bit(v[10],1) ^ bit(v[12],1) ^ bit(v[13],1) ^ bit(v[15],1) ^ 1) << 9;
	
	v[16] = x;
}

static void vest_update (u32 *vest_state, const u32 *vest_family, const vest_core_inputs *vest_structure, const u32 vest_bits)
{
	update_vest_core (vest_state + vest_counters + 1, vest_family + vest_counters, vest_structure, vest_bits);
	update_diffusor (vest_state);
	update_rns_counters (vest_state, vest_family);
}

static void vest_hash (u32 *vest_state, const u32 *vest_family, const vest_core_inputs *vest_structure, const u32 vest_bits, const u8 *data, const u32 data_bits)
{
	u32				i, r = 0;
	
	while (r < data_bits)
	{
		vest_update (vest_state, vest_family, vest_structure, vest_bits);
		for (i = 0; i < hashing_counters; i++, r++)
		{
			vest_state[i] ^= bit8 (data, r) << 1;
		}
	}
}

static void vest_hash_seal (u32 *vest_state, const u32 *vest_family, const vest_core_inputs *vest_structure, const u32 vest_bits, const u32 vest_seal)
{
	u32				i, r = 0;
	
	vest_update (vest_state, vest_family, vest_structure, vest_bits);
	for (i = 0; i < hashing_counters; i++, r++) vest_state[i] ^= bit (vest_seal, i);	/* hashing in 0xFF for data, 0x2B or 0xB2 for keys, 0xE4 for IVs, and 0x4E for family generator keying */
	for (r = vest_sealing_rounds; r; r--)
	{
		vest_update (vest_state, vest_family, vest_structure, vest_bits);
	}
}

void vest_set_key (u32 *vest_state, const u32 *vest_family, const vest_core_inputs *vest_structure, const u32 vest_bits, const u8 *key, const u32 key_bits, const u32 key_seal)
{
	u32				i, k, r;
	
	for (r = 0; r < key_bits; r++)
	{
		vest_update (vest_state, vest_family, vest_structure, vest_bits);
		for (i = 0, k = r; i < vest_counters; i++, k++)
		{
			if (k >= key_bits) k = 0;
			vest_state[i] ^= bit8 (key, k) << 1;
		}
	}
	vest_hash (vest_state, vest_family, vest_structure, vest_bits, key, key_bits);
	vest_hash_seal (vest_state, vest_family, vest_structure, vest_bits, key_seal);
}

void vest_set_IV (u32 *vest_state, const u32 *vest_family, const vest_core_inputs *vest_structure, const u32 vest_bits, const u8 *IV, const u32 IV_bits)
{
	vest_hash (vest_state, vest_family, vest_structure, vest_bits, IV, IV_bits);
	vest_hash_seal (vest_state, vest_family, vest_structure, vest_bits, 0xE4);
}

void vest_generate_family (u32 *vest_family, const u32 *vest_root, const u8 *v, const u32 vest_bits)
{
	u32				rns[32], f[1024], r, s, t, i, j;
	
	for (i = 0; i < 32; i++) rns[i] = i;
	for (i = 0; i < 8; i++)
	{
		s = v[i] & 15;
		vest_swap32 (rns[i*2+16], rns[s+16]);
		s = v[i] >> 4;
		vest_swap32 (rns[i*2+17], rns[s+16]);
		s = v[i+8] & 15;
		vest_swap32 (rns[i*2], rns[s]);
		s = v[i+8] >> 4;
		vest_swap32 (rns[i*2+1], rns[s]);
	}
	for (i = 0; i < vest_counters; i++) vest_family[i] = rns[vest_root[i]];
	for (i = 0; i < vest_bits; i++) f[i] = i;
	for (r = 0, i = 16, j = 0; r < vest_bits-vest_f_width; r++, i++, j += 2)
	{
		if (j >= 8) j -= 8, i++;
		s = ((v[i] >> j) + (v[i+1] << (8-j))) & 1023;
		vest_swap32 (f[r], f[s]);
	}
	memcpy (vest_family + vest_counters * sizeof (long), f, (vest_bits-vest_f_width) * sizeof (long));
	for (r = 0; r < vest_bits-vest_f_width; r++, j += 7)
	{
		if (j >= 8) j -= 8, i++;
		vest_family[vest_counters + vest_bits - vest_f_width] = ((v[i] >> j) + (v[i+1] << (8-j))) & 127;
	}
}

#ifdef INCLUDE_VEST_4

#include "vest_4.h"

static void vest_4_set_key (u32 *vest_state, const u32 *vest_family, const u8 *key, const u32 key_bits, const u32 vest_seal)
{
	memset (vest_state, 0, vest_4_words << 2);
	vest_set_key (vest_state, vest_family, vest_4_structure, vest_4_bits, key, key_bits, vest_seal);
}

static void vest_4_set_IV (u32 *vest_state, const u32 *vest_family, const u8 *IV, const u32 IV_bits)
{
	vest_set_IV (vest_state, vest_family, vest_4_structure, vest_4_bits, IV, IV_bits);
}

static u8 vest_4_next_byte (u32 *vest_state, const u32 *vest_family)
{
	u8				x;

	x = (ob(18)^ob(25)^ob(33)^ob(67)^ob(79)^ob(81));
	x|= (ob(29)^ob(41)^ob(46)^ob(51)^ob(63)^ob(64)) << 1;
	x|= (ob(15)^ob(16)^ob(19)^ob(23)^ob(40)^ob(48)) << 2;
	x|= (ob(35)^ob(47)^ob(57)^ob(72)^ob(76)^ob(78)) << 3;
	vest_update (vest_state, vest_family, vest_4_structure, vest_4_bits);
	x|= (ob(18)^ob(25)^ob(33)^ob(67)^ob(79)^ob(81)) << 4;
	x|= (ob(29)^ob(41)^ob(46)^ob(51)^ob(63)^ob(64)) << 5;
	x|= (ob(15)^ob(16)^ob(19)^ob(23)^ob(40)^ob(48)) << 6;
	x|= (ob(35)^ob(47)^ob(57)^ob(72)^ob(76)^ob(78)) << 7;
	vest_update (vest_state, vest_family, vest_4_structure, vest_4_bits);
	return x;
}

static void vest_4_generate_bytes (u32 *vest_state, const u32 *vest_family, u8 *output, const u32 bytes)
{
	u32				r;
	
	for (r = 0; r < bytes; r++)
	{
		output[r] = vest_4_next_byte (vest_state, vest_family);
	}
}

static void vest_4_process_bytes (u32 *vest_state, const u32 *vest_family, const u8 *input, u8 *output, const u32 bytes)
{
	u32				r;
	
	for (r = 0; r < bytes; r++)
	{
		output[r] = input[r] ^ vest_4_next_byte (vest_state, vest_family);
	}
}

void vest_4_generate_family (u32 *vest_family, const u8 *key, const u32 key_bits, const u8 *IV, const u32 IV_bits)
{
	u32				vest_state[vest_4_words];
	u8				vest_output[((vest_4_bits-vest_f_bits)*17+vest_counters*4+7)/8];
	
	vest_4_set_key (vest_state, provest_4, key, key_bits, 0x4E);
	if (IV_bits) vest_4_set_IV (vest_state, provest_4, IV, IV_bits);
	vest_4_generate_bytes (vest_state, provest_4, vest_output, sizeof (vest_output));
	vest_generate_family (vest_family, provest_4, vest_output, vest_4_bits);
}

#endif

#ifdef INCLUDE_VEST_16

#include "vest_16.h"

static void vest_16_set_key (u32 *vest_state, const u32 *vest_family, const u8 *key, const u32 key_bits, const u32 vest_seal)
{
	memset (vest_state, 0, vest_16_words << 2);
	vest_set_key (vest_state, vest_family, vest_16_structure, vest_16_bits, key, key_bits, vest_seal);
}

static void vest_16_set_IV (u32 *vest_state, const u32 *vest_family, const u8 *IV, const u32 IV_bits)
{
	vest_set_IV (vest_state, vest_family, vest_16_structure, vest_16_bits, IV, IV_bits);
}

static u32 vest_16_next (u32 *vest_state, const u32 *vest_family)
{
	u32				x;
	
	x = (ob(42)^ob( 60)^ob( 79)^ob(147)^ob(162)^ob(314));
	x|= (ob(29)^ob(109)^ob(128)^ob(144)^ob(156)^ob(211)) <<  1;
	x|= (ob(30)^ob(159)^ob(206)^ob(225)^ob(260)^ob(318)) <<  2;
	x|= (ob(14)^ob( 45)^ob( 77)^ob(210)^ob(275)^ob(322)) <<  3;
	x|= (ob(22)^ob( 33)^ob( 43)^ob( 81)^ob(305)^ob(324)) <<  4;
	x|= (ob(66)^ob(138)^ob(140)^ob(182)^ob(240)^ob(313)) <<  5;
	x|= (ob(25)^ob( 36)^ob(185)^ob(224)^ob(232)^ob(316)) <<  6;
	x|= (ob(19)^ob( 20)^ob(107)^ob(133)^ob(212)^ob(257)) <<  7;
	x|= (ob(52)^ob( 88)^ob(101)^ob(115)^ob(261)^ob(325)) <<  8;
	x|= (ob(28)^ob( 98)^ob(125)^ob(214)^ob(221)^ob(259)) <<  9;
	x|= (ob(41)^ob(191)^ob(263)^ob(284)^ob(302)^ob(312)) << 10;
	x|= (ob(35)^ob( 37)^ob(241)^ob(252)^ob(291)^ob(292)) << 11;
	x|= (ob(50)^ob(168)^ob(198)^ob(228)^ob(253)^ob(288)) << 12;
	x|= (ob(47)^ob( 53)^ob( 71)^ob( 84)^ob(276)^ob(323)) << 13;
	x|= (ob(55)^ob(131)^ob(229)^ob(256)^ob(287)^ob(308)) << 14;
	x|= (ob(17)^ob(106)^ob(130)^ob(172)^ob(181)^ob(277)) << 15;
	vest_update (vest_state, vest_family, vest_16_structure, vest_16_bits);
	return x;
}

static void vest_16_generate_bytes (u32 *vest_state, const u32 *vest_family, u8 *output, const u32 bytes)
{
	u32				r = 0, x;
	
	for (;;)
	{
		if (r >= bytes) break;
		x = vest_16_next (vest_state, vest_family);
		output[r++] = (u8) x;
		if (r >= bytes) break;
		output[r++] = (u8) (x >>= 8);
	}
}

static void vest_16_process_bytes (u32 *vest_state, const u32 *vest_family, const u8 *input, u8 *output, const u32 bytes)
{
	u32				r = 0, x;
	
	for (;;)
	{
		if (r >= bytes) break;
		x = vest_16_next (vest_state, vest_family);
		output[r] = input[r] ^ (u8) x; r++;
		if (r >= bytes) break;
		output[r] = input[r] ^ (u8) (x >>= 8); r++;
	}
}

void vest_16_generate_family (u32 *vest_family, const u8 *key, const u32 key_bits, const u8 *IV, const u32 IV_bits)
{
	u32				vest_state[vest_16_words];
	u8				vest_output[((vest_16_bits-vest_f_bits)*17+vest_counters*4+7)/8];
	
	vest_16_set_key (vest_state, provest_16, key, key_bits, 0x4E);
	if (IV_bits) vest_16_set_IV (vest_state, provest_16, IV, IV_bits);
	vest_16_generate_bytes (vest_state, provest_16, vest_output, sizeof (vest_output));
	vest_generate_family (vest_family, provest_16, vest_output, vest_16_bits);
}

#endif

#ifdef INCLUDE_VEST_32

#include "vest_32.h"

static void vest_32_set_key (u32 *vest_state, const u32 *vest_family, const u8 *key, const u32 key_bits, const u32 vest_seal)
{
	memset (vest_state, 0, vest_32_words << 2);
	vest_set_key (vest_state, vest_family, vest_32_structure, vest_32_bits, key, key_bits, vest_seal);
}

static void vest_32_set_IV (u32 *vest_state, const u32 *vest_family, const u8 *IV, const u32 IV_bits)
{
	vest_set_IV (vest_state, vest_family, vest_32_structure, vest_32_bits, IV, IV_bits);
}

static u32 vest_32_next (u32 *vest_state, const u32 *vest_family)
{
	u32				x;
	
	x = (ob(235)^ob(269)^ob(293)^ob(319)^ob(344)^ob(375));
	x|= (ob(159)^ob(182)^ob(312)^ob(376)^ob(418)^ob(437)) <<  1;
	x|= (ob( 55)^ob( 98)^ob(193)^ob(266)^ob(281)^ob(551)) <<  2;
	x|= (ob(177)^ob(244)^ob(349)^ob(374)^ob(432)^ob(457)) <<  3;
	x|= (ob( 27)^ob( 94)^ob(125)^ob(354)^ob(560)^ob(561)) <<  4;
	x|= (ob(210)^ob(346)^ob(347)^ob(363)^ob(424)^ob(428)) <<  5;
	x|= (ob( 11)^ob(277)^ob(285)^ob(313)^ob(334)^ob(483)) <<  6;
	x|= (ob( 31)^ob(117)^ob(191)^ob(194)^ob(256)^ob(366)) <<  7;
	x|= (ob( 63)^ob( 99)^ob(196)^ob(241)^ob(362)^ob(540)) <<  8;
	x|= (ob( 21)^ob( 42)^ob(144)^ob(405)^ob(536)^ob(557)) <<  9;
	x|= (ob(239)^ob(373)^ob(415)^ob(422)^ob(450)^ob(515)) << 10;
	x|= (ob( 40)^ob(254)^ob(338)^ob(429)^ob(486)^ob(491)) << 11;
	x|= (ob( 10)^ob(133)^ob(292)^ob(335)^ob(407)^ob(481)) << 12;
	x|= (ob( 39)^ob(431)^ob(471)^ob(489)^ob(525)^ob(575)) << 13;
	x|= (ob(126)^ob(263)^ob(323)^ob(343)^ob(519)^ob(553)) << 14;
	x|= (ob(106)^ob(139)^ob(147)^ob(382)^ob(499)^ob(571)) << 15;
	x|= (ob(104)^ob(134)^ob(306)^ob(341)^ob(348)^ob(357)) << 16;
	x|= (ob(102)^ob(216)^ob(361)^ob(416)^ob(454)^ob(533)) << 17;
	x|= (ob( 53)^ob(146)^ob(178)^ob(234)^ob(443)^ob(531)) << 18;
	x|= (ob(107)^ob(152)^ob(198)^ob(261)^ob(417)^ob(498)) << 19;
	x|= (ob( 37)^ob(129)^ob(170)^ob(190)^ob(351)^ob(355)) << 20;
	x|= (ob( 69)^ob( 92)^ob(243)^ob(395)^ob(419)^ob(508)) << 21;
	x|= (ob(110)^ob(115)^ob(464)^ob(530)^ob(543)^ob(573)) << 22;
	x|= (ob( 22)^ob(212)^ob(467)^ob(526)^ob(532)^ob(548)) << 23;
	x|= (ob(108)^ob(233)^ob(315)^ob(468)^ob(574)^ob(579)) << 24;
	x|= (ob( 17)^ob( 84)^ob(321)^ob(408)^ob(449)^ob(555)) << 25;
	x|= (ob(127)^ob(128)^ob(438)^ob(445)^ob(487)^ob(496)) << 26;
	x|= (ob( 89)^ob(207)^ob(273)^ob(369)^ob(409)^ob(563)) << 27;
	x|= (ob( 73)^ob(131)^ob(138)^ob(336)^ob(480)^ob(562)) << 28;
	x|= (ob( 26)^ob(119)^ob(169)^ob(189)^ob(214)^ob(413)) << 29;
	x|= (ob( 75)^ob( 97)^ob(205)^ob(232)^ob(448)^ob(501)) << 30;
	x|= (ob( 12)^ob( 20)^ob(290)^ob(305)^ob(488)^ob(521)) << 31;
	vest_update (vest_state, vest_family, vest_32_structure, vest_32_bits);
	return x;
}

static void vest_32_generate_bytes (u32 *vest_state, const u32 *vest_family, u8 *output, const u32 bytes)
{
	u32				r = 0, x;
	
	for (;;)
	{
		if (r >= bytes) break;
		x = vest_32_next (vest_state, vest_family);
		output[r++] = (u8) x;
		if (r >= bytes) break;
		output[r++] = (u8) (x >>= 8);
		if (r >= bytes) break;
		output[r++] = (u8) (x >>= 8);
		if (r >= bytes) break;
		output[r++] = (u8) (x >>= 8);
	}
}

static void vest_32_process_bytes (u32 *vest_state, const u32 *vest_family, const u8 *input, u8 *output, const u32 bytes)
{
	u32				r = 0, x;
	
	for (;;)
	{
		if (r >= bytes) break;
		x = vest_32_next (vest_state, vest_family);
		output[r] = input[r] ^ (u8) x; r++;
		if (r >= bytes) break;
		output[r] = input[r] ^ (u8) (x >>= 8); r++;
		if (r >= bytes) break;
		output[r] = input[r] ^ (u8) (x >>= 8); r++;
		if (r >= bytes) break;
		output[r] = input[r] ^ (u8) (x >>= 8); r++;
	}
}

void vest_32_generate_family (u32 *vest_family, const u8 *key, const u32 key_bits, const u8 *IV, const u32 IV_bits)
{
	u32				vest_state[vest_32_words];
	u8				vest_output[((vest_32_bits-vest_f_bits)*17+vest_counters*4+7)/8];
	
	vest_32_set_key (vest_state, provest_32, key, key_bits, 0x4E);
	if (IV_bits) vest_32_set_IV (vest_state, provest_32, IV, IV_bits);
	vest_32_generate_bytes (vest_state, provest_32, vest_output, sizeof (vest_output));
	vest_generate_family (vest_family, provest_32, vest_output, vest_32_bits);
}

#endif

/* unauthenticated ECRYPT functions */

void ECRYPT_init (void) {}

void ECRYPT_keysetup (ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
#if (VEST_VARIANT == 1)
	vest_4_set_key (ctx->v, provest_4, key, keysize, 0x2B);
#elif (VEST_VARIANT == 2)
	vest_16_set_key (ctx->v, provest_16, key, keysize, 0x2B);
#elif (VEST_VARIANT == 3)
	vest_32_set_key (ctx->v, provest_32, key, keysize, 0x2B);
#endif
	memcpy (ctx->k, ctx->v, VEST_WORDS * 4);
	ctx->ivsize = ivsize;
}

void ECRYPT_ivsetup (ECRYPT_ctx* ctx, const u8* iv)
{
	memcpy (ctx->v, ctx->k, VEST_WORDS * 4);
#if (VEST_VARIANT == 1)
	vest_4_set_IV (ctx->v, provest_4, iv, ctx->ivsize);
#elif (VEST_VARIANT == 2)
	vest_16_set_IV (ctx->v, provest_16, iv, ctx->ivsize);
#elif (VEST_VARIANT == 3)
	vest_32_set_IV (ctx->v, provest_32, iv, ctx->ivsize);
#endif
}

void ECRYPT_process_bytes (int action, ECRYPT_ctx* ctx, const u8* input, u8* output, u32 msglen)
{
#if (VEST_VARIANT == 1)
	vest_4_process_bytes (ctx->v, provest_4, input, output, msglen);
#elif (VEST_VARIANT == 2)
	vest_16_process_bytes (ctx->v, provest_16, input, output, msglen);
#elif (VEST_VARIANT == 3)
	vest_32_process_bytes (ctx->v, provest_32, input, output, msglen);
#endif
}

void ECRYPT_keystream_bytes (ECRYPT_ctx* ctx, u8* keystream, u32 msglen)
{
#if (VEST_VARIANT == 1)
	vest_4_generate_bytes (ctx->v, provest_4, keystream, msglen);
#elif (VEST_VARIANT == 2)
	vest_16_generate_bytes (ctx->v, provest_16, keystream, msglen);
#elif (VEST_VARIANT == 3)
	vest_32_generate_bytes (ctx->v, provest_32, keystream, msglen);
#endif
}

/* authenticated ECRYPT functions */

void ECRYPT_AE_keysetup (ECRYPT_AE_ctx* ctx, const u8* key, u32 keysize, u32 ivsize, u32 macsize)
{
	ECRYPT_keysetup (&ctx->ctx, key, keysize, ivsize);
#if (VEST_VARIANT == 1)
	vest_4_set_key (ctx->macctx.v, provest_4, key, keysize, 0xB2);
#elif (VEST_VARIANT == 2)
	vest_16_set_key (ctx->macctx.v, provest_16, key, keysize, 0xB2);
#elif (VEST_VARIANT == 3)
	vest_32_set_key (ctx->macctx.v, provest_32, key, keysize, 0xB2);
#endif
	memcpy (ctx->macctx.k, ctx->macctx.v, VEST_WORDS * 4);
	ctx->macctx.macsize = macsize >> 3;
}

void ECRYPT_AE_ivsetup (ECRYPT_AE_ctx* ctx, const u8* iv)
{
	ECRYPT_ivsetup (&ctx->ctx, iv);
	memcpy (ctx->macctx.v, ctx->macctx.k, VEST_WORDS * 4);
#if (VEST_VARIANT == 1)
	vest_4_set_IV (ctx->macctx.v, provest_4, iv, ctx->ctx.ivsize);
#elif (VEST_VARIANT == 2)
	vest_16_set_IV (ctx->macctx.v, provest_16, iv, ctx->ctx.ivsize);
#elif (VEST_VARIANT == 3)
	vest_32_set_IV (ctx->macctx.v, provest_32, iv, ctx->ctx.ivsize);
#endif
}

void ECRYPT_AE_encrypt_bytes (ECRYPT_AE_ctx* ctx, const u8* plaintext, u8* ciphertext, u32 msglen)
{
#if (VEST_VARIANT == 1)
	vest_hash (ctx->macctx.v, provest_4, vest_4_structure, vest_4_bits, plaintext, msglen * 8);
	vest_4_process_bytes (ctx->ctx.v, provest_4, plaintext, ciphertext, msglen);
#elif (VEST_VARIANT == 2)
	vest_hash (ctx->macctx.v, provest_16, vest_16_structure, vest_16_bits, plaintext, msglen * 8);
	vest_16_process_bytes (ctx->ctx.v, provest_16, plaintext, ciphertext, msglen);
#elif (VEST_VARIANT == 3)
	vest_hash (ctx->macctx.v, provest_32, vest_32_structure, vest_32_bits, plaintext, msglen * 8);
	vest_32_process_bytes (ctx->ctx.v, provest_32, plaintext, ciphertext, msglen);
#endif
}

void ECRYPT_AE_decrypt_bytes (ECRYPT_AE_ctx* ctx, const u8* ciphertext, u8* plaintext, u32 msglen)
{
#if (VEST_VARIANT == 1)
	vest_4_process_bytes (ctx->ctx.v, provest_4, ciphertext, plaintext, msglen);
	vest_hash (ctx->macctx.v, provest_4, vest_4_structure, vest_4_bits, plaintext, msglen * 8);
#elif (VEST_VARIANT == 2)
	vest_16_process_bytes (ctx->ctx.v, provest_16, ciphertext, plaintext, msglen);
	vest_hash (ctx->macctx.v, provest_16, vest_16_structure, vest_16_bits, plaintext, msglen * 8);
#elif (VEST_VARIANT == 3)
	vest_32_process_bytes (ctx->ctx.v, provest_32, ciphertext, plaintext, msglen);
	vest_hash (ctx->macctx.v, provest_32, vest_32_structure, vest_32_bits, plaintext, msglen * 8);
#endif
}

void ECRYPT_AE_finalize (ECRYPT_AE_ctx* ctx, u8* mac)
{
#if (VEST_VARIANT == 1)
	vest_hash_seal (ctx->macctx.v, provest_4, vest_4_structure, vest_4_bits, 0xFF);
	vest_4_generate_bytes (ctx->macctx.v, provest_4, mac, ctx->macctx.macsize);
#elif (VEST_VARIANT == 2)
	vest_hash_seal (ctx->macctx.v, provest_16, vest_16_structure, vest_16_bits, 0xFF);
	vest_16_generate_bytes (ctx->macctx.v, provest_16, mac, ctx->macctx.macsize);
#elif (VEST_VARIANT == 3)
	vest_hash_seal (ctx->macctx.v, provest_32, vest_32_structure, vest_32_bits, 0xFF);
	vest_32_generate_bytes (ctx->macctx.v, provest_32, mac, ctx->macctx.macsize);
#endif
}

void ECRYPT_AE_authenticate_bytes (ECRYPT_AE_ctx* ctx, const u8* aad, u32 aadlen)
{
#if (VEST_VARIANT == 1)
	vest_hash (ctx->macctx.v, provest_4, vest_4_structure, vest_4_bits, aad, aadlen * 8);
#elif (VEST_VARIANT == 2)
	vest_hash (ctx->macctx.v, provest_16, vest_16_structure, vest_16_bits, aad, aadlen * 8);
#elif (VEST_VARIANT == 3)
	vest_hash (ctx->macctx.v, provest_32, vest_32_structure, vest_32_bits, aad, aadlen * 8);
#endif
}
