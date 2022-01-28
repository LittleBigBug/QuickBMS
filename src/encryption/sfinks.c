#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              sfinks_ECRYPT_ctx
#define ECRYPT_init             sfinks_ECRYPT_init
#define ECRYPT_keysetup         sfinks_ECRYPT_keysetup
#define ECRYPT_ivsetup          sfinks_ECRYPT_ivsetup
#define ECRYPT_process_bytes    sfinks_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    sfinks_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    sfinks_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  sfinks_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   sfinks_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   sfinks_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   sfinks_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   sfinks_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   sfinks_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   sfinks_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks sfinks_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/sfinks/sync/sfinks.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(sfinks_ECRYPT)
