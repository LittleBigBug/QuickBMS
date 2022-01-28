#define WORDS_PER_SEAL_CALL 1024
         
typedef struct {
	unsigned long t[520]; /* 512 rounded up to a multiple of 5 + 5*/
	unsigned long s[265]; /* 256 rounded up to a multiple of 5 + 5*/
	unsigned long r[20]; /* 16 rounded up to multiple of 5 */
	unsigned long counter; /* 32-bit synch value. */
	unsigned long ks_buf[WORDS_PER_SEAL_CALL];
	int ks_pos;
} seal_ctx_t;

void seal_key(seal_ctx_t *c, unsigned char *key);
void seal_encrypt(seal_ctx_t *c, unsigned long *data_ptr, int w);
