#include <inttypes.h>

#define XXTEA_ENCRYPT   1
#define XXTEA_DECRYPT   0

typedef struct {
    uint32_t k[4];
    uint32_t delta;
    uint32_t endian;
    uint32_t cycles;
    int inverted_sign;
}
xxtea_context;

#ifdef __cplusplus
extern "C" {
#endif
void xxtea_setup(xxtea_context *ctx, unsigned char key[16], int custom, uint32_t delta, uint32_t endian, uint32_t cycles, int inverted_sign);
void xxtea_setup_delta(xxtea_context *ctx, uint32_t delta);
void xxtea_crypt(xxtea_context *ctx, int mode, unsigned char *data, int len);
#ifdef __cplusplus
}
#endif
