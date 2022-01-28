/* $Id: speed.c 223 2010-06-09 13:22:59Z tp $ */
/*
 * Speed testing for some hash functions. Each function is invoked
 * repeatedly on messages of increasing sizes (16, 64, 256, 1024 and
 * 8192 bytes). The resulting hash function bandwidth is then printed in
 * megabytes per second. Larger blocks increase bandwidth, since this
 * makes finalization events rarer (e.g. final padding). This test is
 * similar to what "openssl speed" performs.
 *
 * An additional test is performed for a "long message": a single
 * computation over a message consisting of many consecutive blocks
 * of 8192 bytes. This measures top hashing speed for a long stream.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sph_blake.h"
#include "sph_bmw.h"
#include "sph_cubehash.h"
#include "sph_echo.h"
#include "sph_fugue.h"
#include "sph_groestl.h"
#include "sph_hamsi.h"
#include "sph_haval.h"
#include "sph_jh.h"
#include "sph_keccak.h"
#include "sph_luffa.h"
#include "sph_md2.h"
#include "sph_md4.h"
#include "sph_md5.h"
#include "sph_panama.h"
#include "sph_radiogatun.h"
#include "sph_ripemd.h"
#include "sph_sha0.h"
#include "sph_sha1.h"
#include "sph_sha2.h"
#include "sph_shabal.h"
#include "sph_shavite.h"
#include "sph_simd.h"
#include "sph_skein.h"
#include "sph_tiger.h"
#include "sph_whirlpool.h"

#define DATA_LEN   8192

static unsigned char *data;
static size_t data_ptr;

#define SPEED_TEST(Name, cname) \
static double \
speed_ ## cname ## _unit(size_t clen, size_t num) \
{ \
	clock_t orig, end; \
	sph_ ## cname ## _context mc; \
 \
	orig = clock(); \
	while (num -- > 0) { \
		sph_ ## cname ## _init(&mc); \
		sph_ ## cname(&mc, data, clen); \
		sph_ ## cname ## _close(&mc, data + data_ptr); \
		data_ptr += (SPH_SIZE_ ## cname / 8); \
		if (data_ptr > (DATA_LEN - (SPH_SIZE_ ## cname / 8))) \
			data_ptr = 0; \
	} \
	end = clock(); \
	return (end - orig) / (double)CLOCKS_PER_SEC; \
} \
 \
static double \
speed_ ## cname ## _long(size_t clen, size_t num) \
{ \
	clock_t orig, end; \
	sph_ ## cname ## _context mc; \
	unsigned char res[64]; \
 \
	orig = clock(); \
	sph_ ## cname ## _init(&mc); \
	while (num -- > 0) { \
		sph_ ## cname(&mc, data, clen); \
	} \
	sph_ ## cname ## _close(&mc, res); \
	end = clock(); \
	return (end - orig) / (double)CLOCKS_PER_SEC; \
} \
 \
static void \
speed_ ## cname(void) \
{ \
	size_t clen, num; \
 \
	printf("Speed test: %s\n", Name); \
	fflush(stdout); \
	num = 2; \
	for (clen = 16;; clen <<= 2) { \
		double tt; \
 \
 		if (clen == 4096) { \
			clen = DATA_LEN; \
			if (num > 1) \
				num >>= 1; \
		} \
		for (;;) { \
			tt = speed_ ## cname ## _unit(clen, num); \
			if (tt > 6.0) { \
				if (num <= 1) \
					break; \
				num >>= 1; \
			} else if (tt < 2.0) { \
				num += num; \
			} else { \
				break; \
			} \
		} \
		printf("message length = %5lu -> %7.2f MBytes/s\n", \
			(unsigned long)clen, \
			((double)clen * (double)num) / (1000000.0 * tt)); \
		fflush(stdout); \
		if (clen == DATA_LEN) { \
			tt = speed_ ## cname ## _long(clen, num); \
			printf("long messages          -> %7.2f MBytes/s\n", \
				((double)clen * (double)num) \
				/ (1000000.0 * tt)); \
			fflush(stdout); \
			break; \
		} \
		if (num > 4) \
			num >>= 2; \
	} \
}

SPEED_TEST("MD2", md2)
SPEED_TEST("MD4", md4)
SPEED_TEST("MD5", md5)
SPEED_TEST("SHA-0", sha0)
SPEED_TEST("SHA-1", sha1)
SPEED_TEST("SHA-224", sha224)
SPEED_TEST("SHA-256", sha256)
#if SPH_64
SPEED_TEST("SHA-384", sha384)
SPEED_TEST("SHA-512", sha512)
#endif
SPEED_TEST("RIPEMD", ripemd)
SPEED_TEST("RIPEMD-128", ripemd128)
SPEED_TEST("RIPEMD-160", ripemd160)
#if SPH_64
SPEED_TEST("Tiger", tiger)
SPEED_TEST("Tiger2", tiger2)
#endif
SPEED_TEST("PANAMA", panama)
SPEED_TEST("HAVAL[3 passes]", haval256_3)
SPEED_TEST("HAVAL[4 passes]", haval256_4)
SPEED_TEST("HAVAL[5 passes]", haval256_5)
#if SPH_64
SPEED_TEST("WHIRLPOOL", whirlpool)
#endif
SPEED_TEST("RadioGatun[32]", radiogatun32)
#if SPH_64
SPEED_TEST("RadioGatun[64]", radiogatun64)
#endif
SPEED_TEST("Shabal-224", shabal224)
SPEED_TEST("Shabal-256", shabal256)
SPEED_TEST("Shabal-384", shabal384)
SPEED_TEST("Shabal-512", shabal512)
SPEED_TEST("ECHO-224", echo224)
SPEED_TEST("ECHO-256", echo256)
SPEED_TEST("ECHO-384", echo384)
SPEED_TEST("ECHO-512", echo512)
SPEED_TEST("SIMD-224", simd224)
SPEED_TEST("SIMD-256", simd256)
SPEED_TEST("SIMD-384", simd384)
SPEED_TEST("SIMD-512", simd512)
SPEED_TEST("Luffa-224", luffa224)
SPEED_TEST("Luffa-256", luffa256)
SPEED_TEST("Luffa-384", luffa384)
SPEED_TEST("Luffa-512", luffa512)
SPEED_TEST("BLAKE-224", blake224)
SPEED_TEST("BLAKE-256", blake256)
#if SPH_64
SPEED_TEST("BLAKE-384", blake384)
SPEED_TEST("BLAKE-512", blake512)
#endif
#if SPH_64
SPEED_TEST("Skein-224", skein224)
SPEED_TEST("Skein-256", skein256)
SPEED_TEST("Skein-384", skein384)
SPEED_TEST("Skein-512", skein512)
#endif
SPEED_TEST("JH-224", jh224)
SPEED_TEST("JH-256", jh256)
SPEED_TEST("JH-384", jh384)
SPEED_TEST("JH-512", jh512)
SPEED_TEST("Fugue-224", fugue224)
SPEED_TEST("Fugue-256", fugue256)
SPEED_TEST("Fugue-384", fugue384)
SPEED_TEST("Fugue-512", fugue512)
SPEED_TEST("BMW-224", bmw224)
SPEED_TEST("BMW-256", bmw256)
#if SPH_64
SPEED_TEST("BMW-384", bmw384)
SPEED_TEST("BMW-512", bmw512)
#endif
SPEED_TEST("CubeHash-224", cubehash224)
SPEED_TEST("CubeHash-256", cubehash256)
SPEED_TEST("CubeHash-384", cubehash384)
SPEED_TEST("CubeHash-512", cubehash512)
SPEED_TEST("Keccak-224", keccak224)
SPEED_TEST("Keccak-256", keccak256)
SPEED_TEST("Keccak-384", keccak384)
SPEED_TEST("Keccak-512", keccak512)
SPEED_TEST("Groestl-224", groestl224)
SPEED_TEST("Groestl-256", groestl256)
SPEED_TEST("Groestl-384", groestl384)
SPEED_TEST("Groestl-512", groestl512)
SPEED_TEST("Hamsi-224", hamsi224)
SPEED_TEST("Hamsi-256", hamsi256)
SPEED_TEST("Hamsi-384", hamsi384)
SPEED_TEST("Hamsi-512", hamsi512)
SPEED_TEST("SHAvite-224", shavite224)
SPEED_TEST("SHAvite-256", shavite256)
SPEED_TEST("SHAvite-384", shavite384)
SPEED_TEST("SHAvite-512", shavite512)

#define DO_MD2             0x00000001UL
#define DO_MD4             0x00000002UL
#define DO_MD5             0x00000004UL
#define DO_SHA0            0x00000008UL
#define DO_SHA1            0x00000010UL
#define DO_SHA224          0x00000020UL
#define DO_SHA256          0x00000040UL
#if SPH_64
#define DO_SHA384          0x00000080UL
#define DO_SHA512          0x00000100UL
#endif
#define DO_RIPEMD          0x00000200UL
#define DO_RIPEMD128       0x00000400UL
#define DO_RIPEMD160       0x00000800UL
#if SPH_64
#define DO_TIGER           0x00001000UL
#define DO_TIGER2          0x00002000UL
#endif
#define DO_PANAMA          0x00004000UL
#define DO_HAVAL3          0x00008000UL
#define DO_HAVAL4          0x00010000UL
#define DO_HAVAL5          0x00020000UL
#if SPH_64
#define DO_WHIRLPOOL       0x00040000UL
#endif
#define DO_SHABAL224       0x00080000UL
#define DO_SHABAL256       0x00100000UL
#define DO_SHABAL384       0x00200000UL
#define DO_SHABAL512       0x00400000UL
#define DO_RADIOGATUN32    0x01000000UL
#define DO_RADIOGATUN64    0x02000000UL
#define DO_ECHO224         0x04000000UL
#define DO_ECHO256         0x08000000UL
#define DO_ECHO384         0x10000000UL
#define DO_ECHO512         0x20000000UL

#define DO2_SIMD224        0x00000001UL
#define DO2_SIMD256        0x00000002UL
#define DO2_SIMD384        0x00000004UL
#define DO2_SIMD512        0x00000008UL
#define DO2_LUFFA224       0x00000010UL
#define DO2_LUFFA256       0x00000020UL
#define DO2_LUFFA384       0x00000040UL
#define DO2_LUFFA512       0x00000080UL
#define DO2_BLAKE224       0x00000100UL
#define DO2_BLAKE256       0x00000200UL
#define DO2_BLAKE384       0x00000400UL
#define DO2_BLAKE512       0x00000800UL
#define DO2_SKEIN224       0x00001000UL
#define DO2_SKEIN256       0x00002000UL
#define DO2_SKEIN384       0x00004000UL
#define DO2_SKEIN512       0x00008000UL
#define DO2_JH224          0x00010000UL
#define DO2_JH256          0x00020000UL
#define DO2_JH384          0x00040000UL
#define DO2_JH512          0x00080000UL
#define DO2_FUGUE224       0x00100000UL
#define DO2_FUGUE256       0x00200000UL
#define DO2_FUGUE384       0x00400000UL
#define DO2_FUGUE512       0x00800000UL
#define DO2_BMW224         0x01000000UL
#define DO2_BMW256         0x02000000UL
#define DO2_BMW384         0x04000000UL
#define DO2_BMW512         0x08000000UL
#define DO2_CUBEHASH224    0x10000000UL
#define DO2_CUBEHASH256    0x20000000UL
#define DO2_CUBEHASH384    0x40000000UL
#define DO2_CUBEHASH512    0x80000000UL

#define DO3_KECCAK224      0x00000001UL
#define DO3_KECCAK256      0x00000002UL
#define DO3_KECCAK384      0x00000004UL
#define DO3_KECCAK512      0x00000008UL
#define DO3_GROESTL224     0x00000010UL
#define DO3_GROESTL256     0x00000020UL
#define DO3_GROESTL384     0x00000040UL
#define DO3_GROESTL512     0x00000080UL
#define DO3_HAMSI224       0x00000100UL
#define DO3_HAMSI256       0x00000200UL
#define DO3_HAMSI384       0x00000400UL
#define DO3_HAMSI512       0x00000800UL
#define DO3_SHAVITE224     0x00001000UL
#define DO3_SHAVITE256     0x00002000UL
#define DO3_SHAVITE384     0x00004000UL
#define DO3_SHAVITE512     0x00008000UL

static struct {
	char *name;
	unsigned long flags, flags2, flags3;
} function_names[] = {
	{ "MD2",         DO_MD2, 0, 0              },
	{ "MD4",         DO_MD4, 0, 0              },
	{ "MD5",         DO_MD5, 0, 0              },
	{ "SHA-0",       DO_SHA0, 0, 0             },
	{ "SHA-1",       DO_SHA1, 0, 0             },
	{ "SHA-224",     DO_SHA224, 0, 0           },
	{ "SHA-256",     DO_SHA256, 0, 0           },
#if SPH_64
	{ "SHA-384",     DO_SHA384, 0, 0           },
	{ "SHA-512",     DO_SHA512, 0, 0           },
#endif
	{ "SHA2",        DO_SHA224 | DO_SHA256
#if SPH_64
	                 | DO_SHA384 | DO_SHA512
#endif
	                 , 0, 0  },
	{ "RMD",         DO_RIPEMD, 0, 0           },
	{ "RIPEMD",      DO_RIPEMD, 0, 0           },
	{ "RMD-128",     DO_RIPEMD128, 0, 0        },
	{ "RIPEMD-128",  DO_RIPEMD128, 0, 0        },
	{ "RMD-160",     DO_RIPEMD160, 0, 0        },
	{ "RIPEMD-160",  DO_RIPEMD160, 0, 0        },
#if SPH_64
	{ "Tiger",       DO_TIGER, 0, 0            },
	{ "Tiger2",      DO_TIGER2, 0, 0           },
#endif
	{ "Panama",      DO_PANAMA, 0, 0           },
	{ "HAVAL/3",     DO_HAVAL3, 0, 0           },
	{ "HAVAL/4",     DO_HAVAL4, 0, 0           },
	{ "HAVAL/5",     DO_HAVAL5, 0, 0           },
	{ "HAVAL",       DO_HAVAL3 | DO_HAVAL4 | DO_HAVAL5, 0, 0  },
#if SPH_64
	{ "Whirlpool",   DO_WHIRLPOOL, 0, 0        },
#endif
	{ "Shabal-224",  DO_SHABAL224, 0, 0        },
	{ "Shabal-256",  DO_SHABAL256, 0, 0        },
	{ "Shabal-384",  DO_SHABAL384, 0, 0        },
	{ "Shabal-512",  DO_SHABAL512, 0, 0        },
	{ "Shabal",      DO_SHABAL224 | DO_SHABAL256
	                 | DO_SHABAL384 | DO_SHABAL512, 0, 0  },
	{ "RadioGatun-32",  DO_RADIOGATUN32, 0, 0  },
#if SPH_64
	{ "RadioGatun-64",  DO_RADIOGATUN64, 0, 0  },
#endif
	{ "RadioGatun",     DO_RADIOGATUN32 | DO_RADIOGATUN64, 0, 0  },
	{ "ECHO-224",    DO_ECHO224, 0, 0          },
	{ "ECHO-256",    DO_ECHO256, 0, 0          },
	{ "ECHO-384",    DO_ECHO384, 0, 0          },
	{ "ECHO-512",    DO_ECHO512, 0, 0          },
	{ "ECHO",        DO_ECHO224 | DO_ECHO256
	                 | DO_ECHO384 | DO_ECHO512, 0, 0 },

	{ "SIMD-224",    0, DO2_SIMD224, 0      },
	{ "SIMD-256",    0, DO2_SIMD256, 0      },
	{ "SIMD-384",    0, DO2_SIMD384, 0      },
	{ "SIMD-512",    0, DO2_SIMD512, 0      },
	{ "SIMD",        0, DO2_SIMD224 | DO2_SIMD256
	                    | DO2_SIMD384 | DO2_SIMD512, 0 },
	{ "Luffa-224",   0, DO2_LUFFA224, 0     },
	{ "Luffa-256",   0, DO2_LUFFA256, 0     },
	{ "Luffa-384",   0, DO2_LUFFA384, 0     },
	{ "Luffa-512",   0, DO2_LUFFA512, 0     },
	{ "Luffa",       0, DO2_LUFFA224 | DO2_LUFFA256
	                    | DO2_LUFFA384 | DO2_LUFFA512, 0 },
	{ "BLAKE-224",   0, DO2_BLAKE224, 0     },
	{ "BLAKE-256",   0, DO2_BLAKE256, 0     },
#if SPH_64
	{ "BLAKE-384",   0, DO2_BLAKE384, 0     },
	{ "BLAKE-512",   0, DO2_BLAKE512, 0     },
#endif
	{ "BLAKE",       0, DO2_BLAKE224 | DO2_BLAKE256
#if SPH_64
	                    | DO2_BLAKE384 | DO2_BLAKE512
#endif
	                    , 0 },

#if SPH_64
	{ "Skein-224",   0, DO2_SKEIN224, 0     },
	{ "Skein-256",   0, DO2_SKEIN256, 0     },
	{ "Skein-384",   0, DO2_SKEIN384, 0     },
	{ "Skein-512",   0, DO2_SKEIN512, 0     },
	{ "Skein",       0, DO2_SKEIN224 | DO2_SKEIN256
	                    | DO2_SKEIN384 | DO2_SKEIN512, 0 },
#endif
	{ "JH-224",      0, DO2_JH224, 0        },
	{ "JH-256",      0, DO2_JH256, 0        },
	{ "JH-384",      0, DO2_JH384, 0        },
	{ "JH-512",      0, DO2_JH512, 0        },
	{ "JH",          0, DO2_JH224 | DO2_JH256
	                    | DO2_JH384 | DO2_JH512, 0 },
	{ "Fugue-224",   0, DO2_FUGUE224, 0     },
	{ "Fugue-256",   0, DO2_FUGUE256, 0     },
	{ "Fugue-384",   0, DO2_FUGUE384, 0     },
	{ "Fugue-512",   0, DO2_FUGUE512, 0     },
	{ "Fugue",       0, DO2_FUGUE224 | DO2_FUGUE256
	                    | DO2_FUGUE384 | DO2_FUGUE512, 0 },
	{ "BMW-224",     0, DO2_BMW224, 0       },
	{ "BMW-256",     0, DO2_BMW256, 0       },
#if SPH_64
	{ "BMW-384",     0, DO2_BMW384, 0       },
	{ "BMW-512",     0, DO2_BMW512, 0       },
#endif
	{ "BMW",         0, DO2_BMW224 | DO2_BMW256
#if SPH_64
	                    | DO2_BMW384 | DO2_BMW512
#endif
			    , 0 },
	{ "CubeHash-224",   0, DO2_CUBEHASH224, 0       },
	{ "CubeHash-256",   0, DO2_CUBEHASH256, 0       },
	{ "CubeHash-384",   0, DO2_CUBEHASH384, 0       },
	{ "CubeHash-512",   0, DO2_CUBEHASH512, 0       },
	{ "CubeHash",       0, DO2_CUBEHASH224 | DO2_CUBEHASH256
	                       | DO2_CUBEHASH384 | DO2_CUBEHASH512, 0 },

	{ "Keccak-224",     0, 0, DO3_KECCAK224  },
	{ "Keccak-256",     0, 0, DO3_KECCAK256  },
	{ "Keccak-384",     0, 0, DO3_KECCAK384  },
	{ "Keccak-512",     0, 0, DO3_KECCAK512  },
	{ "Keccak",         0, 0, DO3_KECCAK224 | DO3_KECCAK256
	                       | DO3_KECCAK384 | DO3_KECCAK512 },
	{ "Groestl-224",    0, 0, DO3_GROESTL224  },
	{ "Groestl-256",    0, 0, DO3_GROESTL256  },
	{ "Groestl-384",    0, 0, DO3_GROESTL384  },
	{ "Groestl-512",    0, 0, DO3_GROESTL512  },
	{ "Groestl",        0, 0, DO3_GROESTL224 | DO3_GROESTL256
	                       | DO3_GROESTL384 | DO3_GROESTL512 },
	{ "Hamsi-224",      0, 0, DO3_HAMSI224  },
	{ "Hamsi-256",      0, 0, DO3_HAMSI256  },
	{ "Hamsi-384",      0, 0, DO3_HAMSI384  },
	{ "Hamsi-512",      0, 0, DO3_HAMSI512  },
	{ "Hamsi",          0, 0, DO3_HAMSI224 | DO3_HAMSI256
	                       | DO3_HAMSI384 | DO3_HAMSI512 },
	{ "SHAvite-224",    0, 0, DO3_SHAVITE224  },
	{ "SHAvite-256",    0, 0, DO3_SHAVITE256  },
	{ "SHAvite-384",    0, 0, DO3_SHAVITE384  },
	{ "SHAvite-512",    0, 0, DO3_SHAVITE512  },
	{ "SHAvite",        0, 0, DO3_SHAVITE224 | DO3_SHAVITE256
	                       | DO3_SHAVITE384 | DO3_SHAVITE512 },
	{ "SHAvite-3",      0, 0, DO3_SHAVITE224 | DO3_SHAVITE256
	                       | DO3_SHAVITE384 | DO3_SHAVITE512 },

	{ NULL, 0, 0, 0 }
};

static void
fail_unknown(char *name)
{
	size_t u;

	fprintf(stderr, "unknown hash function name: '%s'\n", name);
	fprintf(stderr, "supported names:");
	for (u = 0; function_names[u].name != NULL; u ++) {
		if (u == 0 || (u > 0
			&& (function_names[u].flags
				!= function_names[u - 1].flags
			|| function_names[u].flags2
				!= function_names[u - 1].flags2
			|| function_names[u].flags3
				!= function_names[u - 1].flags3))) {
			fprintf(stderr, "\n   ");
		} else {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "%s", function_names[u].name);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Mame matching is case insensitive"
		" and ignores '-' and '/' characters.\n");
#if SPH_64
	fprintf(stderr, "'SHA2' stands for SHA-224, SHA-256,"
		" SHA-384 and SHA-512.\n");
#else
	fprintf(stderr, "'SHA2' stands for SHA-224 and SHA-256.\n");
#endif
	fprintf(stderr, "'HAVAL' stands for HAVAL/3, HAVAL/4 and"
		" HAVAL/5 (256-bit output).\n");
	fprintf(stderr, "'RadioGatun' stands for RadioGatun-32"
#if SPH_64
		" and RadioGatun-64"
#endif
		".\n");
	fprintf(stderr, "'Shabal' stands for Shabal-224, Shabal-256,"
		" Shabal-384 and Shabal-512.\n");
	fprintf(stderr, "'ECHO' stands for ECHO-224, ECHO-256,"
		" ECHO-384 and ECHO-512.\n");
	fprintf(stderr, "'SIMD' stands for SIMD-224, SIMD-256,"
		" SIMD-384 and SIMD-512.\n");
	fprintf(stderr, "'Luffa' stands for Luffa-224, Luffa-256,"
		" Luffa-384 and Luffa-512.\n");
#if SPH_64
	fprintf(stderr, "'BLAKE' stands for BLAKE-224, BLAKE-256,"
		" BLAKE-384 and BLAKE-512.\n");
#else
	fprintf(stderr, "'BLAKE' stands for BLAKE-224 and BLAKE-256.\n");
#endif
#if SPH_64
	fprintf(stderr, "'Skein' stands for Skein-224, Skein-256,"
		" Skein-384 and Skein-512.\n");
#endif
	fprintf(stderr, "'JH' stands for JH-224, JH-256,"
		" JH-384 and JH-512.\n");
	fprintf(stderr, "'Fugue' stands for Fugue-224, Fugue-256,"
		" Fugue-384 and Fugue-512.\n");
#if SPH_64
	fprintf(stderr, "'BMW' stands for BMW-224, BMW-256,"
		" BMW-384 and BMW-512.\n");
#else
	fprintf(stderr, "'BMW' stands for BMW-224 and BMW-256.\n");
#endif
	fprintf(stderr, "'CubeHash' stands for CubeHash-224, CubeHash-256,"
		" CubeHash-384 and CubeHash-512.\n");
	fprintf(stderr, "'Keccak' stands for Keccak-224, Keccak-256,"
		" Keccak-384 and Keccak-512.\n");
	fprintf(stderr, "'Groestl' stands for Groestl-224, Groestl-256,"
		" Groestl-384 and Groestl-512.\n");
	fprintf(stderr, "'Hamsi' stands for Hamsi-224, Hamsi-256,"
		" Hamsi-384 and Hamsi-512.\n");
	fprintf(stderr, "'SHAvite' and 'SHAvite-3' stand for SHAvite-224,"
		" SHAvite-256, SHAvite-384 and SHAvite-512.\n");
	exit(EXIT_FAILURE);
}

/*
 * Match two function names. Case is ignored. Minus and slash signs are
 * ignored as well.
 *
 * Note: this function assumes an ASCII host.
 */
static int
match_names(char *s1, char *s2)
{
	for (;;) {
		int c1, c2;

		while (*s1 == '-' || *s1 == '/')
			s1 ++;
		while (*s2 == '-' || *s2 == '/')
			s2 ++;
		c1 = *s1;
		c2 = *s2;
		if (c1 == 0 || c2 == 0)
			return c1 == c2;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += ('a' - 'A');
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += ('a' - 'A');
		if (c1 != c2)
			return 0;
		s1 ++;
		s2 ++;
	}
}

int
main(int argc, char *argv[])
{
	unsigned long todo, todo2, todo3;
	int i;

	todo = todo2 = todo3 = 0;
	for (i = 1; i < argc; i ++) {
		char *name;
		size_t u;

		name = argv[i];
		for (u = 0; function_names[u].name != NULL; u ++) {
			if (match_names(name, function_names[u].name)) {
				todo |= function_names[u].flags;
				todo2 |= function_names[u].flags2;
				todo3 |= function_names[u].flags3;
				break;
			}
		}
		if (function_names[u].name == NULL)
			fail_unknown(name);
	}
	if (todo == 0 && todo2 == 0 && todo3 == 0)
		todo = todo2 = todo3 = ~(unsigned long)0;
	data = malloc(DATA_LEN);
	if (data == NULL) {
		fprintf(stderr, "could not allocate input buffer\n");
		exit(EXIT_FAILURE);
	}
	memset(data, 'a', DATA_LEN);
	if (todo & DO_MD2)
		speed_md2();
	if (todo & DO_MD4)
		speed_md4();
	if (todo & DO_MD5)
		speed_md5();
	if (todo & DO_SHA0)
		speed_sha0();
	if (todo & DO_SHA1)
		speed_sha1();
	if (todo & DO_SHA224)
		speed_sha224();
	if (todo & DO_SHA256)
		speed_sha256();
#if SPH_64
	if (todo & DO_SHA384)
		speed_sha384();
	if (todo & DO_SHA512)
		speed_sha512();
#endif
	if (todo & DO_RIPEMD)
		speed_ripemd();
	if (todo & DO_RIPEMD128)
		speed_ripemd128();
	if (todo & DO_RIPEMD160)
		speed_ripemd160();
#if SPH_64
	if (todo & DO_TIGER)
		speed_tiger();
	if (todo & DO_TIGER2)
		speed_tiger2();
#endif
	if (todo & DO_PANAMA)
		speed_panama();
	if (todo & DO_HAVAL3)
		speed_haval256_3();
	if (todo & DO_HAVAL4)
		speed_haval256_4();
	if (todo & DO_HAVAL5)
		speed_haval256_5();
#if SPH_64
	if (todo & DO_WHIRLPOOL)
		speed_whirlpool();
#endif
	if (todo & DO_SHABAL224)
		speed_shabal224();
	if (todo & DO_SHABAL256)
		speed_shabal256();
	if (todo & DO_SHABAL384)
		speed_shabal384();
	if (todo & DO_SHABAL512)
		speed_shabal512();
	if (todo & DO_RADIOGATUN32)
		speed_radiogatun32();
#if SPH_64
	if (todo & DO_RADIOGATUN64)
		speed_radiogatun64();
#endif
	if (todo & DO_ECHO224)
		speed_echo224();
	if (todo & DO_ECHO256)
		speed_echo256();
	if (todo & DO_ECHO384)
		speed_echo384();
	if (todo & DO_ECHO512)
		speed_echo512();

	if (todo2 & DO2_SIMD224)
		speed_simd224();
	if (todo2 & DO2_SIMD256)
		speed_simd256();
	if (todo2 & DO2_SIMD384)
		speed_simd384();
	if (todo2 & DO2_SIMD512)
		speed_simd512();
	if (todo2 & DO2_LUFFA224)
		speed_luffa224();
	if (todo2 & DO2_LUFFA256)
		speed_luffa256();
	if (todo2 & DO2_LUFFA384)
		speed_luffa384();
	if (todo2 & DO2_LUFFA512)
		speed_luffa512();
	if (todo2 & DO2_BLAKE224)
		speed_blake224();
	if (todo2 & DO2_BLAKE256)
		speed_blake256();
#if SPH_64
	if (todo2 & DO2_BLAKE384)
		speed_blake384();
	if (todo2 & DO2_BLAKE512)
		speed_blake512();
#endif
#if SPH_64
	if (todo2 & DO2_SKEIN224)
		speed_skein224();
	if (todo2 & DO2_SKEIN256)
		speed_skein256();
	if (todo2 & DO2_SKEIN384)
		speed_skein384();
	if (todo2 & DO2_SKEIN512)
		speed_skein512();
#endif
	if (todo2 & DO2_JH224)
		speed_jh224();
	if (todo2 & DO2_JH256)
		speed_jh256();
	if (todo2 & DO2_JH384)
		speed_jh384();
	if (todo2 & DO2_JH512)
		speed_jh512();
	if (todo2 & DO2_FUGUE224)
		speed_fugue224();
	if (todo2 & DO2_FUGUE256)
		speed_fugue256();
	if (todo2 & DO2_FUGUE384)
		speed_fugue384();
	if (todo2 & DO2_FUGUE512)
		speed_fugue512();
	if (todo2 & DO2_BMW224)
		speed_bmw224();
	if (todo2 & DO2_BMW256)
		speed_bmw256();
#if SPH_64
	if (todo2 & DO2_BMW384)
		speed_bmw384();
	if (todo2 & DO2_BMW512)
		speed_bmw512();
#endif
	if (todo2 & DO2_CUBEHASH224)
		speed_cubehash224();
	if (todo2 & DO2_CUBEHASH256)
		speed_cubehash256();
	if (todo2 & DO2_CUBEHASH384)
		speed_cubehash384();
	if (todo2 & DO2_CUBEHASH512)
		speed_cubehash512();

	if (todo3 & DO3_KECCAK224)
		speed_keccak224();
	if (todo3 & DO3_KECCAK256)
		speed_keccak256();
	if (todo3 & DO3_KECCAK384)
		speed_keccak384();
	if (todo3 & DO3_KECCAK512)
		speed_keccak512();
	if (todo3 & DO3_GROESTL224)
		speed_groestl224();
	if (todo3 & DO3_GROESTL256)
		speed_groestl256();
	if (todo3 & DO3_GROESTL384)
		speed_groestl384();
	if (todo3 & DO3_GROESTL512)
		speed_groestl512();
	if (todo3 & DO3_HAMSI224)
		speed_hamsi224();
	if (todo3 & DO3_HAMSI256)
		speed_hamsi256();
	if (todo3 & DO3_HAMSI384)
		speed_hamsi384();
	if (todo3 & DO3_HAMSI512)
		speed_hamsi512();
	if (todo3 & DO3_SHAVITE224)
		speed_shavite224();
	if (todo3 & DO3_SHAVITE256)
		speed_shavite256();
	if (todo3 & DO3_SHAVITE384)
		speed_shavite384();
	if (todo3 & DO3_SHAVITE512)
		speed_shavite512();
	return 0;
}
