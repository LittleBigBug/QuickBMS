#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              dicing_ECRYPT_ctx
#define ECRYPT_init             dicing_ECRYPT_init
#define ECRYPT_keysetup         dicing_ECRYPT_keysetup
#define ECRYPT_ivsetup          dicing_ECRYPT_ivsetup
#define ECRYPT_process_bytes    dicing_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    dicing_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    dicing_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  dicing_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   dicing_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   dicing_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   dicing_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   dicing_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   dicing_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   dicing_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks dicing_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/dicing/v2/dicing-v2.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(dicing_ECRYPT)
