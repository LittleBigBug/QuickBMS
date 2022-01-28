#include <stdint.h>



typedef struct {
    uint8_t     bitchr;
    uint8_t     bitpos;
    uint8_t     *data;
    uint8_t     *data_start;
    uint8_t     *data_limit;
    FILE        *fd;
    int         eof;
    int         force_endian;
} mybits_ctx_t;



#ifdef __cplusplus
extern "C" {
#endif

    void mybits_init(mybits_ctx_t *ctx, uint8_t *data, int size, FILE *fd);

    uint64_t mybits_read(mybits_ctx_t *ctx, uint32_t bits, int endian);

    int mybits_write(mybits_ctx_t *ctx, uint64_t num, uint32_t bits, int endian);

    void mybits_flush(mybits_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
