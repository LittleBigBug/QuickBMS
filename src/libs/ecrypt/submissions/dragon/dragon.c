/**
 * @file dragon-ref.c
 * Implementation of Dragon
 * This source is provided without warranty
 * or guarantee of any kind. Use at your own risk.
 * @author Information Security Institute
 */
#include <assert.h>

#include "ecrypt-sync.h"
#include "ecrypt-portable.h"
#include "dragon-sboxes.c"

/**
 * The DRAGON_OFFSET macro calculates the position of the 
 * ith_element within the circular buffer that represents the
 * NLFSR.
 */
#define DRAGON_OFFSET(ctx, ith_element, state_size) \
    ((ctx->nlfsr_offset + ith_element) & state_size)

/**
 * The DRAGON_NLFSR_WORD macro retrieves the ith 32-bit word
 * from the Dragon NLFSR.
 */
#define DRAGON_NLFSR_WORD(ctx, ith_word) \
    *(ctx->nlfsr_word + DRAGON_OFFSET(ctx, ith_word, (DRAGON_NLFSR_SIZE - 1)))

/**
 * The Dragon update function consists of a pre-mixing, post-mixing and s-box
 * layer. The pre- and post-mixing in turn consist of one XOR layer and one
 * ADD layer. The s-box layer contains two sublayers, one which uses three G
 * s-boxes, and the other which uses three H s-boxes.
 */
#define XOR_LAYER(d1, s1, d2, s2, d3, s3) \
    d1 ^= s1; \
    d2 ^= s2; \
    d3 ^= s3; 

#define ADD_LAYER(d1, s1, d2, s2, d3, s3) \
    d1 += s1; \
    d2 += s2; \
    d3 += s3; \

#define SBOX_LAYER(sbox, d1, s1, d2, s2, d3, s3) \
    d1 ^= sbox##1(s1); \
    d2 ^= sbox##2(s2); \
    d3 ^= sbox##3(s3); 

#define DRAGON_UPDATE(a, b, c, d, e, f) \
    XOR_LAYER(b, a, d, c, f, e) \
    ADD_LAYER(c, b, e, d, a, f) \
    SBOX_LAYER(G, d, a, f, c, b, e) \
    SBOX_LAYER(H, a, b, c, d, e, f) \
    ADD_LAYER(f, c, b, e, d, a) \
    XOR_LAYER(a, f, c, b, e, d)
    
/*
 * Key and message independent initialization. This function will be
 * called once when the program starts (e.g., to build expanded S-box
 * tables).
 */
void ECRYPT_init(void)
{
}

/*
 * Key setup. It is the user's responsibility to select the values of
 * keysize and ivsize from the set of supported values specified
 * above.
 */
void ECRYPT_keysetup(
    ECRYPT_ctx*  ctx,
    const u8*    key,
    u32          keysize, /* Key size in bits. */
    u32          ivsize)  /* IV size in bits. */
{
    u32   idx;
    u32   key_word;

    assert(ctx && key);

    ctx->key_size      = keysize;
 
    /**
      * Dragon supports the following combinations of key and IV sizes only:
      * (128, 128) and (256, 256) bits. Mix and matching is not supported,
      * nor are other sizes. The ivsize parameter is ignored here.
      */
    if (keysize == 128) 
    {
        /* For a keysize of 128 bits, the Dragon NLFSR is initialized 
           using K and IV as follows (where k' and iv' represent 
           swapping of halves of key and iv respectively):
           k | k' ^ iv' | iv | (k ^ iv') | k' | (k ^ iv) | iv' | (k' ^ iv)           
         */
        for (idx = 0; idx < 4; idx++) {
            key_word = U8TO32_BIG(key + idx * 4);
            
            ctx->nlfsr_word[0+idx]  = ctx->nlfsr_word[12+idx] = 
                ctx->nlfsr_word[20+idx] = key_word;
        }
                
        /* then write k' */
        for (idx = 0; idx < 2; idx++) {
            key_word = U8TO32_BIG(key + 8 + idx * 4 );
            ctx->nlfsr_word[4+idx] = ctx->nlfsr_word[16+idx] = 
                ctx->nlfsr_word[28+idx] = key_word;

            key_word = U8TO32_BIG(key + idx * 4);
            ctx->nlfsr_word[6+idx] = ctx->nlfsr_word[18+idx] = 
                ctx->nlfsr_word[30+idx] = key_word;
        }
    }
    else 
    {
        /* For a keysize of 256 bits, the Dragon NLFSR is initialized 
           using K and IV as follows (where {k} and {iv} represent 
           the bitwise complements of keys and ivs repsectively:
           k | k ^ iv | {k ^ iv} | iv
         */
        for (idx = 0; idx < 8; idx++) {
            key_word = U8TO32_BIG(key + idx * 4);

            ctx->nlfsr_word[0+idx]  = ctx->nlfsr_word[8+idx] = key_word;
            ctx->nlfsr_word[16+idx] = key_word;
        }
    }

    /* Preserve the state for the key-IV-IV-... scenario described below
     */
    for (idx = 0; idx < DRAGON_NLFSR_SIZE; idx++) {
        ctx->init_state[idx] = ctx->nlfsr_word[idx];
    }
}

#define DRAGON_MIXING_STAGES   16 /* number of mixes during initialization */

/*
 * IV setup. After having called ECRYPT_keysetup(), the user is
 * allowed to call ECRYPT_ivsetup() different times in order to
 * encrypt/decrypt different messages with the same key but different
 * IV's.
 */
void ECRYPT_ivsetup(
    ECRYPT_ctx* ctx,
    const u8* iv)
{
    u32 a, b, c, d;
    u32 e = 0x00004472;
    u32 f = 0x61676F6E;
    u32 iv_word;
    u32 idx;
    
    assert(ctx && iv);

    /**
      * Restore the state to the post-keysetup state.
      */
    for (idx = 0; idx < DRAGON_NLFSR_SIZE; idx++) {
      ctx->nlfsr_word[idx] = ctx->init_state[idx];
    }

    /* For a keysize of 128 bits, the Dragon NLFSR is initialized 
       using K and IV as follows (where k' and iv' represent 
       swapping of halves of key and iv respectively):
       k | k' ^ iv' | iv | k ^ iv' | k' | k ^ iv | iv' | k' ^ iv

       Therefore initialization of locations {0, 1, 8, 9} are 
       deliberately omitted here, as the iv is not involved.
    */
    if (ctx->key_size == 128) 
    {
        /* write iv first */
        for (idx = 0; idx < 4; idx++) {
            iv_word = U8TO32_BIG(iv + idx * 4);

            ctx->nlfsr_word[8+idx]   = iv_word;
            ctx->nlfsr_word[20+idx] ^= iv_word;
            ctx->nlfsr_word[28+idx] ^= iv_word;
        }

        /* then iv' */
        for (idx = 0; idx < 2; idx++) {
            iv_word = U8TO32_BIG(iv + 8 + idx * 4);

            ctx->nlfsr_word[ 4+idx] ^= iv_word;
            ctx->nlfsr_word[12+idx] ^= iv_word;
            ctx->nlfsr_word[24+idx]  = iv_word;

            iv_word = U8TO32_BIG(iv + idx * 4);

            ctx->nlfsr_word[ 6+idx] ^= iv_word;
            ctx->nlfsr_word[14+idx] ^= iv_word;
            ctx->nlfsr_word[26+idx]  = iv_word;
        }
    }
    else 
    {
        /* For a keysize of 256 bits, the Dragon NLFSR is initialized 
           using K and IV as follows (where {k} and {iv} represent 
           the bitwise complements of keys and ivs repsectively:
           k | (k ^ iv) | {k ^ iv} | iv
         */
        for (idx = 0; idx < 8; idx++) {
            iv_word = U8TO32_BIG(iv + idx * 4);

            ctx->nlfsr_word[ 8 + idx] ^= iv_word;
            ctx->nlfsr_word[16 + idx] ^= (iv_word ^ 0xFFFFFFFF);
            ctx->nlfsr_word[24 + idx]  = iv_word;
        }
    }

    ctx->nlfsr_offset = 0;
    
    /** Iterate mixing process */
    for (idx = 0; idx < DRAGON_MIXING_STAGES; idx++) {
        a = DRAGON_NLFSR_WORD(ctx, 0)  ^ 
            DRAGON_NLFSR_WORD(ctx, 24) ^
            DRAGON_NLFSR_WORD(ctx, 28);

        b = DRAGON_NLFSR_WORD(ctx, 1)  ^
            DRAGON_NLFSR_WORD(ctx, 25) ^
            DRAGON_NLFSR_WORD(ctx, 29);

        c = DRAGON_NLFSR_WORD(ctx, 2)  ^
            DRAGON_NLFSR_WORD(ctx, 26) ^
            DRAGON_NLFSR_WORD(ctx, 30);

        d = DRAGON_NLFSR_WORD(ctx, 3)  ^
            DRAGON_NLFSR_WORD(ctx, 27) ^
            DRAGON_NLFSR_WORD(ctx, 31);

        DRAGON_UPDATE(a, b, c, d, e, f); 
     
        ctx->nlfsr_offset += (DRAGON_NLFSR_SIZE - 4); 

        DRAGON_NLFSR_WORD(ctx, 0) = a ^ DRAGON_NLFSR_WORD(ctx, 20);
        DRAGON_NLFSR_WORD(ctx, 1) = b ^ DRAGON_NLFSR_WORD(ctx, 21);
        DRAGON_NLFSR_WORD(ctx, 2) = c ^ DRAGON_NLFSR_WORD(ctx, 22);
        DRAGON_NLFSR_WORD(ctx, 3) = d ^ DRAGON_NLFSR_WORD(ctx, 23);
    }
    ctx->state_counter = ((u64)e << 32) | (u64)f;

    /* reset buffer index */
    ctx->buffer_index = 0;
}

/**
 * DRAGON_ROUND produces one block of keystream
 */
#define DRAGON_ROUND(ctx, a, b, c, d, e, f) \
    a = DRAGON_NLFSR_WORD(ctx, 0); \
    b = DRAGON_NLFSR_WORD(ctx, 9); \
    c = DRAGON_NLFSR_WORD(ctx, 16); \
    d = DRAGON_NLFSR_WORD(ctx, 19); \
    e = DRAGON_NLFSR_WORD(ctx, 30) ^ U32V(ctx->state_counter >> 32); \
    f = DRAGON_NLFSR_WORD(ctx, 31) ^ U32V(ctx->state_counter); \
    DRAGON_UPDATE(a, b, c, d, e, f); \
    ctx->state_counter++; \
    ctx->nlfsr_offset -= 2; \
    DRAGON_NLFSR_WORD(ctx, 0) = b;\
    DRAGON_NLFSR_WORD(ctx, 1) = c;
 
/**
 * Generate #(blocks) 64-bit blocks of keystream. 
 * @param  ctx        [In/Out]  Dragon context
 * @param  keystream  [Out]        pre-allocated array containing 8*(blocks)
 *                                bytes of memory
 * @param  blocks      [In]        number of keystream blocks to produce
 */
void ECRYPT_keystream_blocks(
  ECRYPT_ctx* ctx,
  u8* keystream,
  u32 blocks)
{
    u32 *k_ptr = (u32*)keystream;
    u32 a, b, c, d, e, f;

    assert(ctx && keystream);

    while (blocks > 0) {
        DRAGON_ROUND(ctx, a, b, c, d, e, f)
        *(k_ptr++) = U32TO32_BIG(a);
        *(k_ptr++) = U32TO32_BIG(e);
        blocks--;
    }
}

/**
 * Encrypt/Decrypt #(blocks) 64-bit blocks of text
 * @param  action  [In]         This parameter has no meaning for Dragon
 * @param  ctx     [In/Out]  Dragon context
 * @param  input   [In]      (plain/cipher)text blocks for (en/de)crypting
 * @param  output  [Out]     pre-allocated array for (cipher/plain)text blocks
 *                             consisting of 8*(blocks) bytes
 * @param  blocks  [In]         number of blocks to (en/de)crypt
 */
void ECRYPT_process_blocks(
  int action,                 /* 0 = encrypt; 1 = decrypt; */
  ECRYPT_ctx* ctx, 
  const u8* input, 
  u8* output, 
  u32 blocks)
{ 
    u32 *in = (u32*)input;
    u32 *out = (u32*)output;

    u32 a, b, c, d, e, f;

    assert(ctx && input && output);

    while (blocks > 0) {
        DRAGON_ROUND(ctx, a, b, c, d, e, f)
        *(out++) = U32TO32_BIG(a) ^ *(in++);
        *(out++) = U32TO32_BIG(e) ^ *(in++);
        blocks--;
    }
}

/**
 * Generate an arbitrary number of keystream bytes. Note this API 
 * is slower than block-wise encryption as Dragon is a 64-bit 
 * block-oriented  cipher
 *
 * @param  ctx        [In/Out]  Dragon context
 * @param  keystream  [Out]        pre-allocated array containing (msglen)
 *                                bytes
 * @param  length     [In]        number of keystream bytes to produce
 */
void ECRYPT_keystream_bytes(
    ECRYPT_ctx* ctx,
    u8* keystream,
    u32 length)
{
    assert(ctx && keystream);

    while ((length--) > 0) {
        if (ctx->buffer_index == 0) {
            ECRYPT_keystream_blocks(
                ctx, 
                ctx->keystream_buffer, 
                DRAGON_BUFFER_SIZE);
        }
        *(keystream++) = ctx->keystream_buffer[ctx->buffer_index];
        ctx->buffer_index = ((ctx->buffer_index + 1) % DRAGON_BUFFER_BYTES);
    }
}

/**
 * Encrypt an arbitrary number of bytes. Note this API is slower
 * than block-wise encryption as Dragon is a 64-bit block-oriented 
 * cipher
 *
 * @param  action  [In]      This parameter has no meaning for Dragon
 * @param  ctx     [In/Out]  Dragon context
 * @param  input   [In]         (plain)/(cipher)text for (en/de)crypting
 * @param  output  [Out]     (cipher)/(plain)text for (en/de)crypting
 * @param  msglen  [In]      number of bytes to (en/de)crypt
 */
void ECRYPT_process_bytes(
    int action,                 /* 0 = encrypt; 1 = decrypt; */
    ECRYPT_ctx* ctx,
    const u8* input,
    u8* output,
    u32 msglen)
{
    u32 len = msglen;
    u32 i;

    assert(ctx && input && output);

    ECRYPT_keystream_bytes(ctx, output, len);

    for (i = 0; i < msglen; i++) {
        output[i] ^= input[i];
    }
}

