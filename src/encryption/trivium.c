#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              trivium_ECRYPT_ctx
#define ECRYPT_init             trivium_ECRYPT_init
#define ECRYPT_keysetup         trivium_ECRYPT_keysetup
#define ECRYPT_ivsetup          trivium_ECRYPT_ivsetup
#define ECRYPT_process_bytes    trivium_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    trivium_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    trivium_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  trivium_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   trivium_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   trivium_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   trivium_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   trivium_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   trivium_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   trivium_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks trivium_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/trivium/trivium.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(trivium_ECRYPT)
