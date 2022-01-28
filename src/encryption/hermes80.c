#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              hermes80_ECRYPT_ctx
#define ECRYPT_init             hermes80_ECRYPT_init
#define ECRYPT_keysetup         hermes80_ECRYPT_keysetup
#define ECRYPT_ivsetup          hermes80_ECRYPT_ivsetup
#define ECRYPT_process_bytes    hermes80_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    hermes80_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    hermes80_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  hermes80_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   hermes80_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   hermes80_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   hermes80_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   hermes80_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   hermes80_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   hermes80_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks hermes80_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/hermes/hermes8-80/hermes.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(hermes80_ECRYPT)
