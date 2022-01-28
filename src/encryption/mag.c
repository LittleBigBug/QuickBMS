#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              mag_ECRYPT_ctx
#define ECRYPT_init             mag_ECRYPT_init
#define ECRYPT_keysetup         mag_ECRYPT_keysetup
#define ECRYPT_ivsetup          mag_ECRYPT_ivsetup
#define ECRYPT_process_bytes    mag_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    mag_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    mag_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  mag_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   mag_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   mag_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   mag_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   mag_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   mag_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   mag_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks mag_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/mag/v3/mag.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(mag_ECRYPT)
