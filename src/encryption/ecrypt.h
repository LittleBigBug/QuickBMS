/*
http://cr.yp.to/streamciphers.html
    CryptMT: Patent pending, but noncommercial use is free, and commercial use is free if CryptMT appears in the final eSTREAM portfolio.
    DECIM (version 1 broken): Patent pending. No free use.
    Frogbit (broken): Patent pending. No free use.
    ProVEST (only 100-bit security): Patent pending. No free use.
    Rabbit: Patent pending, but noncommercial use is free.
    ZK-Crypt (version 1 broken): Patent pending. No free use. 
*/

#if defined(ECRYPT_SYNC_AE) || defined(ECRYPT_SSYN_AE)
#ifndef ECRYPT_AE
    #define ECRYPT_AE
    #ifndef ECRYPT_AE_ctx
    #define ECRYPT_AE_ctx              ECRYPT_ctx
    #endif
    #ifndef ECRYPT_AE_init
    #define ECRYPT_AE_init             ECRYPT_init
    #endif
    #ifndef ECRYPT_AE_keysetup
    #define ECRYPT_AE_keysetup         ECRYPT_keysetup
    #endif
    #ifndef ECRYPT_AE_ivsetup
    #define ECRYPT_AE_ivsetup          ECRYPT_ivsetup
    #endif
    #ifndef ECRYPT_AE_process_bytes
    #define ECRYPT_AE_process_bytes    ECRYPT_process_bytes
    #endif
    #ifndef ECRYPT_AE_decrypt_bytes
    #define ECRYPT_AE_decrypt_bytes    ECRYPT_decrypt_bytes
    #endif
    #ifndef ECRYPT_AE_encrypt_bytes
    #define ECRYPT_AE_encrypt_bytes    ECRYPT_encrypt_bytes
    #endif
    #ifndef ECRYPT_AE_keystream_bytes
    #define ECRYPT_AE_keystream_bytes  ECRYPT_keystream_bytes
    #endif
    #ifndef ECRYPT_AE_process_packet
    #define ECRYPT_AE_process_packet   ECRYPT_process_packet
    #endif
    #ifndef ECRYPT_AE_encrypt_packet
    #define ECRYPT_AE_encrypt_packet   ECRYPT_encrypt_packet
    #endif
    #ifndef ECRYPT_AE_decrypt_packet
    #define ECRYPT_AE_decrypt_packet   ECRYPT_decrypt_packet
    #endif
    #ifndef ECRYPT_AE_process_blocks
    #define ECRYPT_AE_process_blocks   ECRYPT_process_blocks
    #endif
    #ifndef ECRYPT_AE_encrypt_blocks
    #define ECRYPT_AE_encrypt_blocks   ECRYPT_encrypt_blocks
    #endif
    #ifndef ECRYPT_AE_decrypt_blocks
    #define ECRYPT_AE_decrypt_blocks   ECRYPT_decrypt_blocks
    #endif
    #ifndef ECRYPT_AE_keystream_blocks
    #define ECRYPT_AE_keystream_blocks ECRYPT_keystream_blocks
    #endif
#endif
#endif



#define ECRYPT_FUNCTION_PROTOTYPE \
    (void *key, int key_len, void *iv, int iv_len, void *data, int size, int dec_enc)

#define ECRYPT_FUNCTION(MYFUNC) \
void do_##MYFUNC ECRYPT_FUNCTION_PROTOTYPE { \
	ECRYPT_ctx ctx; \
	ECRYPT_init(); \
	ECRYPT_keysetup(&ctx, key, key_len * 8, iv_len * 8); \
	ECRYPT_ivsetup(&ctx, iv); \
    if(!dec_enc)    ECRYPT_decrypt_bytes(&ctx, data, data, size); \
    if(dec_enc > 0) ECRYPT_encrypt_bytes(&ctx, data, data, size); \
    if(dec_enc < 0) ECRYPT_keystream_bytes(&ctx,     data, size); \
}

#define ECRYPT_FUNCTION_PREVIOUS(MYFUNC) \
void do_##MYFUNC ECRYPT_FUNCTION_PROTOTYPE { \
	ECRYPT_ctx ctx; \
	ECRYPT_init(); \
	ECRYPT_keysetup(&ctx, key, key_len * 8, iv_len * 8); \
	ECRYPT_ivsetup(&ctx, NULL, iv); \
    if(!dec_enc)    ECRYPT_decrypt_bytes(&ctx, NULL, data, data, size); \
    if(dec_enc > 0) ECRYPT_encrypt_bytes(&ctx, NULL, data, data, size); \
    if(dec_enc < 0) ECRYPT_keystream_bytes(&ctx,     data, size); \
}



    #undef QUICKBMS_ECRYPT_do
    #define QUICKBMS_ECRYPT_do(MYFUNC)



#ifdef QUICKBMS_ECRYPT_defs

    #undef QUICKBMS_ECRYPT_do
    #define QUICKBMS_ECRYPT_do(MYFUNC) \
        enum_##MYFUNC##_ECRYPT,

#endif



#ifdef QUICKBMS_ECRYPT_cmd

    #undef QUICKBMS_ECRYPT_do
    #define QUICKBMS_ECRYPT_do(MYFUNC) \
    } else if(!stricmp(type, #MYFUNC)) { \
        ecrypt_ctx = calloc(1, sizeof(ecrypt_context)); \
        if(!ecrypt_ctx) STD_ERR(QUICKBMS_ERROR_MEMORY); \
        ecrypt_ctx->algo    = enum_##MYFUNC##_ECRYPT; \
        ecrypt_ctx->keysz   = keysz; \
        ecrypt_ctx->key     = malloc_copy(NULL, key,  ecrypt_ctx->keysz); \
        if(ivec) { \
            ecrypt_ctx->ivecsz  = ivecsz; \
            ecrypt_ctx->ivec    = malloc_copy(NULL, ivec, ecrypt_ctx->ivecsz); \
        } else { \
            ecrypt_ctx->ivecsz  = keysz; \
            ecrypt_ctx->ivec    = calloc(ecrypt_ctx->ivecsz, 1); \
        }

#endif



#ifdef QUICKBMS_ECRYPT_perform

    #undef QUICKBMS_ECRYPT_do
    #define QUICKBMS_ECRYPT_do(MYFUNC) \
    } else if(ecrypt_ctx->algo == enum_##MYFUNC##_ECRYPT) { \
        void do_##MYFUNC##_ECRYPT ECRYPT_FUNCTION_PROTOTYPE; \
        do_##MYFUNC##_ECRYPT(ecrypt_ctx->key, ecrypt_ctx->keysz, ecrypt_ctx->ivec, ecrypt_ctx->ivecsz, data, datalen, g_encrypt_mode);

#endif



    QUICKBMS_ECRYPT_do(abc)
    QUICKBMS_ECRYPT_do(achterbahn)
    QUICKBMS_ECRYPT_do(achterbahn128)
    QUICKBMS_ECRYPT_do(cryptmt)
    //QUICKBMS_ECRYPT_do(decim)
    QUICKBMS_ECRYPT_do(dicing)
    QUICKBMS_ECRYPT_do(dragon)
    QUICKBMS_ECRYPT_do(edon80)
    QUICKBMS_ECRYPT_do(ffcsr8)
    //QUICKBMS_ECRYPT_do(frogbit)
    QUICKBMS_ECRYPT_do(fubuki)
    QUICKBMS_ECRYPT_do(grain)
    QUICKBMS_ECRYPT_do(grain128)
    QUICKBMS_ECRYPT_do(hc128)
    QUICKBMS_ECRYPT_do(hc256)
    QUICKBMS_ECRYPT_do(hermes128)
    QUICKBMS_ECRYPT_do(hermes80)
    QUICKBMS_ECRYPT_do(lex)
    QUICKBMS_ECRYPT_do(mag)
    QUICKBMS_ECRYPT_do(mickey)
    QUICKBMS_ECRYPT_do(mickey128)
    QUICKBMS_ECRYPT_do(mir1)
    QUICKBMS_ECRYPT_do(mosquito)
    QUICKBMS_ECRYPT_do(moustique)
    QUICKBMS_ECRYPT_do(nls)
    //QUICKBMS_ECRYPT_do(phelix)
    QUICKBMS_ECRYPT_do(polarbear)
    QUICKBMS_ECRYPT_do(pomaranch)
    QUICKBMS_ECRYPT_do(py)
    QUICKBMS_ECRYPT_do(rabbit)
    QUICKBMS_ECRYPT_do(salsa20)
    QUICKBMS_ECRYPT_do(sfinks)
    QUICKBMS_ECRYPT_do(sosemanuk)
    QUICKBMS_ECRYPT_do(sss)
    QUICKBMS_ECRYPT_do(trivium)
    QUICKBMS_ECRYPT_do(tsc3)
    QUICKBMS_ECRYPT_do(tsc4)
    //QUICKBMS_ECRYPT_do(vest)
    QUICKBMS_ECRYPT_do(wg)
    QUICKBMS_ECRYPT_do(yamb)
    //QUICKBMS_ECRYPT_do(zkcrypt)

