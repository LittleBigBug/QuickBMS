#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              wg_ECRYPT_ctx
#define ECRYPT_init             wg_ECRYPT_init
#define ECRYPT_keysetup         wg_ECRYPT_keysetup
#define ECRYPT_ivsetup          wg_ECRYPT_ivsetup
#define ECRYPT_process_bytes    wg_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    wg_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    wg_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  wg_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   wg_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   wg_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   wg_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   wg_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   wg_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   wg_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks wg_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/wg/v2/long-iv/wg.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(wg_ECRYPT)
