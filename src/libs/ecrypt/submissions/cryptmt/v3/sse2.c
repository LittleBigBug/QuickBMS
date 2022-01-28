/* 
 * This file is included by cryptmt2.c
 * SSE2 code for Intel C/C++ compiler
 */
#include <emmintrin.h>

#define RIGHT_ROT 0x39
#define LEFT_ROT 0x93
#define SHUFF2 0x8d
#define BOOT_ROT1 0x93
#define BOOT_ROT2 0x4b
#define BOOT_SL1 13
#define BOOT_SL2 11

static INLINE __m128i _mm_recursion(const __m128i *x, const __m128i *y, 
				    __m128i z, const __m128i mask);
static INLINE void _mm_genrand_block(ECRYPT_ctx *ctx);
static INLINE void _mm_filter_16bytes(__m128i *sfmt, __m128i *p_accum,
				      u8 cipher[], const u8 plain[],
				      s32 count);
static INLINE void _mm_booter_am(__m128i *acc, __m128i *pos1, __m128i *pos2,
				 s32 count);

/*
 * SFMT recursion
 */
static INLINE __m128i _mm_recursion(const __m128i *x, const __m128i *y, 
				    __m128i z, const __m128i mask) {
    __m128i a, b, c, d;

    a = _mm_load_si128(x);
    a = _mm_shuffle_epi32(a, RIGHT_ROT);
    b = _mm_load_si128(y);
    d = _mm_shuffle_epi32(b, SHUFF2);
    a = _mm_xor_si128(a, d);
    b = _mm_srli_epi64(b, SHIFT64);
    c = _mm_and_si128(z, mask);
    c = _mm_xor_si128(c, b);
    c = _mm_xor_si128(c, a);
    return c;
 }

/*
 * booter automaton
 */
static INLINE void _mm_booter_am(__m128i *p_acc, __m128i *pos1, __m128i *pos2,
				 s32 count)
{
    __m128i a, b, acc, x, y, z;
    int i;
    __m128i mask32;

    mask32 = _mm_set_epi32(0, U32C(0xffffffff), 0, U32C(0xffffffff));
    acc = _mm_load_si128(p_acc);
    y = _mm_load_si128(&pos2[0]);
    for (i = 0; i < count; i++) {
	x = _mm_load_si128(&pos1[i]);
	z = _mm_load_si128(&pos2[i + 1]);
	a = _mm_add_epi32(x, y);
	_mm_store_si128(&pos1[i], a);
	a = _mm_xor_si128(_mm_shuffle_epi32(a, BOOT_ROT1),
			  _mm_srli_epi32(a, BOOT_SL1));
	b = _mm_xor_si128(_mm_shuffle_epi32(z, BOOT_ROT2),
			  _mm_srli_epi32(z, BOOT_SL2));
	y = z;
	/****** _mm_multiply start ******/
	x = acc;
	x = _mm_mul_epu32(x, b);
	x = _mm_and_si128(x, mask32);
	x = _mm_slli_epi32(x, 1);
	x = _mm_add_epi32(x, acc);
	x = _mm_add_epi32(x, b);
	acc = _mm_shuffle_epi32(acc, RIGHT_ROT);
	b = _mm_shuffle_epi32(b, RIGHT_ROT);
	acc = _mm_mul_epu32(acc, b);
	acc = _mm_slli_epi64(acc, 33);
	acc = _mm_add_epi32(acc, x);
	/****** _mm_multiply end ******/
	a = _mm_sub_epi32(a, acc);
	_mm_store_si128(&pos2[i + 2], a);
    }
    _mm_store_si128(p_acc, acc);
}

static INLINE void fast_boot_up(ECRYPT_ctx *ctx, s32 length)
{
    s32 i, p;

    ctx->psfmt = ctx->sfmt[length + 2];
    p = ctx->ivsize / 4;
    for (i = 0; i < 4; i++) {
	ctx->lung[i] = ctx->sfmt[p * 4][i] | 1;
    }
    p = length - 2;
    _mm_booter_am((__m128i *)&ctx->lung, (__m128i *)&ctx->sfmt[0],
		  (__m128i *)&ctx->sfmt[p], length + 2);
    for (i = 0; i < 4; i++) {
	ctx->accum[i] = ctx->sfmt[2 * length + 1][i];
    }
}

static INLINE void _mm_genrand_block(ECRYPT_ctx * ctx)
{
    int i;
    __m128i *sfmt;
    __m128i c;
    const __m128i mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    sfmt = (__m128i *)ctx->psfmt;
    c = _mm_load_si128(&sfmt[N - 1]);
    c = _mm_recursion(&sfmt[0], &sfmt[POS1], c, mask);
    _mm_store_si128(&sfmt[0], c);
    for (i = 1; i < N - POS1; i++) {
	c = _mm_recursion(&sfmt[i], &sfmt[i + POS1], c, mask);
	_mm_store_si128(&sfmt[i], c);
    }
    for (; i < N; i++) {
	c = _mm_recursion(&sfmt[i], &sfmt[i + POS1 - N], c, mask);
	_mm_store_si128(&sfmt[i], c);
    }
}

static void fast_genrand_bytes(ECRYPT_ctx * ctx, u8 cipher[], const u8 plain[],
			  u32 len)
{
    u32 *accum;
    sfmt_t *ps;
    int i;
    s32 count;
    accum = ctx->accum;
    ps = (sfmt_t *)ctx->psfmt;
    count = (len + 7) / 8;
 
    do_recursion(ps->sfmt[0], ps->sfmt[POS1], ps->sfmt[N - 1]);
    count--;
    for (i = 1; (count > 0) && (i < N - POS1); i++, count--) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1], ps->sfmt[i - 1]);
    }
    for (; (count > 0) && (i < N); i++, count--) {
	do_recursion(ps->sfmt[i], ps->sfmt[i + POS1 - N], ps->sfmt[i - 1]);
    }

    filter_16bytes(ps->sfmt[0], ctx->accum, cipher, plain, len / 16);
    i = (len / 16) * 2;
    len = len % 16;
    if (len != 0) {
	filter_bytes(ps->sfmt[i], ctx->accum, &cipher[i * 8],
		     &plain[i * 8], len);
    }
}

static INLINE void fast_genrand_block(ECRYPT_ctx * ctx, u8 cipher[],
				      const u8 plain[])
{
    _mm_genrand_block(ctx);
    _mm_filter_16bytes((__m128i *)ctx->psfmt, (__m128i *)ctx->accum,
    	       cipher, plain, N / 2);
}

#if 0 //aluigi fix// defined(__GNUC__)
static int is_simd_cpu(void)
{
    int sse2;

   __asm__ __volatile__ (
       "movl  $0, %%eax\n\t"
       "cpuid\n\t"
       "cmp   $1, %%eax\n\t"
       "jb    no\n\t"
       "movl  $1, %%eax\n\t"
       "cpuid \n\t"
       "testl $0x04000000, %%edx\n\t"
       "jnz   yes\n\t"
       "movl  $0, %0\n\t"
       "jmp   no\n\t"
       "yes:\n\t"
       "movl  $1, %0\n\t"
       "no:\n\t"
       : "=m" (sse2) : 
       : "%eax", "%ebx", "%ecx", "%edx");
   return sse2;
}
#else
static int is_simd_cpu(void){
    return 1;
}
#endif
static void INLINE fast_genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[],
					    const u8 plain[], u32 len) {
    s32 i, p, count;
    sfmt_t *sp;
 
    count = (len + 7) / 8;
    sp = (sfmt_t *)ctx->psfmt;
    p = ctx->length - 2;
    _mm_booter_am((__m128i *)ctx->lung, (__m128i *)&sp->sfmt[0], 
		  (__m128i *)&sp->sfmt[p], count);
    _mm_filter_16bytes((__m128i *)sp, (__m128i *)ctx->accum,
		       cipher, plain, len / 16);
    i = (len / 16) * 2;
    len = len % 16;
    if (len != 0) {
	filter_bytes(sp->sfmt[i], ctx->accum, &cipher[i * 8],
		     &plain[i * 8], len);
    }
}

static void INLINE fast_genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[],
				     const u8 plain[])
{
    s32 p;
    sfmt_t *ps;
    __m128i c;
    const __m128i mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    ps = (sfmt_t *)ctx->psfmt;
    p = ctx->length - 2;
    _mm_booter_am((__m128i *)ctx->lung, (__m128i *)&ps->sfmt[0],
		  (__m128i *)&ps->sfmt[p], N);
    _mm_filter_16bytes((__m128i *)ps, (__m128i *)ctx->accum,
		       cipher, plain, N / 2);
    ps->sfmt[0][3] = INIL;
    c = _mm_load_si128((__m128i *)ps->sfmt[N - 1]);
    c = _mm_recursion((__m128i *)ps->sfmt[0], (__m128i *)ps->sfmt[POS1],
		      c, mask);
    _mm_store_si128((__m128i *)ps->sfmt[N], c);
    ctx->psfmt = ps->sfmt[1];
}

static INLINE void _mm_filter_16bytes(__m128i *sfmt, __m128i *p_accum,
				      u8 cipher[], const u8 plain[],
				      s32 count) {
    int i;
    __m128i acc, x, y, out;
    __m128i mask16, mask32;

    mask16 = _mm_set_epi32(U32C(0x0000ffff), U32C(0x0000ffff),
			   U32C(0x0000ffff), U32C(0x0000ffff));
    mask32 = _mm_set_epi32(0, U32C(0xffffffff), 0, U32C(0xffffffff));
    acc = _mm_load_si128(p_accum);
    for (i = 0; i < count; i++) {
	y = _mm_load_si128(&sfmt[i*2]);
	x = acc;
	x = _mm_shuffle_epi32(x, RIGHT_ROT);
	x = _mm_srli_epi32(x, 1);
	acc = _mm_xor_si128(acc, x);
	/****** _mm_multiply start ******/
	x = acc;
	x = _mm_mul_epu32(x, y);
	x = _mm_and_si128(x, mask32);
	x = _mm_slli_epi32(x, 1);
	x = _mm_add_epi32(x, acc);
	x = _mm_add_epi32(x, y);
	acc = _mm_shuffle_epi32(acc, RIGHT_ROT);
	y = _mm_shuffle_epi32(y, RIGHT_ROT);
	acc = _mm_mul_epu32(acc, y);
	acc = _mm_slli_epi64(acc, 33);
	acc = _mm_add_epi32(acc, x);
	/****** _mm_multiply end ******/
	y = acc;
	out = _mm_srli_epi32(acc, 16);
	out = _mm_xor_si128(out, y);
	out = _mm_and_si128(out, mask16);
	y = _mm_load_si128(&sfmt[i * 2 + 1]);
	x = acc;
	x = _mm_shuffle_epi32(x, RIGHT_ROT);
	x = _mm_srli_epi32(x, 1);
	acc = _mm_xor_si128(acc, x);
	/****** _mm_multiply start ******/
	x = acc;
	x = _mm_mul_epu32(x, y);
	x = _mm_and_si128(x, mask32);
	x = _mm_slli_epi32(x, 1);
	x = _mm_add_epi32(x, acc);
	x = _mm_add_epi32(x, y);
	acc = _mm_shuffle_epi32(acc, RIGHT_ROT);
	y = _mm_shuffle_epi32(y, RIGHT_ROT);
	acc = _mm_mul_epu32(acc, y);
	acc = _mm_slli_epi64(acc, 33);
	acc = _mm_add_epi32(acc, x);
	/****** _mm_multiply end ******/

	x = _mm_slli_epi32(acc, 16);
	x = _mm_xor_si128(x, acc);
	x = _mm_andnot_si128(mask16, x);
	out = _mm_or_si128(out, x);
	/* output */
	x = _mm_load_si128((__m128i *)&plain[i * 16]);
	x = _mm_xor_si128(x, out);
	_mm_store_si128((__m128i *)&cipher[i * 16], x);
    }
    _mm_store_si128(p_accum, acc);
}

#undef RIGHT_ROT
#undef RIGHT_ROT
#undef BOOT_ROT1
#undef BOOT_ROT2
#undef BOOT_SL1
#undef BOOT_SL2
