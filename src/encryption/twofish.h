typedef struct {
   u32 s[4][256], w[8], k[32];
} TWOFISH_context;

int do_twofish_setkey (TWOFISH_context *ctx, const byte *key, unsigned int keylen);
void do_twofish_encrypt (const TWOFISH_context *ctx, byte *out, const byte *in);
void do_twofish_decrypt (const TWOFISH_context *ctx, byte *out, const byte *in);
