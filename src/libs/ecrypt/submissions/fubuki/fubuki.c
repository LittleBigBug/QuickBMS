/* FUBUKI encryption system, by Hagita-Matsumoto-Nishimura-Saito */
/* Coded by Makoto Matsumoto, 2001/12/25 */
/* Re-Coded by Makoto Matsumoto, 2005/03/29 */
/* Multiplication-based by Makoto Matsumoto, 2005/04/10 */
/* Variable Tuple by Takuji Nishimura, 2005/04/12 */
/* Tough against differential crypt attack, 2005/04/13 */
/* The algorithm is fixed, 2005/04/15 */

/*
  LICENSE CONDITION

  1. A patent on this algorithm is pending as of 2005 May. The  
  intellectual property belongs to Hiroshima University and Ochanomizu  
  University.

  2. This software is free for non-commercial use.
  2-1 "Non-commercial use" explicitly includes research and educational  
  purposes, as well as use in software which is made available for free.
  2-2 "Non-commercial use" also includes any other kind of use, except  
  those stated in item 3.

  3. Contacting the inventors is required for commercial use, as  
  defined in item 3-1.
  3-1. In the case of selling software or hardware which utilizes this  
  software or this algorithm.

  4. Royalties may be required in cases of commercial use as defined in  
  item 3. However with the exception of a few cases as stated in  
  Background, below, it is our intention to allow general royalty-free  
  use.

  5. We disclaim any responsibility for any direct or indirect damages  
  caused by this software or algorithm.

  Background:
  A. The inventors of this algorithm are Makoto Matsumoto (Hiroshima  
  University), Mariko Hagita (Ochanomizu University), Takuji Nishimura  
  (Yamagata University), and Mutsuo Saito (Hiroshima University).  
  (Affiliations as of 2005 Dec.)
  
  B. By the regulations of Hiroshima University and Ochanomizu  
  University, all inventions by the worker(s) are examined with respect  
  to patent, and their intellectual property is owned by the university 
  (ies), if deemed necessary.

  C. The inventors' wish is to allow the algorithm/software to be as  
  freely and widely used as possible.

  D. The desire of the intellectual property centers (IPCs) of  
  Hiroshima University and Ochanomizu University is to obtain the  
  minimum amount of income necessary to cover the expense for the  
  patent application, maintaining the patent, and continuing this  
  research.

  E. Thus, we plan to make this algorithm/software free even for  
  commercial uses, if the IPCs can obtain this amount of income from  
  large companies (such as Microsoft or Apple) or governmental  
  organizations, in the form of royalties or contributions.

  F. The names of companies and organizations who pay royalties or make  
  contributions will be included in this program when it is revised,  
  under the consent of the inventors, and will be advertised as  
  sponsors on the home page of this algorithm.

  G. This algorithm is an applicant for the eSTREAM stream cipher  
  project http://www.ecrypt.eu.org/stream/. If eSTREAM selects this  
  algorithm as one of the recommendable stream ciphers in its final  
  report planned for 2008 January, then we will make this algorithm/ 
  software free even for commercial use, regardless of the condition in  
  E. However, preferably we hope to recieve some royalties or  
  contributions to satisfy the desire in item D.

  These statements are claimed by us, namely, the Inventors listed in  
  item A, the IPC of Hiroshima University, and the IPC of Ochanomizu  
  University.

  Corresponding address: 
  Makoto Matsumoto, Department of Mathematics, Hiroshima University.
  1-3-1 Kagamiyama, Higashi-Hiroshima 739-8526 Japan.
  http://www.math.sci.hiroshima-u.ac.jp/~m-mat/eindex.html
  email: m-mat "at-mark" math.sci.hiroshima-u.ac.jp
*/

#include <stdio.h>
#include "ecrypt-sync.h"
#include "ecrypt-machine.h"

/* This is a stream cipher. One word means a 32 bit word. */
/* Tuple words will be gather to make one Block. */
/* Typically Tuple is 4. Log_Tuple is log2(Tuple) */
#define Log_Tuple 2
#define Tuple (U32C(1) << Log_Tuple)
#define Low_Mask (Tuple-1)
#define Iteration 4 /* <=32: number of primitive encrypt function iterations */
#define Multi_Size 32 /* number of constant multipliers */
#define Log_Add_Size 5 /* Log of Add_Size */
#define Add_Size 32 /* number of constant adders */

static void init_genrand(ECRYPT_ctx* ctx, u32 s);
static void init_by_array(ECRYPT_ctx* ctx, u32 init_key[], int key_length);
static void genrand_whole_array(ECRYPT_ctx* ctx);
static void genrand_tuple_int32(ECRYPT_ctx* ctx, u32 rand_tuple[], s32 len);

static void prepare_multi(ECRYPT_ctx *ctx);
static u32 inv_mod_32(u32 m);
static void prepare_multi_inv(ECRYPT_ctx *ctx);
static void prepare_add_table(ECRYPT_ctx *ctx);
static void crypt_empr(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_empr_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_emer(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_emer_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_emps(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_emps_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_emes(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_emes_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_ma(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_ma_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_mem(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_mem_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_ome(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_ome_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_eme(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_eme_inv(ECRYPT_ctx* ctx, u32 block[Tuple], u32 param[Tuple]);
static void crypt_vert_rotate(ECRYPT_ctx *ctx, u32 block[Tuple]);
static void crypt_vert_rotate_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple]);
static void set_buf(u32 buf[], const u8* text, u32 cpos, u32 msglen);
static void set_array(u32 buf[], u8* text, u32 cpos);


/* Period Parameters of MT */  
#define N 624
#define M 397
#define MATRIX_A U32C(0x9908B0DF)   /* constant vector a */
#define UPPER_MASK U32C(0x80000000) /* most significant w-r bits */
#define LOWER_MASK U32C(0x7FFFFFFF) /* least significant r bits */

/* initializes ctx->mt[N] with a seed */
static void init_genrand(ECRYPT_ctx* ctx, u32 s)
{
    ctx->mt[0]= s & U32C(0xFFFFFFFF);
    for (ctx->i=1; ctx->i<N; ctx->i++) {
        ctx->mt[ctx->i] = 
	    (U32C(1812433253) * (ctx->mt[ctx->i-1] ^ (ctx->mt[ctx->i-1] >> 30)) + ctx->i); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
static void init_by_array(ECRYPT_ctx* ctx, u32 init_key[], int key_length)
{
    int i, j, k;
    init_genrand(ctx, U32C(19650218));
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        ctx->mt[i] = (ctx->mt[i] ^ ((ctx->mt[i-1] ^ (ctx->mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        i++; j++;
        if (i>=N) { ctx->mt[0] = ctx->mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        ctx->mt[i] = (ctx->mt[i] ^ ((ctx->mt[i-1] ^ (ctx->mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        i++;
        if (i>=N) { ctx->mt[0] = ctx->mt[N-1]; i=1; }
    }

    ctx->mt[0] = U32C(0x80000000); /* MSB is 1; assuring non-zero initial array */ 
}

/* generates whole array of random numbers in [0,0xffffffff]-interval */
static void genrand_whole_array(ECRYPT_ctx* ctx)
{
    u32 y;
    static u32 mag01[2]={U32C(0x0), MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    int kk;
    
    for (kk=0;kk<N-M;kk++) {
      y = (ctx->mt[kk]&UPPER_MASK)|(ctx->mt[kk+1]&LOWER_MASK);
      ctx->mt[kk] = ctx->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (;kk<N-1;kk++) {
      y = (ctx->mt[kk]&UPPER_MASK)|(ctx->mt[kk+1]&LOWER_MASK);
      ctx->mt[kk] = ctx->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (ctx->mt[N-1]&UPPER_MASK)|(ctx->mt[0]&LOWER_MASK);
    ctx->mt[N-1] = ctx->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];
    
    ctx->i = 0;
    return ;
}

/* generates len (<624) random numbers on [0,0xffffffff]-interval */
/* rand_tuple[0]-[len-1] will be supplied with random numbers */
static void genrand_tuple_int32(ECRYPT_ctx* ctx, u32 rand_tuple[], s32 len)
{
    s32 i;

    if (ctx->i + len < N) {
	for (i=0; i<len; i++) 
	    rand_tuple[i] = ctx->mt[ctx->i++];
    }
    else {
	for (i=0; i<len; i++) {
	    if (ctx->i >= N) 
		genrand_whole_array(ctx);
	    rand_tuple[i] = ctx->mt[ctx->i++];
	}
    }
}



/**********************************************/
/********  Precomputation of Tables  **********/
/**********************************************/

/* Compute Multiplication Constants: 3 mod 8, 7 mod 16 */
static void prepare_multi(ECRYPT_ctx *ctx)
{
    s32 i;

    for (i=0; i<Multi_Size; i+=4) {
	genrand_tuple_int32(ctx, &ctx->multi_table[i], 4);
    }
    for (i=0; i< Multi_Size; i+=2) {
	ctx->multi_table[i] = (ctx->multi_table[i] & U32C(0xfffffff8)) | U32C(0x3);
	ctx->multi_table[i] |= (U32C(0x80000000) >> (i % 8));
	ctx->multi_table[i] &= ~(U32C(0x40000000) >> (i % 8));

	ctx->multi_table[i+1] = (ctx->multi_table[i+1] & U32C(0xfffffff0)) | U32C(0x7);
	ctx->multi_table[i+1] |= (U32C(0x80000000) >> ((i+1) % 8));
	ctx->multi_table[i+1] &= ~(U32C(0x40000000) >> ((i+1) % 8));
    }  
}

/* compute multiplicative inverse mod 2^32 */
static u32 inv_mod_32(u32 m)
{
    u32 inv;
    s32 i;
    if ((m & U32C(0x1)) == 0) { printf("error\n"); return -1;}
    inv = 1;
    for (i=30; i>=0; i--) {
      if (((inv * m - 1) << i) != 0) inv |= (0x1 << (32-i-1));
    }
    return inv;
}


/* prepare the table of inverses */
static void prepare_multi_inv(ECRYPT_ctx *ctx)
{
    s32 i;
    for (i=0; i< Multi_Size; i++) {
	ctx->inv_table[i] = inv_mod_32(ctx->multi_table[i]);
    }
}

/* Compute addition constants */
static void prepare_add_table(ECRYPT_ctx *ctx)
{
    s32 i;
    
    genrand_tuple_int32(ctx, ctx->add_table, Add_Size);

    for (i=0; i< Add_Size; i++) {
        u32 s;
	s = (i * 1103515245 + 12345) & (Add_Size - 1);
	s ^= (s >> (Log_Add_Size / 2));
        ctx->add_table[i] <<= Log_Add_Size;
        ctx->add_table[i] |= s;
    }  
}

/**********************************************/
/*******Primitive Ecnryption Families**********/
/**********************************************/

/**********************************************/
/*******PEF (almost word wise operation)*******/
/**********************************************/

/* word wise encrypt by Exor Mult table-Plus Rotate */
/* rotate number is between 16 - 23 */
static void crypt_empr(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, s;
    u32 param[Tuple];
    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= param[i];
      block[i] *= ctx->multi_table[param[(i+1) % Tuple]>> (32 - 5)];
      block[(i+ctx->jump) & Low_Mask] += ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] = ((~block[i]) << (32 - s)) | (block[i] >> s);
    }
}

static void crypt_empr_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, s;
    for (i=Tuple-1; i>=0; i--) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] = ((~block[i]) >> (32 - s)) | (block[i] << s);
      block[(i+ctx->jump) & Low_Mask] -= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] *= ctx->inv_table[param[(i+1) % Tuple]>> (32 - 5)];
      block[i] ^= param[i];
    }
}

/* word wise encrypt by Exor Mult table-Exor Rotate */
/* rotate number is between 16 - 23 */
static void crypt_emer(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, s;
    u32 param[Tuple];
    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= param[i];
      block[i] *= ctx->multi_table[param[(i+2) % Tuple]>> (32 - 5)];
      block[(i+ctx->jump) & Low_Mask] ^= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] = ((~block[i]) << (32 - s)) | (block[i] >> s);
    }
}

static void crypt_emer_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, s;
    for (i=Tuple-1; i>=0; i--) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] = ((~block[i]) >> (32 - s)) | (block[i] << s);
      block[(i+ctx->jump) & Low_Mask] ^= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] *= ctx->inv_table[param[(i+2) % Tuple]>> (32 - 5)];
      block[i] ^= param[i];
    }
}

/* word wise encrypt by Exor Mult table-Plus Shift */
/* Shift number is betwee 16 - 23 */
static void crypt_emps(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, s;
    u32 param[Tuple];
    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= param[i];
      block[i] *= ctx->multi_table[param[(i+2) % Tuple]>> (32 - 5)];
      block[(i+ctx->jump) & Low_Mask] += ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] ^= ((~block[i]) >> s);
    }
}

static void crypt_emps_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, s;
    for (i=Tuple-1; i>=0; i--) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= ((~block[i]) >> s);
      block[(i+ctx->jump) & Low_Mask] -= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] *= ctx->inv_table[param[(i+2) % Tuple]>> (32 - 5)];
      block[i] ^= param[i];
    }
}

/* word wise encrypt by Exor Mult table-Exor Shift */
/* Shift number is betwee 16 - 23 */
static void crypt_emes(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, s;
    u32 param[Tuple];
    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= param[i];
      block[i] *= ctx->multi_table[param[(i+3) % Tuple]>> (32 - 5)];
      block[(i+ctx->jump) & Low_Mask] ^= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] ^= ((~block[i]) >> s);
    }
}

static void crypt_emes_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, s;
    for (i=Tuple-1; i>=0; i--) {
      s = ((param[i] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= ((~block[i]) >> s);
      block[(i+ctx->jump) & Low_Mask] ^= ctx->add_table[block[i] >> (32 - Log_Add_Size)];
      block[i] *= ctx->inv_table[param[(i+3) % Tuple]>> (32 - 5)];
      block[i] ^= param[i];
    }
}

/* Inter-word operations */
/* multiply to one and add to the other */
static void crypt_ma(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, j, s;
    u32 param[Tuple];

    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      j = (i - ctx->jump) & Low_Mask;
      s = ((param[j] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] += (block[j]*param[i]);
      block[i] ^= ((~block[i]) >> s);
    }
}

static void crypt_ma_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, j, s;

    for (i=Tuple-1; i>=0; i--) {
      j = (i - ctx->jump) & Low_Mask;
      s = ((param[j] >> (32 - 4)) | 0x10 ) & 0x17;
      block[i] ^= ((~block[i]) >> s);
      block[i] -= (block[j]*param[i]);
    }
}

/* multiply two words, exor to another words, and  minus */
static void crypt_mem(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, j, k;
    u32 param[Tuple];

    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[j]*block[k]);
      block[i] -= param[i];
      block[i] ^= (block[i] >> 16);
    }
}

static void crypt_mem_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, j, k;

    for (i=Tuple-1; i>=0; i--) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[i] >> 16);
      block[i] += param[i];
      block[i] ^= (block[j]*block[k]);
    }
}

/* (one word OR param) times another word) is EXORed to another */
static void crypt_ome(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, j, k;
    u32 param[Tuple];

    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[k]|param[i])*block[j];
      block[i] ^= (block[i] >> 16);
    }
}

static void crypt_ome_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, j, k;

    for (i=Tuple-1; i>=0; i--) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[i] >> 16);
      block[i] ^= (block[k]|param[i])*block[j];
    }
}

/* (one word EXOR param) times another word) is EXORed to another */
static void crypt_eme(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    s32 i, j, k;
    u32 param[Tuple];

    genrand_tuple_int32(ctx, param, Tuple);
    for (i=0; i<Tuple; i++) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[k]^param[i])*block[j];
      block[i] ^= (block[i] >> 17);
    }
}

static void crypt_eme_inv(ECRYPT_ctx* ctx, u32 block[Tuple], u32 param[Tuple])
{
    s32 i, j, k;

    for (i=Tuple-1; i>=0; i--) {
      j = (i - ctx->jump) & Low_Mask;
      k = param[j] >> (32 - Log_Tuple);
      if (k==i) k=(k-1) & Low_Mask;
      block[i] ^= (block[i] >> 17);
      block[i] ^= (block[k]^param[i])*block[j];
    }
}

/* vertical partial rotation with bit inversion*/
static void crypt_vert_rotate(ECRYPT_ctx *ctx, u32 block[Tuple])
{
    u32 key, rkey, s;
    s32 i, j, jump_odd;
    u32 param[Tuple];

    jump_odd = (ctx->jump - 1) | 0x1; 

    genrand_tuple_int32(ctx, param, Tuple);

    key = ((param[0]+param[Tuple-1])<< 2) + 1;
    rkey = ~key;
    s = block[0];
    j = 0; 
    for (i=0; i<Tuple ; i++) {
      int u;
      u = (j-jump_odd) & Low_Mask;
      block[j] = (block[j] & rkey) | (~block[u] & key);
      j = u;
    }
    block[j] = (block[j] & rkey) | (~s & key);
    for (i=0; i<Tuple; i++) {
      block[i] += param[i];
    }
}

static void crypt_vert_rotate_inv(ECRYPT_ctx *ctx, u32 block[Tuple], u32 param[Tuple])
{
    u32 key, rkey, s;
    s32 i, j, jump_odd;

    jump_odd = (ctx->jump - 1) | 0x1; 

    for (i=0; i<Tuple; i++) {
      block[i] -= param[i];
    }

    key = ((param[0]+param[Tuple-1])<< 2) + 1;
    rkey = ~key;
    s = block[0];
    j = 0; 
    for (i=0; i<Tuple; i++) {
      int u;
      u = (j+jump_odd) & Low_Mask;
      block[j] = (block[j] & rkey) | (~block[u] & key);
      j = u;
    }
    block[j] = (block[j] & rkey) | (~s & key);
}

static void set_buf(u32 buf[], const u8* text, u32 cpos, u32 msglen)
{
    u32 x;
    s32 i, j, s, t, diff;

    diff = msglen - cpos; 
    if ( diff >= (4*Tuple) ) {
	for (i=0; i<Tuple; i++) {
	    buf[i] = U8TO32_LITTLE(text + cpos);
	    cpos += 4;
	}
    }
    else {
	for (i=0; i<Tuple; i++) buf[i] = 0;
	s = diff >> 2;
	t = diff & 3;
	for (i=0; i<s; i++) {
	    buf[i] = U8TO32_LITTLE(text + cpos);
	    cpos += 4;
	}
	x = 0;
	for (j=0; j<t; j++) {
	    x |= (u32)text[cpos++] << (j<<3);
	}
	buf[i] = x;
    }
}

static void set_array(u32 buf[], u8* text, u32 cpos)
{
    s32 i;

    for (i=0; i<Tuple; i++) {
	U32TO8_LITTLE(text + cpos, buf[i]);
	cpos += 4;
    }
}


void hmnencode(ECRYPT_ctx *ctx, 
	       const u8* plaintext, u8* ciphertext,  u32 msglen)  /* Message length in bytes. */ 
{
    s32 i, j, repeat; 
    u32 msgbuf[Tuple];
    u32 cinpos, coutpos;

    repeat = msglen/(4*Tuple);
    if (msglen % (4*Tuple)) 
	repeat++;
    cinpos = coutpos = 0;;
    for (i=0; i<repeat; i++) {
	u32 func_choice[Tuple];

	set_buf(msgbuf, plaintext, cinpos, msglen);
	cinpos += 4*Tuple;

	genrand_tuple_int32(ctx, func_choice, 4);
	
	func_choice[2] *= (func_choice[0] | 1);
	func_choice[3] *= (func_choice[1] | 1);
	func_choice[0] ^= (func_choice[3] >> 5);
	func_choice[1] ^= (func_choice[2] >> 5);

	ctx->jump = 1;
	for (j=0; j< 2*Iteration;) {
	    s32 c, t;

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j++ & 0xf) * 2)) & 3;
	    switch (c) {
	    case 0: crypt_empr(ctx, msgbuf); break;
	    case 1: crypt_emer(ctx, msgbuf); break;
	    case 2: crypt_emps(ctx, msgbuf); break;
	    case 3: crypt_emes(ctx, msgbuf); break;
	    }


	    if ((ctx->jump <<= 1) >= Tuple) ctx->jump = 1;

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j++ & 0xf) * 2)) & 3;

	    switch (c) {
	    case 0: crypt_ma(ctx, msgbuf); break;
	    case 1: crypt_mem(ctx, msgbuf); break;
	    case 2: crypt_ome(ctx, msgbuf); break;
	    case 3: crypt_eme(ctx, msgbuf); break;
	    }
	    if ((ctx->jump <<= 1) >= Tuple) ctx->jump = 1;

	    crypt_vert_rotate(ctx, msgbuf);
	    if ((ctx->jump <<= 1) >= Tuple) ctx->jump = 1;
	}

	set_array(msgbuf, ciphertext, coutpos);
	coutpos += 4*Tuple;
    }
}

void hmndecode(ECRYPT_ctx *ctx, 
	       const u8* ciphertext, u8* plaintext, u32 msglen) /* Message length in bytes. */ 
{
    s32 i, j, k, repeat;
    u32 temp_rand[3*Iteration][Tuple];
    u32 msgbuf[Tuple];
    s32 cinpos, coutpos;

    repeat = msglen/(4*Tuple);

    cinpos = coutpos = 0;
    for (i=0; i<repeat; i++) {
	u32 func_choice[Tuple];

	set_buf(msgbuf, ciphertext, cinpos, msglen);
	cinpos += 4*Tuple;

	genrand_tuple_int32(ctx, func_choice, 4);

	func_choice[2] *= (func_choice[0] | 1);
	func_choice[3] *= (func_choice[1] | 1);
	func_choice[0] ^= (func_choice[3] >> 5);
	func_choice[1] ^= (func_choice[2] >> 5);

	for (k=0; k< 3*Iteration; k++) 
	    genrand_tuple_int32(ctx, temp_rand[k], Tuple);

	ctx->jump = 1 << ((3*Iteration-1) % Log_Tuple);

	for (j=2*Iteration - 1; j>=0;) {
	    s32 c, t;

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j-- & 0xf) * 2)) & 3;

	    crypt_vert_rotate_inv(ctx, msgbuf, temp_rand[--k]);
	    if ((ctx->jump >>= 1) == 0) ctx->jump = Tuple/2;

	    switch (c) {
	    case 0: crypt_ma_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 1: crypt_mem_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 2: crypt_ome_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 3: crypt_eme_inv(ctx, msgbuf, temp_rand[--k]); break;
	    }
	    if ((ctx->jump >>= 1) == 0) ctx->jump = Tuple/2;

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j-- & 0xf) * 2)) & 3;

	    switch (c) {
	    case 0: crypt_empr_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 1: crypt_emer_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 2: crypt_emps_inv(ctx, msgbuf, temp_rand[--k]); break;
	    case 3: crypt_emes_inv(ctx, msgbuf, temp_rand[--k]); break;
	    }
	    if ((ctx->jump >>= 1) == 0) ctx->jump = Tuple/2;
	}

	set_array(msgbuf, plaintext, coutpos);
	coutpos += 4*Tuple;
    }
}

void ECRYPT_init()
{
}

void ECRYPT_keysetup(
  ECRYPT_ctx* ctx, 
  const u8* key, 
  u32 keysize,               /* Key size in bits. */ 
  u32 ivsize)                /* IV size in bits. */ 
{
    s32 i;

    ctx->keysize = keysize;
    ctx->ivsize = ivsize;

    for (i = 0; i < ctx->keysize/8; i++)
	ctx->key[i] = key[i];
}


void ECRYPT_ivsetup(
  ECRYPT_ctx* ctx, 
  const u8* iv)
{
    s32 i,j,k,t,s;
    u32 x, init_array[(ECRYPT_MAXKEYSIZE+ECRYPT_MAXIVSIZE)/32];

    j = 0;
    t = ctx->keysize/32;
    for (i=0; i<t; i++) {
	init_array[i] = U8TO32_LITTLE(ctx->key + j);
	j += 4;
    }
    if ( ctx->keysize % 32 != 0 ) {
	x = 0;
	k = (ctx->keysize % 32)/8;
	for (i=0; i<k; i++) {
	    x |= ((u32)ctx->key[j++]) << (8*k);
	}
	init_array[t++] = x;
    }

    j = 0;
    s = ctx->ivsize/32;
    for (i=0; i<s; i++) {
	init_array[t+i] = U8TO32_LITTLE(iv + j);
	j += 4;
    }
    if ( ctx->ivsize % 32 != 0 ) {
	x = 0;
	k = (ctx->ivsize % 32)/8;
	for (i=0; i<k; i++) {
	    x |= ((u32)iv[j++]) << (8*k);
	}
	init_array[t+(s++)] = x;
    }
    init_by_array(ctx, init_array, t+s);

    prepare_multi(ctx);
    prepare_multi_inv(ctx);
    prepare_add_table(ctx);
}

void ECRYPT_keystream_bytes(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 length) /* length % (4*Tuple) == 0 */   /* Length of keystream in bytes. */
{
    s32 i, j, repeat; 
    u32 msgbuf[Tuple];
    u32 coutpos;

    repeat = length/(4*Tuple);
    if (length % (4*Tuple)) 
	repeat++;
    coutpos = 0;;
    for (i=0; i<repeat; i++) {
	u32 func_choice[Tuple];

	for (j=0; j<Tuple; j++) msgbuf[j] = 0;

	genrand_tuple_int32(ctx, func_choice, 4);
	
	func_choice[2] *= (func_choice[0] | 1);
	func_choice[3] *= (func_choice[1] | 1);
	func_choice[0] ^= (func_choice[3] >> 5);
	func_choice[1] ^= (func_choice[2] >> 5);

	for (j=0; j< 2*Iteration;) {
	    s32 c, t;

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j++ & 0xf) * 2)) & 3;
	    switch (c) {
	    case 0: crypt_empr(ctx, msgbuf); break;
	    case 1: crypt_emer(ctx, msgbuf); break;
	    case 2: crypt_emps(ctx, msgbuf); break;
	    case 3: crypt_emes(ctx, msgbuf); break;
	    }

	    t = j >> 4; 
	    c = (func_choice[t] >> ((j++ & 0xf) * 2)) & 3;

	    switch (c) {
	    case 0: crypt_ma(ctx, msgbuf); break;
	    case 1: crypt_mem(ctx, msgbuf); break;
	    case 2: crypt_ome(ctx, msgbuf); break;
	    case 3: crypt_eme(ctx, msgbuf); break;
	    }
	    crypt_vert_rotate(ctx, msgbuf);
	}

	set_array(msgbuf, keystream, coutpos);
	coutpos += 4*Tuple;
    }
}

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* plaintext, 
  u8* ciphertext, 
  u32 msglen)                /* Message length in bytes. */ 
{
    hmnencode(ctx, plaintext, ciphertext, msglen);
}

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx* ctx, 
  const u8* ciphertext, 
  u8* plaintext, 
  u32 msglen)                /* Message length in bytes. */ 
{
    if ( (msglen % (4*Tuple)) != 0 ) {
	printf("ECRYPT_decrypt_bytes: msglen should be multiple of %d.\n", 4*Tuple);
    	return;
    }
    hmndecode(ctx, ciphertext, plaintext, msglen);
}

