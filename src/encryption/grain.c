#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              grain_ECRYPT_ctx
#define ECRYPT_init             grain_ECRYPT_init
#define ECRYPT_keysetup         grain_ECRYPT_keysetup
#define ECRYPT_ivsetup          grain_ECRYPT_ivsetup
#define ECRYPT_process_bytes    grain_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    grain_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    grain_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  grain_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   grain_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   grain_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   grain_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   grain_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   grain_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   grain_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks grain_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/grain/v1/ref/grain-v1.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(grain_ECRYPT)
