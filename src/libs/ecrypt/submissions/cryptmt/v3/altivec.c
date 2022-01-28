/* 
 * This file is included by cryptmt3.c
 * altivec code for Power PC
 */
static __inline__ void vec_genrand_block(ECRYPT_ctx * ctx);

static __inline__ void vec_genrand_block(ECRYPT_ctx * ctx)
{
    int i;
    vector unsigned int *sfmt;
    vector unsigned int a, b, c, d;
    const vector unsigned char perm1 = (vector unsigned char)
	(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3);
    const vector unsigned char perm2 = (vector unsigned char)
	(4, 5, 6, 7, 12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11);
    const vector unsigned char perm64 = (vector unsigned char)
	(4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11);
    const vector unsigned int shift64 = (vector unsigned int)(SHIFT64);
    const vector unsigned int shift64_mask =
	(vector unsigned int)(U32C(0xffffffff), U32C(0xffffffff), 
			      U32C(0x1fffffff), U32C(0xffffffff));
    const vector unsigned int mask = 
	(vector unsigned int)(MSK1, MSK2, MSK3, MSK4);

    sfmt = (vector unsigned int *)ctx->psfmt;
    a = sfmt[0];
    b = sfmt[POS1];
    c = sfmt[N - 1];
    d = vec_perm(b, b, perm2);
    b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
    b = vec_perm(b, b, perm64);
    c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		vec_xor(d, vec_and(c, mask)));
    sfmt[0] = c;
    for (i = 1; i < N - POS1; i++) {
	a = sfmt[i];
	b = sfmt[i + POS1];
	d = vec_perm(b, b, perm2);
	b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
	b = vec_perm(b, b, perm64);
	c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		    vec_xor(d, vec_and(c, mask)));
	sfmt[i] = c;
    }
    for (; i < N; i++) {
	a = sfmt[i];
	b = sfmt[i + POS1 - N];
	d = vec_perm(b, b, perm2);
	b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
	b = vec_perm(b, b, perm64);
	c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		    vec_xor(d, vec_and(c, mask)));
	sfmt[i] = c;
    }
}

static __inline__ void fast_genrand_block_first(ECRYPT_ctx * ctx, u8 cipher[], 
					    const u8 plain[]) {
    genrand_block_first(ctx, cipher, plain);
}

static __inline__ void fast_genrand_block(ECRYPT_ctx * ctx, u8 cipher[], 
					  const u8 plain[])
{
    vec_genrand_block(ctx);
    filter_16bytes(ctx->psfmt, ctx->accum, cipher, plain, N / 2);
}

static __inline__ void fast_genrand_bytes_first(ECRYPT_ctx * ctx, u8 cipher[],
					    const u8 plain[], u32 len) {
    genrand_bytes_first(ctx, cipher, plain, len);
}

static __inline__ void fast_genrand_bytes(ECRYPT_ctx *ctx, u8 cipher[],
					  const u8 plain[], u32 len)
{
    vector unsigned int *sfmt;
    vector unsigned int a, b, c, d;
    const vector unsigned char perm1 = (vector unsigned char)
	(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3);
    const vector unsigned char perm2 = (vector unsigned char)
	(4, 5, 6, 7, 12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11);
    const vector unsigned char perm64 = (vector unsigned char)
	(4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11);
    const vector unsigned int shift64 = (vector unsigned int)(SHIFT64);
    const vector unsigned int shift64_mask =
	(vector unsigned int)(U32C(0xffffffff), U32C(0xffffffff), 
			      U32C(0x1fffffff), U32C(0xffffffff));
    const vector unsigned int mask = 
	(vector unsigned int)(MSK1, MSK2, MSK3, MSK4);
    sfmt_t *ps;
    int i;
    s32 count;
    sfmt = (vector unsigned int *)ctx->psfmt;
    ps = (sfmt_t *)ctx->psfmt;
    count = (len + 7) / 8;
 
    a = sfmt[0];
    b = sfmt[POS1];
    c = sfmt[N - 1];
    d = vec_perm(b, b, perm2);
    b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
    b = vec_perm(b, b, perm64);
    c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		vec_xor(d, vec_and(c, mask)));
    sfmt[0] = c;
    count--;
    for (i = 1; (count > 0) && (i < N - POS1); i++, count--) {
	a = sfmt[i];
	b = sfmt[i + POS1];
	d = vec_perm(b, b, perm2);
	b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
	b = vec_perm(b, b, perm64);
	c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		    vec_xor(d, vec_and(c, mask)));
	sfmt[i] = c;
    }
    for (; (count > 0) && (i < N); i++, count--) {
	a = sfmt[i];
	b = sfmt[i + POS1 - N];
	d = vec_perm(b, b, perm2);
	b = vec_and(vec_srl(vec_perm(b, b, perm64), shift64), shift64_mask);
	b = vec_perm(b, b, perm64);
	c = vec_xor(vec_xor(vec_perm(a, a, perm1), b),
		    vec_xor(d, vec_and(c, mask)));
	sfmt[i] = c;
    }

    filter_16bytes(ps->sfmt[0], ctx->accum, cipher, plain, len / 16);
    i = (len / 16) * 2;
    len = len % 16;
    if (len != 0) {
	filter_bytes(ps->sfmt[i], ctx->accum, &cipher[i * 8],
		     &plain[i * 8], len);
    }
}

static __inline__ void fast_boot_up(ECRYPT_ctx *ctx, s32 length) {
    boot_up(ctx, length);
}

static int is_simd_cpu(void) {
    return 1;
}

