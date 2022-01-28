/*
    Copyright 2009-2021 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

// QuickBMS perform* encryption and compression operations



#define string_to_execute_VAR           (1 << 0)
#define string_to_execute_INPUT         (1 << 1)
#define string_to_execute_INPUT_SIZE    (1 << 2)
#define string_to_execute_OUTPUT        (1 << 3)
#define string_to_execute_OUTPUT_SIZE   (1 << 4)
#define string_to_execute_REDIRECT      (1 << 5)
#define string_to_execute_MULTI         (1 << 6)

u8 *string_to_execute(u8 *str, u8 *INPUT, u8 *INPUT_SIZE, u8 *OUTPUT, u8 *OUTPUT_SIZE, int *res, int exec_checks, int will_be_reparsed_by_bms) {
#define string_to_execute_realloc(X) \
            tmp = X; \
            if((exesz + tmp) >= totsz) { \
                totsz = exesz + tmp + 1024; \
                exe = realloc(exe, totsz + 1); \
                if(!exe) STD_ERR(QUICKBMS_ERROR_MEMORY); \
            }
    int     tmp,
            idx,
            exesz   = 0,
            totsz   = 0;
    u8      *VAR,
            *exe,
            *p,
            *l,
            *limit;

    if(!str) return NULL;
    // do NOT modify the content of str
    if(res) *res = 0;
    totsz = strlen(str) + 1024;
    exe = malloc(totsz + 1);
    if(!exe) STD_ERR(QUICKBMS_ERROR_MEMORY);
    exe[0] = 0;
    limit = str + strlen(str);

    for(p = str; *p && (p < limit);) {
        if((p[0] == '>') || (p[0] == '<')) {
            if(res) *res |= string_to_execute_REDIRECT;
        }
        if((p[0] == ';') || (p[0] == '&') || (p[0] == '|')) {
            if(res) *res |= string_to_execute_MULTI;
        }

#define string_to_execute_assign(X,Y) \
    if(X) { \
        if(exec_checks && mystrchrs(X, "\"\'><;&|")) { \
        } else { \
            string_to_execute_realloc(1 + strlen(X) + 1) \
            if(will_be_reparsed_by_bms || need_quote_delimiters(X)) { \
                exesz += sprintf(exe + exesz, "\"%s\"", X); \
            } else { \
                exesz += sprintf(exe + exesz,   "%s",   X); \
            } \
        } \
    } \
    p += Y; \
    if(res) *res |= string_to_execute_##X;

        if(p[0] == '%') {
            p++;
            if(p[1] == '%') continue;
            l = strchr(p, '%');
            if(!l) continue;
            idx = get_var_from_name(p, l - p);
            if(idx < 0) continue;
            VAR = get_var(idx);
            if(!VAR) continue;
            string_to_execute_assign(VAR, (l - p) + 1)

        } else if(!strnicmp(p, "#INPUT#", 7)) {
            string_to_execute_assign(INPUT, 7)

        } else if(!strnicmp(p, "#INPUT_SIZE#", 12)) {
            string_to_execute_assign(INPUT_SIZE, 12)

        } else if(!strnicmp(p, "#OUTPUT#", 8)) {
            string_to_execute_assign(OUTPUT, 8)

        } else if(!strnicmp(p, "#OUTPUT_SIZE#", 13)) {
            string_to_execute_assign(OUTPUT_SIZE, 13)

        } else {
            string_to_execute_realloc(1)
            exe[exesz++] = *p;
            p++;
        }
    }
    exe = realloc(exe, exesz + 1);
    if(!exe) STD_ERR(QUICKBMS_ERROR_MEMORY);
    exe[exesz] = 0;
    return(exe);
}



u8 *quickbms_execute_pipe_path(u8 *mycmd) {
    int     len,
            tmp,
            quote = 0,
            new_exelen;
    u8      *old_path,
            *old_exe,
            *new_exe,
            *p,
            *l;

    for(p = mycmd; *p && (*p <= ' '); p++);
    l = NULL;
         if(*p == '\"') l = strchr(++p, '\"');
    else if(*p == '\'') l = strchr(++p, '\'');
    if(!l) for(l = p; *l && (*l > ' '); l++);
    tmp = *l;
    *l = 0;
    old_path = mystrdup_simple(p);
    len = strlen(old_path);
#ifdef WIN32
    if((len < 4) || stricmp(old_path + len - 4, ".exe")) {
        old_path = realloc(old_path, len + 4 + 1);
        if(!old_path) STD_ERR(QUICKBMS_ERROR_MEMORY);
        strcpy(old_path + len, ".exe");
    }
#endif
    old_exe = get_filename(old_path);
    *l = tmp;
    if(*l) l++;

    if((old_path[0] == '/') || strchr(old_path, ':')) {
        // do nothing, it's an absolute path
    } else {
        new_exe = quickbms_path_open(old_exe);
        if(new_exe) {
            if(need_quote_delimiters(new_exe)) quote = 1;
            tmp = l - mycmd;
            len = strlen(l);
            new_exelen = strlen(new_exe);
            mycmd = realloc(mycmd, quote + new_exelen + quote + 1 + len + 1);
            if(!mycmd) STD_ERR(QUICKBMS_ERROR_MEMORY);
            mymemmove(mycmd + quote + new_exelen + quote + 1, mycmd + tmp, len + 1);
            p = mycmd;
            if(quote) *p++ = '\"';
            memcpy(p, new_exe, new_exelen);
            p += new_exelen;
            if(quote) *p++ = '\"';
            *p++ = ' ';
            FREE(new_exe)
        } else {
            // return the original command, it will be found automatically by system()
        }
    }
    FREE(old_path)
    return(mycmd);
}



// it's necessary to use real files because
// the executed command may not support sequential data (like pipes)
// so I MUST guarantee the 100% of compatibility even if it's slower
// and occupy more resources
int quickbms_execute_pipe(u8 *cmdstr, u8 *in, int insz, u8 **out, int outsz, u8 *my_fname) {
    FILE    *fd;
    int     res,
            t,
            size    = -1;
    u8      *in_fname   = NULL,
            *out_fname  = NULL,
            *mycmd      = NULL;

    if(!cmdstr) return -1;
    /* I had to comment this code because "log NAME 0 0" with "encryption execute" doesn't work, which may be good or bad depending by the script...
    if(!in) {
        if(!my_fname) return -1;
    }
    */
    if(insz < 0) return -1;

    if(!enable_execute_pipe) {
        fprintf(stderr, "\n"
            "- the script has requested to run an executable:\n"
            "  %s\n"
            "\n"
            "  NOTE THAT I ASK THIS CONFIRMATION ONLY NOW SO CHECK THE SCRIPT BECAUSE YOU\n"
            "  WILL BE NO LONGER PROMPTED TO CONFIRM THE NEXT USAGE OF THE EXECUTE COMMAND!\n"
            "  THIS FEATURE IS DANGEROUS SO BE SURE TO KNOW WHAT YOU ARE DOING\n"
            "\n"
            "  do you want to continue (y/N)? ",
            cmdstr);
        if(get_yesno(NULL) != 'y') myexit(QUICKBMS_ERROR_USER);

        // *fixed, added warning*
        // do NOT enable enable_execute_pipe because if we have
        // multiple comtype then only the first one will be
        // visualized and the others may be malicious!
        enable_execute_pipe = 1;
    }

    if(!my_fname) {
        quickbms_tmpname(&in_fname,  NULL, "tmp");
        mydump(in_fname, in, insz);
        quickbms_tmpname(&out_fname, NULL, "tmp");
    }

    // I'm forced to use system() because it's possible that
    // we have a command which uses stdout and so I must grant
    // it's compatibility... yes I know that it's a big security
    // problem!

    mycmd = string_to_execute(cmdstr,
        my_fname ? my_fname : in_fname,
        NULL,   // ???
        my_fname ? my_fname : out_fname,
        NULL,   // ???
        &res, 1,
        0);
    mycmd = quickbms_execute_pipe_path(mycmd);
    invalid_chars_to_spaces(mycmd);
#ifdef WIN32
    t = strlen(mycmd);
    mycmd = realloc(mycmd, 1 + t + 1);
    if(!mycmd) STD_ERR(QUICKBMS_ERROR_MEMORY);
    mymemmove(mycmd + 1, mycmd, t + 1);
    mycmd[0] = '@'; // crazy but necessary!
#endif
    fprintf(stderr, "- execute:\n  %s\n", mycmd);
    system(mycmd);  // do NOT check the return value
    FREE(mycmd);

    fd = NULL;
    if(res & string_to_execute_OUTPUT) {
        if(out_fname) {
            fd = xfopen(out_fname, "rb");   // check if it exists
            if(!fd) goto quit;
        }
    } else if(res & string_to_execute_INPUT) {
        if(in_fname) {
            fd = xfopen(in_fname, "rb");    // check if it exists
            if(!fd) goto quit;
        }
    }
    if(fd) {
        fseek(fd, 0, SEEK_END);
        size = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        if(out) {
            if(outsz < size) {
                outsz = size;
                MAX_ALLOC_CHECK(size);
                *out = realloc(*out, size + 1);
                if(!*out) STD_ERR(QUICKBMS_ERROR_MEMORY);
                (*out)[size] = 0;
            }
            size = fread(*out, 1, size, fd);
        } else {
            if(insz < size) size = insz;
            size = fread(in, 1, size, fd);
        }
        FCLOSE(fd);
    }

quit:
    if(!my_fname) {
        /*if(!g_keep_temporary_file)*/ unlink(in_fname);
        FREE(in_fname)
        /*if(!g_keep_temporary_file)*/ unlink(out_fname);
        FREE(out_fname)
    }
    return size;
}



int parse_bms(FILE *fds, u8 *inputs, int cmd, int eol_mode);
int CMD_CallDLL_func(int cmd, u8 *input, int input_size, u8 *output, int output_size);



int quickbms_calldll_pipe(u8 *cmdstr, u8 *in, int insz, u8 *out, int outsz) {
    int     cmd,
            ret;
    u8      *calldll_cmd = NULL,
            *p;

    if(!cmdstr) return -1;
    cmdstr = skip_delimit(cmdstr);

    p = mystrchrs(cmdstr, " \t");
    if(!p) return -1;
    *p = 0;
    if(!stricmp(cmdstr, "calldll")) cmdstr = p + 1;
    *p = ' ';   // restore the NULLed char

    p = malloc(32 + strlen(cmdstr) + 1);
    if(!p) STD_ERR(QUICKBMS_ERROR_MEMORY);
    sprintf(p, "calldll %s", cmdstr);

    // #INPUT# -> "#INPUT#" to avoid the parse_bms comments
         if(in  && out)  calldll_cmd = string_to_execute(p, "#INPUT#", "#INPUT_SIZE#",  "#OUTPUT#","#OUTPUT_SIZE#", &ret, 0, 1);
    else if(!in && out)  calldll_cmd = string_to_execute(p, "#OUTPUT#","#OUTPUT_SIZE#", "#OUTPUT#","#OUTPUT_SIZE#", &ret, 0, 1);
    else if(in  && !out) calldll_cmd = string_to_execute(p, "#INPUT#", "#INPUT_SIZE#",  "#INPUT#", "#INPUT_SIZE#",  &ret, 0, 1);
    else                 calldll_cmd = mystrdup_simple(p);
    FREE(p);

    for(cmd = 0; CMD.type != CMD_NONE; cmd++);
    cmd++;

    //if((cmd + 1) < MAX_CMDS) // not needed because parse_bms already does this check

    parse_bms(NULL, calldll_cmd, cmd, 0);
    ret = CMD_CallDLL_func(cmd, in, insz, out, outsz);
    if(ret & string_to_execute_INPUT) {
        if(!(ret & string_to_execute_OUTPUT)) {
            if(out) {
                if(outsz > insz) outsz = insz;
                memcpy(out, in, outsz);
            }
        }
    }

    FREE(calldll_cmd);
    return(outsz);
}



ICE_KEY *do_ice_key(u8 *key, int keysz, int icecrypt) {
    ICE_KEY *ik;
    int     i       = 0,
            k,
            level   = 0;

    if(keysz == 8) {
        level = 0;
    } else if(!(keysz % 16)) {
        level = keysz / 16;
    } else {
        fprintf(stderr, "\nError: your ICE key has an incorrect size\n");
        myexit(QUICKBMS_ERROR_ENCRYPTION);
    }

    if(icecrypt) {
        u8      *buf = alloca(keysz + 1);
        memset(buf, 0, keysz + 1);
        for(k = 0; k < keysz; k++) {
            u8      c = key[k] & 0x7f;
            int     idx = i / 8;
            int     bit = i & 7;

            if (bit == 0) {
                buf[idx] = (c << 1);
            } else if (bit == 1) {
                buf[idx] |= c;
            } else {
                buf[idx] |= (c >> (bit - 1));
                buf[idx + 1] = (c << (9 - bit));
            }
            i += 7;
        }
        key = buf;
    }
    ik = ice_key_create(level);
    if(!ik) return NULL;
    ice_key_set(ik, key);
    return(ik);
}



#ifndef DISABLE_MCRYPT
MCRYPT quick_mcrypt_check(u8 *type) {
    u8      tmp[64],
            *p,
            *mode,
            *algo;

    if(!type) type = "";
    mystrcpy(tmp, type, sizeof(tmp));

    // myisalnum gets also the '-' which is a perfect thing
    // NEVER use '-' as delimiter because "rijndael-*" use it

    algo = tmp;
    if(!strnicmp(tmp, "mcrypt", 6)) {
        for(algo = tmp + 6; *algo; algo++) {
            if(myisalnum(*algo)) break;
        }
    }
    p = mystrchrs(algo, "_, ");
    if(p) {
        *p = 0;
        mode = p + 1;
    } else {
        mode = MCRYPT_ECB;
    }
    return(mcrypt_module_open(algo, NULL, mode, NULL));
}
#endif



#ifndef DISABLE_TOMCRYPT
// nonce:001122334455667788 header:aabbccddeeff0011 ivec:FFff00112233AAbb tweak:0011223344
void tomcrypt_lame_ivec(TOMCRYPT *ctx, u8 *ivec, int ivecsz) {
    int     t,
            *y;
    u8      *p,
            *s,
            *l,
            **x,
            *limit;

    if(!ctx || !ivec || (ivecsz < 0)) return;
    limit = ivec + ivecsz;  // ivec is NULL delimited

    for(p = ivec; p < limit; p = l + 1) {
        while(*p && (*p <= ' ')) p++;
        l = strchr(p, ' ');
        if(!l) l = strchr(p, '\t');
        if(!l) l = limit;

        // ':' in all of the following
        if((s = stristr(p, "nonce:")) || (s = stristr(p, "salt:")) ||
           (s = stristr(p, "adata:")) || (s = stristr(p, "skey:")) ||
           (s = stristr(p, "key2:"))  || (s = stristr(p, /*salt_*/"key:"))) {
            x = &ctx->nonce;
            y = &ctx->noncelen;
        } else if((s = stristr(p, "header:"))) {
            x = &ctx->header;
            y = &ctx->headerlen;
        } else if((s = stristr(p, "ivec:"))) {
            x = &ctx->ivec;
            y = &ctx->ivecsz;
        } else if((s = stristr(p, "tweak:"))) {
            x = &ctx->tweak;
            y = NULL;
        } else {
            break;
        }

        s = strchr(s, ':');
        if(!s) break;
        s++;
        if(l < s) break;
        *x = calloc((l - s) + 1, 1);   // / 2, but it's ok (+1 is not needed)
        if(!*x) STD_ERR(QUICKBMS_ERROR_MEMORY);
        if(y) *y = 0;

        t = unhex(s, l - s, *x, l - s);
        if(t < 0) {
            FREE(*x)
        }
        if((t >= 0) && y) *y = t;
    }

    if(!ctx->ivec) {
        ctx->ivec = malloc(ivecsz);
        if(!ctx->ivec) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memcpy(ctx->ivec, ivec, ivecsz);
        ctx->ivecsz = ivecsz;
    }
}



// badly implemented because it's intended only as a test
TOMCRYPT *tomcrypt_doit(TOMCRYPT *ctx, u8 *type, u8 *in, int insz, u8 *out, int outsz, i32 *ret) {
    #define tomcrypt_doit_error { error_line = __LINE__; goto quit; }
    static int      init = 0;
    symmetric_ECB   ecb;
    symmetric_CFB   cfb;
    symmetric_OFB   ofb;
    symmetric_CBC   cbc;
    symmetric_CTR   ctr;
    symmetric_LRW   lrw;
    symmetric_F8    f8;
    symmetric_xts   xts;
    long    tmp;
    i32     stat;
    int     i,
            use_tomcrypt    = 0,
            error_line  = 0,
            error   = 0;
    u8      tag[64] = "",
            desc[64],
            *p,
            *l;

    int try_again_counter = 0;

    ltc_mp = ltm_desc;

    enum {
        tomcrypt_enum_ecb,
        tomcrypt_enum_cfb,
        tomcrypt_enum_ofb,
        tomcrypt_enum_cbc,
        tomcrypt_enum_ctr,
        tomcrypt_enum_lrw,
        tomcrypt_enum_f8,
        tomcrypt_enum_xts,

        tomcrypt_enum_hmac,
        tomcrypt_enum_omac,
        tomcrypt_enum_pmac,
        tomcrypt_enum_eax,
        tomcrypt_enum_ocb3,
        tomcrypt_enum_ocb,
        tomcrypt_enum_ccm,
        tomcrypt_enum_gcm,
        tomcrypt_enum_pelican,
        tomcrypt_enum_xcbc,
        tomcrypt_enum_f9,
        tomcrypt_enum_poly1305,
        tomcrypt_enum_chacha20poly1305,
        tomcrypt_enum_blake2smac,
        tomcrypt_enum_blake2bmac,

        tomcrypt_enum_invalid
    };

    typedef struct {
        int     idx;
        u8      *name;
        u8      *type;
    } quickbms_tomcrypt_t;
    static int                  quickbms_tomcrypts = 0;
    static quickbms_tomcrypt_t  quickbms_tomcrypt[128];

    if(!init) {
        register_all_ciphers();
        register_all_hashes();
        register_all_prngs();
    }

    #define TOMCRYPT_REGISTER(Z, X,Y) \
        if(!init) { \
            register_##Z(&X##_desc); \
            quickbms_tomcrypt[quickbms_tomcrypts].idx  = find_##Z(Y ? Y : #X); \
            quickbms_tomcrypt[quickbms_tomcrypts].name = #X; \
            quickbms_tomcrypt[quickbms_tomcrypts].type = #Z; \
            quickbms_tomcrypts++; \
            if(Y) { \
                quickbms_tomcrypt[quickbms_tomcrypts].idx  = quickbms_tomcrypt[quickbms_tomcrypts - 1].idx; \
                quickbms_tomcrypt[quickbms_tomcrypts].name = Y; \
                quickbms_tomcrypt[quickbms_tomcrypts].type = #Z; \
                quickbms_tomcrypts++; \
            } \
        }

    TOMCRYPT_REGISTER(cipher, blowfish, NULL)
    TOMCRYPT_REGISTER(cipher, rc5, NULL)
    TOMCRYPT_REGISTER(cipher, rc6, NULL)
    TOMCRYPT_REGISTER(cipher, rc2, NULL)
    TOMCRYPT_REGISTER(cipher, saferp, NULL)
    TOMCRYPT_REGISTER(cipher, safer_k64, "safer-k64")
    TOMCRYPT_REGISTER(cipher, safer_k128, "safer-k128")
    TOMCRYPT_REGISTER(cipher, safer_sk64, "safer-sk64")
    TOMCRYPT_REGISTER(cipher, safer_sk128, "safer-sk128")
    TOMCRYPT_REGISTER(cipher, rijndael, NULL)
    TOMCRYPT_REGISTER(cipher, aes, "rijndael")
    TOMCRYPT_REGISTER(cipher, xtea, NULL)
    TOMCRYPT_REGISTER(cipher, twofish, NULL)
    TOMCRYPT_REGISTER(cipher, des, NULL)
    TOMCRYPT_REGISTER(cipher, des3, "3des")
    TOMCRYPT_REGISTER(cipher, cast5, NULL)
    TOMCRYPT_REGISTER(cipher, noekeon, NULL)
    TOMCRYPT_REGISTER(cipher, skipjack, NULL)
    TOMCRYPT_REGISTER(cipher, khazad, NULL)
    TOMCRYPT_REGISTER(cipher, anubis, NULL)
    TOMCRYPT_REGISTER(cipher, kseed, "seed")
    TOMCRYPT_REGISTER(cipher, kasumi, NULL)
    TOMCRYPT_REGISTER(cipher, multi2, NULL)
    TOMCRYPT_REGISTER(cipher, camellia, NULL)

    TOMCRYPT_REGISTER(hash, chc, NULL)
    TOMCRYPT_REGISTER(hash, whirlpool, NULL)
    TOMCRYPT_REGISTER(hash, sha3_512, "sha3-512")
    TOMCRYPT_REGISTER(hash, sha3_384, "sha3-384")
    TOMCRYPT_REGISTER(hash, sha3_256, "sha3-256")
    TOMCRYPT_REGISTER(hash, sha3_224, "sha3-224")
    TOMCRYPT_REGISTER(hash, sha512, NULL)
    TOMCRYPT_REGISTER(hash, sha384, NULL)
    TOMCRYPT_REGISTER(hash, sha512_256, "sha512-256")
    TOMCRYPT_REGISTER(hash, sha512_224, "sha512-224")
    TOMCRYPT_REGISTER(hash, sha256, NULL)
    TOMCRYPT_REGISTER(hash, sha224, NULL)
    TOMCRYPT_REGISTER(hash, sha1, NULL)
    TOMCRYPT_REGISTER(hash, blake2s_256, "blake2s-256")
    TOMCRYPT_REGISTER(hash, blake2s_224, "blake2s-224")
    TOMCRYPT_REGISTER(hash, blake2s_160, "blake2s-160")
    TOMCRYPT_REGISTER(hash, blake2s_128, "blake2s-128")
    TOMCRYPT_REGISTER(hash, blake2b_512, "blake2b-512")
    TOMCRYPT_REGISTER(hash, blake2b_384, "blake2b-384")
    TOMCRYPT_REGISTER(hash, blake2b_256, "blake2b-256")
    TOMCRYPT_REGISTER(hash, blake2b_160, "blake2b-160")
    TOMCRYPT_REGISTER(hash, md5, NULL)
    TOMCRYPT_REGISTER(hash, md4, NULL)
    TOMCRYPT_REGISTER(hash, md2, NULL)
    TOMCRYPT_REGISTER(hash, tiger, NULL)
    TOMCRYPT_REGISTER(hash, rmd128, NULL)
    TOMCRYPT_REGISTER(hash, rmd160, NULL)
    TOMCRYPT_REGISTER(hash, rmd256, NULL)
    TOMCRYPT_REGISTER(hash, rmd320, NULL)

    TOMCRYPT_REGISTER(prng, yarrow, NULL)
    TOMCRYPT_REGISTER(prng, fortuna, NULL)
    TOMCRYPT_REGISTER(prng, rc4, NULL)
    TOMCRYPT_REGISTER(prng, chacha20_prng, NULL)
    TOMCRYPT_REGISTER(prng, sprng, NULL)
    TOMCRYPT_REGISTER(prng, sober128, NULL)

    if(!init) init = 1;

    if(type) {
        mystrcpy(desc, type, sizeof(desc));

        ctx = calloc(1, sizeof(TOMCRYPT));
        if(!ctx) STD_ERR(QUICKBMS_ERROR_MEMORY);
        ctx->idx = -1;  // 0 is AES

        #define TOMCRYPT_IDX(X,Y) \
            else if(!stricmp(p, #X)) { \
                ctx->idx = X##_idx; \
                Y; \
            }

        for(p = desc; *p; p = l + 1) {
            while(*p && (*p <= ' ')) p++;
            for(l = p; *l && *l > ' '; l++);
            if(*l) *l = 0;
            else   l = NULL;

            if(!strnicmp(p, "lib", 3)) p += 3;  // "libtomcrypt"
            if(!strnicmp(p, "tomcrypt", 8)) {
                use_tomcrypt = 1;
                p += 8;
            } else {
                for(i = 0; i < quickbms_tomcrypts; i++) {
                    if(!stricmp(p, quickbms_tomcrypt[i].name)) {
                        ctx->idx = quickbms_tomcrypt[i].idx;
                        if(!stricmp(quickbms_tomcrypt[i].type, "cipher")) {
                            ctx->cipher = tomcrypt_enum_ecb;
                        } else if(!stricmp(quickbms_tomcrypt[i].type, "hash")) {
                            ctx->hash   = TRUE;
                        } else if(!stricmp(quickbms_tomcrypt[i].type, "prng")) {
                            ctx->prng   = TRUE;
                        }
                        break;
                    }
                }

                if(ctx->idx < 0) ctx->idx = find_cipher(p);
                if(ctx->idx < 0) ctx->idx = find_hash(p);
                if(ctx->idx < 0) ctx->idx = find_prng(p);

                #define TOMCRYPT_REGISTER_ENUM(X) \
                    else if(stristr(p, #X)) ctx->cipher = tomcrypt_enum_##X;

                if(!stricmp(p, "")) {}  // needed because the others are "else"

                TOMCRYPT_REGISTER_ENUM(ecb)
                TOMCRYPT_REGISTER_ENUM(cfb)
                TOMCRYPT_REGISTER_ENUM(ofb)
                TOMCRYPT_REGISTER_ENUM(cbc)
                TOMCRYPT_REGISTER_ENUM(ctr)
                TOMCRYPT_REGISTER_ENUM(lrw)
                TOMCRYPT_REGISTER_ENUM(f8)
                TOMCRYPT_REGISTER_ENUM(xts)

                TOMCRYPT_REGISTER_ENUM(hmac)
                TOMCRYPT_REGISTER_ENUM(omac)
                TOMCRYPT_REGISTER_ENUM(pmac)
                TOMCRYPT_REGISTER_ENUM(eax)
                TOMCRYPT_REGISTER_ENUM(ocb3)
                TOMCRYPT_REGISTER_ENUM(ocb)
                TOMCRYPT_REGISTER_ENUM(ccm)
                TOMCRYPT_REGISTER_ENUM(gcm)
                TOMCRYPT_REGISTER_ENUM(pelican)
                TOMCRYPT_REGISTER_ENUM(xcbc)
                TOMCRYPT_REGISTER_ENUM(f9)
                TOMCRYPT_REGISTER_ENUM(poly1305)
                TOMCRYPT_REGISTER_ENUM(chacha20poly1305)
                TOMCRYPT_REGISTER_ENUM(blake2smac)
                TOMCRYPT_REGISTER_ENUM(blake2bmac)
            }

            if(!l) break;
            *l = ' ';
        }

        if(!use_tomcrypt || (ctx->idx < 0)) {
            FREE(ctx)
        }
        return(ctx);
    }

    if(!out) return ctx;

    //if(outsz > insz) outsz = insz;
    tmp = outsz;

    int blocklen = 0;
    int set_tomcrypt_blocklen(int size, int blocklen) {
        if(blocklen > 0) {
            size = (size / blocklen) * blocklen;
        }
        return size;
    }

    #define TOMCRYPT_CRYPT_MODE3(X) \
        X##_done(&X);
    #define TOMCRYPT_CRYPT_MODE2(X) \
        if(g_encrypt_mode) { if(error = X##_encrypt(in, out, insz, &X)) if(error = X##_encrypt(in, out, set_tomcrypt_blocklen(insz, blocklen), &X)) tomcrypt_doit_error } \
        else               { if(error = X##_decrypt(in, out, insz, &X)) if(error = X##_decrypt(in, out, set_tomcrypt_blocklen(insz, blocklen), &X)) tomcrypt_doit_error } \
        TOMCRYPT_CRYPT_MODE3(X)
    #define TOMCRYPT_CRYPT_MODE(X) \
        if(ctx->ivec && (ctx->ivecsz > 0)) { \
            if(error = X##_setiv(ctx->ivec, ctx->ivecsz, &X)) { \
                ctx->ivecsz = cipher_descriptor[ctx->idx].block_length; \
                if(error = X##_setiv(ctx->ivec, ctx->ivecsz, &X)) { \
                    tomcrypt_doit_error \
                } \
            } \
        } \
        TOMCRYPT_CRYPT_MODE2(X)

    if(ret) *ret = 0;
    if(ctx->idx < 0) ctx->idx = 0;

    i32 keysz;
    cipher_descriptor[ctx->idx].keysize(&keysz);

try_again:
    try_again_counter++;

    if(ctx->hash) {
        if(error = hash_memory(ctx->idx, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->prng) {
        // not supported because doesn't seem to exist an unique way to call the current one
        tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_ecb) {
        if(error = ecb_start(ctx->idx, ctx->key, ctx->keysz, 0, &ecb)) tomcrypt_doit_error
        blocklen = ecb.blocklen;
        TOMCRYPT_CRYPT_MODE2(ecb)

    } else if(ctx->cipher == tomcrypt_enum_cfb) {
        if(error = cfb_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, 0, &cfb)) tomcrypt_doit_error
        blocklen = cfb.blocklen;
        TOMCRYPT_CRYPT_MODE(cfb)

    } else if(ctx->cipher == tomcrypt_enum_ofb) {
        if(error = ofb_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, 0, &ofb)) tomcrypt_doit_error
        blocklen = ofb.blocklen;
        TOMCRYPT_CRYPT_MODE(ofb)

    } else if(ctx->cipher == tomcrypt_enum_cbc) {
        if(error = cbc_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, 0, &cbc)) tomcrypt_doit_error
        blocklen = cbc.blocklen;
        TOMCRYPT_CRYPT_MODE(cbc)

    } else if(ctx->cipher == tomcrypt_enum_ctr) {
        if(error = ctr_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, 0, CTR_COUNTER_LITTLE_ENDIAN, &ctr)) tomcrypt_doit_error
        blocklen = ctr.blocklen;
        TOMCRYPT_CRYPT_MODE(ctr)

    } else if(ctx->cipher == tomcrypt_enum_lrw) {
        if(error = lrw_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, ctx->tweak, 0, &lrw)) tomcrypt_doit_error
        //blocklen = lrw.blocklen;
        TOMCRYPT_CRYPT_MODE(lrw)

    } else if(ctx->cipher == tomcrypt_enum_f8) {
        if(error = f8_start(ctx->idx, ctx->ivec, ctx->key, ctx->keysz, ctx->nonce, ctx->noncelen, 0, &f8)) tomcrypt_doit_error
        blocklen = f8.blocklen;
        TOMCRYPT_CRYPT_MODE(f8)

    } else if(ctx->cipher == tomcrypt_enum_xts) {
        if(error = xts_start(ctx->idx, ctx->key, ctx->nonce, ctx->keysz, 0, &xts)) tomcrypt_doit_error
        //blocklen = xts.blocklen;
        if(g_encrypt_mode) { if(error = xts_encrypt(in, insz, out, ctx->tweak, &xts)) tomcrypt_doit_error }
        else               { if(error = xts_decrypt(in, insz, out, ctx->tweak, &xts)) tomcrypt_doit_error }
        TOMCRYPT_CRYPT_MODE3(xts)

    } else if(ctx->cipher == tomcrypt_enum_hmac) {
        if(error = hmac_memory(ctx->idx, ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_omac) {
        if(error = omac_memory(ctx->idx, ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_pmac) {
        if(error = pmac_memory(ctx->idx, ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_eax) {
        tmp = sizeof(tag);
        if(g_encrypt_mode) {
            if(error = eax_encrypt_authenticate_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce, ctx->noncelen,
                ctx->header, ctx->headerlen,
                in, insz,
                out,
                tag, &tmp)) tomcrypt_doit_error
        } else {
            if(error = eax_decrypt_verify_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce, ctx->noncelen,
                ctx->header, ctx->headerlen,
                in, insz,
                out,
                tag, tmp,
                &stat)) tomcrypt_doit_error
        }
        tmp = insz;

    } else if(ctx->cipher == tomcrypt_enum_ocb3) {
        tmp = sizeof(tag);
        if(g_encrypt_mode) {
            if(error = ocb3_encrypt_authenticate_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce, ctx->noncelen,
                ctx->header, ctx->headerlen,
                in, insz,
                out,
                tag, &tmp)) tomcrypt_doit_error
        } else {
            if(error = ocb3_decrypt_verify_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce, ctx->noncelen,
                ctx->header, ctx->headerlen,
                in, insz,
                out,
                tag, tmp,
                &stat)) tomcrypt_doit_error
        }
        tmp = insz;

    } else if(ctx->cipher == tomcrypt_enum_ocb) {
        tmp = sizeof(tag);
        if(g_encrypt_mode) {
            if(error = ocb_encrypt_authenticate_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce,
                in, insz,
                out,
                tag, &tmp)) tomcrypt_doit_error
        } else {
            if(error = ocb_decrypt_verify_memory(
                ctx->idx,
                ctx->key, ctx->keysz,
                ctx->nonce,
                in, insz,
                out,
                tag, tmp,
                &stat)) tomcrypt_doit_error
        }
        tmp = insz;

    } else if(ctx->cipher == tomcrypt_enum_ccm) {
        tmp = sizeof(tag);
        if(error = ccm_memory(
            ctx->idx,
            ctx->key, ctx->keysz,
            NULL, //uskey,
            ctx->nonce, ctx->noncelen,
            ctx->header, ctx->headerlen,
            in, insz,
            out,
            tag, &tmp,
            g_encrypt_mode ? CCM_ENCRYPT: CCM_DECRYPT)) tomcrypt_doit_error
        tmp = insz;

    } else if(ctx->cipher == tomcrypt_enum_gcm) {
        tmp = sizeof(tag);
        if(error = gcm_memory(
            ctx->idx,
            ctx->key, ctx->keysz,
            ctx->ivec, ctx->ivecsz,
            ctx->nonce, ctx->noncelen, //adata, adatalen,
            in, insz,
            out,
            tag, &tmp,
            g_encrypt_mode ? GCM_ENCRYPT: GCM_DECRYPT)) tomcrypt_doit_error
        tmp = insz;

    } else if(ctx->cipher == tomcrypt_enum_pelican) {
        if(error = pelican_memory(ctx->key, ctx->keysz, in, insz, out)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_xcbc) {
        if(error = xcbc_memory(ctx->idx, ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_f9) {
        if(error = f9_memory(ctx->idx, ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_blake2smac) {
        if(error = blake2smac_memory(ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_blake2bmac) {
        if(error = blake2bmac_memory(ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_poly1305) {
        if(error = poly1305_memory(ctx->key, ctx->keysz, in, insz, out, &tmp)) tomcrypt_doit_error

    } else if(ctx->cipher == tomcrypt_enum_chacha20poly1305) {
        if(error = chacha20poly1305_memory(ctx->key, ctx->keysz, ctx->ivec, ctx->ivecsz, ctx->nonce, ctx->noncelen, in, insz, out, tag, &tmp, g_encrypt_mode ? GCM_ENCRYPT: GCM_DECRYPT)) tomcrypt_doit_error
    }
    if(ret) *ret = tmp;
    return(ctx);
quit:
    if(try_again_counter == 1) {    // 0:ok, 1:retry, 2:giveup
        if(ctx->keysz != keysz) {
            ctx->keysz = keysz;
            goto try_again;
        }
    }
    fprintf(stderr, "Error: tomcrypt_doit error %d (%s:%d)\n", error, __FILE__, error_line);
    return NULL;
}
#endif



#ifndef DISABLE_SSL
// original Java code by Spirolone
// http://www.slightlymagic.net/forum/viewtopic.php?f=117&t=14839&start=135#p180107
int modpow_zed(unsigned char *content, int content_length) {
    #define modpow_zed_System_arraycopy(src, srcPos, dest, destPos, length) \
        memcpy(dest + destPos, src + srcPos, length);

    int     i;
    unsigned char result[256] = "";
    memset(result, 0, sizeof(result));
    if (content_length == 256)
        modpow_zed_System_arraycopy(content, 0, result, 0, content_length) //result = content;
    else if (content_length > 256)
        modpow_zed_System_arraycopy(content, 1, result, 0, 256)
    else if (content_length < 256)
        modpow_zed_System_arraycopy(content, 0, result, 256-content_length, content_length)
    unsigned char m[24] = "";
    for (i=0; i<20; i++)
        m[i] = result[235+i];
    m[20] = 0; m[21] = 0; m[22] = 0;
    unsigned char mask[240] = "";
    //MessageDigest md = MessageDigest.getInstance("SHA-1");
    for (i=0; i<12; i++){
        m[23] = (byte)i;
        SHA1(m, 24, mask + (i*20));
    }
    for (i=0; i<235; i++)
        result[i] ^= mask[i];
    int zeroes = 0;
    // Remove non-data bytes
    if (result[zeroes]==-0x80)
        zeroes++;
    while (result[zeroes]==0)
        zeroes++;
    int decoded_length = 214-zeroes;
    modpow_zed_System_arraycopy(result, zeroes+1, content, 0, decoded_length)
    return decoded_length;
}
#endif



void *DO_QUICKBMS_HASH(u8 *INPUT, int INPUT_SIZE) {
    static u8   OUTPUT[(256 * 2) + 1];
    int     OUTPUT_SIZE = INPUT_SIZE * 2;
    if(OUTPUT_SIZE >= sizeof(OUTPUT)) STD_ERR(QUICKBMS_ERROR_MEMORY);

    add_var(0, "QUICKBMS_HASH", INPUT, 0, INPUT_SIZE);
    byte2hex(INPUT, INPUT_SIZE, OUTPUT, sizeof(OUTPUT), 1);
    add_var(0, "QUICKBMS_HEXHASH", OUTPUT, 0, -1);
    mytolower(OUTPUT);
    add_var(0, "QUICKBMS_HEXHASHL", OUTPUT, 0, -1);
    return OUTPUT;
}



int do_quickbms_hmac(u8 *data, int datalen, u8 *digest) {
#ifndef DISABLE_SSL
    EVP_MD_CTX  *tmpctx = NULL;
    i32     tmp = 0;

    if(evpmd_ctx) {
        tmpctx = calloc(1, sizeof(EVP_MD_CTX));
        if(!tmpctx) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }
    if(hmac_ctx) {
        HMAC_Update(hmac_ctx, data, datalen);
        if(evpmd_ctx) EVP_MD_CTX_copy_ex(tmpctx, evpmd_ctx);
        HMAC_Final(hmac_ctx, digest, &tmp);
    } else if(evpmd_ctx) {
        EVP_DigestUpdate(evpmd_ctx, data, datalen);
        EVP_MD_CTX_copy_ex(tmpctx, evpmd_ctx);
        EVP_DigestFinal(evpmd_ctx, digest, &tmp);
    }
    if(evpmd_ctx) {
        FREE(evpmd_ctx);
        evpmd_ctx = tmpctx;
    }
    DO_QUICKBMS_HASH(digest, tmp);
#endif
    return 0;
}



#ifndef DISABLE_SSL
// Gladman
static void fcrypt_encr_data(unsigned char *data, int d_len, aes_ctr_ctx_t *cx)
{   int i = 0, pos = cx->num;

    while(i < d_len)
    {
        if(pos == AES_BLOCK_SIZE)
        {   int j = 0;
            /* increment encryption nonce   */
            while(j < 8 && !++cx->ivec[j])
                ++j;
            /* encrypt the nonce to form next xor buffer    */
            AES_encrypt(cx->ivec, cx->ecount, &cx->ctx);
            pos = 0;
        }

        data[i++] ^= cx->ecount[pos++];
    }

    cx->num = pos;
}
#endif



// if datalen is negative then it will return 0 if encryption is enabled or -1 if disabled
int perform_encryption(u8 *data, int datalen) {

#define ENCRYPT_BLOCKS(X,Y) { \
            tmp = datalen / X; \
            while(tmp--) { \
                Y; \
                data += X; \
            } \
        }

#ifndef EVP_MAX_MD_SIZE
#define EVP_MAX_MD_SIZE 64
#endif
    u8      digest[EVP_MAX_MD_SIZE];
    u_int   crc     = 0;
    int     i       = 0;
    i32     tmp     = 0,
            stat    = 0;

    // if(datalen <= 0) NEVER ENABLE THIS because it's needed
    // if(!data)        NEVER

    QUICK_CRYPT_CASE(wincrypt_ctx)
        if(!g_encrypt_mode) datalen = wincrypt_decrypt(wincrypt_ctx, data, datalen);
        else                datalen = wincrypt_encrypt(wincrypt_ctx, data, datalen);

#ifndef DISABLE_SSL
    } else QUICK_CRYPT_CASE(zip_aes_ctx)    // before evpmd_ctx, just in case of future updates
        if(!g_encrypt_mode) {
            do_quickbms_hmac(data, datalen, digest);
            fcrypt_encr_data(data, datalen, zip_aes_ctx);
        } else {
            fcrypt_encr_data(data, datalen, zip_aes_ctx);
            do_quickbms_hmac(data, datalen, digest);
        }

    } else QUICK_CRYPT_CASE(evp_ctx)
        if(g_reimport) {
            i = evp_ctx->encrypt;
            evp_ctx->encrypt = g_encrypt_mode;
        }
        tmp = datalen;  // failsafe
        EVP_CipherUpdate(evp_ctx, data, &tmp, data, datalen);
        if(g_reimport) evp_ctx->encrypt = i;
        if(evp_do_final) {
            datalen = tmp;
            tmp = 0;    // failsafe
            EVP_CipherFinal(evp_ctx, data + datalen, &tmp);
            datalen += tmp;
        }

    } else QUICK_CRYPT_CASE(evpmd_ctx)  // probably I seem crazy for all these operations... but it's perfect!
        do_quickbms_hmac(data, datalen, digest);

    } else QUICK_CRYPT_CASE(blowfish_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, BF_decrypt((void *)data, blowfish_ctx))
        else                ENCRYPT_BLOCKS(8, BF_encrypt((void *)data, blowfish_ctx))

    } else QUICK_CRYPT_CASE(aes_ctr_ctx)
        // don't exist 192 and 256
        switch(aes_ctr_ctx->type) {
            #if OPENSSL_VERSION_NUMBER < 0x10100000L
            case aes_ctr_ctx_ctr:       AES_ctr128_encrypt(data, data, datalen, &aes_ctr_ctx->ctx, aes_ctr_ctx->ivec, aes_ctr_ctx->ecount, &aes_ctr_ctx->num);  break;
            #endif
            case aes_ctr_ctx_ige:       AES_ige_encrypt(data, data, datalen, &aes_ctr_ctx->ctx, aes_ctr_ctx->ivec, g_encrypt_mode); break;
            case aes_ctr_ctx_bi_ige:    AES_bi_ige_encrypt(data, data, datalen, &aes_ctr_ctx->ctx, &aes_ctr_ctx->ctx, aes_ctr_ctx->ivec, g_encrypt_mode); break; // placeholder, it's wrong
            case aes_ctr_ctx_heat: {
                tmp = datalen / AES_BLOCK_SIZE;
                if(datalen % AES_BLOCK_SIZE) tmp++;
                while(tmp--) {
                    memcpy(digest, aes_ctr_ctx->ivec, AES_BLOCK_SIZE);
                    memset(aes_ctr_ctx->ecount, 0, AES_BLOCK_SIZE);
                    AES_cbc_encrypt(aes_ctr_ctx->ecount, aes_ctr_ctx->ecount, AES_BLOCK_SIZE, &aes_ctr_ctx->ctx, aes_ctr_ctx->ivec, g_encrypt_mode);
                    memcpy(aes_ctr_ctx->ivec, digest, AES_BLOCK_SIZE);
                    for(i = 0; i < AES_BLOCK_SIZE; i++) {
                        if(!tmp && (i >= (datalen % AES_BLOCK_SIZE))) break;
                        aes_ctr_ctx->ivec[i] ^= data[i];
                        data[i] ^= aes_ctr_ctx->ecount[i];
                    }
                    data += i;
                }
                break;
            }
            default: break;
        } 

    } else QUICK_CRYPT_CASE(modpow_ctx)
        u8      *bck_data = data,
                *p = data;
        i = datalen;
        while(i) {
            tmp = (i < 256) ? i : 256;

            BN_bin2bn(data, tmp, modpow_ctx->c);
            data += tmp;
            i    -= tmp;

            BN_mod_exp(modpow_ctx->r, modpow_ctx->c, modpow_ctx->e, modpow_ctx->n, modpow_ctx->bn_tmp);

            tmp = BN_bn2bin(modpow_ctx->r, p);
            if(modpow_ctx->zed) {
                tmp = modpow_zed(p, tmp);
            }
            p += tmp;
        }
        datalen = p - bck_data;

#endif
#ifndef DISABLE_MCRYPT
    } else QUICK_CRYPT_CASE(mcrypt_ctx)
        if(!g_encrypt_mode) mdecrypt_generic(mcrypt_ctx, data, datalen);
        else                mcrypt_generic(mcrypt_ctx, data, datalen);
#endif
#ifndef DISABLE_TOMCRYPT
    } else QUICK_CRYPT_CASE(tomcrypt_ctx)
        if(tomcrypt_ctx->hash) {
            tomcrypt_doit(tomcrypt_ctx, NULL, data, datalen, digest, EVP_MAX_MD_SIZE, &tmp);
            if(tmp >= 0) {
                DO_QUICKBMS_HASH(digest, tmp);
            }
        } else {
            tomcrypt_doit(tomcrypt_ctx, NULL, data, datalen, data, datalen, NULL);
        }
#endif

    } else QUICK_CRYPT_CASE(tea_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, tea_crypt(tea_ctx, TEA_DECRYPT, data, data))
        else                ENCRYPT_BLOCKS(8, tea_crypt(tea_ctx, TEA_ENCRYPT, data, data))

    } else QUICK_CRYPT_CASE(xtea_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, xtea_crypt_ecb(xtea_ctx, XTEA_DECRYPT, data, data))
        else                ENCRYPT_BLOCKS(8, xtea_crypt_ecb(xtea_ctx, XTEA_ENCRYPT, data, data))

    } else QUICK_CRYPT_CASE(xxtea_ctx)
        if(!g_encrypt_mode) xxtea_crypt(xxtea_ctx, XXTEA_DECRYPT, data, datalen);
        else                xxtea_crypt(xxtea_ctx, XXTEA_ENCRYPT, data, datalen);

    } else QUICK_CRYPT_CASE(swap_ctx)
        swap_crypt(swap_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(math_ctx)
        math_crypt(math_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(xmath_ctx)
        xmath_crypt(xmath_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(random_ctx)
        random_crypt(random_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(xor_ctx)
        xor_crypt(xor_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(rot_ctx)
        if(!g_encrypt_mode) rot_decrypt(rot_ctx, data, datalen);
        else              rot_encrypt(rot_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(rotate_ctx)
        rotate_crypt(rotate_ctx, data, datalen, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(reverse_ctx)
        reverse_crypt(reverse_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(inc_ctx)
        if(!g_encrypt_mode) inc_crypt(inc_ctx, data, datalen, 0);
        else                inc_crypt(inc_ctx, data, datalen, 1);

    } else QUICK_CRYPT_CASE(charset_ctx)
        if(!g_encrypt_mode) charset_decrypt(charset_ctx, data, datalen);
        else                charset_encrypt(charset_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(charset2_ctx)
        if(!g_encrypt_mode) charset_encrypt(charset2_ctx, data, datalen); // yes, it's encrypted first
        else                charset_decrypt(charset2_ctx, data, datalen); // and decrypted

    } else QUICK_CRYPT_CASE(twofish_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, do_twofish_decrypt(twofish_ctx, data, data))
        else                ENCRYPT_BLOCKS(16, do_twofish_encrypt(twofish_ctx, data, data))

    } else QUICK_CRYPT_CASE(seed_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, do_seed_decrypt(seed_ctx, data, data))
        else                ENCRYPT_BLOCKS(16, do_seed_encrypt(seed_ctx, data, data))

    } else QUICK_CRYPT_CASE(serpent_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, serpent_decrypt_internal(serpent_ctx, (void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(16, serpent_encrypt_internal(serpent_ctx, (void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(ice_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, ice_key_decrypt(ice_ctx, (void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(8, ice_key_encrypt(ice_ctx, (void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(rotor_ctx)
        if(!g_encrypt_mode) RTR_d_region(rotor_ctx, data, datalen, TRUE);
        else                RTR_e_region(rotor_ctx, data, datalen, TRUE);

    } else QUICK_CRYPT_CASE(ssc_ctx)
        if(!g_encrypt_mode) ssc_decrypt(ssc_ctx->key, ssc_ctx->keysz, data, datalen);
        else                ssc_encrypt(ssc_ctx->key, ssc_ctx->keysz, data, datalen);

    } else QUICK_CRYPT_CASE(cunprot_ctx)
        if(!g_encrypt_mode) datalen = cunprot_decrypt(cunprot_ctx, data, datalen);
        else                datalen = cunprot_encrypt(cunprot_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(zipcrypto_ctx) // the 12 bytes header must be removed by the user
        if(!g_encrypt_mode) zipcrypto_decrypt(zipcrypto_ctx, (void *)get_crc_table(), data, datalen);
        else                zipcrypto_encrypt(zipcrypto_ctx, (void *)get_crc_table(), data, datalen);
        if(zipcrypto_ctx[3]) {  // yeah this is valid only for the decryption
            if(datalen < 12) {
                datalen = 0;
            } else {
                datalen -= 12;
                mymemmove(data, data + 12, datalen);
            }
        }

    } else QUICK_CRYPT_CASE(threeway_ctx)
        if(!g_encrypt_mode) threeway_decrypt(threeway_ctx, data, datalen);
        else              threeway_encrypt(threeway_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(skipjack_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, skipjack_decrypt(skipjack_ctx, data, data))
        else                ENCRYPT_BLOCKS(8, skipjack_encrypt(skipjack_ctx, data, data))

    } else QUICK_CRYPT_CASE(anubis_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, ANUBISdecrypt(anubis_ctx, data, data))
        else                ENCRYPT_BLOCKS(16, ANUBISencrypt(anubis_ctx, data, data))

    } else QUICK_CRYPT_CASE(aria_ctx)
        ENCRYPT_BLOCKS(16, ARIA_Crypt(data, aria_ctx->Nr, aria_ctx->rk, data))

    } else QUICK_CRYPT_CASE(crypton_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, crypton_decrypt((void *)data, (void *)data, crypton_ctx))
        else                ENCRYPT_BLOCKS(16, crypton_encrypt((void *)data, (void *)data, crypton_ctx))

    } else QUICK_CRYPT_CASE(frog_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, frog_decrypt((void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(16, frog_encrypt((void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(gost_ctx)
        if(!gost_ctx->type) {
            if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, gostdecrypt((void *)data, (void *)data, gost_ctx->key))
            else                ENCRYPT_BLOCKS(8, gostcrypt((void *)data, (void *)data, gost_ctx->key))
        } else if(gost_ctx->type == 1) {
            gostofb((void *)data, (void *)data, datalen, gost_ctx->iv, gost_ctx->key);
        } else if(gost_ctx->type == 2) {
            if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, gostcfbdecrypt((void *)data, (void *)data, datalen, gost_ctx->iv, gost_ctx->key))
            else                ENCRYPT_BLOCKS(8, gostcfbencrypt((void *)data, (void *)data, datalen, gost_ctx->iv, gost_ctx->key))
        }

    } else QUICK_CRYPT_CASE(lucifer_ctx)
        ENCRYPT_BLOCKS(16, lucifer(data))

    } else if(kirk_ctx >= 0) {  // set_int3 here?
        if(datalen < 0) return 0;
        switch(kirk_ctx)
        {
            case 0:  kirk_CMD0(data, data, datalen, 0); break;
            case 1:  kirk_CMD1(data, data, datalen, 0); break;
            case 4:  kirk_CMD4(data, data, datalen);    break;
            case 7:  kirk_CMD7(data, data, datalen);    break;
            case 10: kirk_CMD10(data, datalen);         break;
            case 11: kirk_CMD11(data, data, datalen);   break;
            case 14: kirk_CMD14(data, datalen);         break;
            default: break;
        }

    } else QUICK_CRYPT_CASE(mars_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, mars_decrypt((void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(16, mars_encrypt((void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(misty1_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, misty1_decrypt_block(misty1_ctx, (void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(8, misty1_encrypt_block(misty1_ctx, (void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(noekeon_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, NOEKEONdecrypt(noekeon_ctx, (void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(16, NOEKEONencrypt(noekeon_ctx, (void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(seal_ctx)
        seal_encrypt(seal_ctx, (void *)data, datalen);

    } else QUICK_CRYPT_CASE(safer_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(8, Safer_Decrypt_Block((void *)data, (void *)safer_ctx, (void *)data))
        else                ENCRYPT_BLOCKS(8, Safer_Encrypt_Block((void *)data, (void *)safer_ctx, (void *)data))

    } else QUICK_CRYPT_CASE(pc1_128_ctx)
        pc1_128(pc1_128_ctx, data, datalen, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(pc1_256_ctx)
        pc1_256(pc1_256_ctx, data, datalen, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(crc_ctx)
        crc = crc_calc(crc_ctx, data, datalen, &tmp);   // tmp is the error
        if(tmp) {
            add_var(0, "QUICKBMS_CRC", NULL, 0, sizeof(u_int));
        } else {
            add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
        }

    } else QUICK_CRYPT_CASE(execute_ctx)
        quickbms_execute_pipe(execute_ctx, data, datalen, NULL, 0, NULL);

    } else QUICK_CRYPT_CASE(calldll_ctx)
        quickbms_calldll_pipe(calldll_ctx, data, datalen, NULL, 0);

    } else QUICK_CRYPT_CASE(sph_ctx)
        tmp = sph(sph_ctx, data, datalen, digest);

        DO_QUICKBMS_HASH(digest, tmp);

    } else QUICK_CRYPT_CASE(mpq_ctx)
        if(!g_encrypt_mode) DecryptMpqBlock(data, datalen, *mpq_ctx);
        else                EncryptMpqBlock(data, datalen, *mpq_ctx);

    } else QUICK_CRYPT_CASE(rc6_ctx)
        if(!g_encrypt_mode) ENCRYPT_BLOCKS(16, rc6_decrypt(rc6_ctx, (void *)data, (void *)data))
        else                ENCRYPT_BLOCKS(16, rc6_encrypt(rc6_ctx, (void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(xor_prev_next_ctx)
        xor_prev_next(xor_prev_next_ctx, data, datalen, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(rsa_ctx)
#ifndef DISABLE_TOMCRYPT
        static prng_state yarrow_prng;
#endif
        tmp = datalen;
        if(!g_encrypt_mode) {

        // X = public or private    Y = decrypt or encrypt
        #define QUICKBMS_OPENSSL_RSA(X, Y) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_PKCS1_PADDING) < 0) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_SSLV23_PADDING) < 0) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_NO_PADDING) < 0) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_PKCS1_OAEP_PADDING) < 0) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_X931_PADDING) < 0) \
            if(!rsa_ctx->openssl_rsa_##X  || RSA_##X##_##Y (datalen, data, data, rsa_ctx->openssl_rsa_##X,  RSA_PKCS1_PSS_PADDING) < 0) \

        // X = function name    ... = arguments
        #define QUICKBMS_TOMCRYPT_RSA(X, ...) \
            if(!rsa_ctx->is_tomcrypt || !(tmp = datalen) || (X(data, datalen, data, (void *)&tmp, NULL, 0, __VA_ARGS__, &rsa_ctx->tomcrypt_rsa) != CRYPT_OK))

#ifndef DISABLE_SSL
            QUICKBMS_OPENSSL_RSA(public,  decrypt)
            QUICKBMS_OPENSSL_RSA(private, decrypt)
#endif
#ifndef DISABLE_TOMCRYPT
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("md5"),     LTC_PKCS_1_V1_5, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("md5"),     LTC_PKCS_1_OAEP, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("md5"),     LTC_PKCS_1_PSS,  &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha1"),    LTC_PKCS_1_V1_5, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha1"),    LTC_PKCS_1_OAEP, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha1"),    LTC_PKCS_1_PSS,  &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha256"),  LTC_PKCS_1_V1_5, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha256"),  LTC_PKCS_1_OAEP, &stat)
            QUICKBMS_TOMCRYPT_RSA(rsa_decrypt_key_ex, find_hash("sha256"),  LTC_PKCS_1_PSS,  &stat)
#endif
                return -1;
        } else {
#ifndef DISABLE_SSL
            QUICKBMS_OPENSSL_RSA(public,  encrypt)
            QUICKBMS_OPENSSL_RSA(private, encrypt)
#endif
#ifndef DISABLE_TOMCRYPT
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("md5"),     LTC_PKCS_1_V1_5)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("md5"),     LTC_PKCS_1_OAEP)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("md5"),     LTC_PKCS_1_PSS)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha1"),    LTC_PKCS_1_V1_5)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha1"),    LTC_PKCS_1_OAEP)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha1"),    LTC_PKCS_1_PSS)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha256"),  LTC_PKCS_1_V1_5)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha256"),  LTC_PKCS_1_OAEP)
            QUICKBMS_TOMCRYPT_RSA(rsa_encrypt_key_ex, &yarrow_prng, find_prng("yarrow"), find_hash("sha256"),  LTC_PKCS_1_PSS)
#endif
                return -1;
        }

    } else QUICK_CRYPT_CASE(ecrypt_ctx)

        if(0) {
        #define QUICKBMS_ECRYPT_perform
        #include "encryption/ecrypt.h"
        #undef QUICKBMS_ECRYPT_perform
        }

    } else QUICK_CRYPT_CASE(isaac_ctx)
        ISAAC__cipher(isaac_ctx, data, data, datalen);

    } else QUICK_CRYPT_CASE(isaac_vernam_ctx)
        isaacx_crypt(NULL, -1, data, datalen, -1);

    } else QUICK_CRYPT_CASE(isaac_caesar_ctx)
        isaacx_crypt(NULL, -1, data, datalen, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(hsel_ctx)
        hsel_crypt(NULL, data, datalen, g_encrypt_mode, NULL);

    } else QUICK_CRYPT_CASE(rng_ctx)
        rng_crypt(data, datalen);

    } else QUICK_CRYPT_CASE(flip_ctx)
        flip_crypt(flip_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(bcrypt_ctx)
        datalen = bcrypt_encrypt(bcrypt_ctx, data, datalen, NULL, 0, g_encrypt_mode);

    } else QUICK_CRYPT_CASE(molebox_ctx)
        molebox_decrypt(molebox_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(replace_ctx)
        datalen = replace_crypt(replace_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(rc4_ctx)
        arc4_crypt(rc4_ctx, data, datalen);

    } else QUICK_CRYPT_CASE(d3des_ctx)
        ENCRYPT_BLOCKS(8, des((void *)data, (void *)data))

    } else QUICK_CRYPT_CASE(chacha_ctx)
        chacha20_encrypt(chacha_ctx, data, data, datalen);  // encrypt and decrypt are the same

    } else QUICK_CRYPT_CASE(spookyhash_ctx)
        u64     spookyhash1 = spookyhash_ctx->hash1,
                spookyhash2 = spookyhash_ctx->hash2;
        switch(spookyhash_ctx->size) {
            case 32:
                spookyhash1 = (u32)spookyhash_32(data, datalen, spookyhash1);
                memcpy(digest,               &spookyhash1, sizeof(u64));
                break;
            case 64:
                spookyhash1 =      spookyhash_64(data, datalen, spookyhash1);
                memcpy(digest,               &spookyhash1, sizeof(u64));
                break;
            default:
                                   spookyhash_128(data, datalen, &spookyhash1, &spookyhash2);
                dump128num(digest, spookyhash1, spookyhash2);
                break;
        }
        add_var(0, "QUICKBMS_CRC", NULL, spookyhash1, sizeof(u_int));   // useless for 128
        tmp = spookyhash_ctx->size / 8;
        DO_QUICKBMS_HASH(digest, tmp);

    } else QUICK_CRYPT_CASE(murmurhash_ctx)
        switch(murmurhash_ctx) {
            case -32:
                crc = qhashfnv1_32(data, datalen);
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
            case -64:
                crc = qhashfnv1_64(data, datalen);
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
            case 32:
                crc = qhashmurmur3_32(data, datalen);
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
            default:
                qhashmurmur3_128(data, datalen, digest);
                DO_QUICKBMS_HASH(digest, 16);
                break;
        }

    } else QUICK_CRYPT_CASE(xxhash_ctx)
        XXH128_hash_t    xxh128 = {0};
        switch(xxhash_ctx->size) {
            case -32:
            case 32:
                crc = XXH32(data, datalen, xxhash_ctx->hash1);
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
            case -64:
                crc = XXH3_64bits_withSecret(data, datalen, xxhash_ctx->key, xxhash_ctx->keysz);
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
            case -128:
                xxh128 = XXH3_128bits_withSecret(data, datalen, xxhash_ctx->key, xxhash_ctx->keysz);
                dump128num(digest, xxh128.low64, xxh128.high64);
                DO_QUICKBMS_HASH(digest, 16);
                break;
            case 128:
                if(xxhash_ctx->keysz > 0) {
                    xxh128 = XXH3_128bits_withSecret(data, datalen, xxhash_ctx->key, xxhash_ctx->keysz);
                } else {
                    xxh128 = XXH128(data, datalen, xxhash_ctx->hash1);
                }
                dump128num(digest, xxh128.low64, xxh128.high64);
                DO_QUICKBMS_HASH(digest, 16);
                break;
            default: //case 64:
                if(xxhash_ctx->keysz > 0) {
                    crc = XXH3_64bits_withSecret(data, datalen, xxhash_ctx->key, xxhash_ctx->keysz);
                } else {
                    crc = XXH64(data, datalen, xxhash_ctx->hash1);
                }
                add_var(0, "QUICKBMS_CRC", NULL, crc, sizeof(u_int));
                break;
        }

#ifndef DISABLE_TOMCRYPT
#ifdef LTC_PKCS_5
    } else QUICK_CRYPT_CASE(PBKDF_ctx)
        unsigned long   tlong = datalen;
        if(PBKDF_ctx->algo == 2) {
                tmp = pkcs_5_alg2        (PBKDF_ctx->key, PBKDF_ctx->keysz, PBKDF_ctx->ivec, PBKDF_ctx->ivecsz, PBKDF_ctx->iter, PBKDF_ctx->hash, data, &tlong);
        } else {
            if(PBKDF_ctx->openssl) {
                tmp = pkcs_5_alg1_openssl(PBKDF_ctx->key, PBKDF_ctx->keysz, PBKDF_ctx->ivec,                    PBKDF_ctx->iter, PBKDF_ctx->hash, data, &tlong);
            } else {
                tmp = pkcs_5_alg1        (PBKDF_ctx->key, PBKDF_ctx->keysz, PBKDF_ctx->ivec,                    PBKDF_ctx->iter, PBKDF_ctx->hash, data, &tlong);
            }
        }
        if(tmp != CRYPT_OK) return -1;
        datalen = tlong;
#endif
#endif

    } else {
        if(datalen < 0) return -1;
    }
    //return 0;  // don't return datalen because they are almost all block cipher encryptions and so it's all padded/aligned
    return(datalen);    // from version 0.3.11 I return datalen, only if I'm 100% sure that it's correct
}



int perform_compression(u8 *in, int zsize, u8 **ret_out, int size, int *outsize, u_int offset_info) {
    size_t  size_t_tmp;
    int     tmp1,
            tmp2,
            tmp3,
            tmp4,
            tmp5,
            tmp6,
            old_outsize,
            bck_size;
    size_t  t_size;
    i32     t32 = 0,
            tt32;
    u8      *out,
            *p,
            *l;

    int     tmp_outsize = size;
    if(!outsize) outsize = &tmp_outsize;

    old_outsize = *outsize;
    bck_size = size;

    out = *ret_out;
    if(g_ignore_comp_errors) memset(out, 0, size);  // necessary

    switch(g_compression_type) {
        case COMP_ERROR: size = -1; // QUICK_COMP_CASE sets the "break;"
        QUICK_COMP_CASE(ZLIB) size = unzip_zlib(in, zsize, out, size, 0);
        QUICK_COMP_CASE(DEFLATE) size = unzip_deflate(in, zsize, out, size, 0);
        QUICK_COMP_CASE(LZO1)  size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1A) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1B) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1C) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1F) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1X) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1Y) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1Z) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO2A) size = unlzo(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZSS) size = unlzss(in, zsize, out, size, g_comtype_dictionary);
        QUICK_COMP_CASE(LZSSX) size = unlzssx(in, zsize, out, size, g_comtype_dictionary);
        QUICK_COMP_CASE(LZSS0) size = unlzss(in, zsize, out, size, "12 4 2 2 0");
        QUICK_COMP_CASE(LZX) size = unlzx(in, zsize, out, size);
        QUICK_COMP_CASE(GZIP) t32 = *outsize; size = ungzip(in, zsize, &out, &t32, 0); *outsize = t32;
        QUICK_COMP_CASE(EXPLODE) size = unexplode(in, zsize, out, size);
        #define QUICK_COMP_CASE_LZMA_DOIT(X,FLAGS,Z) \
            t32 = *outsize; \
            size = unlzma##X(in, zsize, &out, size, FLAGS, &t32, Z); \
            *outsize = t32;
        QUICK_COMP_CASE(LZMA)            QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_NONE,0)
        QUICK_COMP_CASE(LZMA_86HEAD)     QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_86_HEADER,0) // contains the uncompressed size
        QUICK_COMP_CASE(LZMA_86DEC)      QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_86_DECODER,0)
        QUICK_COMP_CASE(LZMA_86DECHEAD)  QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER,0) // contains the uncompressed size
        QUICK_COMP_CASE(LZMA_EFS)        QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_EFS,0)
        QUICK_COMP_CASE(LZMA_0)          QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_PROP0 | LZMA_FLAGS_NONE,0)
        QUICK_COMP_CASE(LZMA_86HEAD0)    QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_HEADER,0)  // contains the uncompressed size
        QUICK_COMP_CASE(LZMA_86DEC0)     QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_DECODER,0)
        QUICK_COMP_CASE(LZMA_86DECHEAD0) QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER,0)  // contains the uncompressed size
        QUICK_COMP_CASE(LZMA_EFS0)       QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_PROP0 | LZMA_FLAGS_EFS,0)
        QUICK_COMP_CASE(BZIP2) size = unbzip2(in, zsize, out, size);
        QUICK_COMP_CASE(XMEMLZX) t32 = *outsize; size = unxmemlzx(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(HEX) size = unhex(in, zsize, out, size);
        QUICK_COMP_CASE(BASE64) size = unbase64(in, zsize, out, size);
        QUICK_COMP_CASE(UUENCODE) size = uudecode(in, zsize, out, size, 0);
        QUICK_COMP_CASE(XXENCODE) size = uudecode(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ASCII85) size = unascii85(in, zsize, out, size);
        QUICK_COMP_CASE(YENC) size = unyenc(in, zsize, out, size);
        QUICK_COMP_CASE(UNLZW) size = unlzw(out, size, in, zsize);
        QUICK_COMP_CASE(UNLZWX) size = unlzwx(out, size, in, zsize);
        //QUICK_COMP_CASE(CAB) size = unmspack_cab(in, zsize, out, size);
        //QUICK_COMP_CASE(CHM) size = unmspack_chm(in, zsize, out, size);
        //QUICK_COMP_CASE(SZDD) size = unmspack_szdd(in, zsize, out, size);
        QUICK_COMP_CASE(LZXCAB) size = unmspack(in, zsize, out, size, 21, 0, -1, 0);
        QUICK_COMP_CASE(LZXCHM) size = unmspack(in, zsize, out, size, 16, 2, -1, 0);
        QUICK_COMP_CASE(RLEW) size = unrlew(in, zsize, out, size);
        QUICK_COMP_CASE(LZJB) size = lzjb_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(SFL_BLOCK) size = expand_block(in, out, zsize, size);
        QUICK_COMP_CASE(SFL_RLE) size = expand_rle(in, out, zsize, size);
        QUICK_COMP_CASE(SFL_NULLS) size = expand_nulls(in, out, zsize, size);
        QUICK_COMP_CASE(SFL_BITS) size = expand_bits(in, out, zsize, size);
        QUICK_COMP_CASE(LZMA2)            QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_NONE,0)
        QUICK_COMP_CASE(LZMA2_86HEAD)     QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_86_HEADER,0) // contains the uncompressed size
        QUICK_COMP_CASE(LZMA2_86DEC)      QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_86_DECODER,0)
        QUICK_COMP_CASE(LZMA2_86DECHEAD)  QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER,0) // contains the uncompressed size
        QUICK_COMP_CASE(LZMA2_EFS)        QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_EFS,0)
        QUICK_COMP_CASE(LZMA2_0)          QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_PROP0 | LZMA_FLAGS_NONE,0)
        QUICK_COMP_CASE(LZMA2_86HEAD0)    QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_HEADER,0)    // contains the uncompressed size
        QUICK_COMP_CASE(LZMA2_86DEC0)     QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_DECODER,0)
        QUICK_COMP_CASE(LZMA2_86DECHEAD0) QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_PROP0 | LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER,0)    // contains the uncompressed size
        QUICK_COMP_CASE(LZMA2_EFS0)       QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_PROP0 | LZMA_FLAGS_EFS,0)
        QUICK_COMP_CASE(NRV2b) size = ucl_decompress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(NRV2d) size = ucl_decompress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(NRV2e) size = ucl_decompress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(HUFFBOH) size = huffboh_unpack_mem2mem(in, zsize, out, size);
        QUICK_COMP_CASE(UNCOMPRESS) size = uncompress_lzw(in, zsize, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(DMC) size = undmc(in, zsize, out, size);
        QUICK_COMP_CASE(LZH) size = unlzh(in, zsize, out, size);
        QUICK_COMP_CASE(LZARI) size = unlzari(in, zsize, out, size);
        QUICK_COMP_CASE(TONY) size = Screen__decompressTony(in, zsize, out, size);
        QUICK_COMP_CASE(RLE7) size = Screen__decompressRLE7(in, zsize, out, size);
        QUICK_COMP_CASE(RLE0) size = Screen__decompressRLE0(in, zsize, out, size);
        QUICK_COMP_CASE(RLE) size = /*rle_decode*/ unrle(out, in, zsize);
        QUICK_COMP_CASE(RLEA) size = another_rle(in, zsize, out, size);
        QUICK_COMP_CASE(BPE) size = bpe_expand(in, zsize, out, size);
        QUICK_COMP_CASE(QUICKLZ) size = unquicklz(in, zsize, out, size);
        QUICK_COMP_CASE(Q3HUFF) size = unq3huff(in, zsize, out, size);
        QUICK_COMP_CASE(UNMENG) size = unmeng(in, zsize, out, size);
        QUICK_COMP_CASE(LZ2K) unlz2k_init(); size = unlz2k(in, out, zsize, size);
        QUICK_COMP_CASE(DARKSECTOR) size = undarksector(in, zsize, out, size, 1);
        QUICK_COMP_CASE(MSZH) size = mszh_decomp(in, zsize, out, size);
        QUICK_COMP_CASE(UN49G) un49g_init(); size = un49g(out, in);
        QUICK_COMP_CASE(UNTHANDOR) size = unthandor(in, zsize, out, size);
        QUICK_COMP_CASE(DOOMHUFF) size = doomhuff(0, in, zsize, out, size, 0);
        QUICK_COMP_CASE(ZDAEMON) size = doomhuff(1, in, zsize, out, size, 0);
        QUICK_COMP_CASE(SKULLTAG) size = doomhuff(2, in, zsize, out, size, 0);
        QUICK_COMP_CASE(APLIB) size = aP_depack_safe(in, zsize, out, size);
        QUICK_COMP_CASE(TZAR_LZSS)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the tzar_lzss decompression requires the setting of the dictionary\n"
                    "       field in comtype with the name of the variable containing the type\n"
                    "       of tzar decompression (from 0xa1 to 0xc5), like:\n"
                    "         comtype tzar_lzss MYVAR\n");
                ////myexit(QUICKBMS_ERROR_BMS);
                //size = -1;
                //break;
                g_comtype_dictionary = "0xc5"; // for the comtype scanner
            }

            tzar_lzss_init();
            t32 = size;
            tzar_lzss(in, zsize, out, &t32,    // it's so horrible because the last argument is dynamic
                //get_var32(get_var_from_name(g_comtype_dictionary, g_comtype_dictionary_len))
                myatoi(g_comtype_dictionary)
            );
            size = t32;
        QUICK_COMP_CASE(LZF) size = lzf_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(CLZ77) size = CLZ77_Decode(out, size, in, zsize);
        QUICK_COMP_CASE(DHUFF) size = undhuff(in, zsize, out, size);
        QUICK_COMP_CASE(FIN) size = unfin(in, zsize, out, size);
        QUICK_COMP_CASE(LZAH) size = de_lzah(in, zsize, out, size);
        QUICK_COMP_CASE(LZH12) size = de_lzh(in, zsize, out, size, 12);
        QUICK_COMP_CASE(LZH13) size = de_lzh(in, zsize, out, size, 13);
#ifdef WIN32   // the library is not by default in linux and it's too big to attach in quickbms
        QUICK_COMP_CASE(GRZIP) size = GRZip_DecompressBlock(in, zsize, out);
#endif
        QUICK_COMP_CASE(CKRLE) size = CK_RLE_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(QUAD) size = unquad(in, zsize, out, size);
        QUICK_COMP_CASE(BALZ) size = unbalz(in, zsize, out, size);
        // it's a zlib with the adding of inflateBack9 which is not default
        QUICK_COMP_CASE(DEFLATE64) size = inflate64(in, zsize, out, size);
        QUICK_COMP_CASE(SHRINK) size = unshrink(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDI) size = ppmdi_decompress /*unppmdi*/ (in, zsize, out, size); // PKWARE specifics
        QUICK_COMP_CASE(MULTIBASE)
            size = multi_base_decoder(  // the usage of g_comtype_dictionary_len avoids wasting 2 vars
                g_comtype_dictionary_len & 0xff, (g_comtype_dictionary_len >> 8) & 0xff,
                in, zsize, out, size,
                g_comtype_dictionary);
        QUICK_COMP_CASE(BRIEFLZ) size = blz_depack_safe(in, zsize, out, size);
        QUICK_COMP_CASE(PAQ6)
            size = -1;
            /* wasting of memory and speed!!!

            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the PAQ6 decompression requires the setting of the dictionary\n"
                    "       field in comtype with the name of the variable containing the\n"
                    "       level of compression (from 0 to 9), like:\n"
                    "         comtype paq6 MYVAR\n"
                    "         comtype paq6 3 # default level\n");
                ////myexit(QUICKBMS_ERROR_BMS);
                //size = -1;
                //break;
                g_comtype_dictionary = "3"; // for the comtype scanner
            }
            size = unpaq6(in, zsize, out, size,
                //get_var32(get_var_from_name(g_comtype_dictionary, g_comtype_dictionary_len))
                myatoi(g_comtype_dictionary)
            );
            */
        QUICK_COMP_CASE(SHCODEC) size = sh_DecodeBlock(in, out, zsize);
        QUICK_COMP_CASE(HSTEST1) size = hstest_hs_unpack(out, in, zsize);
        QUICK_COMP_CASE(HSTEST2) size = hstest_unpackc(out, in, zsize);
        QUICK_COMP_CASE(SIXPACK) size = unsixpack(in, zsize, out, size);
        QUICK_COMP_CASE(ASHFORD) size = unashford(in, zsize, out, size);
        QUICK_COMP_CASE(JCALG)
#ifdef WIN32
            size = JCALG1_Decompress_Small(in, out);
#else
            size = DecompressJCALG1(out, in);
#endif
        QUICK_COMP_CASE(JAM) size = unjam(in, zsize, out, size);
        QUICK_COMP_CASE(LZHLIB) size = unlzhlib(in, zsize, out, size);
        QUICK_COMP_CASE(SRANK) size = unsrank(in, zsize, out, size);
        QUICK_COMP_CASE(ZZIP)
            if(size >= zsize) { // zzip is horrible to use in this way
                memcpy(out, in, zsize);
                size = ZzUncompressBlock(out);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(SCPACK) size = strexpand(out, in, zsize, size, g_comtype_dictionary, 1);
        QUICK_COMP_CASE(RLE3) size = rl3_decode(in, zsize, out, size, -1);
        QUICK_COMP_CASE(BPE2) size = unbpe2(in, zsize, out, size);
        QUICK_COMP_CASE(BCL_HUF) Huffman_Uncompress(in, out, zsize, size);
        QUICK_COMP_CASE(BCL_LZ) LZ_Uncompress(in, out, zsize);
        QUICK_COMP_CASE(BCL_RICE)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the BCL_RICE decompression requires the setting of the dictionary\n"
                    "       field in comtype with the name of the variable containing the type\n"
                    "       of compression (from 1 to 8, read rice.h), like:\n"
                    "         comtype bcl_rice 1\n");
                ////myexit(QUICKBMS_ERROR_BMS);
                //size = -1;
                //break;
                g_comtype_dictionary = "2"; // for the comtype scanner
            }
            Rice_Uncompress(in, out, zsize, size,
                //get_var32(get_var_from_name(g_comtype_dictionary, g_comtype_dictionary_len))
                myatoi(g_comtype_dictionary)
            );
        QUICK_COMP_CASE(BCL_RLE) size = RLE_Uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(BCL_SF) SF_Uncompress(in, out, zsize, size);
        QUICK_COMP_CASE(SCZ)
            t32 = size;
            p = NULL;
            if(Scz_Decompress_Buffer2Buffer(in, zsize, (void *)&p, &t32) && (t32 <= size)) {
                size = t32;
                memcpy(out, p, size);
                real_free(p);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(SZIP) t_size = size; SZ_BufftoBuffDecompress(out, &t_size, in, zsize, NULL); size = t_size;
        QUICK_COMP_CASE(PPMDI_RAW)
            if(!g_comtype_dictionary) {
                g_comtype_dictionary = "10 4 0"; // for the comtype scanner
                fprintf(stderr, "\n"
                    "Error: the PPMDi decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying SaSize, MaxOrder and Method, like:\n"
                    "         comtype ppmdi_raw \"%s\"\n", g_comtype_dictionary);
            }
            tmp1 = tmp2 = tmp3 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, NULL);
            size = ppmdi_decompress_raw /*unppmdi_raw*/ (in, zsize, out, size, tmp1, tmp2, tmp3);
        QUICK_COMP_CASE(PPMDG) size = unppmdg(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDG_RAW)
            if(!g_comtype_dictionary) {
                g_comtype_dictionary = "10 4"; // for the comtype scanner
                fprintf(stderr, "\n"
                    "Error: the PPMdG decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying SaSize and MaxOrder, like:\n"
                    "         comtype ppmdg_raw \"%s\"\n", g_comtype_dictionary);
            }
            tmp1 = tmp2 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, NULL);
            size = unppmdg_raw(in, zsize, out, size, tmp1, tmp2);
        QUICK_COMP_CASE(PPMDJ) size = unppmdj(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDJ_RAW)
            if(!g_comtype_dictionary) {
                g_comtype_dictionary = "10 4 0"; // for the comtype scanner
                fprintf(stderr, "\n"
                    "Error: the PPMdJ decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying SaSize, MaxOrder and CutOff, like:\n"
                    "         comtype ppmdj_raw \"%s\"\n", g_comtype_dictionary);
            }
            tmp1 = tmp2 = tmp3 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, NULL);
            size = unppmdj_raw(in, zsize, out, size, tmp1, tmp2, tmp3);
        QUICK_COMP_CASE(SR3C) size = unsr3c(in, zsize, out, size);
        QUICK_COMP_CASE(HUFFMANLIB)
            t32 = size;
            p = NULL;
            if(!huffman_decode_memory(in, zsize, &p, &t32) && (t32 <= size)) {
                size = t32;
                memcpy(out, p, size);
                real_free(p);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(SFASTPACKER)  size = SFUnpack(in, zsize, out, size, 0);
        QUICK_COMP_CASE(SFASTPACKER2) size = SFUnpack(in, zsize, out, size, 1); // smart mode only
        //QUICK_COMP_CASE(DK2) undk2_init(); size = undk2(out, in, 0);
        QUICK_COMP_CASE(LZ77WII) t32 = *outsize; size = unlz77wii(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(LZ77WII_RAW10) size = unlz77wii_raw10(in, zsize, out, size);
        QUICK_COMP_CASE(DARKSTONE) size = undarkstone(in, zsize, out, size);
        QUICK_COMP_CASE(SFL_BLOCK_CHUNKED) size = sfl_block_chunked(in, zsize, out, size);
        QUICK_COMP_CASE(YUKE_BPE) size = yuke_bpe(in, zsize, out, size, 1);
        QUICK_COMP_CASE(STALKER_LZA)
            stalker_lza_init();
            t32 = size;
            p = NULL;
            stalker_lza(in, zsize, &p, &t32);  // size is filled by the function
            size = t32;
            if(/*(tmp1 >= 0) &&*/ (size > 0)) {
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                FREE(p);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(PRS_8ING)  size = prs_8ing_uncomp(out, size, in, zsize);
        QUICK_COMP_CASE(PUYO_CNX)  size = puyo_cnx(in, zsize, out, size);  // _unpack is the old one
        QUICK_COMP_CASE(PUYO_CXLZ) size = puyo_cxlz_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(PUYO_LZ00) size = puyo_lz00(in, zsize, out, size); // _unpack is the old one
        QUICK_COMP_CASE(PUYO_LZ01) size = puyo_lz01(in, zsize, out, size); //._unpack is the old one
        QUICK_COMP_CASE(PUYO_LZSS) size = puyo_lzss_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(PUYO_ONZ)  size = puyo_onz_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(PUYO_PRS)  size = puyo_prs(in, zsize, out, size);  // _unpack is the old one
        //QUICK_COMP_CASE(PUYO_PVZ) size = puyo_pvz_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(FALCOM) size = falcom_DecodeData(out, size, in, zsize);
        QUICK_COMP_CASE(CPK) size = CPK_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(BZIP2_FILE) t32 = *outsize; size = unbzip2_file(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(LZ77WII_RAW11) size = unlz77wii_raw11(in, zsize, out, size);
        QUICK_COMP_CASE(LZ77WII_RAW30) size = unlz77wii_raw30(in, zsize, out, size);
        QUICK_COMP_CASE(LZ77WII_RAW20) size = unlz77wii_raw20(in, zsize, out, size, 8);
        QUICK_COMP_CASE(PGLZ) size = pglz_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(SLZ) size = UnPackSLZ(in, zsize, out, size);
        QUICK_COMP_CASE(SLZ_01) t32 = *outsize; size = slz_triace(in, zsize, &out, size, 1, &t32); *outsize = t32;
        QUICK_COMP_CASE(SLZ_02) t32 = *outsize; size = slz_triace(in, zsize, &out, size, 2, &t32); *outsize = t32;
        QUICK_COMP_CASE(SLZ_03) t32 = *outsize; size = slz_triace(in, zsize, &out, size, 3, &t32); *outsize = t32;
        //QUICK_COMP_CASE(LZHL) size = unlzhl(in, zsize, out, size);
        QUICK_COMP_CASE(D3101) size = d3101(in, zsize, out, size);
        QUICK_COMP_CASE(SQUEEZE) size = unsqueeze(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_ahuff) size = ahuff_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_arith) size = arith_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_arith1) size = arith1_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_arith1e) size = arith1e_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_arithn) size = arithn_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_compand) size = compand_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_huff) size = huff_ExpandMemory(in, zsize, out, size);
        //QUICK_COMP_CASE(TDCB_lzss) size = lzss_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_lzss)
            tmp1 = 12;  // INDEX_BIT_COUNT
            tmp2 = 4;   // LENGTH_BIT_COUNT
            tmp3 = 9;   // DUMMY9
            tmp4 = 0;   // END_OF_STREAM
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, &tmp4, NULL);
            tdcb_lzss_init(tmp1, tmp2, tmp3, tmp4);
            size = lzss_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_lzw12) size = lzw12_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_lzw15v) size = lzw15v_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(TDCB_silence) size = silence_ExpandMemory(in, zsize, out, size);
        QUICK_COMP_CASE(RDC) size = rdc_decompress(in, zsize, out);
        QUICK_COMP_CASE(ILZR) size = ilzr_expand(in, zsize, out, size);
        QUICK_COMP_CASE(DMC2) size = dmc2_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(diffcomp) size = diffcomp(in, zsize, out, size);
        QUICK_COMP_CASE(LZR) size = LZRDecompress(out, size, in, in + zsize);
        QUICK_COMP_CASE(LZS) size = unlzs(in, zsize, out, size, 0);
        QUICK_COMP_CASE(LZS_BIG) size = unlzs(in, zsize, out, size, 1);
        QUICK_COMP_CASE(COPY) size = uncopy(in, zsize, out, size);  // this is never called because COMP_COPY is handled in dumpa() to avoid wasting two buffers
        QUICK_COMP_CASE(COPY2) size = uncopy(in, zsize, out, size);
        QUICK_COMP_CASE(MOHLZSS) size = moh_lzss(in, zsize, out, size);
        QUICK_COMP_CASE(MOHRLE) size = moh_rle(in, zsize, out, size);
        QUICK_COMP_CASE(YAZ0) size = decodeYaz0(in, zsize, out, size);
        QUICK_COMP_CASE(BYTE2HEX) size = byte2hex(in, zsize, out, size, 1);
        QUICK_COMP_CASE(UN434A) un434a_init(); size = un434a(in, out);
        QUICK_COMP_CASE(UNZIP_DYNAMIC)   t32 = *outsize; size = unzip_dynamic(in, zsize, &out, &t32,   0); *outsize = t32;
        QUICK_COMP_CASE(GZPACK) size = gz_unpack(in, zsize, out, size);
        //QUICK_COMP_CASE(ZLIB_NOERROR)    size = unzip_zlib   (in, zsize, out, size, 1);
        //QUICK_COMP_CASE(DEFLATE_NOERROR) size = unzip_deflate(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ZLIB_NOERROR)    t32 = *outsize; size = unzip_dynamic(in, zsize, &out, &t32,  15); *outsize = t32;
        QUICK_COMP_CASE(DEFLATE_NOERROR) t32 = *outsize; size = unzip_dynamic(in, zsize, &out, &t32, -15); *outsize = t32;
        QUICK_COMP_CASE(PPMDH) size = ppmdh_decompress /*unppmdh*/ (in, zsize, out, size);
        QUICK_COMP_CASE(PPMDH_RAW)
            if(!g_comtype_dictionary) {
                g_comtype_dictionary = "10 4"; // for the comtype scanner
                fprintf(stderr, "\n"
                    "Error: the PPMdH decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying SaSize and MaxOrder, like:\n"
                    "         comtype ppmdh_raw \"%s\"\n", g_comtype_dictionary);
            }
            tmp1 = tmp2 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, NULL);
            size = ppmdh_decompress_raw /*unppmdh_raw*/ (in, zsize, out, size, tmp1, tmp2);
        QUICK_COMP_CASE(RNC)  t32 = *outsize; size = my_rnc_decompress(in, zsize, &out, &t32, size); *outsize = t32; //_rnc_unpack(in, zsize, out, 0);
        QUICK_COMP_CASE(RNCb) size = rnc_unpack(in, out, NULL, -1, -1);
        QUICK_COMP_CASE(RNCb_RAW) size = rnc_unpack(in, out, NULL, zsize, size);
        QUICK_COMP_CASE(RNCc_RAW) size = _rnc_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(RNC_RAW) size = RncDecoder__unpackM1(in, zsize, out, size);  // auto guess
        QUICK_COMP_CASE(FITD)
            if(!g_comtype_dictionary) {
                g_comtype_dictionary = "0xff"; // for the comtype scanner
                fprintf(stderr, "\n"
                    "Error: the PAK_explode decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying info5 (%s), like:\n"
                    "         get info5 byte\n"
                    "         comtype pak_explode info5\n", g_comtype_dictionary);
            }
            tmp1 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, NULL);
            PAK_explode(in, out, zsize, size, tmp1);    // no return value
        QUICK_COMP_CASE(KENS_Nemesis) size = KENS_Nemesis(in, zsize, out, size);
        QUICK_COMP_CASE(KENS_Kosinski) size = KENS_Kosinski(in, zsize, out, size, 0);
        QUICK_COMP_CASE(KENS_Kosinski_moduled) size = KENS_Kosinski(in, zsize, out, size, 1);
        QUICK_COMP_CASE(KENS_Enigma) size = KENS_Enigma(in, zsize, out, size);
        QUICK_COMP_CASE(KENS_Saxman) size = KENS_Saxman(in, zsize, out, size);
        QUICK_COMP_CASE(DRAGONBALLZ) size = undragonballz(in, zsize, out);
        QUICK_COMP_CASE(NITROSDK)
            t32 = (in[1]) | (in[2] << 8) | (in[3] << 16);
            if(t32 > *outsize) {
                *outsize = t32;
                myalloc(&out, size, outsize);
            }
            size = nitroDecompress(in, zsize, out, 1);
        QUICK_COMP_CASE(MSF) size = unmsf(in, zsize, out, size);
        QUICK_COMP_CASE(STARGUNNER) size = filter_stargunner_decompress(in, size /*correct*/, out);
        QUICK_COMP_CASE(NTCOMPRESS) // it's the SAME of LZ77WII so there is no real reason to use it here but keep it for legacy
            ntcompress_init();
            switch(ntcompress_type(in[0])) {
                case 0x00:  size = unlz77wii_raw00(in+4, zsize-4, out, size);   break;
                case 0x10:  size = unlz77wii_raw10(in+4, zsize-4, out, size);   break;
                case 0x11:  size = unlz77wii_raw11(in+4, zsize-4, out, size);   break;
                case 0x20:  size = unlz77wii_raw20(in, zsize, out, size, 0);    break;
                case 0x30:  size = ntcompress_30(in+4, zsize-4, out);           break;
                case 0x40:  size = ntcompress_40(in+4, zsize-4, out);           break;
                default:    size = -1;
                // modify nintendo.h too
            }
            ntcompress_free(NULL);
            if(size < 0) size = nitroDecompress(in, zsize, out, 1); // just because size is not mandatory
        QUICK_COMP_CASE(CRLE) size = CRLE_Decode(out, size, in, zsize);
#ifdef WIN32    // too boring to make the .a for linux too
        QUICK_COMP_CASE(CTW) size = unctw(in, zsize, out, size);
#endif
        QUICK_COMP_CASE(DACT_DELTA) size = dact_delta_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_MZLIB2) size = dact_mzlib2_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_MZLIB) size = dact_mzlib_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_RLE) size = dact_rle_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_SNIBBLE) size = dact_snibble_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_TEXT) size = dact_text_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(DACT_TEXTRLE) size = dact_textrle_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(EXECUTE)
            size = quickbms_execute_pipe(g_comtype_dictionary, in, zsize, &out, size, NULL);
            if(size >= 0) *outsize = size;
        QUICK_COMP_CASE(CALLDLL) size = quickbms_calldll_pipe(g_comtype_dictionary, in, zsize, out, size);
        QUICK_COMP_CASE(LZ77_0) size = PDRLELZ_DecompLZ2(in, out, zsize);
        QUICK_COMP_CASE(LZBSS) size = LZBSS_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(BPAQ0) size = BPAQ0_DecodeData(in, out);
        QUICK_COMP_CASE(LZPX) size = lzpx_unpack(in, out);
        QUICK_COMP_CASE(MAR_RLE) size = MAR_RLE(in, zsize, out, size);
        QUICK_COMP_CASE(GDCM_RLE) size = gdcm_rle(in, zsize, out, size);
        QUICK_COMP_CASE(LZMAT) t32 = size; if(lzmat_decode(out, &t32, in, zsize)) size = -1; size = t32;
        QUICK_COMP_CASE(DICT)
            t32 = size;
            if(DictDecode(in, zsize, out, &t32) < 0) {
                size = -1;
            } else {
                size = t32;
            }
        QUICK_COMP_CASE(REP) size = unrep(in, zsize, out, size);
        QUICK_COMP_CASE(LZP) size = LZPDecode(in, zsize, out, 32);
        QUICK_COMP_CASE(ELIAS_DELTA) size = eliasDeltaDecode(in, zsize, out);
        QUICK_COMP_CASE(ELIAS_GAMMA) size = eliasGammaDecode(in, zsize, out);
        QUICK_COMP_CASE(ELIAS_OMEGA) size = eliasOmegaDecode(in, zsize, out);
        QUICK_COMP_CASE(PACKBITS) size = unpackbits(in, zsize, out);
        QUICK_COMP_CASE(DARKSECTOR_NOCHUNKS) size = undarksector(in, zsize, out, size, 0);
        QUICK_COMP_CASE(ENET)
            ENetRangeCoder  enet_ctx;
            memset(&enet_ctx, 0, sizeof(enet_ctx));
            size = enet_range_coder_decompress (&enet_ctx, in, zsize, out, size);
        QUICK_COMP_CASE(EDUKE32) size = eduke32_lzwuncompress(in, zsize, out, size);
        QUICK_COMP_CASE(XU4_RLE) size = xu4_rleDecompress(in, zsize, out, size);
        QUICK_COMP_CASE(RVL) size = RVLCompress_decompress_ints(in, (void *)out, zsize);
        QUICK_COMP_CASE(LZFU)
            t_size = size; // unused
            p = pst_lzfu_decompress(in, zsize, &t_size, 1);
            size = t_size;
            if(p) {
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                FREE(p);
            }
        QUICK_COMP_CASE(LZFU_RAW)
            t_size = size; // unused
            p = pst_lzfu_decompress(in, zsize, &t_size, 0);
            size = t_size;
            if(p) {
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                FREE(p);
            }
        QUICK_COMP_CASE(XU4_LZW) size = ultima4_lzwDecompress(in, out, zsize);
        QUICK_COMP_CASE(HE3) size = decode_he3_data(in, out, size, 0); // don't check the signature
        QUICK_COMP_CASE(IRIS) size = iris_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(IRIS_HUFFMAN) size = iris_huffman(in, zsize, out, size);
        QUICK_COMP_CASE(IRIS_UO_HUFFMAN) size = iris_uo_huffman(in, zsize, out, size);
        QUICK_COMP_CASE(NTFS) size = ntfs_decompress(out, size, in, zsize);
        QUICK_COMP_CASE(PDB) size = pdb_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_SPREAD) size = decode_spread(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_RLE1) size = decode_rle1(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_RLE2) size = decode_rle2(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_RLE3) size = decode_rle3(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_RLE4) size = rle4decoding(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_ARITH) size = arithshift_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(COMPRLIB_SPLAY) size = splay_trees(in, zsize, out, size, 0);
        QUICK_COMP_CASE(CABEXTRACT) size = LZXdecompress(in, out, zsize, size);
        QUICK_COMP_CASE(MRCI) MRCIDecompressWrapper(in, zsize, out, size); // no size returned
        QUICK_COMP_CASE(HD2_01) hd2_init(); size = hd2_01(in, out, zsize);
        QUICK_COMP_CASE(HD2_08)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the hd2_08 decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying the 0x2c bytes of the first chunk\n");
                //myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            hd2_init();
            size = hd2_08(in, out, zsize, g_comtype_dictionary);
        QUICK_COMP_CASE(HD2_01raw) hd2_init(); size = hd2_01raw(in, out, zsize);
        QUICK_COMP_CASE(RTL_LZNT1) size = rtldecompress(RTL_COMPRESSION_FORMAT_LZNT1, in, zsize, out ,size);
        QUICK_COMP_CASE(RTL_XPRESS) size = rtldecompress(RTL_COMPRESSION_FORMAT_XPRESS, in, zsize, out ,size);
        QUICK_COMP_CASE(RTL_XPRESS_HUFF) size = rtldecompress(RTL_COMPRESSION_FORMAT_XPRESS_HUFF, in, zsize, out ,size);
        QUICK_COMP_CASE(PRS) size = prs_decompress(in, out);
        QUICK_COMP_CASE(SEGA_LZ77) size = sega_lz77(in, zsize, out, 0);
        QUICK_COMP_CASE(SAINT_SEYA) size = Model_GMI_Decompress(in, zsize, out, size);
        QUICK_COMP_CASE(NTCOMPRESS30) size = ntcompress_30(in, zsize, out);
        QUICK_COMP_CASE(NTCOMPRESS40) size = ntcompress_40(in, zsize, out);
        QUICK_COMP_CASE(YAKUZA) size = unyakuza(in, zsize, out, size, 0);
        QUICK_COMP_CASE(LZ4)
            if(g_comtype_dictionary) {
                size = LZ4_decompress_safe_usingDict(in, out, zsize, size, g_comtype_dictionary, g_comtype_dictionary_len);
            } else {
                // hard choice here:
                // LZ4_decompress_safe returns errors if there are additional bytes after the compressed stream (because it's raw)
                // LZ4_decompress_safe_partial returns no errors if the stream is valid
                // currently I opt for the second one because gives more freedom to quickbms and its scanner

                //size = LZ4_decompress_safe(in, out, zsize, size);
                size = LZ4_decompress_safe_partial(in, out, zsize, size, *outsize);
            }
        QUICK_COMP_CASE(SNAPPY)
            //size = unsnappy(in, zsize, out, size);
            size_t_tmp = size;
            if(snappy_uncompress(in, zsize, out, &size_t_tmp) == SNAPPY_OK) {
                size = size_t_tmp;
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(LUNAR_LZ1)  size = lunar_uncompress(in, zsize, out, size, LC_LZ1,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ2)  size = lunar_uncompress(in, zsize, out, size, LC_LZ2,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ3)  size = lunar_uncompress(in, zsize, out, size, LC_LZ3,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ4)  size = lunar_uncompress(in, zsize, out, size, LC_LZ4,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ5)  size = lunar_uncompress(in, zsize, out, size, LC_LZ5,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ6)  size = lunar_uncompress(in, zsize, out, size, LC_LZ6,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ7)  size = lunar_uncompress(in, zsize, out, size, LC_LZ7,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ8)  size = lunar_uncompress(in, zsize, out, size, LC_LZ8,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ9)  size = lunar_uncompress(in, zsize, out, size, LC_LZ9,  myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ10) size = lunar_uncompress(in, zsize, out, size, LC_LZ10, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ11) size = lunar_uncompress(in, zsize, out, size, LC_LZ11, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ12) size = lunar_uncompress(in, zsize, out, size, LC_LZ12, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ13) size = lunar_uncompress(in, zsize, out, size, LC_LZ13, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ14) size = lunar_uncompress(in, zsize, out, size, LC_LZ14, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ15) size = lunar_uncompress(in, zsize, out, size, LC_LZ15, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ16) size = lunar_uncompress(in, zsize, out, size, LC_LZ16, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ17) size = lunar_uncompress(in, zsize, out, size, LC_LZ17, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ18) size = lunar_uncompress(in, zsize, out, size, LC_LZ18, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_LZ19) size = lunar_uncompress(in, zsize, out, size, LC_LZ19, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_RLE1) size = lunar_uncompress(in, zsize, out, size, LC_RLE1, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_RLE2) size = lunar_uncompress(in, zsize, out, size, LC_RLE2, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_RLE3) size = lunar_uncompress(in, zsize, out, size, LC_RLE3, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(LUNAR_RLE4) size = lunar_uncompress(in, zsize, out, size, LC_RLE4, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(GOLDENSUN) size = goldensun(in, zsize, out, size);
        QUICK_COMP_CASE(LUMINOUSARC) size = luminousarc(in, zsize, out, size);
        QUICK_COMP_CASE(LZV1) size = rLZV1(in, out, zsize, size);
        QUICK_COMP_CASE(FASTLZAH) size = fastlz_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(ZAX) size = zax_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(SHRINKER) size = shrinker_decompress(in, out, size);  // yes, it's size
        QUICK_COMP_CASE(MMINI_HUFFMAN)
            p = malloc(MMINI_HUFFHEAP_SIZE);
            if(!p) STD_ERR(QUICKBMS_ERROR_MEMORY);
            size = mmini_huffman_decompress(in, zsize, out, size, p);
            FREE(p);
        QUICK_COMP_CASE(MMINI_LZ1) size = mmini_lzl_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(MMINI)
            l = malloc(size);
            p = malloc(MMINI_HUFFHEAP_SIZE);
            if(!l || !p) STD_ERR(QUICKBMS_ERROR_MEMORY);
            size = mmini_decompress(in, zsize, l, size, out, size, p);
            FREE(p);
            FREE(l);
        QUICK_COMP_CASE(CLZW)
            lzw_dec_t  *clzw_ctx;
            clzw_ctx = calloc(1, sizeof(lzw_dec_t));
            if(!clzw_ctx) STD_ERR(QUICKBMS_ERROR_MEMORY);
            clzw_outsz = 0;
            lzw_dec_init(clzw_ctx, out);
            lzw_decode(clzw_ctx, in, zsize);
            size = clzw_outsz;
            FREE(clzw_ctx);
        QUICK_COMP_CASE(LZHAM) size = lzham_unpack(in, zsize, out, size, g_comtype_dictionary);
        QUICK_COMP_CASE(LPAQ8)
            size = -1;
            /* wasting of memory and speed!!!
            tmp1 = 0;
            tmp2 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, NULL);
            size = unlpaq8(in, zsize, out, size, tmp1, tmp2);
            */
        QUICK_COMP_CASE(SEGA_LZS2) t32 = *outsize; size = sega_lzs2(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(WOLF) size = dewolf(in, zsize, out, size, 9);
        QUICK_COMP_CASE(COREONLINE) size = CO_Decompress(out, size, in);
        QUICK_COMP_CASE(MSZIP) size = unmspack(in, zsize, out, size, -1, -1, -1, 1);
        QUICK_COMP_CASE(QTM)
            tmp1 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, NULL);
            size = unmspack(in, zsize, out, size, tmp1 ? tmp1 : 21, -1, -1, 2);
        QUICK_COMP_CASE(MSLZSS) size = unmspack(in, zsize, out, size, -1,  0, -1, 3);
        QUICK_COMP_CASE(MSLZSS1) size = unmspack(in, zsize, out, size, -1,  1, -1, 3);
        QUICK_COMP_CASE(MSLZSS2) size = unmspack(in, zsize, out, size, -1,  2, -1, 3);
        QUICK_COMP_CASE(KWAJ) size = unmspack(in, zsize, out, size, -1, -1, -1, 4);
        QUICK_COMP_CASE(LZLIB) size = unlzlib(in, zsize, out, size);
        QUICK_COMP_CASE(DFLT) size = undflt(in, zsize, out, size);
        QUICK_COMP_CASE(LZMA_DYNAMIC) QUICK_COMP_CASE_LZMA_DOIT(,LZMA_FLAGS_NONE,1)
        QUICK_COMP_CASE(LZMA2_DYNAMIC) QUICK_COMP_CASE_LZMA_DOIT(2,LZMA_FLAGS_NONE,1)
        QUICK_COMP_CASE(LZXCAB_DELTA) size = unmspack(in, zsize, out, size, 21, 0, -1, 5);
        QUICK_COMP_CASE(LZXCHM_DELTA) size = unmspack(in, zsize, out, size, 16, 2, -1, 5);
        QUICK_COMP_CASE(FFCE) ffce_init(); size = ffce(in, out, 0);
        //QUICK_COMP_CASE(SCUMMVM1)  size = Screen__decompressTony(in, zsize, out, size);
        //QUICK_COMP_CASE(SCUMMVM2)  size = Screen__decompressRLE7(in, zsize, out, size);
        //QUICK_COMP_CASE(SCUMMVM3)  size = Screen__decompressRLE0(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM4)  size = Screen__decompressHIF(in, out);
        QUICK_COMP_CASE(SCUMMVM5)  size = decompressSPCN(in, out, size);
        QUICK_COMP_CASE(SCUMMVM6)  size = RncDecoder__unpackM1(in, zsize, out, size);   // RNC1
        QUICK_COMP_CASE(SCUMMVM7)  size = RncDecoder__unpackM2(in, zsize, out, size);   // RNC2
        QUICK_COMP_CASE(SCUMMVM8)  size = CineUnpacker__unpack(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM9)  size = delphineUnpack(out, size, in, zsize);
        QUICK_COMP_CASE(SCUMMVM10) size = DataIO__unpackChunk(in, out, size);
        QUICK_COMP_CASE(SCUMMVM11) size = DecompressorHuffman__unpack(in, out, zsize, size);
        QUICK_COMP_CASE(SCUMMVM12) size = DecompressorLZW__unpackLZW(in, out, zsize, size);
        QUICK_COMP_CASE(SCUMMVM13) size = DecompressorLZW__unpackLZW1(in, out, zsize, size);
        QUICK_COMP_CASE(SCUMMVM14) size = DecompressorLZW__reorderPic(in, out, size);
        QUICK_COMP_CASE(SCUMMVM15) size = DecompressorLZW__reorderView(in, out);
        QUICK_COMP_CASE(SCUMMVM16) size = DecompressorLZS__unpack(in, out, zsize, size);
        QUICK_COMP_CASE(SCUMMVM17) size = decodeSRLE(out, in, size);
        QUICK_COMP_CASE(SCUMMVM18) size = Screen__unpackRle(in, out, size);
        QUICK_COMP_CASE(SCUMMVM19) size = PS2Icon__decompressData(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM20) size = LZWDecoder__lzwExpand(in, out, size);
        QUICK_COMP_CASE(SCUMMVM21) size = uncompressPlane(in, out, size);
        QUICK_COMP_CASE(SCUMMVM22) size = unbarchive(in, zsize, out);
        QUICK_COMP_CASE(SCUMMVM23) size = Screen__decompressRLE256(out, in, size);
        QUICK_COMP_CASE(SCUMMVM24) size = Screen__decompressHIF_other(in, out /*, uint32 *skipData*/);
        QUICK_COMP_CASE(SCUMMVM25) size = FileExpander__process(out, in, size, zsize);
        QUICK_COMP_CASE(SCUMMVM26) size = SoundTownsPC98_v2__voicePlay(in, out, size);
        QUICK_COMP_CASE(SCUMMVM27) size = VQAMovie__decodeSND1(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM28) size = Background__decodeComponent(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM29) size = AnimFrame__decomp34(in, out, size, 0x7, 3);
        QUICK_COMP_CASE(SCUMMVM30) size = AnimFrame__decomp34(in, out, size, 0xf, 4);
        QUICK_COMP_CASE(SCUMMVM31) size = AnimFrame__decomp5(in, out, size);
        QUICK_COMP_CASE(SCUMMVM32) size = AnimFrame__decomp7(in, out, size);
        QUICK_COMP_CASE(SCUMMVM33) size = AnimFrame__decompFF(in, out, size);
        QUICK_COMP_CASE(SCUMMVM34) size = SavegameStream__readCompressed(in, out, size);
        QUICK_COMP_CASE(SCUMMVM35) size = dimuse_compDecode(in, out);
        QUICK_COMP_CASE(SCUMMVM36) size = decompressADPCM(in, out, 1);
        QUICK_COMP_CASE(SCUMMVM37) size = decompressADPCM(in, out, 2);
        QUICK_COMP_CASE(SCUMMVM38) size = MohawkBitmap__unpackRiven(in, zsize, out);
        QUICK_COMP_CASE(SCUMMVM39) size = MohawkBitmap__drawRLE8(in, out, size);
        QUICK_COMP_CASE(SCUMMVM40) size = LzhDecompressor__decompress(in, out, zsize, size);
        QUICK_COMP_CASE(SCUMMVM41) size = AnimationDecoder__decode_data(in, zsize, out);
        QUICK_COMP_CASE(SCUMMVM42) size = MusicPlayerMac_t7g__decompressMidi(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM43) size = StuffItArchive__decompress14(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM44) size = decompressIconPlanar(out, size, in);
        QUICK_COMP_CASE(SCUMMVM45) size = DrasculaEngine__decodeRLE(in, out /*, uint16 pitch*/);
        QUICK_COMP_CASE(SCUMMVM46) size = bompDecodeLine(out, in, size);
        QUICK_COMP_CASE(SCUMMVM47) size = bompDecodeLineReverse(out, in, size);
        QUICK_COMP_CASE(SCUMMVM48) size = ToucheEngine__res_decodeScanLineImageRLE(in, out, size);
        QUICK_COMP_CASE(SCUMMVM49) size = AnimationPlayer__rleDecode(in, out, size);
        QUICK_COMP_CASE(SCUMMVM50) size = Graphics__decodeRLE(out, size, in);
        QUICK_COMP_CASE(SCUMMVM51) size = MSRLEDecoder__decode8(in, zsize, out, size);
        QUICK_COMP_CASE(SCUMMVM52) size = unarj(in, zsize, out, size, 0);
        QUICK_COMP_CASE(SCUMMVM53) size = unarj(in, zsize, out, size, 1);
        QUICK_COMP_CASE(LZS_UNZIP)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the LZS_UNZIP decompression requires the setting of the dictionary\n"
                    "       field in comtype, like:\n"
                    "         comtype lzs_unzip \"abcabcabcabc\"\n");
                //myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            size = lzs_unzip(in, zsize, g_comtype_dictionary, g_comtype_dictionary_len, out, size, 5);
        QUICK_COMP_CASE(LEGEND_OF_MANA) size = legend_of_mana(in, zsize, out, size);
        QUICK_COMP_CASE(DIZZY) size = dizzy_decompress(in, zsize, out);
        QUICK_COMP_CASE(EDL1) size = EDLdec1(0, in, out, zsize, size, g_endian);
        QUICK_COMP_CASE(EDL2) size = EDLdec2(0, in, out, zsize, size, g_endian);
        QUICK_COMP_CASE(DUNGEON_KID) size = dungeon_kid(in, out);
        QUICK_COMP_CASE(FRONTMISSION2) size = frontmission2(in, zsize, out);
        QUICK_COMP_CASE(RLEINC1) size = rleinc1(in, zsize, out);
        QUICK_COMP_CASE(RLEINC2) size = rleinc2(in, zsize, out);
        QUICK_COMP_CASE(EVOLUTION) size = evolution_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(PUYO_LZ10) size = puyo_lz10(in, zsize, out, size);
        QUICK_COMP_CASE(PUYO_LZ11) size = puyo_lz11(in, zsize, out, size);
        QUICK_COMP_CASE(NISLZS) size = nislzs(in, out, size);
        QUICK_COMP_CASE(UNKNOWN1)  compression_unknown_init(); size = compression_unknown1(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN2)  compression_unknown_init(); size = compression_unknown2(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN3)  compression_unknown_init(); size = compression_unknown3(in, zsize, out, size, 0);
        QUICK_COMP_CASE(UNKNOWN4)  compression_unknown_init(); size = compression_unknown4(in, zsize, g_comtype_dictionary, g_comtype_dictionary_len, out, size);
        QUICK_COMP_CASE(UNKNOWN5)  compression_unknown_init(); size = compression_unknown5(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN6)  compression_unknown_init(); size = compression_unknown6(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN7)  compression_unknown_init(); size = compression_unknown7(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN8)  compression_unknown_init(); size = compression_unknown8(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN9)  compression_unknown_init(); size = compression_unknown9(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN10) compression_unknown_init(); size = compression_unknown10(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN11) compression_unknown_init(); size = compression_unknown11(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN12) compression_unknown_init(); size = compression_unknown12(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN13) compression_unknown_init(); size = compression_unknown13(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN14) compression_unknown_init(); size = compression_unknown14(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN15) compression_unknown_init(); size = compression_unknown15(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN16) compression_unknown_init(); size = compression_unknown16(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN17) compression_unknown_init(); size = compression_unknown17(in, zsize, out, size);
        QUICK_COMP_CASE(UNKNOWN18) compression_unknown_init(); size = compression_unknown18(in, zsize, g_comtype_dictionary, g_comtype_dictionary_len, out, size);
        QUICK_COMP_CASE(UNKNOWN19) compression_unknown_init(); size = compression_unknown19(g_comtype_dictionary, in, out, size);
        QUICK_COMP_CASE(BLACKDESERT) size = blackdesert_unpack(in, out);
        QUICK_COMP_CASE(BLACKDESERT_RAW) size = blackdesert_unpack_core(in, out, size, out, zsize);
        QUICK_COMP_CASE(PUCRUNCH) size = pucrunch_UnPack(-1, in, out, 0);
        QUICK_COMP_CASE(ZPAQ) size = zpaq_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(ZYXEL_LZS) size = zyxel_lzs_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(BLOSC) size = blosclz_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(GIPFELI) size = gipfeli_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(CRUSH) size = crush_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(YAPPY) size = yappy_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZG) size = LZG_Decode(in, zsize, out, size);
        QUICK_COMP_CASE(DOBOZ) size = doboz_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(TORNADO) size = tornado_decompress(in, zsize, out, size, -1);
        QUICK_COMP_CASE(XPKSQSH) size = xpkSQSH_unsqsh(in, out);
        #if defined(__i386__) || defined(__x86_64__)
        #ifndef __APPLE__
        QUICK_COMP_CASE(AMIGA_UNSQUASH) size = amiga_unsquash(in, zsize, out);
        QUICK_COMP_CASE(AMIGA_BYTEKILLER) BYTUNP(zsize, 0, 0, 0, 0, in, out, size);
        QUICK_COMP_CASE(AMIGA_FLASHSPEED) size = UFLSP(out, in, 0);
        QUICK_COMP_CASE(AMIGA_IAMICE) if(size > zsize) { memcpy(out, in, zsize); IAMICE(out); }
        QUICK_COMP_CASE(AMIGA_IAMATM) if(size > zsize) { memcpy(out, in, zsize); IAMATM(out); }
        QUICK_COMP_CASE(AMIGA_ISC1P) size = ISC1P(in, out, 0, 0);
        QUICK_COMP_CASE(AMIGA_ISC2P) size = ISC2P(in, out, 0, 0);
        QUICK_COMP_CASE(AMIGA_ISC3P) size = ISC3P(in, out, 0, 0, zsize);
        QUICK_COMP_CASE(AMIGA_UPCOMP) UPCOMP(out, in, zsize, size);
        QUICK_COMP_CASE(AMIGA_UPHD) p = calloc(3670,1); UPHD(in, out, p); FREE(p);
        QUICK_COMP_CASE(AMIGA_BYTEKILLER3) p = calloc(0x10000 /*???*/,1); DeCr00(in, out, p); FREE(p);
        QUICK_COMP_CASE(AMIGA_BYTEKILLER2) p = calloc(0x10000 /*???*/,1); ByteKiller2(in, out, p); FREE(p);
        QUICK_COMP_CASE(AMIGA_CRUNCHMANIA17b) crunchmania_17b(out, size, in, zsize);
        QUICK_COMP_CASE(AMIGA_POWERPACKER) pp_DecrunchBuffer(in + zsize, out, myatoi(g_comtype_dictionary));
        QUICK_COMP_CASE(AMIGA_STONECRACKER2) stonecracker2(in, out);
        QUICK_COMP_CASE(AMIGA_STONECRACKER3) p = calloc(0x10000 /*???*/,1); stonecracker3(in, out, p); FREE(p);
        QUICK_COMP_CASE(AMIGA_STONECRACKER4) stonecracker403(out, in);
        QUICK_COMP_CASE(AMIGA_CRUNCHMASTER) size = UCRMAS(in, out, size);
        QUICK_COMP_CASE(AMIGA_CRUNCHMANIA) crunchmania_FastDecruncher(out, size, in, zsize);
        QUICK_COMP_CASE(AMIGA_CRUNCHMANIAh) crunchmania_FastDecruncherHuff(out, size, in, zsize);
        QUICK_COMP_CASE(AMIGA_CRUNCHOMATIC) if(size > zsize) { memcpy(out, in, zsize); UCMAT(out + 12, size, out); /*totally invented, just a stub*/ }
        QUICK_COMP_CASE(AMIGA_DISCOVERY) if(size > zsize) { memcpy(out, in, zsize); UNDIMP(out); }
        QUICK_COMP_CASE(AMIGA_LIGHTPACK) LIGHT15(in, out, size);
        QUICK_COMP_CASE(AMIGA_MASTERCRUNCHER) UMAST31(out, in);
        QUICK_COMP_CASE(AMIGA_MAXPACKER) MAX12(out, in);
        QUICK_COMP_CASE(AMIGA_MEGACRUNCHER) UMEGA(in + zsize, size, out);
        QUICK_COMP_CASE(AMIGA_PACKIT) PACIT(in, in + zsize, out);
        QUICK_COMP_CASE(AMIGA_SPIKECRUNCHER) USPIKE(in, out, out + size);
        QUICK_COMP_CASE(AMIGA_TETRAPACK) UTETR(in, in + zsize, out, size);
        QUICK_COMP_CASE(AMIGA_TIMEDECRUNCH) time_decrunch(in, out);
        QUICK_COMP_CASE(AMIGA_TRYIT) TRY101(in, out);
        QUICK_COMP_CASE(AMIGA_TUC) UTUC(in, out, size);
        QUICK_COMP_CASE(AMIGA_TURBOSQUEEZER61) UTSQ61(in + zsize, out + size, out);
        QUICK_COMP_CASE(AMIGA_TURBOSQUEEZER80) UTSQ80(in + zsize, out + size, out);
        QUICK_COMP_CASE(AMIGA_TURTLESMASHER) LhDecode(zsize, in, out, 1);
        QUICK_COMP_CASE(AMIGA_DMS) size = DMSUNP(in, out);
        QUICK_COMP_CASE(AMIGA_PACKFIRE) p = calloc(15980,1); packfire(in, out, p); FREE(p);
        #endif
        #endif
        QUICK_COMP_CASE(ALBA_BPE) size = alba_BPD(in, zsize, out);
        QUICK_COMP_CASE(ALBA_BPE2) size = alba_BPD2(in, zsize, out);
        QUICK_COMP_CASE(FLZP) size = flzp_decompress(in, zsize, out);
        QUICK_COMP_CASE(SR2) size = sr3_decompress(in, zsize, out, size, 2, 0);
        QUICK_COMP_CASE(SR3) size = sr3_decompress(in, zsize, out, size, 3, 0);
        QUICK_COMP_CASE(BPE2v3) size = bpe2_decompress(in, zsize, out);
        QUICK_COMP_CASE(BPE_ALT1) size = bpe_alt1_decode_buf(size, zsize, in, out, 0);
        QUICK_COMP_CASE(BPE_ALT2) size = bpe_alt1_decode_buf(size, zsize, in, out, 1);
        QUICK_COMP_CASE(CBPE) size = CBPE__Decode(out, in, zsize);
        QUICK_COMP_CASE(SCPACK0)
            if(g_comtype_dictionary && (g_comtype_dictionary_len < 10) && (myatoi(g_comtype_dictionary) > 0)) {
                size = strexpand(out, in, zsize, size, NULL, myatoi(g_comtype_dictionary));
            } else {
                size = strexpand(out, in, zsize, size, g_comtype_dictionary, 0);
            }
        QUICK_COMP_CASE(LZOVL) size = LZOvl_Decompress(in, zsize, out);
        QUICK_COMP_CASE(NITROSDK_DIFF8) size = DiffFiltRead(in, zsize, out, 8);
        QUICK_COMP_CASE(NITROSDK_DIFF16) size = DiffFiltRead(in, zsize, out, 16);
        QUICK_COMP_CASE(NITROSDK_HUFF8) size = HuffCompRead(in, zsize, out, 8);
        QUICK_COMP_CASE(NITROSDK_HUFF16) size = HuffCompRead(in, zsize, out, 16);
        QUICK_COMP_CASE(NITROSDK_LZ) size = LZCompRead(in, zsize, out);
        QUICK_COMP_CASE(NITROSDK_RL) size = RLCompRead(in, zsize, out);
        QUICK_COMP_CASE(QCMP) size = qcmp_unpack(in, zsize, out);
        QUICK_COMP_CASE(SPARSE) size = DecompressSparse(in, zsize, out, size);
        QUICK_COMP_CASE(STORMHUFF) size = StormLib_Decompress_huff(out, size, in, zsize);
        QUICK_COMP_CASE(GZIP_STRICT) t32 = *outsize; size = ungzip(in, zsize, &out, &t32, 1); *outsize = t32;
        QUICK_COMP_CASE(CT_HughesTransform) size = shadowforce_decompress(CT_HughesTransform, in, zsize, out, size);
        QUICK_COMP_CASE(CT_LZ77) size = shadowforce_decompress(CT_LZ77, in, zsize, out, size);
        QUICK_COMP_CASE(CT_ELSCoder) size = shadowforce_decompress(CT_ELSCoder, in, zsize, out, size);
        QUICK_COMP_CASE(CT_RefPack) size = shadowforce_decompress(CT_RefPack, in, zsize, out, size);
        QUICK_COMP_CASE(QFS)
            size = -1;
            t32 = zsize;
            p = qfs_uncompress_data(in, &t32);
            if(p) {
                size = t32;
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                real_free(p);
            }
        QUICK_COMP_CASE(PXP) unpxp_init(); size = unpxp(in, out, size);
        QUICK_COMP_CASE(BOH) size = unboh_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(GRC) size = ungrc(in, zsize, out);
        QUICK_COMP_CASE(ZEN)
            #ifdef WIN32
            t32 = zen_decompress(in, out, size);
            if(t32 < 0) size = -1;
            else        size -= t32;    // usually t32 is zero which means ok
            #else
            size = -1;
            #endif
        QUICK_COMP_CASE(LZHUFXR)
            p = NULL;
            t32 = size;
            _decompressLZ(&p, &t32, in, zsize);
            if(p) {
                size = t32;
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                real_free(p);
            }
        QUICK_COMP_CASE(FSE) size = FSE_decompress(out, size, in, zsize);
        QUICK_COMP_CASE(FSE_RLE) size = -1; /*it was just a 1:1 copy!*/ /*size = FSE_decompressRLE(out, size, in, zsize);*/
        QUICK_COMP_CASE(ZSTD)
            tmp2 = size;
            size = myzstd_decompress(out, size, in, zsize, g_comtype_dictionary, g_comtype_dictionary_len);
            if(size < 0) {  // just in case it's necessary more space for the decompression, totally useless but why not doing it...
                if(ZSTD_isLegacy(in, zsize) > 0) {
                    tmp1 = ZSTD_getDecompressedSize_legacy(in, zsize);
                } else {
                    tmp1 = ZSTD_getFrameContentSize(in, zsize);
                }
                if((tmp1 > 0) /*0 is error*/ && (tmp1 > tmp2)) {
                    size = tmp1;
                    myalloc(&out, size, outsize);
                    size = myzstd_decompress(out, size, in, zsize, g_comtype_dictionary, g_comtype_dictionary_len);
                }
            }
        QUICK_COMP_CASE(AZO) size = unazo(in, zsize, out, size);
        QUICK_COMP_CASE(PP20) size = unpp20(in, zsize, out, size);
        QUICK_COMP_CASE(DS_BLZ) nintendo_ds_set_inout(in, zsize, out, size); BLZ_Decode(""); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_HUF) size = unlz77wii_raw20(in, zsize, out, size, 8);
        QUICK_COMP_CASE(DS_LZE) nintendo_ds_set_inout(in, zsize, out, size); LZE_Decode(""); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_LZS) nintendo_ds_set_inout(in, zsize, out, size); LZS_Decode(""); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_LZX) nintendo_ds_set_inout(in, zsize, out, size); LZX_Decode(""); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_RLE) nintendo_ds_set_inout(in, zsize, out, size); RLE_Decode(""); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(FAB) size = FabDecompressor__decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZ4F) size = lz4f_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(PCLZFG) size = unpclzfg(in, zsize, out);
        QUICK_COMP_CASE(LZOO) /*no size returned*/ lzd(in, out);
        QUICK_COMP_CASE(DELZC) size = de_compress(zsize, 14, in, out);
        QUICK_COMP_CASE(DEHUFF) size = de_huffman(size, in, out);
        QUICK_COMP_CASE(HEATSHRINK) size = heatshrink_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(SMAZ) size = smaz_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZFX)
            t32 = size;
            if(lzfx_decompress(in, zsize, out, &t32) < 0) size = -1;
            else size = t32;
        QUICK_COMP_CASE(PITHY) size = mypithy_Decompress(in, zsize, out, size);
        QUICK_COMP_CASE(ZLING) size = zling_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(CSC) size = csc_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(DENSITY)
            density_processing_result   density_res;
            density_res = density_decompress(in, zsize, out, size);
            size = density_res.bytesWritten;
        QUICK_COMP_CASE(BROTLI)
            t_size = size;
            if(BrotliDecoderDecompress(zsize, in, &t_size, out) != BROTLI_DECODER_RESULT_SUCCESS) size = -1;
            else size = t_size;
        QUICK_COMP_CASE(RLE32) size = rle32_decode(in, zsize, out);
        QUICK_COMP_CASE(RLE35) size = rle35_decode(in, zsize, out);
        QUICK_COMP_CASE(BSC)
            bsc_init(LIBBSC_FEATURE_NONE);
            t32 = size;
            tt32 = zsize;
            if(!bsc_block_info(in, zsize, &tt32, &t32, LIBBSC_FEATURE_NONE)) {
                size = t32;
                myalloc(&out, size, outsize);
                if(bsc_decompress(in + (zsize - tt32), tt32, out, size, LIBBSC_FEATURE_NONE)) size = -1;
            } else {
                if(bsc_decompress(in, zsize, out, size, LIBBSC_FEATURE_NONE)) size = -1;
                // there is no way to know the size
            }
        QUICK_COMP_CASE(SHOCO) size = shoco_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(WFLZ)
            t32 = wfLZ_GetDecompressedSize(in);
            if((t32 > 0) && (t32 < size)) {
                size = t32;
                wfLZ_Decompress(in, out);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(FASTARI)
            t_size = size;
            fa_decompress(in, out, zsize, &t_size);
            size = t_size;
        QUICK_COMP_CASE(RLE_ORCOM) size = rle_orcom(in, zsize, out);
        QUICK_COMP_CASE(DICKY)
            p = NULL;
            t_size = 0;
            if(!dicky_uncompress((char**)&p, &t_size, in, zsize)) {
                size = t_size;
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
            } else {
                size = -1;
            }
            dicky_free(p);
        QUICK_COMP_CASE(SQUISH) size = squish_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(LZNT1)
            t_size = size;
            if(lznt1_decompress(in, zsize, out, &t_size) < 0) size = t_size;
            size = -1;
        QUICK_COMP_CASE(XPRESS)
            t_size = size;
            if(xpress_decompress(in, zsize, out, &t_size) < 0) size = t_size;
            size = -1;
        QUICK_COMP_CASE(XPRESS_HUFF)
            t_size = size;
            if(xpress_huff_decompress(in, zsize, out, &t_size) < 0) size = t_size;
            size = -1;
        QUICK_COMP_CASE(LZJODY) size = lzjody_decompress(in, out, zsize, 0);
        QUICK_COMP_CASE(LZHL)
            p = lzhlight_initDecomp();
            size = lzhlight_decompress(p, in, zsize, out, size);
            lzhlight_delDecomp(p);
        QUICK_COMP_CASE(NEPTUNIA) size = neptunia(in, zsize, out, size);
        QUICK_COMP_CASE(TRLE) size = trled(in, zsize, out, size);
        QUICK_COMP_CASE(SRLE) size = srled(in, zsize, out, size);
        QUICK_COMP_CASE(MRLE) size = mrled(in, out, size);
        QUICK_COMP_CASE(JCH) size = jch_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZRW1KH) size = lzrw1kh_Decompression(in, out, zsize);
        QUICK_COMP_CASE(LHA_lz5) size = lha_decoder(in, zsize, out, size, "-lz5-");
        QUICK_COMP_CASE(LHA_lzs) size = lha_decoder(in, zsize, out, size, "-lzs-");
        QUICK_COMP_CASE(LHA_lh1) size = lha_decoder(in, zsize, out, size, "-lh1-");
        QUICK_COMP_CASE(LHA_lh4) size = lha_decoder(in, zsize, out, size, "-lh4-");
        QUICK_COMP_CASE(LHA_lh5) size = lha_decoder(in, zsize, out, size, "-lh5-");
        QUICK_COMP_CASE(LHA_lh6) size = lha_decoder(in, zsize, out, size, "-lh6-");
        QUICK_COMP_CASE(LHA_lh7) size = lha_decoder(in, zsize, out, size, "-lh7-");
        QUICK_COMP_CASE(LHA_lhx) size = lha_decoder(in, zsize, out, size, "-lhx-");
        QUICK_COMP_CASE(LHA_pm1) size = lha_decoder(in, zsize, out, size, "-pm1-");
        QUICK_COMP_CASE(LHA_pm2) size = lha_decoder(in, zsize, out, size, "-pm2-");
        QUICK_COMP_CASE(SQX1) size = sqx1_decoder(in, zsize, out, size, 0);
        QUICK_COMP_CASE(MDIP_ARAD)      size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_ARST)      size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_DELTA)     size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_FREQ)      size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_HUFFMAN)   size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_CANONICAL) size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_LZSS)      size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_LZW)       size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_RICE)      size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_RLE)       size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(MDIP_VPACKBITS) size = dipperstein_decompress(in, zsize, out, size, g_compression_type - COMP_MDIP_ARAD);
        QUICK_COMP_CASE(BIZARRE)      size = old_bizarre(in, zsize, out, size, 0);
        QUICK_COMP_CASE(BIZARRE_SKIP) size = old_bizarre(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ASH)
            p = DecryptAsh(in, &t32, size);
            if(p) {
                size = t32;
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
                real_free(p);
            } else {
                size = -1;
            }
        QUICK_COMP_CASE(YAY0) size = Yay0_decodeAll(in, zsize, out);
        QUICK_COMP_CASE(DSTACKER) size = stac_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(DSTACKER_SD3) size = sd3_decomp(in, zsize, out, size, 0);
        QUICK_COMP_CASE(DSTACKER_SD4) size = sd4_decomp(in, zsize, out, size, 0);
        QUICK_COMP_CASE(DBLSPACE) size = ds_dec(in, zsize, out, size, 0x4000);
        QUICK_COMP_CASE(DBLSPACE_JM) size = jm_dec(in, zsize, out, size, 0x4000);
        QUICK_COMP_CASE(DK2)        t32 = *outsize; size = xrefpack(in, zsize, &out, &t32, size, 0); *outsize = t32;
        QUICK_COMP_CASE(XREFPACK)   t32 = *outsize; size = xrefpack(in, zsize, &out, &t32, size, 0); *outsize = t32;
        QUICK_COMP_CASE(XREFPACK0)  t32 = *outsize; size = xrefpack(in, zsize, &out, &t32, size, 1); *outsize = t32;
        QUICK_COMP_CASE(QCMP2) size = UFG__qDecompressLZ(in, zsize, out, size, NULL);
        QUICK_COMP_CASE(DEFLATEX)
            t32 = size;
            size = -1;
            tinf_init();
            if(tinf_uncompress(out, &t32, in, zsize, (g_comtype_dictionary && (g_comtype_dictionary_len >= 3)) ? g_comtype_dictionary : NULL) == TINF_OK) size = t32;
        QUICK_COMP_CASE(ZLIBX)
            t32 = size;
            size = -1;
            tinf_init();
            if(tinf_uncompress(out, &t32, in + 2, zsize - 2, (g_comtype_dictionary && (g_comtype_dictionary_len >= 3)) ? g_comtype_dictionary : NULL) == TINF_OK) size = t32;
        QUICK_COMP_CASE(LZRW1)  perform_lzrw_decompress(1,  0)
        QUICK_COMP_CASE(LZRW1a) perform_lzrw_decompress(1a, 0)
        QUICK_COMP_CASE(LZRW2)  perform_lzrw_decompress(2,  (4096*(sizeof(u8*)+sizeof(u16))) + 100)
        QUICK_COMP_CASE(LZRW3)  perform_lzrw_decompress(3,  (4096*(sizeof(u8*))) + 16)
        QUICK_COMP_CASE(LZRW3a) perform_lzrw_decompress(3a, (4096*(sizeof(u8*))) + 16)
        QUICK_COMP_CASE(LZRW5)  perform_lzrw_decompress(5,  786752)
        QUICK_COMP_CASE(LEGO_IXS) lego_ixs_init(); lego_ixs(in, out);
        QUICK_COMP_CASE(MCOMP)   size = LibMComp(in, zsize, out, size, -1);
        QUICK_COMP_CASE(MCOMP0)  size = LibMComp(in, zsize, out, size,  0);
        QUICK_COMP_CASE(MCOMP1)  size = LibMComp(in, zsize, out, size,  1);
        QUICK_COMP_CASE(MCOMP2)  size = LibMComp(in, zsize, out, size,  2);
        QUICK_COMP_CASE(MCOMP3)  size = LibMComp(in, zsize, out, size,  3);
        QUICK_COMP_CASE(MCOMP4)  size = LibMComp(in, zsize, out, size,  4);
        QUICK_COMP_CASE(MCOMP5)  size = LibMComp(in, zsize, out, size,  5);
        QUICK_COMP_CASE(MCOMP6)  size = LibMComp(in, zsize, out, size,  6);
        QUICK_COMP_CASE(MCOMP7)  size = LibMComp(in, zsize, out, size,  7);
        QUICK_COMP_CASE(MCOMP8)  size = LibMComp(in, zsize, out, size,  8);
        QUICK_COMP_CASE(MCOMP9)  size = LibMComp(in, zsize, out, size,  9);
        QUICK_COMP_CASE(MCOMP10) size = LibMComp(in, zsize, out, size, 10);
        QUICK_COMP_CASE(MCOMP13) size = LibMComp(in, zsize, out, size, 13);
        QUICK_COMP_CASE(MCOMP14) size = LibMComp(in, zsize, out, size, 14);
        QUICK_COMP_CASE(MCOMP15) size = LibMComp(in, zsize, out, size, 15);
        QUICK_COMP_CASE(MCOMP16) size = LibMComp(in, zsize, out, size, 16);
        QUICK_COMP_CASE(MCOMP17) size = LibMComp(in, zsize, out, size, 17);
        QUICK_COMP_CASE(IROLZ)  size = irolz_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(IROLZ2) size = irolz2_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(UCLPACK) size = uclpack(in, zsize, out, size);
        QUICK_COMP_CASE(ACE) size = unace1(in, zsize, out, size);
        QUICK_COMP_CASE(EA_COMP) size = ea_comp(in, zsize, out, size);
        QUICK_COMP_CASE(EA_HUFF) size = ea_huff(in, zsize, out, size);
        QUICK_COMP_CASE(EA_JDLZ) size = ea_jdlz(in, zsize, out, size);
        QUICK_COMP_CASE(TORNADO_BYTE) size = tornado_decompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(TORNADO_BIT) size = tornado_decompress(in, zsize, out, size, 2);
        QUICK_COMP_CASE(TORNADO_HUF) size = tornado_decompress(in, zsize, out, size, 3);
        QUICK_COMP_CASE(TORNADO_ARI) size = tornado_decompress(in, zsize, out, size, 4);
        // TORNADO_ROLZ used in Tornado 0.5 is no longer available and just wastes memory... removed
        QUICK_COMP_CASE(LBALZSS1) size = lbalzss(in, zsize, out, size, 1, 0);
        QUICK_COMP_CASE(LBALZSS2) size = lbalzss(in, zsize, out, size, 2, 0);
        QUICK_COMP_CASE(DBPF) size = maxis_dbpf_uncompress(in, zsize, out, size);
        QUICK_COMP_CASE(TITUS_LZW) size = opentitus_lzw_decode(in, zsize, out, size);
        QUICK_COMP_CASE(TITUS_HUFFMAN) size = opentitus_huffman_decode(in, zsize, out, size);
        QUICK_COMP_CASE(KB_LZW) size = KB_funLZW(out, size, in, zsize);
        QUICK_COMP_CASE(KB_DOSLZW) size = DOS_LZW(out, size, in, zsize);
        QUICK_COMP_CASE(CARMACK) size = CAL_CarmackExpand(in, out, size);
        QUICK_COMP_CASE(MBASH) size = filter_bash_unrle(out, size, in, zsize);
        QUICK_COMP_CASE(DDAVE) size = filter_ddave_unrle(out, size, in, zsize);
        QUICK_COMP_CASE(GOT) size = filter_got_unlzss(out, size, in, zsize);
        QUICK_COMP_CASE(SKYROADS) size = filter_skyroads_unlzs(out, size, in, zsize);
        QUICK_COMP_CASE(ZONE66) size = filter_z66_decompress(out, size, in, zsize);
        QUICK_COMP_CASE(EXEPACK) size = exepack_unpack(out + size, in + zsize);
        QUICK_COMP_CASE(DE_LZW) size = deLZW(in, out, size);
        QUICK_COMP_CASE(JJRLE) size = jazz_jackrabbit_rle(in, zsize, out);
        QUICK_COMP_CASE(K13RLE) size = keen13_rle(in, zsize, out, size);
        QUICK_COMP_CASE(SFRLC) size = sango_fighter_rlc(in, zsize, out, size);
        QUICK_COMP_CASE(WESTWOOD1) size = westwood1(in, zsize, out, size);
        QUICK_COMP_CASE(WESTWOOD3) size = westwood3(in, out, size, 0);
        QUICK_COMP_CASE(WESTWOOD3b) size = westwood3(in, out, size, 1);
        QUICK_COMP_CASE(WESTWOOD40) size = westwood40(in, out);
        QUICK_COMP_CASE(WESTWOOD80) size = westwood80(in, out);
        QUICK_COMP_CASE(PKWARE_DCL) size = pkware_dcl_explode(in, zsize, out, size);
        QUICK_COMP_CASE(TERSE)           size = terse_decompress(in, zsize, out, size,  0, 0);   // header
        QUICK_COMP_CASE(TERSE_SPACK_RAW) size = terse_decompress(in, zsize, out, size, -1, 1);   // PACK binary
        QUICK_COMP_CASE(TERSE_PACK_RAW)  size = terse_decompress(in, zsize, out, size,  1, 1);   // SPACK binary
        QUICK_COMP_CASE(REDUCE1) size = reduce_decompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(REDUCE2) size = reduce_decompress(in, zsize, out, size, 2);
        QUICK_COMP_CASE(REDUCE3) size = reduce_decompress(in, zsize, out, size, 3);
        QUICK_COMP_CASE(REDUCE4) size = reduce_decompress(in, zsize, out, size, 4);
        QUICK_COMP_CASE(LZW_ENGINE)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the LZW_ENGINE decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying the following fields:\n:"
                    "         initial codeword length (in bits)\n"
                    "         maximum codeword length (in bits)\n"
                    "         first valid codeword\n"
                    "         EOF codeword is first codeword\n"
                    "         reset codeword is shared with EOF\n"
                    "         flags (check src/compression/filter-lzw.h)\n");
                myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            tmp1 = tmp2 = tmp3 = tmp4 = tmp5 = tmp6 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, NULL);
            size = camoto_lzw_decompress(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6
            , in, zsize, out, size);
        QUICK_COMP_CASE(LZW_BASH) size = camoto_lzw_decompress(
			9,   // initial codeword length (in bits)
			12,  // maximum codeword length (in bits)
			257, // first valid codeword
			256, // EOF codeword is first codeword
			256, // reset codeword is shared with EOF
			LZW_LITTLE_ENDIAN    | // bits are split into bytes in little-endian order
			LZW_EOF_PARAM_VALID  | // Has codeword reserved for EOF
			LZW_RESET_PARAM_VALID  // Has codeword reserved for dictionary reset
            , in, zsize, out, size);
        QUICK_COMP_CASE(LZW_EPFS) size = camoto_lzw_decompress(
			9,   // initial codeword length (in bits)
			14,  // maximum codeword length (in bits)
			256, // first valid codeword
			0,   // EOF codeword is max codeword
			-1,  // reset codeword is max-1
			LZW_BIG_ENDIAN        | // bits are split into bytes in big-endian order
			LZW_NO_BITSIZE_RESET  | // bitsize doesn't go back to 9 after dict reset
			LZW_EOF_PARAM_VALID   | // Has codeword reserved for EOF
			LZW_RESET_PARAM_VALID   // Has codeword reserved for dict reset
            , in, zsize, out, size);
        QUICK_COMP_CASE(LZW_STELLAR7) size = camoto_lzw_decompress(
			9,   // initial codeword length (in bits)
			12,  // maximum codeword length (in bits)
			257, // first valid codeword
			0,   // EOF codeword is unused
			256, // reset codeword is first codeword
			LZW_LITTLE_ENDIAN     | // bits are split into bytes in little-endian order
			LZW_RESET_PARAM_VALID | // has codeword reserved for dictionary reset
			LZW_FLUSH_ON_RESET      // Jump to next word boundary on dict reset
            , in, zsize, out, size);
        QUICK_COMP_CASE(ULTIMA6) size = ultima6_lzw_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZ5)    // copy from LZ4
            if(g_comtype_dictionary) {
                size = LZ5_decompress_safe_usingDict(in, out, zsize, size, g_comtype_dictionary, g_comtype_dictionary_len);
            } else {
                size = LZ5_decompress_safe_partial(in, out, zsize, size, *outsize);
            }
        QUICK_COMP_CASE(LZ5F) size = lz5f_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(YALZ77) size = yalz77_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZKN1) size = lzkn1_decompress(in, out);
        QUICK_COMP_CASE(LZKN2) size = lzkn2_decompress(in, out);
        QUICK_COMP_CASE(LZKN3) size = lzkn3_decompress(in, out);
        QUICK_COMP_CASE(TFLZSS) size = tropical_freeze_DecompressLZSS(in, zsize, out, size);
        QUICK_COMP_CASE(SYNLZ1) SynLZ_Init(); size = SynLZdecompress1pas(in, zsize, out);
        QUICK_COMP_CASE(SYNLZ1partial) SynLZ_Init(); size = SynLZdecompress1partial(in, zsize, out, size);
        QUICK_COMP_CASE(SYNLZ1b) SynLZ_Init(); size = SynLZdecompress1b(in, zsize, out);
        QUICK_COMP_CASE(SYNLZ2) SynLZ_Init(); size = SynLZdecompress2(in, zsize, out);
        QUICK_COMP_CASE(PPMZ2) size = ppmz2_decode(in, zsize, out, size, g_comtype_dictionary, g_comtype_dictionary_len);
        QUICK_COMP_CASE(OPENDARK) size = opendark_DecodeFile(in, zsize, out, size);
        QUICK_COMP_CASE(DSLZSS) size = dslzss(in, zsize, out, size);
        QUICK_COMP_CASE(KOF) size = _KOFdecompress(in, zsize, out, size, 0);
        QUICK_COMP_CASE(KOF1) size = _KOFdecompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(RFPK) size = rfpk_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(WP16) size = wp16_decompress(in, out, zsize, size);
        QUICK_COMP_CASE(LZ4_STREAM)
            if(g_comtype_dictionary) LZ4_setStreamDecode (g_LZ4_streamDecode, g_comtype_dictionary, g_comtype_dictionary_len);
            size = LZ4_decompress_safe_continue (g_LZ4_streamDecode, in, out, zsize, size);
        QUICK_COMP_CASE(OODLE)          size = myOodleLZ_Decompress(in, zsize, out, size, NULL);
        QUICK_COMP_CASE(OODLE_LZH)      size = myOodleLZ_Decompress(in, zsize, out, size, "LZH");
        QUICK_COMP_CASE(OODLE_LZHLW)    size = myOodleLZ_Decompress(in, zsize, out, size, "LZHLW");
        QUICK_COMP_CASE(OODLE_LZNIB)    size = myOodleLZ_Decompress(in, zsize, out, size, "LZNIB");
        QUICK_COMP_CASE(OODLE_LZB16)    size = myOodleLZ_Decompress(in, zsize, out, size, "LZB16");
        QUICK_COMP_CASE(OODLE_LZBLW)    size = myOodleLZ_Decompress(in, zsize, out, size, "LZBLW");
        QUICK_COMP_CASE(OODLE_LZNA)     size = myOodleLZ_Decompress(in, zsize, out, size, "LZNA");
        QUICK_COMP_CASE(OODLE_BitKnit)  size = myOodleLZ_Decompress(in, zsize, out, size, "BitKnit");
        QUICK_COMP_CASE(OODLE_LZA)      size = myOodleLZ_Decompress(in, zsize, out, size, "LZA");
        QUICK_COMP_CASE(OODLE_LZQ1)     size = myOodleLZ_Decompress(in, zsize, out, size, "Kraken");
        QUICK_COMP_CASE(OODLE_LZNIB2)   size = myOodleLZ_Decompress(in, zsize, out, size, "Mermaid");
        QUICK_COMP_CASE(SEGS) t32 = *outsize; size = segs_decompress(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(OODLE_Selkie)   size = myOodleLZ_Decompress(in, zsize, out, size, "Selkie");
        QUICK_COMP_CASE(OODLE_Akkorokamui)  size = myOodleLZ_Decompress(in, zsize, out, size, "Akkorokamui");
        QUICK_COMP_CASE(ALZ) size = alz_decompress(in, zsize, out, size, 2);
        QUICK_COMP_CASE(REVELATION_ONLINE) Revelation_Online_Decompress_init(); size = Revelation_Online_Decompress(in, out, zsize, size);
        QUICK_COMP_CASE(PS_LZ77) t32 = *outsize; size = ps_lz77_decompress(in, in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(LZFSE) size = lzfse_decode_buffer(out, size, in, zsize, NULL);
        QUICK_COMP_CASE(ZLE) size = zle_decompress(in, out, zsize, size, 0);
        QUICK_COMP_CASE(KOF2) size = KOFdecompress(in, zsize, out, size, 0);
        QUICK_COMP_CASE(KOF3) size = KOFdecompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(HSQ) size = unhsq(in, out);
        QUICK_COMP_CASE(FACT5LZ) size = fact5lz_decompress(in, out);
        QUICK_COMP_CASE(LZCAPTSU) size = LZCapTsu_decompress(in, out);
        QUICK_COMP_CASE(TF3_RLE) size = tf3_rle_decompress(in, out, size);
        QUICK_COMP_CASE(WINIMPLODE) memcpy(out, in, (zsize < size) ? zsize : size); size = winimplode_explode(out);
        QUICK_COMP_CASE(DZIP) size = dzip_decompress(in, zsize, out, size, 0);
        QUICK_COMP_CASE(DZIP_COMBUF) size = dzip_decompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(LBALZSS1X) size = lbalzss(in, zsize, out, size, 1, 1);
        QUICK_COMP_CASE(LBALZSS2X) size = lbalzss(in, zsize, out, size, 2, 1);
        QUICK_COMP_CASE(GHIREN) size = ghiren_decompress(in, out, size);
        QUICK_COMP_CASE(FALCOM_DIN)  size = falcom_din(in, zsize, out, 0);
        QUICK_COMP_CASE(FALCOM_DIN1) size = falcom_din(in, zsize, out, 1);
        QUICK_COMP_CASE(FALCOM_DIN0) size = falcom_din(in, zsize, out, 2);
        QUICK_COMP_CASE(FALCOM_DINX) size = falcom_din(in, zsize, out, 3);
        QUICK_COMP_CASE(GLZA) size = glza_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(M99CODER) size = m99coder(in, zsize, out, 0);
        QUICK_COMP_CASE(LZ4X) size = lz4x(in, zsize, out, 0);
        QUICK_COMP_CASE(TAIKO) size = taiko_decompress(in, zsize, out);
        QUICK_COMP_CASE(LZ77EA_970) size = lz77ea_970(in, zsize, out);
        QUICK_COMP_CASE(DRV3_SRD) size = drv3_srd_dec(in, zsize, out);
        QUICK_COMP_CASE(RECET) size = recetunpack(in, zsize, out, size);
        QUICK_COMP_CASE(LIZARD)    // copy from LZ4
            if(g_comtype_dictionary) {
                size = Lizard_decompress_safe_usingDict(in, out, zsize, size, g_comtype_dictionary, g_comtype_dictionary_len);
            } else {
                size = Lizard_decompress_safe_partial(in, out, zsize, size, *outsize);
            }
        QUICK_COMP_CASE(MICROVISION) size = microvision_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(DR12AE) size = dr_dec(in, zsize, out, size);
        QUICK_COMP_CASE(MSPACK)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the MSPACK decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying the following fields (-1 for default):\n:"
                    "         window_bits\n"
                    "         interval_mode\n"
                    "         reset_interval\n"
                    "         algo: 0:LZX 1:MSZIP 2:QTM  3:LZSS  4:KWAJ/LZH 5:LZX_delta\n");
                myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            tmp1 = tmp2 = tmp3 = tmp4 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, &tmp4, NULL);
            size = unmspack(in, zsize, out, size, tmp1, tmp2, tmp3, tmp4);
        QUICK_COMP_CASE(KONAMIAC) size = konamiac(in, zsize, out, size, 1);
        QUICK_COMP_CASE(WOLF0) size = dewolf(in, zsize, out, size, 0);
        QUICK_COMP_CASE(ARTSTATION) size = artstation_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LEVEL5) t32 = *outsize; size = unlz77wii_level5(in, zsize, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(ZENPXP)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the ZENPXP decompression requires the setting of the dictionary\n"
                    "       field in comtype specifying the algorithm type (1, 2, 3, 0xd and so on)\n");
                myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            zenpxp_decompress(myatoi(g_comtype_dictionary), in, out, size); // must ignore the return value! (probably 0 is success)
        QUICK_COMP_CASE(ZENPXP1) zenpxp_decompress(1, in, out, size);       // must ignore the return value!
        QUICK_COMP_CASE(ZENPXP2) zenpxp_decompress(2, in, out, size);       // must ignore the return value!
        QUICK_COMP_CASE(ZENPXP34) zenpxp_decompress(3, in, out, size);      // must ignore the return value!
        QUICK_COMP_CASE(ZENPXPde) zenpxp_decompress(0xd, in, out, size);    // must ignore the return value!
        QUICK_COMP_CASE(LIBLZS) size = lzs_decompress(out, size, in, zsize);
        QUICK_COMP_CASE(SHREK) size = shrek_decompress(out, size, in, zsize);
        QUICK_COMP_CASE(EA_MADDEN) size = EA_Madden_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(NVCACHE) size = nvcache_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(DE_HTML) myalloc(&out, size *= 2, outsize); size = de_html(in, zsize, out, size);
        QUICK_COMP_CASE(HTML_EASY) myalloc(&out, size *= 2, outsize); size = html_easy(in, zsize, out, size);
        QUICK_COMP_CASE(JSON_VIEWER)
            p = json_viewer(in);
                size = strlen(p);
                myalloc(&out, size, outsize);
                memcpy(out, p, size);
            // do not free p, it's a static buffer!
        QUICK_COMP_CASE(XML_JSON_PARSER)
                xml_json_parser_ctx_t   ctx;
                memset(&ctx, 0, sizeof(ctx));
                xml_json_parser(in, in + zsize, -1, -1, -1, &ctx, NULL);
                size = ctx.names_size;  //mystrdup(&var1, ctx.names);
                myalloc(&out, size, outsize);
                memcpy(out, ctx.names, size);
                xml_json_parser_free(&ctx);
        QUICK_COMP_CASE(OodleNetwork1UDP_State_Uncompact)   // mandatory for OodleNetwork1UDP_Decode
            t32 = *outsize;
            size = myOodleNetwork1UDP_State_Uncompact(in, zsize, &out, &t32);
            *outsize = t32;
            if(size > 0) {
                myalloc(&g_oodlenetwork_state,  size, &g_oodlenetwork_state_size);
                memcpy(g_oodlenetwork_state, out, size);
            }
        QUICK_COMP_CASE(OodleNetwork1_Shared_SetWindow)     // mandatory for OodleNetwork1UDP_Decode
            t32 = *outsize;
            size = myOodleNetwork1_Shared_SetWindow(in, zsize, &out, &t32, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
            *outsize = t32;
            if(size > 0) {
                myalloc(&g_oodlenetwork_shared, size, &g_oodlenetwork_shared_size);
                memcpy(g_oodlenetwork_shared, out, size);
            }
        QUICK_COMP_CASE(OodleNetwork1UDP_Decode) size = myOodleNetwork1UDP_Decode(g_oodlenetwork_state, g_oodlenetwork_shared, in, zsize, out, size);
        QUICK_COMP_CASE(OodleNetwork1UDP_Encode) size = myOodleNetwork1UDP_Encode(g_oodlenetwork_state, g_oodlenetwork_shared, in, zsize, out, size);
        QUICK_COMP_CASE(QCMP1) size = qcmp1_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(YKCMP) size = ykcmp_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZWAB) size = lzwab_compress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(NCOMPRESS) size = ncompress_main(in, zsize, out, size, 1);
        QUICK_COMP_CASE(SWZAP) size = swzap_decompress(in, out);
        QUICK_COMP_CASE(MZX) size = mzx0_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(LZRRV) size = lzrrv_decompress(in, zsize, out);
        QUICK_COMP_CASE(BCM) size = bcm_decompress(in, zsize, out);
        QUICK_COMP_CASE(ULZ) size = ulz_decompress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(SLZ_ROF) size = slz_rof(in, out, zsize, size);
        QUICK_COMP_CASE(LZ4X_NEW) size = lz4x_new(in, zsize, out, 0);
        QUICK_COMP_CASE(SLZ_03b) SLZ3_Decode(in, out);  // no return value, trust size
        QUICK_COMP_CASE(MPPC)       size = mppc_unpack(in, zsize, out, size, 0);
        QUICK_COMP_CASE(MPPC_BIG)   size = mppc_unpack(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ALZSS) size = ALLZ_Decode(&out /*don't worry, doesn't get modified*/, in, zsize);
        QUICK_COMP_CASE(CLZ) size = CLZ__unpack(in, zsize, out);
        QUICK_COMP_CASE(GTC) size = ungtc(in, out);
        QUICK_COMP_CASE(ANCO)  size = anco_unpack (in, zsize, out, size);
        QUICK_COMP_CASE(ANCO0) size = anco_unpack0(in, zsize, out, size);
        QUICK_COMP_CASE(ANCO1) size = anco_unpack1(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ANCO2) size = anco_unpack2(in, zsize, out, size, 1);
        QUICK_COMP_CASE(ANCO3) size = anco_unpack3(in, zsize, out, size);
        QUICK_COMP_CASE(ANCO4) size = anco_unpack4(in, zsize, out, size);
        QUICK_COMP_CASE(ANCO5) size = anco_unpack5(in, zsize, out, size);
        QUICK_COMP_CASE(konami_lz77) size = konami_lz77_decompress(in, zsize, out);
        QUICK_COMP_CASE(vct_lzs) size = vct_lzs_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(umesoft) size = umesoft_unpack(in, out, size);
        QUICK_COMP_CASE(systemaqua_catf) size = systemaqua_catf_unpack(in, out, size);
        QUICK_COMP_CASE(sogna) size = sogna_unpack(in, out, size);
        QUICK_COMP_CASE(pac_ads) size = pac_ads_unpack(in, out, size);
        QUICK_COMP_CASE(ail_lzs) size = ail_lzs_unpack(in, out, size);
        QUICK_COMP_CASE(agsi) size = agsi_unpack(in, out, size);
        QUICK_COMP_CASE(foster_fa2) size = foster_fa2_unpack(in, out, size);
        QUICK_COMP_CASE(an21) size = an21_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(arc_link) size = arc_link_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(maika_bk) size = maika_bk_unpack(in, out, size);
        QUICK_COMP_CASE(maika_mk2) size = maika_mk2_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(propeller_mgr) size = propeller_mgr_unpack(in, out, size);
        QUICK_COMP_CASE(qlie) size = qlie_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(avg32_seen) size = avg32_seen_unpack(in, out, size);
        QUICK_COMP_CASE(sas5_iar) size = sas5_iar_unpack(in, out, size);
        QUICK_COMP_CASE(seraphim_scn) size = seraphim_scn_unpack(in, out, size);
        QUICK_COMP_CASE(ugos_det) size = ugos_det_unpack(in, out, size);
        QUICK_COMP_CASE(aaru_fl4) size = aaru_fl4_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(inspire_ida) size = inspire_ida_unpack(in, out, size);
        QUICK_COMP_CASE(kurumi_mpk) size = kurumi_mpk_unpack(in, out, size);
        QUICK_COMP_CASE(dice_rlz) size = dice_rlz_unpack(in, out, size);
        QUICK_COMP_CASE(pulltop) size = pulltop_unpack(in, out, size);
        QUICK_COMP_CASE(vnsystem) size = vnsystem_unpack(in, out, size);
        QUICK_COMP_CASE(QlzUnpack) size = QlzUnpack(in, out, size);
        QUICK_COMP_CASE(umesoft_pk) size = umesoft_pk_unpack(in, out, size);
        QUICK_COMP_CASE(tomcat_tcd) size = tomcat_tcd_unpack(in, out, size);
        QUICK_COMP_CASE(tail_pren) size = tail_pren_unpack(in, out, size);
        QUICK_COMP_CASE(tail_crp0) size = tail_crp0_unpack(in, out, size);
        QUICK_COMP_CASE(tail_hp) size = tail_hp_unpack(in, out, size);
        QUICK_COMP_CASE(tactics_arc) size = tactics_arc_unpack(in, out, size);
        QUICK_COMP_CASE(sviu_pkz) size = sviu_pkz_unpack(in, out, size);
        QUICK_COMP_CASE(nekox_gpc) size = nekox_gpc_unpack(in, out, size);
        QUICK_COMP_CASE(rec_arc) size = rec_arc_unpack(in, out, size);
        QUICK_COMP_CASE(warc) size = warc_unpack(in, zsize, out);
        QUICK_COMP_CASE(warc10) size = warc10_unpack(in, out, size);
        QUICK_COMP_CASE(warc_ylz) size = warc_ylz_unpack(in, out, size);
        QUICK_COMP_CASE(warc_huff) size = warc_huff_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(sh_him) size = sh_him_unpack(in, out, size);
        QUICK_COMP_CASE(pandora_pbx) size = pandora_pbx_unpack(in, out, size);
        QUICK_COMP_CASE(origin_lz) size = origin_lz_unpack(in, out, size);
        QUICK_COMP_CASE(origin_huffman) size = origin_huffman_unpack(in, out, size);
        QUICK_COMP_CASE(origin_rle) size = origin_rle_unpack(in, out, size);
        QUICK_COMP_CASE(origin_alphav2) size = origin_alphav2_unpack(in, out, size);
        QUICK_COMP_CASE(garbro_huffman) size = garbro_huffman_unpack(in, out, size);
        QUICK_COMP_CASE(ankh_grp) size = ankh_grp_unpack(in, out, size);
        QUICK_COMP_CASE(ankh_hdj) size = ankh_hdj_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(caramelbox_arc3) size = caramelbox_arc3_unpack(in, out, size);
        QUICK_COMP_CASE(caramelbox_arc4) size = caramelbox_arc4_unpack(in, zsize, out);
        QUICK_COMP_CASE(circus_V1) size = circus_V1_unpack(in, out, size);
        QUICK_COMP_CASE(circus_V2) size = circus_V2_unpack(in, out, size);
        QUICK_COMP_CASE(circus_V3) size = circus_V3_unpack(in, out, size);
        QUICK_COMP_CASE(cmvs_cpz) size = cmvs_cpz_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(daisystem_pac) size = daisystem_pac_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(ethornell_bgi) size = ethornell_bgi_unpack(in, out, size);
        QUICK_COMP_CASE(fc01_mrg) size = fc01_mrg_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(fc01_mrg_quant) size = fc01_mrg_quant_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(fc01_pak_lz) size = fc01_pak_lz_unpack(in, out, size);
        QUICK_COMP_CASE(favorite_lzw) size = favorite_lzw_unpack(in, out, size);
        QUICK_COMP_CASE(frontwing_rle) size = frontwing_rle_unpack(in, out, size);
        QUICK_COMP_CASE(frontwing_huffman) size = frontwing_huffman_unpack(in, out, size);
        QUICK_COMP_CASE(g2_gcex) size = g2_gcex_unpack(in, out, size);
        QUICK_COMP_CASE(gss_arc) size = gss_arc_unpack(in, out, size);
        QUICK_COMP_CASE(hypatia_mariel) size = hypatia_mariel_unpack(in, out, size);
        QUICK_COMP_CASE(interheart_fpk) size = interheart_fpk_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(kaguya_ari) size = kaguya_ari_unpack(in, out, size);
        QUICK_COMP_CASE(kaguya_lin2) size = kaguya_lin2_unpack(in, out, size);
        QUICK_COMP_CASE(kaguya_link) size = kaguya_link_unpack(in, out, size, g_comtype_dictionary ? myatoi(g_comtype_dictionary) : -1);
        QUICK_COMP_CASE(kaguya_uf) size = kaguya_uf_unpack(in, out, size);
        QUICK_COMP_CASE(kid_dat) size = kid_dat_unpack(in, out, size);
        QUICK_COMP_CASE(lambda_lax) size = lambda_lax_unpack(in, out, size);
        QUICK_COMP_CASE(microvision_arc) size = microvision_arc_unpack(in, out, size);
        QUICK_COMP_CASE(moonhir_fpk) size = moonhir_fpk_unpack(in, out, size);
        QUICK_COMP_CASE(spack) size = spack_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(azsys) size = azsys_unpack(in, out, size);
        QUICK_COMP_CASE(dxlib) size = dxlib_unpack(in, zsize, out);
        QUICK_COMP_CASE(glibg) size = glibg_unpack(in, out, size);
        QUICK_COMP_CASE(gamesystem_cmp) size = gamesystem_cmp_unpack(in, out, size);
        QUICK_COMP_CASE(puremail) size = puremail_unpack(in, out, size);
        QUICK_COMP_CASE(groover_pcg) size = groover_pcg_unpack(in, zsize, out, size);
        QUICK_COMP_CASE(mnp_mma) size = mnp_mma_unpack(in, out, size);
        QUICK_COMP_CASE(strikes_pck) size = strikes_pck_unpack(in, size, out, size);
        QUICK_COMP_CASE(SEGA_LZ77X) size = sega_lz77(in, zsize, out, 1);
        QUICK_COMP_CASE(NEPTUNIA0) size = neptunia_decompress(in, zsize, out, size);
        QUICK_COMP_CASE(puff8) size = analyze_Huf8(in, out, size);
        QUICK_COMP_CASE(lzh8) size = analyze_LZH8(in, out, size);
        QUICK_COMP_CASE(romchu) size = romchiu(in, zsize, out, size);
        QUICK_COMP_CASE(okage) size = okage_decompress(in, out);
        QUICK_COMP_CASE(lzsd_of)   t32 = *outsize; size = lzsd_of_decompress  (in, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(lzsd_gfd)  t32 = *outsize; size = lzsd_gfd_decompress (in, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(lzsd_gba2) t32 = *outsize; size = lzsd_gba2_decompress(in, &out, &t32); *outsize = t32;
        QUICK_COMP_CASE(pzz) size = PZZ_Decompress(in, out, zsize);
        QUICK_COMP_CASE(SL01) size = sld_decode((void *)in, out);



        /*
        ############
        compressions
        ############
        */

        #define QUICK_COMP_COMPRESS_DEFAULT(X) \
            if(g_quickbms_dll) { \
                /* do nothing for avoiding allocation problems */ \
            } else { \
                size = X + MAXZIPLEN(size); \
                myalloc(&out, size, outsize); \
            }

        QUICK_COMP_CASE(ZLIB_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = libdeflate_compress(15, in, zsize, out, size);
        QUICK_COMP_CASE(DEFLATE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = libdeflate_compress(-15, in, zsize, out, size);
        QUICK_COMP_CASE(LZO1_COMPRESS)  QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1X_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO2A_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1A_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1B_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1C_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1F_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1Y_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZO1Z_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzo_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(XMEMLZX_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = xmem_compress(in, zsize, out, size);
        QUICK_COMP_CASE(BZIP2_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = bzip2_compress(in, zsize, out, size);
        QUICK_COMP_CASE(GZIP_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(20); size = gzip_compress(in, zsize, out, size);
        QUICK_COMP_CASE(LZSS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzss_compress(in, zsize, out, size, g_comtype_dictionary);
        QUICK_COMP_CASE(SFL_BLOCK_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = compress_block(in, out, zsize);
        QUICK_COMP_CASE(SFL_RLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = compress_rle(in, out, zsize);
        QUICK_COMP_CASE(SFL_NULLS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = compress_nulls(in, out, zsize);
        QUICK_COMP_CASE(SFL_BITS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = compress_bits(in, out, zsize);
        QUICK_COMP_CASE(LZF_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzf_compress_best(in, zsize, out, size);
        QUICK_COMP_CASE(BRIEFLZ_COMPRESS)
            if(!g_quickbms_dll) myalloc(&out, size = blz_max_packed_size(size), outsize);
            p = malloc(blz_workmem_size(zsize));
            if(!p) STD_ERR(QUICKBMS_ERROR_MEMORY);
            size = blz_pack(in, out, zsize, p);
            FREE(p);
#ifdef WIN32    // the Linux alternative is using the compiled code directly
        QUICK_COMP_CASE(JCALG_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = JCALG1_Compress(in, zsize, out, 1024 * 1024, &JCALG1_AllocFunc, &JCALG1_DeallocFunc, &JCALG1_CallbackFunc, 0); if(!size) size = -1;
#endif
        QUICK_COMP_CASE(BCL_HUF_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = Huffman_Compress(in, out, zsize);
        QUICK_COMP_CASE(BCL_LZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = LZ_Compress(in, out, zsize);
        QUICK_COMP_CASE(BCL_RICE_COMPRESS)
            if(!g_comtype_dictionary) {
                fprintf(stderr, "\n"
                    "Error: the BCL_RICE decompression requires the setting of the dictionary\n"
                    "       field in comtype with the name of the variable containing the type\n"
                    "       of compression (from 1 to 8, read rice.h), like:\n"
                    "         comtype bcl_rice 1\n");
                //myexit(QUICKBMS_ERROR_BMS);
                size = -1;
                break;
            }
            QUICK_COMP_COMPRESS_DEFAULT(0);
            size = Rice_Compress(in, out, zsize,
                //get_var32(get_var_from_name(g_comtype_dictionary, g_comtype_dictionary_len))
                myatoi(g_comtype_dictionary)
            );
        QUICK_COMP_CASE(BCL_RLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = RLE_Compress(in, zsize, out, size);
        QUICK_COMP_CASE(BCL_SF_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = SF_Compress(in, out, zsize);
        QUICK_COMP_CASE(SZIP_COMPRESS)
            // not implemented and totally useless, currently it returns error
            QUICK_COMP_COMPRESS_DEFAULT(0);
            t_size = size;
            size = -1;
            if(!SZ_BufftoBuffCompress(out, &t_size, in, zsize, NULL)) size = t_size;
        QUICK_COMP_CASE(HUFFMANLIB_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            t32 = size;
            size = -1;
            p = NULL;
            if(!huffman_encode_memory(in, zsize, &p, &t32)) {
                if(!g_quickbms_dll) myalloc(&out, size = t32, outsize);
                memcpy(out, p, size);
                real_free(p);
            }
        QUICK_COMP_CASE(LZMA_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_NONE);
        QUICK_COMP_CASE(LZMA_86HEAD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(8); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_86_HEADER);
        QUICK_COMP_CASE(LZMA_86DEC_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_86_DECODER);
        QUICK_COMP_CASE(LZMA_86DECHEAD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1 + 8); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER);
        QUICK_COMP_CASE(LZMA_EFS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(4); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_EFS);
        QUICK_COMP_CASE(FALCOM_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = falcom_EncodeData(out, size, in, zsize);
        QUICK_COMP_CASE(KZIP_ZLIB_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = uberflate(in, zsize, out, size, 1);
        QUICK_COMP_CASE(KZIP_DEFLATE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = uberflate(in, zsize, out, size, 0);
        QUICK_COMP_CASE(ZOPFLI_ZLIB_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            size = -1;
            t32 = 0;
            p = myzopfli(in, zsize, &t32, ZOPFLI_FORMAT_ZLIB, g_reimport ? 0 : 1);
            if(p) {
                if(!g_quickbms_dll) myalloc(&out, size = t32, outsize);
                memcpy(out, p, size);
                real_free(p);   // p is returned by the lib, not unz.c
            }
        QUICK_COMP_CASE(ZOPFLI_DEFLATE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            size = -1;
            t32 = 0;
            p = myzopfli(in, zsize, &t32, ZOPFLI_FORMAT_DEFLATE, g_reimport ? 0 : 1);
            if(p) {
                if(!g_quickbms_dll) myalloc(&out, size = t32, outsize);
                memcpy(out, p, size);
                real_free(p);   // p is returned by the lib, not unz.c
            }
        QUICK_COMP_CASE(PRS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(((9 * zsize) / 8) + 2); size = prs_compress(in, out, zsize);
        QUICK_COMP_CASE(RNC_COMPRESS) // QUICK_COMP_COMPRESS_DEFAULT(0);
            long plen;
            size = -1;
            p = rnc_pack(in, zsize, &plen);
            if(p) {
                if(!g_quickbms_dll) myalloc(&out, size = plen, outsize);
                memcpy(out, p, size);
                real_free(p);
            }
        QUICK_COMP_CASE(LZ4_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = LZ4_compress_HC(in, out, zsize, size, 9 /*LZ4HC_CLEVEL_MAX is too slow*/); //size = LZ4_compress_default(in, out, zsize, size);
        QUICK_COMP_CASE(SFL_BLOCK_CHUNKED_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = sfl_block_chunked_compress(in, zsize, out, size);
        QUICK_COMP_CASE(SNAPPY_COMPRESS)
            QUICK_COMP_COMPRESS_DEFAULT(
                snappy_max_compressed_length(zsize)
            );
            size_t_tmp = size;
            size = -1;
            if(snappy_compress(in, zsize, out, &size_t_tmp) == SNAPPY_OK) size = size_t_tmp;
        QUICK_COMP_CASE(ZPAQ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = zpaq_compress(in, zsize, out, size);
        QUICK_COMP_CASE(BLOSC_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1024 /*???*/); size = blosclz_compress(9, in, zsize, out, size);
        QUICK_COMP_CASE(GIPFELI_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = gipfeli_compress(in, zsize, out, size);
        QUICK_COMP_CASE(YAPPY_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = yappy_compress(in, zsize, out, size, -1);
        QUICK_COMP_CASE(LZG_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = LZG_Encode(in, zsize, out, size, NULL);
        QUICK_COMP_CASE(DOBOZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = doboz_compress(in, zsize, out, size);
        QUICK_COMP_CASE(NITROSDK_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = nitroCompress(in, zsize, out, "h8" /*"l255" blah, bigger than uncompressed!*/, 0);
        QUICK_COMP_CASE(HEX_COMPRESS)
            if(!g_quickbms_dll) myalloc(&out, (size = zsize * 2 /*zsize is ok here*/) + 1, outsize);
            for(t32 = 0; t32 < zsize; t32++) {
                sprintf(out + (t32 * 2), "%02x", in[t32]);
            }
            out[size] = 0;
        QUICK_COMP_CASE(BASE64_COMPRESS)
            if(!g_quickbms_dll) myalloc(&out, size = ((zsize / 3) * 4) + 6 /* zsize is ok here */, outsize);
            size = mybase64_encode(in, zsize, out, size);
        QUICK_COMP_CASE(LZMA2_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_NONE);
        QUICK_COMP_CASE(LZMA2_86HEAD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_86_HEADER);
        QUICK_COMP_CASE(LZMA2_86DEC_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_86_DECODER);
        QUICK_COMP_CASE(LZMA2_86DECHEAD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1 + 8); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER);
        QUICK_COMP_CASE(LZMA2_EFS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(4); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_EFS);
        QUICK_COMP_CASE(LZMA_0_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzma_compress(in, zsize, out, size, LZMA_FLAGS_PROP0 | LZMA_FLAGS_NONE);
        QUICK_COMP_CASE(LZMA2_0_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzma2_compress(in, zsize, out, size, LZMA_FLAGS_PROP0 | LZMA_FLAGS_NONE);
        QUICK_COMP_CASE(STORMHUFF_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = StormLib_Compress_huff(out, size, in, zsize);
        QUICK_COMP_CASE(CT_HughesTransform_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(1024); size = shadowforce_compress(CT_HughesTransform, in, zsize, out, size);
        QUICK_COMP_CASE(CT_LZ77_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = shadowforce_compress(CT_LZ77, in, zsize, out, size);
        QUICK_COMP_CASE(CT_ELSCoder_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = shadowforce_compress(CT_ELSCoder, in, zsize, out, size);
        QUICK_COMP_CASE(CT_RefPack_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = shadowforce_compress(CT_RefPack, in, zsize, out, size);
        QUICK_COMP_CASE(DK2_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);   // same as QFS_COMPRESS
            t32 = zsize;
            qfs_compress_data(in, &t32, out);
            size = t32;
        QUICK_COMP_CASE(QFS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            t32 = zsize;
            qfs_compress_data(in, &t32, out);
            size = t32;
        QUICK_COMP_CASE(LZHUFXR_COMPRESS)
            p = NULL;
            t32 = size;
            _compressLZ(&p, &t32, in, zsize);
            if(p) {
                if(!g_quickbms_dll) myalloc(&out, size = t32, outsize);
                memcpy(out, p, size);
                real_free(p);
            }
        QUICK_COMP_CASE(FSE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(FSE_compressBound(zsize)); size = FSE_compress(out, size, in, zsize); if(size <= 1) size = -1;
        QUICK_COMP_CASE(ZSTD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(ZSTD_compressBound(zsize)); size = myzstd_compress(out, size, in, zsize, g_comtype_dictionary, g_comtype_dictionary_len, /*ZSTD_MAX_CLEVEL*/ 20);
        QUICK_COMP_CASE(DS_BLZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); BLZ_Encode("", 1/*BLZ_BEST*/);                                        size = nintendo_ds_set_inout(NULL, 0, NULL, 0); // sucks at compression
        QUICK_COMP_CASE(DS_HUF_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); HUF_Encode("", 0x20/*CMD_CODE_20*/);                                  size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_LZE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); LZE_Encode("");                                                       size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_LZS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); LZS_Encode("", 0x40/*LZS_BEST*/);                                     size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(DS_LZX_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); LZX_Encode("", 0x40/*CMD_CODE_11/CMD_CODE_40(best)*/, 1/*LZX_VRAM*/); size = nintendo_ds_set_inout(NULL, 0, NULL, 0); // may return -1 with some files
        QUICK_COMP_CASE(DS_RLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); nintendo_ds_set_inout(in, zsize, out, size); RLE_Encode("");                                                       size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
        QUICK_COMP_CASE(HEATSHRINK_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = heatshrink_compress(in, zsize, out, size);
        QUICK_COMP_CASE(SMAZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = smaz_compress(in, zsize, out, size);
        QUICK_COMP_CASE(LZFX_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            t32 = size;
            if(lzfx_compress(in, zsize, out, &t32, 16 + (5-1)) < 0) size = -1;
            else size = t32;
        QUICK_COMP_CASE(PITHY_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = mypithy_Compress(in, zsize, out, size);
        QUICK_COMP_CASE(ZLING_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = zling_compress(in, zsize, out, size);
        QUICK_COMP_CASE(DENSITY_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            density_processing_result   density_resc;
            density_resc = density_compress(in, zsize, out, size, DENSITY_ALGORITHM_LION);
            size = density_resc.bytesWritten;
        QUICK_COMP_CASE(BSC_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(LIBBSC_HEADER_SIZE);
            bsc_init(LIBBSC_FEATURE_NONE);
            size = bsc_compress(in, out, zsize, 16, 128, LIBBSC_BLOCKSORTER_BWT, LIBBSC_CODER_QLFC_STATIC, LIBBSC_FEATURE_NONE);
        QUICK_COMP_CASE(SHOCO_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = shoco_compress(in, zsize, out, size); // shoco works only with strings!!!
        QUICK_COMP_CASE(WFLZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); p = malloc(wfLZ_GetWorkMemSize()); size = wfLZ_Compress(in, zsize, out, p, 0); FREE(p);
        QUICK_COMP_CASE(FASTARI_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); t_size = size; fa_compress(in, out, zsize, &t_size); size = t_size;
        QUICK_COMP_CASE(DICKY_COMPRESS)
            p = NULL;
            t_size = 0;
            if(!dicky_compress(&p, &t_size, in, zsize)) {
                if(!g_quickbms_dll) myalloc(&out, size = t_size, outsize);
                memcpy(out, p, size);
            } else {
                size = -1;
            }
            dicky_free(p);
        QUICK_COMP_CASE(SQUISH_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = squish_pack(in, zsize, out, size);
        QUICK_COMP_CASE(LZHL_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            p = lzhlight_initComp();
            size = lzhlight_compress(p, in, zsize, out);
            lzhlight_delComp(p);
        QUICK_COMP_CASE(LZHAM_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzham_pack(in, zsize, out, size, g_comtype_dictionary);
        QUICK_COMP_CASE(TRLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = trlec(in, zsize, out);
        QUICK_COMP_CASE(SRLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = srlec(in, zsize, out);
        QUICK_COMP_CASE(MRLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = mrlec(in, zsize, out);
        QUICK_COMP_CASE(CPK_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = CompressLAYLA(in, zsize, out, size);
        QUICK_COMP_CASE(LZRW1KH_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzrw1kh_Compression(in, out, zsize);
        QUICK_COMP_CASE(BPE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = bpe_compress(in, zsize, out, size, 0);
        QUICK_COMP_CASE(NRV2b_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ucl_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(NRV2d_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ucl_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(NRV2e_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ucl_compress(in, zsize, out, size, g_compression_type);
        QUICK_COMP_CASE(LZSS0_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzss_compress(in, zsize, out, size, "12 4 2 2 0");
        QUICK_COMP_CASE(CLZW_COMPRESS)
            QUICK_COMP_COMPRESS_DEFAULT(0);
            lzw_enc_t  *clzwe_ctx;
            clzwe_ctx = calloc(1, sizeof(lzw_enc_t));
            if(!clzwe_ctx) STD_ERR(QUICKBMS_ERROR_MEMORY);
            clzw_outsz = 0;
            lzw_enc_init(clzwe_ctx, out);
            lzw_encode(clzwe_ctx, in, zsize);
            size = clzw_outsz;
            FREE(clzwe_ctx);
            if(!size) size = -1;
        QUICK_COMP_CASE(QUICKLZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = doquicklz(in, zsize, out, size);
        QUICK_COMP_CASE(PKWARE_DCL_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = pkware_dcl_implode(in, zsize, out, size);
        QUICK_COMP_CASE(LZ5_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = LZ5_compress_HC(in, out, zsize, size, 16);
        QUICK_COMP_CASE(YALZ77_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = yalz77_compress(in, zsize, out, size);
        QUICK_COMP_CASE(SYNLZ1_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); SynLZ_Init(); size = SynLZcompress1pas(in, zsize, out);
        QUICK_COMP_CASE(SYNLZ2_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); SynLZ_Init(); size = SynLZcompress2(in, zsize, out);
        QUICK_COMP_CASE(PPMZ2_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ppmz2_encode(in, zsize, out, size, g_comtype_dictionary, g_comtype_dictionary_len);
        QUICK_COMP_CASE(EA_JDLZ_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(16); size = jdlz_compress(in, zsize, out);
        QUICK_COMP_CASE(OODLE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = myOodleLZ_Compress(in, zsize, out);
        QUICK_COMP_CASE(LZFSE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzfse_encode_buffer(out, size, in, zsize, NULL);
        QUICK_COMP_CASE(M99CODER_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = m99coder(in, zsize, out, 1);
        QUICK_COMP_CASE(LZ4X_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lz4x(in, zsize, out, 1);
        QUICK_COMP_CASE(YUKE_BPE_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = bpe_compress(in, zsize, out, size, 1);
        QUICK_COMP_CASE(LIZARD_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = Lizard_compress(in, out, zsize, size, 16);
        QUICK_COMP_CASE(LIBLZS_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lzs_compress(out, size, in, zsize);
        QUICK_COMP_CASE(PRS_8ING_COMPRESS)  QUICK_COMP_COMPRESS_DEFAULT(0); size = prs_8ing_compress(in, zsize, out);
        QUICK_COMP_CASE(DIZZY_COMPRESS)  QUICK_COMP_COMPRESS_DEFAULT(0); size = dizzy_compress(in, zsize, out);
        QUICK_COMP_CASE(LEVEL5_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(4); size = lz77wii_level5_compress(in, zsize, out, size);
        QUICK_COMP_CASE(BROTLI_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            t_size = size;
            if(BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, zsize, in, &t_size, out) != BROTLI_TRUE) size = -1;
            else size = t_size;
        QUICK_COMP_CASE(DRV3_SRD_COMPRESS)      QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = drv3_srd_enc_fake(in, zsize, out);
        QUICK_COMP_CASE(YAZ0_COMPRESS)          QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = encodeYaz0_fake(in, zsize, out);
        QUICK_COMP_CASE(BIZARRE_COMPRESS)       QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = old_bizarre_compress_fake(in, zsize, out, 0);
        QUICK_COMP_CASE(BIZARRE_SKIP_COMPRESS)  QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = old_bizarre_compress_fake(in, zsize, out, 1);
        QUICK_COMP_CASE(BLACKDESERT_COMPRESS)   QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = blackdesert_pack_fake(in, zsize, out);
        QUICK_COMP_CASE(DR12AE_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = dr_enc_fake(in, zsize, out);
        QUICK_COMP_CASE(EA_COMP_COMPRESS)       QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = ea_comp_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(LBALZSS_COMPRESS)       QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = lbalzss_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(MOHLZSS_COMPRESS)       QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = moh_lzss_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(MOHRLE_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = moh_rle_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(NISLZS_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = nislzs_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(QCMP1_COMPRESS)         QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = qcmp1_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(RFPK_COMPRESS)          QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = rfpk_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(RLEW_COMPRESS)          QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = unrlew_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(SAINT_SEYA_COMPRESS)    QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = Model_GMI_Compress_fake(in, zsize, out);
        QUICK_COMP_CASE(SHREK_COMPRESS)         QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = shrek_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(SLZ_01_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = slz_triace_compress_fake(in, zsize, out, 1);
        QUICK_COMP_CASE(SLZ_02_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = slz_triace_compress_fake(in, zsize, out, 2);
        QUICK_COMP_CASE(SLZ_03_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = slz_triace_compress_fake(in, zsize, out, 3);
        QUICK_COMP_CASE(UCLPACK_COMPRESS)       QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = uclpack_compress(in, zsize, out);   // both fake and not fake
        QUICK_COMP_CASE(SEGA_LZS2_COMPRESS)     QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = sega_lzs2_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(WOLF_COMPRESS)          QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = dewolf_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(YAKUZA_COMPRESS)        QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = unyakuza_compress(in, zsize, out);
        QUICK_COMP_CASE(YKCMP_COMPRESS)         QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = ykcmp_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(LZWAB_COMPRESS)         QUICK_COMP_COMPRESS_DEFAULT(0); size = lzwab_compress(in, zsize, out, size, 0);
        QUICK_COMP_CASE(NCOMPRESS_COMPRESS)     QUICK_COMP_COMPRESS_DEFAULT(0); size = ncompress_main(in, zsize, out, size, 0);
        QUICK_COMP_CASE(UNCOMPRESS_COMPRESS)    QUICK_COMP_COMPRESS_DEFAULT(0); size = ncompress_main(in, zsize, out, size, 0); if(size > 3) mymemmove(out, out + 3, size -= 3);
        QUICK_COMP_CASE(LZ4X_NEW_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lz4x_new(in, zsize, out, 1);
        QUICK_COMP_CASE(DRAGONBALLZ_COMPRESS)   QUICK_COMP_COMPRESS_DEFAULT(zsize /*fake*/); size = undragonballz_compress_fake(in, zsize, out);
        QUICK_COMP_CASE(MPPC_COMPRESS)      QUICK_COMP_COMPRESS_DEFAULT(0); size = mppc_pack(in, zsize, out, size, 0);
        QUICK_COMP_CASE(MPPC_BIG_COMPRESS)  QUICK_COMP_COMPRESS_DEFAULT(0); size = mppc_pack(in, zsize, out, size, 1);
        QUICK_COMP_CASE(APLIB_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = aplib_compress(in, zsize, out);
        QUICK_COMP_CASE(okage_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = okage_compress(in, zsize, out);
        QUICK_COMP_CASE(LZ4F_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lz4f_compress(in, zsize, out, size);
        QUICK_COMP_CASE(LZ5F_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = lz4f_compress(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDH_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ppmdh_compress(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDI_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0); size = ppmdi_compress(in, zsize, out, size);
        QUICK_COMP_CASE(PPMDH_RAW_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            tmp1 = tmp2 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, NULL);
            size = ppmdh_compress_raw(in, zsize, out, size, tmp1, tmp2);
        QUICK_COMP_CASE(PPMDI_RAW_COMPRESS) QUICK_COMP_COMPRESS_DEFAULT(0);
            tmp1 = tmp2 = tmp3 = 0;
            get_parameter_numbers(g_comtype_dictionary, &tmp1, &tmp2, &tmp3, NULL);
            size = ppmdi_compress_raw(in, zsize, out, size, tmp1, tmp2, tmp3);

        /*QUICK_COMP_CASE last break*/ break;
        default: {
            fprintf(stderr, "\nError: unsupported compression type %d\n", (i32)g_compression_type);
            break;
        }
    }
    *ret_out = out;

    if(size < 0) {
        fprintf(stderr,
            "Info:  algorithm   %"PRId"\n"
            "       offset      %"PRIx"\n"
            "       input size  0x%"PRIx" %"PRIu"\n"
            "       output size 0x%"PRIx" %"PRIu"\n"
            "       result      0x%"PRIx" %"PRId"\n",
            g_compression_type,
            offset_info,
            zsize, zsize,
            bck_size, bck_size,
            size, size);
    }

    //if(size <= 0) { // zero too because some algorithms like oodle return zero for errors
    if(size < 0) {  // who cares, if I have a 0-bytes file compressed with zlib I want the original empty files and not an error
        if(g_ignore_comp_errors) {  // it's important to display the warning message for debugging purposes
            size = bck_size;
            if(size > *outsize) size = *outsize;
        }
    }

    if(*outsize == old_outsize) {   // the check is made on those algorithms that don't reallocate out
        if((u64)size > (u64)*outsize) {       // "limit" possible overflows with some unsafe algorithms (like sflcomp)
            fprintf(stderr, "\n"
                "Error: uncompressed data (%"PRId") bigger than allocated buffer (%"PRId")\n"
                "       It usually means that data is not compressed or uses another algorithm\n",
                size, *outsize);
            myexit(QUICKBMS_ERROR_COMPRESSION);
            size = -1;  // in case myexit() doesn't terminate, for example via ipc()
        }
    }
    return size;
}


