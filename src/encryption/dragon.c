#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              dragon_ECRYPT_ctx
#define ECRYPT_init             dragon_ECRYPT_init
#define ECRYPT_keysetup         dragon_ECRYPT_keysetup
#define ECRYPT_ivsetup          dragon_ECRYPT_ivsetup
#define ECRYPT_process_bytes    dragon_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    dragon_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    dragon_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  dragon_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   dragon_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   dragon_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   dragon_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   dragon_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   dragon_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   dragon_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks dragon_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/dragon/dragon.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(dragon_ECRYPT)
