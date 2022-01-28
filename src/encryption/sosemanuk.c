#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              sosemanuk_ECRYPT_ctx
#define ECRYPT_init             sosemanuk_ECRYPT_init
#define ECRYPT_keysetup         sosemanuk_ECRYPT_keysetup
#define ECRYPT_ivsetup          sosemanuk_ECRYPT_ivsetup
#define ECRYPT_process_bytes    sosemanuk_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    sosemanuk_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    sosemanuk_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  sosemanuk_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   sosemanuk_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   sosemanuk_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   sosemanuk_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   sosemanuk_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   sosemanuk_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   sosemanuk_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks sosemanuk_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/sosemanuk/sosemanuk.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(sosemanuk_ECRYPT)
