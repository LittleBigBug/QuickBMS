typedef struct 
{
  u32 keyschedule[32];
} SEED_context;

int do_seed_setkey (SEED_context *ctx, const byte *key, const unsigned keylen);
void do_seed_encrypt (const SEED_context *ctx, byte *outbuf, const byte *inbuf);
void do_seed_decrypt (SEED_context *ctx, byte *outbuf, const byte *inbuf);
