#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              salsa20_ECRYPT_ctx
#define ECRYPT_init             salsa20_ECRYPT_init
#define ECRYPT_keysetup         salsa20_ECRYPT_keysetup
#define ECRYPT_ivsetup          salsa20_ECRYPT_ivsetup
#define ECRYPT_process_bytes    salsa20_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    salsa20_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    salsa20_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  salsa20_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   salsa20_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   salsa20_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   salsa20_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   salsa20_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   salsa20_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   salsa20_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks salsa20_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/salsa20/full/ref/salsa20.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(salsa20_ECRYPT)
