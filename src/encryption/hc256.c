#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              hc256_ECRYPT_ctx
#define ECRYPT_init             hc256_ECRYPT_init
#define ECRYPT_keysetup         hc256_ECRYPT_keysetup
#define ECRYPT_ivsetup          hc256_ECRYPT_ivsetup
#define ECRYPT_process_bytes    hc256_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    hc256_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    hc256_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  hc256_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   hc256_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   hc256_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   hc256_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   hc256_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   hc256_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   hc256_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks hc256_ECRYPT_keystream_blocks

#define generate_keystream  hc256_generate_keystream
#define setup_update        hc256_setup_update

#include "../libs/ecrypt/submissions/hc-256/hc-256/200701/hc-256.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(hc256_ECRYPT)
