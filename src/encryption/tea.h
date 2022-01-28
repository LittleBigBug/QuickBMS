#include <inttypes.h>

#define TEA_ENCRYPT     1
#define TEA_DECRYPT     0

typedef struct {
    uint32_t k[4];
    uint32_t delta;
    uint32_t sum;
    uint32_t endian;
    uint32_t cycles;
    int inverted_sign;
}
tea_context;

#ifdef __cplusplus
extern "C" {
#endif
void tea_setup(tea_context *ctx, unsigned char key[16], int custom, uint32_t delta, uint32_t sum, uint32_t endian, uint32_t cycles, int inverted_sign, int encrypt_mode);
void tea_setup_delta(tea_context *ctx, uint32_t delta, uint32_t sum);
void tea_crypt(tea_context *ctx, int mode, unsigned char input[8], unsigned char output[8]);
#ifdef __cplusplus
}
#endif
