#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              hc128_ECRYPT_ctx
#define ECRYPT_init             hc128_ECRYPT_init
#define ECRYPT_keysetup         hc128_ECRYPT_keysetup
#define ECRYPT_ivsetup          hc128_ECRYPT_ivsetup
#define ECRYPT_process_bytes    hc128_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    hc128_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    hc128_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  hc128_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   hc128_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   hc128_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   hc128_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   hc128_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   hc128_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   hc128_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks hc128_ECRYPT_keystream_blocks

#define generate_keystream  hc128_generate_keystream
#define setup_update        hc128_setup_update

#include "../libs/ecrypt/submissions/hc-256/hc-128/200701b/hc-128.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(hc128_ECRYPT)
