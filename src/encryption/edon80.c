#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              edon80_ECRYPT_ctx
#define ECRYPT_init             edon80_ECRYPT_init
#define ECRYPT_keysetup         edon80_ECRYPT_keysetup
#define ECRYPT_ivsetup          edon80_ECRYPT_ivsetup
#define ECRYPT_process_bytes    edon80_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    edon80_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    edon80_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  edon80_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   edon80_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   edon80_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   edon80_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   edon80_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   edon80_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   edon80_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks edon80_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/edon80/edon80.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(edon80_ECRYPT)
