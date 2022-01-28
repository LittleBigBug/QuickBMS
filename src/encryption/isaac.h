#include <stdint.h>

typedef struct {
    uint32_t    A,
                B,
                C,
                position,
                state[256];
    uint8_t     buffer[256 * 4];
} ISAAC_ctx_t;

void ISAAC__cipher(ISAAC_ctx_t *ISAAC_ctx, const uint8_t *in, uint8_t *out, uint32_t length);
void ISAAC__key(ISAAC_ctx_t *ISAAC_ctx, const uint8_t *key, uint32_t length);
