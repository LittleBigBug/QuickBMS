#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              rabbit_ECRYPT_ctx
#define ECRYPT_init             rabbit_ECRYPT_init
#define ECRYPT_keysetup         rabbit_ECRYPT_keysetup
#define ECRYPT_ivsetup          rabbit_ECRYPT_ivsetup
#define ECRYPT_process_bytes    rabbit_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    rabbit_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    rabbit_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  rabbit_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   rabbit_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   rabbit_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   rabbit_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   rabbit_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   rabbit_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   rabbit_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks rabbit_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/rabbit/ref/rabbit.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(rabbit_ECRYPT)
