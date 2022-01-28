#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              abc_ECRYPT_ctx
#define ECRYPT_init             abc_ECRYPT_init
#define ECRYPT_keysetup         abc_ECRYPT_keysetup
#define ECRYPT_ivsetup          abc_ECRYPT_ivsetup
#define ECRYPT_process_bytes    abc_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    abc_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    abc_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  abc_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   abc_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   abc_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   abc_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   abc_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   abc_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   abc_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks abc_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/abc/v3/abc-v3.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(abc_ECRYPT)
