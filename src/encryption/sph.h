typedef struct {
    unsigned char   *algo;
    void            *cc;
    int             sph_function_size;
    void (*sph_function_init) (void *cc);
    void (*sph_function) (void *cc, const void *data, size_t len);
    void (*sph_function_close) (void *cc, void *dst);
} sph_context;



int sph(sph_context *ctx, unsigned char *data, int len, unsigned char *digest);

