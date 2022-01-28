/* $Id: hsum.c 243 2010-06-21 17:13:32Z tp $ */
/*
 * Command-line utility to compute hash functions over files. This is
 * intended to work similarly to the usual "md5sum" utility, but for
 * all hash functions implemented by sphlib.
 *
 * Usage is the following:
 * <pre>
 *   sphsum function [ options ] [ file... ]
 * </pre>
 * where <code>function</code> is the hash function name (short internal
 * name, such as <code>md5</code> or <code>whirlpool0</code>). Options
 * specify whether the file must be read as text or binary (it makes no
 * difference on Unix systems); moreover, with the <code>"-c"</code> option,
 * a list of checksums can be verified. If no file name is specified, then
 * standard input is used; the special file name <code>"-"</code> (a single
 * minus sign) is also an alias for standard input.
 *
 * Alternatively, the executable binary may be named after the hash
 * function itself. In that situation, the <code>function</code> parameter
 * must be omitted. For function name recognition, suffixes <code>".exe"</code>
 * and <code>"sum"</code> are suppressed; thus, the executable may be named
 * <code>"md5sum"</code> or <code>"md5sum.exe"</code>, and will then behave
 * as the standard Linux utility <code>"md5sum"</code>.
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

/**
 * The program name, as extracted from the invocation name.
 */
static char *program_name;

/**
 * Extract the program name from the invocation name.
 *
 * @param fullname   the invocation name
 */
static void
make_program_name(char *fullname)
{
	char *c, *d;

	if (fullname == NULL) {
		program_name = "hsum";
		return;
	}
	for (c = d = fullname; *d != 0; d ++)
		if (*d == '/' || *d == '\\')
			c = d + 1;
	program_name = c;
}

/**
 * Print out usage and exit.
 *
 * @param genf   non-zero if generic executable
 * @param fail   non-zero to exit with a failure status
 */
static void
usage(int genf, int fail)
{
	fprintf(stderr,
"usage: %s%s [options] [file...]\n"
"options:\n"
"  -b, --binary\n"
"      read in binary mode\n"
"  -c, --check\n"
"      read sums from the files and check them\n"
"  -t, --text\n"
"      read in text mode (default)\n"
"  --status\n"
"      output is discarded; success or failure is shown by the exit code\n"
"  -w, --warn\n"
"      warn about improperly formatted checksum lines (which are ignored)\n"
"  -h, --help\n"
"      display this help and exit\n"
"  -v, --version\n"
"      display version information and exit\n",
		program_name, genf ? " function" : "");
	exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**
 * Print out version information and exit with a success status.
 */
static void
version(void)
{
	fprintf(stderr,
"sphlib: version 2.1\n"
"Copyright (c) 2007-2010  Projet RNRT SAPHIR\n"
"This software is provided WITHOUT ANY WARRANTY, to the extent\n"
"permitted by law.\n");
	exit(EXIT_SUCCESS);
}

#define MAKECC(name)   sph_ ## name ## _context cc_ ## name

/**
 * The hash function context is static; this union ensures that we have
 * a properly sized and aligned context for all functions (but not
 * simultaneously, of course).
 */
static union {
	MAKECC(blake224);
	MAKECC(blake256);
#if SPH_64
	MAKECC(blake384);
	MAKECC(blake512);
#endif
	MAKECC(bmw224);
	MAKECC(bmw256);
#if SPH_64
	MAKECC(bmw384);
	MAKECC(bmw512);
#endif
	MAKECC(cubehash224);
	MAKECC(cubehash256);
	MAKECC(cubehash384);
	MAKECC(cubehash512);
	MAKECC(echo224);
	MAKECC(echo256);
	MAKECC(echo384);
	MAKECC(echo512);
	MAKECC(fugue224);
	MAKECC(fugue256);
	MAKECC(fugue384);
	MAKECC(fugue512);
	MAKECC(groestl224);
	MAKECC(groestl256);
	MAKECC(groestl384);
	MAKECC(groestl512);
	MAKECC(hamsi224);
	MAKECC(hamsi256);
	MAKECC(hamsi384);
	MAKECC(hamsi512);
	MAKECC(haval128_3);
	MAKECC(haval128_4);
	MAKECC(haval128_5);
	MAKECC(haval160_3);
	MAKECC(haval160_4);
	MAKECC(haval160_5);
	MAKECC(haval192_3);
	MAKECC(haval192_4);
	MAKECC(haval192_5);
	MAKECC(haval224_3);
	MAKECC(haval224_4);
	MAKECC(haval224_5);
	MAKECC(haval256_3);
	MAKECC(haval256_4);
	MAKECC(haval256_5);
	MAKECC(jh224);
	MAKECC(jh256);
	MAKECC(jh384);
	MAKECC(jh512);
	MAKECC(keccak224);
	MAKECC(keccak256);
	MAKECC(keccak384);
	MAKECC(keccak512);
	MAKECC(luffa224);
	MAKECC(luffa256);
	MAKECC(luffa384);
	MAKECC(luffa512);
	MAKECC(md2);
	MAKECC(md4);
	MAKECC(md5);
	MAKECC(panama);
	MAKECC(radiogatun32);
#if SPH_64
	MAKECC(radiogatun64);
#endif
	MAKECC(ripemd128);
	MAKECC(ripemd160);
	MAKECC(ripemd);
	MAKECC(sha0);
	MAKECC(sha1);
	MAKECC(sha224);
	MAKECC(sha256);
#if SPH_64
	MAKECC(sha384);
	MAKECC(sha512);
#endif
	MAKECC(shabal224);
	MAKECC(shabal256);
	MAKECC(shabal384);
	MAKECC(shabal512);
	MAKECC(shavite224);
	MAKECC(shavite256);
	MAKECC(shavite384);
	MAKECC(shavite512);
	MAKECC(simd224);
	MAKECC(simd256);
	MAKECC(simd384);
	MAKECC(simd512);
#if SPH_64
	MAKECC(skein224);
	MAKECC(skein256);
	MAKECC(skein384);
	MAKECC(skein512);
#endif
	MAKECC(tiger2);
	MAKECC(tiger);
	MAKECC(whirlpool0);
	MAKECC(whirlpool1);
	MAKECC(whirlpool);
} hcontext;

/**
 * File data will come through this buffer. Using the union ensures
 * proper alignment. Eight kilobytes are used for the buffer: this
 * should be enough to get near-optimal speed.
 */
static union {
	unsigned char buf[8192];
	long l;
	void *p;
	sph_u32 w32;
#if SPH_64
	sph_u64 w64;
#endif
} ubuf;

#define MAKEFFGEN(id, name, len)   { \
		&sph_ ## id ## _init, &sph_ ## id, \
		&sph_ ## id ## _close, #name, len \
	}

#define MAKEFF(id)   MAKEFFGEN(id, id, SPH_SIZE_ ## id / 8)

/**
 * This array contains the callbacks to the hash function implementation.
 */
static struct known_function {
	void (*ff_init)(void *cc);
	void (*ff_update)(void *cc, const void *data, size_t len);
	void (*ff_close)(void *cc, void *dst);
	char *name;
	size_t out_len;
} known_functions[] = {
	MAKEFF(blake224),
	MAKEFF(blake256),
#if SPH_64
	MAKEFF(blake384),
	MAKEFF(blake512),
#endif
	MAKEFF(bmw224),
	MAKEFF(bmw256),
#if SPH_64
	MAKEFF(bmw384),
	MAKEFF(bmw512),
#endif
	MAKEFF(cubehash224),
	MAKEFF(cubehash256),
	MAKEFF(cubehash384),
	MAKEFF(cubehash512),
	MAKEFF(echo224),
	MAKEFF(echo256),
	MAKEFF(echo384),
	MAKEFF(echo512),
	MAKEFF(fugue224),
	MAKEFF(fugue256),
	MAKEFF(fugue384),
	MAKEFF(fugue512),
	MAKEFF(groestl224),
	MAKEFF(groestl256),
	MAKEFF(groestl384),
	MAKEFF(groestl512),
	MAKEFF(hamsi224),
	MAKEFF(hamsi256),
	MAKEFF(hamsi384),
	MAKEFF(hamsi512),
	MAKEFF(haval128_3),
	MAKEFF(haval128_4),
	MAKEFF(haval128_5),
	MAKEFF(haval160_3),
	MAKEFF(haval160_4),
	MAKEFF(haval160_5),
	MAKEFF(haval192_3),
	MAKEFF(haval192_4),
	MAKEFF(haval192_5),
	MAKEFF(haval224_3),
	MAKEFF(haval224_4),
	MAKEFF(haval224_5),
	MAKEFF(haval256_3),
	MAKEFF(haval256_4),
	MAKEFF(haval256_5),
	MAKEFF(jh224),
	MAKEFF(jh256),
	MAKEFF(jh384),
	MAKEFF(jh512),
	MAKEFF(keccak224),
	MAKEFF(keccak256),
	MAKEFF(keccak384),
	MAKEFF(keccak512),
	MAKEFF(luffa224),
	MAKEFF(luffa256),
	MAKEFF(luffa384),
	MAKEFF(luffa512),
	MAKEFF(md2),
	MAKEFF(md4),
	MAKEFF(md5),
	MAKEFF(panama),
	MAKEFF(radiogatun32),
#if SPH_64
	MAKEFF(radiogatun64),
#endif
	MAKEFF(ripemd128),
	MAKEFF(ripemd160),
	MAKEFF(ripemd),
	MAKEFFGEN(ripemd128, rmd128, 16),
	MAKEFFGEN(ripemd160, rmd160, 20),
	MAKEFFGEN(ripemd, rmd, 16),
	MAKEFF(sha0),
	MAKEFF(sha1),
	MAKEFF(sha224),
	MAKEFF(sha256),
#if SPH_64
	MAKEFF(sha384),
	MAKEFF(sha512),
#endif
	MAKEFF(shabal224),
	MAKEFF(shabal256),
	MAKEFF(shabal384),
	MAKEFF(shabal512),
	MAKEFF(shavite224),
	MAKEFF(shavite256),
	MAKEFF(shavite384),
	MAKEFF(shavite512),
	MAKEFF(simd224),
	MAKEFF(simd256),
	MAKEFF(simd384),
	MAKEFF(simd512),
#if SPH_64
	MAKEFF(skein224),
	MAKEFF(skein256),
	MAKEFF(skein384),
	MAKEFF(skein512),
#endif
	MAKEFF(tiger2),
	MAKEFF(tiger),
	MAKEFF(whirlpool0),
	MAKEFF(whirlpool1),
	MAKEFF(whirlpool),

	{ 0, 0, 0, 0, 0 }
};

/**
 * Compare two strings, case insensitive. Note: this function assumes
 * an architecture which operates with an ASCII-compatible charset.
 *
 * @param s1   the first string
 * @param s2   the second string
 * @return  non-zero on string equality
 */
static int
equals_string_nocase(char *s1, char *s2)
{
	while (*s1 || *s2) {
		int c1, c2;

		c1 = *s1 ++;
		c2 = *s2 ++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';
		if (c1 != c2)
			return 0;
	}
	return 1;
}

/**
 * Recognize the function name.
 *
 * @param name   the name to match
 * @return  the function index, of -1
 */
static int
get_function_name(char *name)
{
	int i;
	size_t u, v, w, len;
	char name_ext[30];

	len = strlen(name);
	v = 0;
	for (u = 0; u < len; u ++)
		if (name[u] == '/' || name[u] == '\\')
			v = u + 1;
	w = len;
	if (w > (v + 4) && equals_string_nocase(name + (w - 4), ".exe"))
		w -= 3;
	if (w > (v + 3) && equals_string_nocase(name + (w - 3), "sum"))
		w -= 3;
	if ((w - v) >= sizeof name_ext)
		return -1;
	memcpy(name_ext, name + v, w - v);
	name_ext[w - v] = 0;
	for (i = 0; known_functions[i].name != NULL; i ++)
		if (equals_string_nocase(name_ext, known_functions[i].name))
			return i;
	return -1;
}

/*
 * Static state: options, counts for computations and failures, and
 * the function to use.
 */

static int function_id, binary, check, nostatus, nowarn;
static int has_failed;
static long mismatch_count, computed_count;
static struct known_function kf;

/**
 * Print out a file hash.
 *
 * @param buf     the hash output
 * @param len     the hash output length
 * @param fname   the hashed file name
 */
static void
print_hash(unsigned char *buf, size_t len, char *fname)
{
	while (len -- > 0)
		printf("%02x", (unsigned)*buf ++);
	printf(" %s%s\n", binary ? "*" : " ", fname);
}

/**
 * Print out the file status (when checking). A <code>NULL</code> file name
 * means "standard input".
 *
 * @param fname   the file name (may be <code>NULL</code>)
 * @param ok      zero for a failure
 */
static void
print_status(char *fname, int ok)
{
	if (nostatus)
		return;
	if (fname == NULL)
		fname = "-";
	printf("%s: %s\n", fname, ok ? "OK" : "FAILED");
}

/**
 * Hash some data
 *
 * @param in      the data input stream
 * @param fname   the file name (for error reporting), or <code>NULL</code>
 * @param dst     the output buffer for the hash result
 * @return  0 on success, -1 on error (I/O error)
 */
static int
hash_file(FILE *in, char *fname, void *dst)
{
	for (;;) {
		size_t len;

		len = fread(ubuf.buf, 1, sizeof ubuf.buf, in);
		kf.ff_update(&hcontext, ubuf.buf, len);
		if (len < sizeof ubuf.buf)
			break;
	}
	if (ferror(in)) {
		fprintf(stderr, "%s: %s: ", program_name,
			fname == NULL ? "<stdin>" : fname);
		perror("fread");
		return -1;
	}
	kf.ff_close(&hcontext, dst);
	return 0;
}

/**
 * Get the numerical value of an hexadecimal digit.
 *
 * @param c   the character value
 * @return  the numerical value, or -1 on error
 */
static int
hexnum(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	switch (c) {
	case 'a': case 'A': return 10;
	case 'b': case 'B': return 11;
	case 'c': case 'C': return 12;
	case 'd': case 'D': return 13;
	case 'e': case 'E': return 14;
	case 'f': case 'F': return 15;
	}
	return -1;
}

/**
 * Parse a line from a checksum file. The terminating newline must have
 * been removed from the line.
 *
 * @param line       the read line
 * @param line_num   the file line number (for error reporting)
 * @param out        the output buffer for the expected hash result
 * @param rbin       set to 1 if the "binary" flag is set in the line
 * @return  pointer to the file name (within the line), or <code>NULL</code>
 */
static char *
parse_line(char *line, long line_num, unsigned char *out, int *rbin)
{
	char *c;
	size_t u;

	for (c = line; *c == ' ' || *c == '\t'; c ++);
	for (u = 0; u < kf.out_len; u ++) {
		int z1, z2;

		z1 = hexnum(*c ++);
		if (z1 < 0)
			goto error;
		z2 = hexnum(*c ++);
		if (z2 < 0)
			goto error;
		out[u] = (z1 << 4) | z2;
	}
	if (*c ++ != ' ')
		goto error;
	switch (*c ++) {
	case ' ':
		*rbin = 0;
		break;
	case '*':
		*rbin = 1;
		break;
	default:
		goto error;
	}
	return c;

error:
	if (!nowarn)
		fprintf(stderr, "%s: warning: improperly formatted"
			" checksum file line %ld, skipping.\n",
			program_name, line_num);
	return NULL;
}

/**
 * Process the provided file name.
 *
 * @param fname   the file name (<code>NULL</code> for standard input)
 */
static void
process_file(char *fname)
{
	FILE *in;

	if (fname == NULL) {
		in = stdin;
	} else {
		in = fopen(fname, (binary && !check) ? "rb" : "r");
		if (in == NULL) {
			fprintf(stderr, "%s: %s: ", program_name, fname);
			perror("fopen");
			has_failed = 1;
			if (check)
				mismatch_count ++;
			return;
		}
	}
	if (check) {
		char line[4096];
		long line_num;

		line_num = 0;
		while (fgets(line, sizeof line, in) != NULL) {
			size_t n;
			char *fname2;
			int rbin;
			FILE *in2;
			unsigned char buf[128];
			unsigned char exp_res[128];
			int good;

			line_num ++;
			n = strlen(line);
			if (n > 0 && line[n - 1] == '\n')
				line[-- n] = 0;
			if (n == 1 + sizeof line) {
				int quit;

				if (!nowarn)
					fprintf(stderr, "%s: warning: checksum"
						" file line %ld too long,"
						" skipping.\n",
						program_name, line_num);
				quit = 0;
				for (;;) {
					if (fgets(line,
						sizeof line, in) == NULL) {
						quit = 1;
						break;
					}
					n = strlen(line);
					if (n > 0 && line[n - 1] == '\n')
						break;
				}
				if (quit)
					break;
			}
			fname2 = parse_line(line, line_num, exp_res, &rbin);
			if (fname2 == NULL)
				continue;
			if (strcmp(fname2, "-") == 0) {
				if (fname == NULL) {
					fprintf(stderr, "%s: error: stdin"
						" double use (checksum file"
						" line %ld), skipping.\n",
						program_name, line_num);
					has_failed = 1;
					mismatch_count ++;
					continue;
				}
				fname2 = NULL;
				in2 = stdin;
			} else {
				in2 = fopen(fname2, rbin ? "rb" : "r");
				if (in2 == NULL) {
					fprintf(stderr, "%s: %s: ",
						program_name, fname2);
					perror("fopen");
					has_failed = 1;
					mismatch_count ++;
				}
			}
			computed_count ++;
			if (hash_file(in2, fname2, buf) < 0) {
				has_failed = 1;
				mismatch_count ++;
			} else {
				good = (memcmp(buf, exp_res, kf.out_len) == 0);
				print_status(fname2, good);
				if (!good) {
					has_failed = 1;
					mismatch_count ++;
				}
			}
			if (fname2 != NULL)
				fclose(in2);
		}
		if (ferror(in)) {
			fprintf(stderr, "%s: error: read error on checksum"
				" file\n", program_name);
			has_failed = 1;
		}
	} else {
		unsigned char buf[128];

		if (hash_file(in, fname, buf) < 0) {
			has_failed = 1;
			if (check)
				mismatch_count ++;
		} else {
			print_hash(buf, kf.out_len,
				fname == NULL ? "-" : fname);
		}
	}
	if (fname != NULL)
		fclose(in);
}

/**
 * Main function. See <code>usage()</code> for options.
 *
 * @param argc   the argument count
 * @param argv   the program arguments
 * @return  the exit status
 */
int
main(int argc, char *argv[])
{
	int i;
	int fid, skip, ff;

	binary = 0;
	check = 0;
	nostatus = 0;
	nowarn = 0;
	make_program_name(argv[0]);
	if (argc <= 0)
		usage(1, 1);
	fid = get_function_name(program_name);
	if (fid < 0) {
		if (argc <= 1)
			usage(1, 1);
		fid = get_function_name(argv[1]);
		if (fid < 0) {
			if (!strcmp(argv[1], "-v")
				|| !strcmp(argv[1], "--version"))
				version();
			usage(1, 1);
		}
		skip = 1;
	} else {
		skip = 0;
	}
	for (ff = 0, i = 1 + skip; i < argc; i ++) {
		char *opt;

		opt = argv[i];
		if (!strcmp(opt, "-b") || !strcmp(opt, "--binary")) {
			binary = 1;
		} else if (!strcmp(opt, "-c") || !strcmp(opt, "--check")) {
			check = 1;
		} else if (!strcmp(opt, "-t") || !strcmp(opt, "--text")) {
			binary = 0;
		} else if (!strcmp(opt, "--status")) {
			nostatus = 1;
		} else if (!strcmp(opt, "-w") || !strcmp(opt, "--warn")) {
			nowarn = 1;
		} else if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) {
			usage(skip, 0);
		} else if (!strcmp(opt, "-v") || !strcmp(opt, "--version")) {
			version();
		} else if (!strcmp(opt, "--")) {
			if ((i + 1) < argc)
				ff = 1;
			argv[i] = NULL;
			break;
		} else {
			ff = 1;
			continue;
		}
		argv[i] = NULL;
	}
	if ((nostatus || nowarn) && !check)
		usage(skip, 1);
	has_failed = 0;
	mismatch_count = 0;
	computed_count = 0;
	function_id = fid;
	kf = known_functions[fid];
	kf.ff_init(&hcontext);
	if (ff) {
		for (i = 1 + skip; i < argc; i ++) {
			char *fname;

			fname = argv[i];
			if (fname == NULL)
				continue;
			if (!strcmp(fname, "-"))
				fname = NULL;
			process_file(fname);
		}
	} else {
		process_file(NULL);
	}
	if (check && mismatch_count > 0 && !nostatus) {
		fprintf(stderr, "%s: WARNING: %ld of %ld computed checksum%s"
			" did NOT match\n",
			program_name, mismatch_count, computed_count,
			computed_count > 1 ? "s" : "");
	}
	return has_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
