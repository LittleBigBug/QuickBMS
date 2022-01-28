#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              grain128_ECRYPT_ctx
#define ECRYPT_init             grain128_ECRYPT_init
#define ECRYPT_keysetup         grain128_ECRYPT_keysetup
#define ECRYPT_ivsetup          grain128_ECRYPT_ivsetup
#define ECRYPT_process_bytes    grain128_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    grain128_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    grain128_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  grain128_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   grain128_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   grain128_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   grain128_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   grain128_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   grain128_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   grain128_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks grain128_ECRYPT_keystream_blocks

#define grain_keystream         grain128_keystream

#include "../libs/ecrypt/submissions/grain/128/ref/grain-128.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(grain128_ECRYPT)
