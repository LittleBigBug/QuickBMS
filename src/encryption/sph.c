#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sph.h"

#ifdef WIN32
#else
    #define stricmp     strcasecmp
    #define strnicmp    strncasecmp
#endif



#include "../libs/sphlib/c/sph_types.h"
#include "../libs/sphlib/c/sha3nist.h"
#include "../libs/sphlib/c/sph_blake.h"
#include "../libs/sphlib/c/sph_bmw.h"
#include "../libs/sphlib/c/sph_cubehash.h"
#include "../libs/sphlib/c/sph_echo.h"
#include "../libs/sphlib/c/sph_fugue.h"
#include "../libs/sphlib/c/sph_groestl.h"
#include "../libs/sphlib/c/sph_hamsi.h"
#include "../libs/sphlib/c/sph_haval.h"
#include "../libs/sphlib/c/sph_jh.h"
#include "../libs/sphlib/c/sph_keccak.h"
#include "../libs/sphlib/c/sph_luffa.h"
#include "../libs/sphlib/c/sph_md2.h"
#include "../libs/sphlib/c/sph_md4.h"
#include "../libs/sphlib/c/sph_md5.h"
#include "../libs/sphlib/c/sph_panama.h"
#include "../libs/sphlib/c/sph_radiogatun.h"
#include "../libs/sphlib/c/sph_ripemd.h"
#include "../libs/sphlib/c/sph_sha0.h"
#include "../libs/sphlib/c/sph_sha1.h"
#include "../libs/sphlib/c/sph_sha2.h"
#include "../libs/sphlib/c/sph_sha3.h"
#include "../libs/sphlib/c/sph_shabal.h"
#include "../libs/sphlib/c/sph_shavite.h"
#include "../libs/sphlib/c/sph_simd.h"
#include "../libs/sphlib/c/sph_skein.h"
#include "../libs/sphlib/c/sph_tiger.h"
#include "../libs/sphlib/c/sph_whirlpool.h"



#define SPH_FUNCTION_ASSIGN(X) \
    if(!stricmp(ctx->algo, #X)) { \
        ctx->sph_function_size  = SPH_SIZE_##X; \
        ctx->cc = calloc(1, sizeof(sph_##X##_context)); \
        if(!ctx->cc) return -1; \
        ctx->sph_function_init  = sph_##X##_init; \
        ctx->sph_function       = sph_##X; \
        ctx->sph_function_close = sph_##X##_close; \
    } else



int sph(sph_context *ctx, unsigned char *data, int len, unsigned char *digest) {
    if(!ctx) return -1;

    if(!ctx->sph_function) {
        if(!ctx->algo) return -1;
        
        if(!strnicmp(ctx->algo, "sph", 3)) ctx->algo += 3;
        while(ctx->algo[0] == '_') ctx->algo++;
    
        SPH_FUNCTION_ASSIGN(blake224)
        SPH_FUNCTION_ASSIGN(blake256)
        SPH_FUNCTION_ASSIGN(blake384)
        SPH_FUNCTION_ASSIGN(blake512)
        SPH_FUNCTION_ASSIGN(bmw224)
        SPH_FUNCTION_ASSIGN(bmw256)
        SPH_FUNCTION_ASSIGN(bmw384)
        SPH_FUNCTION_ASSIGN(bmw512)
        SPH_FUNCTION_ASSIGN(cubehash224)
        SPH_FUNCTION_ASSIGN(cubehash256)
        SPH_FUNCTION_ASSIGN(cubehash384)
        SPH_FUNCTION_ASSIGN(cubehash512)
        SPH_FUNCTION_ASSIGN(echo224)
        SPH_FUNCTION_ASSIGN(echo256)
        SPH_FUNCTION_ASSIGN(echo384)
        SPH_FUNCTION_ASSIGN(echo512)
        SPH_FUNCTION_ASSIGN(fugue224)
        SPH_FUNCTION_ASSIGN(fugue256)
        SPH_FUNCTION_ASSIGN(fugue384)
        SPH_FUNCTION_ASSIGN(fugue512)
        SPH_FUNCTION_ASSIGN(groestl224)
        SPH_FUNCTION_ASSIGN(groestl256)
        SPH_FUNCTION_ASSIGN(groestl384)
        SPH_FUNCTION_ASSIGN(groestl512)
        SPH_FUNCTION_ASSIGN(hamsi224)
        SPH_FUNCTION_ASSIGN(hamsi256)
        SPH_FUNCTION_ASSIGN(hamsi384)
        SPH_FUNCTION_ASSIGN(hamsi512)
        SPH_FUNCTION_ASSIGN(haval128_3)
        SPH_FUNCTION_ASSIGN(haval128_4)
        SPH_FUNCTION_ASSIGN(haval128_5)
        SPH_FUNCTION_ASSIGN(haval160_3)
        SPH_FUNCTION_ASSIGN(haval160_4)
        SPH_FUNCTION_ASSIGN(haval160_5)
        SPH_FUNCTION_ASSIGN(haval192_3)
        SPH_FUNCTION_ASSIGN(haval192_4)
        SPH_FUNCTION_ASSIGN(haval192_5)
        SPH_FUNCTION_ASSIGN(haval224_3)
        SPH_FUNCTION_ASSIGN(haval224_4)
        SPH_FUNCTION_ASSIGN(haval224_5)
        SPH_FUNCTION_ASSIGN(haval256_3)
        SPH_FUNCTION_ASSIGN(haval256_4)
        SPH_FUNCTION_ASSIGN(haval256_5)
        SPH_FUNCTION_ASSIGN(jh224)
        SPH_FUNCTION_ASSIGN(jh256)
        SPH_FUNCTION_ASSIGN(jh384)
        SPH_FUNCTION_ASSIGN(jh512)
        SPH_FUNCTION_ASSIGN(keccak224)
        SPH_FUNCTION_ASSIGN(keccak256)
        SPH_FUNCTION_ASSIGN(keccak384)
        SPH_FUNCTION_ASSIGN(keccak512)
        SPH_FUNCTION_ASSIGN(luffa224)
        SPH_FUNCTION_ASSIGN(luffa256)
        SPH_FUNCTION_ASSIGN(luffa384)
        SPH_FUNCTION_ASSIGN(luffa512)
        SPH_FUNCTION_ASSIGN(md2)
        SPH_FUNCTION_ASSIGN(md4)
        SPH_FUNCTION_ASSIGN(md5)
        SPH_FUNCTION_ASSIGN(panama)
        SPH_FUNCTION_ASSIGN(radiogatun32)
        SPH_FUNCTION_ASSIGN(radiogatun64)
        SPH_FUNCTION_ASSIGN(ripemd)
        SPH_FUNCTION_ASSIGN(ripemd128)
        SPH_FUNCTION_ASSIGN(ripemd160)
        SPH_FUNCTION_ASSIGN(sha0)
        SPH_FUNCTION_ASSIGN(sha1)
        SPH_FUNCTION_ASSIGN(sha224)
        SPH_FUNCTION_ASSIGN(sha256)
        SPH_FUNCTION_ASSIGN(sha384)
        SPH_FUNCTION_ASSIGN(sha512)
        SPH_FUNCTION_ASSIGN(shabal192)
        SPH_FUNCTION_ASSIGN(shabal224)
        SPH_FUNCTION_ASSIGN(shabal256)
        SPH_FUNCTION_ASSIGN(shabal384)
        SPH_FUNCTION_ASSIGN(shabal512)
        SPH_FUNCTION_ASSIGN(shavite224)
        SPH_FUNCTION_ASSIGN(shavite256)
        SPH_FUNCTION_ASSIGN(shavite384)
        SPH_FUNCTION_ASSIGN(shavite512)
        SPH_FUNCTION_ASSIGN(simd224)
        SPH_FUNCTION_ASSIGN(simd256)
        SPH_FUNCTION_ASSIGN(simd384)
        SPH_FUNCTION_ASSIGN(simd512)
        SPH_FUNCTION_ASSIGN(skein224)
        SPH_FUNCTION_ASSIGN(skein256)
        SPH_FUNCTION_ASSIGN(skein384)
        SPH_FUNCTION_ASSIGN(skein512)
        SPH_FUNCTION_ASSIGN(tiger)
        SPH_FUNCTION_ASSIGN(tiger2)
        SPH_FUNCTION_ASSIGN(whirlpool)
        SPH_FUNCTION_ASSIGN(whirlpool0)
        SPH_FUNCTION_ASSIGN(whirlpool1)
        return -1;
        
        if(!ctx->sph_function_init) return -1;
        ctx->sph_function_init(ctx->cc);
    }
    
    if(data && (len > 0)) {
        if(!ctx->sph_function) return -1;
        ctx->sph_function(ctx->cc, data, len);
    }
    if(digest) {
        if(!ctx->sph_function_close) return -1;
        ctx->sph_function_close(ctx->cc, digest);
        return ctx->sph_function_size / 8;
    }    
    return 0;
}

